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
static int finished_thread[5];

void thread_test_func(void *tid){
    TRACE_ENTRY;
    int cnt;
    for (cnt = 0; cnt < 10; cnt++){
        Log("The %d thread prints: %d", (int)tid, cnt);
    }
    finished_thread[(int)tid] = 1;
    Log("The %d thread is finished.", (int)tid);
    while (1);
}

void thread_test() {
  Log("thread_test begin...");   
  int i;
  for (i = 0; i < 5; i++){
    finished_thread[i] = 0;
  }
  for (i = 0; i < 5; i++){
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
//    Log("cnt: %d", cnt);
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
  Log("sem_test begin");

  Log("sem_test end\n============================");
}

static void os_run() {
  _intr_write(1);
  alloc_test();
  Log("intr_status: %d", _intr_read());
  thread_test();
  sem_test();
  _intr_write(1);
  while (1) ; // should never return
}

static _RegSet *os_interrupt(_Event ev, _RegSet *regs) {
  if (ev.event == _EVENT_IRQ_TIMER){
 //   Log("_EVENT_IRQ_TIMER");
    if (current_thread != NULL)
      current_thread->tf = regs;
    current_thread = kmt->schedule();
    return current_thread->tf;
  } 
  if (ev.event == _EVENT_IRQ_IODEV) Log("_EVENT_IRQ_IODEV");
  if (ev.event == _EVENT_ERROR) {
    Log("_EVENT_IRQ_ERROR");
    _halt(1);
  }
  return NULL; // this is allowed by AM
}
