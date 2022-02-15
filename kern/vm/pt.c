#include "pt.h"

pt_entry* pt_create(vaddr_t vaddr){

	pt_entry* pte = kmalloc(sizeof(pt_entry));
	pte->page = vaddr;
	pte->frame = 0;
	pte->in_mem = 0;	
	pte->in_swap = 0;
	pte->next=NULL;
	return pte;

}
void pt_free(pt_entry* pt){
	pt_entry* p;
	
	while(pt!=NULL){
		p = pt;
		pt = (pt_entry*)p->next;
		kfree(p);
	}
}
void pt_update(pt_entry* pt, vaddr_t vaddr){
	pt_entry* tmp = pt;
	
	while(tmp != NULL){
		if(tmp->page == vaddr){
			tmp->in_mem = 0;
			tmp->in_swap = 1;
			tmp->frame = 0;
			break;
		}
		tmp = (pt_entry*) tmp->next;
	}
}
void pt_print_state(pt_entry* pte){
	pt_entry* pt = pte;	
	kprintf("Printing PageTable\n");
	while(pt!= NULL){
		kprintf("vaddr 0x%x - paddr 0x%x - inmem %d inswap %d \n",pt->page,pt->frame,pt->in_mem,pt->in_swap);
		pt = (pt_entry*) pt->next;
	}
	kprintf("\n");

}
