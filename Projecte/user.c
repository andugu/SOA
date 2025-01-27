#include <libc.h>

char buff[24];
sem_t sem;
int global;

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
  if (sem_post(sem) < 0) write (1, "ERROR!!!!!!\n", strlen("ERROR!!!!!!\n"));
  write(1, "Dad awake\n", strlen("Dad awake\n"));

  pthread_exit(&local);
}

/* Joc de proves 2: pthread_create(), pthreads_exit(), pthreads_join() with only one process */
int joc_proves_2()
{
  pthread_t id;
  int value = 15;
  int ret = 0;

  if (sem_init(&sem, 0) < 0) return -1;;

  if (pthread_create(&id,(unsigned int*) &joc_proves_2_aux_thread, &value)) return -1;

  /* Wait for slave to become zombie */
  write(1, "Dad going to sleep on signal\n", strlen("Dad going to sleep on signal\n"));
  if (sem_wait(sem) < 0) return -1;;
  write(1, "Dad wake up from signal\n", strlen("Dad wake up from signal\n"));

  write(1, "Going to take join value", strlen("Going to take join value"));
  if (pthread_join(id, &ret) < 0) return -1;

  if (ret != value) return -1;

  write(1, "ret == value\n", strlen("ret == value\n"));

  if (sem_destroy(sem) < 0) return -1;

  return 0;
}

void joc_proves_3_aux_thread()
{
  write(1, "New thread created with TID: ", strlen("New thread created with TID: "));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  write(1, "Going to wakeup Dad\n", strlen("Going to wakeup Dad\n"));
  if (sem_post(sem) < 0) write(1, "Error!!\n", strlen("Error!!\n"));
  write(1, "Dad awake\n", strlen("Dad awake\n"));

  pthread_exit((void*) 0);
}

/* Joc de proves 3: sem_init(), sem_wait(), sem_post(), sem_destroy() with only one process and two threads */
int joc_proves_3()
{
  pthread_t id;

  /* Init for sync */
  if (sem_init(&sem, 0) < 0) return -1;

  if (pthread_create(&id,(unsigned int*) &joc_proves_3_aux_thread, (void*)0) < 0) return -1;

  write(1, "Dad going to sleep on signal\n", strlen("Dad going to sleep on signal\n"));
  if (sem_wait(sem) < 0) return -1;
  write(1, "Dad wake up from signal\n", strlen("Dad wake up from signal\n"));

  if (sem_destroy(sem) < 0) return -1;

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
  if (pthread_join(id, &ret) < 0) return -1;
  write(1, "Dad exited pthread_join()\n", strlen("Dad exited pthread_join()\n"));

  if (ret != 5)
    write (1, "Incorrect return value of thread\n", strlen("Incorrect return value of thread\n"));

  return 0;
}

int joc_proves_5_aux_thread(int* param)
{
  write(1, "New thread with aux1 func created with TID: ", strlen("New thread with aux1 func created with TID: "));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  if (sem_post(sem) < 0) return -1;

  *param += 5;

  return *param;
}

int joc_proves_5_aux2_thread(int* param)
{
  write(1, "New thread with aux2 func created with TID: ", strlen("New thread with aux2 func created with TID: "));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  if (sem_wait(sem) < 0) return -1;

  *param += 5;

  global += 1;

  pthread_exit(param);

  return -1;
}

