#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "synch.h"
#include <hash.h>

enum thread_status
  {
    THREAD_RUNNING,     
    THREAD_READY,     
    THREAD_BLOCKED,   
    THREAD_DYING        
  };

typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          

#define PRI_MIN 0                      
#define PRI_DEFAULT 31                  
#define PRI_MAX 63                      

struct pcb
  {
    int exit_code;
    bool is_exited;
    bool is_loaded;

    struct file **fd_table;
    int fd_count;
    struct file *file_ex;

    struct semaphore sema_wait;
    struct semaphore sema_load;
  };

struct thread
  {
    tid_t tid;                         
    enum thread_status status;         
    char name[16];                      
    uint8_t *stack;                    
    int priority;                      
    struct list_elem allelem;          

    struct list_elem elem;             

#ifdef USERPROG
    uint32_t *pagedir;                 
    struct pcb *pcb;                   

    struct thread *parent_process;
    struct list list_child_process;
    struct list_elem elem_child_process;
#endif

    struct hash spt;

    unsigned magic;                  
  };

extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

struct pcb *get_child_pcb (tid_t child_tid);
struct thread *get_child_thread (tid_t child_tid);

#endif /* threads/thread.h */