#define MAX_NAME_LEN 100

struct spinlock{
    int locked; 
};

struct semaphore{
    int count;
    char name[MAX_NAME_LEN];
    struct spinlock mutex;
};

struct thread{
    int sleeping;
};

typedef struct cond_node{
    struct thread waiting_thread;
    struct spinlock *mutex;
    struct cond_node *next;
} cond_node_t;

typedef struct cond{
    struct cond_node *q;
} cond_t;


