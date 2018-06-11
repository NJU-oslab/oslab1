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

extern thread_t *thread_head;

mount_path_t procfs_path;
static mount_path_t devfs_path;
static mount_path_t kvfs_path;

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
            Log("a filesystem is created but cannot be identified.");
    }
    for (int i = 0; i < MAX_INODE_NUM; i++)
        fs->inodes[i] = NULL;
    fs->dev = dev;
}

static inode_t *fsops_lookup_open(struct filesystem *fs, const char *path, int flags){
    //TODO:
    return NULL;
}

static inode_t *fsops_lookup_access(struct filesystem *fs, const char *path, int mode){
    //TODO:
    return NULL;
}

static int fsops_close(inode_t *inode){
    //TODO:
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

static void create_procinodes() {
    inode_t *cpuinfo_inode = (inode_t *)pmm->alloc(sizeof(inode_t));
    if (!cpuinfo_inode) panic("inode allocation failed");
    cpuinfo_inode->can_read = 1;
    cpuinfo_inode->can_write = 0;
    cpuinfo_inode->open_thread_num = 0;
    strcpy(cpuinfo_inode->name, procfs_path.name);
    strcat(cpuinfo_inode->name, "/cpuinfo");
    strcpy(cpuinfo_inode->content, "This is the cpuinfo file.\n");
    int i;
    for (i = 0; i < MAX_INODE_NUM; i++){
        if (procfs_path.fs->inodes[i] == NULL){
            procfs_path.fs->inodes[i] = cpuinfo_inode;
            break;
        }
    }
    if (i == MAX_INODE_NUM){
        Log("The procfs's inode pool is full.");
        return;
    }
    Log("cpuinfo created.\nname: %s\ncontent:\n%s\n", procfs_path.fs->inodes[i]->name, procfs_path.fs->inodes[i]->content);

    inode_t *meminfo_inode = (inode_t *)pmm->alloc(sizeof(inode_t));
    if (!meminfo_inode) panic("inode allocation failed");
    meminfo_inode->can_read = 1;
    meminfo_inode->can_write = 0;
    meminfo_inode->open_thread_num = 0;
    strcpy(meminfo_inode->name, procfs_path.name);
    strcat(meminfo_inode->name, "/meminfo");
    strcpy(meminfo_inode->content, "This is the meminfo file.\n");
    for (i = 0; i < MAX_INODE_NUM; i++){
        if (procfs_path.fs->inodes[i] == NULL){
            procfs_path.fs->inodes[i] = meminfo_inode;
            break;
        }
    }
    if (i == MAX_INODE_NUM){
        Log("The procfs's inode pool is full.");
        return;
    }
    Log("cpuinfo created.\nname: %s\ncontent:\n%s\n", procfs_path.fs->inodes[i]->name, procfs_path.fs->inodes[i]->content);
}

static filesystem_t *create_devfs() {
    //TODO:
    return NULL;
}

static filesystem_t *create_kvfs() {
    //TODO:
    return NULL;
}


//file's implementation

static fileops_t file_ops;

static int fileops_open(inode_t *inode, file_t *file, int flags){
    //TODO:
    return 0;
}

static ssize_t fileops_read(inode_t *inode, file_t *file, char *buf, size_t size){
    //TODO:
    return 0;
}

static ssize_t fileops_write(inode_t *inode, file_t *file, const char *buf, size_t size){
    //TODO:
    return 0;
}

static off_t fileops_lseek(inode_t *inode, file_t *file, off_t offset, int whence){
    //TODO:
    return 0;
}

static int fileops_close(inode_t *inode, file_t *file){
    //TODO:
    return 0;
}

//other helper functions

static int search_for_file_index(int fd){
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++){
        if (file_pool[i]->fd == fd){
            break;
        }
    }
    if (i == MAX_FILE_NUM){
        Log("Cannot find the fd, so lseek failed.");
        return -1;
    }
    return i;
}

// vfs initialization helper functions

static void pool_init(){
    memset(fd_pool, 0, sizeof(fd_pool));
    fd_pool[0] = fd_pool[1] = fd_pool[2] = 1;
    for (int i = 0; i < MAX_FILE_NUM; i++)
        file_pool[i] = NULL;
}

static void oop_func_init(){
    procfs_ops.init = &fsops_init;
    procfs_ops.lookup_open = &fsops_lookup_open;
    procfs_ops.lookup_access = &fsops_lookup_access;
    procfs_ops.close = &fsops_close;

    file_ops.open = &fileops_open;
    file_ops.read = &fileops_read;
    file_ops.write = &fileops_write;
    file_ops.lseek = &fileops_lseek;
    file_ops.close = &fileops_close;
}

