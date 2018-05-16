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
  Log("============================\nalloc_test begin...");
  int *arr = (int*)pmm->alloc(20 * sizeof(int));
  for (int i = 0; i < 20; i++){
    arr[i] = i;
  }
  for (int i = 0; i < 20; i++){
    Log("arr[%d]: %d", i, arr[i]);
  }
  pmm->free(arr);
  pmm->alloc(16);
  pmm->alloc(20);
  pmm->alloc(24);
  Log("_heap.end: 0x%x", _heap.end);
  Log("alloc_test end.\n============================");
}

static thread_t test_thread[5];
static int finished_thread[5];

static void thread_test_func(void *tid){
    int cnt;
    for (cnt = 0; cnt < 10; cnt++){
        Log("The %d thread prints: %d", (int)tid, cnt);
    }
    Log("The %d thread is finished.");

}

void thread_test() {
  Log("============================\nthread_test begin...");   
  int i;
  for (i = 0; i < 5; i++){
    finished_thread[i] = 0;
    kmt->create(&test_thread[i], thread_test_func, (void *)i);
    Log("Thread %d created.", i);
  }
  while (1){
    int j;
    int cnt = 0;
    for (j = 0; j < 5; j++){
        if (finished_thread[j] == 1)
            cnt++;
    }
    if (cnt == 5)
        break;
  }
  Log("All the thread has finished.");
  for (i = 0; i < 5; i++)
    kmt->teardown(&test_thread[i]);
  Log("All the thread has been teardowned.");
  Log("thread_test end\n============================");
}

void sem_test() {
  Log("============================\nsem_test begin");

  Log("sem_test end\n============================");
}

static void os_run() {
  alloc_test();
  thread_test();
  sem_test();
  _intr_write(1);
  while (1) ; // should never return
}

static _RegSet *os_interrupt(_Event ev, _RegSet *regs) {
  if (ev.event == _EVENT_IRQ_TIMER){
 //   Log("_EVENT_IRQ_TIMER");
/*    if (current_thread != NULL)
      current_thread->tf = regs;
    current_thread = kmt->schedule();
    return current_thread->tf;*/
  } 
  if (ev.event == _EVENT_IRQ_IODEV) Log("_EVENT_IRQ_IODEV");
  if (ev.event == _EVENT_ERROR) {
    Log("_EVENT_IRQ_ERROR");
    _halt(1);
  }
  return NULL; // this is allowed by AM
}
