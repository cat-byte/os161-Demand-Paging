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

#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <types.h>
#include "pt.h"

/*
 * Segment structure and operations.
 */

typedef struct{
	int read;
	int write;
	int exec;
}permissions;

typedef struct{
	vaddr_t first_addr; 		// primo indirizzo virtuale del segmento (allineato alla pagina)
	int npages;			// numero di pagine del segmento
	size_t size;			// dimensione esatta del segmento, letta dall'elf
	off_t offset;			// offset del segmento nell'elf. Va usato per accesso diretto al segmento nel file elf e per capire se il segmento è allineato alle pagine
	permissions* permission;	// permessi del segmento
	pt_entry* first_pt_entry;	// punta alla prima pagina di questo segmento. Serve a ottimizzare la ricerca nella page table
	struct segment_entry* next;
}segment_entry;

/*
 * Functions in segment.c:
 *
 *    sgm_create	- Alloca un segmento.
 *    sgm_free		- Dealloca un segmento.
 */
segment_entry* sgm_create(vaddr_t vaddr,off_t offset, int sz,size_t segsz,int r,int w ,int x,segment_entry* next);	
void sgm_free(segment_entry* sge);
#endif /* _SEGMENT_H_ */
