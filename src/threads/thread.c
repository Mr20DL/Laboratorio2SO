#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/fixed_point.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif
#include "filesys/file.h"
#include "vm/page.h"


#define THREAD_MAGIC 0xcd6abf4b


static struct list ready_list;


static struct list all_list;


static struct list sleep_list;


static struct thread *idle_thread;


static struct thread *initial_thread;


static struct lock tid_lock;


struct kernel_thread_frame 
  {
    void *eip;                  
    thread_func *function;     
    void *aux;                 
  };


static long long idle_ticks;    
static long long kernel_ticks; 
static long long user_ticks;    


#define TIME_SLICE 4           
static unsigned thread_ticks;   


bool thread_mlfqs;
int load_avg;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);


void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);
  list_init (&sleep_list);


  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();

  initial_thread->nice = 0;
  initial_thread->recent_cpu = 0;
}


void
thread_start (void) 
{

  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);
  load_avg = 0;


  intr_enable ();


  sema_down (&idle_started);
}


void
thread_tick (void) 
{
  struct thread *t = thread_current ();


  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;


  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();
}


void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}


bool 
compare_thread_awake_tick (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
    return list_entry (a, struct thread, elem)->awake_tick
      < list_entry (b, struct thread, elem)->awake_tick;
}


bool 
compare_thread_priority (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
    return list_entry (a, struct thread, elem)->priority
      > list_entry (b, struct thread, elem)->priority;
}


tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);


  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;


  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();


  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;


  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;


  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  t->parent_process = thread_current ();

  t->pcb = palloc_get_page (0);

  if (t->pcb == NULL) {
    return TID_ERROR;
  }

  t->pcb->fd_table = palloc_get_page (PAL_ZERO);

  if (t->pcb->fd_table == NULL) {
    palloc_free_page (t->pcb);
    return TID_ERROR;
  }

  t->pcb->fd_count = 2;
  t->pcb->file_ex = NULL;
  t->pcb->exit_code = -1;
  t->pcb->is_exited = false;
  t->pcb->is_loaded = false;

  sema_init (&(t->pcb->sema_wait), 0);
  sema_init (&(t->pcb->sema_load), 0);

  list_push_back (&(t->parent_process->list_child_process), &(t->elem_child_process));

  init_spt (&t->spt);

  list_init (&t->mmf_list);
  t->mapid = 0;


  thread_unblock (t);


  thread_preepmt();

  return tid;
}


void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}


void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
  list_insert_ordered (&ready_list, &t->elem, compare_thread_priority, NULL);
  t->status = THREAD_READY;
  intr_set_level (old_level);
}


void
thread_sleep (int64_t ticks)
{
  enum intr_level old_level = intr_disable ();
  struct thread *t = thread_current ();

  ASSERT (is_thread (t));
  ASSERT (t != idle_thread);

  t->awake_tick = ticks;

  list_insert_ordered (&sleep_list, &t->elem, compare_thread_awake_tick, 0); 

  thread_block ();

  intr_set_level (old_level);
}


void
thread_awake (int64_t ticks)
{
  struct list_elem *elem_t;
  struct thread *t;

  for (elem_t = list_begin (&sleep_list); elem_t != list_end (&sleep_list);) {
    t = list_entry (elem_t, struct thread, elem);

    if (t->awake_tick > ticks) { break; }

    elem_t = list_remove (elem_t);
    thread_unblock (t);
  }
}


void
thread_preepmt (void)
{
  if (list_empty (&ready_list)) { return; }

  struct thread *t = thread_current ();
  struct thread *ready_list_t = list_entry (
    list_front (&ready_list), struct thread, elem
  );

  if (t->priority < ready_list_t->priority) {
    thread_yield (); 
  }
}


const char *
thread_name (void) 
{
  return thread_current ()->name;
}


struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  

  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}


tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}


void
thread_exit (void) 
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif


  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}


void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    list_insert_ordered (&ready_list, &cur->elem, compare_thread_priority, NULL);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}


void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}


void
thread_set_priority (int new_priority) 
{
  if (thread_mlfqs) 
    return;

  thread_current ()->init_priority = new_priority;

  update_priority ();
  thread_preepmt ();
}


int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}


void
mlfqs_priority (struct thread *t)
{
  if (t == idle_thread)
    return; 
  
  int priority = fp_to_int_round (add_fp_int (div_fp_int (t->recent_cpu, -4), PRI_MAX - t->nice * 2));

  if (priority > PRI_MAX) {
    t->priority = PRI_MAX;
  }
  else if (priority < PRI_MIN) {
    t->priority = PRI_MIN;
  } else 
    t->priority = priority;
}


void
mlfqs_recent_cpu (struct thread *t)
{
  t->recent_cpu = add_fp_int (mult_fp (div_fp (mult_fp_int (load_avg, 2), add_fp_int (mult_fp_int (load_avg, 2), 1)), t->recent_cpu), t->nice);
}


void
mlfqs_load_avg (void)
{
  int ready_threads = list_size (&ready_list);
  if (thread_current () != idle_thread) 
    ready_threads++;
    
  load_avg = add_fp (mult_fp (div_fp_int (int_to_fp (59), 60), load_avg), 
              mult_fp_int (div_fp_int (int_to_fp (1), 60), ready_threads));
}


void 
incr_recent_cpu (void)
{
  struct thread *t = thread_current ();
  if (t != idle_thread) 
    t ->recent_cpu = add_fp_int (thread_current ()->recent_cpu, 1);
}


void 
mlfqs_update_recent_cpu (void)
{
  struct list_elem *e;

  for (e = list_begin (&all_list); e != list_end (&all_list); e = list_next (e)) {
    struct thread *t = list_entry (e, struct thread, allelem);
    mlfqs_recent_cpu (t);
  }
}


