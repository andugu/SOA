/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <types.h>
#include <hardware.h>
#include <segment.h>
#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>
#include <p_stats.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
struct task_struct protected_tasks[NR_TASKS+2];
struct task_struct *task = &protected_tasks[1]; /* == struct task_struct task[NR_TASKS] */

union thread_union protected_thread[NR_THREADS]
  __attribute__((__section__(".data.task")));
union thread_union *thread = &protected_thread[0]; /* == union thread_union thread[NR_TASKS] */

// Semafors
struct sem_t semafors[NR_SEMAFORS];

extern struct list_head blocked;

// Free task structs
struct list_head freequeue;
// Ready queue
struct list_head readyqueue;
// freeThread queue
struct list_head freeThread;
// freeSemafor queue
struct list_head freeSemafor;

void init_stats(struct stats *s)
{
	s->user_ticks = 0;
	s->system_ticks = 0;
	s->blocked_ticks = 0;
	s->ready_ticks = 0;
	s->elapsed_total_ticks = get_ticks();
	s->total_trans = 0;
	s->remaining_ticks = get_ticks();
}

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}

/* allocate_DIR - allocated a dir for a task 't' using the task vector */
int allocate_DIR(struct task_struct *t) 
{
	int pos;
	pos = ((int)t-(int)task)/sizeof(struct task_struct);
	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

#define DEFAULT_QUANTUM 10
#define DEFAULT_QUANTUM_THREAD 5

int remaining_quantum = 0;
int remaining_thread_quantum = 0;

struct task_struct *idle_task=NULL;
struct thread_struct *idle_thread=NULL;

void update_sched_data_rr(void)
{
  remaining_quantum--;
  remaining_thread_quantum--;
}

int needs_sched_rr(void)
{
  if ((remaining_quantum==0)&&(!list_empty(&readyqueue))) return 1;
  if (remaining_quantum==0) remaining_quantum=current()->total_quantum;
  return 0;
}

int needs_sched_thread(void)
{
  if ((remaining_thread_quantum==0)&&(!list_empty(&(current()->readyThreads)))) return 1;
  if (remaining_thread_quantum==0) remaining_thread_quantum=current_thread()->total_quantum;
  return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
  if (t->state!=ST_RUN) list_del(&(t->list));
  if (dst_queue!=NULL)
  {
    list_add_tail(&(t->list), dst_queue);
    if (dst_queue!=&readyqueue) t->state=ST_BLOCKED;
    else
    {
      update_stats(&(t->p_stats.system_ticks), &(t->p_stats.elapsed_total_ticks));
      t->state=ST_READY;
    }
  }
  else
  {
    t->state=ST_RUN;
  }

  /* TODO: why? */
  update_thread_state_rr(current_thread(), &(current()->readyThreads));
}

void update_thread_state_rr(struct thread_struct *t, struct list_head *dst_queue)
{
  if (t->state!=ST_RUN) list_del(&(t->list));
  if (dst_queue!=NULL)
  {
    list_add_tail(&(t->list), dst_queue);
    if (dst_queue!=&(current()->readyThreads)) t->state=ST_BLOCKED;
    else
    {
      //update_stats(&(t->t_stats.system_ticks), &(t->t_stats.elapsed_total_ticks));
      t->state=ST_READY;
    }
  }
  else t->state=ST_RUN;
}

void sched_next_rr(void)
{
  struct list_head *e;
  struct task_struct *t;

  if (!list_empty(&readyqueue)) {
    e = list_first(&readyqueue);
    list_del(e);
    t=list_head_to_task_struct(e);
  }
  else t=idle_task;

  t->state=ST_RUN;
  remaining_quantum=t->total_quantum;

  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
  //update_stats(&(t->p_stats.ready_ticks), &(t->p_stats.elapsed_total_ticks));
  t->p_stats.total_trans++;

  sched_next_thread_of_proc(t);
}

void sched_next_thread(void)
{
  struct list_head *e;
  struct thread_struct *t;

  if (!list_empty(&(current()->readyThreads)))
  {
    e = list_first(&(current()->readyThreads));
    list_del(e);
    t = list_head_to_thread_struct(e);
  } else t=idle_thread;

/*
  else
  {
    if (!check_current_threads_blocked())
      // If a process has no unblocked threads left
      force_task_switch_to_blocked();
    else
      // No threads to run but not blocked
      force_task_switch();
  } 
*/

  t->state=ST_RUN;
  remaining_thread_quantum=t->total_quantum;

  //update_stats(&(current_thread()->t_stats.system_ticks), &(current_thread()->t_stats.elapsed_total_ticks));
  update_stats(&(t->t_stats.ready_ticks), &(t->t_stats.elapsed_total_ticks));
  t->t_stats.total_trans++;

  thread_switch((union thread_union*)t);
}

void sched_next_thread_of_proc(struct task_struct* c)
{
  struct list_head *e;
  struct thread_struct *t;

  if (!list_empty(&(c->readyThreads)))
  {
    e = list_first(&(c->readyThreads));
    list_del(e);
    t=list_head_to_thread_struct(e);
  } else t = idle_thread;

/*
  else
  {
    if (!check_current_threads_blocked())
      // If a process has no unblocked threads left
      force_task_switch_to_blocked();
    else
      // No threads to run but not blocked
      force_task_switch();
  }*/

  t->state=ST_RUN;
  remaining_thread_quantum=t->total_quantum;

  //update_stats(&(current_thread()->t_stats.system_ticks), &(current_thread()->t_stats.elapsed_total_ticks));
  update_stats(&(t->t_stats.ready_ticks), &(t->t_stats.elapsed_total_ticks));
  t->t_stats.total_trans++;

  if (t != current_thread()) {
// In force_task_switch, a thread could thread_switch with itself
    set_cr3(get_DIR(t->Dad));
    thread_switch((union thread_union*)t);
  }
}

void schedule()
{
  update_sched_data_rr();
  /* Process Level Scheduling */
  if (needs_sched_rr())
  {
    update_process_state_rr(current(), &readyqueue);
    sched_next_rr();
  }
  /* Thread Level Scheduling */
  else if (needs_sched_thread())
  {
    update_thread_state_rr(current_thread(), &(current()->readyThreads));
    sched_next_thread();
  }
}

void setMSR(unsigned long msr_number, unsigned long high, unsigned long low);

void init_idle (void)
{
  struct list_head *l = list_first(&freequeue);
  list_del(l);
  struct task_struct *c = list_head_to_task_struct(l);
  INIT_LIST_HEAD(&(c->readyThreads));
  
  struct list_head *th = list_first(&freeThread);
  list_del(th);
  struct thread_struct *thr = list_head_to_thread_struct(th);
  union thread_union *uc = (union thread_union*)thr;
  link_process_with_thread(c, thr);

  thr->TID = 0;
  thr->total_quantum=DEFAULT_QUANTUM_THREAD;
  INIT_LIST_HEAD(&(thr->notifyAtExit));

  c->PID=0;
  c->total_quantum=DEFAULT_QUANTUM;

  init_stats(&c->p_stats);
  init_stats(&thr->t_stats);

  allocate_DIR(c);

  /* Return address */
  uc->stack[KERNEL_STACK_SIZE-1]=(unsigned long)&cpu_idle;
  /* Register %ebp */
  uc->stack[KERNEL_STACK_SIZE-2]=0;

  /* Top of the stack */
  thr->kernel_esp=(int)&(uc->stack[KERNEL_STACK_SIZE-2]);

  idle_task=c;
  idle_thread=thr;
}

void init_task1(void)
{
  struct list_head *l = list_first(&freequeue);
  list_del(l);
  struct task_struct *c = list_head_to_task_struct(l);
  INIT_LIST_HEAD(&(c->readyThreads));

  struct list_head *th = list_first(&freeThread);
  list_del(th);
  struct thread_struct *thr = list_head_to_thread_struct(th);
  union thread_union *uc = (union thread_union*)thr;
  link_process_with_thread(c, thr);
  /* link_process_with_thread adds thr by default to readyThreads */
  list_del(&(thr->list));

  thr->TID = 1;
  thr->total_quantum = DEFAULT_QUANTUM_THREAD;
  thr->state = ST_RUN;
  thr->joinable = 1;
  INIT_LIST_HEAD(&(thr->notifyAtExit));

  c->PID = 1;
  c->total_quantum = DEFAULT_QUANTUM;
  c->state = ST_RUN;

  remaining_quantum = c->total_quantum;
  remaining_thread_quantum = thr->total_quantum;

  init_stats(&c->p_stats);
  init_stats(&thr->t_stats);

  allocate_DIR(c);

  set_user_pages(c);

  tss.esp0=(DWord)&(uc->stack[KERNEL_STACK_SIZE]);
  setMSR(0x175, 0, (unsigned long)&(uc->stack[KERNEL_STACK_SIZE]));

  set_cr3(c->dir_pages_baseAddr);
}

void init_freequeue()
{
  INIT_LIST_HEAD(&freequeue);

  /* Insert all task structs in the freequeue */
  for (int i = 0; i < NR_TASKS; i++)
  {
    task[i].PID=-1;
    list_add_tail(&(task[i].list), &freequeue);
  }
}

void init_freeThread()
{
  INIT_LIST_HEAD(&freeThread);

  /* Insert all thread structs in the freeThread */
  for (int i = 0; i < NR_THREADS; i++)
  {
    thread[i].thread.TID=-1;
    thread[i].thread.joinable=0;
    list_add_tail(&(thread[i].thread.list), &freeThread);
  }
}

void init_freeSemafor()
{
  INIT_LIST_HEAD(&freeSemafor);

  /* Insert all thread structs in the freeThread */
  for (int i = 0; i < NR_SEMAFORS; i++)
  {
    semafors[i].id=i;
    semafors[i].in_use=0;
    list_add_tail(&(semafors[i].list), &freeSemafor);
  }
}

void init_sched()
{
  init_freequeue();
  init_freeThread();
  init_freeSemafor();
  INIT_LIST_HEAD(&readyqueue);
  INIT_LIST_HEAD(&blocked);
}

struct thread_struct* current_thread()
{
  int ret_value;
  return (struct thread_struct*)( ((unsigned int)&ret_value) & 0xfffff000);
}

struct task_struct* current()
{
  return current_thread()->Dad;
}

struct thread_struct* list_head_to_thread_struct(struct list_head *l)
{
  return list_entry(l, struct thread_struct, list);
}

struct task_struct* list_head_to_task_struct(struct list_head *l)
{
  return list_entry(l, struct task_struct, list);
}

struct sem_t* list_head_to_sem_t(struct list_head *l)
{
  return list_entry(l, struct sem_t, list);
}

/* Do the magic of a thread switch */
void inner_thread_switch(union thread_union *new)
{
  /* Update TSS and MSR to make it point to the new stack */
  tss.esp0=(int)&(new->stack[KERNEL_STACK_SIZE]);
  setMSR(0x175, 0, (unsigned long)&(new->stack[KERNEL_STACK_SIZE]));

  switch_stack((int*)(&current_thread()->kernel_esp), new->thread.kernel_esp);
}

/* Checks if current process has unblocked threads 
 * Pre: none
 * Post:
      0 -> all threads of current process blocked 
      1 -> not all threads blocked */
int check_current_threads_blocked()
{
  int ret = 0;

  for (int i = 0; i < NR_THREADS; ++i)
    if (current()->threads[i] != NULL && current()->threads[i]->state != ST_BLOCKED)
      ret = 1;

  return ret;
}

/* Checks if a process 't' has unblocked threads 
 * Pre: none
 * Post:
      0 -> all threads of current process blocked 
      1 -> not all threads blocked */
int check_blocked_threads(struct task_struct *t)
{
  int ret = 0;

  for (int i = 0; i < NR_THREADS; ++i)
    if (t->threads[i] != NULL && t->threads[i]->state != ST_BLOCKED)
      ret = 1;

  return ret;
}

/* Force a task switch assuming that the scheduler does not work with priorities */
void force_task_switch()
{
  update_process_state_rr(current(), &readyqueue);
  sched_next_rr();
}

/* Force task switch when no unblocked threads left */
void force_task_switch_to_blocked()
{
  update_process_state_rr(current(), &blocked);
  sched_next_rr();
}

/* Force a thread switch */
void force_thread_switch()
{
  update_thread_state_rr(current_thread(), &(current()->readyThreads));
  sched_next_thread();
}

/* Force a thread switch to a specific blocked list */
void force_thread_switch_to_blocked(struct list_head* blocked)
{
  update_thread_state_rr(current_thread(), blocked);
  sched_next_thread();
}

/* Links a process and a thread 
 * Pre: thr must be a free thread not on freeThread list
 * Post:
      0  -> OK!
      -1 -> pro already has 10 threads
      -2 -> pro already has a thread with the same TID */
int link_process_with_thread(struct task_struct* pro, struct thread_struct* thr)
{
  int pos = -1;
  for (int i=0; i < NR_THREADS; i++) {
    if (pro->threads[i] != NULL && pro->threads[i]->TID == thr->TID) return -2;
    if (pos == -1 && (pro->threads[i] == NULL || pro->threads[i]->TID == -1)) pos = i;
  }
  if (pos < 0) return pos;
  pro->threads[pos] = thr;
  list_add_tail(&(thr->list), &(pro->readyThreads));
  thr->Dad = pro;
  return 0;
}

/* Number of threads in process
 * Pre: None
 * Post: Number of threads in pro*/
int num_threads(struct task_struct* pro) {
  int n = 10;
  for (int i=0; i < NR_THREADS; i++) {
     if (pro->threads[i] == NULL || pro->threads[i]->TID == -1) --n;
  }
  return n;
}
