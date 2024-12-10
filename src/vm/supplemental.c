// src/vm/supplemental.c

#include "vm/supplemental.h"
#include "vm/vm.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "hash.h"
#include "string.h"

/* Funci�n que copia la tabla de p�ginas suplementaria de un proceso padre a un hijo. */
bool supplemental_page_table_copy(struct supplemental_page_table *dst, 
                                  struct supplemental_page_table *src) {
    struct hash_iterator i;

    // Itera sobre todas las p�ginas en la tabla de p�ginas original
    hash_first(&i, &src->pages);
    while (hash_next(&i)) {
        struct page *parent_page = hash_entry(hash_cur(&i), struct page, hash_elem);
        enum vm_type type = page_get_type(parent_page);
        void *upage = parent_page->va;
        bool writable = parent_page->writable;
        vm_initializer *init = parent_page->uninit.init;
        void *aux = parent_page->uninit.aux;

        // Si la p�gina es no inicializada, debemos inicializarla en el hijo.
        if (parent_page->uninit.type & VM_MARKER_0) {
            // Si la p�gina tiene un marcador especial, realiza la inicializaci�n (ejemplo: stack).
            setup_stack(&thread_current()->tf);
        } else if (parent_page->operations->type == VM_UNINIT) {
            // Si la p�gina es no inicializada, asigna una nueva p�gina en el hijo.
            if (!vm_alloc_page_with_initializer(type, upage, writable, init, aux)) {
                return false;
            }
        } else {
            // Si la p�gina es de otro tipo (por ejemplo, an�nima o de archivo), asigna y reclama la p�gina.
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

/* Funci�n que destruye la tabla de p�ginas suplementaria de un proceso */
void supplemental_page_table_kill(struct supplemental_page_table *spt) {
    struct hash_iterator i;

    // Itera sobre todas las p�ginas en la tabla de p�ginas
    hash_first(&i, &spt->pages);
    while (hash_next(&i)) {
        struct page *page = hash_entry(hash_cur(&i), struct page, hash_elem);

        // Libera los recursos de la p�gina, dependiendo de su tipo
        if (page->operations->free != NULL) {
            page->operations->free(page);
        }

        // Libera la estructura de la p�gina
        free(page);
    }

    // Destruye la tabla de p�ginas (libera los recursos asociados)
    hash_destroy(&spt->pages, NULL);
}

/* Funci�n que inicializa la tabla de p�ginas suplementarias */
void supplemental_page_table_init(struct supplemental_page_table *spt) {
    hash_init(&spt->pages, page_hash, page_less, NULL);
    lock_init(&spt->lock);
}

/* Funci�n que busca una p�gina en la tabla de p�ginas suplementarias */
struct page *spt_find_page(struct supplemental_page_table *spt, void *va) {
    struct page p;
    struct hash_elem *e;

    p.va = va;
    e = hash_find(&spt->pages, &p.hash_elem);

    return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Funci�n que inserta una p�gina en la tabla de p�ginas suplementarias */
bool spt_insert_page(struct supplemental_page_table *spt, struct page *page) {
    struct hash_elem *e = hash_insert(&spt->pages, &page->hash_elem);
    return e == NULL;
}

