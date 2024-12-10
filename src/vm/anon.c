// src/vm/anon.c

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/anon.h"
#include "vm/swap.h"
#include <stdio.h>
#include <string.h>

/* Inicializa una página anónima */
bool
anon_initializer(struct page *page, enum vm_type type, void *kva) {
    struct uninit_page *uninit = &page->uninit;
    memset(uninit, 0, sizeof(struct uninit_page));

    // Asignar la operación de la página como anon_ops
    page->operations = &anon_ops;

    struct anon_page *anon_page = &page->anon;
    anon_page->swap_index = -1;  // Inicializar swap index a un valor no válido

    return true;
}

