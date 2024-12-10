// src/vm/vm.h

#ifndef VM_VM_H
#define VM_VM_H

#include <stdbool.h>
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "anon.h"
#include "file.h"
#include "lib/kernel/hash.h"
#include "vm_types.h"
#include "uninit.h"  // Incluir uninit.h después para asegurar que enum vm_type ya esté definido




struct page;
struct thread;

/* Estructura que representa una página en la memoria virtual */
struct page {
    void *va;                          // Dirección virtual de la página
    bool writable;                     // ¿Es escribible esta página?
    void *frame;                       // Marco físico donde está cargada la página (si existe)
    struct supplemental_page_table *spt; // Tabla de páginas complementaria
    struct uninit_page uninit;          // Información sobre páginas no inicializadas
    struct anon_page anon;             // Información sobre páginas anónimas
    struct file_page file;             // Información sobre páginas respaldadas por archivo
    struct list_elem elem;             // Elemento de lista (para agregar a la tabla de páginas complementarias)
};

/* Estructura para la tabla de páginas complementarias */
struct supplemental_page_table {
    struct hash pages;  // Hash table de páginas virtuales
    struct lock lock;   // Lock para proteger el acceso a la tabla de páginas
};

/* Operaciones específicas para cada tipo de página */
struct page_operations {
    bool (*load)(struct page *page, void *kva);    // Función para cargar la página en memoria
    void (*free)(struct page *page);                // Función para liberar la página
};

/* Operaciones específicas para páginas anónimas */
extern struct page_operations anon_ops;
/* Operaciones específicas para páginas respaldadas por archivos */
extern struct page_operations file_ops;

/* Inicializa una nueva página */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
                                     vm_initializer *init, void *aux);

/* Inserta una página en la tabla de páginas complementarias */
bool spt_insert_page(struct supplemental_page_table *spt, struct page *page);

/* Encuentra una página en la tabla de páginas complementarias */
struct page *spt_find_page(struct supplemental_page_table *spt, void *va);

/* Reclama una página (si está disponible) */
bool vm_claim_page(void *va);

/* Libera una página */
void vm_free_page(struct page *page);

#endif  // VM_VM_H

