// src/vm/anon.h

#ifndef VM_ANON_H
#define VM_ANON_H

#include "vm/vm.h"

struct anon_page {
    int swap_index;  // Índice de swap, inicialmente no válido
};

#endif  // VM_ANON_H

