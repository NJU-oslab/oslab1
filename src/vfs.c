#include "os.h"

/* vfs API*/
static void vfs_init();
static int vfs_access(const char *path, int mode);
static int vfs_mount(const char *path, filesystem_t *fs);
static int vfs_unmount(const char *path);
static int vfs_open(const char *path, int flags);
static ssize_t vfs_read(int fd, void *buf, size_t nbyte);
static ssize_t vfs_write(int fd, void *buf, size_t nbyte);
static off_t vfs_lseek(int fd, off_t offset, int whence);
static int vfs_close(int fd);

/* fs's operations*/
static void fsops_init(struct filesystem *fs, const char *name);
static inode_t *fsops_lookup(struct filesystem *fs, const char *path);
static int fsops_close(inode_t *inode);

/* create inodes*/
static void create_inodes(filesystem_t *fs, char can_read, char can_write, char *inode_name, char *content, mount_path_t *path);
static void create_procinodes(filesystem_t *fs);
static void create_kvinodes(filesystem_t *fs);
static void create_devinodes(filesystem_t *fs);

/* create filesystem*/
static filesystem_t *create_procfs();
static filesystem_t *create_devfs();
static filesystem_t *create_kvfs();

/* file's operations */
static int fileops_open(inode_t *inode, file_t *file, int flags);
static ssize_t fileops_read(inode_t *inode, file_t *file, char *buf, size_t size);
static ssize_t fileops_write(inode_t *inode, file_t *file, const char *buf, size_t size);
static off_t fileops_lseek(inode_t *inode, file_t *file, off_t offset, int whence);
static int fileops_close(inode_t *inode, file_t *file);

/* helper functions*/
static int search_for_file_index(int fd);
static void mount_new_fs(mount_path_t *path, filesystem_t *fs);
static void unmount_fs(mount_path_t *path);

/* initialization*/
static void spinlock_init();
static void path_init();
static void rand_init();
static void pool_init();
static void oop_func_init();
static void fs_init();

/* debugging*/
void print_proc_inodes();



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

fsops_t procfs_ops;
fsops_t devfs_ops;
fsops_t kvfs_ops;

static void fsops_init(struct filesystem *fs, const char *name){
    TRACE_ENTRY;
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
    for (int i = 0; i < MAX_INODE_NUM; i++)
        fs->inodes[i] = NULL;
    fs->next_fs_under_same_path = NULL;
    TRACE_EXIT;
}

static inode_t *fsops_lookup(struct filesystem *fs, const char *path){
    TRACE_ENTRY;
    for (int i = 0; i < MAX_INODE_NUM; i++){
        if (fs->inodes[i] != NULL && strcmp(fs->inodes[i]->name, path) == 0){
            TRACE_EXIT;
            return fs->inodes[i];
        }
    }
    TRACE_EXIT;
    return NULL;
}

static int fsops_close(inode_t *inode){
    return 0;
}


static void create_procinodes(filesystem_t *fs) {
    TRACE_ENTRY;
    create_inodes(fs, 1, 0, "/cpuinfo", "This is the cpuinfo file.\n", &procfs_path);
    create_inodes(fs, 1, 0, "/meminfo", "This is the meminfo file.\n", &procfs_path);
    TRACE_EXIT;
}

static void create_kvinodes(filesystem_t *fs){
    create_inodes(fs, 1, 1, "a.txt", NULL, &kvfs_path);
}

static void create_devinodes(filesystem_t *fs){
    create_inodes(fs, 1, 1, "/null", NULL, &devfs_path);
    create_inodes(fs, 1, 1, "/zero", NULL, &devfs_path);
    create_inodes(fs, 1, 1, "/random", NULL, &devfs_path);
}

static filesystem_t *create_procfs() {
    filesystem_t *fs = (filesystem_t *)pmm->alloc(sizeof(filesystem_t));
    if (!fs) panic("fs allocation failed");
    fs->ops = &procfs_ops;
    fs->ops->init(fs, "procfs");
    vfs->mount("/proc", fs);
    return fs;
}

static filesystem_t *create_devfs() {
    filesystem_t *fs = (filesystem_t *)pmm->alloc(sizeof(filesystem_t));
    if (!fs) panic("fs allocation failed");
    fs->ops = &devfs_ops;
    fs->ops->init(fs, "devfs");
    vfs->mount("/dev", fs);
    return fs;
}

static filesystem_t *create_kvfs() {
    filesystem_t *fs = (filesystem_t *)pmm->alloc(sizeof(filesystem_t));
    if (!fs) panic("fs allocation failed");
    fs->ops = &kvfs_ops;
    fs->ops->init(fs, "kvfs");
    vfs->mount("/", fs);
    return fs;
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
    default: printf("undefined flags."); return -1;
    }

    if (file->can_write == 1) {
        inode->can_write = 0;
        inode->can_read = 0;
    }
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
        printf("Fd pool is full. You cannot open more file anymore.\n");
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
    if (inode->fs->fs_type == DEVFS){
        if (strcmp(inode->name, "/dev/null") == 0){
            return -1;
        }
        else if (strcmp(inode->name, "/dev/zero") == 0){
            if (size + file->open_offset > MAX_INODE_CONTENT_LEN)
                size = strlen(file->content) - file->open_offset;
            for (int i = 0; i < size; i++)
                buf[i] = 0;
            file->open_offset += size;
            return size;
        }
        else if (strcmp(inode->name, "/dev/random") == 0){
            if (size + file->open_offset > MAX_INODE_CONTENT_LEN)
                size = strlen(file->content) - file->open_offset;
            for (int i = 0; i < size; i++)
                buf[i] = rand() % 256 - 128;
            file->open_offset += size;
            return size;
        }
    }
    if (size + file->open_offset > strlen(file->content)){
        size = strlen(file->content) - file->open_offset;
    }
    Log("size: %d\n", size);
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
    if (inode->fs->fs_type == DEVFS){
        if (strcmp(inode->name, "/dev/null") == 0){
            return 0;
        }
        else if (strcmp(inode->name, "/dev/zero") == 0){
            return 0;
        }
        else if (strcmp(inode->name, "/dev/random") == 0){
            return 0;
        }
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
    default: printf("Undefined whence.\n"); return -1;
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
        printf("Cannot find the fd.\n");
        return -1;
    }
    return i;
}

