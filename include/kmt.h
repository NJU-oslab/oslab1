#include "os.h"
#define MAX_NAME_LEN 100
#define MAX_STACK_SIZE 4 * 1024 * 1024

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
    int runnable;
    int tid;
    _RegSet *tf;
    _Area stack;
    sem_t *waiting_sem;
    struct thread *next;
} thread_t;



