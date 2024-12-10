// src/vm/file.c

#include "vm/vm.h"  // Asegúrate de que vm.h se incluya correctamente
#include "vm/supplemental.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include <stdio.h>




// Definición correcta de la función
bool file_backed_initializer(struct supplemental_page *page, enum vm_type type, void *kva) {
    struct uninit_page *uninit = &page->uninit;
    struct file *file = (struct file *)uninit->aux;

    // Leer el contenido del archivo en la página (en memoria)
    file_read(file, kva, PGSIZE);

    // Asignar operaciones específicas de un archivo
    page->operations = &file_ops;  // Asegúrate de que 'file_ops' esté definido en algún lugar
    return true;
}

