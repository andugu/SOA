/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <interrupt.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

struct list_head freequeue;
struct list_head readyqueue;

struct task_struct *idle_task;

int execution_quantum;

extern struct list_head blocked;

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

int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

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

void init_idle (void)
{
	struct list_head   *first_free = list_first(&freequeue);
	struct task_struct *pcb        = list_head_to_task_struct(first_free);
	list_del(first_free);

	// Init stadistics
	pcb->stadistics.user_ticks = 0;
	pcb->stadistics.system_ticks = 0;
	pcb->stadistics.blocked_ticks = 0;
	pcb->stadistics.ready_ticks = 0;
	pcb->stadistics.elapsed_total_ticks = get_ticks();
	pcb->stadistics.total_trans = 0;
	pcb->stadistics.remaining_ticks = get_ticks();

	pcb->PID = 0;
	pcb->quantum = 0;
	allocate_DIR(pcb);
	pcb->status = ST_READY;

	idle_task = pcb;

	union task_union *a = (union task_union*) pcb;
	a->stack[KERNEL_STACK_SIZE - 1] = (unsigned long int) &cpu_idle;
	a->stack[KERNEL_STACK_SIZE - 2] = 0;
	pcb->kernel_esp = &a->stack[KERNEL_STACK_SIZE - 2];
}

void init_task1(void)
{
	struct list_head   *first_free = list_first(&freequeue);
	struct task_struct *pcb        = list_head_to_task_struct(first_free);
	list_del(first_free);

	// Init stadistics
	pcb->stadistics.user_ticks = 0;
	pcb->stadistics.system_ticks = 0;
	pcb->stadistics.blocked_ticks = 0;
	pcb->stadistics.ready_ticks = 0;
	pcb->stadistics.elapsed_total_ticks = get_ticks();
	pcb->stadistics.total_trans = 0;
	pcb->stadistics.remaining_ticks = get_ticks();

	pcb->PID = 1;
	pcb->quantum = execution_quantum = DEFAULT_QUANTUM;
	pcb->status = ST_RUN;
	allocate_DIR(pcb);

	set_user_pages(pcb);

	union task_union *a = (union task_union*) pcb;
	tss.esp0 = KERNEL_ESP(a);

	set_cr3(get_DIR(pcb));
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

void task_switch(union task_union* t)
{
	__asm__ __volatile__(
		"pushl %esi\n\t"
		"pushl %edi\n\t"
		"pushl %ebx\n\t"
		);

	inner_task_switch(t);

	__asm__ __volatile__(
		"popl %ebx\n\t"
		"popl %edi\n\t"
		"popl %esi\n\t"
		);
}

void inner_task_switch(union task_union* t)
{
	tss.esp0 = KERNEL_ESP(t);
	writeMSR(0x175, (int) KERNEL_ESP(t));
	set_cr3(get_DIR(&t->task));

	__asm__ __volatile__(
		"movl %%ebp, %0\n\t"
		"movl %1, %%esp\n\t"
		"popl %%ebp\n\t"
		"ret\n\t"
		: "=g" (current()->kernel_esp)
		: "g" (t->task.kernel_esp)
	);
}

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
	return list_entry(l, struct task_struct, list);
}

void init_sched()
{
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);

	for (int i = 0; i < NR_TASKS; ++i)
		list_add_tail(&(task[i].task.list), &freequeue);
}

void schedule()
{	
	update_sched_data_rr();
	if(needs_sched_rr())
	{
		update_process_state_rr(current(), &readyqueue);
		sched_next_rr();
	}
}

int strlen(char *a) {
	int i = 0;
	while (a[i]!=0)i++;
	return i;
}

void sched_next_rr()
{
	struct task_struct *task;

	if (list_empty(&readyqueue))
		task = idle_task;
	else
	{
		struct list_head *elem = list_first(&readyqueue);
		list_del(elem);
		task = list_head_to_task_struct(elem);
	}

	task->status = ST_RUN;
	execution_quantum = get_quantum(task);

	update_ticks_struct(&(task->stadistics.ready_ticks), &(task->stadistics.elapsed_total_ticks));
	update_ticks_struct(&(current()->stadistics.system_ticks), &(current()->stadistics.elapsed_total_ticks));

	char a[64];
	itoa(current()->PID, a);
	sys_write(1, a, strlen(a));

	task_switch((union task_union*) task);
}

void update_process_state_rr(struct task_struct *t, struct list_head *dest)
{
	if (t->status != ST_RUN) list_del(&(t->list));
	if (dest != NULL)
	{
		list_add_tail(&(t->list), dest);
		if (dest != &readyqueue) t->status = ST_BLOCKED;
		else
		{
			t->status = ST_READY;
			update_ticks_struct(&(t->stadistics.system_ticks), &(t->stadistics.elapsed_total_ticks));
		}
	}
	else
		t->status = ST_RUN;
}

int needs_sched_rr()
{
	if (execution_quantum <= 0 && (!list_empty(&readyqueue))) return 1;
	else if (execution_quantum <= 0) execution_quantum = get_quantum(current());
	return 0;
}

void update_sched_data_rr()
{
	execution_quantum -= 1;
}

int get_quantum (struct task_struct *t)
{
	return t->quantum;
}

void set_quantum (struct task_struct *t, int new_quantum)
{
	t->quantum = new_quantum;
}
