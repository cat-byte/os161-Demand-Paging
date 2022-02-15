#include <types.h>
#include <lib.h>
#include <synch.h>
#include <spl.h>
#include "vm_stats.h"


/* Array contatori per le statistiche */
static unsigned int stat_counters[ TOT_COUNTERS]; 

struct spinlock stats_lock = SPINLOCK_INITIALIZER;

/* Stringhe usate per stampare le statistiche */
static const char *stat_labels[] = {
 /*  0 */ "TLB Faults", 
 /*  1 */ "TLB Faults with Free",
 /*  2 */ "TLB Faults with Replace",
 /*  3 */ "TLB Invalidations",
 /*  4 */ "TLB Reloads",
 /*  5 */ "Page Faults (Zeroed)",
 /*  6 */ "Page Faults (Disk)",
 /*  7 */ "Page Faults from ELF",
 /*  8 */ "Page Faults from Swapfile",
 /*  9 */ "Swapfile Writes",
};

/* Azzeramento iniziale array */
void
vmstats_init(void)
{
	int i = 0;
	spinlock_acquire(& stats_lock);
	for (i=0; i< TOT_COUNTERS; i++) {
		stat_counters[i] = 0;
	}
	spinlock_release(&stats_lock);	
}


/* Funzione per incrementare un contatore specifico
 * usando come indice le define da vmstats.h
 * esempio: vmstats_inc( TLB_FAULT);
 */

void
vmstats_inc(unsigned int pos)
{
    spinlock_acquire(& stats_lock);
	/* verifico che la posizione fornita sia lecita prima di incrementare */
    KASSERT(pos <  TOT_COUNTERS);
	stat_counters[pos]++;
    spinlock_release(& stats_lock);
}

/* Funzione per stampa statistiche */
void vmstats_print(void){

	int tlb_fault = 0;
	int sum_tlbfree_tlbreplace = 0;
	int sum_tlbreload_disk_zeroed = 0;
	int sum_pfelf_pfswap = 0;
	int pf_disk = 0;
	int i = 0;
/* Calcolo contatori per verifiche */
	tlb_fault = stat_counters[ TLB_FAULT];
	sum_tlbfree_tlbreplace = stat_counters[ TLB_FAULT_FREE] + stat_counters[ TLB_FAULT_REPLACE];
	sum_tlbreload_disk_zeroed = stat_counters[ PAGE_FAULT_DISK] + stat_counters[ PAGE_FAULT_ZERO] + stat_counters[ TLB_RELOAD];
	sum_pfelf_pfswap = stat_counters[ PAGE_FAULT_ELF] + stat_counters[ PAGE_FAULT_SWAP];
	pf_disk = stat_counters[ PAGE_FAULT_DISK];

/* Stampa statistiche */
	kprintf("\nVirtual memory statistics:\n");
	for (i=0; i< TOT_COUNTERS; i++) {
		kprintf("VM_STATS %30s = %10d\n", stat_labels[i], stat_counters[i]);
	}
	/* Controllo TLB Fault 1 */
	kprintf("\nVirtual memory checks:\n");
	kprintf("VM_STATS TLB Faults with Free + TLB Faults with Replace = %d\n", sum_tlbfree_tlbreplace);
	if (sum_tlbfree_tlbreplace != tlb_fault) {
		kprintf("Warning: TLB Faults with Free + TLB Faults with Replace (%d) != TLB Faults (%d)\n\n",
		sum_tlbfree_tlbreplace, tlb_fault);
	}
	else {
		kprintf("OK! TLB Faults with Free + TLB Faults with Replace (%d) = TLB Faults (%d)\n\n",
		sum_tlbfree_tlbreplace, tlb_fault);
	}
	/* Controllo TLB Fault 2 */
	kprintf("VM_STATS TLB Reloads + Page Faults (Disk) + Page Faults (Zeroed) = %d\n", sum_tlbreload_disk_zeroed);
	if (sum_tlbreload_disk_zeroed != tlb_fault) {
		kprintf("Warning: TLB Reloads + Page Faults (Disk) + Page Faults (Zeroed) (%d) != TLB Faults (%d)\n\n",
		sum_tlbreload_disk_zeroed, tlb_fault);
	}
	else {
		kprintf("OK! TLB Reloads + Page Faults (Disk) + Page Faults (Zeroed)(%d) = TLB Faults (%d)\n\n",
		sum_tlbreload_disk_zeroed, tlb_fault);
	}
	/* Controllo Page Fault */
	kprintf("VM_STATS Page Faults from ELF + Page Faults from Swapfile = %d\n", sum_pfelf_pfswap);
	if (sum_pfelf_pfswap != pf_disk) {
		kprintf("Warning: Page Faults from ELF + Page Faults from Swapfile (%d) != Page Faults (Disk) (%d)\n\n",
		sum_pfelf_pfswap, pf_disk);
	}
	else {
		kprintf("OK! Page Faults from ELF + Page Faults from Swapfile (%d) = Page Faults (Disk) (%d)\n\n",
		sum_pfelf_pfswap, pf_disk);
	}
}

