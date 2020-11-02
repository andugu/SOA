#include <libc.h>

int pid;

void test_write_gettime()
{
	write(0, "aaa", strlen("aaa")); // EBADF
	write(1, "Perror content is: ", strlen("Perror content is: "));
	perror();

	write(1, 0, 0); // EFAULT
	write(1, "Perror content is: ", strlen("Perror content is: "));
	perror();

	write(1, "aaa", -1); // EINVAL
	write(1, "Perror content is: ", strlen("Perror content is: "));
	perror();

	write(1, "Ticks since SO booted: ", strlen("Ticks since SO booted: "));

	char time_char[30];
	int time = gettime();
	itoa(time, time_char);
	write(1, time_char, strlen(time_char));

	write(1, "\n", strlen("\n"));
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

	write(1, "    User program reached...\n", strlen("    User program reached...\n"));

	write(1, "Going to infinite loop, bye! :)", strlen("Going to infinite loop, bye! :)"));

  	while(1)
  	{
  	}
}
