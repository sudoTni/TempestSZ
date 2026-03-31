/*
 * winpthread_stubs.c
 *
 * The MSYS2/ucrt64 GCC runtime (exception handling, TLS) calls several
 * pthread functions through DLL import pointers (__imp_pthread_xxx), which
 * would normally require libwinpthread-1.dll at runtime.
 *
 * Supplying these __imp_ data symbols here — pointing at the same functions
 * from libwinpthread.a — satisfies the GCC runtime without needing the DLL.
 *
 * Only the symbols that appear in the PE import table need to be listed;
 * add more here if a future dependency introduces new __imp_pthread_* refs.
 */

/* Suppress the ISO C pedantic warning about function-pointer → void* casts.
   These casts are legal in POSIX and on all targeted platforms (Win64). */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

#include <pthread.h>
#include <time.h>

/* ── POSIX time ─────────────────────────────────────────────────────────── */
void* __imp_clock_gettime64       = (void*)clock_gettime;

/* ── Condition variables ─────────────────────────────────────────────────── */
void* __imp_pthread_cond_broadcast = (void*)pthread_cond_broadcast;
void* __imp_pthread_cond_wait      = (void*)pthread_cond_wait;

/* ── Thread-local storage ────────────────────────────────────────────────── */
void* __imp_pthread_getspecific    = (void*)pthread_getspecific;
void* __imp_pthread_key_create     = (void*)pthread_key_create;
void* __imp_pthread_setspecific    = (void*)pthread_setspecific;

/* ── One-time initialisation ─────────────────────────────────────────────── */
void* __imp_pthread_once           = (void*)pthread_once;

#pragma GCC diagnostic pop