void 
mlfqs_update_priority (void)
{
  struct list_elem *e;

  for (e = list_begin (&all_list); e != list_end (&all_list); e = list_next (e)) {
    struct thread *t = list_entry (e, struct thread, allelem);
    mlfqs_priority (t);
  }

  list_sort (&ready_list, compare_thread_priority, NULL);
}


void
thread_set_nice (int nice UNUSED) 
{
    enum intr_level old_level;
    old_level = intr_disable();
    
    struct thread *t = thread_current();
    t->nice = nice;
    mlfqs_priority (t);

    list_sort (&ready_list, compare_thread_priority, NULL);

    if (t != idle_thread)
      thread_preepmt ();
    
    intr_set_level(old_level);
}


int
thread_get_nice (void) 
{
  enum intr_level old_level;
  old_level = intr_disable();
  
  int ret_nice = thread_current ()->nice;
  
  intr_set_level(old_level);
  
  return ret_nice;
}


int
thread_get_load_avg (void) 
{
  enum intr_level old_level;
  old_level = intr_disable ();

  int ret_load_avg = fp_to_int_round (mult_fp_int (load_avg, 100));

  intr_set_level (old_level);
  return ret_load_avg;
}


int
thread_get_recent_cpu (void) 
{
  enum intr_level old_level;
  old_level = intr_disable ();

  int ret_recent_cpu = fp_to_int_round (mult_fp_int (thread_current ()->recent_cpu, 100));

  intr_set_level (old_level);
  return ret_recent_cpu;
}

static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {

      intr_disable ();
      thread_block ();

      asm volatile ("sti; hlt" : : : "memory");
    }
}


static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();     
  function (aux);   
  thread_exit ();    
}

struct thread *
running_thread (void) 
{
  uint32_t *esp;


  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}


static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}


static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->init_priority = priority;
  t->released_lock = NULL;
  t->magic = THREAD_MAGIC;
  
  t->recent_cpu = 0;
  t->nice = 0;
  
  list_init (&t->donations);
  
  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);

  list_init(&(t->list_child_process));
}


static void *
alloc_frame (struct thread *t, size_t size) 
{

  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}


static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}


void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);


  cur->status = THREAD_RUNNING;


  thread_ticks = 0;

#ifdef USERPROG

  process_activate ();
#endif


  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);

    }
}

void
update_priority (void)
{
  struct thread *t = thread_current ();

  int target_priority = t->init_priority;
  int test_priority;

  if (!list_empty (&t->donations)) {
    test_priority = list_entry (list_max(&t->donations, compare_thread_priority, NULL), struct thread, donation_elem)->priority;
    target_priority = (test_priority > target_priority) ? test_priority : target_priority;
  }

  t->priority = target_priority;
}

void
donate_priority (void)
{
  int depth = 0;
  struct thread *t = thread_current ();
  
  for (depth = 0; t->released_lock != NULL && depth < DONATION_DEPTH_MAX; ++depth, t = t->released_lock->holder) {
    if (t->released_lock->holder->priority < t->priority) {
      t->released_lock->holder->priority = t->priority;
    }
  }
}

void
remove_threads_from_donations (struct lock *lock)
{
  struct thread *t = thread_current ();
  struct list_elem *e;

  for (e = list_begin (&t->donations); e != list_end (&t->donations);) {
    if(list_entry (e, struct thread, donation_elem)->released_lock == lock) {
      e = list_remove (e);
    } else {
      e = list_next (e);
    }
  }
}


static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

uint32_t thread_stack_ofs = offsetof (struct thread, stack);

struct pcb *
get_child_pcb (tid_t child_tid)
{
  struct thread *t = thread_current ();
  struct thread *child;
  struct list *child_list = &(t->list_child_process);
  struct list_elem *e;

  for (e = list_begin (child_list); e != list_end (child_list); e = list_next (e)) 
  {
    child = list_entry (e, struct thread, elem_child_process);
    if (child->tid == child_tid) 
      return child->pcb;
  }

  return NULL;
}

struct thread *
get_child_thread (tid_t child_tid)
{
  struct thread *t = thread_current ();
  struct thread *child;
  struct list *child_list = &(t->list_child_process);
  struct list_elem *e;

  for (e = list_begin (child_list); e != list_end (child_list); e = list_next (e)) 
  {
    child = list_entry (e, struct thread, elem_child_process);
    if (child->tid == child_tid) 
      return child;
  }

  return NULL;
}

struct mmf *
init_mmf (int id, struct file *file, void *upage)
{
  struct mmf *mmf = (struct mmf *) malloc (sizeof *mmf);
  
  mmf->id = id;
  mmf->file = file;
  mmf->upage = upage;

  off_t ofs;
  int size = file_length (file);
  struct hash *spt = &thread_current ()->spt;

  for (ofs = 0; ofs < size; ofs += PGSIZE)
    if (get_spte (spt, upage + ofs))
      return NULL;

  for (ofs = 0; ofs < size; ofs += PGSIZE)
  {
    uint32_t read_bytes = ofs + PGSIZE < size ? PGSIZE : size - ofs;
    init_file_spte (spt, upage, file, ofs, read_bytes, PGSIZE - read_bytes, true);
    upage += PGSIZE;
  }

  list_push_back (&thread_current ()->mmf_list, &mmf->mmf_list_elem);

  return mmf;
}

struct mmf *
get_mmf (int mapid)
{
  struct list *list = &thread_current ()->mmf_list;
  struct list_elem *e;

  for (e = list_begin (list); e != list_end (list); e = list_next (e))
  {
    struct mmf *f = list_entry (e, struct mmf, mmf_list_elem);

    if (f->id == mapid)
      return f;
  }

  return NULL;
}