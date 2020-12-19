/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) {
    current_thread()->errno = EBADF;
    return -EBADF; 
  }
  if (permissions!=ESCRIPTURA) {
    current_thread()->errno = EACCES;
    return -EACCES; 
  } 
  return 0;
}

void user_to_system(void)
{
  //update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  //update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
  current_thread()->errno = ENOSYS;
	return -ENOSYS; 
}

int sys_errno()
{
  return current_thread()->errno;
}

int sys_getpid()
{
	return current()->PID;
}

int sys_getTid()
{
	return current_thread()->TID;
}

int global_PID=1000;
int global_TID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  /* Any free task_struct and thread_struct? */
  if (list_empty(&freequeue) || list_empty(&freeThread)) {
    current_thread()->errno = ENOMEM;
    return -ENOMEM;
  }

  struct list_head *lhcurrent = list_first(&freequeue);
  list_del(lhcurrent);
  /* New task_struct */
  struct task_struct *lh = list_head_to_task_struct(lhcurrent);

  struct list_head *thr = list_first(&freeThread);
  list_del(thr);
  /* New thread_union */
  union thread_union *uchild =(union thread_union*)list_head_to_thread_struct(thr);
  
  /* Copy the parent's thread union to child's */
  copy_data(current_thread(), uchild, sizeof(union thread_union));

  lh->total_quantum = current()->total_quantum; // Ja no es fa al copy_data perquè total quantum de procés no està a thread_union

  /* new pages dir */
  allocate_DIR(lh);

  page_table_entry *process_PT = get_PT(lh);

  int stack_page = -1;
  if (uchild->thread.pag_userStack != 283) //Daddy's new process is not thread init => we must copy the user stack
  {
    stack_page=alloc_frame();
    if (stack_page!=-1)
    {
      set_ss_pag(process_PT, uchild->thread.pag_userStack, stack_page);
    }
    else 
    {
      current_thread()->errno = EAGAIN;
      return -EAGAIN;
    }
  }
  
  /* Allocate pages for DATA */
  int new_ph_pag, pag, i;
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      if (stack_page != -1) {
        free_frame(get_frame(process_PT, uchild->thread.pag_userStack));
        del_ss_pag(process_PT, uchild->thread.pag_userStack);
      }
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct and thread_struct */
      list_add_tail(lhcurrent, &freequeue);
      list_add_tail(thr, &freeThread);
      
      /* Return error */
      current_thread()->errno = EAGAIN;
      return -EAGAIN; 
    }
  }


  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }

  pag=NUM_PAG_KERNEL+NUM_PAG_CODE;
  set_cr3(get_DIR(current())); // To reuse the page pag to copy the new stack
  /* Copy User Stack */
  set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, uchild->thread.pag_userStack));
  copy_data((void*)((current_thread()->pag_userStack)<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
  del_ss_pag(parent_PT, pag+NUM_PAG_DATA);

  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  lh->PID=++global_PID;
  lh->state=ST_READY;
  uchild->thread.TID = ++global_TID;
  INIT_LIST_HEAD(&(lh->readyThreads));
  link_process_with_thread(lh, &(uchild->thread)); // NO possible error


  /*  int register_ebp;		// frame pointer
  // Map Parent's ebp to child's stack
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->thread.register_esp=register_ebp + sizeof(DWord);
  DWord temp_ebp=*(DWord*)register_ebp;

  // Prepare child stack for context switch
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  */
  uchild->stack[KERNEL_STACK_SIZE - 19] = 0;
  // Child's kernel_esp points to fake ebp
  uchild->thread.kernel_esp = (unsigned int) &(uchild->stack[KERNEL_STACK_SIZE - 19]);
  // @ret <- &ret_form_fork
  uchild->stack[KERNEL_STACK_SIZE - 18] = (unsigned long int) &ret_from_fork;

  /* Set stats to 0 */
  init_stats(&(uchild->thread.t_stats));
  // 		TASK STATS????

  /* Queue child process into readyqueue */
  uchild->thread.state=ST_READY;
  lh->state = ST_READY;
  list_add_tail(&(lh->list), &readyqueue);
  //list_add_tail(&(uchild->thread.list), &(lh->readyThreads)); JA ES FA A LINK_PROCESS_WITH_THREAD
  
  return lh->PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
  char localbuffer [TAM_BUFFER];
  int bytes_left;
  int ret;

  if ((ret = check_fd(fd, ESCRIPTURA))) {
    current_thread()->errno = EIO;
    return ret;
  }
  if (nbytes < 0) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }
  if (!access_ok(VERIFY_READ, buffer, nbytes)) {
    current_thread()->errno = EFAULT;
    return -EFAULT;
  }

  bytes_left = nbytes;
  while (bytes_left > TAM_BUFFER) {
  	copy_from_user(buffer, localbuffer, TAM_BUFFER);
  	ret = sys_write_console(localbuffer, TAM_BUFFER);
  	bytes_left-=ret;
  	buffer+=ret;
  }
  if (bytes_left > 0) {
  	copy_from_user(buffer, localbuffer,bytes_left);
  	ret = sys_write_console(localbuffer, bytes_left);
  	bytes_left-=ret;
  }
  if ((nbytes-bytes_left) < 0) current_thread()->errno = EIO;
  return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

int sys_getticks(unsigned long* result)
{
  unsigned long c = get_ticks();
  copy_to_user(&c, result, sizeof(unsigned long));
  return 0;
}

void sys_exit()
{
  page_table_entry *process_PT = get_PT(current());

  /* Free remaining threads */
  for (int i = 0; i < NR_THREADSxTASK; i++) {
    if (current()->threads[i] != NULL && current()->threads[i]->TID != -1) {
      if (current()->threads[i]->state != ST_RUN) {
        list_del(&(current()->threads[i]->list));
        if (current()->threads[i]->pag_userStack != 283) // 283 is the init thread user Stack that will be deleted with all the DATA pages
        {
          free_frame(get_frame(process_PT, current()->threads[i]->pag_userStack));
          del_ss_pag(process_PT, current()->threads[i]->pag_userStack);
        }
      }
      list_add_tail(&(current()->threads[i]->list), &freeThread);
      current()->threads[i]->TID = -1;
    }
  }

  // Deallocate all the propietary physical pages
  for (int i = 0; i < NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) {
    current_thread()->errno = EFAULT;
    return -EFAULT; 
  }
  
  if (pid < 0) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }
  for (int i = 0; i < NR_TASKS; i++)
  {
    if (task[i].PID == pid)
    {
      task[i].p_stats.remaining_ticks = remaining_quantum;
      copy_to_user(&(task[i].p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  current_thread()->errno = ESRCH;
  return -ESRCH; /*ESRCH */
}

int sys_pthread_create(int *id, unsigned int* start_routine, void *arg, unsigned int* ret)
{
  struct list_head *lhcurrent = NULL;
    
  /* Any free threads? */
  if (list_empty(&freeThread) || num_threads(current()) >= NR_THREADSxTASK) {
    current_thread()->errno = ENOMEM;
    return -ENOMEM;
  }

  lhcurrent = list_first(&freeThread);
  list_del(lhcurrent);

  struct thread_struct *thr = (struct thread_struct*)list_head_to_thread_struct(lhcurrent);
  union thread_union *thr_u = (union thread_union*)thr;

  /* Copy thread_union from caller */
  copy_data(current_thread(), thr_u, sizeof(union thread_union));
  
  /* Update thread_union with new values */
  thr->TID=++global_TID;
  *id = thr->TID;
  thr->state = ST_READY;
  thr->joinable = 1;
  init_stats(&thr->t_stats);
  INIT_LIST_HEAD(&(thr->notifyAtExit));

  if (link_process_with_thread(current(), thr) < 0) {
    current_thread()->errno = ENOMEM;
    return -ENOMEM;
  }
  /* link_process_with_thread already adds thr to readyThreads */

  /* Allocate User Stack in memory */
  page_table_entry *process_PT = get_PT(current());
  /* Find free logical pag */
  int pos = -1;
  for (int pag = TOTAL_PAGES-1; pos == -1 && pag > PAG_LOG_INIT_DATA+NUM_PAG_DATA; pag--)
    if (process_PT[pag].entry == 0) pos = pag;
  thr->pag_userStack = pos;
  /* Find free physical pag */
  int new_ph_pag = alloc_frame();
  /* Bind pages together */
  set_ss_pag(process_PT, thr->pag_userStack, new_ph_pag);
  /* Build up User Stack */
  /* @pthread_ret <- %esp
     *arg */
  unsigned int *p = (unsigned int*)(((thr->pag_userStack+1)<<12)-4);
  *p = (int) arg;
  --p;
  *p = (unsigned int) ret;

  /* Build up Kernel Stack */
  /* %ebp       -18
     @handler   -17
     SW Context -6 size 11
     HW Context 0 size 5 */
  /* kernel_esp points to top of stack */
  thr->kernel_esp = (unsigned int) &(thr_u->stack[KERNEL_STACK_SIZE - 18]);
  /* %eip */
  thr_u->stack[KERNEL_STACK_SIZE-5] = (unsigned int)start_routine;
  /* %esp */
  thr_u->stack[KERNEL_STACK_SIZE-2] = (int)p;
  return 0;
}

int sys_pthread_join(int id, int *retval)
{
  /* Check if thread(id) exists */
  struct thread_struct *thr = NULL;
  for (int i = 0; i < NR_THREADSxTASK; ++i)
    if (current()->threads[i] != NULL && current()->threads[i]->TID == id) thr = current()->threads[i];
  if (thr == NULL) {
    current_thread()->errno = ESRCH;
    return -ESRCH;
  }

  /* Check if joining self */
  if (thr == current_thread()) {
    current_thread()->errno = ESRCH;
    return -ESRCH;
  }

  /* Check if thread(id) is joinable */
  if (thr->joinable == 0) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }
  else thr->joinable = 0;

  if (thr->state != ST_ZOMBIE)
    /* Block caller thread while finishing */
    force_thread_switch_to_blocked(&(thr->notifyAtExit));
  
  if (retval != NULL)
    *retval = thr->result;
  
  /* Free TCB of ZOMBIE */
  thr->TID = -1;
  list_add(&thr->list, &freeThread);
  unlink_process_and_thread(current(), thr);

	return 0;
}

void zombify_and_wakeup()
{
  /* Make ST_ZOMBIE and Wake up calls */
  current_thread()->state = ST_ZOMBIE;

  struct list_head *lhcurrent = NULL;
  struct thread_struct *thr;

  while (!list_empty(&current_thread()->notifyAtExit))
  {
    lhcurrent = list_first(&current_thread()->notifyAtExit);

    thr = (struct thread_struct*)list_head_to_thread_struct(lhcurrent);
    update_thread_state_rr(thr, &thr->Dad->readyThreads);
  }

  /* Free all resources but TCB */
  page_table_entry *process_PT = get_PT(current());
  free_frame( get_frame(process_PT, current_thread()->pag_userStack) );
  del_ss_pag(process_PT, current_thread()->pag_userStack);

  if (num_threads(current()) == 1)
    /* Last thread on exit */
    sys_exit();
  else
    /* Force scheduling without adding to queues */
    sched_next_thread();
}

void sys_pthread_exit(int *retval)
{
  /* Store result */
  if (current_thread()->joinable && retval != NULL)
    current_thread()->result = *retval;

  zombify_and_wakeup();
}

void sys_pthread_ret()
{
  /* Called when created thread finishes */
  /* User Stack is as follow:
     ...
     %eax == Thread result
     %ebx
     %ebp
     *arg <- ((pag_userStack+1)<<12)-4 */

  /* Store result */
  unsigned int *p = (unsigned int*) (((current_thread()->pag_userStack+1)<<12)-4*4);
  copy_from_user(p, &(current_thread()->result), sizeof(int));

  zombify_and_wakeup();
}

int sys_sem_init(int* id, unsigned int value)
{
  struct list_head *lhcurrent = NULL;
  struct sem_t *sem;
  
  /* Any free semafor? */
  if (list_empty(&freeSemafor)) {
    current_thread()->errno = ENOMEM;
    return -ENOMEM;
  }
  /* Incorrect value */
  if (value > NR_SEMAFORS || value < 0) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }

  lhcurrent = list_first(&freeSemafor);
  list_del(lhcurrent);

  sem = (struct sem_t*)list_head_to_sem_t(lhcurrent);

  *id = sem->id;
  sem->count = value;
  sem->in_use = 1;
  INIT_LIST_HEAD(&(sem->blocked));

  return 0;
}

int sys_sem_wait(int id)
{
  /* Invalid id */
  if (id < 0 || id >= NR_SEMAFORS) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }

  struct sem_t *sem = &semafors[id];

  /* Uninited semafor */
  if (sem->in_use == 0) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }

  sem->count--;
  if (sem->count < 0)
    /* Block thread */
    force_thread_switch_to_blocked(&sem->blocked);

  return 0;
}

int sys_sem_post(int id)
{
  /* Invalid id */
  if (id < 0 || id >= NR_SEMAFORS) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }

  struct sem_t *sem = &semafors[id];

  /* Uninited semafor */
  if (sem->in_use == 0) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }

  sem->count++;
  if (sem->count <= 0)
  {
    /* Wake one thread on blocked queue */
    struct list_head *lhcurrent = list_first(&sem->blocked);
    /* update_thread_state_rr already does list_del() */

    struct thread_struct* thr = list_head_to_thread_struct(lhcurrent);

    update_thread_state_rr(thr, &(thr->Dad->readyThreads));
  }

  return 0;
}

int sys_sem_destroy(int id)
{
  /* Invalid id */
  if (id < 0 || id >= NR_SEMAFORS) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }

  struct sem_t *sem = &semafors[id];

  /* Uninited semafor */
  if (sem->in_use == 0) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }

  /* Semafor still in use */
  if (!list_empty(&sem->blocked)) {
    current_thread()->errno = EINVAL;
    return -EINVAL;
  }

  sem->count = 0;
  sem->in_use = 0;
  list_add(&sem->list, &freeSemafor);

  return 0;
}
