#include "libc.h"

char * strcpy(char *dst, const char *src) {
  assert(dst != NULL);
  assert(src != NULL);
  char * ret = dst;
  while ((*dst++ = *src++) != '\0');
  assert(src != NULL);
  return ret;
}

