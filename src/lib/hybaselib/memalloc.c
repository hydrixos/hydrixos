/*
 *
 * memalloc.c
 *
 * (C)2005 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Heap memory allocation
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

/*
 * TODO: Try to unify the logical memory managment in pmap.c
 *	 with alloc.c. It is stupid to have nearly the same
 *	 jobs done in two different files.
 *
 *	 Friedrich Gr�ter
 *
 */

/* The start address of the heap */
uintptr_t lib_heap_start = NULL;

/*
 * The block descriptor type
 *
 */
typedef struct {
	uintptr_t	start;		/* The start address of the block (incl. header) */
	uintptr_t	data;		/* The start address of the block data (excl. header) */
	
	size_t		size;		/* Block size (without header) */
	int		status;		/* Block status (see LIB_BLOCKSTAT_* Macros) */
	int		fbl;		/* FBL number of a block with this size */
	
	list_t		gbl_ls;		/* Links to the general block list */
	list_t		ufbl_ls;	/* Links to the free / used block list (according to status) */
	
}block_t;

	/* Our block is a free block, but some pages are currently removed */
#define LIB_BLOCKSTAT_FREE_AND_EMPTY		-1
	/* Our block is a free block with all pages existing */
#define	LIB_BLOCKSTAT_FREE			0
	/* Our block is currently in use as a memory buffer */
#define LIB_BLOCKSTAT_USED			222

mtx_t lib_heap_mutex = MTX_DEFINE();		/* The block table mutex */

static block_t *lib_general_bl = NULL;		/* The general block list (GBL) */
static block_t *lib_last_gbl_entry = NULL;	/* Last block descriptor of the GBL */

static block_t *lib_used_bl = NULL;		/* The used block list (UBL) */

static block_t *lib_free_bl[31] = 		/* The free block lists (FBL[i]) */
	{
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};

/*
 * lib_init_heap
 *
 * Initializes the heap managment.
 *
 * Return value:
 *	0	Operation successful
 *	1	Operation failed
 */
int lib_init_heap(void)
{
	lib_general_bl = NULL;
	lib_last_gbl_entry = NULL;
	lib_used_bl = NULL;
	
	int l__i = 31;
	
	while (l__i --) lib_free_bl[l__i] = NULL; 		
	
	
	return 0;
}

/*
 * lib_get_right_fbl (size)
 *
 * Returns the ordinal number of the 
 * FBL for a certain block size.
 *
 * Return value:
 *	>= 0	Number of the FBL
 *	<  0	Invalid size
 *
 */
static inline int lib_get_right_fbl(size_t sz)
{
	int l__i = 0;
	size_t l__startsz = 1;
	
	if (sz == 0) return -1;
	
	/*
	 * The valid size range of a FBL of
	 * the ordinal number i are
	 *
	 * from (2^i) to (2^(i+1) - 1) bytes.
	 *
	 */
	do
	{
		/* I know, we can do this in a simplier
		 * way using some math. Unfortunally we
		 * have no FPU (for log2) or probably I'm 
		 * currently to stupid for a completly
		 * other solution.
		 */
		size_t l__endsz = (l__startsz * 2) - 1;
		
		if ((sz >= l__startsz) && (sz <= l__endsz))
			return l__i;
			
		l__startsz *= 2;		
	}while (++ l__i < 31);
	
	return -1;
}

/*
 * lib_add_to_fbl (block)
 *
 * Adds an block "block" to a FBL. 
 *
 * NOTE: 
 *	 - The parameter block have to point to a 
 *	   correct datastructure
 *	 - The block have to be already declared
 *	   as a free memory block
 *	 - The block can't be also a member of the
 *	   UBL
 *	 - The heap mutex sould have been closed
 *
 */
static inline void lib_add_to_fbl(block_t *block)
{
	int l__fbl = 0;
	block_t *l__tmpbl = NULL;
	
	/* Find the fitting FBL */
	l__fbl = block->fbl;
	
	/* Delete existing bindings */
	block->ufbl_ls.p = NULL;
	block->ufbl_ls.n = NULL;	
	
	/* Add it to the beginning of the FBL */
	l__tmpbl = lib_free_bl[l__fbl];
	
	if (l__tmpbl != NULL)
	{
		l__tmpbl->ufbl_ls.p = block;
		block->ufbl_ls.p = NULL;
		block->ufbl_ls.n = l__tmpbl;
	}
	
	lib_free_bl[l__fbl] = block;
	
	return;
}

