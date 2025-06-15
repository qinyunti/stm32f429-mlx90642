#include <string.h>
#include <stdint.h>

void *memcpy(void *str1, const void *str2, size_t n){
    const uint8_t* src = str2;
    uint8_t* dst = str1;
    for(int i=0; i<n; i++){
        *dst++ = *src++;
    }
    return str1;
}

int strncmp(const char *str1, const char *str2, size_t n){
    const uint8_t* s1 = (const uint8_t*)str1;
    const uint8_t* s2 = (const uint8_t*)str2;
    for(int i=0; i<n; i++){
        if(*s1 > *s2){
            return 1;
        }else if(*s1 < *s2){
            return -1;
        }
    }
    return 0;
}

void *memset(void *str, int c, size_t n){
    uint8_t* p = (uint8_t*)str;
    for(int i=0;i<n;i++){
        p[i]=(uint8_t)c;
    }
    return str;
}

size_t strlen(const char *str){
    const char* ps = str;
    char* pe = (char*)str;
    while(*pe++);
    return pe - ps -1;
}