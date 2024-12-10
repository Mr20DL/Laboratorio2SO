// src/vm/supplemental.c

#include "vm/supplemental.h"
#include "vm/vm.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "hash.h"
#include "string.h"

/* Función que copia la tabla de páginas suplementaria de un proceso padre a un hijo. */
bool supplemental_page_table_copy(struct supplemental_page_table *dst, 
                                  struct supplemental_page_table *src) {
    struct hash_iterator i;

    // Itera sobre todas las páginas en la tabla de páginas original
    hash_first(&i, &src->pages);
    while (hash_next(&i)) {
        struct page *parent_page = hash_entry(hash_cur(&i), struct page, hash_elem);
        enum vm_type type = page_get_type(parent_page);
        void *upage = parent_page->va;
        bool writable = parent_page->writable;
        vm_initializer *init = parent_page->uninit.init;
        void *aux = parent_page->uninit.aux;

        // Si la página es no inicializada, debemos inicializarla en el hijo.
        if (parent_page->uninit.type & VM_MARKER_0) {
            // Si la página tiene un marcador especial, realiza la inicialización (ejemplo: stack).
            setup_stack(&thread_current()->tf);
        } else if (parent_page->operations->type == VM_UNINIT) {
            // Si la página es no inicializada, asigna una nueva página en el hijo.
            if (!vm_alloc_page_with_initializer(type, upage, writable, init, aux)) {
                return false;
            }
        } else {
            // Si la página es de otro tipo (por ejemplo, anónima o de archivo), asigna y reclama la página.
            if (!vm_alloc_page(type, upage, writable)) {
                return false;
            }
            if (!vm_claim_page(upage)) {
                return false;
            }
        }
    }
    
    return true;
}

/* Función que destruye la tabla de páginas suplementaria de un proceso */
void supplemental_page_table_kill(struct supplemental_page_table *spt) {
    struct hash_iterator i;

    // Itera sobre todas las páginas en la tabla de páginas
    hash_first(&i, &spt->pages);
    while (hash_next(&i)) {
        struct page *page = hash_entry(hash_cur(&i), struct page, hash_elem);

        // Libera los recursos de la página, dependiendo de su tipo
        if (page->operations->free != NULL) {
            page->operations->free(page);
        }

        // Libera la estructura de la página
        free(page);
    }

    // Destruye la tabla de páginas (libera los recursos asociados)
    hash_destroy(&spt->pages, NULL);
}

/* Función que inicializa la tabla de páginas suplementarias */
void supplemental_page_table_init(struct supplemental_page_table *spt) {
    hash_init(&spt->pages, page_hash, page_less, NULL);
    lock_init(&spt->lock);
}

/* Función que busca una página en la tabla de páginas suplementarias */
struct page *spt_find_page(struct supplemental_page_table *spt, void *va) {
    struct page p;
    struct hash_elem *e;

    p.va = va;
    e = hash_find(&spt->pages, &p.hash_elem);

    return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Función que inserta una página en la tabla de páginas suplementarias */
bool spt_insert_page(struct supplemental_page_table *spt, struct page *page) {
    struct hash_elem *e = hash_insert(&spt->pages, &page->hash_elem);
    return e == NULL;
}

