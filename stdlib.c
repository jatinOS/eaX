#include "kernel.h"

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

char *strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n && src[i]) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i++] = 0;
    }
    return dest;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return n ? (*s1 - *s2) : 0;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c) return (char *)s;
        s++;
    }
    return NULL;
}

char *strrchr(const char *s, int c) {
    char *last = NULL;
    while (*s) {
        if (*s == c) last = (char *)s;
        s++;
    }
    return last;
}

char *strcat(char *dest, const char *src) {
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return dest;
}

int atoi(const char *s) {
    int result = 0;
    int sign = 1;
    
    while (*s == ' ') s++;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    
    return result * sign;
}

long strtol(const char *nptr, char **endptr, int base) {
    long result = 0;
    int negative = 0;
    
    while (*nptr == ' ' || *nptr == '\t') nptr++;
    
    if (*nptr == '-') {
        negative = 1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }
    
    if (base == 0) {
        if (nptr[0] == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) {
            base = 16;
            nptr += 2;
        } else if (nptr[0] == '0') {
            base = 8;
            nptr++;
        } else {
            base = 10;
        }
    }
    
    while (*nptr) {
        int digit;
        if (*nptr >= '0' && *nptr <= '9') {
            digit = *nptr - '0';
        } else if (*nptr >= 'a' && *nptr <= 'f') {
            digit = *nptr - 'a' + 10;
        } else if (*nptr >= 'A' && *nptr <= 'F') {
            digit = *nptr - 'A' + 10;
        } else {
            break;
        }
        
        if (digit >= base) break;
        
        result = result * base + digit;
        nptr++;
    }
    
    if (endptr) *endptr = (char *)nptr;
    return negative ? -result : result;
}

void *malloc(size_t size) {
    if (size == 0) return NULL;
    
    static uint8_t heap[16777216] = {0};
    static size_t heap_used = 0;
    
    if (heap_used + size > sizeof(heap)) {
        return NULL;
    }
    
    void *ptr = &heap[heap_used];
    heap_used += size;
    
    return ptr;
}

void free(void *ptr) {
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    void *new_ptr = malloc(size);
    if (new_ptr && ptr) {
        memcpy(new_ptr, ptr, size);
    }
    return new_ptr;
}

int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int isalpha(int c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int isalnum(int c) {
    return (isalpha(c) || isdigit(c));
}

int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

int isupper(int c) {
    return (c >= 'A' && c <= 'Z');
}

int islower(int c) {
    return (c >= 'a' && c <= 'z');
}

int tolower(int c) {
    if (isupper(c)) return c + 32;
    return c;
}

int toupper(int c) {
    if (islower(c)) return c - 32;
    return c;
}

int printf(const char *format, ...) {
    return 0;
}

int sprintf(char *str, const char *format, ...) {
    return 0;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    return 0;
}

int vsprintf(char *str, const char *format, void *ap) {
    return 0;
}

int vsnprintf(char *str, size_t size, const char *format, void *ap) {
    return 0;
}

int puts(const char *s) {
    return 0;
}

int putchar(int c) {
    return c;
}

int fputc(int c, void *stream) {
    return c;
}

size_t fread(void *ptr, size_t size, size_t nmemb, void *stream) {
    return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, void *stream) {
    return nmemb;
}

int fopen(const char *filename, const char *mode) {
    return 0;
}

int fclose(int stream) {
    return 0;
}

int fseek(int stream, long offset, int whence) {
    return 0;
}

long ftell(int stream) {
    return 0;
}

void rewind(int stream) {
}

int abs(int j) {
    return (j < 0) ? -j : j;
}

long labs(long j) {
    return (j < 0) ? -j : j;
}

uint32_t rand(void) {
    static uint32_t seed = 1;
    seed = seed * 1103515245 + 12345;
    return (seed >> 16) & 0x7fff;
}

void srand(uint32_t s) {
}

int qsort_compare(const void *a, const void *b, int (*compar)(const void *, const void *)) {
    return compar(a, b);
}

void qsort(void *base, size_t nmemb, size_t size,
          int (*compar)(const void *, const void *)) {
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *)) {
    return NULL;
}
