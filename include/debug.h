#include "os.h"

#define assert(cond) \
    do { \
      if (!(cond)) { \
        printf("Assertion fail at %s:%d\n", __FILE__, __LINE__); \
        _halt(1); \
      } \
    } while (0)

#define TRACE_ENTRY \
    printf("[trace] %s:entry\n", __func__)
#define TRACE_EXIT \
    printf("[trace] %s:exit\n", __func__)
