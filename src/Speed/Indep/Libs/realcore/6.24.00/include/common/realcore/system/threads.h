#ifndef REALCORE_COMMON_SYSTEM_THREADS_H
#define REALCORE_COMMON_SYSTEM_THREADS_H

struct THREAD;

void THREAD_init();
void THREAD_yield(int a);
bool THREAD_iscurrent(THREAD *thread);

// El resto de la API (firmas de los stabs de la A124).
bool THREAD_create(THREAD *hThread, int (*entryproc)(void *), void *param, void *stack_addr, int stack_size,
                   int priority);
void THREAD_destroy(THREAD *thread);
void THREAD_yieldticks(int numticks);
int THREAD_getpriority(THREAD *thread);
bool THREAD_setpriority(THREAD *thread, int priority);
bool THREAD_waitexit(THREAD *thread, int ticks);

#endif
