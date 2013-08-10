/*
 *
 * alloc.c
 *
 * (C)2004 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Page frame allocation
 *
 */
#include <hydrixos/types.h>
#include <stdio.h>
#include <setup.h>
#include <mem.h>
#include <error.h>
#include <sysc.h>
#include <page.h>
#include <info.h>
#include <current.h>
#include <hymk/sysinfo.h>


/*
 * kmem_zero_out(page)
 *
 * Fills a newly allocate page frame 'page' (void*) 
 * with zeros
 *
 * Use this macro carefully: It would change the
 * string direction flag and write to the ES segment.
 *
 */
#define kmem_zero_out(___page) \
{\
	uint32_t d1, d2;\
	\
	__asm__ __volatile__ (\
			      "cld\n"\
			      "rep stosl\n"\
			      : "=&c" (d1), "=&D" (d2)\
			      : "0" (1024), "a" (0), "1" ((___page))\
			      : "memory"\
			     );\
}

/*
 * kmem_alloc_kernel_pageframe()
 *
 * Allocates a page frame of the page buffer for
 * use in kernel mode. This page frame will be
 * only taken from the normal zone.
 *
 * Return value: != NULL adress of the allocated area
 * 		    NULL  not enough free memory
 *
 */
void* kmem_alloc_kernel_pageframe(void)
{
	void* l__retval;	
	
	/* Is the normal zone empty? */
	if (normal_zone.frame_stack_count != 0)
	{
		/* No. Let's take a page */
		l__retval = (void*)(uintptr_t)(*(normal_zone.frame_stack_ptr));
		
		normal_zone.frame_stack_ptr ++;
		normal_zone.frame_stack_count --;
		
		PAGE_BUF_TAB((uintptr_t)l__retval).usage = 1;
		PAGE_BUF_TAB((uintptr_t)l__retval).owner.single.pid = 
					SID_PLACEHOLDER_KERNEL;
		PAGE_BUF_TAB((uintptr_t)l__retval).owner.single.u_adr = 
					((uintptr_t)l__retval) 
				      + VAS_KERNEL_START;
		
		kmem_zero_out(l__retval);
		
		return l__retval;
	}	
	
	/* Out of memory */
	return NULL;	
}

/*
 * kmem_alloc_user_pageframe()
 *
 * Allocates a page frame of the page buffer for
 * use in user mode. This page frame might be
 * placed in the high zone as well as in the normal
 * zone, so it might be impossible to address it
 * from kernel mode.
 *
 * Return value: != NULL adress of the allocated area
 * 		    NULL  not enough free memory
 *
 */
void* kmem_alloc_user_pageframe(sid_t pid, uintptr_t u_adr)
{
	void* l__retval;	

	/* Try high_zone first */
	if (high_zone.frame_stack_count != 0)
	{
		uint32_t *l__ktab = ikp_start + 1024;
		
		/* No. Let's take a page */
		l__retval = (void*)(uintptr_t)(*(high_zone.frame_stack_ptr));
		high_zone.frame_stack_count --;
		high_zone.frame_stack_ptr ++;
			
		PAGE_BUF_TAB((uintptr_t)l__retval).usage = 1;
		PAGE_BUF_TAB((uintptr_t)l__retval).owner.single.pid = pid;
		PAGE_BUF_TAB((uintptr_t)l__retval).owner.single.u_adr = u_adr;
		
		/* Just map the page frame into the UMCA to zero out */
		l__ktab[0xFFFE2 - 0xC0000] =   ((uintptr_t)l__retval) 
					     | GENFLAG_PRESENT | GENFLAG_WRITABLE;

		INVLPG(0xFFFE2000);

		kmem_zero_out(((void*)(uintptr_t)(0xFFFE2000 - 0xC0000000)));
		
		l__ktab[0xFFFE2 - 0xC0000] = 0;
		INVLPG(0xFFFE2000);
		
		return l__retval;
	}

	/* Try the normal_zone */
	if (normal_zone.frame_stack_count != 0)
	{
		/* No. Let's take a page */
		l__retval = (void*)(uintptr_t)(*(normal_zone.frame_stack_ptr));
		normal_zone.frame_stack_ptr ++;
		normal_zone.frame_stack_count --;
		
		PAGE_BUF_TAB((uintptr_t)l__retval).usage = 1;
		PAGE_BUF_TAB((uintptr_t)l__retval).owner.single.pid = pid;
		PAGE_BUF_TAB((uintptr_t)l__retval).owner.single.u_adr = u_adr;
		
		kmem_zero_out(l__retval);
		
		return l__retval;
	}	
	
	/* Out of memory */
	return NULL;	
}

