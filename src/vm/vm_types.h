// vm_types.h
#ifndef VM_TYPES_H
#define VM_TYPES_H

enum vm_type {
    VM_UNINIT = 0,  // Página no inicializada
    VM_ANON = 1,    // Página anónima
    VM_FILE = 2     // Página respaldada por archivo
};

typedef bool vm_initializer(struct page *page, enum vm_type type, void *kva);

#endif  // VM_TYPES_H

