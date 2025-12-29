/**
 * Ejemplo básico de uso de la librería MicroFS
 * Crear, escribir, leer y eliminar archivos
 */

#include <stdint.h>
#include "microfs.h"

/* Buffer para lectura */
char buffer[128];

int main(void) {
    uint8_t result;
    uint16_t bytes_read;
    mfs_fileinfo_t info;
    uint8_t i;
    
    /* Montar filesystem */
    result = mfs_mount();
    
    if (result == MFS_ERR_NOFS) {
        /* No hay filesystem, formatear */
        result = mfs_format();
        if (result != MFS_OK) {
            while (1);  /* Error */
        }
        result = mfs_mount();
    }
    
    if (result != MFS_OK) {
        while (1);  /* Error de montaje */
    }
    
    /* Crear un archivo nuevo */
    mfs_delete("TEST.TXT");  /* Borrar si existe */
    result = mfs_create("TEST.TXT", 256);
    
    if (result == MFS_OK) {
        /* Escribir datos */
        mfs_write("Hello World!\r\n", 14);
        mfs_write("MicroFS Example\r\n", 17);
        mfs_close();
    }
    
    /* Abrir y leer el archivo */
    result = mfs_open("TEST.TXT");
    if (result == MFS_OK) {
        bytes_read = mfs_read(buffer, 100);
        buffer[bytes_read] = '\0';
        mfs_close();
        /* buffer contiene "Hello World!\r\nMicroFS Example\r\n" */
    }
    
    /* Listar todos los archivos */
    for (i = 0; i < MFS_MAX_FILES; i++) {
        if (mfs_list(i, &info) == MFS_OK) {
            /* info.name contiene el nombre */
            /* info.size contiene el tamaño */
        }
    }
    
    /* Eliminar archivo */
    mfs_delete("TEST.TXT");
    
    while (1);
    return 0;
}