/*
 * lib_remove_from_fbl (block)
 *
 * Removes an block "block" from its FBL.
 *
 * NOTE: 
 *	 - The parameter block have to point to a 
 *	   correct datastructure
 *	 - The block have to be already declared
 *	   as a free memory block
 *	 - The block can't be also a member of the
 *	   UBL
 *	 - The heap mutex sould have been closed
 *
 */
static inline void lib_remove_from_fbl(block_t *block)
{
	block_t *l__tmp_n = block->ufbl_ls.n;
	block_t *l__tmp_p = block->ufbl_ls.p;
	
	if (l__tmp_n != NULL)
	{
		l__tmp_n->ufbl_ls.p = l__tmp_p;
		block->ufbl_ls.n = NULL;
	}
	
	if (l__tmp_p != NULL)
	{
		l__tmp_p->ufbl_ls.n = l__tmp_n;
		block->ufbl_ls.p = NULL;
	}
	 else
	{
		/* It seems we are the first entry of our FBL */
		int l__fbl = block->fbl;
		
		lib_free_bl[l__fbl] = l__tmp_n;
		
	}
	
}

/*
 * lib_create_free_entry(start, size, type, prev)
 *
 * Creates a free memory entry to the position "start"
 * with the data-size (without header) "size".
 * "type" can be LIB_BLOCKSTAT_FREE or 
 * LIB_BLOCKSTAT_FREE_AND_EMPTY.
 * The function will add the new area to the
 * right fbl. The pointer "prev" will point to the
 * GBL entry that should previous to the new entry.
 * If "prev" ist NULL, the function will initialize
 * the GBL.
 * The function may overwrite an existing "used"
 * entry.
 *
 * NOTE: 
 *	 - The caller have to close the heap mutex
 *	 - The memory area needs to have a
 *	   working page mapping for the header of the
 *	   free new block at least
 *	 - The "prev" entry have to be correct or "NULL". 
 *	   The usage of an incorrect entry will damage
 *	   the memory manamgnet system.
 *	 - Make sure that the entry "prev" don't overlap
 *	   into the area of our new entry!
 *	 - The function won't validate the parameters!
 *
 * Return value:
 *	Pointer to this data structure
 *	NULL, if failed.
 *
 */
static block_t* lib_create_free_entry(uintptr_t start, size_t sz, int type, block_t *prev)
{
	block_t *l__newbl = (void*)start;
	
	/* Initialize the new block descriptor */
	l__newbl->start = start;
	l__newbl->data = start + sizeof(block_t);
	l__newbl->size = sz;
	l__newbl->status = type;
	l__newbl->fbl = lib_get_right_fbl(sz);
	l__newbl->ufbl_ls.n = NULL;
	l__newbl->ufbl_ls.p = NULL;
	
	/* Add it to the GBL */
	if (prev != NULL)
	{
		block_t *l__tmp = prev->gbl_ls.n;
		
		l__newbl->gbl_ls.p = prev;
		l__newbl->gbl_ls.n = prev->gbl_ls.n;
		prev->gbl_ls.n = l__newbl;
		
		if (l__tmp != NULL)
		{
			l__tmp->gbl_ls.p = l__newbl;
		}
		 else
		{
			/* We seem to stay at the end of the GBL */
			lib_last_gbl_entry = l__newbl;
		}
	}
	 else /* Initialize the GBL */
	{
		l__newbl->gbl_ls.p = NULL;
		l__newbl->gbl_ls.n = NULL;
		lib_general_bl = l__newbl;
		lib_last_gbl_entry = l__newbl;
	}

	/* Add it to the right FBL */
	lib_add_to_fbl(l__newbl);

	return l__newbl;
}

/*
 * lib_add_to_ubl (block)
 *
 * Adds the used entry "block" to the UBL. If
 * there is no UBL entry, the function will
 * initialize the UBL.
 *
 * The function will return ERR_MEMORY_CORRUPTED into
 * tls_errno if there is something going wrong...
 *
 * NOTE:
 *	 - The caller has to lock the heap_mutex
 *	 - The function expects that "block" is already
 *	   configured as a "locked" entry and removed
 *	   from its former FBL.
 *
 */
