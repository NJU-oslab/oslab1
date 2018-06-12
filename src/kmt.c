#include <os.h>

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
static thread_t * current_thread = NULL;
static int thread_cnt = 0;
static spinlock_t thread_lock;
static spinlock_t sem_lock;

thread_t * thread_head = NULL;
extern mount_path_t procfs_path;

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

//changes made by kmt_create to the filesystem /proc/[pid]

static void add_procfs_inodes(thread_t *thread){
    inode_t *new_proc_inode = (inode_t *)pmm->alloc(sizeof(inode_t));
    if (!new_proc_inode) panic("inode allocation failed");

    new_proc_inode->can_read = 1;
    new_proc_inode->can_write = 1;
    new_proc_inode->open_thread_num = 0;
    new_proc_inode->pid = thread->tid;
    new_proc_inode->fs = procfs_path.fs;

    char pid[10], runnable[10], tf[200];
    sprintf(pid, "%d", thread->tid);
    sprintf(runnable, "%d", thread->runnable);
    sprintf(tf, "eax: 0x%x; ebx: 0x%x; ecx: 0x%x; edx: 0x%x; esi: 0x%x; edi: 0x%x; ebp: 0x%x; esp3: 0x%x",
        thread->tf->eax, thread->tf->ebx, thread->tf->ecx, thread->tf->edx, thread->tf->esi, thread->tf->edi, 
        thread->tf->ebp, thread->tf->esp3);

    strcpy(new_proc_inode->name, procfs_path.name);
    strcat(new_proc_inode->name, "/");
    strcat(new_proc_inode->name, pid);
    strcat(new_proc_inode->name, "/status");
    strcpy(new_proc_inode->content, "pid: ");
    strcat(new_proc_inode->content, pid);
    strcat(new_proc_inode->content, "\nrunnable: ");
    strcat(new_proc_inode->content, runnable);
    strcat(new_proc_inode->content, "\nregs: ");
    strcat(new_proc_inode->content, tf);
    strcat(new_proc_inode->content, "\n");
    int i;
    for (i = 0; i < MAX_INODE_NUM; i++){
        if (procfs_path.fs->inodes[i] == NULL){
            procfs_path.fs->inodes[i] = new_proc_inode;
            break;
        }
    }
    if (i == MAX_INODE_NUM){
        Log("The procfs's inode pool is full.");
        return;
    }
 //   Log("cpuinfo created.\nname: %s\ncontent:\n%s\n", procfs_path.fs->inodes[i]->name, procfs_path.fs->inodes[i]->content);
}

static void update_procfs_inode(thread_t *thread){
    char name[MAX_NAME_LEN], content[MAX_INODE_CONTENT_LEN];
    char pid[10], runnable[10], tf[200];
    sprintf(pid, "%d", thread->tid);
    sprintf(runnable, "%d", thread->runnable);
    sprintf(tf, "eax: 0x%x; ebx: 0x%x; ecx: 0x%x; edx: 0x%x; esi: 0x%x; edi: 0x%x; ebp: 0x%x; esp3: 0x%x",
        thread->tf->eax, thread->tf->ebx, thread->tf->ecx, thread->tf->edx, thread->tf->esi, thread->tf->edi, 
        thread->tf->ebp, thread->tf->esp3);

    strcpy(name, procfs_path.name);
    strcat(name, "/");
    strcat(name, pid);
    strcat(name, "/status");
    strcpy(content, "pid: ");
    strcat(content, pid);
    strcat(content, "\nrunnable: ");
    strcat(content, runnable);
    strcat(content, "\nregs: ");
    strcat(content, tf);
    strcat(content, "\n");
    int fd = vfs->open(name, O_RDWR);
    vfs->write(fd, content, sizeof(content));
    vfs->close(fd);
}

static void kmt_init(){
    current_thread = NULL;
    thread_head = NULL;
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

    add_procfs_inodes(thread);

    thread_t *new_thread = thread;
    if (thread_head == NULL){
        new_thread->next = NULL;
        thread_head = new_thread;
    }
    else{
        new_thread->next = thread_head;
        thread_head = new_thread;
    }
    
    Log("entry: 0x%x\targ: %d", entry, arg);


    /* print threads' information
    Log("------------------------------------------");
    Log("Now, the thread information is as follows.");
    thread_t *cur = thread_head;
    while (cur != NULL){
        Log("thread %d: runnbale: %d", cur->tid, cur->runnable);
        cur = cur->next;
    }
    Log("------------------------------------------");*/

    kmt_spin_unlock(&thread_lock);
    TRACE_EXIT;
    return 0;
}
static void kmt_teardown(thread_t *thread){
    kmt_spin_lock(&thread_lock);
    thread_t *cur = thread_head;
    int find_flag = 0;
    if (cur == NULL){
        Log("There's no thread that can be recycled.");
        assert(0);
    }
    else if (cur == thread){
        pmm->free(thread_head->stack.start);
        thread_head = cur->next;
        find_flag = 1;
    }
    else {
        while (cur->next != NULL){
            if (cur->next == thread){
                thread_t *p = cur->next;
                int i;
                for (i = 0; i < MAX_INODE_NUM; i++){
                    if (procfs_path.fs->inodes[i]->pid == cur->next->tid){
                        pmm->free(procfs_path.fs->inodes[i]);
                        procfs_path.fs->inodes[i] = NULL;
                        break;
                    }
                }
                if (i == MAX_INODE_NUM){
                    panic("Cannot delete the according proc inode.");
                    assert(0);
                }
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

    /* print threads' information
    Log("------------------------------------------");
    Log("Now, the thread information is as follows.");
    thread_t *curr = thread_head;
    while (curr != NULL){
        Log("thread %d: runnbale: %d", curr->tid, curr->runnable);
        curr = curr->next;
    }
    Log("------------------------------------------");*/

    kmt_spin_unlock(&thread_lock);
}
static thread_t *kmt_schedule(){
//    Log("Now, thread is %d", current_thread->tid);
    if (current_thread == NULL)
        current_thread = thread_head;
    thread_t *next_thread = current_thread;
    if (current_thread->next == NULL)
        next_thread = thread_head;
    else
        next_thread = current_thread->next;
    while (1){
        if (next_thread != NULL && next_thread->runnable == 1)
            break;
        if (next_thread->next != NULL)
            next_thread = next_thread->next;
        else
            next_thread = thread_head;
    }
    if (next_thread != NULL)
        update_procfs_inode(next_thread);
    current_thread = next_thread;
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
    kmt_spin_lock(&sem_lock);
    Log("%s: sem_count: 0x%x",sem->name, sem->count);
    while (sem->count == 0){
        current_thread->runnable = 0;
        current_thread->waiting_sem = sem;
        Log("%s: 0x%x",current_thread->waiting_sem->name, current_thread->waiting_sem);
        kmt_spin_unlock(&sem_lock);
//        while (current_thread->runnable == 0);
		_yield();
        kmt_spin_lock(&sem_lock);
    }
    sem->count --;
    kmt_spin_unlock(&sem_lock);
}

static void kmt_sem_signal(sem_t *sem){
    kmt_spin_lock(&sem_lock);
    thread_t *cur = thread_head;
    sem->count++;
    Log("%s: sem_count: 0x%x",sem->name, sem->count);
    while (cur != NULL){
        Log("sem: 0x%x", cur->waiting_sem);
        if (cur->runnable == 0 && cur->waiting_sem == sem){
            cur->runnable = 1;
            cur->waiting_sem = NULL;
            break;
        }
        cur = cur->next;
    }
    kmt_spin_unlock(&sem_lock);
}