/*
 * kmem_free_kernel_pageframe(page)
 *
 * Finally frees an page frame on the physical address
 * 'page', even if the page frame is used by other
 * processes. So if you want to free an user mode
 * page frame, you should probably use 
 * kmem_free_user_pageframe.
 *
 * The function will reset the page frame status information
 * of the PBT without removing PST-entries.
 *
 */
void kmem_free_kernel_pageframe(void* page)
{
	uintptr_t l__page = (uintptr_t)page;
	l__page &= (~0xFFF);
	
	/* Is it a valid middle-zone page frame */
	if (    (l__page < page_buffer_start)
	     || (l__page > (page_buffer_start + page_buffer_sz))
	   )
	{
		return;
	}
	
	PAGE_BUF_TAB(l__page).usage = 0;	
	PAGE_BUF_TAB((uintptr_t)l__page).owner.single.pid = 0;
	PAGE_BUF_TAB((uintptr_t)l__page).owner.single.u_adr = 0;
		
	/* Is it a high zone page? */
	if (l__page >= NORMAL_ZONE_END)
	{
		high_zone.frame_stack_count ++;
		high_zone.frame_stack_ptr --;	
		*(high_zone.frame_stack_ptr) = l__page;
		
		return;
	}
	
	normal_zone.frame_stack_count ++;
	normal_zone.frame_stack_ptr --;	
	*(normal_zone.frame_stack_ptr) = l__page;	

	return;
}

/*
 * kmem_free_user_pageframe(page, pid, u_adr)
 *
 * Removes the useage information of a page frame at physical
 * address 'page' for the process 'pid' at address 'u_adr'.
 * If the page frame will be only used by a single process by now, 
 * the function removes the useage-list. If the page frame will 
 * be unused, the function frees the page using 
 * 'kmem_free_kernel_pageframe'.
 * 
 * If the selected process 'pid' would not use the page frame
 * at address 'u_adr', nothing happens.
 *
 */
