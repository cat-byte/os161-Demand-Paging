#include <types.h>
#include <syscall.h>
#include <lib.h>
#include <kern/unistd.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>

void sys__exit(int status){

	(void) status;
	struct proc* procc = curproc;
	// si usa procc perchè curproc è una macro che recupera tproc dal thread ma dato che procremthread la mette a null, se lo usiamo accediamo ad un campo nullo
	//saving the status into the proc structure
	curproc->exit_status = (size_t)status; 	
	
	//as_destroy(curproc->p_addrspace);
	as_destroy(procc->p_addrspace);	

	//thread_exit will set the status of the thread as zombie
	thread_exit();
	
	/*NOTE : exorcise() called in thread_destroy() will effectively
	 deallocate thread stack and all its  data from the memory*/
	
	return ;	
}

