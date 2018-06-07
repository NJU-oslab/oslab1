#include "os.h"
#define MAX_FS_NAME_LEN 128
#define MAX_INODE_NAME_LEN 128
#define MAX_INODE_CONTENT_LEN 1024

enum open_flags{
  O_RDONLY, O_WRONLY, O_RDWR
};

typedef struct fsops {
    void (*init)(struct filesystem *fs, const char *name, inode_t *dev);
    inode_t *(*lookup)(struct filesystem *fs, const char *path, int flags);
    int (*close)(inode_t *inode);
} fsops_t;

typedef struct fileops {
    int (*open)(inode_t *inode, file_t *file, int flags);
    ssize_t (*read)(inode_t *inode, file_t *file, char *buf, size_t size);
    ssize_t (*write)(inode_t *inode, file_t *file, const char *buf, size_t size);
    off_t (*lseek)(inode_t *inode, file_t *file, off_t offset, int whence);
} fileops_t;


typedef struct file{
    fileops_t *ops;
} file_t;

typedef struct inode{
    char name[MAX_INODE_NAME_LEN];
    char content[MAX_INODE_CONTENT_LEN];
} inode_t;

typedef struct filesystem{
    char name[MAX_FS_NAME_LEN];
    fsops_t *ops;
} filesystem_t;