static void lib_add_to_ubl(block_t *block)
{
	/* Delete existing bindings */
	block->ufbl_ls.p = NULL;
	block->ufbl_ls.n = NULL;	
	
	/* Are we the very first UBL block? */
	if (lib_used_bl == NULL)
	{
		lib_used_bl = block;

		return;
	}
	
	/* Are we the very first address of the UBL? */
	if (lib_used_bl->start > block->start)
	{
		block->ufbl_ls.n = lib_used_bl;
		block->ufbl_ls.p = NULL;
		lib_used_bl->ufbl_ls.p = block;
		lib_used_bl = block;
	}
	 else
	{
		/*
	 	 * Okay, we are somewhere with in the UBL, so we use
	 	 * the GBL to find our previous UBL entry as fast as 
	 	 * possible
	 	 */
		block_t *l__block = block->gbl_ls.p;
		block_t *l__prevubl = NULL;
	
		while(l__block != NULL)
		{	
			if (    (l__block->start < block->start) 
		     	     && (l__block->status == LIB_BLOCKSTAT_USED)
		     	   )
			{
				l__prevubl = l__block;
				break;
			}
		
			l__block = l__block->gbl_ls.p;
		}
	
		if (l__prevubl == NULL)
		{
			/* This sould not happen */
			*tls_errno = ERR_MEMORY_CORRUPTED;
			return;
		}
	
		/* Add it to the UBL */
		block->ufbl_ls.n = l__prevubl->ufbl_ls.n;
		block->ufbl_ls.p = l__prevubl;
		l__prevubl->ufbl_ls.n = block;
	
		if (block->ufbl_ls.n != NULL)
		{
			block_t *l__tmp = block->ufbl_ls.n;
			l__tmp->ufbl_ls.p = block;
		}
	}
	
	return;
}

/*
 * lib_remove_from_ubl (block)
 *
 * Removes the used entry "block" from the UBL. If the UBL
 * will get empty, the function will de-initialize the UBL.
 *
 * NOTE: - The caller has to lock the heap_mutex
 *	 - The parameter "block" hast to be correct
 *
 */
static void lib_remove_from_ubl(block_t *block)
{
	/* Are we at the beginnig of the UBL? */
	if (block == lib_used_bl)
	{
		block_t *l__tmpbl = block->ufbl_ls.n;
		
		if (l__tmpbl != NULL)
		{
			/* Set our next entry as first entry */
			lib_used_bl = l__tmpbl;
			l__tmpbl->ufbl_ls.p = NULL;
		}
		 else
		{
			/* De-initialize the UBL */
			lib_used_bl = NULL;
		}
	}
	 else
	{
		/* We are somewhere within the UBL */
		block_t *l__tmpbl_n = block->ufbl_ls.n;
		block_t *l__tmpbl_p = block->ufbl_ls.p;
		
		if (l__tmpbl_n != NULL)
		{
			l__tmpbl_n->ufbl_ls.p = l__tmpbl_p;
		}
		
		if (l__tmpbl_p != NULL)
		{
			l__tmpbl_p->ufbl_ls.n = l__tmpbl_n;
		}
	}
	
	block->ufbl_ls.n = NULL;
	block->ufbl_ls.p = NULL;
	
	return;
}

/*
 * lib_split_free_entry (block, sz)
 *
 * Splits an free entry "block" in to a locked entry and a
 * free entry. The locked entry gets the size "sz".
 * If the new rest entry is smaller than sizeof(block_t)
 * the function won't split the entry.
 * The newly created locked entry will be identical with
 * "block". The parameter "sz" is not including the header
 * size.
 *
 * The function will return ERR_MEMORY_CORRUPTED into
 * tls_errno if there is something going wrong...
 *
 * NOTE: 
 *	 - The caller has to close the heap_mutex
 *	 - The function expects a valid page mapping for the
 *	   used entry and of the header of the new
 *	   free entry 
 *
 * Return value:
 *	 0 - Successful
 *	-1 - Failed
 *
 */
static int lib_split_free_entry(block_t *block, size_t sz)
{
	/* Are we able to split? */
	if (block->size <= (sz +sizeof(block_t)))
	{
		/* No. So just set it as used */
		
		/* Do we need to re-allocate page frames? */
		if (block->status == LIB_BLOCKSTAT_FREE_AND_EMPTY)
		{
			unsigned l__pages = sz / ARCH_PAGE_SIZE;
			if (sz % ARCH_PAGE_SIZE) l__pages ++;
			
			/* Allocate new pages */
			hysys_alloc_pages((void*)block->data, l__pages);
			if (*tls_errno) return -1;
		}
		
		lib_remove_from_fbl(block);
		
		/* No. Just reconfigure the entry */
		block->status = LIB_BLOCKSTAT_USED;
				
		lib_add_to_ubl(block);
	}
	 else
	{
		int l__oldstat = block->status;
		
		/* Do we need to re-allocate page frames? */
		if (block->status == LIB_BLOCKSTAT_FREE_AND_EMPTY)
		{
			/* 
			 * "sz + sizeof(block_t)" because we need pages for the
			 * header of the new free memory area
			 */
			unsigned l__pages = (sz + sizeof(block_t)) / ARCH_PAGE_SIZE;
			if ((sz + sizeof(block_t)) % ARCH_PAGE_SIZE) l__pages ++;
			uintptr_t l__pstart = block->data;
			
			/* Is the start aligned to a page? */
			if (l__pstart & ARCH_PAGE_OFFSET_MASK)
			{
				/* No. We will start a page later */
				l__pstart = block->data & (~ARCH_PAGE_OFFSET_MASK);
				l__pstart += ARCH_PAGE_SIZE;
			}			
			
			/* Allocate the new pages */
			hysys_alloc_pages((void*)l__pstart, l__pages);
			if (*tls_errno) return -1;
		}			
		
		/* Reconfigure the current entry */
		size_t l__rsize = block->size - sz - sizeof(block_t);
		
		lib_remove_from_fbl(block);
		
		block->status = LIB_BLOCKSTAT_USED;
				
		block->size = sz;
		block->fbl = lib_get_right_fbl(block->size); /* We change the block size! */
		
		lib_add_to_ubl(block);		
		
		/* Create the new block */
		uintptr_t l__newbl_start = block->data + block->size;
		block_t *l__newbl = lib_create_free_entry(l__newbl_start, l__rsize, l__oldstat, block);
		if (l__newbl == NULL)
		{
			*tls_errno = ERR_MEMORY_CORRUPTED;
			return -1;
		}
	}
	
	return 0;
}

