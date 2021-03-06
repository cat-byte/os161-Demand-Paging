#include "swap.h"

static struct spinlock sw_lock = SPINLOCK_INITIALIZER;
static struct swap_entry* swapspace;
static struct vnode* swapfile;
static const char swapfilename[] = "emu0:swapfile";
/* 		
* 	swapspace_bootstrap 
*/
void swapspace_bootstrap(void){
	
	int i,result;
	char path[sizeof(swapfilename)];
	if(SWAP_SIZE > 9*1024*1024){
		panic("\nswapfile can't be larger than 9MB\n");
	}
	
	swapspace = kmalloc(SWAP_SIZE * sizeof(struct swap_entry));
	
	spinlock_acquire(&sw_lock);
	for (i=0; i<SWAP_SIZE; i++){
		swapspace[i].as = NULL;
		swapspace[i].vaddr = 0;
	}
	spinlock_release(&sw_lock);
	strcpy(path, swapfilename);
	
	result = vfs_open(path, O_RDWR | O_CREAT , 0, &swapfile);
	if( result){
		panic("\nError opening swapfile!\n");
	}

}
/* 		
* 	swap_in 
*/
void swap_in(struct addrspace* as, vaddr_t vaddr){
	int i;
	struct iovec iov;
	struct uio u;
	int result;

	spinlock_acquire(&sw_lock);
	for(i=0; i<SWAP_SIZE; i++){
		if((swapspace[i].as == as) && (swapspace[i].vaddr == vaddr)){
			swapspace[i].as = NULL;
			swapspace[i].vaddr = 0;
		 	break;
		}
	}
	spinlock_release(&sw_lock);
	if(i == SWAP_SIZE){
		panic("Swapfile - vaddr not found!\n"); 
	}
	iov.iov_ubase = (userptr_t)vaddr;
	iov.iov_len = PAGE_SIZE;	  // length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = PAGE_SIZE;          // amount to read from the file
	u.uio_offset = i*PAGE_SIZE;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = as;

	result = VOP_READ(swapfile, &u);
	if (result) {
		panic("Swapfile - swap in error.\n");
	}
	print_swap_state("swap_in\n");
	
}
/* 		
* 	swap_out
*/
void swap_out(struct addrspace* as, vaddr_t vaddr){
	int i, result;
	struct iovec iov;
	struct uio u;
	
	spinlock_acquire(&sw_lock);
	
	for(i=0; i<SWAP_SIZE; i++){
		if(swapspace[i].as == NULL){
			break;
		}
	}
	if(i == SWAP_SIZE){
		panic("Swapfile - swapfile is full!\n"); 
	}
	
	swapspace[i].as = as;
	swapspace[i].vaddr = vaddr;
	
	spinlock_release(&sw_lock);
	
	iov.iov_ubase = (userptr_t)vaddr;
	iov.iov_len = PAGE_SIZE;	  // length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = PAGE_SIZE;          // amount to write to the file
	u.uio_offset = i*PAGE_SIZE;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = as;
	
	result = VOP_WRITE(swapfile, &u);
	if (result) {
		panic("Swapfile - swap out error.\n");
	}
	print_swap_state("swap_out\n");
}
/* 		
* 	print_swap_state
*/
void print_swap_state(const char* msg){
	int i;
	kprintf("%s",msg);
	for(i=0;i<SWAP_SIZE; i++){
		if(swapspace[i].as!=NULL){
			kprintf("%d - %#010x\n",i,swapspace[i].vaddr);
		}
	}
	kprintf("\n");
}
/* 		
* 	swap_asfree
*/
void swap_asfree(struct addrspace* as){
	int i;
	spinlock_acquire(&sw_lock);
	for(i=0; i<SWAP_SIZE; i++){
		if(swapspace[i].as == as){
			swapspace[i].as = NULL;
			swapspace[i].vaddr = 0;
		}
	}
	spinlock_release(&sw_lock);
}
/* 		
* 	swapspace_shutdown 
*/
void swapspace_shutdown(void){
	kfree(swapspace);
	vfs_close(swapfile);
}