/* Joc de proves 5: Creating multiple threads and reinitializing semafors */
int joc_proves_5()
{
  pthread_t id1, id2, id3;
  int param1 = 5, param2 = 15, param3 = 50;
  int ret1 = 0, ret2 = 0, ret3 = 0;
  global = 0;

  /* Test reinit of semafor */
  if (sem_init(&sem, 0) < 0) return -1;
  itoa(sem, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  if (sem_destroy(sem) < 0) return -1;

  if (sem_init(&sem, 0) < 0) return -1;
  itoa(sem, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  if (pthread_create(&id1,(unsigned int*) &joc_proves_5_aux_thread, &param1) < 0) return -1;
  if (pthread_create(&id2,(unsigned int*) &joc_proves_5_aux2_thread, &param2) < 0) return -1;
  if (pthread_create(&id3,(unsigned int*) &joc_proves_5_aux_thread, &param3) < 0) return -1;

  write(1, "Dad gona sleep\n", strlen("Dad gona sleep\n"));
  if (sem_wait(sem) < 0) return -1;
  write(1, "Dad gona exit\n", strlen("Dad gona exit\n"));

  if (pthread_join(id1, &ret1) < 0) return -1;
  /* Holds exit status */
  if (pthread_join(id2, &ret2) < 0) return -1;
  if (pthread_join(id3, &ret3) < 0) return -1;

  write(1, "Values as follow: param1, ret1, param2, param3, ret3\n", strlen("Values as follow: param1, ret1, param2, param3, ret3\n"));

  itoa(param1, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  itoa(ret1, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  itoa(param2, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  itoa(param3, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  itoa(ret3, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  if (param1+param2+param3 != ret1+param2+ret3 || global != 1 || ret2 != 0){
    write (1, "Incorrect result\n", strlen("Incorrect result\n"));
    return -1;
  }

  return 0;
}

int joc_proves_6_aux_thread()
{
  int tid = getTid();

  return tid;
}

/* Joc de proves 6: Reusing a TCB */
int joc_proves_6()
{
  pthread_t id;
  int ret1, ret2, ret3, ret4, ret5, ret6;

  if (pthread_create(&id,(unsigned int*) &joc_proves_6_aux_thread, (void*) 0) < 0) return -1;
  if (pthread_join(id, &ret1) < 0) return -1;
  write(1, "Thread 1 returned; ", strlen("Thread 1 returned; "));
  itoa(ret1, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  if (pthread_create(&id,(unsigned int*) &joc_proves_6_aux_thread, (void*) 0) < 0) return -1;
  if (pthread_join(id, &ret2) < 0) return -1;
  write(1, "Thread 2 returned; ", strlen("Thread 2 returned; "));
  itoa(ret2, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  if (pthread_create(&id,(unsigned int*) &joc_proves_6_aux_thread, (void*) 0) < 0) return -1;
  if (pthread_join(id, &ret3) < 0) return -1;
  write(1, "Thread 3 returned; ", strlen("Thread 3 returned; "));
  itoa(ret3, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  if (pthread_create(&id,(unsigned int*) &joc_proves_6_aux_thread, (void*) 0) < 0) return -1;
  if (pthread_join(id, &ret4) < 0) return -1;
  write(1, "Thread 4 returned; ", strlen("Thread 4 returned; "));
  itoa(ret4, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  if (pthread_create(&id,(unsigned int*) &joc_proves_6_aux_thread, (void*) 0) < 0) return -1;
  if (pthread_join(id, &ret5) < 0) return -1;
  write(1, "Thread 5 returned; ", strlen("Thread 5 returned; "));
  itoa(ret5, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  if (pthread_create(&id,(unsigned int*) &joc_proves_6_aux_thread, (void*) 0) < 0) return -1;
  if (pthread_join(id, &ret6) < 0) return -1;
  write(1, "Thread 6 returned; ", strlen("Thread 6 returned; "));
  itoa(ret6, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen(buff));

  return 0;
}


void joc_proves_7_aux_thread()
{
  write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
  itoa(getpid(), buff);
  write(1, buff, strlen(buff));
  write(1, ",", strlen(","));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  write(1, "My main thread should come back to CPU\n", strlen("My main thread should come back to CPU\n"));
  pthread_exit((void*) 0);
  write(1, "ERROR!!\n", strlen("ERROR!!\n"));
}

/* Joc de proves 7: Thread_switch between multiple threads and multiple process to test process management */
int joc_proves_7()
{
  pthread_t id;
  int pid = fork();
  if (pthread_create(&id,(unsigned int*) &joc_proves_7_aux_thread, (void*)0) < 0) return -1;
  if (pid == 0) {
    write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
    write(1, "Now, my Dad's second thread should come back to CPU\n", strlen("Now, my Dad's second thread should come back to CPU\n"));
    if (yield() < 0) return -1;
    write(1, "I'm back, {PID,TID} is ", strlen("I'm back, {PID,TID} is "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
  } else if (pid > 0) {
    write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
    write(1, "Now, my main child thread should come to CPU\n", strlen("Now, my main child thread should come to CPU\n"));
    if (yield() < 0) return -1;
    write(1, "I'm back, {PID,TID} is ", strlen("I'm back, {PID,TID} is "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
    write(1, "Now, my second child thread should come to CPU\n", strlen("Now, my second child thread should come to CPU\n"));
    if (yield() < 0) return -1;
    write(1, "I'm back, {PID,TID} is ", strlen("I'm back, {PID,TID} is "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
  } else return -1;

  if (pid == 0) {
    write(1, "Bye, I'm leaving!\n", strlen("Bye, I'm leaving!\n"));
    exit(); /* Dad should exit */
    write(1, "Error!!!\n", strlen("Error!!!\n"));
    return -1;
  }

  return 0;
}

void joc_proves_8_aux_thread2()
{
  while(1);
  write(1, "Error!!!!!!", strlen("Error!!!!!!"));
}

void joc_proves_8_aux_thread()
{
  write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
  itoa(getpid(), buff);
  write(1, buff, strlen(buff));
  write(1, ",", strlen(","));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));

  int pid = fork();
  if (pid == 0) {
    write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
    pthread_t id;
    if (pthread_create(&id,(unsigned int*) &joc_proves_8_aux_thread2, (void*)0) < 0) write(1, "Error!!!", strlen("Error!!!"));
    exit();
    write(1, "ERROR!!!\n", strlen("ERROR!!!\n"));
  } else {
    if (yield() < 0) write(1, "Error!!!", strlen("Error!!!"));
    write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
    itoa(getpid(), buff);
    write(1, buff, strlen(buff));
    write(1, ",", strlen(","));
    itoa(getTid(), buff);
    write(1, buff, strlen(buff));
    write(1, "\n", strlen("\n"));
  }

  pthread_exit((void*) 0);
  write(1,"ERROR!!\n", strlen("ERROR!!\n"));
}

/* Joc de proves 8: A pthread doing a Fork()! && doing an exit with pthread alive!*/
int joc_proves_8()
{
  pthread_t id;
  int ret;
  write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
  itoa(getpid(), buff);
  write(1, buff, strlen(buff));
  write(1, ",", strlen(","));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));
  if (pthread_create(&id,(unsigned int*) &joc_proves_8_aux_thread, (void*)0) < 0) return -1;
  pthread_join(id, &ret);
  write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
  itoa(getpid(), buff);
  write(1, buff, strlen(buff));
  write(1, ",", strlen(","));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));
  return 0;
}

void joc_proves_9_aux_thread()
{
  write(1, "My {PID,TID} is: ", strlen("My {PID,TID} is: "));
  itoa(getpid(), buff);
  write(1, buff, strlen(buff));
  write(1, ",", strlen(","));
  itoa(getTid(), buff);
  write(1, buff, strlen(buff));
  write(1, "\n", strlen("\n"));
  if (sem_wait(sem) < 0) write(1, "ERROR!!\n", strlen("ERROR!!\n"));
  if (sem_post(sem) < 0) write(1, "ERROR!!\n", strlen("ERROR!!\n"));
  pthread_exit((void*)0);
  write(1, "ERROR!!\n", strlen("ERROR!!\n"));
}

int joc_proves_9_threads()
{
  pthread_t id;
  if (sem_init(&sem, 0) < 0) return -1;
  for (int w = 0; w < 10; w++) { // in w == 9, it should return error.
    if (pthread_create(&id,(unsigned int*) &joc_proves_9_aux_thread, (void*)0) < 0) {
      if (w == 9) return 0;
      return -1;
    }
  }
  return -1;
}

/* Joc de proves 9: Having more than 10 threads for 1 process! */
int joc_proves_9()
{
  if (joc_proves_9_threads() != 0) return -1;
  if (sem_post(sem) < 0) return -1;
  if (yield() < 0) return -1;
  write(1, "Errno is: ", strlen("Errno is: "));
  itoa(get_errno(), buff);
  write(1, buff, strlen(buff));
  if (get_errno() != 12) return -1; // ENOMEM = 12
  write(1, "\nError number is correct!\n", strlen("\nError number is correct!\n"));
  return 0;
}

void joc_proves_10_aux_thread()
{
  if (sem_wait(sem) < 0) write(1, "ERROR!!\n", strlen("ERROR!!\n"));
  if (sem_post(sem) < 0) write(1, "ERROR!!\n", strlen("ERROR!!\n"));
  pthread_exit((void*)0);
}

int joc_proves_10_threads(int first)
{
  pthread_t id;
  if (sem_init(&sem, 0) < 0) return -1;
  int MAX = 8;
  if (first) MAX = 9; // The reason for MAX is we will be able to make 17 threads (2 of main processes + 1 of idle)
  for (int w = 0; w < 10; w++) { // in w == 9, it should return error.
    if (pthread_create(&id,(unsigned int*) &joc_proves_10_aux_thread, (void*)0) < 0) {
      if (w == MAX) return 0;
      return -1;
    }
  }
  return -1; // should never reach this point
}

/* Joc de proves 10: Having more than 20 threads in the system!*/
int joc_proves_10()
{
  int pid = fork();
  if (pid < 0) return -1;
  if (joc_proves_10_threads(pid > 0) != 0) return -1;
  if (yield() < 0) return -1;
  if (sem_post(sem) < 0) return -1;
  write (1, "I am ", strlen("I am "));
  itoa(getpid(), buff);
  write(1, buff, strlen(buff));
  write(1, " and my errno is: ", strlen(" and my errno is: "));
  itoa(get_errno(), buff);
  write(1, buff, strlen(buff));
  if (get_errno() != 12) return -1; // ENOMEM = 12
  write(1, "\nError number is correct!\n", strlen("\nError number is correct!\n"));
  if (pid == 0) {
    exit();
    return -1;
  }
  if (yield() < 0) return -1; // So that the child ends its execution before finishing the testing
  return 0;
}

/* Joc de proves 11: Having more than 10 processes in the system! */
int joc_proves_11()
{
  write (1, "I am ", strlen("I am "));
  itoa(getpid(), buff);
  write(1, buff, strlen(buff));
  write (1, " and I am going to occupy all TCB\n", strlen(" and I am going to occupy all TCB\n"));
  for (int i = 0; i < 10; i++) { // i should never value more than 8
    int pid = fork();
    if (pid == 0) {
      write (1, "I am ", strlen("I am "));
      itoa(getpid(), buff);
      write(1, buff, strlen(buff));
      write (1, "\n", strlen("\n"));
      exit();
      return -1; // Should never reach this point
    } else if (pid < 0) { 
      if (i == 8 && get_errno() == 12) { // 10 PCB - (Init+idle) = 8 && ENOMEM = 12
        if (yield() < 0) return -1;
        write(1, "All TCB have been ocupied!\nError number is correct!\n", strlen("All TCB have been ocupied!\nError number is correct!\n"));
        return 0;
      }
      return -1;
    }
  }
  return -1; // Should never reach this point
}

/* Joc de proves 12: Having more than 20 semaphores in the system! */
int joc_proves_12()
{
  sem_t sem[21];
  int err;
  for (int i = 0; i < 30; i++) { // i should never value more than 20
    err = sem_init(&sem[i], 1);
    if (err < 0) {
      if (i == 20 && get_errno() == 12) {  // ENOMEM = 12
        for (int e = 0; e < i; e++) if(sem_destroy(sem[e]) < 0) return -1; // We destroy all the created semaphores
        write(1, "We tried creating one more semaphore, but an error appeared.\nError number is correct!\n",strlen("We tried creating one more semaphore, but an error appeared.\nError number is correct!\n"));
        return 0;
      }
      return -1;
    }
    write(1, "Semaphore number ", strlen("Semaphore number "));
    itoa(i+1, buff);
    write(1, buff, strlen(buff));
    write(1, " has been created!\n", strlen(" has been created!\n"));
  }
  return -1; // Should never reach this point
}

void carrega_de_treball_auxiliar() 
{
  pthread_exit((void*)0);
  write(1, "ERROR!!\n", strlen("ERROR!!\n"));
}

int carrega_de_treball() 
{
  pthread_t id[9];
  unsigned long inici;
  if (getticks(&inici) < 0) return -1;
  int NUMBER_OP = 1000; /* By changing this number, one can decide the number of operations to use for the analysis */
  for (int w = 0; w < NUMBER_OP; w++) {
    for (int i = 0; i < 9; i++) {
      if (pthread_create(&id[i],(unsigned int*) &carrega_de_treball_auxiliar, (void*)0) < 0) return -1;
    }
    if (yield() < 0) return -1;
    for (int i = 0; i < 9; i++) {
      if (pthread_join(id[i],(void*)0) < 0) return -1;
    }
  }

  unsigned long fi;
  if (getticks(&fi) < 0) return -1;
  unsigned long diff = fi - inici;

  write(1, "Hem comencat amb ", strlen("Hem comencat amb "));
  ltoa(&inici, buff);
  write(1, buff, strlen(buff));
  write(1, " ticks, i hem acabat amb ", strlen(" ticks, i hem acabat amb "));
  ltoa(&fi, buff);
  write(1, buff, strlen(buff));
  write(1, " ticks.\nI aixo es una diferencia de ", strlen(" ticks.\nI aixo es una diferencia de "));
  ltoa(&diff, buff);
  write(1, buff, strlen(buff));
  write(1, " ticks.\nHem fet ", strlen(" ticks.\nHem fet "));
  itoa(NUMBER_OP*9, buff);
  write(1, buff, strlen(buff));
  write(1, " operacions.\nPer tant, la mitjana de crear i destruir un thread es de: ", strlen(" operacions.\nPer tant, la mitjana de crear i destruir un thread es de: "));
  
  /* We use this variables to avoid using float or doubles */
  unsigned long mitjana = diff*100/9;
  unsigned long mitjana_high = mitjana/(100*NUMBER_OP); // Natural part of the number
  unsigned long mitjana_low = mitjana%(100*NUMBER_OP); // Decimal part of the number
  unsigned long mitjana_low_aux = mitjana_low;
  
  int num = 0; // num is used to calculate the number of '0' that has the decimal part before the first integer
  while(mitjana_low_aux > 0) {
    mitjana_low_aux /= 10;
    num++;
  }
  int aux = NUMBER_OP;
  int dec = 1; // dec is used to calculate the number of digits of mitjana_low
  while(aux > 0) {
    aux /= 10;
    dec++;
  }

  ltoa(&mitjana_high, buff); /* ltoa(unsigned long* d, char *b); -> long to ascii */
  write(1, buff, strlen(buff));
  write(1, ".", strlen("."));

  for (int i = num; i < dec; i++) write(1, "0", strlen("0"));
  
  ltoa(&mitjana_low, buff);
  write(1, buff, strlen(buff));
  write(1, " ticks.\n", strlen(" ticks.\n"));
  
  return 0;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
  /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
  /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  
  write(1, "\n", strlen("\n"));
  
  int selected = 0;
  /* selected = 0 per càrrega de treball. 
     selected > 0 per joc de proves.*/

  switch (selected)
    {
      case 0:
        if (carrega_de_treball() != 0)   write(1, "Carrega de treball FAILED\n", strlen("Carrega de treball FAILED\n"));
        else write(1, "Carrega de treball OK\n", strlen("Carrega de treball OK\n"));
        break;
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
      case 5:
        if (joc_proves_5() != 0)   write(1, "Error Joc proves 5\n", strlen("Error Joc proves 5\n"));
        else write(1, "Joc proves 5 completed with success\n", strlen("Joc proves 5 completed with success\n"));
        break;
      case 6:
        if (joc_proves_6() != 0)   write(1, "Error Joc proves 6\n", strlen("Error Joc proves 6\n"));
        else write(1, "Joc proves 6 completed with success\n", strlen("Joc proves 6 completed with success\n"));
        break;
      case 7:
        if (joc_proves_7() != 0)   write(1, "Error Joc proves 7\n", strlen("Error Joc proves 7\n"));
        else write(1, "Joc proves 7 completed with success\n", strlen("Joc proves 7 completed with success\n"));
        break;
      case 8:
        if (joc_proves_8() != 0)   write(1, "Error Joc proves 8\n", strlen("Error Joc proves 8\n"));
        else write(1, "Joc proves 8 completed with success\n", strlen("Joc proves 8 completed with success\n"));
        break;
      case 9:
        if (joc_proves_9() != 0)   write(1, "Error Joc proves 9\n", strlen("Error Joc proves 9\n"));
        else write(1, "Joc proves 9 completed with success\n", strlen("Joc proves 9 completed with success\n"));
        break;
      case 10:
        if (joc_proves_10() != 0)   write(1, "Error Joc proves 10\n", strlen("Error Joc proves 10\n"));
        else write(1, "Joc proves 10 completed with success\n", strlen("Joc proves 10 completed with success\n"));
        break;
      case 11:
        if (joc_proves_11() != 0)   write(1, "Error Joc proves 11\n", strlen("Error Joc proves 11\n"));
        else write(1, "Joc proves 11 completed with success\n", strlen("Joc proves 11 completed with success\n"));
        break;
      case 12:
        if (joc_proves_12() != 0)   write(1, "Error Joc proves 12\n", strlen("Error Joc proves 12\n"));
        else write(1, "Joc proves 12 completed with success\n", strlen("Joc proves 12 completed with success\n"));
        break;
      default:
        write(1, "No such test\n", strlen("No such test\n"));
    }

  /* Infinite loop */
  while(1){}
}
