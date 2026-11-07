#include <setjmp.h>
#include <stdio.h>

int main(void) {
    jmp_buf env;
    int plain = 0;
    volatile int safe = 0;
    if (setjmp(env) == 0) {
        plain = 1;
        safe = 1;
        longjmp(env, 1);
    }
    printf("plain=%d  safe=%d\n", plain, safe);
    return 0;
}
