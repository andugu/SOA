/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>

struct pthread_t {
	int tid;
};

extern int errno;

int write(int fd, char *buffer, int size);

void itoa(int a, char *b);

int strlen(char *a);

void perror();

int getpid();

int fork();

void exit();

int yield();

int get_stats(int pid, struct stats *st);

int pthread_create(struct pthread_t *thread, void *(*start_routine) (void *), void *arg);

void pthread_exit(void *retval);

int pthread_join(struct pthread_t thread, void **retval);


#endif  /* __LIBC_H__ */
