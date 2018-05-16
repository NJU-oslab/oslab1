#include "libc.h"
#include "os.h"

char * strncpy(char *dst, const char *src, size_t n) {
  assert(dst != NULL);
  assert(src != NULL);
  char * ret = dst;
  while (n-- && (*dst++ = *src++) != '\0');
  assert(src != NULL);
  return ret;
}