/* 
 * lib_grow_heap (sz)
 *
 * Increases the size of the heap using mem_heap_inc
 * and creates a new free memory block.
 *
 * NOTE:
 *	 - The calling function has to close the
 *	   heap_mutex.
 *	 
 * Return value:
 *	Pointer to the new free memory block
 *	NULL, if failed.
 */
static inline block_t* lib_grow_heap(size_t sz)
{
	block_t *l__retval = NULL;
	uintptr_t l__newstart =   (lib_heap_start) 
	                        + (heap_region->usable_pages * ARCH_PAGE_SIZE)
	                        ;
	unsigned l__pages = sz / ARCH_PAGE_SIZE;
	if (sz % ARCH_PAGE_SIZE) l__pages ++;

	/* Increase the size of the heap */
	mem_heap_inc(l__pages);
	if (*tls_errno) return NULL;

	/* Create a new free heap entry */
	l__retval = lib_create_free_entry(l__newstart, 
					  (l__pages * ARCH_PAGE_SIZE) - sizeof(block_t), 
					  LIB_BLOCKSTAT_FREE, 
					  lib_last_gbl_entry
					 );
	if (*tls_errno) return NULL;	
	
	return l__retval;
}

/*
 * lib_shrink_heap
 *
 * Reduces the size of the heap by removing
 * the last GBL-entry and its memory using mem_heap_dec.
 * If the GBL will be reduced to zero entries, the function
 * will automatically de-initialize the GBL. The function
 * will remove the last entry from its FBL or the UBL
 * (dependend on its status).
 *
 * NOTE:
 *	 - the calling function has to lock the heap_mutex
 *
 */
static inline void lib_shrink_heap(void)
{
	block_t *l__block = lib_last_gbl_entry;
	unsigned l__pages;
	
	if (l__block == NULL) return;
	if (l__block->gbl_ls.n != NULL)
	{
		/* There is something wrong... */
		*tls_errno = ERR_MEMORY_CORRUPTED;
		return;
	}
	
	/* Calculate the count of pages we have to remove */
	l__pages = (l__block->size + sizeof(block_t)) / ARCH_PAGE_SIZE;
	
	/* Okay, if block is not aligned to the
	 * beginnig of the current page, we will
	 * enhance the previous entry to the end
	 * of the page
	 */	
	block_t *l__tmp_p = l__block->gbl_ls.p;
	
	if ((l__block->start & ARCH_PAGE_OFFSET_MASK) && (l__pages > 0))
	{
		if (l__tmp_p != NULL)
		{
			size_t l__rest =   ARCH_PAGE_SIZE 
			                 - (l__block->start & ARCH_PAGE_OFFSET_MASK);
			l__tmp_p->size += l__rest;
			
			int l__tmpfbl = lib_get_right_fbl(l__tmp_p->size);
			
			/* We change it size, so we have to change its FBL */
			if ((l__tmp_p->status != LIB_BLOCKSTAT_USED) && (l__tmp_p->fbl != l__tmpfbl))
			{
				lib_remove_from_fbl(l__tmp_p);
				l__tmp_p->fbl = l__tmpfbl;
				lib_add_to_fbl(l__tmp_p);
			}
			 else
			{
				l__tmp_p->fbl = l__tmpfbl;
			}
		}
	}
	
	/* We will shrink the GBL/Heap only if it makes sense */
	if (l__pages > 0)
	{
		/* Are we the first enty of the GBL? */
		if (l__tmp_p == NULL)
		{
			/* De-initialize the GBL */
			lib_general_bl = NULL;
			lib_last_gbl_entry = NULL;
		}
	 	 else
		{
			l__tmp_p->gbl_ls.n = NULL;
			lib_last_gbl_entry = l__tmp_p;
		}

		/* Do we need to remove it from a UBL? */
		if (l__block->status == LIB_BLOCKSTAT_USED)
		{
			lib_remove_from_ubl(l__block);
		}
	 	 else
		{
			/* No. We have to get removed from FBL */
			lib_remove_from_fbl(l__block);
		}
	
		/* Shrink the heap */
		mem_heap_dec(l__pages);
	}
	
	return;
}

