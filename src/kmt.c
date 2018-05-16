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
static cond_t cond;
static thread_t current_thread;


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

}
static int kmt_create(thread_t *thread, void (*entry)(void *arg), void *arg){
    return 0;
}
static void kmt_teardown(thread_t *thread){

}
static thread_t *kmt_schedule(){
    return NULL;
}
static void kmt_spin_init(spinlock_t *lk, const char *name){
    lk->locked = 0;
}
static void kmt_spin_lock(spinlock_t *lk){
    if (lk->locked == 0){
        lk->locked = 1;
        spin_cnt ++;
        if (spin_cnt > 0 && _intr_read() == 1)
            _intr_write(0);
    }
}
static void kmt_spin_unlock(spinlock_t *lk){
    if (lk->locked == 1){
        lk->locked = 0;
        spin_cnt --;
        if (spin_cnt <= 0 && _intr_read() == 0)
            _intr_write(1);
    }
}

static void cond_wait(cond_t *cond, spinlock_t *lk){
    cond_node_t *new_cond_node;
    kmt_spin_unlock(lk);

 //   current_thread.sleeping = 1;
    //new_cond_node->waiting_thread = current_thread;
   // new_cond_node->mutex = lk;
   // new_cond_node->next = cond->q;

  //  cond->q = new_cond_node;
}

static void cond_signal(cond_t *cond){
    cond_node_t *current_cond_node;
 /*   while (current_cond_node != NULL){
        
    }*/
}

static void kmt_sem_init(sem_t *sem, const char *name, int value){
    sem->count = value;
    strcpy(sem->name, name);
    kmt_spin_init(&sem->mutex, name);
}

static void kmt_sem_wait(sem_t *sem){
    kmt_spin_lock(&sem->mutex);
    while (sem->count == 0){
        cond_wait(&cond, &sem->mutex);
    }
    sem->count --;
    kmt_spin_unlock(&sem->mutex);
}

static void kmt_sem_signal(sem_t *sem){
    kmt_spin_lock(&sem->mutex);
    sem->count++;
    cond_signal(&cond);
    kmt_spin_unlock(&sem->mutex);
}