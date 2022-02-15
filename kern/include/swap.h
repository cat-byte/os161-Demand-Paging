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

#ifndef _SWAP_H_
#define _SWAP_H_

#include <types.h>
#include <spinlock.h>
#include <vnode.h>
#include <vfs.h>
#include <addrspace.h>
#include <kern/fcntl.h>
#include <uio.h>

#define SWAP_SIZE 9*1024*1024 / 4096

/*
 * Swapfile structure
 */

struct swap_entry{
	struct addrspace* as;
	vaddr_t vaddr;
};

/*
 * Functions in swap.c:
 * swapspace_bootstrap	- Alloca il vettore swapspace parallelo allo swapfile, apre lo swapfile.
 * swap_in		- Scrive un frame nello swapfile.
 * swap_out		- Legge un frame dallo swapfile.
 * print_swap_state	- Stampa le entry piene del vettore swapspace.
 * swap_asfree		- Elimina dal vettore swapspace tutte le entry relative all'address space. Chiamata in as_destroy.
 * swapspace_shutdown	- Dealloca il vettore swapspace e chiude lo swapfile. Chiamamta da vm_shutdown.
 */
 
void swapspace_bootstrap(void);
void swap_in(struct addrspace* as, vaddr_t vaddr);
void swap_out(struct addrspace* as, vaddr_t vaddr);
void print_swap_state(const char* msg);
void swap_asfree(struct addrspace* as);
void swapspace_shutdown(void);
#endif /* _SWAP_H_ */
