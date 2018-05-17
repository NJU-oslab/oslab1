#include "os.h"
#include "test.h"

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

static void os_run() {
//  alloc_test();
  thread_test();
//  sem_test();
  _intr_write(1);
  while (1) ; // should never return
}

static _RegSet *os_interrupt(_Event ev, _RegSet *regs) {
  if (ev.event == _EVENT_IRQ_TIMER || ev.event == _EVENT_YIELD){
//    Log("_EVENT_IRQ_TIMER");
    if (current_thread != NULL)
      current_thread->tf = regs;
//    printf("Event\n");
    current_thread = kmt->schedule();
//    printf("%x\n",current_thread);
    return current_thread->tf;
  } 
  if (ev.event == _EVENT_IRQ_IODEV) Log("_EVENT_IRQ_IODEV");
  if (ev.event == _EVENT_ERROR) {
    Log("_EVENT_IRQ_ERROR");
    _halt(1);
  }
  return NULL; // this is allowed by AM
}
