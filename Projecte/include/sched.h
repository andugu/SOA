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
#define NR_SEMAFORS 		20
#define KERNEL_STACK_SIZE   1024
#define USER_STACK_SIZE		1024

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
  int PID;								/* Process ID. This MUST be the first field of the struct. */
  page_table_entry * dir_pages_baseAddr;/* Process directory */
  struct list_head list;				/* Task struct enqueuing */
  enum state_t state;					/* State of the process */
  struct stats p_stats;					/* Process stats */
  int total_quantum;					/* Total quantum of the process */
  struct list_head readyThreads;		/* List of threads of process ready to exec */
  struct thread_union* threads[NR_THREADS]; /* Pointers to threads of the process */
};

struct thread_struct {
	int TID;							/* Thread ID */
	int total_quantum;					/* Total quantum of the thread */
	struct list_head list; 				/* Thread struct enqueuing */
	enum state_t state;					/* State of the thread */
	struct stats t_stats;				/* Thread stats */
	struct localStorage_struct storage; /* Thread private storage */
	unsigned long userStack				/* Pointer to start of User Stack */
};

struct localStorage_struct {
	int errno;
	unsigned long ebx;
	unsigned long ecx;
	unsigned long edx;
	unsigned long esi;
	unsigned long edi;
	unsigned long ebp;
	unsigned long eax;
	unsigned long ds;
	unsigned long es;
	unsigned long fs;
	unsigned long gs;
	unsigned long eip;
	unsigned long cs;
	unsigned long flags;
	unsigned long esp;
	unsigned long ss;
};

union thread_union {
  struct thread_struct thread;
  unsigned long stack[KERNEL_STACK_SIZE]; /* Thread System Stack */
};

struct sem_t {
	int id; /* Semafor ID */
	int count; /* Blocked counter */
	struct list_head list; /* Semafor struct enqueuing */
	struct list_head blocked; /* Threads blocked by semafor */
};

extern task_struct protected_tasks[NR_TASKS+2];
extern task_struct *task; /* Vector de tasques */

extern union thread_union protected_thread[NR_THREADS+2];
extern union thread_union *thread; /* Vector de threads */

extern struct task_struct *idle_task;

extern struct sem_t semafors[NR_SEMAFORS]; /* Vector de semafors */


#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP       	KERNEL_ESP(&task[1])

extern struct list_head freequeue;
extern struct list_head readyqueue;

extern struct list_head freeThread;

extern struct list_head freeSemafor;


/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

void schedule(void);

struct task_struct * current();

void task_switch(union task_union*t);
void switch_stack(int * save_sp, int new_sp);

void sched_next_rr(void);

void force_task_switch(void);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();
void update_process_state_rr(struct task_struct *t, struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();

void init_stats(struct stats *s);

#endif  /* __SCHED_H__ */
