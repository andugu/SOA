/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

struct list_head freequeue;
struct list_head readyqueue;

struct task_struct *idle_task;

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

	pcb->PID = 0;
	allocate_DIR(pcb);

	idle_task = pcb;

	union task_union *a = (union task_union)pcb;
	a->stack[a.stack.size() - 1] = (DWord) cpu_idle();
	a->stack[a.stack.size() - 2] = 0;
	pcb->kernel_esp = &a.stack[a.stack.size() - 2];

	/*__asm__ __volatile__(
		"movl %0, %%esp\n\t"
		"pop %%ebp\n\t"
		"ret\n\t"
		: // no output
		: "g" (pcb->kernel_esp));
	*/
}

void init_task1(void)
{
	struct list_head   *first_free = list_first(&freequeue);
	struct task_struct *pcb        = list_head_to_task_struct(first_free);
	list_del(first_free);

	pcb->PID = 1;	
	allocate_DIR(pcb);

	set_user_pages(pcb);

	union task_union *a = (union task_union)pcb;
	tss.esp0 = KERNEL_ESP(a);

	set_cr3(pcb->dir_pages_baseAddr);

	// return_gate(Word ds, Word ss, DWord esp, Word cs, DWord eip);
}

void init_sched()
{
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);

	for (int i = 0; i < NR_TASKS; ++i)
		list_add_tail(&(task[i].task.list), &freequeue);
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
		"pushl %%esi\n\t"
		"pushl %%edi\n\t"
		"pushl %%ebx\n\t"
		);

	inner_task_switch(t);

	__asm__ __volatile__(
		"popl %%ebx\n\t"
		"popl %%edi\n\t"
		"popl %%esi\n\t"
		);
}

void inner_task_switch(union task_union* t)
{
	writeMSR(0x175, (int) KERNEL_ESP(t));
	set_cr3(t->task->dir_pages_baseAddr);

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
