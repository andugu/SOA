#include <libc.h>

char buff[24];

int pid;

sem_t sem;

void do_nothing (int *val) {
  write(1, "I am the new thread! \n", strlen("I am the new thread! \n"));
  //itoa(*val, buff);
  //write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));
  int i = 5;
  int* p = &i;
  sem_post(sem);
  //pthread_exit(p);
  write(1, "Error:(\n", strlen("Error:(\n"));
  return i;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  /*int pid = fork();
  if (pid == 0) {
    itoa(getpid(), buff);
    write(1, "Child ", strlen("Child "));
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
  } else if (pid > 0) {
    itoa(getpid(), buff);
    write(1, "Dad ", strlen("Dad "));
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
  } else perror();
  */
  //if (getpid() == 1001) exit();

  int variable = 15;
  pthread_t id;
  //yield();
  int p = pthread_create(&id, (void*) do_nothing, &variable);
  if (p == 0)     write(1, "Thread created! \n", strlen("Thread created! \n"));
  else perror();


  sem_init(&sem, 0);
  sem_wait(sem);
  sem_destroy(sem);

  write(1, "1\n", strlen("1\n"));

  int ret;
  // Arribem que el thread ja és zombie
  pthread_join(id, &ret);

  write(1, "2\n", strlen("2\n"));

  itoa(ret, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  write(1, "3\n", strlen("3\n"));

  while(1) { }
}