static void create_inodes(filesystem_t *fs, char can_read, char can_write, char *inode_name, char *content, mount_path_t *path){
    inode_t *new_inode = (inode_t *)pmm->alloc(sizeof(inode_t));
    if (!new_inode) panic("inode allocation failed");
    new_inode->can_read = can_read;
    new_inode->can_write = can_write;
    new_inode->open_thread_num = 0;
    new_inode->fs = fs;
    new_inode->thread = NULL;
    strcpy(new_inode->name, path->name);
    strcat(new_inode->name, inode_name);
    if (content != NULL)
        strcpy(new_inode->content, content);
    int i;
    for (i = 0; i < MAX_INODE_NUM; i++){
        if (path->fs->inodes[i] == NULL){
            path->fs->inodes[i] = new_inode;
            break;
        }
    }
    if (i == MAX_INODE_NUM){
        printf("The fs's inode pool is full.\n");
        return;
    }
}

static void mount_new_fs(mount_path_t *path, filesystem_t *fs){
    if (path->fs == NULL){
        fs->next_fs_under_same_path = NULL;
        path->fs = fs;
    }
    else{
        fs->next_fs_under_same_path = path->fs;
        path->fs = fs;
    }
}

static void unmount_fs(mount_path_t *path){
    if (path->fs == NULL){
        printf("The path has no filesystem mounted on it. Unmounting failed.\n");
    }
    else{
        path->fs = path->fs->next_fs_under_same_path;
    }
}

// vfs initialization helper functions 

static void spinlock_init(){
    kmt->spin_init(&fs_lock, "fs_lock");
}

static void path_init(){
    strcpy(procfs_path.name, "/proc");
    strcpy(devfs_path.name ,"/dev");
    strcpy(kvfs_path.name, "/");
}

static void rand_init(){
    srand(uptime());
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

    kvfs_ops.init = &fsops_init;
    kvfs_ops.lookup = &fsops_lookup;
    kvfs_ops.close = &fsops_close;

    devfs_ops.init = &fsops_init;
    devfs_ops.lookup = &fsops_lookup;
    devfs_ops.close = &fsops_close;

    file_ops.open = &fileops_open;
    file_ops.read = &fileops_read;
    file_ops.write = &fileops_write;
    file_ops.lseek = &fileops_lseek;
    file_ops.close = &fileops_close;
}

static void fs_init(){
    filesystem_t *fs;
    fs = create_procfs();
    create_procinodes(fs);
    fs = create_devfs();
    create_devinodes(fs);
    fs = create_kvfs();
    create_kvinodes(fs);
}

//vfs API

static void vfs_init(){
    spinlock_init();
    path_init();
    rand_init();
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
        printf("Cannot find the path.\n");
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
        default: printf("Undefined mode.\n"); kmt->spin_unlock(&fs_lock); return -1;
        }
    }
    else{
        printf("Cannot find the path you want to lookup.\n");
        kmt->spin_unlock(&fs_lock);
        return -1;
    }
}

static int vfs_mount(const char *path, filesystem_t *fs){
    kmt->spin_lock(&fs_lock);
    if (strcmp(path, procfs_path.name) == 0 && fs->fs_type == PROCFS)
        mount_new_fs(&procfs_path, fs);
    else if (strcmp(path, devfs_path.name) == 0 && fs->fs_type == DEVFS)
        mount_new_fs(&devfs_path, fs);
    else if (strcmp(path, kvfs_path.name) == 0 && fs->fs_type == KVFS)
        mount_new_fs(&kvfs_path, fs);
    kmt->spin_unlock(&fs_lock);
    return 0;
}

static int vfs_unmount(const char *path){
    kmt->spin_lock(&fs_lock);
    if (strcmp(path, procfs_path.name) == 0){
        unmount_fs(&procfs_path);
    }
    else if (strcmp(path, devfs_path.name) == 0){
        unmount_fs(&devfs_path);
    }
    else if (strcmp(path, kvfs_path.name) == 0){
        unmount_fs(&kvfs_path);
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
        printf("Cannot find the path.\n");
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
            printf("file open error.\n");
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
            printf("File pool is full, you cannot open more files.\n");
            kmt->spin_unlock(&fs_lock);
            return -1;
        }
        kmt->spin_unlock(&fs_lock);
        return ret_fd;
    }
    else{
        printf("Cannot find the path you want to lookup.\n");
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
    if (!fs) return;
    int i;
    for (i = 0; i < MAX_INODE_NUM; i++){
        if (fs->inodes[i] != NULL){
            printf("name: %s\ncontent:\n%s\n", procfs_path.fs->inodes[i]->name, procfs_path.fs->inodes[i]->content);
        }
    }
}