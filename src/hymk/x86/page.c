/*
 *
 * page.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Paging functions
 *
 */
#include <hydrixos/types.h>
#include <setup.h>
#include <mem.h>
#include <stdio.h>
#include <page.h>
#include <current.h>

uint32_t *i386_current_pdir = 0;

/*
 * kmem_get_pagetable_entry(pdir, adr)
 *
 * Returns the pointer to the page table entry of the
 * page 'adr'. The page table is part of the virtual
 * address space of the page directory 'pdir'.
 *
 * NULL:	Page table not existing
 *
 */
static inline uint32_t* kmem_get_pagetable_entry(uint32_t *pdir, uintptr_t adr)
{
	uint32_t l__tab = pdir[adr / 1024 * 4096];	
	uint32_t l__offs = (adr / 4096) & 0x3FF;
	
	if (!(l__tab & PFLAG_PRESENT)) return NULL;

	l__tab &= (~0xFFF);
	
	return (uint32_t*)(l__tab + l__offs);
}

/*
 * kmem_map_page_frame(pdir, pstat, p_adr, v_adr, pages, flags)
 *
 * Maps an area of 'pages' page frames at address 'p_adr' 
 * into a virtual address space defined by the page directory 
 * 'pdir' to the virtual address 'v_adr'. The function will
 * update the table usage statistic table 'pstat'.
 *
 * You can define the access flags of this pages through
 * the 'flags' parameter.
 *
 * If a page should be mapped to an area of the virtual
 * address space that currently isn't described by a 
 * page table, the function will automatically allocate 
 * and map a new page table.
 *
 * The function will neither free page frames, that will
 * be removed from the address space nor change the
 * usage counter of the selected page frames.
 *
 * ! If you want to map a page into the current virtual
 * ! address space YOU HAVE TO use kmem_map_page_frame_cur
 * ! to prevent the use of an invalid TLB content.
 *
 * Using the flag GENFLAG_DONT_OVERWRITE_SETTINGS will prevent
 * that the function overwrites existing areas.
 * 
 * Return Value:
 *	== 0	Successful
 *	> 0	n pages have been skipped
 *	< 0	Error (not enough memory to allocate a new page table)
 *
 */
long kmem_map_page_frame(uint32_t *pdir, 
			uint32_t *pstat,
		  	uintptr_t p_adr, 
		  	uintptr_t v_adr, 
			unsigned long pages,
		  	unsigned flags
		       )
{
	uint32_t *l__tab = NULL;
	unsigned long l__lasttab = 0; 
	long l__retval = 0;
		
	/* The page offset isn't interesting */
	v_adr &= (~0xFFFU);
	p_adr &= (~0xFFFU);
	
	/* Change the page descriptors */
	while(pages --)
	{
		unsigned long l__offs = (v_adr / 4096) & 0x3ff;
		int l__do_invlpg = 0;
		
		/* Is there a need of selecting a new page table ? */
		if (   ((v_adr / (4096 * 1024)) != l__lasttab) 
		    || (l__tab == NULL)
		   )
		{
			l__tab = kmem_get_table(pdir, v_adr, true);
			if (l__tab == NULL) return -1;
			l__lasttab = v_adr / (4096 * 1024);
		}
		
		if (    (flags & GENFLAG_DONT_OVERWRITE_SETTINGS)
		     && (l__tab[l__offs] & GENFLAG_PRESENT)
		   )
		{
			l__retval ++;
			continue;	
		}

		/* Update usage status (page descriptor used) */
		if (    (flags & GENFLAG_PRESENT)
		     && (v_adr < VAS_USER_END)
		     && (!(l__tab[l__offs] & GENFLAG_PRESENT))
		   )
		{
			pstat[v_adr / (4096 * 1024)] ++;
		}
		
		if (    (l__tab[l__offs] & GENFLAG_PRESENT)
		     && (!(flags & GENFLAG_PRESENT))
		     && (v_adr < VAS_USER_END)
		   )
		{
			pstat[v_adr / (4096 * 1024)] --;
		}
		
		/* Do we need to INVLPG? */
		if ((l__tab[l__offs] & GENFLAG_GLOBAL) || (flags & GENFLAG_GLOBAL)) l__do_invlpg = 1;
		
		/* Write the page descriptor */
		l__tab[l__offs] =   (p_adr & (~0xFFFU))		
		                  | (flags & 0xFFFU);
		
		/* Invalidate TLB, if needed */
		if (l__do_invlpg) INVLPG(v_adr);
				  
		/* Increment the addresses */
		p_adr += 4096;
		v_adr += 4096;
	}
	
	return l__retval;
}		 

