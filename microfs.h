/**
 * MICROFS.H - Sistema de archivos minimalista para 6502
 * 
 * Formato:
 *   Sector 0: 16 entradas de archivo (32 bytes c/u)
 *   Sector 1+: Datos contiguos
 * 
 * Lectura y escritura desde 6502
 * Tamaño código: ~1.2KB
 */

#ifndef MICROFS_H
#define MICROFS_H

#include <stdint.h>

/* Constantes */
#define MFS_MAX_FILES   16
#define MFS_NAME_LEN    12
#define MFS_MAGIC       0x4D46

/* Errores */
#define MFS_OK          0
#define MFS_ERR_DISK    1
#define MFS_ERR_NOFS    2
#define MFS_ERR_NOTFOUND 3
#define MFS_ERR_FULL    4
#define MFS_ERR_EXISTS  5

/* Info de archivo */
typedef struct {
    char     name[MFS_NAME_LEN];
    uint16_t size;
    uint8_t  index;
} mfs_fileinfo_t;

/* Montar filesystem */
uint8_t mfs_mount(void);

/* Formatear (borra todo) */
uint8_t mfs_format(void);

/* Abrir archivo existente */
uint8_t mfs_open(const char *name);

/* Crear archivo nuevo (reserva sectores) */
uint8_t mfs_create(const char *name, uint16_t size);

/* Leer bytes (retorna bytes leídos) */
uint16_t mfs_read(void *buf, uint16_t len);

/* Escribir bytes (retorna bytes escritos) */
uint16_t mfs_write(const void *buf, uint16_t len);

/* Cerrar archivo (guarda cambios) */
void mfs_close(void);

/* Eliminar archivo por nombre */
uint8_t mfs_delete(const char *name);

/* Listar archivo por índice */
uint8_t mfs_list(uint8_t index, mfs_fileinfo_t *info);

#endif