static void fs_init(){
    create_procfs();
    create_devfs();
    create_kvfs();
    create_procinodes();
}

//vfs API

static void vfs_init(){
    pool_init();
    oop_func_init();
    fs_init();
}

static int vfs_access(const char *path, int mode){
    inode_t *open_inode = NULL;
    if (strncmp(procfs_path.name, path, strlen(procfs_path.name)) == 0){
        open_inode = procfs_path.fs->ops->lookup_access(procfs_path.fs, path, mode);
    }
    else if (strncmp(devfs_path.name, path, strlen(devfs_path.name)) == 0){
        open_inode = devfs_path.fs->ops->lookup_access(devfs_path.fs, path, mode);
    }
    else if (strncmp(kvfs_path.name, path, strlen(kvfs_path.name)) == 0){
        open_inode = kvfs_path.fs->ops->lookup_access(kvfs_path.fs, path, mode);
    }
    else{
        Log("Cannot find the path.");
        return -1;
    }

    if (open_inode != NULL){
        switch (mode){
        case R_OK:  if (open_inode->can_read == 1) return 0; else return -1;
        case W_OK:  if (open_inode->can_write == 1) return 0; else return -1;
        case X_OK:  return -1;
        case F_OK:  return 0;
        default: Log("Undefined mode."); return -1;
        }
    }
    else{
        Log("Cannot find the path you want to lookup.");
        return -1;
    }
}

static int vfs_mount(const char *path, filesystem_t *fs){
    switch (fs->fs_type){
    case PROCFS: procfs_path.fs = fs; fs->path = &procfs_path; strcpy(procfs_path.name, path); break;
    case DEVFS: devfs_path.fs = fs; fs->path = &devfs_path; strcpy(devfs_path.name, path); break;
    case KVFS: kvfs_path.fs = fs; fs->path = &kvfs_path; strcpy(kvfs_path.name, path); break;
    default: break;
    }
    return 0;
}

static int vfs_unmount(const char *path){
    if (strcmp(path, procfs_path.name) == 0){
        procfs_path.fs = NULL;
        procfs_path.fs->path = NULL;
    }
    else if (strcmp(path, devfs_path.name) == 0){
        devfs_path.fs = NULL;
        devfs_path.fs->path = NULL;
    }
    else if (strcmp(path, kvfs_path.name) == 0){
        kvfs_path.fs = NULL;
        kvfs_path.fs->path = NULL;
    }
    return 0;
}

static int vfs_open(const char *path, int flags){
    inode_t *open_inode = NULL;
    if (strncmp(procfs_path.name, path, strlen(procfs_path.name)) == 0){
        open_inode = procfs_path.fs->ops->lookup_open(procfs_path.fs, path, flags);
    }
    else if (strncmp(devfs_path.name, path, strlen(devfs_path.name)) == 0){
        open_inode = devfs_path.fs->ops->lookup_open(devfs_path.fs, path, flags);
    }
    else if (strncmp(kvfs_path.name, path, strlen(kvfs_path.name)) == 0){
        open_inode = kvfs_path.fs->ops->lookup_open(kvfs_path.fs, path, flags);
    }
    else{
        Log("Cannot find the path.");
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
            Log("file open error");
            return -1;
        }
        //TODO assign the fd is implemented in ops 
        int i;
        for (i = 3; i < MAX_FILE_NUM; i++){
            if (file_pool[i] == NULL){
                file_pool[i] = f;
            }
        }
        if (i == MAX_FILE_NUM){
            Log("File pool is full, you cannot open more files.");
            return -1;
        }
        return ret_fd;
    }
    else{
        Log("Cannot find the path you want to lookup.");
        return -1;
    }
}

static ssize_t vfs_read(int fd, void *buf, size_t nbyte){
    int file_index = search_for_file_index(fd);
    if (file_index == -1) return -1;
    return file_pool[file_index]->ops->read(file_pool[file_index]->f_inode, file_pool[file_index], buf, nbyte);
}

static ssize_t vfs_write(int fd, void *buf, size_t nbyte){
    int file_index = search_for_file_index(fd);
    if (file_index == -1) return -1;
    return file_pool[file_index]->ops->write(file_pool[file_index]->f_inode, file_pool[file_index], buf, nbyte);
}

static off_t vfs_lseek(int fd, off_t offset, int whence){
    int file_index = search_for_file_index(fd);
    if (file_index == -1) return -1;
    return file_pool[file_index]->ops->lseek(file_pool[file_index]->f_inode, file_pool[file_index], offset, whence);
}

static int vfs_close(int fd){
    int file_index = search_for_file_index(fd);
    if (file_index == -1) return -1;
    return file_pool[file_index]->ops->close(file_pool[file_index]->f_inode, file_pool[file_index]);
}