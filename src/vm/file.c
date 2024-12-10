// src/vm/file.c

#include "vm/vm.h"  // Aseg�rate de que vm.h se incluya correctamente
#include "vm/supplemental.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include <stdio.h>




// Definici�n correcta de la funci�n
bool file_backed_initializer(struct supplemental_page *page, enum vm_type type, void *kva) {
    struct uninit_page *uninit = &page->uninit;
    struct file *file = (struct file *)uninit->aux;

    // Leer el contenido del archivo en la p�gina (en memoria)
    file_read(file, kva, PGSIZE);

    // Asignar operaciones espec�ficas de un archivo
    page->operations = &file_ops;  // Aseg�rate de que 'file_ops' est� definido en alg�n lugar
    return true;
}

