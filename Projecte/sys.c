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
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS; 
}

int sys_getpid()
{
	return current()->PID;
}

int global_PID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
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
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
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

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
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

int global_TID=1000;

int sys_pthread_create(int *id, unsigned int* start_routine, void *arg)
{
  struct list_head *lhcurrent = NULL;
  struct thread_struct *thr;
  
  /* Any free threads? */
  if (list_empty(&freeThread)) return -ENOMEM;

  lhcurrent = list_first(&freeThread);
  list_del(lhcurrent);

  thr = (struct thread_struct*)list_head_to_thread_struct(lhcurrent);
  thr_u = (union thread_union*)list_head_to_thread_struct(lhcurrent);

  if (link_process_with_thread(current(), thr) < 0) return -ENOMEM;
  /* link_process_with_thread already adds thr to readyThreads */

  /* TODO: Add values to regs? */

  /* Redirect exec to @start_routine */
  /* Return address */
  thr_u->stack[KERNEL_STACK_SIZE-1]=(unsigned long)start_routine;
  /* Register %ebp */
  thr_u->stack[KERNEL_STACK_SIZE-2]=0;
  /* Top of the stack */
  thr->kernel_esp=(int)&(thr_u->stack[KERNEL_STACK_SIZE-2]);
  
  /* TODO: User Stack? */

  thr->TID=++global_TID;
  *id = thr->TID;

	return 0;
}

void sys_pthread_exit(int *retval)
{
	return;
}

int sys_pthread_join(int id, int *retval)
{
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
  INIT_LIST_HEAD(sem->blocked);

  return 0;
}

int sys_sem_wait(int id)
{
  /* Invalid id */
  if (id < 0 || id >= NR_SEMAFORS) return -EINVAL;

  struct sem_t *sem = &semafors[id];

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

    update_thread_state_rr(thr, thr->Dad->readyThreads);
  }

  return 0;
}

int sys_sem_destroy(int id)
{
  /* Invalid id */
  if (id < 0 || id >= NR_SEMAFORS) return -EINVAL;

  struct sem_t *sem = &semafors[id];

  /* Semafor still in use */
  if (!list_empty(&sem->blocked)) return -EINVAL;

  sem->count = 0;
  list_add(&sem->list, &freeSemafor);

  return 0;
}
