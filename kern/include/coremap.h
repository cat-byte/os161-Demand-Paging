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

#ifndef _COREMAP_H_
#define _COREMAP_H_

/*
 * Coremap structure and operations.
 */

#include <types.h>
#include <../arch/mips/include/vm.h>
#include <addrspace.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>

#define SWAP_TEST 0
#define NUM_FREEFRAMES_TEST 17 // palin 17


/*
 * Coremap - data structure 
 *
 *	     frame_state:  spiegare gli stati dei frame
 *	     cm_entry:  will keep info about a frame in ram.
 *
 */

typedef enum{
	FREE, 		// frame libero
	FIXED,		// frame di kernel, non può essere selezionato come vittima
	LOADING,	// frame allocato, in fase di caricamento da file elf o swapfile. Non può essere selezionato come vittima.
	CLEAN		// frame allocato, può essere selezionato come vittima
}frame_state;

typedef struct{
	struct addrspace* as;
        vaddr_t virt_addr;
        frame_state state;
        int npages;
        unsigned int timestamp;
}cm_entry;

/*
 * Functions in coremap.c:
 *
 *    cm_bootstrap	 - Bootstrap della coremap. La coremap viene allocata e i frame disponibili vengono contrassegnati come FREE.
 *    cm_bootstrap_4test - Bootstrap di test della coremap. Serve a testare l'uso dello swapfile in caso di memoria fisica piena. Il numero di 
 *			   frame minimo per permettere a palin di funzionare è 17. 
 *    cm_print		 - Stampa le prime 50 entry della coremap.
 *    is_bootstrapped	 - Per sapere se la coremap è stata inizializzata.
 *    frame_kalloc	 - Allocazione di frame consecutivi per il kernel. I frame allocati hanno stato FIXED. Chiamata da alloc_kpages.
 *    frame_alloc	 - Allocazione di un frame per un processo user. I frame allocati possono avere stato LOADING o CLEAN. Per paginazione on
 *			  demand viene chiamata da vm_fault.
 *    frame_kfree	 - Deallocazione di frame di kernel. Chimata da free_kpages.
 *    cm_asfree 	 - Cancellazione dalla coremap di tutti i frame relativi a un address space. Chiamata da as_destroy.
 *    cm_evict		 - Ricerca una vittima tra i frame allocati al processo. Usa politica FIFO.
 *    cm_update_vaddr	 - Da usare in seguito a cm_evict. Aggiorna il vaddr associato alla vittima trovata.
 *    cm_check_state	 - Controlla stato di un frame.
 *    cm_update_state	 - Aggiorna lo stato di un frame.
 *    cm_shutdown	 - Dealloca la coremap. Chiamata da vm_shutdown.
 */

void cm_bootstrap(void);
void cm_bootstrap_4test(void); // per testare lo swapping
void cm_print(const char* msg);
int is_bootstrapped(void);
paddr_t frame_kalloc(unsigned int nframes);
paddr_t frame_alloc(vaddr_t vaddr, struct addrspace* as);
int frame_kfree(vaddr_t vaddr);
void cm_asfree( struct addrspace* as);
vaddr_t cm_evict(struct addrspace* as, paddr_t* paddr, int* pos);
int cm_check_state(paddr_t paddr, frame_state state);
void cm_update_vaddr(struct addrspace* as, int pos, vaddr_t vaddr);
void cm_update_state(paddr_t paddr, frame_state state);
void cm_shutdown(void);
#endif /* _COREMAP_H_ */
