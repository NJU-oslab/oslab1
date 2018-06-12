#include "os.h"

extern void print_proc_inodes();
static void procfs_test();
static void devfs_test();
static void kvfs_test();
static void mount_test();
static void error_processing_test();
static void multithread_test();

extern fsops_t kvfs_ops;

void alloc_test() {
  TestLog("alloc_test begin...");
  TestLog("intr_status: %d", _intr_read());
  int *arr = (int*)pmm->alloc(20 * sizeof(int));
  TestLog("intr_status: %d", _intr_read());
  for (int i = 0; i < 20; i++){
    arr[i] = i;
  }
  for (int i = 0; i < 20; i++){
    TestLog("arr[%d]: %d", i, arr[i]);
  }
  pmm->free(arr);
  TestLog("intr_status: %d", _intr_read());
  pmm->alloc(16);
  pmm->alloc(20);
  pmm->alloc(24);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  pmm->alloc(120);
  TestLog("intr_status: %d", _intr_read());
  TestLog("_heap.end: 0x%x", _heap.end);
  TestLog("alloc_test end.\n============================");
}

static thread_t test_thread[5];

static void thread_test_func(void *tid){
    TRACE_ENTRY;
    int cnt;
    for (cnt = 0; cnt < 10; cnt++){
        TestLog("The %d thread prints: %d", (int)tid, cnt);
    }
    TestLog("The %d thread is finished.", (int)tid);
    while (1);
}

void thread_test() {
  TestLog("thread_test begin...");   
  int i;
  for (i = 0; i < 5; i++){
    kmt->create(&test_thread[i], thread_test_func, (void *)i);
    TestLog("Thread %d created.", i);
  }
/*  kmt->teardown(&test_thread[2]);
  TestLog("Thread 2 has been teardowned.");
  kmt->teardown(&test_thread[4]);
  TestLog("THread 4 has been teardowned.");*/
  TestLog("thread_test end\n============================");
}

static sem_t empty;
static sem_t fill;
static thread_t sem_test_thread[2];

static void producer(void *arg) {
  while (1){
    kmt->sem_wait(&empty);
    printf("(");
    kmt->sem_signal(&fill);
  }
}

static void consumer(void *arg) {
  while (1){
    kmt->sem_wait(&fill);
    printf(")");
    kmt->sem_signal(&empty);
  }
}

void sem_test() {
  TestLog("sem_test begin");
  kmt->sem_init(&empty, "empty", 20);
  kmt->sem_init(&fill, "fill", 0);
  kmt->create(&sem_test_thread[0], producer, NULL);
  kmt->create(&sem_test_thread[1], consumer, NULL);
  TestLog("sem_test end.");
//  printf("sem_test end\n============================");
}


static thread_t fs_test_thread[5];

static void fs_test_func(void *tid){
    while (1);
}

static int random_number(int min, int max)
{
	static int dev_random_fd = -1;
	char *next_random_byte;
	int bytes_to_read;
	unsigned random_value;
	assert( max > min );
	if( dev_random_fd == -1){
		dev_random_fd = vfs->open( "/dev/random", O_RDONLY);
		assert( dev_random_fd != -1);
  }
	next_random_byte = ( char * ) &random_value;
	bytes_to_read = sizeof( random_value );
	do {
		int bytes_read;
		bytes_read = vfs->read( dev_random_fd, next_random_byte, bytes_to_read );
		bytes_to_read -= bytes_read;
		next_random_byte += bytes_read;
	} while ( bytes_to_read > 0 );
	return min + ( random_value % ( max - min + 1 ));
}

static void procfs_test(){
  TestLog("procfs_test begins...");
  int i;
  for (i = 0; i < 5; i++){
    kmt->create(&fs_test_thread[i], fs_test_func, (void *)i);
    TestLog("Thread %d created.", i);
  }
  kmt->teardown(&fs_test_thread[2]);
  TestLog("Thread 2 deleted.");
  print_proc_inodes();
  int fd = vfs->open("/proc/1/status", O_RDWR);
  if (fd == -1){
    panic("open failed.\n");
  }
  assert(vfs->access("/proc/1/status", W_OK) == -1);
  char buf[200];
  if (vfs->read(fd, buf, sizeof(buf) - 1) == -1)
    panic("read failed");
  printf("buf------\n%s\n", buf);
  if (vfs->write(fd, "1234", 4) == -1)
    panic("write failed");
  if (vfs->read(fd, buf, sizeof(buf) - 1) == -1)
    panic("read failed");
  printf("buf------\n%s\n", buf);
  assert(strcmp(buf, "1234") == 0);
  vfs->close(fd);
  assert(vfs->access("/proc/1/status", F_OK) == 0);
  assert(vfs->access("/proc/1/status", R_OK) == 0);
  assert(vfs->access("/proc/1/status", W_OK) == 0);
  assert(vfs->access("/proc/1/status", X_OK) == -1);
  TestLog("procfs_test passed.");
}

static void devfs_test(){
  TestLog("devfs_test begins...");
  int test = 5, ret;
  char buf[10];
	while(test--){
    ret = random_number(1, 10);
    assert(ret >= 1 && ret <= 10);
    printf("%d ", ret);
  }
  printf("\n");
  int fd = vfs->open("/dev/null", O_RDONLY);
  assert(vfs->read(fd, buf, sizeof(buf)) == -1);
  TestLog("devfs_test passed.");
}

static void kvfs_test(){
  TestLog("kvfs_test begins...");
  int fd = vfs->open("/a.txt", O_RDWR);
  char buf[20];
  if (fd == -1){
    panic("open failed.\n");
  }
  assert(vfs->access("/a.txt", W_OK) == -1);
  if (vfs->write(fd, "1234", 4) == -1)
    panic("write failed");
  if (vfs->read(fd, buf, sizeof(buf) - 1) == -1)
    panic("read failed");
  printf("buf------\n%s\n", buf);
  vfs->close(fd);
  assert(vfs->access("/a.txt", F_OK) == 0);
  assert(vfs->access("/a.txt", R_OK) == 0);
  assert(vfs->access("/a.txt", W_OK) == 0);
  assert(vfs->access("/a.txt", X_OK) == -1);
  TestLog("kvfs_test passed.");
}

static void mount_test(){
  TestLog("mount_test begins...");
  filesystem_t *fs = (filesystem_t *)pmm->alloc(sizeof(filesystem_t));
  if (!fs) panic("fs allocation failed");
  fs->ops = &kvfs_ops;
  fs->ops->init(fs, "kvfs");
  vfs->mount("/", fs);
  vfs->unmount("/");
  assert(vfs->open("/a.txt", O_RDONLY) != -1);
  TestLog("unmount_test passed.");
}

static void multithread_test(){

}

static void error_processing_test(){

}

void fs_test() {
  TestLog("fs_test begin...");
  procfs_test();
  devfs_test();
  kvfs_test();
  mount_test();
  multithread_test();
  error_processing_test();


  TestLog("fs_test end");
}