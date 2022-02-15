/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include "coremap.h"
#include "swap.h"
#include "tlb.h"
#include "vm_stats.h"
#include "opt-final.h"

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 *
 * NOTE: it's been found over the years that students often begin on
 * the VM assignment by copying dumbvm.c and trying to improve it.
 * This is not recommended. dumbvm is (more or less intentionally) not
 * a good design reference. The first recommendation would be: do not
 * look at dumbvm at all. The second recommendation would be: if you
 * do, be sure to review it from the perspective of comparing it to
 * what a VM system is supposed to do, and understanding what corners
 * it's cutting (there are many) and why, and more importantly, how.
 */

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define DUMBVM_STACKPAGES    18


/*
 * Wrap ram_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

void
vm_bootstrap(void)
{
	if(SWAP_TEST){
		cm_bootstrap_4test();
	}
	else{
		cm_bootstrap();
	}

}

/*
 * Check if we're in a context that can sleep. While most of the
 * operations in dumbvm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * dumbvm starts blowing up during the VM assignment.
 */

void
dumbvm_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}


paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;
	
	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);

	spinlock_release(&stealmem_lock);
	
	return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;
	if(is_bootstrapped()){
		pa = frame_kalloc(npages);
	}
	else{
		dumbvm_can_sleep();
		pa = getppages(npages);
	}
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)
{
	int res;
	if(is_bootstrapped()){
		res = frame_kfree(addr);
		KASSERT(res!=0);
	}
	else{
		/*
		*	leak memory
		*/
		(void)addr;
		(void)res;
	}
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

/*
*	compute_memsz
*/
static size_t compute_memsz(segment_entry *segment, int i){
	size_t memsz, resid;
	
	if(segment->size < PAGE_SIZE){ 				// la dimensione totale del segmento è inferiore a 4KB.
		memsz = segment->size;
	}
	else if(i==1){						// prima pagina del segmento. E' possibile che il primo indirizzo del segmento non si allineato ad un multiplo di pagina.
		memsz = PAGE_SIZE - (segment->offset&~PAGE_FRAME);
	}
	else if(i == segment->npages){ 				// ultima pagina del segmento. Calcolo frammentazione finale e sottraggo eventuale offset iniziale.
		resid = (segment->npages*PAGE_SIZE) - (segment->size);
		memsz = PAGE_SIZE - (resid - (segment->offset&~PAGE_FRAME));
	}
	else{
		memsz = PAGE_SIZE;
	}
	return memsz;
}
/*
*	compute_offset
*/
static off_t compute_offset(off_t offset,int i){

	off_t ret;
	ret = offset + (i-1)*PAGE_SIZE;
	if (i!=1){
		ret&=PAGE_FRAME;		// l'offset può non essere un multiplo di pagina. Per la prima pagina l'offset è quello letto dall'elf,
						// per le pagine successive occorre riportarlo ad un multiplo di pagina,
	}
	return ret;
}
/*
*	handle_victim_and_swapout
*/
static
int handle_victim_and_swapout(struct addrspace* as,paddr_t* paddr,vaddr_t faultaddress){
		
	int pos;
	vaddr_t victim;
	
	vmstats_inc(SWAP_FILE_WRITE);

	victim = cm_evict(as, paddr, &pos); 	// seleziona la vittima scorrendo la coremap, politica FIFO. Restituisce vaddr, paddr e posizione nella coremap.
					   	// coremap[pos] dovrà essere riempita con il nuovo vaddr.
	
	swap_out(as, victim);			// scrittura del frame nello swapfile
	pt_update(as->pt, victim); 		// scorre tutte le entry della pt per cercare la vittima e segnare che non è piu in memoria ( in_swap = 1 )
	cm_update_vaddr(as, pos, faultaddress); // Aggiorna coremap[pos] con il nuovo vaddr

	if(*paddr == 0)
		return EFAULT;
	tlbR(victim,faultaddress,*paddr);
	
	return 0;
}
/*
*	vm_fault
*/
int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	paddr_t paddr=0;
	int i;
	uint32_t ehi, elo;
	
	struct addrspace *as;

	int spl;

	faultaddress &= PAGE_FRAME;		// faultaddress viene riportato ad un multiplo di pagina
	

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		panic("dumbvm: got VM_FAULT_READONLY 0x%x\n", faultaddress);
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		return EFAULT;
	}
	
#if !OPT_PT
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		return EFAULT;
	}
	
	
