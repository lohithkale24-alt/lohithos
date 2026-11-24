#include "types.h"
#include <stdarg.h>

void *memset(void *dest, int val, size_t count) {
    uint8_t *d = (uint8_t *)dest;
    for (size_t i = 0; i < count; i++) {
        d[i] = (uint8_t)val;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t count) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        for (size_t i = 0; i < count; i++) d[i] = s[i];
    } else {
        for (size_t i = count; i > 0; i--) d[i - 1] = s[i - 1];
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0') {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}

char *strcat(char *dest, const char *src) {
    char *ret = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return ret;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    if (c == 0) return (char *)s;
    return NULL;
}

static void itoa_dec(int val, char *buf) {
    char tmp[32];
    int i = 0, j = 0;
    bool neg = false;
    if (val < 0) {
        neg = true;
        val = -val;
    } else if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0) {
        tmp[i++] = (val % 10) + '0';
        val /= 10;
    }
    if (neg) buf[j++] = '-';
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

static void utoa_hex(uint32_t val, char *buf, bool upper) {
    char tmp[32];
    int i = 0, j = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0) {
        tmp[i++] = digits[val & 0xF];
        val >>= 4;
    }
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

void sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *out = buf;
    char temp[64];

    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            if (*fmt == 's') {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                while (*s) *out++ = *s++;
            } else if (*fmt == 'd' || *fmt == 'i') {
                int d = va_arg(args, int);
                itoa_dec(d, temp);
                char *p = temp;
                while (*p) *out++ = *p++;
            } else if (*fmt == 'u') {
                uint32_t u = va_arg(args, uint32_t);
                itoa_dec((int)u, temp);
                char *p = temp;
                while (*p) *out++ = *p++;
            } else if (*fmt == 'x' || *fmt == 'X') {
                uint32_t x = va_arg(args, uint32_t);
                utoa_hex(x, temp, (*fmt == 'X'));
                char *p = temp;
                while (*p) *out++ = *p++;
            } else if (*fmt == 'c') {
                char c = (char)va_arg(args, int);
                *out++ = c;
            } else if (*fmt == '%') {
                *out++ = '%';
            }
        } else {
            *out++ = *fmt;
        }
        fmt++;
    }
    *out = '\0';
    va_end(args);
}

int atoi(const char *str) {
    int res = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}
