/*
 * libc.c 
 */

#include <libc.h>
#include <errno.h>
#include <types.h>
#include <string.h>

int errno;

void itoa(int a, char *b)
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

void perror()
{
  switch (errno)
  {
    case 9: /*EBADF*/
      string msg = ["Bad file number"];
      write(1, msg, sizeof(msg));
      break;

    case 13: /*EACCES*/
      string msg = ["Permission denied"];
      write(1, msg, sizeof(msg));
      break;

    case 14: /*EFAULT*/
      string msg = ["Bad address"];
      write(1, msg, sizeof(msg));
      break;

    case 22: /*EINVAL*/
      string msg = ["Invalid argument"];
      write(1, msg, sizeof(msg));
      break;

    case 38: /*ENOSYS*/
      string msg = ["Invalid system call number"];
      write(1, msg, sizeof(msg));
      break;
  }
}
