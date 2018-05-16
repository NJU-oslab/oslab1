#include "libc.h"
#include "am.h"

void * memset(void *b, int c, size_t n) {
  if (n < 0 || b == NULL) return NULL;
  char * tmp_b = (char *)b;
  while (n--) {
	*tmp_b = (char)c;
	tmp_b++;
  }
  *tmp_b = '\0';
  return b;
}
