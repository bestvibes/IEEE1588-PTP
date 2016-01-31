#ifndef ENDIAN_IMPORTED
#define ENDIAN_IMPORTED

#include <stdint.h>
#include <limits.h>

#if CHAR_BIT != 8
	#error "unsupported char size"
#endif

union uint48_t {
    char c[6];
    uint64_t v:48;
};

int is_little_endian()
{
    union {
        uint32_t i;
        char c[4];
    } u = {0x01020304};

    return u.c[0] == 4; 
}

char *hton(char *in, long size, char *out) {
    if(is_little_endian()) {
        int i;
        for(i = 0; i < size; i++) {
            out[size-1-i] = in[i];
        }
    }
    else {
        out = in;
    }
}

char *hton2(void *in, long size, char *out) {
    if(is_little_endian()) {
        int i;
        for(i = 0; i < size; i++) {
            out[size-1-i] = *(uint64_t *)in >> 8*i;
        }
    }
    else {
        out = in;
    }
}
#endif