#define MAX_NAME_LEN 100

struct spinlock{
    int locked; 
};

struct cond_node{
    struct cond_node *next;
};

struct semaphore{
    int count;
    char name[MAX_NAME_LEN];
    struct spinlock mutex;
    struct cond_node *q;
};