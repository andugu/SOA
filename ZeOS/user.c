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

void print_stats()
{
	pid = getpid();
	struct stats stadistics;
	struct stats *p = &stadistics;

	char buffer[64];

	getstats(pid, p);

	write(1, "user_ticks: ", strlen("user_ticks: "));
	itoa((int)stadistics.user_ticks, buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));

	write(1, "system_ticks: ", strlen("system_ticks: "));
	itoa((int)stadistics.system_ticks, buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));

	write(1, "blocked_ticks: ", strlen("blocked_ticks: "));
	itoa((int)stadistics.blocked_ticks, buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));

	write(1, "ready_ticks: ", strlen("ready_ticks: "));
	itoa((int)stadistics.ready_ticks, buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));

	write(1, "elapsed_total_ticks: ", strlen("elapsed_total_ticks: "));
	itoa((int)stadistics.elapsed_total_ticks, buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));

	write(1, "total_trans: ", strlen("total_trans: "));
	itoa((int)stadistics.total_trans, buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));

	write(1, "remaining_ticks: ", strlen("remaining_ticks: "));
	itoa((int)stadistics.remaining_ticks, buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));
}

void test_fork_stats()
{
	write(1, "Test reached with pid: ", strlen("Test reached with pid: "));
	char buffer[64];
	itoa(getpid(), buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));

	pid = fork();

	// Make child lose time
	if (pid != 0){
		int t;
		for (int i = 0; i < 1000; ++i){
			t = gettime();
			char b[64];
			itoa(t, b);
			t = strlen(b);
		}
	}

	// Kill child
	int kill_child = 1;
	if (pid != 0 && kill_child){
		write(1, "Killing child\n", strlen("Killing child\n"));
		exit();
		write(1, "If this is on your screen... something is wrong :(\n", strlen("If this is on your screen... something is wrong :(\n"));
	}

	write(1, "Pid is: ", strlen("Pid is: "));
	itoa(pid, buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));

	print_stats();
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	write(1, "    User program reached...\n", strlen("    User program reached...\n"));
	
	test_fork_stats();

	write(1, "Going to infinite loop, bye! :)", strlen("Going to infinite loop, bye! :)"));

  	while(1)
  	{
  	}
}