/*
 * kmem_map_page_frame_cur(p_adr, v_adr, pages, flags)
 *
 * Maps an area of 'pages' page frames at address 'p_adr' 
 * into a virtual address space defined by the currently
 * active page directory to the virtual address 'v_adr'.
 *
 * You can define the access flags of this pages through
 * the 'flags' parameter.
 *
 * If a page should be mapped to an area of the virtual
 * address space that currently isn't described by a 
 * page table, the function will automatically allocate 
 * and map a new page table.
 *
 * The function will neither free page frames, that will
 * be removed from the address space nor change the
 * usage counter of the selected page frames.
 *
 * This function will also invalidate the TLB of the
 * selected pages.
 *
 * ! If you want to map a page into the current virtual
 * ! address space YO HAVE TO use kmem_map_page_frame_cur
 * ! to prevent the use of an invalid TLB content.
 *
 * Using the flag GENFLAG_DONT_OVERWRITE_SETTINGS will prevent
 * that the function overwrites existing areas.
 *
 * Return Value:
 *	== 0	Successful
 *	> 0	n pages have been skipped
 *	< 0	Error (not enough memory to allocate a new page table)
 *
 */
long kmem_map_page_frame_cur(uintptr_t p_adr, 
		  	     uintptr_t v_adr, 
			     unsigned long pages,
			     unsigned flags
			    )
{
	/* Map the pages */
	long l__retval =
		kmem_map_page_frame(i386_current_pdir, 
				    &current_p[PRCTAB_X86_MMTABLE],
			            p_adr, 
		  		    v_adr, 
				    pages,
		  		    flags
		       		   );

	v_adr &= (~0xFFF);
	/* Invalidate the TLB */				   				   
	while (pages --)
	{
		INVLPG(v_adr);
		v_adr += 4096;
	}		   

	return l__retval;
}
			   
/* 
 * kmem_init_krnl_spc
 *
 * Creates the initial kernel page tables (IKP) and page directory.
 *
 */
int kmem_init_krnl_spc(void)
{
	uint32_t *l__pdir = ikp_start;	/* ptr to krnl page dir */
	long l__n = 0, l__o = 0;
	uintptr_t l__ctr = 0;
	
	/*
	 * Initialize the page directory
	 *
	 */
	/* The first 3 GiB are unused and locked */
	l__n = 0xC0000 / 1024;
	
	while(l__n --)
		l__pdir[l__n] = 0;

	/* The following 896 MiB are mapped to the first
	 * 1 GiB of the physical memory. They are using
	 * the page table space of the IKP
	 */
	l__ctr = ((uintptr_t)ikp_start) + ((((0xF7FFF - 0xC0000) / 1024) + 1) * 4096);

	for (l__n = (0xF7FFF / 1024);
	     l__n >= (0xC0000 / 1024);
	     l__n --, l__ctr -= 4096
	    )
	{
		l__pdir[l__n] =   l__ctr
				| PFLAG_PRESENT
				| PFLAG_READWRITE
				| PFLAG_GLOBAL
				      ;
	}
		
	/*
	 * The highest 128 MiB will be initialized in detail
	 * during the initialization of the info page system.
	 * However the page tables are already set to the right
	 * area within the IKP.
	 */
	l__ctr = ((uintptr_t)ikp_start) + ((((0xFFFFF - 0xC0000) / 1024) + 1) * 4096);
	
	for (l__n = (0xFFFFF / 1024);
	     l__n >= (0xF8000 / 1024);
	     l__n --, l__ctr -= 4096
	    )
	{
		l__pdir[l__n] =   l__ctr
			        | PFLAG_PRESENT
			        | PFLAG_GLOBAL
				| PFLAG_USER
			      ;
	}

	/* The lowest 4 MiB of the kernel virtual address space
	 * (address 0 not 3 GiB!) are using the same page tables 
	 * as the first 4 MiB of the last 1 GiB. This is needed 
	 * for paging initialization.
	 * The area will be locked after paging initialization.
	 */
	l__o = (0xC0000 / 1024);
	l__pdir[0] = l__pdir[l__o] | PFLAG_PRESENT
			           | PFLAG_READWRITE
			           | PFLAG_GLOBAL
				  ;
				  
	/*
	 * Initialize the page tables
	 *
	 */
	uint32_t *l__ptab = ikp_start + 1024;	/* ptr to krnl page tabs */
	l__ctr = (896 * 1024 * 1024);
	l__n = (896 * 1024 * 1024) / 4096;

	while(l__n --)
	{
		l__ctr -= 4096;
		l__ptab[l__n] =    l__ctr
			         | PFLAG_PRESENT
			         | PFLAG_READWRITE
			         | PFLAG_GLOBAL
				;
	}

	/*
	 * Initialize the page tables of the info page area
	 *
	 */
	l__ptab = ikp_start + 1024;	/* ptr to krnl page tabs */
	
	for(l__n = (0xFFFFF - 0xC0000);
	    l__n >= (0xF8000 - 0xC0000);
	    l__n --
	   )
	{
		l__ptab[l__n] = PFLAG_GLOBAL;
	}	

	/* Cache will be disabled for the lowest 1 MB because this
	 * area contains different memory I/O areas (EGA Buffer) and won't be
	 * used otherwise
	 */
	l__n = (1 * 1024 * 1024) / 4096;
	
	while(l__n --)
	{
		l__ptab[l__n] |= PFLAG_CACHE_DISABLED;
	}
	
	return 0;		
}

