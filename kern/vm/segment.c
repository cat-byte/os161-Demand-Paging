#include "segment.h"

segment_entry* sgm_create(vaddr_t vaddr,off_t offset, int sz,size_t segsz,int r,int w ,int x, segment_entry* next){
	
	segment_entry* sgm = kmalloc(sizeof(segment_entry));

	sgm->first_addr=vaddr;
	sgm->npages=sz;
	sgm->size = segsz;
	sgm->offset=offset;	
	sgm->permission = kmalloc(sizeof(permissions));
	sgm->permission->read = r;
	sgm->permission->write = w;
	sgm->permission->exec = x;
	sgm->first_pt_entry=NULL;	
	sgm->next= (struct segment_entry*)next;
	return sgm;

}

void sgm_free(segment_entry* sge){
	segment_entry* s;
	
	while(sge!=NULL){
		s = sge;
		kfree(sge->permission);
		sge = (segment_entry*)s->next;
		kfree(s);
	}
}

