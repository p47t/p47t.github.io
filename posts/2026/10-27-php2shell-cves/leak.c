#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int active; char *buf; } session_t;
static session_t *g_session;
static jmp_buf bailout_env;
static int alloc_count = 0;
static int fail_at = 0;

static void *tracked_alloc(size_t n) {
    alloc_count++;
    if (fail_at > 0 && alloc_count == fail_at) {
        puts("  bailout: request aborted");
        longjmp(bailout_env, 1);
    }
    void *p = calloc(1, n);
    if (!p) {
        puts("  bailout: allocation failed");
        longjmp(bailout_env, 1);
    }
    return p;
}

static void handle_request(const char *name) {
    g_session = tracked_alloc(sizeof *g_session);
    g_session->active = 1;                 /* (1) marked active FIRST */
    g_session->buf = NULL;
    g_session->buf = tracked_alloc(64);    /* (2) may longjmp away here */
    strncpy(g_session->buf, name, 63);
    g_session->buf[63] = '\0';
}

int main(void) {
    fail_at = 2; /* Fail on the second allocation (buf) */
    puts("[req-A] start");
    if (setjmp(bailout_env) == 0) {
        handle_request("alice");
    }
    printf("after request 0: g_session->active=%d buf=%p\n",
           g_session ? g_session->active : -1,
           g_session ? (void *)g_session->buf : NULL);
    if (g_session) free(g_session);
    return 0;
}
