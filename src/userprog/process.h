#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "filesys/off_t.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
// Declaración de la función lazy_load_segment



struct container {
    struct file *file;
    off_t offset;
    size_t page_read_bytes;
};

#endif /* userprog/process.h */
