// src/vm/anon.h

#ifndef VM_ANON_H
#define VM_ANON_H

#include "vm/vm.h"

struct anon_page {
    int swap_index;  // �ndice de swap, inicialmente no v�lido
};

#endif  // VM_ANON_H

