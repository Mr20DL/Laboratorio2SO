#ifndef VM_UNINIT_H 
#define VM_UNINIT_H 


#include "vm_types.h"

struct page;  // Forward declaration of struct page

/* Estructura que representa una página no inicializada */ 
struct uninit_page { 
    vm_initializer *init;      
    enum vm_type type;         
    void *aux;                 
    bool (*page_initializer)(struct page *, enum vm_type, void *);  
}; 

/* Función que crea una nueva página no inicializada */ 
void uninit_new(struct page *page, void *va, vm_initializer *init, 
                 enum vm_type type, void *aux, 
                 bool (*initializer)(struct page *, enum vm_type, void *)); 

#endif  // VM_UNINIT_H 
