// src/vm/supplemental.h

#ifndef VM_SUPPLEMENTAL_H
#define VM_SUPPLEMENTAL_H

#include "threads/synch.h"
#include "lib/kernel/hash.h"
#include "uninit.h"
#include "threads/thread.h"


/* Estructura que representa una página en la tabla de páginas suplementaria. */
struct supplemental_page {
    void *va;                  /* Dirección virtual de la página. */
    struct hash_elem hash_elem; /* Elemento para usar con la tabla hash. */
    bool writable;             /* Indica si la página es escribible. */
    struct uninit_page uninit; /* Información de inicialización de la página (para páginas no inicializadas). */
    struct page_operations *operations; /* Operaciones asociadas a la página. */
};

/* Estructura que representa la tabla de páginas suplementarias de un proceso. */
struct supplemental_page_table {
    struct hash pages; /* Tabla hash de páginas. */
    struct lock lock;  /* Bloqueo para proteger la tabla de páginas en acceso concurrente. */
};

/* Funciones para gestionar la tabla de páginas suplementarias. */

/* Inicializa la tabla de páginas suplementarias. */
void supplemental_page_table_init(struct supplemental_page_table *spt);

/* Destruye la tabla de páginas suplementarias. */
void supplemental_page_table_kill(struct supplemental_page_table *spt);

/* Copia la tabla de páginas suplementarias de un proceso a otro. */
bool supplemental_page_table_copy(struct supplemental_page_table *dst, 
                                  struct supplemental_page_table *src);

/* Busca una página en la tabla de páginas suplementarias. */
struct supplemental_page *spt_find_page(struct supplemental_page_table *spt, void *va);

/* Inserta una página en la tabla de páginas suplementarias. */
bool spt_insert_page(struct supplemental_page_table *spt, struct supplemental_page *page);

#endif /* VM_SUPPLEMENTAL_H */

