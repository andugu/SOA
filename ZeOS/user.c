#include <libc.h>

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

	int t = gettime();
	if (write(1, "0 \n", -1) < 0) perror();
	
	write(1, "HolaHoLa \n", strlen("HolaHoLa \n"));

  	while(1)
  	{
	  	int tmp = gettime();
	  	if (tmp > t+99){
		  	char user_buffer[30];
		  	itoa(tmp, user_buffer);
		  	write(1, user_buffer, strlen(user_buffer));
		  	write(1, "\n", strlen("\n"));
		  	t = tmp;
	  	}
  	}
}
