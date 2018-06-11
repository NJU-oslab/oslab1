#include "libc.h"
#include "os.h"

char *strcat(char *s1, char *s2)  
{  
    assert(s1 != NULL && s2 != NULL);
    char *pt = s1;  
    while(*s1 != '\0') s1++;  
    while(*s2 != '\0') *s1++ = *s2++;  
    *s1 = '\0';  
    return pt;  
}