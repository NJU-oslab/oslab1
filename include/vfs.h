#include "os.h"

typedef struct inode{

} inode_t;

typedef struct fsops {
  void (*init)(struct filesystem *fs, const char *name, inode_t *dev);
  inode_t *(*lookup)(struct filesystem *fs, const char *path, int flags);
  int (*close)(inode_t *inode);
} fsops_t;

typedef struct filesystem{
    fsops_t *ops;
} filesystem_t;