#include <setjmp.h>
#include <stdio.h>

static jmp_buf bailout_env;

static void deep(int depth) {
    if (depth == 3) {
        puts("  deep: fatal condition, jumping out");
        longjmp(bailout_env, 1);          /* never returns */
    }
    printf("  deep: depth=%d, entering\n", depth);
    deep(depth + 1);
    printf("  deep: depth=%d, cleanup runs\n", depth);   /* skipped! */
}

int main(void) {
    if (setjmp(bailout_env) == 0) {       /* first return: 0 */
        puts("run request");
        deep(0);
        puts("request finished normally");
    } else {                              /* returns again: 1 */
        puts("bailout: back at the top");
    }
    return 0;
}
