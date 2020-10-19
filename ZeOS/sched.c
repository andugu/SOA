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
	// TODO: 4. Init a execution context?
	pcb->PID = 0;
	allocate_DIR(pcb);

	idle_task = pcb;
}

void init_task1(void)
{
	struct list_head   *first_free = list_first(&freequeue);
	struct task_struct *pcb        = list_head_to_task_struct(first_free);
	list_del(first_free);

	pcb->PID = 1;	
	pcb->dir_pages_baseAddr = allocate_DIR(pcb); //No s'ha de fer l'assignació

	// TODO: 3. Address space? -------> set_user_pages(pcb);
	// TODO: 4. TSS?

	set_cr3(pcb->dir_pages_baseAddr);
}


void init_sched()
{
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);

	for (int i = 0; i < NR_TASKS; ++i)
		list_add_tail(&(task[i].task.list), &freequeue);
}

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
	return list_entry(l, struct task_struct, list);
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

