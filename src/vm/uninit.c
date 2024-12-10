// src/vm/uninit.c

#include "vm.h"
#include "uninit.h"
#include "threads/vaddr.h"
#include "vm/anon.h"
#include "vm/file.h"
#include "userprog/process.h"
#include <stdio.h>
#include <string.h>

void
uninit_new(struct page *page, void *va, vm_initializer *init, 
           enum vm_type type, void *aux, 
           bool (*initializer)(struct page *, enum vm_type, void *)) {
    ASSERT(page != NULL);

    *page = (struct page) {
        .operations = &uninit_ops,
        .va = va,
        .frame = NULL,  // No tiene frame por ahora
        .uninit = (struct uninit_page) {
            .init = init,
            .type = type,
            .aux = aux,
            .page_initializer = initializer,
        }
    };
}

