#include "libc.h"
#include "am.h"

int strncmp(const char *s1, const char *s2, size_t n) {
  assert(s1 != NULL && s2 != NULL && n >= 0);
  while (n-- && *s1 && *s2) {
	if (*s1 < *s2) return -1;
	if (*s1 > *s2) return 1;
	s1++; s2++;
  }
  return 0;
}
