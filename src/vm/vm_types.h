// vm_types.h
#ifndef VM_TYPES_H
#define VM_TYPES_H

enum vm_type {
    VM_UNINIT = 0,  // P�gina no inicializada
    VM_ANON = 1,    // P�gina an�nima
    VM_FILE = 2     // P�gina respaldada por archivo
};

typedef bool vm_initializer(struct page *page, enum vm_type type, void *kva);

#endif  // VM_TYPES_H

