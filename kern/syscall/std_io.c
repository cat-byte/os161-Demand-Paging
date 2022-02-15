
#include <types.h>

#include <syscall.h>
#include <lib.h>
#include <kern/unistd.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>
#include "opt-supportfs.h"

#if OPT_SUPPORTFS

void uio_uinit(struct iovec *iov, struct uio *u,void *ubuf, size_t len, off_t pos, enum uio_rw rw);

int sys_open(const char *pathname, int flags, mode_t mode){
	struct proc* p = curproc;
	struct openfile* pos; 
	struct  vnode* v;
	int fd;
	int result;
	
	/* Open the file. */
        result = vfs_open((char *)pathname, flags, mode, &v);

	
        if (result) {
		kprintf("result negativo\n");
            return -1;
        }

	//aggiungo il file nella system wide open table
	//restituisce fd e mette in pos il puntatore alla cella della swoptable
	addOpenedFile(v,&pos);	

	//aggiorno la tabella del processo corrente
	fd = addOFPerProcess(p,pos);
	if (fd < 3) 
		vfs_close(v);
	return fd;
}

int sys_close(int fd){

	struct proc* proc = curproc;
	//remOFPerProcess(proc,fd);
	vfs_close(proc->openedFiles[fd]->vn);	
	proc->openedFiles[fd]->vn = NULL;	
	proc->openedFiles[fd] = NULL;

	return 0;
}


void uio_uinit(struct iovec *iov, struct uio *u,void *ubuf, size_t len, off_t pos, enum uio_rw rw){
      struct proc* proc = curproc;	

      iov->iov_kbase = ubuf;
      iov->iov_len = len;
      u->uio_iov = iov;
      u->uio_iovcnt = 1;
      u->uio_offset = pos;
      u->uio_resid = len;
      u->uio_segflg = UIO_USERSPACE;
      u->uio_rw = rw;
      u->uio_space = proc->p_addrspace;
}

#endif

ssize_t sys_write(int fd, const void *buf, size_t count){
	
	#if OPT_SUPPORTFS
	 struct proc* proc = curproc;
	 struct vnode* v;
	 struct iovec iov;
         struct uio u;
         int result;
    
	if(fd != STDOUT_FILENO  && fd != STDERR_FILENO ){
	  off_t off =  getOffsetOF(proc,fd);
	  uio_uinit(&iov, &u, (void*)buf, count , off , UIO_WRITE);
	  v = proc->openedFiles[fd]->vn;	
	  result = VOP_WRITE(v, &u);
	
 	if (result) {
 	        return -1;
        }
        if (u.uio_resid > 0) {
           /* short write; problem? */
           kprintf("\nshort write on file\n");
           return -1;
        }

	//se è tutto ok allora aggiorno l'offset
	proc->openedFiles[fd]->offset = u.uio_offset;	 	
	return count;
			
	}
	else{
		int i;
		for(i = 0 ; i <(int)count; i++){
		   kprintf("%c",((char *)buf)[i]);	
		}
		return count;
	}		

	#else
	int i; 
	//scrittura solo su stdin
	if(fd != STDOUT_FILENO  && fd != STDERR_FILENO ){
	   kprintf("Print on output is not supperted in STDIOSYSCALLS\n");	
	  return -1;	
	}
	
	for(i = 0 ; i <(int)count; i++){
	   kprintf("%c",((char *)buf)[i]);	
	}
	return count;	
	#endif
}

ssize_t sys_read(int fd, void *buf, size_t count){

	#if OPT_SUPPORTFS	
 	 struct proc* proc = curproc;
	 struct vnode* v;
	 struct iovec iov;
         struct uio u;
         int result;

	if(fd != STDIN_FILENO  && fd != STDERR_FILENO ){
	  
	  uio_uinit(&iov, &u, buf, count , proc->openedFiles[fd]->offset , UIO_READ);
	  v = proc->openedFiles[fd]->vn;	 
	 result = VOP_READ(v, &u);

 	if (result) {
 	        return -1;
        }
   
        if (u.uio_resid > 0) {
           /* short read; problem? */
           kprintf("\nshort read on file\n");
           return -1;
        }

	//se è tutto ok allora aggiorno l'offset
	(proc->openedFiles[fd])->offset = u.uio_offset;	 	
	return count;
	}
	else{
		char tmp[2];	
		kgets(tmp,1);
		*(char*) buf = tmp[0];	
	
		return count;
	}		

	#else
	if(fd != STDIN_FILENO  && fd != STDERR_FILENO  ){
	   kprintf("Input is supperted only for STDIN in STDIOSYSCALLS\n");	
	  return -1;	
	}
	//useless avoids warning
	count= count+1;

	char tmp[2];	
	kgets(tmp,1);
	*(char*) buf = tmp[0];	
	
	return 1;
	#endif
}



