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

unsigned int next_PID = 70;

char kernel_buffer[1024];

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; // EBADF
  if (permissions!=ESCRIPTURA) return -13; // EACCES
  return 0;
}

int sys_ni_syscall()
{
	return -38; // ENOSYS
}

int sys_getpid()
{
	return current()->PID;
}

int ret_form_fork(void) {
	// Uses handler return @
	return 0;
}

int sys_fork()
{
	int PID = -1;

	if (list_empty(&freequeue)) return -105; //ENOBUFS
	
	struct list_head *first_free = list_first(&freequeue);
	list_del(first_free);
	union task_union *child = (union task_union*) list_head_to_task_struct(first_free);
	
	/* Copy father's union task_union */
	// Save list from overwrite 
	struct list_head list = child->task.list;
	copy_data(current(), child, sizeof(union task_union));
	// Restore list
	child->task.list = list;
	
	/* Copy father's TP */
	// Creating a directory for the child
	allocate_DIR(&(child->task));
	// TP's
	page_table_entry *TP_dad = get_PT(current());
	page_table_entry *TP_child = get_PT(&(child->task));

	// Code pages are shared (they are read-only)
	for (int pag = 0; pag < NUM_PAG_CODE; pag++)
	  	TP_child[PAG_LOG_INIT_CODE + pag] = TP_dad[PAG_LOG_INIT_CODE + pag];

	// Kernel pages are shared (Kernel starts at 0)
	for (int pag = 0; pag < NUM_PAG_KERNEL; pag++)
	  	TP_child[pag] = TP_dad[pag];

	/* Finding a dad's free logical page */
	int free_page = -1;
	for (int pag = 0; (pag < TOTAL_PAGES) && (free_page < 0); pag++)
		if (TP_dad[pag].entry == 0)
			free_page = pag;
	if (free_page < 0) {
		// No free dad logic page -> Fork aborted -> free child's pcb
		list_add_tail(first_free, &freequeue);
		return -105; // Dad has no free pages -> ENOBUFS
	}

	for (int pag = 0; pag < NUM_PAG_DATA; pag++){
		// New frames for child's data
		int new_ph_pag = alloc_frame();
		if (new_ph_pag < 0) {
			// No enough free frames -> Fork aborted -> free child's pcb & frame pages
			free_user_pages(&(child->task));
			list_add_tail(first_free, &freequeue);
			return -23; // ENFILE
		}
		set_ss_pag(TP_child, PAG_LOG_INIT_DATA + pag, new_ph_pag);
	}
	
	/* Copy father's data frames data to child's data frames data */
	for (int pag = 0; pag < NUM_PAG_DATA; pag++){
		set_ss_pag(TP_dad, free_page, TP_child[PAG_LOG_INIT_DATA + pag].bits.pbase_addr);
		copy_data((unsigned int*) ((PAG_LOG_INIT_DATA + pag) << 12), (unsigned int*) (free_page << 12), PAGE_SIZE);
	}
	
	del_ss_pag(TP_dad, free_page);
	// Flush dad's TLB (remove free_page)
	set_cr3(current()->dir_pages_baseAddr);
	
	// Assign next PID
	child->task.PID = PID = next_PID;
	next_PID += 2;
	
	/* Update child's task_union */
	// Modify @ret for child, so it will return a 0 (PID) through ret_form_fork
	// -(11(SAVE_ALL)+4(HW CONTEXT)+1(@ret)+1(EBP)+1(NEW_EBP))
	// New fake ebp
	child->stack[KERNEL_STACK_SIZE - 17] = 0;
	// Child's kernel_esp points to fake ebp
	child->task.kernel_esp = (unsigned long*) &(child->stack[KERNEL_STACK_SIZE - 17]);
	// @ret <- &ret_form_fork
	child->stack[KERNEL_STACK_SIZE - 16] = (unsigned long int) &ret_form_fork;
	
	/* Insert child in readyqueue */
	child->task.status = ST_READY;
	list_add_tail(first_free, &readyqueue);

	return PID;
}

int needs_sched_rr();

void sys_exit()
{
	// Get pcb (task_struct)
	struct task_struct *pcb = current();

	// Free data physical pages
	free_user_pages(pcb);

	// Free pcb (task_struct)
	list_add_tail(&(current()->list), &freequeue);

	// Run next process
	sched_next_rr();

	return;
}

int sys_write(int fd, char * buffer, int size)
{
	if (check_fd(fd, ESCRIPTURA) != 0) return check_fd(fd, ESCRIPTURA);
	if (buffer == NULL) return -14; // EFAULT
	if (size < 0) return -22; // EINVAL

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
