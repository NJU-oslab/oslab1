#include <os.h>
#include "libc.h"

static void kmt_init();
static int kmt_create(thread_t *thread, void (*entry)(void *arg), void *arg);
static void kmt_teardown(thread_t *thread);
static thread_t *kmt_schedule();
static void kmt_spin_init(spinlock_t *lk, const char *name);
static void kmt_spin_lock(spinlock_t *lk);
static void kmt_spin_unlock(spinlock_t *lk);
static void kmt_sem_init(sem_t *sem, const char *name, int value);
static void kmt_sem_wait(sem_t *sem);
static void kmt_sem_signal(sem_t *sem);

static int spin_cnt = 0;
static int intr_ready = 0;
static thread_t *current_thread = NULL;
static thread_t *head = NULL;
static int thread_cnt = 0;
static spinlock_t thread_lock;
static spinlock_t sem_lock;

MOD_DEF(kmt) {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .schedule = kmt_schedule,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
};

static void kmt_init(){
    current_thread = NULL;
    head = NULL;
    thread_cnt = 0;
    kmt_spin_init(&thread_lock, "thread_lock");
    kmt_spin_init(&sem_lock, "sem_lock");
}
static int kmt_create(thread_t *thread, void (*entry)(void *arg), void *arg){
    TRACE_ENTRY;
    kmt_spin_lock(&thread_lock);

    Log("Allocate begins.");
    thread->stack.start = pmm->alloc(MAX_STACK_SIZE);
    Log("The thread has been allocated memory.");
    thread->stack.end = thread->stack.start + MAX_STACK_SIZE;
    thread->runnable = 1;
    thread->waiting_sem = NULL;
    thread->tid = thread_cnt++;
    thread->tf = _make(thread->stack, entry, arg);

    thread_t *new_thread = thread;
    if (head == NULL){
        new_thread->next = NULL;
        head = new_thread;
    }
    else{
        new_thread->next = head;
        head = new_thread;
    }
    
    Log("entry: 0x%x\targ: %d", entry, arg);


    /* print threads' information*/
    Log("------------------------------------------");
    Log("Now, the thread information is as follows.");
    thread_t *cur = head;
    while (cur != NULL){
        Log("thread %d: runnbale: %d", cur->tid, cur->runnable);
        cur = cur->next;
    }
    Log("------------------------------------------");

    kmt_spin_unlock(&thread_lock);
    TRACE_EXIT;
    return 0;
}
static void kmt_teardown(thread_t *thread){
    kmt_spin_lock(&thread_lock);
    thread_t *cur = head;
    int find_flag = 0;
    if (cur == NULL){
        Log("There's no thread that can be recycled.");
        assert(0);
    }
    else if (cur == thread){
        pmm->free(head->stack.start);
        head = NULL;
        find_flag = 1;
    }
    else {
        while (cur->next != NULL){
            if (cur->next == thread){
                thread_t *p = cur->next;
                pmm->free(cur->next->stack.start);
                cur->next = p->next;
                find_flag = 1;
                break;
            }
            cur = cur->next;
        }
    }
    if (find_flag == 0){
        Log("Cannot find the thread you want to recycle.");
    }

    /* print threads' information*/
    Log("------------------------------------------");
    Log("Now, the thread information is as follows.");
    thread_t *curr = head;
    while (curr != NULL){
        Log("thread %d: runnbale: %d", curr->tid, curr->runnable);
        curr = curr->next;
    }
    Log("------------------------------------------");

    kmt_spin_unlock(&thread_lock);
}
static thread_t *kmt_schedule(){
    thread_t * cnm = head;
    while (cnm != NULL){
//        printf("0x%x\t %d\n", cnm, cnm->runnable);
        cnm = cnm->next;
    }
    if (current_thread == NULL)
        current_thread = head;
    thread_t *next_thread = current_thread;
 //   if (current_thread != NULL)
 //       Log("current_thread->tid: %d", current_thread->tid);
    if (current_thread->next == NULL)
        next_thread = head;
    else
        next_thread = current_thread->next;
//    printf("next_thread: 0x%x\n %d\n", next_thread, next_thread->runnable);
    while (1){
//        printf("next_thread: 0x%x %d\n", next_thread,next_thread->runnable);
        if (next_thread != NULL && next_thread->runnable == 1)
            break;
        if (next_thread->next != NULL)
            next_thread = next_thread->next;
        else
            next_thread = head;
    }
    current_thread = next_thread;
//    printf("Sc over\n");
    return next_thread;
}
static void kmt_spin_init(spinlock_t *lk, const char *name){
    lk->locked = 0;
    strncpy(lk->name, name, MAX_NAME_LEN);
}
static void kmt_spin_lock(spinlock_t *lk){
    int intr_status = _intr_read();
    _intr_write(0);
    if (lk->locked == 0){
        lk->locked = 1;
        if (spin_cnt == 0)
            intr_ready = intr_status;
        spin_cnt ++;
    }
    else{
        Log("The spinlock has been locked!");
    }
}
static void kmt_spin_unlock(spinlock_t *lk){
    if (lk->locked == 1){
        lk->locked = 0;
        spin_cnt --;
        if (spin_cnt == 0)
            _intr_write(intr_ready);
    }
    else{
        Log("The spinlock has been unlocked!");
    }
}

static void kmt_sem_init(sem_t *sem, const char *name, int value){
    sem->count = value;
    strncpy(sem->name, name, MAX_NAME_LEN);
}

static void kmt_sem_wait(sem_t *sem){
//    if (strcmp(current_thread->name, "consumer") == 0)
//        printf("consumer!\n");
    kmt_spin_lock(&sem_lock);
    Log("%s: sem_count: 0x%x",sem->name, sem->count);
    while (sem->count == 0){
        current_thread->runnable = 0;
        current_thread->waiting_sem = sem;
        Log("%s: 0x%x",current_thread->waiting_sem->name, current_thread->waiting_sem);
        kmt_spin_unlock(&sem_lock);
       // printf("lock name: %s\n", sem_lock.name);
        while (current_thread->runnable == 0){
            kmt_spin_lock(&sem_lock);
//            printf("current t: %x\n",current_thread);
            kmt_spin_unlock(&sem_lock);
            //_putc('f');
        };
        //_yield();
//        printf("Come back\n");
        kmt_spin_lock(&sem_lock);
    }
    sem->count --;
//    printf("Out of wait\n");
    kmt_spin_unlock(&sem_lock);
}

static void kmt_sem_signal(sem_t *sem){
    kmt_spin_lock(&sem_lock);
    thread_t *cur = head;
    sem->count++;
    Log("%s: sem_count: 0x%x",sem->name, sem->count);
    while (cur != NULL){
        Log("sem: 0x%x", cur->waiting_sem);
        if (cur->runnable == 0 && cur->waiting_sem == sem){
//            printf("%s: 0x%x 0x%x\n ",cur->waiting_sem->name, cur->waiting_sem,cur);
            cur->runnable = 1;
            cur->waiting_sem = NULL;
            break;
        }
        cur = cur->next;
    }
//    printf("Out of signal");
    kmt_spin_unlock(&sem_lock);
}