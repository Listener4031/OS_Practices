#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

void fail(const char*, const char*, int);

inline void fail(const char* message, const char* function, int line){
    printf("[x] Test failed at %s: %d: %s\n", function, line, message);
    exit(-1);
}

#define assert_msg(cond, fmt, ...) assert((cond) || !fprintf(stderr, (fmt), ##__VA_ARGS__))