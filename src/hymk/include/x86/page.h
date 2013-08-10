/*
 *
 * page.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * x86 Paging operations
 *
 */
#ifndef _PAGE_H
#define _PAGE_H

#include <hydrixos/types.h>
#include <mem.h>

/*
 * kmem_get_table(pdir, adr, doalloc)
 *
 * Returns the pointer to the page table, that is containing
 * the page descriptor of the page 'adr' of the virtual
 * address space described by the page directory 'pdir'.
 *
 * If 'doalloc' is set to TRUE, the function will allocate
 * automaticaly a new page table if the PFLAG_PRESENT flag of the page
 * directory entry of the page table isn't set.
 *
 * Return value:
 *  != NULL	The address of the table
 *  NULL	
 *		doalloc = FALSE		Table entry emtpy
 *		doalloc = TRUE		Not enough free memory to 
 *					allocate the table
 *
 */
static inline uint32_t* kmem_get_table(uint32_t *pdir, 
				       uintptr_t adr,
				       bool doalloc
				      )
{
	unsigned long l__n = (adr / (1024 * 4096));
	if (!(pdir[l__n] & GENFLAG_PRESENT))
	{
		uintptr_t l__tab;
		if (doalloc == FALSE) return NULL;
	
		l__tab = (uintptr_t)kmem_alloc_kernel_pageframe();
		if (l__tab == 0) return NULL;
				
		pdir[l__n] =   l__tab
			     | PFLAG_PRESENT
			     | PFLAG_READWRITE
			     | PFLAG_USER
			    ;
		
		/* Invalidate the memory area of the page table */
		INV_TLB_COMPLETE();	
	}
		
	return (void*)(uintptr_t)(pdir[l__n] & (~0xFFFU));
}

/*
 * KMEM_TABLE_ENTRY(tab, adr)
 *
 * Access to a table entry of the table 'tab' which contains the
 * page descriptor of the page at virtual address 'adr'.
 *
 */
#define KMEM_TABLE_ENTRY(___tab, ___adr) (___tab[(___adr / 4096) & (0x3FF)])

/*
 * KMEM_SET_FLAGS(tab, adr, flags)
 *
 * Sets the flags 'flags' on the table entry of the table 'tab' which 
 * contains the page descriptor of the page at virtual address 'adr'.
 *
 */
#define KMEM_SET_FLAGS(___tab, ___adr, ___flags) \
(\
	KMEM_TABLE_ENTRY(___tab, ___adr) |= (___flags & 0xFFF);\
)

/*
 * KMEM_DEL_FLAGS(tab, adr, flags)
 *
 * Deletes the flags 'flags' from the table entry of the table 
 * 'tab' which contains the page descriptor of the page at virtual 
 * address 'adr'.
 *
 */
#define KMEM_DEL_FLAGS(___tab, ___adr, ___flags) \
(\
	KMEM_TABLE_ENTRY(___tab, ___adr) &= ~(___flags & 0xFFF);\
)

/*
 * KMEM_SET_ADDRESS(tab, adr, padr)
 *
 * Sets the page frame address 'padr' to the table entry of the table 
 * 'tab' which contains the page descriptor of the page at virtual 
 * address 'adr'.
 *
 */
#define KMEM_SET_ADDRESS(___tab, ___adr, ___padr) \
(\
	KMEM_TABLE_ENTRY(___tab, ___adr) &= 0xFFF;\
	KMEM_TABLE_ENTRY(___tab, ___adr) |= (padr & (~0xFFF));\
)

/*
 * Page mapping functions
 *
 */
long kmem_map_page_frame(uint32_t *pdir, 
	 		 uint32_t *pstat,
		  	 uintptr_t p_adr, 
		  	 uintptr_t v_adr, 
			 unsigned long pages,
		  	 uint32_t flags
		        );
long kmem_map_page_frame_cur(uintptr_t p_adr, 
		  	     uintptr_t v_adr, 
			     unsigned long pages,
			     uint32_t flags
			    );
		       
#endif