/*
 * lib_clean_free_entry (block)
 *
 * This function tries to unmap all useless free pages
 * of a free block "block". The function will set
 * the block status to LIB_BLOCKSTAT_FREE_AND_EMPTY
 * if it was possible to remove pages.
 *
 * NOTE:
 *	 - This function expects the "heap_mutex" to be
 *	   closed
 *	 - The parameter "block" has to be correct.
 *	 - The parameter "block" has to be a free entry!
 *
 */
static inline void lib_clean_free_entry(block_t *block)
{
	/* Is it possible to remove the useless page frames? */
	if (block->size >= ARCH_PAGE_SIZE)
	{
		unsigned l__pages = block->size / ARCH_PAGE_SIZE;
		uintptr_t l__pstart = block->data;

		/* Is the start aligned to a page? */
		if (l__pstart & ARCH_PAGE_OFFSET_MASK)
		{
			/* No. We will remove a page less */
			l__pages --;
			l__pstart = block->data & (~ARCH_PAGE_OFFSET_MASK);
			l__pstart += ARCH_PAGE_SIZE;
		}

		/* Is the end address aligned to a page? */
		if (    ((l__pstart + block->size) & ARCH_PAGE_OFFSET_MASK) 
		     && (l__pages > 0)
		   )
		{
			l__pages --;
		}

		if (l__pages > 0)
		{
			/* Remove the pages */
			hysys_unmap(0, (void*)l__pstart, l__pages, UNMAP_COMPLETE);
		}
	}

	block->status = LIB_BLOCKSTAT_FREE_AND_EMPTY;

	return;
}

/*
 * lib_unify_free_entry (block)
 *
 * This function tries to merge a free memory
 * block "block" with the free entries of its GBL-neighbourhood.
 *
 * NOTE:
 *	 - This function expects the "heap_mutex" to be
 *	   closed.
 *	 - The parameter block has to be correct.
 *	 - The parameter block has to be declared as a
 *	   free entry already and should already be a
 *	   part of a FBL
 *
 */
static void lib_unify_free_entry(block_t *block)
{
	block_t *l__newblock = block;
	block_t *l__new_p = block->gbl_ls.p;
	block_t *l__new_n = block->gbl_ls.n;
	size_t l__newsz = block->size;
	int l__newstat = block->status;
	int l__dochg = 0;
	
	block_t *l__old_p = block->gbl_ls.p;
	block_t *l__old_n = block->gbl_ls.n;
		
	/* Can we merge with our previous neighbour? */
	if (l__old_p != NULL)
	{
		if (l__old_p->status != LIB_BLOCKSTAT_USED) 
		{
			/*
			 * It seems so. Our previous neighbour 
			 * will be the new GBL member 
			 */
			
			/* First, remove the prev-entry from the FBL */
			lib_remove_from_fbl(l__old_p);

			/* Merge with our previous neighbour */
			l__newblock = l__old_p;
			l__new_p = l__old_p->gbl_ls.p;
			l__newsz += l__old_p->size + sizeof(block_t);
			
			/* Merge the status */
			if (l__newstat > l__old_p->status) l__newstat = l__old_p->status;

			l__dochg = 1;
		}
	}

	/* Can we merge with our previous neighbour? */
	if (l__old_n != NULL)
	{
		if (l__old_n->status != LIB_BLOCKSTAT_USED)
		{
			/* It seems so. Remove the next neighbour from our FBL */
			lib_remove_from_fbl(l__old_n);

			/* Merge us (or the new GBL member) with the next neighbour */
			l__new_n = l__old_n->gbl_ls.n;
			l__newsz += l__old_n->size + sizeof(block_t);

			/* Merge the status */
			if (l__newstat > l__old_n->status) l__newstat = l__old_n->status;

			l__dochg = 1;			
		}
	}

	/* Changes possible? */
	if (l__dochg)
	{
		/* Remove us from the FBL */
		lib_remove_from_fbl(block);

		/* Create the new, merged entry */
		l__newblock->size = l__newsz;
		l__newblock->gbl_ls.p = l__new_p;
		l__newblock->gbl_ls.n = l__new_n;
		l__newblock->fbl = lib_get_right_fbl(l__newsz);
		l__newblock->status = l__newstat;

		/* Add it to its new FBL */
		lib_add_to_fbl(l__newblock);
		
		/* Are we now the new GBL end? */
		if (l__new_n == NULL)
		{
			lib_last_gbl_entry = l__newblock;
			/* Try to shrink, if possible */
			lib_shrink_heap();
		}
		 else
		{
			/* Change the previous descriptor of new_n */
			l__new_n->gbl_ls.p = l__newblock;

			/* Remove its useless pages, if possible... */
			lib_clean_free_entry(l__newblock);

		}
	}
	
	return;
}

