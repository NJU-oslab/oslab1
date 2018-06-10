#include "os.h"

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

static char fd_pool[MAX_FD_NUM];
static file_t *file_pool[MAX_FILE_NUM];

//procfs's implementation

static fsops_t procfs_ops;
static fsops_t devfs_ops;
static fsops_t kvfs_ops;

static void fsops_init(struct filesystem *fs, const char *name, inode_t *dev){
    if (name != NULL) {
        strcpy(fs->name, name);
        if (strcmp("procfs", name) == 0)
            fs->fs_type = PROCFS;
        else if (strcmp("devfs", name) == 0)
            fs->fs_type = DEVFS;
        else if (strcmp("kvfs", name) == 0)
            fs->fs_type = KVFS;
        else
            panic("a filesystem is created but cannot be identified.");
    }
    fs->dev = dev;
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
    vfs->mount("/proc", fs);
    return fs;
}

//file's implementation

static fileops_t file_ops;

static int fileops_open(inode_t *inode, file_t *file, int flags){
    return 0;
}

static ssize_t fileops_read(inode_t *inode, file_t *file, char *buf, size_t size){
    return 0;
}

static ssize_t fileops_write(inode_t *inode, file_t *file, const char *buf, size_t size){
    return 0;
}

static off_t fileops_lseek(inode_t *inode, file_t *file, off_t offset, int whence){
    return 0;
}

//vfs API

static mount_path_t procfs_path;
static mount_path_t devfs_path;
static mount_path_t kvfs_path;

static void vfs_init(){
    memset(fd_pool, 0, sizeof(fd_pool));
    procfs_ops.init = &fsops_init;
    procfs_ops.lookup = &fsops_lookup;
    procfs_ops.close = &fsops_close;
    strcpy(procfs_path.name, "/proc");
    strcpy(devfs_path.name, "/dev");
    strcpy(kvfs_path.name, "/");
}

static int vfs_access(const char *path, int mode){
    inode_t *open_inode = NULL;
    if (strncmp("/proc", path, 5) == 0){
        open_inode = procfs_path.fs->ops->lookup(procfs_path.fs, path, mode);
    }
    else if (strncmp("/dev", path, 4) == 0){
        open_inode = devfs_path.fs->ops->lookup(devfs_path.fs, path, mode);
    }
    else if (strncmp("/", path, 1) == 0){
        open_inode = kvfs_path.fs->ops->lookup(kvfs_path.fs, path, mode);
    }
    else{
        panic("Cannot find the path.");
        return -1;
    }

    if (open_inode != NULL){
        
    }
    else{
        panic("Cannot find the path you want to lookup.");
        return -1;
    }
}

static int vfs_mount(const char *path, filesystem_t *fs){
    switch (fs->fs_type){
    case PROCFS: procfs_path.fs = fs; fs->path = &procfs_path; break;
    case DEVFS: devfs_path.fs = fs; fs->path = &devfs_path; break;
    case KVFS: kvfs_path.fs = fs; fs->path = &kvfs_path; break;
    default: break;
    }
    return 0;
}

static int vfs_unmount(const char *path){
    if (strcmp(path, procfs_path.name) == 0){
        procfs_path.fs = NULL;
    }
    else if (strcmp(path, devfs_path.name) == 0){
        devfs_path.fs = NULL;
    }
    else if (strcmp(path, kvfs_path.name) == 0){
        kvfs_path.fs = NULL;
    }
    return 0;
}

static int vfs_open(const char *path, int flags){
    inode_t *open_inode = NULL;
    if (strncmp("/proc", path, 5) == 0){
        open_inode = procfs_path.fs->ops->lookup(procfs_path.fs, path, flags);
    }
    else if (strncmp("/dev", path, 4) == 0){
        open_inode = devfs_path.fs->ops->lookup(devfs_path.fs, path, flags);
    }
    else if (strncmp("/", path, 1) == 0){
        open_inode = kvfs_path.fs->ops->lookup(kvfs_path.fs, path, flags);
    }
    else{
        panic("Cannot find the path.");
        return -1;
    }

    if (open_inode != NULL){
        int ret_fd = -1;
        file_t *f = (file_t *)pmm->alloc(sizeof(file_t));
        if (!f) {
            panic("file allocation failed");
            return -1;
        }
        f->ops = &file_ops;
        ret_fd = f->ops->open(open_inode, f, flags);
        if (ret_fd < 0){
            panic("file open error");
            return -1;
        }
        //TODO assign the fd is implemented in ops 
        int i;
        for (i = 0; i < MAX_FILE_NUM; i++){
            if (file_pool[i] == NULL){
                file_pool[i] = f;
            }
        }
        if (i == MAX_FILE_NUM){
            panic("File pool is full, you cannot open more files.");
            return -1;
        }
        return ret_fd;
    }
    else{
        panic("Cannot find the path you want to lookup.");
        return -1;
    }
}

static ssize_t vfs_read(int fd, void *buf, size_t nbyte){
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++){
        if (file_pool[i]->fd == fd){
            break;
        }
    }
    if (i == MAX_FILE_NUM){
        panic("Cannot find the fd, so read failed.");
        return -1;
    }
    return file_pool[i]->ops->read(file_pool[i]->f_inode, file_pool[i], buf, nbyte);
}

static ssize_t vfs_write(int fd, void *buf, size_t nbyte){
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++){
        if (file_pool[i]->fd == fd){
            break;
        }
    }
    if (i == MAX_FILE_NUM){
        panic("Cannot find the fd, so write failed.");
        return -1;
    }
    return file_pool[i]->ops->write(file_pool[i]->f_inode, file_pool[i], buf, nbyte);
}

static off_t vfs_lseek(int fd, off_t offset, int whence){
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++){
        if (file_pool[i]->fd == fd){
            break;
        }
    }
    if (i == MAX_FILE_NUM){
        panic("Cannot find the fd, so lseek failed.");
        return -1;
    }
    return file_pool[i]->ops->lseek(file_pool[i]->f_inode, file_pool[i], offset, whence);
}

static int vfs_close(int fd){
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++){
        if (file_pool[i]->fd == fd){
            break;
        }
    }
    if (i == MAX_FILE_NUM){
        panic("Cannot find the fd, so write failed.");
        return -1;
    }
    pmm->free(file_pool[i]);
    file_pool[i] = NULL;
    return 0;
}