#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char *mem; size_t used, cap; } arena_t;
static arena_t arena;                      /* per-request memory pool */
static jmp_buf bailout_env;

typedef struct { void (*on_close)(const char *); char tag[24]; } upload_t;
static upload_t *g_upload;                 /* process-wide, lives across requests */

static void *arena_alloc(size_t n) {
    /* Invariant: arena.used <= arena.cap */
    if (n > arena.cap - arena.used) longjmp(bailout_env, 1);   /* quota exceeded */
    void *p = arena.mem + arena.used;
    arena.used += n;
    return p;
}

static void legit_close(const char *t) { printf("  close(%s)\n", t); }

static void begin_request(size_t limit) {
    if (g_upload)                          /* "flush the previous upload state" */
        g_upload->on_close(g_upload->tag); /* (3) trusts g_upload blindly */
    arena.mem = calloc(1, limit);
    if (!arena.mem) longjmp(bailout_env, 1);
    arena.cap = limit;
    arena.used = 0;
}

static void parse_upload(size_t payload) {
    g_upload = arena_alloc(sizeof *g_upload);     /* (1) allocated from the arena */
    g_upload->on_close = legit_close;
    strncpy(g_upload->tag, "progress", sizeof(g_upload->tag) - 1);
    g_upload->tag[sizeof(g_upload->tag) - 1] = '\0';
    arena_alloc(payload);                         /* OOM here => longjmp */
}

static void finish_request(void) {                /* normal path only */
    g_upload->on_close(g_upload->tag);
    g_upload = NULL;                              /* (2) reset ... only on this path */
}

static void run(const char *name, size_t limit, size_t payload) {
    printf("[%s]\n", name);
    if (setjmp(bailout_env) == 0) {
        begin_request(limit);
        parse_upload(payload);
        finish_request();
    } else {
        puts("  bailout: finish_request() skipped");
    }
    if (arena.mem) {
        free(arena.mem);
        arena.mem = NULL;
    }
    memset(&arena, 0, sizeof arena);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    run("request 1: normal", 4096, 8);
    run("request 2: attacker exceeds the limit", 64, 1000);
    run("request 3: innocent victim", 4096, 8);   /* begin_request reads stale g_upload */
    return 0;
}