#elif !OPT_ONDEMAND /* PAGINAZIONE - il file elf è gia stato interamente caricato in memoria */
	segment_entry* seg = as->segments;
	while(seg != NULL){
		vaddr_t end = seg->first_addr+(seg->npages*PAGE_SIZE);
		if(faultaddress >= seg->first_addr && faultaddress < end){
			break;
		}
		seg = (segment_entry*)seg->next;
	}
	
	if(seg == NULL){
		return EFAULT;
	}
	
	i=1; // per contare le pagine
	pt_entry* pte = seg->first_pt_entry;
	while(pte != NULL){
		if(faultaddress == pte->page){
			paddr = pte->frame;
			break;
		}
		pte = (pt_entry*)pte->next;
		i++;
	}		
	
#else	/* PAGINAZIONE ON DEMAND - il file elf non è stato caricato in memoria, i frame vengono caricati solo quando ce n'è bisogno */

	vmstats_inc(TLB_FAULT);
	
	size_t memsz, filesz;
	off_t offset;
	int result;
	
	// cerco il segmento corrispondente
	segment_entry* seg = as->segments;
	while(seg != NULL){
		vaddr_t end = seg->first_addr+(seg->npages*PAGE_SIZE);
		if(faultaddress >= seg->first_addr && faultaddress < end){
			break;
		}
		seg = (segment_entry*)seg->next;
	}
	
	if(seg == NULL){
		return EFAULT;
	}
	
	//cerco tra le pagine del segmento corrispondente
	i=1; // per contare le pagine
	pt_entry* pte = seg->first_pt_entry;
	while(i <= seg->npages){ 	
		if(faultaddress == pte->page){
			if(pte->in_mem){				// mapping page-frame già presente in page table
				paddr = pte->frame;
				
				vmstats_inc(TLB_RELOAD);
					
				if(cm_check_state(paddr,LOADING)){	// il frame è in fase di caricamento: serve sempre il permesso di scrittura	
					tlbW(faultaddress, paddr, 2);				
				}
				else{					// il frame è già stato caricato: servono i permessi corretti
					tlbW(faultaddress, paddr, seg->permission->write); 
				}
				return 0;
			}
			else if(pte->in_swap){ 				// frame nello swapfile -> swap_in
				paddr = frame_alloc(faultaddress, as);
				if (paddr == 0){			// occorre cercare una vittima tra i frame già allocati e farne swap_out
					result = handle_victim_and_swapout(as,&paddr,faultaddress);
					if ( result == EFAULT )
						return EFAULT;
				}
				pte->frame = paddr;
				pte->in_mem = 1;
				pte->in_swap = 0; 
				
				swap_in(as, faultaddress);
				
				vmstats_inc(PAGE_FAULT_SWAP);
				vmstats_inc(PAGE_FAULT_DISK);
				
				cm_update_state(paddr, CLEAN);
				tlbW(faultaddress, paddr, seg->permission->write); 
				return 0;
			}
			else{ 						// la pagina non è stata ancora caricata in memoria. Alloco un frame e leggo dall'elf.
				paddr = frame_alloc(faultaddress, as);
				if (paddr == 0){			// occorre cercare una vittima tra i frame già allocati e farne swap_out.
					result = handle_victim_and_swapout(as,&paddr,faultaddress);
					if( result )
						return EFAULT;
				}
				pte->frame = paddr;
				pte->in_mem = 1;
				pte->in_swap = 0; 

				bzero((void *)faultaddress, PAGE_SIZE);
				
				if(seg->offset < 0){ 			// segmento di stack. Non c'è da fare nessun caricamento
					tlbW(faultaddress, paddr, seg->permission->write);
					vmstats_inc(PAGE_FAULT_ZERO);	// contatore dei frame azzerati e non caricati da disco
					return 0;
				}
				
				/*
				* Calcolo la dimensione del blocco da leggere e l'offset.
				* Occorre tenere conto dell'eventuale frammentazione iniziale e finale.
				*/
				memsz = compute_memsz(seg, i);
				filesz = memsz;				
				offset = compute_offset(seg->offset, i);
				
				if( i==1){ 				// per la prima pagina è da considerare un eventuale offset iniziale
					faultaddress = faultaddress + (seg->offset&~PAGE_FRAME);
				}

				load_page_from_elf(as, faultaddress, offset, memsz, filesz, seg->permission->exec);

				vmstats_inc(PAGE_FAULT_ELF);
				vmstats_inc(PAGE_FAULT_DISK);
				
				cm_update_state(paddr, CLEAN);
				
				tlbW(faultaddress&PAGE_FRAME, paddr, seg->permission->write); // &PAGE_FRAME per la prima pagina
				return 0;
			}
		}
		pte = (pt_entry*)pte->next;
		i++;
	}
		
	if(i>seg->npages){
		return EFAULT;
	}
#endif		
	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	
	
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		kprintf("TLB :: vaddr 0x%x - paddr 0x%x \n",ehi,elo);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
	
}

void vm_shutdown(void){
#if OPT_FINAL
	vmstats_print();
	swapspace_shutdown();
	cm_shutdown();
#endif
}
