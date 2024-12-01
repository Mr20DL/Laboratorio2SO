#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "threads/synch.h"

#define MAX_OPEN_FILES 128
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define MAX_FILE_SIZE 0x8000000  

static struct lock filesys_lock;

static struct file *open_files[MAX_OPEN_FILES];

static void syscall_handler(struct intr_frame *);
static bool is_valid_ptr(const void *ptr);
static bool verify_buffer(const void *buffer, unsigned size);
static bool is_valid_string(const char *str);
static bool is_valid_fd(int fd);
static void sys_exit(int status);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool
is_valid_ptr(const void *ptr) 
{
    if (ptr == NULL || !is_user_vaddr(ptr))
        return false;
    return pagedir_get_page(thread_current()->pagedir, ptr) != NULL;
}

static bool
verify_buffer(const void *buffer, unsigned size) 
{
    if (buffer == NULL)
        return false;
    
    char *local_buffer = (char *)buffer;
    for (unsigned i = 0; i < size; i++)
        if (!is_valid_ptr(local_buffer + i))
            return false;
    return true;
}

static bool
is_valid_string(const char *str)
{
  if (!is_valid_ptr(str))
    return false;
  
  while (is_valid_ptr(str))
    {
      if (*str == '\0')
        return true;
      str++;
    }
  return false;
}

static bool
is_valid_fd(int fd)
{
  return fd >= 0 && fd < MAX_OPEN_FILES && open_files[fd] != NULL;
}

static int
sys_write(int fd, const void *buffer, unsigned size)
{
    if (!verify_buffer(buffer, size))
        sys_exit(-1);
  
    if (size > MAX_FILE_SIZE)
        return -1;

    if (fd == STDOUT_FILENO) {
        putbuf(buffer, size);
        return size;
    }

    if (!is_valid_fd(fd))
        return -1;

    lock_acquire(&filesys_lock);
    int bytes_written = file_write(open_files[fd], buffer, size);
    lock_release(&filesys_lock);

    return bytes_written;
}

static void
sys_exit(int status)
{
    struct thread *cur = thread_current();
    cur->exit_status = status;
    thread_exit();
}

static bool
create(const char *file, unsigned initial_size)
{
    if (!is_valid_string(file))
        sys_exit(-1);
  
    if (initial_size > MAX_FILE_SIZE)
        return false;

    lock_acquire(&filesys_lock);
    bool success = filesys_create(file, initial_size);
    lock_release(&filesys_lock);

    return success;
}

static int
open(const char *file)
{
    if (!is_valid_string(file))
        sys_exit(-1);

    lock_acquire(&filesys_lock);
    struct file *f = filesys_open(file);
    if (!f)
    {
        lock_release(&filesys_lock);
        return -1;
    }

    int fd;
    for (fd = 2; fd < MAX_OPEN_FILES; fd++)
        if (!open_files[fd])
        {
            open_files[fd] = f;
            lock_release(&filesys_lock);
            return fd;
        }

    file_close(f);
    lock_release(&filesys_lock);
    return -1;
}

static tid_t
sys_exec(const char *cmd_line)
{
    if (!is_valid_string(cmd_line))
        sys_exit(-1);

    return process_execute(cmd_line);
}

static void
sys_halt(void)
{
    shutdown_power_off();
}

static bool
sys_remove(const char *file)
{
    if (!is_valid_string(file))
        sys_exit(-1);

    lock_acquire(&filesys_lock);
    bool success = filesys_remove(file);
    lock_release(&filesys_lock);
    
    return success;
}

static int
sys_filesize(int fd)
{
    if (!is_valid_fd(fd))
        return -1;
    
    lock_acquire(&filesys_lock);
    int size = file_length(open_files[fd]);
    lock_release(&filesys_lock);

    return size;
}

