#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

//yield();

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

  while(1) { }
}
