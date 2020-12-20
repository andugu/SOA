/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */

#ifndef __LIBC_H__
#define __LIBC_H__

typedef int sem_t;
typedef int pthread_t;

int write(int fd, char *buffer, int size);

void itoa(int a, char *b);

void ltoa(unsigned long* d, char *b);

int strlen(char *a);

void perror();

int getpid();

int getTid();

int getticks(unsigned long *a);

int get_errno();

int fork();

void exit();

int yield();

int pthread_create(pthread_t *id, unsigned int* start_routine, void *arg);

void pthread_exit(int *retval);

int pthread_join(pthread_t id, int *retval);

void pthread_ret();

int sem_init(sem_t* id, unsigned int value);

int sem_wait(sem_t id);

int sem_post(sem_t id);

int sem_destroy(sem_t id);


#endif  /* __LIBC_H__ */