/*
 * lib_find_free_block_within (size)
 *
 * Searches a free block with "size" bytes size within
 * the existing free memory blocks. The function will
 * not increase the size of the heap. The returned
 * block will be converted to a free memory block. If
 * there is not fitting memory block, the function will
 * automatically split this block. The function will
 * fill the new useable memory area with pages, if needed.
 *
 * Errors:
 *	The function will set ERR_INVALID_ARGUMENT
 *	if "size" is invalid.
 *
 * Return value:
 *	Pointer to the searched memory block
 *	NULL, if no block was found
 *
 * NOTE: 
 *	 - The calling function has to lock the heap_mutex.
 *	 
 */
static block_t* lib_find_free_block_within(size_t sz)
{
	block_t *l__retval = NULL;
	block_t *l__bestfit = NULL;
	block_t *l__bl = NULL;
	int l__fbl = lib_get_right_fbl(sz);
	if (l__fbl == -1)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}

	/* Search within the fbls */
	do
	{
		/* Get the right FBL */
		l__bl = lib_free_bl[l__fbl];		

		while (l__bl != NULL)
		{
			/* Is there a table manipulation? */
			if (l__bl->status == LIB_BLOCKSTAT_USED)
			{
				*tls_errno = ERR_MEMORY_CORRUPTED;
				return NULL;
			}

			/* Is this block "exactly" fitting? */
			if (l__bl->size == sz)
			{
				l__retval = l__bl;
				break;
			}

			/* Is it bigger? */
			if (l__bl->size > sz)
			{
				if (l__bestfit == NULL)
				{
					/* Select it as best-fit */
					l__bestfit = l__bl;
				}
				 else
				{
					/* If it is smaller as our current best-fit
					 * we will prefer it as our new best-fit
					 * block now.
					 */
					if (l__bl->size < l__bestfit->size)
						l__bestfit = l__bl;
				}
			}

			l__bl = l__bl->ufbl_ls.n;
		}

		/* Do we have something useful? */
		if ((l__retval != NULL) || (l__bestfit != NULL)) break;

	}while(++ l__fbl < 31);

	/* We have a best-fit block. We will split it, if possible */
	if ((l__retval == NULL) && (l__bestfit != NULL))
	{
		if (lib_split_free_entry(l__bestfit, sz) == 0)
		{
			l__retval = l__bestfit;
		}
		else
		{
			return NULL;
		}
	}
	/* We have an exactly fitting block. We will use it as we have got it */
	 else if ((l__retval != NULL) && (l__bestfit == NULL))
	{
		/* Do we need to fill it with page frames? */
		if (l__retval->status == LIB_BLOCKSTAT_FREE_AND_EMPTY)
		{
			unsigned l__pages = sz / ARCH_PAGE_SIZE;
			if (sz % ARCH_PAGE_SIZE) l__pages ++;
			uintptr_t l__pstart = l__retval->data;
			
			/* Is the start aligned to a page? */
			if (l__pstart & ARCH_PAGE_OFFSET_MASK)
			{
				/* No. We will start a page later */
				l__pstart = l__retval->data & (~ARCH_PAGE_OFFSET_MASK);
				l__pstart += ARCH_PAGE_SIZE;
			}			
			/* Allocate new pages */
			hysys_alloc_pages((void*)l__pstart, l__pages);
			if (*tls_errno) return NULL;
		}		
		
		l__retval->status = LIB_BLOCKSTAT_USED;

		lib_remove_from_fbl(l__retval);
		
		lib_add_to_ubl(l__retval);
	}

	return l__retval;
}