int kmem_switch_krnl_spc(void)
{
	/*
	 * Switch into the kernel virtual address space
	 *
	 */
	__asm__ __volatile__(/* Set the IKP as page directory */
	    		     "movl ikp_start, %%eax\n"
	    		     "movl %%eax, %%cr3\n"
	    		     /* Set paging (PG) */
	    		     "movl %%cr0, %%eax\n"
	    		     "orl $0x80000000, %%eax\n"	
	    		     "movl %%eax, %%cr0\n"
	    		     :
	    		     :
	    		     :"eax"
	    		    );
	/*
	 * Comment: Because the kernel doesn't access directly to
	 * any user address space page, there is no need for supporting
	 * Copy-On-Write for kernl-mode memory access. So we don't
	 * have to set the WP-Flag in the cr0 register.
	 *
	 * However the write to user-mode write-protected pages from
	 * kernel mode is needed to implement the info pages.
	 *
	 */
	   	   
	#ifdef DEBUG_MODE
	kprintf("Entering kernel virtual address space...");
	#endif
	   
	/*
	 * Load the new GDT and IDT
	 *
	 */
	__asm__ __volatile__(
			     ".extern i386_gdt_des_new\n"
			     ".extern i386_idt_des_new\n"
			     "lgdt i386_gdt_des_new\n"
			     "lidt i386_idt_des_new\n"
			    );
   
	/*
	 * Load the new DS, ES, FS, GS, SS
	 * And jump into the new kernel code segment
	 * at virtual address >3 GiB
	 *
	 */
	__asm__ __volatile__(
			     "movw $0x20, %%ax\n"		/* 0x20 = kernel data seg */
			     "movw %%ax, %%ds\n"
			     "movw %%ax, %%es\n"
			     "movw %%ax, %%fs\n"
			     "movw %%ax, %%gs\n"
			     "movw %%ax, %%ss\n"
			     /* Long jump to 0x18:<current> */
			     ".byte 0xea\n"			/* "ljmp" */
			     ".long i386_newseg_reentry\n"	/* dest offs */
			     ".word 0x18\n"			/* dest seg: 0x18 = kernel code */
			     "i386_newseg_reentry:\n"
			     :
			     :
			     :"eax"
	   		    );
			
	/*
	 * Lock the virtual pages < 4 MiB 
	 *
	 */
	uint32_t *l__pdir = ikp_start;
	l__pdir[0] = 0;
	
	/*
	 * Set the paging global extension (PGE)
	 * (Please note, that it is needed to do that
	 *  _after_ locking the < 4 MiB area of
	 *  the initial kernel address space!)
	 *
	 */
	if (i386_do_pge == 1)
	{
		__asm__ __volatile__(
				     "movl %%cr4, %%eax\n"
				     "orl $0x40, %%eax\n"		
				     "movl %%eax, %%cr4\n"
				     :
				     :
				     :"eax"
	   			    );
	}
	
	#ifdef DEBUG_MODE
	kprintf("( DONE )\n\n");
	#endif
	
	i386_current_pdir = ikp_start;	
	
	return 0;
}

/*
 * kmem_create_space
 *
 * Creates a new virtual address space and returns the
 * address of its page directory
 *
 * It sets the memory part above > 3 GiB to the
 * kernel page tables with the 'PFLAG_SUPERVISOR' flag.
 *
 * Return value:
 *	!= NULL		Pointer to the created page directory
 *	== NULL		Allocation failed. Not enough memory.
 *
 */
uint32_t* kmem_create_space(void)
{
	uint32_t *l__pdir = kmem_alloc_kernel_pageframe();
	uint32_t *l__krnldir = ikp_start;
	long l__n = (3 * 1024 * 1024) / (1024 * 4096);

	/* If there is no free page for a page directory */
	if (l__pdir == NULL) return NULL;
		
	/* The upper 1 GiB will be identical to the kernel */
	for (l__n = (3 * 1024 * 1024) / (1024 * 4096);
	     l__n < 1024;
	     l__n ++
	    )
	{
		l__pdir[l__n] = l__krnldir[l__n];	
	}
		
	return l__pdir;
}

/*
 * kmem_destroy_space(pagedir)
 *
 * Destroys a virtual address space by freeing
 * all of its page tables and its page directory.
 * IT WILL NOT FREE ITS PAGE FRAMES! 
 *
 */