static int
sys_read(int fd, void *buffer, unsigned size)
{
    if (!verify_buffer(buffer, size))
        sys_exit(-1);
  
    if (size > MAX_FILE_SIZE)
        return -1;

    if (fd == STDIN_FILENO)
    {
        uint8_t *buf = buffer;
        for (unsigned i = 0; i < size; i++)
            buf[i] = input_getc();
        return size;
    }
    
    if (!is_valid_fd(fd))
        return -1;

    lock_acquire(&filesys_lock);
    int bytes_read = file_read(open_files[fd], buffer, size);
    lock_release(&filesys_lock);

    return bytes_read;
}

static void
sys_seek(int fd, unsigned position)
{
    if (!is_valid_fd(fd))
        return;

    lock_acquire(&filesys_lock);
    file_seek(open_files[fd], position);
    lock_release(&filesys_lock);
}

static unsigned
sys_tell(int fd)
{
    if (!is_valid_fd(fd))
        return -1;

    lock_acquire(&filesys_lock);
    unsigned position = file_tell(open_files[fd]);
    lock_release(&filesys_lock);

    return position;
}

static void
sys_close(int fd)
{
    if (!is_valid_fd(fd))
        return;

    lock_acquire(&filesys_lock);
    file_close(open_files[fd]);
    open_files[fd] = NULL;
    lock_release(&filesys_lock);
}

static void
syscall_handler(struct intr_frame *f)
{
    uint32_t *esp = f->esp;
    
    if (!is_valid_ptr(esp))
        sys_exit(-1);

    int syscall_number = *(int*)esp;
    
    switch (syscall_number) {
        case SYS_HALT:
            sys_halt();
            NOT_REACHED();
            break;

        case SYS_EXIT:
            if (!verify_buffer(esp + 1, sizeof(int)))
                sys_exit(-1);
            sys_exit(*(int*)(esp + 1));
            break;

        case SYS_EXEC:
            if (!verify_buffer(esp + 1, sizeof(char *)))
                sys_exit(-1);
            f->eax = sys_exec(*(const char **)(esp + 1));
            break;

        case SYS_READ:
            if (!verify_buffer(esp + 1, sizeof(int) * 3))
                sys_exit(-1);
            f->eax = sys_read(*(int*)(esp + 1), *(void**)(esp + 2), *(unsigned*)(esp + 3));
            break;

        case SYS_WRITE:
            if (!verify_buffer(esp + 1, sizeof(int) * 3))
                sys_exit(-1);
            f->eax = sys_write(*(int*)(esp + 1), *(void**)(esp + 2), *(unsigned*)(esp + 3));
            break;

        case SYS_CREATE:
            if (!verify_buffer(esp + 1, sizeof(int) * 2))
                sys_exit(-1);
            f->eax = create(*(const char**)(esp + 1), *(unsigned*)(esp + 2));
            break;

        case SYS_REMOVE:
            if (!verify_buffer(esp + 1, sizeof(char *)))
                sys_exit(-1);
            f->eax = sys_remove(*(const char **)(esp + 1));
            break;

        case SYS_OPEN:
            if (!verify_buffer(esp + 1, sizeof(int)))
                sys_exit(-1);
            f->eax = open(*(const char**)(esp + 1));
            break;

        case SYS_FILESIZE:
            if (!verify_buffer(esp + 1, sizeof(int)))
                sys_exit(-1);
            f->eax = sys_filesize(*(int*)(esp + 1));
            break;

        case SYS_SEEK:
            if (!verify_buffer(esp + 1, sizeof(int) * 2))
                sys_exit(-1);
            sys_seek(*(int*)(esp + 1), *(unsigned*)(esp + 2));
            break;

        case SYS_TELL:
            if (!verify_buffer(esp + 1, sizeof(int)))
                sys_exit(-1);
            f->eax = sys_tell(*(int*)(esp + 1));
            break;

        case SYS_CLOSE:
            if (!verify_buffer(esp + 1, sizeof(int)))
                sys_exit(-1);
            sys_close(*(int*)(esp + 1));
            break;

        default:
            sys_exit(-1);
            break;
    }
}