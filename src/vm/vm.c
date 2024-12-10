// src/vm/vm.c

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "vm/uninit.h"
#include "vm/file.h"
#include "vm/anon.h"
#include "userprog/process.h"
#include <stdio.h>
#include <stdlib.h>

/* Crea una página pendiente con el inicializador adecuado. */
bool
vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
                               vm_initializer *init, void *aux) {
    ASSERT(VM_TYPE(type) != VM_UNINIT);

    struct supplemental_page_table *spt = &thread_current()->spt;

    // Verificar si la página ya está ocupada
    if (spt_find_page(spt, upage) == NULL) {
        // Crear la página pendiente (uninitialized)
        struct page *page = (struct page *)malloc(sizeof(struct page));

        // Seleccionar el inicializador correcto según el tipo de página
        typedef bool (*initializerFunc)(struct page *, enum vm_type, void *);
        initializerFunc initializer = NULL;

        switch (VM_TYPE(type)) {
            case VM_ANON:
                initializer = anon_initializer;
                break;
            case VM_FILE:
                initializer = file_backed_initializer;
                break;
        }

        // Crear la página no inicializada (uninit) usando el inicializador adecuado
        uninit_new(page, upage, init, type, aux, initializer);

        // Establecer la página como escribible si es necesario
        page->writable = writable;

        // Insertar la página en la tabla de páginas complementaria (SPT)
        return spt_insert_page(spt, page);
    }

    // Si la página ya existe, devolver false
    return false;
}

bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
                         bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
    struct supplemental_page_table *spt UNUSED = &thread_current()->spt;

    if (is_kernel_vaddr(addr)) {
        return false;
    }

    void *rsp_stack = is_kernel_vaddr(f->rsp) ? thread_current()->rsp_stack : f->rsp;

    if (not_present) {
        if (!vm_claim_page(addr)) {
            if (rsp_stack - 8 <= addr && USER_STACK - 0x100000 <= addr && addr <= USER_STACK) {
                vm_stack_growth(thread_current()->stack_bottom - PGSIZE);
                return true;
            }
            return false;
        } else {
            return true;
        }
    }

    return false;
}

