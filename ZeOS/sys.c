/*
 * sys.c - Syscalls implementation
 */

#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
}

void sys_exit()
{  
}

int sys_write(int fd, char * buffer, int size)
{
	if (check_fd(fd, ESCRIPTURA) != 0) return check_fd(fd, ESCRIPTURA);
	if (buffer == NULL) return -14; /*EFAULT*/
	if (size < 0) return -22; /*EINVAL*/

	char kernel_buffer[size];
	if (copy_from_user(buffer, kernel_buffer, size) < 0) return -1;
	return sys_write_console(kernel_buffer, size);

}

int sys_gettime()
{
	return zeos_ticks;
}
