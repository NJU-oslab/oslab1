#include <os.h>

static void os_init();
static void os_run();
static _RegSet *os_interrupt(_Event ev, _RegSet *regs);

MOD_DEF(os) {
  .init = os_init,
  .run = os_run,
  .interrupt = os_interrupt,
};

static void os_init() {
  for (const char *p = "Hello, OS World!\n"; *p; p++) {
    _putc(*p);
  }
}

static void alloc_test() {
  int *arr = (int*)pmm->alloc(20 * sizeof(int));
  for (int i = 0; i < 20; i++){
    arr[i] = i;
  }
  for (int i = 0; i < 20; i++){
    Log("arr[%d]: %d", i, arr[i]);
  }
  pmm->free(arr);
  pmm->alloc(8192);
  pmm->alloc(400);
  pmm->alloc(1023);
  Log("_heap.end: %d", _heap.end);
}

static void os_run() {
  _intr_write(1); // enable interrupt
  alloc_test();
//  while (1) ; // should never return
}

static _RegSet *os_interrupt(_Event ev, _RegSet *regs) {
  if (ev.event == _EVENT_IRQ_TIMER) _putc('*');
  if (ev.event == _EVENT_IRQ_IODEV) _putc('I');
  if (ev.event == _EVENT_ERROR) {
    _putc('x');
    _halt(1);
  }
  return NULL; // this is allowed by AM
}
