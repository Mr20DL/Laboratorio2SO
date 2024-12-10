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
#include "uninit.h"  // Incluir uninit.h despu�s para asegurar que enum vm_type ya est� definido




struct page;
struct thread;

/* Estructura que representa una p�gina en la memoria virtual */
struct page {
    void *va;                          // Direcci�n virtual de la p�gina
    bool writable;                     // �Es escribible esta p�gina?
    void *frame;                       // Marco f�sico donde est� cargada la p�gina (si existe)
    struct supplemental_page_table *spt; // Tabla de p�ginas complementaria
    struct uninit_page uninit;          // Informaci�n sobre p�ginas no inicializadas
    struct anon_page anon;             // Informaci�n sobre p�ginas an�nimas
    struct file_page file;             // Informaci�n sobre p�ginas respaldadas por archivo
    struct list_elem elem;             // Elemento de lista (para agregar a la tabla de p�ginas complementarias)
};

/* Estructura para la tabla de p�ginas complementarias */
struct supplemental_page_table {
    struct hash pages;  // Hash table de p�ginas virtuales
    struct lock lock;   // Lock para proteger el acceso a la tabla de p�ginas
};

/* Operaciones espec�ficas para cada tipo de p�gina */
struct page_operations {
    bool (*load)(struct page *page, void *kva);    // Funci�n para cargar la p�gina en memoria
    void (*free)(struct page *page);                // Funci�n para liberar la p�gina
};

/* Operaciones espec�ficas para p�ginas an�nimas */
extern struct page_operations anon_ops;
/* Operaciones espec�ficas para p�ginas respaldadas por archivos */
extern struct page_operations file_ops;

/* Inicializa una nueva p�gina */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
                                     vm_initializer *init, void *aux);

/* Inserta una p�gina en la tabla de p�ginas complementarias */
bool spt_insert_page(struct supplemental_page_table *spt, struct page *page);

/* Encuentra una p�gina en la tabla de p�ginas complementarias */
struct page *spt_find_page(struct supplemental_page_table *spt, void *va);

/* Reclama una p�gina (si est� disponible) */
bool vm_claim_page(void *va);

/* Libera una p�gina */
void vm_free_page(struct page *page);

#endif  // VM_VM_H