void kmem_free_user_pageframe(uintptr_t page, sid_t pid, uintptr_t u_adr)
{
	long l__n;
	sidlist_t *l__slist_last;
	sidlist_t *l__slist_cur;
	page &= (~0xFFF);

	/* Is it a valid middle-zone page frame */
	if (    (page < page_buffer_start)
	     || (page > (page_buffer_start + page_buffer_sz))
	   )
	{
		return;
	}	
	
	/* The page is only used by one owner */
	if (PAGE_BUF_TAB(page).usage == 1)
	{
		/* Is this the correct owner and the right user-space address? */
		if (    (PAGE_BUF_TAB(page).owner.single.pid == pid)
		     && (PAGE_BUF_TAB(page).owner.single.u_adr == u_adr)
		   )
		{
			/* Yes. Free the page frame */
			kmem_free_kernel_pageframe((void*)page);
		}
		
		return;
	}

	/* The page is used by multiple owners, search it */
	l__n = PAGE_BUF_TAB(page).usage;
	l__slist_last = NULL;
	l__slist_cur = PAGE_BUF_TAB(page).owner.multi;

	while (l__n --)
	{
		/* Is this the searched owner entry? */
		if (    (l__slist_cur->user.pid == pid)
		     && (l__slist_cur->user.u_adr == u_adr)
		   )
		{
			/* Okay, just remove the entry from the list*/
			if (l__slist_last != NULL ) 
			{
				/* Entry is somewhere in the list */
				l__slist_last->nxt = l__slist_cur->nxt;
			}
			 else
			{
				/* Entry is at the beginnig of the list */
				PAGE_BUF_TAB(page).owner.multi = l__slist_cur->nxt;
			}
			
			/* At the entry to the free entries list of the PST */
			l__slist_cur->nxt = page_share_table_free->nxt;

			if (page_share_table_free != NULL)
				page_share_table_free->nxt = l__slist_cur;
			else
				page_share_table_free = l__slist_cur;

			/* Decrement usage status */
			PAGE_BUF_TAB(page).usage --;
			
			/* Do we need still a list? */
			if (PAGE_BUF_TAB(page).usage == 1)
			{
				/* No? Switch to single-owner mode */
				l__slist_cur = PAGE_BUF_TAB(page).owner.multi;
				
				PAGE_BUF_TAB(page).owner.single.pid = l__slist_cur->user.pid;
				PAGE_BUF_TAB(page).owner.single.u_adr = l__slist_cur->user.u_adr;
				
				/* Free the remaining list entry */
				l__slist_cur->nxt = page_share_table_free->nxt;
				page_share_table_free->nxt = l__slist_cur;
			}
			
			return;
		}
		 else
		{
			/* Next turn, save the pointer to last entry */
			l__slist_last = l__slist_cur;
			l__slist_cur = l__slist_cur->nxt;
		
			if (l__slist_cur->nxt == NULL) break;
		}
	}
	
	/* Page not included in list */
	return;
}

/*
 * sysc_alloc_pages
 *
 * (Implementation of the "alloc_pages" system call)
 *
 * Allocates a defined number of page frames and maps
 * it into the current virtual address space to a given
 * destination address.
 *
 * If the destination area is already associated with page
 * frames, these page frames should be freed first.
 *
 * The destination area may not contain the kernel memory,
 * the zero page or the thread local storage.
 *
 * Parameters:
 *	start	Destination address 		
 *	pages	Number of page frames
 *
 */
void sysc_alloc_pages(uintptr_t start, unsigned long pages)
{
	void* l__memory;
	
	/* Align start address to the start of the page */
	start &= (~0xfff);

	/* Test the execution restriction */
	if (pages > MEM_MAX_PAGE_OP_NUM)	
	{
		SET_ERROR(ERR_SYSCALL_RESTRICTED);
		return;
	}
	
	/* Is the destination area within a invalid memory area? */
	if (    ((start + (pages * 4096)) > VAS_KERNEL_START)
	     || (start > VAS_KERNEL_START)
	     || ((start + (pages * 4096)) < start)
	     || (start == VAS_THREAD_LOCAL_STORAGE)
	     || ((start + (pages * 4096)) > VAS_THREAD_LOCAL_STORAGE)
	   )
	{
		SET_ERROR(ERR_INVALID_ADDRESS);
		return;
	}
		   
	/* Allocate the new memory area */
	while(pages --)
	{
		l__memory = kmem_alloc_user_pageframe(current_p[PRCTAB_SID], 
						      start 
						     );

		if (l__memory == NULL)
		{
			SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
			return;	
		}

		/* Map it into the current virtual address space */
		long l__ret = kmem_map_page_frame_cur((uintptr_t)l__memory, 
		        	    		      start, 
		        	    	 	      1,
		        	    	 	        GENFLAG_PRESENT
		        	    		      | GENFLAG_READABLE
		        	    		      | GENFLAG_WRITABLE
		        	    	 	      | GENFLAG_EXECUTABLE
		        	    	 	      | GENFLAG_USER_MODE
					  	      | GENFLAG_DONT_OVERWRITE_SETTINGS
		        	    	 	     );
		        	    	 	     
		/* No free page frame? */		        	    	 	   
		if (l__ret < 0)
		{
			SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
			return;	
		}
		
		/* There was an existing mapping? */
		if (l__ret > 0)
		{
			kmem_free_kernel_pageframe(l__memory);
		}
		
		start += 4096;
	}

	return;				   
}
