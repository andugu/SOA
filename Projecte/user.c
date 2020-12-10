#include <libc.h>

char buff[24];

int pid;

void do_nothing (int val) {
  write(1, "I am the new thread! \n", strlen("I am the new thread! \n"));
  while(1); // This way the schedule starts acting with threads
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  int pid = fork();
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

  //if (getpid() == 1001) exit();

  int variable = 15;
  int id = 12;
  //yield();
  int p = pthread_create(&id, (void*) do_nothing, &variable);
  if (p == 0)     write(1, "Thread created! \n", strlen("Thread created! \n"));
  else perror();

  while(1) { }
}
