/*
 *
 * heap.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Managment of the heap
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/hymk.h>
#include <hydrixos/errno.h>
#include <hydrixos/mem.h>
#include <hydrixos/mutex.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>

#include "hybaselib.h"


extern uintptr_t lib_heap_start;

/*
 * mem_heap_inc(pages)
 *
 * Increment the size of the heap about "pages" pages
 * by mapping newly allocated pages to the heap.
 * This function will modify the region descriptor of
 * the heap, so we will need to lock the region mutex.
 * This function won't create a new free memory block,
 * this has to be done by the calling function.
 *
 */
void mem_heap_inc(unsigned pages)
{
	uintptr_t l__start = 0;

	/* No sensless sizes...*/
	if (pages <= 0)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return;
	}
			
	/* Lock the region mutex */
	mtx_lock(&reg_mutex, -1);
	
	/* Do we have enogh free heap address space? */
	if ((heap_region->usable_pages + pages) > (heap_region->pages - 1))
	{
		*tls_errno = ERR_NOT_ENOUGH_MEMORY;
		mtx_unlock(&reg_mutex);
		return;
	}
	
	/* Calculate the start address of our new mapping */
	l__start =   ((uintptr_t)lib_heap_start) 
	           + (heap_region->usable_pages * ARCH_PAGE_SIZE) 
	          ;
	
	/* Allocate the new pages */
	hysys_alloc_pages((void*)l__start, pages);
	if (*tls_errno)
	{
		/* Not possible... */
		mtx_unlock(&reg_mutex);
		return;
	}
	
	/*
	 * Remove the execution flag of the new pages, because the heap
	 * doesn't contain executable data for normal
	 *
	 */
	hysys_unmap(0, (void*)l__start, pages, UNMAP_EXECUTE);
	
	/* Resize the heap size information */
	heap_region->usable_pages += pages;	
	heap_region->readable_pages += pages;	
	heap_region->writeable_pages += pages;	
	
	mtx_unlock(&reg_mutex);
}

/*
 * mem_heap_dec (pages)
 *
 * Reduces the size of the heap by unmapping
 * "pages" pages at the end of the heap and
 * by reducing the count of usable pages
 * of the heap.
 * This function won't touch any memory block
 * descriptor. Before calling it you should
 * be aware of removing any block descriptor 
 * out of this area to prevent inconsistencies
 * within the block table.
 *
 */
void mem_heap_dec(unsigned pages)
{
	/* Close the regions mutex at first */
	mtx_lock(&reg_mutex, -1);
	
	/* Is it a valid page count? */
	if (heap_region->usable_pages < pages)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		mtx_unlock(&reg_mutex);
		return;
	}
	
	/* Calculate the address of the heap's end */
	uintptr_t l__adr =    ((uintptr_t)lib_heap_start) 
	                    + ((heap_region->usable_pages - pages) * ARCH_PAGE_SIZE)
	                   ;
	
	/* Okay, unmap the pages */
	hysys_unmap(0, (void*)l__adr, pages, UNMAP_COMPLETE);

	/*
	 * NOTE: We are ignoring any returned error value, because
	 *       there is nothing we could do if unmap() returns an
	 *       error. The best thing we could do is to resize the
	 *	 heap - so we prevent lefting the heap in an unkown
	 *	 state...
	 */

	/* Resize the used heap region */
	heap_region->usable_pages -= pages;
	heap_region->readable_pages -= pages;
	heap_region->writeable_pages -= pages;
	
	mtx_unlock(&reg_mutex);
	
	return;
}


