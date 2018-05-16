#include "os.h"
#define MAX_NAME_LEN 100
#define MAX_STACK_SIZE 16 * 1024

struct spinlock{
    int locked; 
};

struct semaphore{
    int count;
    char name[MAX_NAME_LEN];
    struct spinlock mutex;
};

typedef struct thread{
    union {
        uint8_t stack[MAX_STACK_SIZE];
        struct {
            uint8_t pad[MAX_STACK_SIZE - sizeof(_RegSet)];
            _RegSet *tf;
        };
    };
    int runnable;
    int tid;
    struct thread *next;
} thread_t;

typedef struct cond_node{
    struct thread *waiting_thread;
    struct spinlock *mutex;
    struct cond_node *next;
} cond_node_t;

typedef struct cond{
    struct cond_node *q;
} cond_t;


