/*
 *
 * region.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Managment of memory regions
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/hymk.h>
#include <hydrixos/errno.h>
#include <hydrixos/mem.h>
#include <hydrixos/mutex.h>
#include <hydrixos/stdfun.h>
#include "hybaselib.h"

region_t *regions = NULL; 	/* Linked list of memory regions */
region_t *code_region = NULL;	/* Descriptor of the code region */
region_t *data_region = NULL; 	/* Descriptor of the data region */
region_t *stack_region = NULL;	/* Descriptor of the stack region */
region_t *heap_region = NULL;	/* Descriptor of the heap region */

mtx_t reg_mutex = MTX_DEFINE();	/* Mutex for the regions data structure */ 

/*
 * reg_create (region)
 *
 * Creates a region according to the descriptor "region".
 * This function will allocate and map a new page to the
 * start address of the region where the region header will
 * be placed to.
 *
 * If the region overlaps with an existing region, this
 * operation will fail. This function will also only
 * accept the correct region flags and won't allow the
 * creation of a region into the upper memory areas.
 *
 */
region_t* reg_create(region_t region)
{
	uintptr_t l__regstart = (uintptr_t)region.start;
	
	/* Error if flags are invalid */
	if (region.flags & (~0xf))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	/* Is this region overlaping with another? */
	if (regions != NULL)
	{
		region_t *l__region = regions;
				
		mtx_lock(&reg_mutex, -1);
		/* Browse the region list */
		while (l__region != NULL) 
		{
			uintptr_t l__otherstart = (uintptr_t)l__region->start;
			
			/* l__region starts before region */
			if (    (l__otherstart <= l__regstart)
			     && (((l__otherstart + (l__region->pages * ARCH_PAGE_SIZE)) > l__regstart))
			   )
			{
				mtx_unlock(&reg_mutex);
				*tls_errno = ERR_RESOURCE_BUSY;
				
				return NULL;
			}
			
			/* l__region start behind region */
			if (    (l__otherstart >= l__regstart)
			     && (((l__regstart + (region.pages * ARCH_PAGE_SIZE)) > l__otherstart))
			   )
			{
				mtx_unlock(&reg_mutex);
				*tls_errno = ERR_RESOURCE_BUSY;
				
				return NULL;
			}
			
			/* Select next region for testing */
			l__region = l__region->ls.n;
		}
	}
	
	mtx_unlock(&reg_mutex);
	
	/* Alloc a page */
	hymk_alloc_pages((void*)region.start, 1);
	if (*tls_errno) return NULL;
	
	/* Write the region descriptor */
	region_t *l__dregion = region.start;
	
	*l__dregion = region;
	
	/* Calculate the checksum */
	l__dregion->chksum = (l__regstart + region.pages) * 2;
	
	/* Add the descriptor to the regions list */
	lst_init(l__dregion);
	
	mtx_lock(&reg_mutex, -1);
	lst_add(regions, l__dregion);
	mtx_unlock(&reg_mutex);
	
	return l__dregion;
}

/*
 * reg_destroy(region)
 *
 * Destroys a memory region "region" and removes all of its pages.
 *
 */
void reg_destroy(region_t *region)
{
	region_t l__region;
	
	/* Is this a valid pointer? */
	if (region == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return;
	}
	
	/* Test the checksum of this region */
	if (region->chksum != (((uintptr_t)region->start + region->pages) * 2))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return;
	}
	
	/* Lock all further region operations */
	mtx_lock(&reg_mutex, -1);

	/* Remove the region descriptor from the regions list */
	l__region = *region;
	lst_del(region);
	
	/* Remove its pages */
	hymk_unmap(0, l__region.start, l__region.pages, UNMAP_COMPLETE);
	
	mtx_unlock(&reg_mutex);

	return;
}

/*
 * lib_init_regions
 *
 * Initializes the memory regions.
 *
 * Return value:
 *	0	Operation successful.
 *	1	Operation failed.
 *
 * TODO: Currently we can't imaginge how the hydrixos
 *       program loader will look like. So this function
 *	 makes a statical initialization of the memory
 *	 regions based on the temporary kernel ABI 
 *       (lib_grub_module_start and lib_grub_module_pages). 
 *
 *       In later versions we need something
 *	 like a memory descriptor delivered by the
 *	 program loader to do that properly.
 */
extern uintptr_t lib_grub_module_start;
extern uint32_t lib_grub_module_pages;
extern uintptr_t lib_heap_start;

region_t lib_tmp_code_region;
region_t lib_tmp_data_region;

