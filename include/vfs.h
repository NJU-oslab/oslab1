#include <os.h>
#define MAX_FS_NAME_LEN 128
#define MAX_INODE_NAME_LEN 128
#define MAX_INODE_CONTENT_LEN 1024
#define MAX_FD_NUM 4096
#define MAX_PATH_LEN 128
#define MAX_FILE_NUM 512
#define MAX_INODE_NUM 512
#define MAX_DEV_NUM 32

typedef struct fsops fsops_t;
typedef struct fileops fileops_t;
typedef struct file file_t;
typedef struct inode inode_t;
typedef struct filesystem filesystem_t;
typedef struct mount_path mount_path_t;

enum open_flags{
    O_RDONLY, O_WRONLY, O_RDWR
};

enum fstype{
    PROCFS, DEVFS, KVFS
};

struct fsops {
    void (*init)(filesystem_t *fs, const char *name);
    inode_t *(*lookup)(filesystem_t *fs, const char *path);
    int (*close)(inode_t *inode);
};

struct fileops {
    int (*open)(inode_t *inode, file_t *file, int flags);
    ssize_t (*read)(inode_t *inode, file_t *file, char *buf, size_t size);
    ssize_t (*write)(inode_t *inode, file_t *file, const char *buf, size_t size);
    off_t (*lseek)(inode_t *inode, file_t *file, off_t offset, int whence);
    int (*close)(inode_t *inode, file_t *file);
};

struct file{
    char name[MAX_INODE_NAME_LEN];
    char content[MAX_INODE_CONTENT_LEN];
    fileops_t *ops;
    inode_t *f_inode;
    int open_offset;
    int fd;
    char can_read;
    char can_write;
};

struct inode{
    char name[MAX_INODE_NAME_LEN];
    char content[MAX_INODE_CONTENT_LEN];
    filesystem_t *fs;
    int open_thread_num;
    char can_read;
    char can_write;
    int pid;
};

struct filesystem{
    char name[MAX_FS_NAME_LEN];
    mount_path_t *path;
    fsops_t *ops;
    int fs_type;
    inode_t *inodes[MAX_INODE_NUM];
};

struct mount_path{
    char name[MAX_PATH_LEN];
    filesystem_t *fs;
};