void kmem_destroy_space(uint32_t* pagedir)
{
	void *l__tab;
	long l__n = (3 * 1024 * 1024) / (1024 * 4096);
	
	/* Free the tables of the virtual address space */
	while(l__n --)
	{
		l__tab = (void*)(uintptr_t)(pagedir[l__n] & (~0xFFFU));
		if (l__tab != NULL) kmem_free_kernel_pageframe(l__tab);
	}
	
	kmem_free_kernel_pageframe(pagedir);
	
	return;
}

/* 
 * kmem_do_copy_on_write(pdir, sid, usradr)
 *
 * Execute the copy-on-write operation to the user address "usradr".
 * The operation will be done for the address space "pdir" and
 * the process "sid".
 * This function will temporaly map two user mode page frames
 * into the kernel address space to make them accessable.
 *
 * Return value:
 *	>0	Not successful
 *	0	Successful
 *
 */
int kmem_do_copy_on_write(uint32_t* pdir, sid_t sid, uintptr_t usradr)
{
	uint32_t *l__ptab = NULL;
	uintptr_t l__phyadr = 0;
	uintptr_t l__newphy = 0;
	uint32_t *l__entry = NULL;
	uint32_t *l__newframe = NULL;
	uint32_t *l__ktab = ikp_start + 1024;
	uint32_t *l__newptr = (void*)(uintptr_t)(0xFFFE1000 - 0xC0000000);
	uint32_t *l__oldptr = (void*)(uintptr_t)(0xFFFE0000 - 0xC0000000);

	l__ptab = kmem_get_table(pdir,
				 usradr,
				 false
				);

	/* Invalid page table... */
	if (l__ptab == NULL) return 1;
	
	l__entry = &l__ptab[(usradr >> 12) & (0x3ffu)];
	l__phyadr = *l__entry & (~0xfffu);
	
	/* Not selected for copy on write, other exception */
	if (!(*l__entry & GENFLAG_DO_COPYONWRITE)) return 2;
	
	/* No other process is still using this page too? */
	if (PAGE_BUF_TAB(l__phyadr).usage == 1)
	{
		/* Just remove the COW flag, don't copy... */
		*l__entry &= (~GENFLAG_DO_COPYONWRITE);
		*l__entry |= GENFLAG_WRITABLE;
		
		return 0;
	}
		
	/* Allocate new page frame */
	l__newframe = kmem_alloc_user_pageframe(sid, usradr);
	if (l__newframe == NULL) return 3;
	l__newphy = (uintptr_t)l__newframe;
	
	/* Decrement usage counter */
	PAGE_BUF_TAB(l__phyadr).usage --;
	
	/* Map the old page frame into the UMCA */
	l__ktab[0xFFFE0 - 0xC0000] =  l__phyadr | GENFLAG_PRESENT | GENFLAG_READABLE;
	
	/* Map the new page frame into the UMCA */
	l__ktab[0xFFFE1 - 0xC0000] = l__newphy | GENFLAG_PRESENT | GENFLAG_WRITABLE;
	
	/* Invalidate TLB */
	INVLPG(0xFFFE0000);
	INVLPG(0xFFFE1000);

	/* Copy datas */
	int l__n = 1024;
	while(l__n --)
	{
		l__newptr[l__n] = l__oldptr[l__n];
	}
	
	l__ktab[0xFFFE0 - 0xC0000] = 0;
	l__ktab[0xFFFE1 - 0xC0000] = 0;
	
	/* Invalidate TLB again */
	INVLPG(0xFFFE0000);
	INVLPG(0xFFFE1000);	
	
	/* 
	 * Remove the process from the old page frame's 
	 * owners list (PST) and actualize its page table
	 * entry.
	 */
	 kmem_free_user_pageframe(l__phyadr, sid, usradr);
	 
	 *l__entry = l__newphy | (   GENFLAG_PRESENT
		        	  | GENFLAG_READABLE
		        	  | GENFLAG_WRITABLE
		        	  | GENFLAG_EXECUTABLE
		        	  | GENFLAG_USER_MODE 
				);
	
	/* Invalidate TLB? */
	if (pdir == i386_current_pdir) INVLPG(usradr);

	return 0;
}

/*
 * kmem_copy_on_write()
 *
 * Tries to handle the copy-on-write exception, if possible.
 *
 * Return value:
 *	>0	Not successful
 *	0	Successful
 */
int kmem_copy_on_write(void)
{
	uintptr_t l__usradr = 0;

	/* Get the user mode address of the access violation */
	__asm__ __volatile__ (
			      "movl %%cr2, %%eax\n"
			      : "=a" (l__usradr)
			     );

	/* Try to copy it */
	return kmem_do_copy_on_write(i386_current_pdir, current_p[PRCTAB_SID], l__usradr);
}
