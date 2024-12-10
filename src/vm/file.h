// src/vm/file.h

#ifndef VM_FILE_H
#define VM_FILE_H

#include "vm/vm.h"
#include "filesys/file.h"

// Definición de las operaciones específicas para páginas respaldadas por archivos.
extern struct page_operations file_ops;

/* Inicializa una página respaldada por archivo */
extern bool file_backed_initializer(struct page *page, enum vm_type type, void *kva);

/* Estructura para páginas respaldadas por archivos */
struct file_page {
    struct file *file;      // Archivo asociado con la página
    off_t offset;           // Desplazamiento dentro del archivo
    uint32_t read_bytes;    // Cantidad de bytes a leer
    uint32_t zero_bytes;    // Cantidad de bytes a inicializar a cero
    bool writable;          // ¿Es escribible la página?
};

#endif  // VM_FILE_H

