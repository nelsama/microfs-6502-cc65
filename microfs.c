/**
 * MICROFS.C - Sistema de archivos minimalista para 6502
 * 
 * Funciones de archivo en C puro.
 * Helpers de string en ASM (microfs_asm.s) para optimización.
 */

#include "microfs.h"
#include "sdcard.h"
#include <string.h>

/* Estado archivo abierto - ANTES de los buffers para evitar corrupción */
uint8_t  f_open;
uint8_t  f_dirty;        /* Sector modificado */
uint8_t  f_idx;          /* Índice en tabla */
uint16_t f_start;
uint16_t f_size;
uint16_t f_pos;
uint16_t f_sector;
uint16_t f_offset;

/* Buffers en zona alta de RAM para no interferir con programas en $0400+ */
/* filetab: $3800-$39FF (512 bytes) */
/* secbuf:  $3A00-$3BFF (512 bytes) */
/* Programas usuario: $0400-$37FF (~13KB) */
uint8_t * const filetab = (uint8_t *)0x3800;
uint8_t * const secbuf  = (uint8_t *)0x3A00;

/* Funciones en ASM (microfs_asm.s) */
extern uint8_t mfs_streq(const char *a, const char *b);
extern void mfs_strcpy_n(char *d, const char *s, uint8_t n);
/* mfs_close está en ASM */

/* Guardar tabla de archivos */
static uint8_t save_filetab(void) {
    return sd_write_sector(0, filetab);
}

uint8_t mfs_mount(void) {
    f_open = 0;
    if (sd_init()) return MFS_ERR_DISK;
    if (sd_read_sector(0, filetab)) return MFS_ERR_DISK;
    if (filetab[0] != 0x46 || filetab[1] != 0x4D) return MFS_ERR_NOFS;
    return MFS_OK;
}

uint8_t mfs_format(void) {
    f_open = 0;
    if (sd_init()) return MFS_ERR_DISK;
    
    /* Limpiar tabla con memset (más eficiente que loop) */
    memset(filetab, 0, 512);
    
    /* Magic "FM" (0x46, 0x4D) y next_sector=1 */
    filetab[0] = 0x46;  /* 'F' */
    filetab[1] = 0x4D;  /* 'M' */
    filetab[6] = 1;     /* next_sector low */
    filetab[7] = 0;     /* next_sector high */
    
    return save_filetab() ? MFS_ERR_DISK : MFS_OK;
}

uint8_t mfs_open(const char *name) {
    uint8_t i;
    uint8_t *p;
    
    mfs_close();
    
    for (i = 0; i < MFS_MAX_FILES; i++) {
        p = filetab + 16 + (i * 32);
        if (p[0] == 0) continue;
        
        if (mfs_streq((char*)p, name)) {
            f_idx = i;
            f_start = p[12] | ((uint16_t)p[13] << 8);
            f_size = p[14] | ((uint16_t)p[15] << 8);
            f_pos = 0;
            f_sector = f_start;
            f_offset = 512;
            f_dirty = 0;
            f_open = 1;
            return MFS_OK;
        }
    }
    return MFS_ERR_NOTFOUND;
}

uint8_t mfs_create(const char *name, uint16_t size) {
    uint8_t i, slot = 0xFF;
    uint8_t *p;
    uint16_t next_sector, sectors;
    
    mfs_close();
    
    /* Buscar slot libre y verificar que no existe */
    for (i = 0; i < MFS_MAX_FILES; i++) {
        p = filetab + 16 + (i * 32);
        if (p[0] == 0) {
            if (slot == 0xFF) slot = i;
        } else if (mfs_streq((char*)p, name)) {
            return MFS_ERR_EXISTS;
        }
    }
    
    if (slot == 0xFF) return MFS_ERR_FULL;
    
    /* Obtener próximo sector libre */
    next_sector = filetab[6] | ((uint16_t)filetab[7] << 8);
    sectors = (size + 511) / 512;
    if (sectors == 0) sectors = 1;
    
    /* Crear entrada */
    p = filetab + 16 + (slot * 32);
    mfs_strcpy_n((char*)p, name, MFS_NAME_LEN - 1);
    p[12] = next_sector & 0xFF;
    p[13] = next_sector >> 8;
    p[14] = size & 0xFF;
    p[15] = size >> 8;
    p[16] = sectors & 0xFF;
    p[17] = sectors >> 8;
    
    /* Actualizar próximo sector */
    next_sector += sectors;
    filetab[6] = next_sector & 0xFF;
    filetab[7] = next_sector >> 8;
    
    if (save_filetab()) return MFS_ERR_DISK;
    
    /* Limpiar sectores del archivo (inicializar a 0) */
    memset(secbuf, 0, 512);
    for (i = 0; i < sectors; i++) {
        uint16_t sect = (p[12] | ((uint16_t)p[13] << 8)) + i;
        sd_write_sector(sect, secbuf);
    }
    
    /* Abrir el archivo creado */
    return mfs_open(name);
}

