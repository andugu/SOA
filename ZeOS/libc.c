/*
 * libc.c 
 */

#include <libc.h>
#include <errno.h>
#include <types.h>

int errno;

void itoa(int a, char *b) //Converteix integer a char*
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

void perror(void)
{
  switch (errno)
  {
    case 9: /*EBADF*/
      write(1, "Bad file number\n", strlen("Bad file number\n"));
      break;

    case 13: /*EACCES*/
      write(1, "Permission denied\n", strlen("Permission denied\n"));
      break;

    case 14: /*EFAULT*/
      write(1, "Bad address\n", strlen("Bad address\n"));
      break;

    case 22: /*EINVAL*/
      write(1, "Invalid argument\n", strlen("Invalid argument\n"));
      break;

    case 38: /*ENOSYS*/
      write(1, "Invalid system call number\n", strlen("Invalid system call number\n"));
      break;
  }
}