/*
 * lib_find_ubl_block(adr)
 *
 * Find the UBL-block that belongs to the
 * address "adr". The address "adr" have to point to
 * the start address of the memory area.
 *
 * Return value:
 *	Pointer to the block data structure
 *	
 * NOTE:
 *	 - This function expects that the "heap_mutex"
 *	   is already locked.
 *	 - This function will not test the parameter
 *	   "adr".
 */
static inline block_t* lib_find_ubl_block(uintptr_t adr)
{
	block_t *l__block = lib_used_bl;
	
	/* Search the UBL for a block with the same data-address */
	while (l__block != NULL)
	{
		if (l__block->data == adr)
		{
			return l__block;
		}
		
		l__block = l__block->ufbl_ls.n;
	}
	
	return NULL;
}
 
/*
 * mem_alloc (size)
 *
 * Allocates "size" bytes on the heap.
 * (For details on the allocation algorithm
 *  please read the hydrixOS documentation)
 * 
 * Return value:
 *	Pointer to the allocated area
 *	NULL, if failed.
 *
 */
void* mem_alloc(size_t sz)
{
	block_t *l__block = NULL;
	
	mtx_lock(&lib_heap_mutex, -1);
	
	/* Okay, try to find a free block first */
	l__block = lib_find_free_block_within(sz);

	if (*tls_errno)
	{
		mtx_unlock(&lib_heap_mutex);
		return NULL;
	}
	
	/* Nothing found? Increase the heap and try again. */
	if (l__block == NULL) 
	{
		l__block = lib_grow_heap(sz + sizeof(block_t));
		if ((*tls_errno) || (l__block == NULL))
		{
			mtx_unlock(&lib_heap_mutex);
			return NULL;
		}
	
		if (lib_split_free_entry(l__block, sz) == -1)
		{
			mtx_unlock(&lib_heap_mutex);
			return NULL;
		}
	}
	
	l__block->status = LIB_BLOCKSTAT_USED;
	
	mtx_unlock(&lib_heap_mutex);
	return (void*)l__block->data;
}

/*
 * mem_free(ptr)
 *
 * Removes the allocated memory area that
 * begins at the position where "ptr" points
 * to from the heap and frees its memory.
 *
 */
void mem_free(void* mem)
{
	block_t *l__block = NULL;

	mtx_lock(&lib_heap_mutex, -1);

	if (mem == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		mtx_unlock(&lib_heap_mutex);
		return;
	}

	/* Find the block */
	l__block = lib_find_ubl_block((uintptr_t)mem);
	if (l__block == NULL) 
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		mtx_unlock(&lib_heap_mutex);
		return;
	}

	/* Convert it to a free entry */
	l__block->status = LIB_BLOCKSTAT_FREE;
	lib_remove_from_ubl(l__block);

	lib_add_to_fbl(l__block);

	/* Is it at the end of the heap? */
	if (l__block == lib_last_gbl_entry)
	{
		lib_shrink_heap();
	}
	 else
	{
		/* Try to merge the block */
		lib_unify_free_entry(l__block);	 
		
		/* NOTE: lib_unify_free_entry will shrink the
		 *       heap, if possible
		 */
	}

	mtx_unlock(&lib_heap_mutex);

	return;
}

/*
 * mem_realloc(mem, size)
 *
 * Resizes the memory block that is pointed
 * by "mem" to "size" bytes.
 * If "mem" is "NULL", the function will work
 * like mem_alloc. If "size" is 0, the function
 * will free the memory area.
 *
 * The function tries to expand the current area
 * at is current memory position. If this is not
 * possible, it will move the content to a new
 * memory block with the size "size". If an
 * area should be shrinked content at the end
 * of the memory area can be lost.
 *
 * Return value:
 *	Pointer to the new area of "size" bytes.
 *	NULL if failed.
 *
 */
