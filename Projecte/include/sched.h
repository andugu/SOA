/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>
#include <stats.h>


#define NR_TASKS            10
#define NR_THREADS          20
#define NR_SEMAFORS         20
#define KERNEL_STACK_SIZE   1024
#define USER_STACK_SIZE	    1024

enum state_t { ST_RUN, ST_READY, ST_BLOCKED , ST_ZOMBIE};

struct task_struct {
  int PID;                                      /* Process ID. This MUST be the first field of the struct. */
  struct list_head list;                        /* Task struct enqueuing. It MUST be the second field of the struct */
  page_table_entry * dir_pages_baseAddr;        /* Process directory */
  enum state_t state;                           /* State of the process */
  struct stats p_stats;                         /* Process stats */
  int total_quantum;                            /* Total quantum of the process */
  struct list_head readyThreads;                /* List of threads of process ready to exec */
  struct thread_struct* threads[NR_THREADS];    /* Pointers to threads of the process */
};

struct thread_struct {
	int TID;                                   /* Thread ID */
	struct task_struct *Dad;                   /* Pointer to Dad's task_struct */
	int total_quantum;                         /* Total quantum of the thread */
	struct list_head list;                     /* Thread struct enqueuing */
	enum state_t state;                        /* State of the thread */
	struct stats t_stats;                      /* Thread stats */
	unsigned int kernel_esp;                   /* Thread kernel %esp reg */
	unsigned long pag_userStack;               /* Logical page number of User Stack */
	struct list_head notifyAtExit;			   /* List of threads to wake up on exit */
	int joinable;							   /* 1 if the thread is joinable, 0 otherwise */ 
	/* Thread Local Storage */
	int errno;								   /* Errno value of thread */
	int result;								   /* Thread return value */
};

union thread_union {
  struct thread_struct thread;
  unsigned long stack[KERNEL_STACK_SIZE];      /* Thread System Stack */
};

struct sem_t {
	int id;                                    /* Semafor ID */
	int count;                                 /* Blocked counter */
	int in_use;							   	   /* 1 if in use, 0 otherwise */
	struct list_head list;                     /* Semafor struct enqueuing */
	struct list_head blocked;                  /* Threads blocked by semafor */
};

extern struct task_struct protected_tasks[NR_TASKS+2];
extern struct task_struct *task; /* Tasks vector */

extern union thread_union protected_thread[NR_THREADS];
extern union thread_union *thread; /* Threads vector */

extern struct task_struct *idle_task;
extern struct thread_struct *idle_thread;

extern struct sem_t semafors[NR_SEMAFORS]; /* Semafors vector */


#define KERNEL_ESP(t)	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP		KERNEL_ESP(&thread[1])

extern struct list_head freequeue;
extern struct list_head readyqueue;
extern struct list_head freeThread;
extern struct list_head freeSemafor;

void init_task1(void);
void init_idle(void);

void init_sched(void);

void schedule(void);

struct task_struct * current();
struct thread_struct * current_thread();

void thread_switch(union thread_union*t);

void switch_stack(int * save_sp, int new_sp);

void sched_next_rr(void);

int check_current_threads_blocked();
int check_blocked_threads(struct task_struct *t);

void force_task_switch(void);
void force_task_switch_to_blocked(void);
void force_thread_switch(void);
void force_thread_switch_to_blocked(struct list_head* blocked);

struct task_struct *list_head_to_task_struct(struct list_head *l);
struct thread_struct* list_head_to_thread_struct(struct list_head *l);
struct sem_t* list_head_to_sem_t(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();
void sched_next_thread();
void sched_next_thread_of_proc(struct task_struct* c);
void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue);
void update_thread_state_rr(struct thread_struct *t, struct list_head *dst_queue);
int needs_sched_rr();
int needs_sched_thread();
void update_sched_data_rr();

void init_stats(struct stats *s);
int num_threads(struct task_struct* pro);

int link_process_with_thread(struct task_struct* pro, struct thread_struct* thr);

#endif  /* __SCHED_H__ */