int lib_init_regions(void)
{
	/* Setup the code region*/
	str_copy(lib_tmp_code_region.name, "code", 5);
	lib_tmp_code_region.id = 1;
	
	lib_tmp_code_region.start = (void*)lib_grub_module_start;
	lib_tmp_code_region.pages = lib_grub_module_pages;
	
	lib_tmp_code_region.flags =   REGFLAGS_READABLE 
	                            | REGFLAGS_WRITEABLE 
	                            | REGFLAGS_EXECUTABLE
	                           ;
	lib_tmp_code_region.usable_pages = lib_grub_module_pages;
	lib_tmp_code_region.readable_pages = lib_grub_module_pages;
	lib_tmp_code_region.writeable_pages = lib_grub_module_pages;
	lib_tmp_code_region.executable_pages = lib_grub_module_pages;
	lib_tmp_code_region.shared_pages = 0;
			
	lib_tmp_code_region.alloc = NULL;
	lib_tmp_code_region.free = NULL;
			
	lib_tmp_code_region.chksum = (lib_grub_module_start + lib_grub_module_pages) * 2;
	lib_tmp_code_region.ls.p = NULL;
	lib_tmp_code_region.ls.n = &lib_tmp_data_region;
	
	/* Setup the data region */
	str_copy(lib_tmp_data_region.name, "data", 5);
	lib_tmp_data_region.id = 2;
	
	lib_tmp_data_region.start = (void*)lib_grub_module_start;
	lib_tmp_data_region.pages = lib_grub_module_pages;
	
	lib_tmp_data_region.flags =   REGFLAGS_READABLE 
	                            | REGFLAGS_WRITEABLE 
	                           ;
	lib_tmp_data_region.usable_pages = lib_grub_module_pages;
	lib_tmp_data_region.readable_pages = lib_grub_module_pages;
	lib_tmp_data_region.writeable_pages = lib_grub_module_pages;
	lib_tmp_data_region.executable_pages = 0;
	lib_tmp_data_region.shared_pages = 0;
			
	lib_tmp_data_region.alloc = NULL;
	lib_tmp_data_region.free = NULL;
			
	lib_tmp_data_region.chksum = (lib_grub_module_start + lib_grub_module_pages) * 2;
	lib_tmp_data_region.ls.p = &lib_tmp_code_region;
	lib_tmp_data_region.ls.n = NULL;	
	
	/* Init the regions list */
	regions = &lib_tmp_code_region;
	code_region = &lib_tmp_code_region;
	data_region = &lib_tmp_data_region;
	
	/*
	 * Create the stack region 
	 *
	 * Currently we use a 100 MiB virtual memory area
	 * for the stack region.
	 *
	 * TODO: Currently there is no usage for the
	 *       stack region, so we keep it as small
	 *       as possible.
	 */
	region_t l__region;
	
	str_copy(l__region.name, "stack", 6);
	l__region.id = 3;
	
	l__region.start = (void*)(lib_grub_module_start + (lib_grub_module_pages * 4096));
	l__region.pages = 2;	/* Remember: No usage for stack region currently! */
	
	l__region.flags =    REGFLAGS_READABLE 
	                   | REGFLAGS_WRITEABLE 
	                  ;
	l__region.usable_pages = 1;
	l__region.readable_pages = 1;
	l__region.writeable_pages = 1;
	l__region.executable_pages = 0;
	l__region.shared_pages = 0;
			
	l__region.alloc = mem_stack_alloc;
	l__region.free = mem_stack_free;
			
	l__region.chksum = ((uintptr_t)l__region.start + l__region.pages) * 2;
	l__region.ls.p = NULL;
	l__region.ls.n = NULL;
	
	stack_region = reg_create(l__region);
	if (stack_region == NULL) return 1;
	
	/*
	 * Create the heap region 
	 *
	 * Currently we use a 1 GiB virtual memory area
	 * for the stack region.
	 *
	 */	
	str_copy(l__region.name, "heap", 5);
	l__region.id = 4;
	
	l__region.start = (void*)((uintptr_t)stack_region->start + (stack_region->pages * 4096));
	l__region.pages = 262144;
	
	l__region.flags =    REGFLAGS_READABLE 
	                   | REGFLAGS_WRITEABLE 
	                  ;
	l__region.usable_pages = 0;
	l__region.readable_pages = 0;
	l__region.writeable_pages = 0;
	l__region.executable_pages = 0;
	l__region.shared_pages = 0;
			
	l__region.alloc = mem_alloc;
	l__region.free = mem_free;
			
	l__region.chksum = ((uintptr_t)l__region.start + l__region.pages) * 2;
	l__region.ls.p = NULL;
	l__region.ls.n = NULL;
	
	heap_region = reg_create(l__region);
	if (heap_region == NULL) return 1;
	
	/* We have to set the start address of the usable heap memory to lib_heap_start */
	lib_heap_start = (((uintptr_t)l__region.start) + 4096);
		
	return 0;
}
