#ifndef STRING_H
#define STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void *memcpy(void *str1, const void *str2, size_t n);
int strncmp(const char *str1, const char *str2, size_t n);
void *memset(void *str, int c, size_t n);
size_t strlen(const char *str);

#ifdef __cplusplus
}
#endif

#endif