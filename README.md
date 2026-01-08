# microfs-6502-cc65

Sistema de archivos minimalista para sistemas 6502 con cc65.

## Características

- Máximo 16 archivos
- Nombres formato 8.3 (12 caracteres)
- Operaciones: crear, leer, escribir, eliminar, listar
- Compatible con herramienta Python para acceso desde PC
- Buffers en zona alta de RAM ($3800-$3BFF) para no interferir con programas en $0400+
- Tamaño: ~2.8 KB

## Mapa de Memoria

```
$0400-$37FF  Área libre para programas usuario (~13KB)
$3800-$39FF  filetab - Tabla de archivos (512 bytes)
$3A00-$3BFF  secbuf  - Buffer de sector (512 bytes)
```

Los buffers están en direcciones fijas para evitar conflictos con programas 
cargados en $0400. Si necesitas cambiar estas direcciones, modifica en `microfs.c`:

```c
uint8_t * const filetab = (uint8_t *)0x3800;
uint8_t * const secbuf  = (uint8_t *)0x3A00;
```

## Instalación

```bash
git clone https://github.com/nelsama/microfs-6502-cc65.git libs/microfs-6502-cc65
```

## Dependencias

Esta librería requiere:
- [sdcard-spi-6502-cc65](https://github.com/nelsama/sdcard-spi-6502-cc65)
- [spi-6502-cc65](https://github.com/nelsama/spi-6502-cc65)

## API

```c
#include "microfs.h"

// Sistema
uint8_t mfs_mount(void);                              // Montar
uint8_t mfs_format(void);                             // Formatear

// Archivos
uint8_t mfs_create(const char *name, uint16_t size);  // Crear y abrir
uint8_t mfs_open(const char *name);                   // Abrir existente
uint16_t mfs_read(void *buf, uint16_t len);           // Leer bytes
uint16_t mfs_write(const void *buf, uint16_t len);    // Escribir bytes
void mfs_close(void);                                  // Cerrar
uint8_t mfs_delete(const char *name);                 // Eliminar
uint16_t mfs_get_size(void);                          // Tamaño archivo abierto

// Listado
uint8_t mfs_list(uint8_t index, mfs_fileinfo_t *info);
```

## Códigos de Error

```c
MFS_OK           0   // Éxito
MFS_ERR_DISK     1   // Error de disco
MFS_ERR_NOFS     2   // No hay filesystem
MFS_ERR_NOTFOUND 3   // Archivo no encontrado
MFS_ERR_FULL     4   // Tabla llena (16 archivos)
MFS_ERR_EXISTS   5   // Archivo ya existe
```

## Ejemplo

```c
#include "microfs.h"

char buffer[64];

int main(void) {
    // Montar (formatear si necesario)
    if (mfs_mount() == MFS_ERR_NOFS) {
        mfs_format();
        mfs_mount();
    }
    
    // Crear archivo
    mfs_delete("HELLO.TXT");
    if (mfs_create("HELLO.TXT", 64) == MFS_OK) {
        mfs_write("Hello World!\r\n", 14);
        mfs_close();
    }
    
    // Leer archivo
    if (mfs_open("HELLO.TXT") == MFS_OK) {
        uint16_t n = mfs_read(buffer, 60);
        buffer[n] = '\0';
        mfs_close();
    }
    
    return 0;
}
```

## Formato de Disco

```
Sector 0: Tabla de archivos (512 bytes)
├── Bytes 0-1:   Magic "FM" (0x46, 0x4D)
├── Bytes 6-7:   Próximo sector libre
└── Bytes 16-527: 16 entradas × 32 bytes

Entrada de archivo (32 bytes):
├── Bytes 0-11:  Nombre (null-terminated)
├── Bytes 12-13: Sector inicial
├── Bytes 14-15: Tamaño en bytes
└── Bytes 16-17: Sectores reservados

Sector 1+: Datos de archivos (contiguos)
```

## Herramienta PC (Python)

```bash
# Listar archivos
python microfs.py /dev/sdX list

# Leer archivo
python microfs.py /dev/sdX read HELLO.TXT

# Escribir archivo
python microfs.py /dev/sdX write HELLO.TXT "contenido"

# Formatear
python microfs.py /dev/sdX format
```

## Integración con Makefile

```makefile
MICROFS_DIR = libs/microfs-6502-cc65
INCLUDES += -I$(MICROFS_DIR)

$(BUILD_DIR)/microfs.o: $(MICROFS_DIR)/microfs.c
	$(CL65) -t none $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/microfs_asm.o: $(MICROFS_DIR)/microfs_asm.s
	$(CA65) -t none -o $@ $<
```

## Archivos

| Archivo | Descripción |
|---------|-------------|
| `microfs.h` | Header con API |
| `microfs.c` | Implementación principal |
| `microfs_asm.s` | Helpers optimizados |
| `examples/` | Ejemplos de uso |

## Limitaciones

- Máximo 16 archivos
- Sin subdirectorios
- Archivos contiguos (sin fragmentación)
- Tamaño máximo: 65535 bytes por archivo
- Sin timestamps
- RAM requerida: 1024 bytes fijos ($3800-$3BFF)

## Changelog

### v1.1.0
- **Fix**: Corregido bug en `mfs_read()` y `mfs_write()` que fallaba al leer/escribir archivos mayores a 512 bytes
- **Cambio**: Buffers movidos a zona alta de RAM ($3800-$3BFF) para no interferir con programas en $0400+
- **Optimización**: Uso de `memcpy()` en lugar de copia byte a byte

### v1.0.0
- Versión inicial

## Licencia

MIT License - ver [LICENSE](LICENSE)
