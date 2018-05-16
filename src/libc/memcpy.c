#include "libc.h"
#include "os.h"

void * memcpy(void *dst, const void *src, size_t n) {
  if (dst == NULL || src == NULL)
	return NULL;
  char * tmp_dst = (char *)dst;
  char * tmp_src = (char *)src;
  while (n--) {
	*tmp_dst++ = *tmp_src++;
  }
  return dst;
}
