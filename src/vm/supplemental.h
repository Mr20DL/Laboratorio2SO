// src/vm/supplemental.h

#ifndef VM_SUPPLEMENTAL_H
#define VM_SUPPLEMENTAL_H

#include "threads/synch.h"
#include "lib/kernel/hash.h"
#include "uninit.h"
#include "threads/thread.h"


/* Estructura que representa una p�gina en la tabla de p�ginas suplementaria. */
struct supplemental_page {
    void *va;                  /* Direcci�n virtual de la p�gina. */
    struct hash_elem hash_elem; /* Elemento para usar con la tabla hash. */
    bool writable;             /* Indica si la p�gina es escribible. */
    struct uninit_page uninit; /* Informaci�n de inicializaci�n de la p�gina (para p�ginas no inicializadas). */
    struct page_operations *operations; /* Operaciones asociadas a la p�gina. */
};

/* Estructura que representa la tabla de p�ginas suplementarias de un proceso. */
struct supplemental_page_table {
    struct hash pages; /* Tabla hash de p�ginas. */
    struct lock lock;  /* Bloqueo para proteger la tabla de p�ginas en acceso concurrente. */
};

/* Funciones para gestionar la tabla de p�ginas suplementarias. */

/* Inicializa la tabla de p�ginas suplementarias. */
void supplemental_page_table_init(struct supplemental_page_table *spt);

/* Destruye la tabla de p�ginas suplementarias. */
void supplemental_page_table_kill(struct supplemental_page_table *spt);

/* Copia la tabla de p�ginas suplementarias de un proceso a otro. */
bool supplemental_page_table_copy(struct supplemental_page_table *dst, 
                                  struct supplemental_page_table *src);

/* Busca una p�gina en la tabla de p�ginas suplementarias. */
struct supplemental_page *spt_find_page(struct supplemental_page_table *spt, void *va);

/* Inserta una p�gina en la tabla de p�ginas suplementarias. */
bool spt_insert_page(struct supplemental_page_table *spt, struct supplemental_page *page);

#endif /* VM_SUPPLEMENTAL_H */

