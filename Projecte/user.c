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

/* Joc de proves 1:  Fork, exit, and process managment fully modified, but tested for 1 thread/process */
int joc_probes_1() {
  int pid = fork();
  if (pid == 0) {
    write(1, "I'm the Child, and my {PID,TID} is: ", strlen("I'm the Child, and my {PID,TID} is: "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
  } else if (pid > 0) {
    write(1, "I'm the Dad, and my {PID,TID} is: ", strlen("I'm the Dad, and my {PID,TID} is: "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
    write(1, "We are going to force a task_switch\n", strlen("We are going to force a task_switch\n"));
    if (yield() < 0) return -1; /* Dad to readyqueue -> Child's turn to start his execution */
    write(1, "I'm Dad and I'm back!\n", strlen("I'm Dad and I'm back!\n"));
  } else perror();

  if (pid > 0) {
    exit(); /* Dad should exit */
    write(1, "Error!!!\n", strlen("Error!!!\n"));
    return -1;
  }

  write(1, "We are going to let Dad to die (force_task_switch)\n", strlen("We are going to let Dad to die (force_task_switch)\n"));
  if (yield() < 0) return -1; /* Child should be put to readyqueue -> Dad's turn to occupy CPU */
  write(1, "I'm Child and I'm back!\n", strlen("I'm Child and I'm back!\n"));

  write(1, "Dad should have exited, only the child should continue\n", strlen("Dad should have exited, only the child should continue\n"));
  /* Only the Children should reach this point */  
  write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
  itoa(getpid(), buff);
  write(1, buff, strlen(buff));
  write(1, ",", strlen(","));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));
  
  return 0;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

/*
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

  write(1, "3\n", strlen("3\n")); 	*/

  write(1, "\n", strlen("\n"));
  if (joc_probes_1() != 0)   write(1, "Error Joc probes 1\n", strlen("Error Joc probes 1\n"));
  else write(1, "Joc probes 1 completed with success\n", strlen("Joc probes 1 completed with success\n"));

  while(1) { }
}
