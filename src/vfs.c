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
static spinlock_t fs_lock;

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

static inode_t *fsops_lookup(struct filesystem *fs, const char *path){
    switch (fs->fs_type){
    case PROCFS:{
        for (int i = 0; i < MAX_INODE_NUM; i++){
            if (fs->inodes[i] != NULL && strcmp(fs->inodes[i]->name, path) == 0){
                return fs->inodes[i];
            }
        }
        break;
    }
    case DEVFS://TODO:
    case KVFS://TODO:
    default: break;
    }
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

static void create_procinodes() {
    inode_t *cpuinfo_inode = (inode_t *)pmm->alloc(sizeof(inode_t));
    if (!cpuinfo_inode) panic("inode allocation failed");
    cpuinfo_inode->can_read = 1;
    cpuinfo_inode->can_write = 1;
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
//    Log("cpuinfo created.\nname: %s\ncontent:\n%s\n", procfs_path.fs->inodes[i]->name, procfs_path.fs->inodes[i]->content);

    inode_t *meminfo_inode = (inode_t *)pmm->alloc(sizeof(inode_t));
    if (!meminfo_inode) panic("inode allocation failed");
    meminfo_inode->can_read = 1;
    meminfo_inode->can_write = 1;
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
 //   Log("cpuinfo created.\nname: %s\ncontent:\n%s\n", procfs_path.fs->inodes[i]->name, procfs_path.fs->inodes[i]->content);
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
    if ((inode->can_read == 0 && (flags == O_RDONLY || flags == O_RDWR))
        || (inode->can_write == 0 && (flags == O_WRONLY || flags == O_RDWR))){
        printf("R/W permission denied.\n");
        return -1;
    }
    file->open_offset = 0;
    strcpy(file->name, inode->name);
    strcpy(file->content, inode->content);
    switch (flags){
    case O_RDONLY: file->can_read = 1; file->can_write = 0; break;
    case O_WRONLY: file->can_write = 1; file->can_read = 0; break;
    case O_RDWR: file->can_read = 1; file->can_write = 1; break;
    default: panic("undefined flags."); return -1;
    }

    if (file->can_write == 1) inode->can_write = 0;
    inode->open_thread_num++;
    file->f_inode = inode;

    int new_fd;
    for (new_fd = 0; new_fd < MAX_FD_NUM; new_fd++){
        if (fd_pool[new_fd] == 0){
            fd_pool[new_fd] = 1;
            break;
        }
    }
    if (new_fd == MAX_FD_NUM){
        printf("Fd pool is full. You cannot open more file anymore.");
        return -1;
    }
    file->fd = new_fd;
    Log("File info-------\nfd: %d\tname: %s\tcan_read: %d\tcan_write: %d\n", file->fd, file->name, file->can_read, file->can_write);
    return new_fd;
}

static ssize_t fileops_read(inode_t *inode, file_t *file, char *buf, size_t size){
    if (file->can_read == 0){
        printf("Read permission denied.\n");
        return -1;
    }
    if (size + file->open_offset > strlen(file->content)){
        size = strlen(file->content) - file->open_offset;
    }
    printf("size: %d\n", size);
    int i;
    for (i = 0; i < size; i++){
        buf[i] = file->content[i + file->open_offset];
    }
    buf[i] = '\0';
    file->open_offset += size;
    return size;
}

static ssize_t fileops_write(inode_t *inode, file_t *file, const char *buf, size_t size){
    //ONLY CONSIDER THE OVER-WRITE MODE
    if (file->can_write == 0){
        printf("Write permission denied.\n");
        return -1;
    }
    if (size > MAX_INODE_CONTENT_LEN)
        size = MAX_INODE_CONTENT_LEN;
    int i;
    for (i = 0; i < size; i++){
        file->content[i] = buf[i];
        inode->content[i] = buf[i];
    }
    file->content[i] = '\0';
    inode->content[i] = '\0';
    file->open_offset = 0;
    return size;
}

static off_t fileops_lseek(inode_t *inode, file_t *file, off_t offset, int whence){
    switch (whence){
    case SEEK_SET: file->open_offset = offset; break;
    case SEEK_CUR: file->open_offset += offset; break;
    case SEEK_END: file->open_offset = strlen(file->content) + offset; break;
    default: panic("Undefined whence.");
    }

    if (file->open_offset > strlen(file->content))
        file->open_offset = strlen(file->content);
    return file->open_offset;
}

static int fileops_close(inode_t *inode, file_t *file){
    inode->open_thread_num--;
    inode->can_read = 1;
    inode->can_write = 1;
    fd_pool[file->fd] = 0;
    for (int i = 0; i < MAX_FILE_NUM; i++){
        if (file_pool[i] != NULL && file_pool[i]->fd == file->fd){
            file_pool[i] = NULL;
            break;
        }
    }
    pmm->free(file);
    return 0;
}

//other helper functions

static int search_for_file_index(int fd){
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++){
        if (file_pool[i] != NULL && file_pool[i]->fd == fd){
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

static void spinlock_init(){
    kmt->spin_init(&fs_lock, "fs_lock");
}

static void pool_init(){
    memset(fd_pool, 0, sizeof(fd_pool));
    fd_pool[0] = fd_pool[1] = fd_pool[2] = 1;
    for (int i = 0; i < MAX_FILE_NUM; i++)
        file_pool[i] = NULL;
}

static void oop_func_init(){
    procfs_ops.init = &fsops_init;
    procfs_ops.lookup = &fsops_lookup;
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
    spinlock_init();
    pool_init();
    oop_func_init();
    fs_init();
}

static int vfs_access(const char *path, int mode){
    kmt->spin_lock(&fs_lock);
    inode_t *open_inode = NULL;
    if (strncmp(procfs_path.name, path, strlen(procfs_path.name)) == 0){
        open_inode = procfs_path.fs->ops->lookup(procfs_path.fs, path);
    }
    else if (strncmp(devfs_path.name, path, strlen(devfs_path.name)) == 0){
        open_inode = devfs_path.fs->ops->lookup(devfs_path.fs, path);
    }
    else if (strncmp(kvfs_path.name, path, strlen(kvfs_path.name)) == 0){
        open_inode = kvfs_path.fs->ops->lookup(kvfs_path.fs, path);
    }
    else{
        Log("Cannot find the path.");
        kmt->spin_unlock(&fs_lock);
        return -1;
    }

    if (open_inode != NULL){
        switch (mode){
        case R_OK:  if (open_inode->can_read == 1){
            kmt->spin_unlock(&fs_lock);
            return 0;
        }else{
            kmt->spin_unlock(&fs_lock);
            return -1;
        }
        case W_OK:  if (open_inode->can_write == 1){
            kmt->spin_unlock(&fs_lock);
            return 0;
        }else{
            kmt->spin_unlock(&fs_lock);
            return -1;
        }
        case X_OK:  kmt->spin_unlock(&fs_lock); return -1;
        case F_OK:  kmt->spin_unlock(&fs_lock); return 0;
        default: Log("Undefined mode."); kmt->spin_unlock(&fs_lock); return -1;
        }
    }
    else{
        Log("Cannot find the path you want to lookup.");
        kmt->spin_unlock(&fs_lock);
        return -1;
    }
}

static int vfs_mount(const char *path, filesystem_t *fs){
    kmt->spin_lock(&fs_lock);
    switch (fs->fs_type){
    case PROCFS: procfs_path.fs = fs; fs->path = &procfs_path; strcpy(procfs_path.name, path); break;
    case DEVFS: devfs_path.fs = fs; fs->path = &devfs_path; strcpy(devfs_path.name, path); break;
    case KVFS: kvfs_path.fs = fs; fs->path = &kvfs_path; strcpy(kvfs_path.name, path); break;
    default: break;
    }
    kmt->spin_unlock(&fs_lock);
    return 0;
}

static int vfs_unmount(const char *path){
    kmt->spin_lock(&fs_lock);
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
    kmt->spin_unlock(&fs_lock);
    return 0;
}

static int vfs_open(const char *path, int flags){
    kmt->spin_lock(&fs_lock);
    inode_t *open_inode = NULL;
    if (strncmp(procfs_path.name, path, strlen(procfs_path.name)) == 0){
        open_inode = procfs_path.fs->ops->lookup(procfs_path.fs, path);
    }
    else if (strncmp(devfs_path.name, path, strlen(devfs_path.name)) == 0){
        open_inode = devfs_path.fs->ops->lookup(devfs_path.fs, path);
    }
    else if (strncmp(kvfs_path.name, path, strlen(kvfs_path.name)) == 0){
        open_inode = kvfs_path.fs->ops->lookup(kvfs_path.fs, path);
    }
    else{
        Log("Cannot find the path.");
        kmt->spin_unlock(&fs_lock);
        return -1;
    }

    if (open_inode != NULL){
        int ret_fd = -1;
        file_t *f = (file_t *)pmm->alloc(sizeof(file_t));
        if (!f) {
            panic("file allocation failed");
            kmt->spin_unlock(&fs_lock);
            return -1;
        }
        f->ops = &file_ops;
        ret_fd = f->ops->open(open_inode, f, flags);
        if (ret_fd < 0){
            Log("file open error");
            kmt->spin_unlock(&fs_lock);
            return -1;
        }
        int i;
        for (i = 3; i < MAX_FILE_NUM; i++){
            if (file_pool[i] == NULL){
                file_pool[i] = f;
                break;
            }
        }
        if (i == MAX_FILE_NUM){
            Log("File pool is full, you cannot open more files.");
            kmt->spin_unlock(&fs_lock);
            return -1;
        }
        kmt->spin_unlock(&fs_lock);
        return ret_fd;
    }
    else{
        Log("Cannot find the path you want to lookup.");
        kmt->spin_unlock(&fs_lock);
        return -1;
    }
}

static ssize_t vfs_read(int fd, void *buf, size_t nbyte){
    kmt->spin_lock(&fs_lock);
    int file_index = search_for_file_index(fd);
    if (file_index == -1){
        kmt->spin_unlock(&fs_lock);
        return -1;
    }
    ssize_t ret = file_pool[file_index]->ops->read(file_pool[file_index]->f_inode, file_pool[file_index], buf, nbyte);
    kmt->spin_unlock(&fs_lock);
    return ret;
}

static ssize_t vfs_write(int fd, void *buf, size_t nbyte){
    kmt->spin_lock(&fs_lock);
    int file_index = search_for_file_index(fd);
    if (file_index == -1){
        kmt->spin_unlock(&fs_lock);
        return -1;
    }
    ssize_t ret = file_pool[file_index]->ops->write(file_pool[file_index]->f_inode, file_pool[file_index], buf, nbyte);
    kmt->spin_unlock(&fs_lock);
    return ret;
}

static off_t vfs_lseek(int fd, off_t offset, int whence){
    kmt->spin_lock(&fs_lock);
    int file_index = search_for_file_index(fd);
    if (file_index == -1){
        kmt->spin_unlock(&fs_lock);
        return -1;
    }
    off_t ret = file_pool[file_index]->ops->lseek(file_pool[file_index]->f_inode, file_pool[file_index], offset, whence);
    kmt->spin_unlock(&fs_lock);
    return ret;
}

static int vfs_close(int fd){
    kmt->spin_lock(&fs_lock);
    int file_index = search_for_file_index(fd);
    if (file_index == -1) {
        kmt->spin_unlock(&fs_lock);
        return -1;
    }
    int ret =  file_pool[file_index]->ops->close(file_pool[file_index]->f_inode, file_pool[file_index]);
    kmt->spin_unlock(&fs_lock);
    return ret;
}

//Helper functions for debugging

void print_proc_inodes(){
    filesystem_t *fs = procfs_path.fs;
    int i;
    for (i = 0; i < MAX_INODE_NUM; i++){
        if (fs->inodes[i] != NULL){
            printf("name: %s\ncontent:\n%s\n", procfs_path.fs->inodes[i]->name, procfs_path.fs->inodes[i]->content);
        }
    }
}
