#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char *mem; size_t used, cap; } arena_t;
static arena_t arena;
static jmp_buf bailout_env;
typedef struct { void (*on_close)(const char *); char tag[24]; } upload_t;
static upload_t *g_upload;

static void *arena_alloc(size_t n) {
    /* Invariant: arena.used <= arena.cap */
    if (n > arena.cap - arena.used) longjmp(bailout_env, 1);
    void *p = arena.mem + arena.used;
    arena.used += n;
    return p;
}

static void legit_close(const char *t) { printf("  close(%s)\n", t); }

static void begin_request(size_t limit) {
    arena.mem = calloc(1, limit);
    if (!arena.mem) longjmp(bailout_env, 1);
    arena.cap = limit;
    arena.used = 0;
}

static void end_request(void) {                   /* the single exit, ALL paths */
    g_upload = NULL;                              /* request-scoped globals reset here */
    if (arena.mem) {
        free(arena.mem);
        arena.mem = NULL;
    }
    arena.cap = 0;
    arena.used = 0;
}

static void parse_upload(size_t payload) {
    upload_t *u = arena_alloc(sizeof *u);         /* build a local first */
    u->on_close = legit_close;
    strncpy(u->tag, "progress", sizeof(u->tag) - 1);
    u->tag[sizeof(u->tag) - 1] = '\0';
    arena_alloc(payload);                         /* may bail out: g_upload untouched */
    g_upload = u;                                 /* publish LAST */
}

static void run(const char *name, size_t limit, size_t payload) {
    printf("[%s]\n", name);
    if (setjmp(bailout_env) == 0) {
        begin_request(limit);
        parse_upload(payload);
        g_upload->on_close(g_upload->tag);
    } else {
        puts("  bailout");
    }
    end_request();                                /* runs whether we bailed out or not */
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    run("request 1: normal", 4096, 8);
    run("request 2: attacker exceeds the limit", 64, 1000);
    run("request 3: innocent victim", 4096, 8);
    return 0;
}