void* mem_realloc(void* mem, size_t nsz)
{
	void* l__retval = NULL;
	block_t *l__block = NULL;
	size_t l__block_sz = 0;

	/* Will we do nothing? */
	if ((mem == NULL) && (nsz == 0))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}

	/* We just want to allocate a new area */
	if (mem == NULL) return mem_alloc(nsz);

	/* We just want to free the area */
	if (nsz == NULL) {mem_free(mem); return NULL;}

	mtx_lock(&lib_heap_mutex, -1);
	
	/* Get the memory block of "mem" */
	l__block = lib_find_ubl_block((uintptr_t)mem);
	if (l__block == NULL) 
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		mtx_unlock(&lib_heap_mutex);
		return NULL;
	}

	l__block_sz = l__block->size;

	/* Is there a need for resizing it? */
	if (l__block_sz == nsz)
	{
		l__retval = (void*)l__block->data;
		mtx_unlock(&lib_heap_mutex);
		return l__retval;
	}

	/*
	 * Shrink a block
	 * --------------
	 *
	 */
	if (l__block->size > nsz)
	{
		size_t l__rest = l__block->size - nsz;
		block_t *l__newblock = NULL;
		uintptr_t l__newblock_start = 0;

		/* Can we create a new free memory area ? */
		if (l__rest <= sizeof(block_t))
		{
			l__retval = (void*)l__block->data;
			/* No it is too small. No effect. */
			mtx_unlock(&lib_heap_mutex);
			return l__retval;
		}

		l__block->size = nsz;
		l__block->fbl = lib_get_right_fbl(nsz);

		l__rest -= sizeof(block_t);
	
		/* Create the new free block */
		l__newblock_start = l__block->data + nsz;
		l__newblock = lib_create_free_entry(l__newblock_start, 
				  	            l__rest, 
				      		    LIB_BLOCKSTAT_FREE, 
				      		    l__block
				     		   );

		/* Try to unify it with your neighbourhood */
		lib_unify_free_entry(l__newblock);	 

		/* Should we shrink the heap? */
		if (lib_last_gbl_entry == l__newblock)
		{
			lib_shrink_heap();
		}
		 else
		{
			/* Can we clean the entry? */
			lib_clean_free_entry(l__newblock);
		}

		l__retval = (void*)l__block->data;
		mtx_unlock(&lib_heap_mutex);
		return l__retval;
	}
	
	/*
	 * Expand a block
	 * --------------
	 *
	 * Is there a free block in the "upper" neighbourhood, 
	 * we can merge with?
	 */
	if (l__block->gbl_ls.n != NULL)
	{
		block_t *l__tmp_n = l__block->gbl_ls.n;
		
		/* Is it a free area? */
		if (l__tmp_n->status != LIB_BLOCKSTAT_USED)
		{
			size_t l__tmpsz = nsz - l__block->size;
			
			/* It seems to fit exactly */
			if ((l__tmp_n->size + sizeof(block_t)) == l__tmpsz)
			{
				/* Just merge them completly */
				lib_remove_from_fbl(l__tmp_n);
				
				block_t *l__tmpnx = l__tmp_n->gbl_ls.n;
				
				if (l__tmpnx != NULL)
				{
					l__tmpnx->gbl_ls.p = l__block;
				}
				else
				{
					lib_last_gbl_entry = l__block;
				}
				
				l__block->gbl_ls.n = l__tmpnx;
				l__block->size = nsz;
				l__block->fbl = lib_get_right_fbl(l__block->size);
				
				l__retval = (void*)l__block->data;
				mtx_unlock(&lib_heap_mutex);
				return l__retval;
			}
			 /* It is graeter than, so we have to split it */
			 else if (l__tmp_n->size > l__tmpsz)
			{
				if (lib_split_free_entry(l__tmp_n, l__tmpsz) != - 1)
				{
					/* Now we have our fitting entry */
					block_t *l__tmpnx = l__tmp_n->gbl_ls.n;
				
					lib_remove_from_ubl(l__tmp_n);
				
					if (l__tmpnx != NULL)
					{
						l__tmpnx->gbl_ls.p = l__block;
					}
					 else
					{
						lib_last_gbl_entry = l__block;
					}
				
					l__block->gbl_ls.n = l__tmpnx;
					l__block->size = nsz;
					l__block->fbl = lib_get_right_fbl(l__block->size);
				
					l__retval = (void*)l__block->data;
					mtx_unlock(&lib_heap_mutex);
					return l__retval;				
				}
			}
		}
	}

	/*
	 * Okay, it seems we haven't found a fitting block in our
	 * direct environment, so we have to choose another one,
	 * copy us to it and free our old block.
	 *
	 */

	mtx_unlock(&lib_heap_mutex);
	
	l__retval = mem_alloc(nsz);

	if (l__block_sz > nsz) l__block_sz = nsz;
	buf_copy(l__retval, mem, l__block_sz);
	mem_free(mem);

	return l__retval;	
}

/*
 * mem_size(area)
 *
 * Returns the size of a heap element "area"
 * in bytes.
 *
 * Return value:
 * > 0 Size of the element
 * ==0, if failed
 */
size_t mem_size(void *area)
{
	void* l__retval = NULL;
	block_t *l__block = NULL;
	size_t l__block_sz = 0;

	/* Will we do nothing? */
	if (area == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return 0;
	}

	/* Get the memory block of "mem" */
	l__block = lib_find_ubl_block((uintptr_t)area);
	if (l__block == NULL) 
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		mtx_unlock(&lib_heap_mutex);
		return 0;
	}

	return l__block->size;
}