uint16_t mfs_read(void *buf, uint16_t len) {
    uint8_t *dst = (uint8_t *)buf;
    uint16_t total = 0;
    uint16_t chunk;
    uint16_t need_sector;
    
    if (!f_open) return 0;
    
    while (len && f_pos < f_size) {
        /* Calcular qué sector necesitamos */
        need_sector = f_start + (f_pos / 512);
        
        /* Cargar sector si es diferente al actual o primera lectura */
        if (f_offset >= 512 || f_sector != need_sector) {
            /* Guardar sector anterior si fue modificado */
            if (f_dirty) {
                sd_write_sector(f_sector, secbuf);
                f_dirty = 0;
            }
            f_sector = need_sector;
            sd_read_sector(f_sector, secbuf);
            f_offset = f_pos % 512;
        }
        
        /* Calcular cuántos bytes leer de este sector */
        chunk = 512 - f_offset;
        if (chunk > len) chunk = len;
        if (f_pos + chunk > f_size) chunk = f_size - f_pos;
        
        /* Copiar datos del buffer */
        memcpy(dst, secbuf + f_offset, chunk);
        dst += chunk;
        f_offset += chunk;
        f_pos += chunk;
        len -= chunk;
        total += chunk;
    }
    return total;
}

uint16_t mfs_write(const void *buf, uint16_t len) {
    const uint8_t *src = (const uint8_t *)buf;
    uint16_t total = 0;
    uint16_t chunk;
    uint16_t need_sector;
    
    if (!f_open) return 0;
    
    while (len && f_pos < f_size) {
        /* Calcular qué sector necesitamos */
        need_sector = f_start + (f_pos / 512);
        
        /* Cargar sector si es diferente al actual o primera escritura */
        if (f_offset >= 512 || f_sector != need_sector) {
            /* Guardar sector anterior si fue modificado */
            if (f_dirty) {
                sd_write_sector(f_sector, secbuf);
                f_dirty = 0;
            }
            f_sector = need_sector;
            sd_read_sector(f_sector, secbuf);
            f_offset = f_pos % 512;
        }
        
        /* Calcular cuántos bytes escribir en este sector */
        chunk = 512 - f_offset;
        if (chunk > len) chunk = len;
        if (f_pos + chunk > f_size) chunk = f_size - f_pos;
        
        /* Copiar datos al buffer */
        memcpy(secbuf + f_offset, src, chunk);
        src += chunk;
        f_offset += chunk;
        f_pos += chunk;
        len -= chunk;
        total += chunk;
        f_dirty = 1;
    }
    return total;
}

/* Cerrar archivo y guardar cambios */
void mfs_close(void) {
    if (!f_open) return;
    if (f_dirty) {
        sd_write_sector(f_sector, secbuf);
    }
    f_open = 0;
    f_dirty = 0;
}

/* Eliminar archivo por nombre */
uint8_t mfs_delete(const char *name) {
    uint8_t i;
    uint8_t *p;
    
    mfs_close();
    
    for (i = 0; i < MFS_MAX_FILES; i++) {
        p = filetab + 16 + (i * 32);
        if (p[0] == 0) continue;
        
        if (mfs_streq((char*)p, name)) {
            p[0] = 0;  /* Marcar como eliminado */
            return save_filetab() ? MFS_ERR_DISK : MFS_OK;
        }
    }
    return MFS_ERR_NOTFOUND;
}

uint8_t mfs_list(uint8_t index, mfs_fileinfo_t *info) {
    uint8_t *p;
    uint8_t i;
    
    if (index >= MFS_MAX_FILES) return MFS_ERR_NOTFOUND;
    
    p = filetab + 16 + (index * 32);
    if (p[0] == 0) return MFS_ERR_NOTFOUND;
    
    for (i = 0; i < MFS_NAME_LEN - 1 && p[i]; i++) {
        info->name[i] = p[i];
    }
    info->name[i] = 0;
    info->size = p[14] | ((uint16_t)p[15] << 8);
    info->index = index;
    
    return MFS_OK;
}

uint16_t mfs_get_size(void) {
    return f_size;
}
