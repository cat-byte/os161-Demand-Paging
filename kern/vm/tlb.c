#include "tlb.h"
/*
*	tlb_print
*/
void tlb_print(void){
	int i;
	uint32_t ehi, elo;
	kprintf("TLB printing\n");
	for(i =0;i< NUM_TLB;i++){
		tlb_read(&ehi, &elo, i);
		kprintf("TLB :: vaddr 0x%x - paddr 0x%x \n",ehi,elo);	
	}
	kprintf("fine\n");
	
}

/*
*	tlb_get_rr_victim
*/
static int tlb_get_rr_victim(void){
	int victim;
	static unsigned int next_victim = 0;
	victim = next_victim;
	next_victim = (next_victim+1) % NUM_TLB;
	return victim;
}
/*
*	tlbR - Replace
*/
void tlbR(vaddr_t victim,vaddr_t faultaddress, paddr_t paddr){
	
	int spl;
	int x;	
	uint32_t ehi, elo;
	spl = splhigh();
	
	ehi = victim;
	x = tlb_probe( ehi, 0);
	if (x >= 0){
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;  // perche la pagina che deve essere riempita dev essere scrivibile
		tlb_write(ehi, elo, x);
		vmstats_inc(TLB_FAULT_FREE);
	}
	splx(spl);
}
/*
*	tlbW - Write
*/
void tlbW(vaddr_t faultaddress, paddr_t paddr, int write){ // write != 0 -> dirty bit=1
	int spl;
	int i;
	uint32_t ehi, elo;
	spl = splhigh();
	//tlb_print();
	ehi = faultaddress;
	elo = paddr | TLBLO_VALID;
	if(write>0){
		elo = elo | TLBLO_DIRTY ;
	}
	
	i = tlb_probe( ehi, 0);
	if (i >= 0){
		//qui non viene aggiornto nessun contatore perchè siamo in un TLB HIT 
		tlb_write(ehi, elo, i);
		splx(spl);
		vmstats_inc(TLB_FAULT_FREE);
		return;
	}
	
	//cerca il primo posto valido altrimenti rimpiazzo con politica RR 
	for (i=0; i<NUM_TLB; i++) {	
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		ehi = faultaddress;
		elo = paddr | TLBLO_VALID;
		if(write>0){
			elo = elo | TLBLO_DIRTY ;
		}
		//kprintf("TLB :: vaddr 0x%x - paddr 0x%x \n",ehi,elo);
		tlb_write(ehi, elo, i);
		splx(spl);
		vmstats_inc(TLB_FAULT_FREE);
		return;
	}

	// TLB piena, uso politica di rimpiazzamento RR
	// kprintf("TLB piena!\n");
	i = tlb_get_rr_victim();
	ehi = faultaddress;
	elo = paddr | TLBLO_VALID;
	if(write>0){
		elo = elo | TLBLO_DIRTY ;
	}
	tlb_write(ehi, elo, i);
	splx(spl);
	vmstats_inc(TLB_FAULT_REPLACE);
}

