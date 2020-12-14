#include <libc.h>

char buff[24];
sem_t sem;

/* Joc de proves 1: Fork(), exit(), and process managment fully modified, but tested for 1 thread/process */
int joc_proves_1()
{
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

void joc_proves_2_aux_thread(int* value)
{
  write(1, "New thread created with TID: ", strlen("New thread created with TID: "));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  int local = *value;

  write(1, "Going to wakeup Dad\n", strlen("Going to wakeup Dad\n"));
  sem_post(sem);
  write(1, "Dad awake\n", strlen("Dad awake\n"));

  pthread_exit(&local);
}

/* Joc de proves 2: pthread_create(), pthreads_exit(), pthreads_join() with only one process */
int joc_proves_2()
{
  pthread_t id;
  int value = 15;
  int ret = 0;

  sem_init(&sem, 0);

  if (pthread_create(&id,(unsigned int*) &joc_proves_2_aux_thread, &value)) return -1;

  /* Wait for slave to become zombie */
  write(1, "Dad going to sleep on signal\n", strlen("Dad going to sleep on signal\n"));
  sem_wait(sem);
  write(1, "Dad wake up from signal\n", strlen("Dad wake up from signal\n"));

  write(1, "Going to take join value", strlen("Going to take join value"));
  pthread_join(id, &ret);

  if (ret != value) return -1;

  write(1, "ret == value\n", strlen("ret == value\n"));

  sem_destroy(sem);

  return 0;
}

void joc_proves_3_aux_thread()
{
  write(1, "New thread created with TID: ", strlen("New thread created with TID: "));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  write(1, "Going to wakeup Dad\n", strlen("Going to wakeup Dad\n"));
  sem_post(sem);
  write(1, "Dad awake\n", strlen("Dad awake\n"));

  pthread_exit((void*) 0);
}

/* Joc de proves 3: sem_init(), sem_wait(), sem_post(), sem_destroy() with only one process and two threads */
int joc_proves_3()
{
  pthread_t id;

  /* Init for sync */
  sem_init(&sem, 0);

  if (pthread_create(&id,(unsigned int*) &joc_proves_3_aux_thread, (void*)0) < 0) return -1;

  write(1, "Dad going to sleep on signal\n", strlen("Dad going to sleep on signal\n"));
  sem_wait(sem);
  write(1, "Dad wake up from signal\n", strlen("Dad wake up from signal\n"));

  sem_destroy(sem);

  return 0;
}

int joc_proves_4_aux_thread()
{
  write(1, "New thread created with TID: ", strlen("New thread created with TID: "));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  return 5;
}

/* Joc de proves 4: pthread_join() when a child thread isn't a Zombie & pthread_ret() */
int joc_proves_4()
{
  pthread_t id;
  int ret = 0;

  if (pthread_create(&id,(unsigned int*) &joc_proves_4_aux_thread, (void*)0) < 0) return -1;

  write(1, "Dad enters pthread_join()\n", strlen("Dad enters pthread_join()\n"));
  pthread_join(id, &ret);
  write(1, "Dad exited pthread_join()\n", strlen("Dad exited pthread_join()\n"));

  if (ret != 5)
    write (1, "Incorrect return value of thread\n", strlen("Incorrect return value of thread\n"));

  return 0;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
  /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
  /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  
  write(1, "\n", strlen("\n"));
  
  int selected = 2;

  switch (selected)
  {
    case 1:
      if (joc_proves_1() != 0)   write(1, "Error Joc proves 1\n", strlen("Error Joc proves 1\n"));
      else write(1, "Joc proves 1 completed with success\n", strlen("Joc proves 1 completed with success\n"));
      break;
    case 2:
      if (joc_proves_2() != 0)   write(1, "Error Joc proves 2\n", strlen("Error Joc proves 2\n"));
      else write(1, "Joc proves 2 completed with success\n", strlen("Joc proves 2 completed with success\n"));
      break;
    case 3:
      if (joc_proves_3() != 0)   write(1, "Error Joc proves 3\n", strlen("Error Joc proves 3\n"));
      else write(1, "Joc proves 3 completed with success\n", strlen("Joc proves 3 completed with success\n"));
      break;
    case 4:
      if (joc_proves_4() != 0)   write(1, "Error Joc proves 4\n", strlen("Error Joc proves 4\n"));
      else write(1, "Joc proves 4 completed with success\n", strlen("Joc proves 4 completed with success\n"));
      break;
    default:
      write(1, "No such test\n", strlen("No such test\n"));
  }

  /* Infinite loop */
  while(1){}
}
