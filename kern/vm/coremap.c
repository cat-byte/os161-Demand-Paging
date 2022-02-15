#include "coremap.h"
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
/* 		
* 	Coremap - Data structures
*	
*/

static struct spinlock cm_lock = SPINLOCK_INITIALIZER;

static unsigned int ram_frames;
static unsigned int timestamp;

static paddr_t firstpaddr;
static paddr_t lastpaddr;
int bootstrapped = 0 ;
cm_entry* coremap;

/* 		
* 	cm_bootstrap
*/

void cm_bootstrap(void){
	
	spinlock_acquire(&cm_lock);
	lastpaddr = ram_getsize();
	firstpaddr = ram_getfirstfree_(); 
	ram_frames = (lastpaddr - firstpaddr)/PAGE_SIZE;
	timestamp=0;
	spinlock_release(&cm_lock);
	
	coremap = kmalloc(ram_frames*sizeof(cm_entry));
	
	spinlock_acquire(&cm_lock);
	unsigned int space = ram_frames*sizeof(cm_entry);
	unsigned int i=0;
	for( i =0; i < ram_frames ; i++){
	
		coremap[i].as = NULL;
		coremap[i].virt_addr = 0;
		coremap[i].npages = 0; 
		coremap[i].timestamp = -1; 
	
		if( i <= space/PAGE_SIZE ){
			coremap[i].state = FIXED; 
			coremap[i].timestamp = timestamp++; 
		}
		else{
			coremap[i].state = FREE;
		}
	}

	bootstrapped = 1;
	spinlock_release(&cm_lock);
}
/* 		
* 	cm_bootstrap_4test
*/
void cm_bootstrap_4test(void){
	
	spinlock_acquire(&cm_lock);
	lastpaddr = ram_getsize();
	firstpaddr = ram_getfirstfree_(); 
	ram_frames = (lastpaddr - firstpaddr)/PAGE_SIZE;
	timestamp=0;
	spinlock_release(&cm_lock);
	
	coremap = kmalloc(ram_frames*sizeof(cm_entry));
	
	spinlock_acquire(&cm_lock);
	unsigned int i=0;
	/*
	* Occupo tutta la coremap tranne gli ultimi 17 frame. Così i programmi saranno forzati a fare swapping.
	*/
	for( i =0; i < ram_frames ; i++){
	
		coremap[i].as = NULL;
		coremap[i].virt_addr = 0;
		coremap[i].npages = 0; 
		coremap[i].timestamp = -1; 
		coremap[i].state = FIXED; 
		coremap[i].timestamp = timestamp++; 

		
	}
	
	for(i=ram_frames-NUM_FREEFRAMES_TEST; i<ram_frames; i++){ 
		coremap[i].state=FREE;
	}
	
	bootstrapped = 1;
	spinlock_release(&cm_lock);
}
/* 		
* 	cm_print
*/
void cm_print(const char* msg){
	unsigned int i;
	spinlock_acquire(&cm_lock);

	kprintf("\ncaller: %s\n",msg);
	for(i=0; i<50; i++){
		kprintf("[s: %d - t: %d]\n ",coremap[i].state,coremap[i].timestamp);
	}
	kprintf("end\n");

	spinlock_release(&cm_lock);
}
/* 		
* 	is_bootstrapped
*/
int is_bootstrapped(void){
	int res;
	
	spinlock_acquire(&cm_lock);
	res = bootstrapped;
	spinlock_release(&cm_lock);
	return res;
}
/* 		
* 	frame_kalloc
*/
paddr_t frame_kalloc(unsigned int nframes){
	
	unsigned int first,i,found=0 ;
	spinlock_acquire(&cm_lock);
	
		for(i=0;i<ram_frames && !found;i++){
			if( coremap[i].state == FREE ){
				if( (i==0) || (coremap[i-1].state != FREE) ){
					first = i;
				}
				if( i - first +1 >= nframes ){ //c'è lo spazio necessario
					found = 1 ; 
				}
			}
		}
	
		if(found){
			for(i=first;i<nframes+first ;i++){
				if(i == first){
					coremap[i].npages = nframes;
				}
				coremap[i].state = FIXED;
				coremap[i].as = NULL;
				coremap[i].virt_addr = PADDR_TO_KVADDR(firstpaddr+(i*PAGE_SIZE)); 
				coremap[i].timestamp = timestamp; 
			}
			timestamp++;
				
			spinlock_release(&cm_lock);
			return firstpaddr+(first*PAGE_SIZE);			
		}
	
	spinlock_release(&cm_lock);
	return 0;
}
/* 		
* 	frame_alloc
*/
paddr_t frame_alloc(vaddr_t vaddr, struct addrspace* as){
	
	unsigned int i;
	spinlock_acquire(&cm_lock);
	
	for(i=0;i<ram_frames ;i++){
			    if( coremap[i].state == FREE ){
					coremap[i].state = LOADING;  // il caricamento è iniziato, quando finirà lo stato del frame verrà aggiornato in CLEAN
					coremap[i].npages = 1;
					coremap[i].as = as;
					coremap[i].virt_addr = vaddr;
					coremap[i].timestamp = timestamp++; 
					spinlock_release(&cm_lock);
					return firstpaddr+(i*PAGE_SIZE);
			    }
			}
	spinlock_release(&cm_lock);
	return 0;		
}
/* 		
* 	frame_kfree 
*/
int frame_kfree(vaddr_t vaddr){

	unsigned int pos,j, npages;
	paddr_t paddr = vaddr - MIPS_KSEG0;

	spinlock_acquire(&cm_lock);
	if(!bootstrapped){
		spinlock_release(&cm_lock);
		return 0;
	}
	
	pos = (paddr - firstpaddr)/PAGE_SIZE;
	npages = coremap[pos].npages;

	for (j=pos; j< pos+npages; j++){
		coremap[j].state = FREE;
		coremap[j].npages = 0;
		coremap[j].virt_addr = 0; 
		coremap[j].timestamp = -1;
	}
	spinlock_release(&cm_lock);
	return 1;
}
/* 		
* 	cm_asfree
*/
void cm_asfree( struct addrspace* as){
	unsigned int i;
	spinlock_acquire(&cm_lock);
	for(i=0;i<ram_frames;i++){
		if((coremap[i].as == as) && ( coremap[i].state!=FIXED )){
			coremap[i].as = NULL;
			coremap[i].state = FREE;
			coremap[i].npages = 0;
			coremap[i].virt_addr = 0; 
			coremap[i].timestamp = -1;	
		}
	}
	spinlock_release(&cm_lock);
}
/* 		
* 	cm_evict
*/
vaddr_t cm_evict(struct addrspace* as, paddr_t* paddr, int* pos){
	unsigned int min, min_pos, i;
	vaddr_t victim;
	
	spinlock_acquire(&cm_lock);
	min = timestamp;
	min_pos = -1;
	
	// trovo il frame che si trova in memoria da più tempo
	for(i=0; i<ram_frames; i++){
		if((coremap[i].as == as) && ( coremap[i].state!=FIXED ) && ( coremap[i].state!=LOADING ) && (coremap[i].timestamp < min)){
			min = coremap[i].timestamp;
			min_pos = i;
		}
	}
	if((int)min_pos<0){
		panic("Cannot find victim!\n");
	}
	victim = coremap[min_pos].virt_addr;
	*paddr = firstpaddr+(min_pos*PAGE_SIZE);
	*pos = min_pos;
	
	coremap[min_pos].as = NULL;
	coremap[min_pos].state = LOADING;
	coremap[min_pos].npages = 0;
	coremap[min_pos].virt_addr = 0; 
	coremap[min_pos].timestamp = -1;
	
	spinlock_release(&cm_lock);

	return victim;
}
/* 		
* 	cm_update_vaddr
*/
void cm_update_vaddr(struct addrspace* as, int pos, vaddr_t vaddr){
	KASSERT(pos >=0 && pos<(int)ram_frames);
	
	spinlock_acquire(&cm_lock);
	coremap[pos].state = LOADING; 
	coremap[pos].as = as;
	coremap[pos].virt_addr = vaddr;
	coremap[pos].timestamp = timestamp++;
	coremap[pos].npages = 1;
	spinlock_release(&cm_lock);
}
/* 		
* 	cm_check_state
*/
int cm_check_state(paddr_t paddr,frame_state state){
	unsigned int pos = (paddr-firstpaddr)/PAGE_SIZE;
	int res;
	spinlock_acquire(&cm_lock);
	res = (coremap[pos].state == state)? 1 : 0;
	spinlock_release(&cm_lock);
	return res;
}
/* 		
* 	cm_update_state
*/
void cm_update_state(paddr_t paddr, frame_state state){
	unsigned int pos = (paddr-firstpaddr)/PAGE_SIZE;
	spinlock_acquire(&cm_lock);
	coremap[pos].state = state;
	spinlock_release(&cm_lock);
}
/* 		
* 	cm_shutdown
*/
void cm_shutdown(void){
	kfree(coremap);
}
