/*
 * sys.c - Syscalls implementation
 */

#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;
extern struct list_head freequeue;
extern struct list_head readyqueue;

unsigned int next_PID = 49;

char kernel_buffer[1024];

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

void free_user_pages( struct task_struct *task );

int ret_form_fork(void) {
	return 0;
}

int sys_fork()
{
	int PID=-1;
	if (list_empty(&freequeue)) return -105; /*ENOBUFS*/
	
	struct list_head *first_free = list_first(&freequeue);
	//struct task_struct *pcb = list_head_to_task_struct(first_free);
	list_del(first_free);
	union task_union *child;

	int pag;
	/*Copy the father's union task_union*/
	copy_data(current(), child, sizeof(union task_union));
	
	/*Copy the father's TP*/
	allocate_DIR(&(child->task)); //Creating a directory for the child
	//plantilla de còpia de frames: void set_user_pages( struct task_struct *task ) de mm.c
	page_table_entry *TP_dad = get_PT(current());
	page_table_entry *TP_child = get_PT(&(child->task));
	for (pag=0;pag<NUM_PAG_CODE;pag++) // Code pages are shared (they are read-only)
	  	TP_child[PAG_LOG_INIT_CODE+pag] = TP_dad[PAG_LOG_INIT_CODE+pag];
	for (pag=0;pag<NUM_PAG_KERNEL;pag++) // Kernel pages are shared
	  	TP_child[pag] = TP_dad[pag]; // Kernel starts at 0


	int new_ph_pag;
	for (pag=0;pag<NUM_PAG_DATA;pag++){ //new frames for child's data
		new_ph_pag=alloc_frame();
		if (new_ph_pag < 0) {
			free_user_pages(&(child->task)); //If fork is aborted -> child's frame pages are freed
			list_add_tail(first_free, &freequeue);
			return -23; /*ENFILE*/
		}
	  	TP_child[PAG_LOG_INIT_DATA+pag].entry = 0;
	  	TP_child[PAG_LOG_INIT_DATA+pag].bits.pbase_addr = new_ph_pag;
	  	TP_child[PAG_LOG_INIT_DATA+pag].bits.user = 1;
	  	TP_child[PAG_LOG_INIT_DATA+pag].bits.rw = 1;
	  	TP_child[PAG_LOG_INIT_DATA+pag].bits.present = 1;
	}
	
	/*Copy father's data frames' data to child's data frames' data*/
	//Finding a dad's free logical page
	int free_page = -1;
	for (pag=0; (pag<TOTAL_PAGES) && (free_page<0); pag++) 
		if (TP_dad[pag].entry == 0) free_page = pag;
	if (free_page < 0) {
		free_user_pages(&(child->task));
		list_add_tail(first_free, &freequeue);
		return -105;/*Dad has no free pages-> ENOBUFS*/
	}
	for (pag=0; pag<NUM_PAG_DATA;pag++){
		set_ss_pag(TP_dad, free_page, TP_child[PAG_LOG_INIT_DATA+pag].bits.pbase_addr);
		copy_data((unsigned int*) ((PAG_LOG_INIT_DATA+pag) << 12), (unsigned int*) (free_page << 12), PAGE_SIZE);
	}
	
	del_ss_pag(TP_dad, free_page);
	set_cr3(current()->dir_pages_baseAddr); //clean dad's TLB
	
	//falta apartat f fins el final
	child->task.PID = PID = next_PID;
	next_PID++;
	
	/*Update child's task_union*/
	child->stack[KERNEL_STACK_SIZE-17] = 0; //new FAKE EBP
	child->task.kernel_esp = (unsigned long*) &(child->stack[KERNEL_STACK_SIZE-17]); // -(11(SAVE_ALL)+4(HW CONTEXT)+1(@ret)+1(EBP)+1(NEW_EBP))
	child->stack[KERNEL_STACK_SIZE-18] = (unsigned long int) &ret_form_fork;
	
	/*Insert child in readyqueue*/
	list_add_tail(first_free, &readyqueue);
	return PID;
}

void sys_exit()
{  
}

int sys_write(int fd, char * buffer, int size)
{
	if (check_fd(fd, ESCRIPTURA) != 0) return check_fd(fd, ESCRIPTURA);
	if (buffer == NULL) return -14; /*EFAULT*/
	if (size < 0) return -22; /*EINVAL*/

	int err;
	while (size > 1024) {
		if (copy_from_user(buffer, kernel_buffer, 1024) < 0) return -1;
		if ((err = sys_write_console(kernel_buffer, 1024)) < 0) return err;
		buffer += 1024;
		size -= 1024;
	}
	if (copy_from_user(buffer, kernel_buffer, size) < 0) return -1;
	return sys_write_console(kernel_buffer, size);
}

int sys_gettime()
{
	return zeos_ticks;
}
