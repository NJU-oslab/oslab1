#include <os.h>

static void vfs_init();
static int vfs_access(const char *path, int mode);
static int vfs_mount(const char *path, filesystem_t *fs);
static int vfs_unmount(const char *path);
static int vfs_open(const char *path, int flags);
static ssize_t vfs_read(int fd, void *buf, size_t nbyte);
static ssize_t vfs_write(int fd, void *buf, size_t nbyte);
static off_t vfs_lseek(int fd, off_t offset, int whence);
static int vfs_close(int fd);

MOD_DEF(vfs){
    .init = vfs_init,
    .access = vfs_access,
    .mount = vfs_mount,
    .unmount = vfs_unmount,
    .open = vfs_open,
    .read = vfs_read,
    .write = vfs_write,
    .lseek = vfs_lseek,
    .close = vfs_close,
};

//procfs's implementation

static fsops_t procfs_ops;
static fsops_t devfs_ops;
static fsops_t kvfs_ops;

static void fsops_init(struct filesystem *fs, const char *name, inode_t *dev){

}

static inode_t *fsops_lookup(struct filesystem *fs, const char *path, int flags){
    return NULL;
}

static int fsops_close(inode_t *inode){
    return 0;
}

static filesystem_t *create_procfs() {
    filesystem_t *fs = (filesystem_t *)pmm->alloc(sizeof(filesystem_t));
    if (!fs) panic("fs allocation failed");
    fs->ops = &procfs_ops;
    fs->ops->init(fs, "procfs", NULL);
    return fs;
}


//vfs API

static void vfs_init(){
    procfs_ops.init = &fsops_init;
    procfs_ops.lookup = &fsops_lookup;
    procfs_ops.close = &fsops_close;
}

static int vfs_access(const char *path, int mode){
    return 0;
}

static int vfs_mount(const char *path, filesystem_t *fs){
    return 0;
}

static int vfs_unmount(const char *path){
    return 0;
}

static int vfs_open(const char *path, int flags){
    return 0;
}

static ssize_t vfs_read(int fd, void *buf, size_t nbyte){
    return 0;
}

static ssize_t vfs_write(int fd, void *buf, size_t nbyte){
    return 0;
}

static off_t vfs_lseek(int fd, off_t offset, int whence){
    return 0;
}

static int vfs_close(int fd){
    return 0;
}