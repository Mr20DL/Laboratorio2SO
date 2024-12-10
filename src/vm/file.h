// src/vm/file.h

#ifndef VM_FILE_H
#define VM_FILE_H

#include "vm/vm.h"
#include "filesys/file.h"

// Definici�n de las operaciones espec�ficas para p�ginas respaldadas por archivos.
extern struct page_operations file_ops;

/* Inicializa una p�gina respaldada por archivo */
extern bool file_backed_initializer(struct page *page, enum vm_type type, void *kva);

/* Estructura para p�ginas respaldadas por archivos */
struct file_page {
    struct file *file;      // Archivo asociado con la p�gina
    off_t offset;           // Desplazamiento dentro del archivo
    uint32_t read_bytes;    // Cantidad de bytes a leer
    uint32_t zero_bytes;    // Cantidad de bytes a inicializar a cero
    bool writable;          // �Es escribible la p�gina?
};

#endif  // VM_FILE_H

