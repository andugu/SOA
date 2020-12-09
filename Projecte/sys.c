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

void * get_ebp();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
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

int global_PID=1000;
int global_TID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  /* Any free task_struct and thread_struct? */
  if (list_empty(&freequeue) || list_empty(&freeThread)) return -ENOMEM;

  struct list_head *lhcurrent = list_first(&freequeue);
  list_del(lhcurrent);
  struct task_struct *lh = list_head_to_task_struct(lhcurrent);

  struct list_head *thr = list_first(&freeThread);
  list_del(thr);
  union thread_union *uchild =(union thread_union*)list_head_to_thread_struct(thr);

  // lh => new task (task_struct)
  // uchild => new thread (thread_union)
  
  /* Copy the parent's thread union to child's */
  copy_data(current_thread(), uchild, sizeof(union thread_union));
  /* new pages dir */
  allocate_DIR(lh);
  
  /************************************	TODO: STACK MISSING!	***********************************/
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(lh);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
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
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  lh->PID=++global_PID;
  lh->state=ST_READY;
  uchild->thread.TID = ++global_TID;
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
  list_add_tail(&(uchild->thread.list), &(lh->readyThreads));
  
  return lh->PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
  char localbuffer [TAM_BUFFER];
  int bytes_left;
  int ret;

  if ((ret = check_fd(fd, ESCRIPTURA)))
	return ret;
  if (nbytes < 0)
	return -EINVAL;
  if (!access_ok(VERIFY_READ, buffer, nbytes))
	return -EFAULT;

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
  return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
  int i;

  // free all remaining threads -> millor for del vector que anar fent sys_pthread_exit perquè així ens assegurem que un thread bloquejat per semàfor no evita ser alliberat
  for (i = 0; i < NR_THREADS; i++) {
    if (current()->threads[i]->TID != -1) {
      if (current()->threads[i]->state != ST_RUN) {
        list_del(&(current()->threads[i]->list));
      }
      list_add_tail(&(current()->threads[i]->list), &freeThread);
      current()->threads[i]->TID = -1;
    }
  }

  // TODO: Free the thread's stack frame????

  page_table_entry *process_PT = get_PT(current());
  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
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
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid < 0) return -EINVAL;
  for (int i = 0; i < NR_TASKS; i++)
  {
    if (task[i].PID == pid)
    {
      task[i].p_stats.remaining_ticks = remaining_quantum;
      copy_to_user(&(task[i].p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}

int sys_pthread_create(int *id, unsigned int* start_routine, void *arg)
{
  struct list_head *lhcurrent = NULL;
  
  /* Any free threads? */
  if (list_empty(&freeThread) || num_threads(current()) == 10) return -ENOMEM;

  lhcurrent = list_first(&freeThread);
  list_del(lhcurrent);

  struct thread_struct *thr = (struct thread_struct*)list_head_to_thread_struct(lhcurrent);
  union thread_union *thr_u = (union thread_union*)list_head_to_thread_struct(lhcurrent);

  /* Copy thread_union from caller */
  copy_data(current_thread(), thr_u, sizeof(union thread_union));
  
  /* Update thread_union with new values */
  thr->TID=++global_TID;
  *id = thr->TID;
  thr->state = ST_READY;
  init_stats(&thr->t_stats);
  /* TODO: crash? */
  INIT_LIST_HEAD(&(thr->notifyAtExit));

  if (link_process_with_thread(current(), thr) < 0) return -ENOMEM;
  /* link_process_with_thread already adds thr to readyThreads */

  /* Allocate User Stack in memory */
  page_table_entry *process_PT = get_PT(current());
  /* Find free logical pag */
  int pos = -1;
  for (int pag = 0; pos != -1 && pos < TOTAL_PAGES-PAG_LOG_INIT_DATA; pag++)
    if (process_PT[PAG_LOG_INIT_DATA+pag].entry == 0) pos = pag;
  thr->pag_userStack = PAG_LOG_INIT_DATA+pos;
  /* Find free physical pag */
  int new_ph_pag = alloc_frame();
  /* Bind pages together */
  set_ss_pag(process_PT, thr->pag_userStack, new_ph_pag);

  /* TODO: Build up User Stack */
  // int new_ebp = (thr->Pag_userStack+1)<<12;
  // TODO: args?
  thr->kernel_esp = (unsigned int) &(thr_u->stack[KERNEL_STACK_SIZE - 18]);
  /* %eip */
  thr_u->stack[KERNEL_STACK_SIZE-5] = (int)start_routine;
  /* %ebp */
  thr_u->stack[KERNEL_STACK_SIZE-11] = -1;
  /* %esp */
  thr_u->stack[KERNEL_STACK_SIZE-2] = -1;

  return 0;
}

/* The pthread_exit() function terminates the calling thread and returns
  a value via retval that (if the thread is joinable) is available to
  another thread in the same process that calls pthread_join(3).

  Performing a return from the start function of any thread other than
  the main thread results in an implicit call to pthread_exit(), using
  the function's return value as the thread's exit status.

  After the last thread in a process terminates, the process terminates
  as by calling exit(3) with an exit status of zero; thus, process-
  shared resources are released and functions registered using
  atexit(3) are called. */
void sys_pthread_exit(int *retval)
{

  current_thread()->state = ST_ZOMBIE;
  *retval = current_thread()->result;

  if (num_threads(current()) == 0)
  {
    current_thread()->TID = -1;
    list_add_tail(current_thread()->list, &freeThread);
    exit();
  }

	return;
}

int sys_pthread_join(int id, int *retval)
{
  /* Check if thread(id) exists */
  struct thread_struct *thr = NULL;
  for (int i = 0; i < NR_THREADS; ++i)
    if (current()->threads[i]->TID == id) thr = &threads[i];
  if (thr == NULL) return -ESRCH;

  /* Check if joining self */
  if (thr == current_thread()) return -ESRCH;

  /* Check if thread(id) is joinable */
  if (thr->joinable == 0) return -EINVAL;
  else thr->joinable = 0;

  if (thr->state != ST_ZOMBIE)
    /* Block caller thread while finishing */
    force_thread_switch_to_blocked(&thr->notifyAtExit);
  else
  {
    if (retval != NULL)
      *retval = thr->result;
  }

	return 0;
}

int sys_sem_init(int* id, unsigned int value)
{
  struct list_head *lhcurrent = NULL;
  struct sem_t *sem;
  
  /* Any free semafor? */
  if (list_empty(&freeSemafor)) return -ENOMEM;
  /* Incorrect value */
  if (value > NR_THREADS || value < 0) return -EINVAL;

  lhcurrent = list_first(&freeSemafor);
  list_del(lhcurrent);

  sem = (struct sem_t*)list_head_to_sem_t(lhcurrent);

  *id = sem->id;
  sem->count = value;
  sem->in_use = 1;
  /* TODO: Crash? */
  INIT_LIST_HEAD(&(sem->blocked));

  return 0;
}

int sys_sem_wait(int id)
{
  /* Invalid id */
  if (id < 0 || id >= NR_SEMAFORS) return -EINVAL;

  struct sem_t *sem = &semafors[id];

  /* Uninited semafor */
  if (sem->in_use == 0) return -EINVAL;

  sem->count--;
  if (sem->count < 0)
    /* Block thread */
    force_thread_switch_to_blocked(&sem->blocked);

  return 0;
}

int sys_sem_post(int id)
{
  /* Invalid id */
  if (id < 0 || id >= NR_SEMAFORS) return -EINVAL;

  struct sem_t *sem = &semafors[id];

  /* Uninited semafor */
  if (sem->in_use == 0) return -EINVAL;

  sem->count++;
  if (sem->count <= 0)
  {
    /* Wake one thread on blocked queue */
    struct list_head *lhcurrent = list_first(&sem->blocked);
    /* update_thread_state_rr already does list_del() */

    struct thread_struct* thr = list_head_to_thread_struct(lhcurrent);

    if (!check_blocked_threads(thr->Dad)){
      /* All threads of process blocked */
      /* Process is in ST_BLOCKED state */
      update_process_state_rr(thr->Dad, &readyqueue);
    }

    update_thread_state_rr(thr, &(thr->Dad->readyThreads));
  }

  return 0;
}

int sys_sem_destroy(int id)
{
  /* Invalid id */
  if (id < 0 || id >= NR_SEMAFORS) return -EINVAL;

  struct sem_t *sem = &semafors[id];

  /* Uninited semafor */
  if (sem->in_use == 0) return -EINVAL;

  /* Semafor still in use */
  if (!list_empty(&sem->blocked)) return -EINVAL;

  sem->count = 0;
  sem->in_use = 0;
  list_add(&sem->list, &freeSemafor);

  return 0;
}
