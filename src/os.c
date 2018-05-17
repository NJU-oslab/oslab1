#include "os.h"

static void os_init();
static void os_run();
static _RegSet *os_interrupt(_Event ev, _RegSet *regs);
static thread_t *current_thread;


MOD_DEF(os) {
  .init = os_init,
  .run = os_run,
  .interrupt = os_interrupt,
};

static void os_init() {
  for (const char *p = "Hello, OS World!\n"; *p; p++) {
    _putc(*p);
  }
  current_thread = NULL;
}

void alloc_test() {
  Log("alloc_test begin...");
  Log("intr_status: %d", _intr_read());
  int *arr = (int*)pmm->alloc(20 * sizeof(int));
  Log("intr_status: %d", _intr_read());
  for (int i = 0; i < 20; i++){
    arr[i] = i;
  }
  for (int i = 0; i < 20; i++){
    Log("arr[%d]: %d", i, arr[i]);
  }
  pmm->free(arr);
  Log("intr_status: %d", _intr_read());
  pmm->alloc(16);
  pmm->alloc(20);
  pmm->alloc(24);
  Log("intr_status: %d", _intr_read());
  Log("_heap.end: 0x%x", _heap.end);
  Log("alloc_test end.\n============================");
}

static thread_t test_thread[5];

static void thread_test_func(void *tid){
    TRACE_ENTRY;
    int cnt;
    for (cnt = 0; cnt < 10; cnt++){
        Log("The %d thread prints: %d", (int)tid, cnt);
    }
    Log("The %d thread is finished.", (int)tid);
    while (1);
}

void thread_test() {
  Log("thread_test begin...");   
  int i;
  for (i = 0; i < 5; i++){
    kmt->create(&test_thread[i], thread_test_func, (void *)i);
    Log("Thread %d created.", i);
  }
  kmt->teardown(&test_thread[2]);
  Log("Thread 2 has been teardowned.");
  Log("thread_test end\n============================");
}

static sem_t empty;
static sem_t fill;
static thread_t sem_test_thread[2];

static void producer(void *arg) {
  while (1){
    _putc('s');
    kmt->sem_wait(&empty);
    _putc('t');
    printf("(");
    kmt->sem_signal(&fill);
  }
}

static void consumer(void *arg) {
  printf("Hello world from consumer");
  while (1){
    kmt->sem_wait(&fill);
    
    printf(")");
    kmt->sem_signal(&empty);
  }
}

void sem_test() {
  Log("sem_test begin");
  kmt->sem_init(&empty, "empty", 20);
  kmt->sem_init(&fill, "fill", 0);
  kmt->create(&sem_test_thread[0], producer, NULL);
  kmt->create(&sem_test_thread[1], consumer, NULL);
  printf("sem_test end\n============================");
}

static void os_run() {
//  alloc_test();
//  thread_test();
  sem_test();
  _intr_write(1);
  while (1) ; // should never return
}

static _RegSet *os_interrupt(_Event ev, _RegSet *regs) {
  if (ev.event == _EVENT_IRQ_TIMER || ev.event == _EVENT_YIELD){
//    Log("_EVENT_IRQ_TIMER");
    if (current_thread != NULL)
      current_thread->tf = regs;
    current_thread = kmt->schedule();
    printf("%x\n",current_thread);
    return current_thread->tf;
  } 
  if (ev.event == _EVENT_IRQ_IODEV) Log("_EVENT_IRQ_IODEV");
  if (ev.event == _EVENT_ERROR) {
    Log("_EVENT_IRQ_ERROR");
    _halt(1);
  }
  return NULL; // this is allowed by AM
}
