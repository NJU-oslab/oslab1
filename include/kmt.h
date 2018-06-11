#include "os.h"
#define MAX_NAME_LEN 100
#define MAX_STACK_SIZE 16 * 1024

struct spinlock{
    int locked; 
    char name[MAX_NAME_LEN];
};

struct semaphore{
    int count;
    char name[MAX_NAME_LEN];
    struct spinlock mutex;
};

typedef struct thread{
    volatile int runnable;
    int tid;
    int fd;
    _RegSet *tf;
    _Area stack;
    sem_t *waiting_sem;
    struct thread *next;
} thread_t;


