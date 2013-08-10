/*
 *
 * pmap.c
 *
 * (C)2005 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Managment of the page mapping memory
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
#include <hydrixos/pmap.h>
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
uintptr_t libpmap_pmapheap_start = NULL;

/*
 * The block descriptor type
 *
 */
typedef struct {
	uintptr_t	start;		/* The start address of the block */
	
	unsigned	pages;		/* Page count of the block */
	int		status;		/* Block status (see LIB_BLOCKSTAT_* Macros) */
	int		fbl;		/* FBL number of a block with this size */
	
	list_t		gbl_ls;		/* Links to the general block list */
	list_t		ufbl_ls;	/* Links to the free / used block list (according to status) */
	
}pmapblock_t;

	/* Our block is a free block without pages */
#define	LIBPMAP_BLOCKSTAT_FREE			0
	/* Our block is currently in used as a PMAP memory buffer */
#define LIBPMAP_BLOCKSTAT_USED			1

mtx_t libpmap_pmapheap_mutex = MTX_DEFINE();	/* The block table mutex */

static pmapblock_t *libpmap_general_bl = NULL;			/* The general block list (GBL) */
static pmapblock_t *libpmap_last_gbl_entry = NULL;		/* Last block descriptor of the GBL */

static pmapblock_t *libpmap_used_bl = NULL;			/* The used block list (UBL) */

static pmapblock_t *libpmap_free_bl[20] = 			/* The free block lists (FBL[i]) */
	{
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL
	};


/* The pmap region */
region_t *pmap_region;

/*
 * libpmap_get_right_fbl (pages)
 *
 * Returns the ordinal number of the 
 * FBL for a certain count of pages.
 *
 * Return value:
 *	>= 0	Number of the FBL
 *	<  0	Invalid size
 *
 */
static inline int libpmap_get_right_fbl(unsigned pages)
{
	int l__i = 0;
	unsigned l__startpg = 1;
	
	if (pages == 0) return -1;
	
	/*
	 * The valid size range of a FBL of
	 * the ordinal number i are
	 *
	 * from (2^i) to (2^(i+1) - 1) pages.
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
		unsigned l__endpg = (l__startpg * 2) - 1;
		
		if ((pages >= l__startpg) && (pages <= l__endpg))
			return l__i;
			
		l__startpg *= 2;		
	}while (++ l__i < 31);
	
	return -1;
}


/*
 * lib_init_pmap
 *
 * This function initializes the mapping region. It will
 * lock the libpmap_pmapheap_mutex to ensure that it won't
 * be entered at the same time. 
 *
 * Return value:
 *	0	Success
 *	1	Error
 *
 */
extern uintptr_t lib_heap_start;

int lib_init_pmap(void)
{
	mtx_lock(&libpmap_pmapheap_mutex, -1);
		
	/* Create the pmap region */
	region_t l__region;
	str_copy(l__region.name, "map", 5);
	l__region.id = 4;
	
	l__region.start = (void*)((uintptr_t)heap_region->start + (heap_region->pages * ARCH_PAGE_SIZE));
	l__region.pages = 262144;
	
	l__region.flags =    REGFLAGS_READABLE 
	                   | REGFLAGS_WRITEABLE 
	                  ;
	l__region.usable_pages = 0;
	l__region.readable_pages = 0;
	l__region.writeable_pages = 0;
	l__region.executable_pages = 0;
	l__region.shared_pages = 0;
			
	l__region.alloc = pmap_alloc;
	l__region.free = pmap_free;
			
	l__region.chksum = ((uintptr_t)l__region.start + l__region.pages) * 2;
	l__region.ls.p = NULL;
	l__region.ls.n = NULL;
	
	pmap_region = reg_create(l__region);
	if (pmap_region == NULL) 
	{
		mtx_unlock(&libpmap_pmapheap_mutex);
		return 1;
	}
	
	/* We have to set the start address of the usable heap memory to lib_heap_start */
	lib_heap_start = (((uintptr_t)l__region.start) + ARCH_PAGE_SIZE);	
		
	/* Calculate the start address of the buffers area of the PMAP region */
	libpmap_pmapheap_start = (uintptr_t)pmap_region->start;
	libpmap_pmapheap_start += sizeof(region_t);
	libpmap_pmapheap_start &= (~0xFFF);
	libpmap_pmapheap_start += ARCH_PAGE_SIZE;

	/* Create the initial FBL entry */
	pmapblock_t* l__initfbl = mem_alloc(sizeof(pmapblock_t));
	
	l__initfbl->start = libpmap_pmapheap_start;
	l__initfbl->pages = pmap_region->pages - 1;
	l__initfbl->status = LIBPMAP_BLOCKSTAT_FREE;	
	l__initfbl->fbl = libpmap_get_right_fbl(l__initfbl->pages);
	
	l__initfbl->gbl_ls.p = NULL;
	l__initfbl->gbl_ls.n = NULL;
	
	l__initfbl->ufbl_ls.p = NULL;
	l__initfbl->ufbl_ls.n = NULL;
	
	/* Add it to the FBL */
	libpmap_free_bl[l__initfbl->fbl] = l__initfbl;
	libpmap_last_gbl_entry = l__initfbl;
	
	/* Signalize that the PMAP heap was initialized. */
	mtx_unlock(&libpmap_pmapheap_mutex);
	
	return 0;

}

/*
 * libpmap_add_to_fbl (block)
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
static inline void libpmap_add_to_fbl(pmapblock_t *block)
{
	int l__fbl = 0;
	pmapblock_t *l__tmpbl = NULL;
	
	/* Find the fitting FBL */
	l__fbl = block->fbl;
	
	/* Delete existing bindings */
	block->ufbl_ls.p = NULL;
	block->ufbl_ls.n = NULL;	
	
	/* Add it to the beginning of the FBL */
	l__tmpbl = libpmap_free_bl[l__fbl];
	
	if (l__tmpbl != NULL)
	{
		l__tmpbl->ufbl_ls.p = block;
		block->ufbl_ls.p = NULL;
		block->ufbl_ls.n = l__tmpbl;
	}
	
	libpmap_free_bl[l__fbl] = block;
	
	return;
}

/*
 * libpmap_remove_from_fbl (block)
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
static inline void libpmap_remove_from_fbl(pmapblock_t *block)
{
	pmapblock_t *l__tmp_n = block->ufbl_ls.n;
	pmapblock_t *l__tmp_p = block->ufbl_ls.p;
	
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
		
		libpmap_free_bl[l__fbl] = l__tmp_n;
		
	}
	
}

/*
 * libpmap_create_free_entry(start, n, prev)
 *
 * Creates a free memory entry to the position "start"
 * with the size of "n" pages.
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
static inline pmapblock_t* libpmap_create_free_entry(uintptr_t start, unsigned n, pmapblock_t *prev)
{
	pmapblock_t *l__newbl = mem_alloc(sizeof(pmapblock_t));
	
	/* Initialize the new block descriptor */
	l__newbl->start = start;
	l__newbl->pages = n;
	l__newbl->status = LIBPMAP_BLOCKSTAT_FREE;
	l__newbl->fbl = libpmap_get_right_fbl(n);
	l__newbl->ufbl_ls.n = NULL;
	l__newbl->ufbl_ls.p = NULL;
	
	/* Add it to the GBL */
	if (prev != NULL)
	{
		pmapblock_t *l__tmp = prev->gbl_ls.n;
		
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
			libpmap_last_gbl_entry = l__newbl;
		}
	}
	 else /* Initialize the GBL */
	{
		l__newbl->gbl_ls.p = NULL;
		l__newbl->gbl_ls.n = NULL;
		libpmap_general_bl = l__newbl;
		libpmap_last_gbl_entry = l__newbl;
	}

	/* Add it to the right FBL */
	libpmap_add_to_fbl(l__newbl);

	return l__newbl;
}

/*
 * libpmap_add_to_ubl (block)
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
static inline void libpmap_add_to_ubl(pmapblock_t *block)
{
	/* Delete existing bindings */
	block->ufbl_ls.p = NULL;
	block->ufbl_ls.n = NULL;	
	
	/* Are we the very first UBL block? */
	if (libpmap_used_bl == NULL)
	{
		libpmap_used_bl = block;

		return;
	}
	
	/* Are we the very first address of the UBL? */
	if (libpmap_used_bl->start > block->start)
	{
		block->ufbl_ls.n = libpmap_used_bl;
		block->ufbl_ls.p = NULL;
		libpmap_used_bl->ufbl_ls.p = block;
		libpmap_used_bl = block;
	}
	 else
	{
		/*
	 	 * Okay, we are somewhere with in the UBL, so we use
	 	 * the GBL to find our previous UBL entry as fast as 
	 	 * possible
	 	 */
		pmapblock_t *l__block = block->gbl_ls.p;
		pmapblock_t *l__prevubl = NULL;
	
		while(l__block != NULL)
		{	
			if (    (l__block->start < block->start) 
		     	     && (l__block->status == LIBPMAP_BLOCKSTAT_USED)
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
			pmapblock_t *l__tmp = block->ufbl_ls.n;
			l__tmp->ufbl_ls.p = block;
		}
	}
	
	return;
}

/*
 * libpmap_remove_from_ubl (block)
 *
 * Removes the used entry "block" from the UBL. If the UBL
 * will get empty, the function will de-initialize the UBL.
 *
 * NOTE: - The caller has to lock the heap_mutex
 *	 - The parameter "block" hast to be correct
 *
 */
static inline void libpmap_remove_from_ubl(pmapblock_t *block)
{
	/* Are we at the beginnig of the UBL? */
	if (block == libpmap_used_bl)
	{
		pmapblock_t *l__tmpbl = block->ufbl_ls.n;
		
		if (l__tmpbl != NULL)
		{
			/* Set our next entry as first entry */
			libpmap_used_bl = l__tmpbl;
			l__tmpbl->ufbl_ls.p = NULL;
		}
		 else
		{
			/* De-initialize the UBL */
			libpmap_used_bl = NULL;
		}
	}
	 else
	{
		/* We are somewhere within the UBL */
		pmapblock_t *l__tmpbl_n = block->ufbl_ls.n;
		pmapblock_t *l__tmpbl_p = block->ufbl_ls.p;
		
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
 * libpmap_split_free_entry (block, n)
 *
 * Splits an free entry "block" in to a locked entry and a
 * free entry. The locked entry will have "n" pages.
 * If there is no rest, the function won't split the entry.
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
static inline int libpmap_split_free_entry(pmapblock_t *block, unsigned n)
{
	/* Are we able to split? */
	if (block->pages == n)
	{
		/* No. So just set it as used */
		libpmap_remove_from_fbl(block);
		
		block->status = LIBPMAP_BLOCKSTAT_USED;
				
		libpmap_add_to_ubl(block);
	}
	 else
	{
		/* Reconfigure the current entry */
		unsigned l__rsize = block->pages - n;
		
		libpmap_remove_from_fbl(block);
		
		block->status = LIBPMAP_BLOCKSTAT_USED;
				
		block->pages = n;
		block->fbl = libpmap_get_right_fbl(block->pages); /* We change the block size! */
		
		libpmap_add_to_ubl(block);		
		
		/* Create the new block */
		uintptr_t l__newbl_start = block->start + (block->pages * ARCH_PAGE_SIZE);
		pmapblock_t *l__newbl = libpmap_create_free_entry(l__newbl_start, l__rsize, block);
		if (l__newbl == NULL)
		{
			*tls_errno = ERR_MEMORY_CORRUPTED;
			return -1;
		}
	}
	
	return 0;
}

/*
 * libpmap_unify_free_entry (block)
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
static inline void libpmap_unify_free_entry(pmapblock_t *block)
{
	pmapblock_t *l__newblock = block;
	pmapblock_t *l__new_p = block->gbl_ls.p;
	pmapblock_t *l__new_n = block->gbl_ls.n;
	unsigned l__newpg = block->pages;
	int l__dochg = 0;
	
	pmapblock_t *l__old_p = block->gbl_ls.p;
	pmapblock_t *l__old_n = block->gbl_ls.n;
		
	/* Can we merge with our previous neighbour? */
	if (l__old_p != NULL)
	{
		if (l__old_p->status != LIBPMAP_BLOCKSTAT_USED) 
		{
			/*
			 * It seems so. Our previous neighbour 
			 * will be the new GBL member 
			 */
			
			/* First, remove the prev-entry from the FBL */
			libpmap_remove_from_fbl(l__old_p);

			/* Merge with our previous neighbour */
			l__newblock = l__old_p;
			l__new_p = l__old_p->gbl_ls.p;
			l__newpg += l__old_p->pages;
			
			/* Free the old block descriptor */
			mem_free(block);

			l__dochg = 1;
		}
	}

	/* Can we merge with our previous neighbour? */
	if (l__old_n != NULL)
	{
		if (l__old_n->status != LIBPMAP_BLOCKSTAT_USED)
		{
			/* It seems so. Remove the next neighbour from our FBL */
			libpmap_remove_from_fbl(l__old_n);

			/* Merge us (or the new GBL member) with the next neighbour */
			l__new_n = l__old_n->gbl_ls.n;
			l__newpg += l__old_n->pages;

			/* Free its descriptor */
			mem_free(l__old_n);

			l__dochg = 1;			
		}
	}

	/* Changes possible? */
	if (l__dochg)
	{
		/* Remove us from the FBL */
		libpmap_remove_from_fbl(block);

		/* Create the new, merged entry */
		l__newblock->pages = l__newpg;
		l__newblock->gbl_ls.p = l__new_p;
		l__newblock->gbl_ls.n = l__new_n;
		l__newblock->fbl = libpmap_get_right_fbl(l__newpg);
		l__newblock->status = LIBPMAP_BLOCKSTAT_FREE;

		/* Add it to its new FBL */
		libpmap_add_to_fbl(l__newblock);
		
		/* Are we now the new GBL end? */
		if (l__new_n == NULL)
		{
			libpmap_last_gbl_entry = l__newblock;
		}
		 else
		{
			/* Change the previous descriptor of new_n */
			l__new_n->gbl_ls.p = l__newblock;

		}
	}
	
	return;
}

/*
 * libpmap_find_free_block_within (n)
 *
 * Searches a free block with "n" pages size within
 * the existing free memory blocks. The function will
 * not increase the size of the heap. The returned
 * block will be converted to a free memory block. If
 * there is not fitting memory block, the function will
 * automatically split this block. The function will
 * fill the new usable memory area with pages, if needed.
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
static inline pmapblock_t* libpmap_find_free_block_within(unsigned n)
{
	pmapblock_t *l__retval = NULL;
	pmapblock_t *l__bestfit = NULL;
	pmapblock_t *l__bl = NULL;
	int l__fbl = libpmap_get_right_fbl(n);
	if (l__fbl == -1)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}

	/* Search within the fbls */
	do
	{
		/* Get the right FBL */
		l__bl = libpmap_free_bl[l__fbl];		

		while (l__bl != NULL)
		{
			/* Is there a table manipulation? */
			if (l__bl->status == LIBPMAP_BLOCKSTAT_USED)
			{
				*tls_errno = ERR_MEMORY_CORRUPTED;
				return NULL;
			}

			/* Is this block "exactly" fitting? */
			if (l__bl->pages == n)
			{
				l__retval = l__bl;
				break;
			}

			/* Is it bigger? */
			if (l__bl->pages > n)
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
					if (l__bl->pages < l__bestfit->pages)
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
		if (libpmap_split_free_entry(l__bestfit, n) == 0)
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
		l__retval->status = LIBPMAP_BLOCKSTAT_USED;

		libpmap_remove_from_fbl(l__retval);
		
		libpmap_add_to_ubl(l__retval);
	}

	return l__retval;
}

/*
 * libpmap_find_ubl_block(adr)
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
static inline pmapblock_t* libpmap_find_ubl_block(uintptr_t adr)
{
	pmapblock_t *l__block = libpmap_used_bl;
	
	/* Search the UBL for a block with the same data-address */
	while (l__block != NULL)
	{
		if (l__block->start == adr)
		{
			return l__block;
		}
		
		l__block = l__block->ufbl_ls.n;
	}
	
	return NULL;
}

/*
 * pmap_alloc (size)
 *
 * Allocates "size" bytes within the pmap region.
 * The parameter size will be rounded to "pages".
 *
 * (For details on the allocation algorithm
 *  please read the hydrixOS documentation)
 * 
 * Return value:
 *	Pointer to the allocated area
 *	NULL, if failed.
 *
 */
void* pmap_alloc(size_t sz)
{
	pmapblock_t *l__block = NULL;
	unsigned l__pg = sz / ARCH_PAGE_SIZE;
	if (sz % ARCH_PAGE_SIZE) l__pg ++;
	
	mtx_lock(&libpmap_pmapheap_mutex, -1);
	
	/* Okay, try to find a free block first */
	l__block = libpmap_find_free_block_within(l__pg);

	if (*tls_errno)
	{
		mtx_unlock(&libpmap_pmapheap_mutex);
		return NULL;
	}
	
	/* Nothing found... */
	if (l__block == NULL) 
	{
		*tls_errno = ERR_NOT_ENOUGH_MEMORY;
		mtx_unlock(&libpmap_pmapheap_mutex);
		return NULL;
	}
	
	l__block->status = LIBPMAP_BLOCKSTAT_USED;
	
	mtx_unlock(&libpmap_pmapheap_mutex);

	return (void*)l__block->start;
}

/*
 * pmap_free(ptr)
 *
 * Removes the allocated memory area that
 * begins at the position where "ptr" points
 * to from the heap and frees its memory.
 *
 */
void pmap_free(void* mem)
{
	pmapblock_t *l__block = NULL;

	mtx_lock(&libpmap_pmapheap_mutex, -1);

	if (mem == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		mtx_unlock(&libpmap_pmapheap_mutex);
		return;
	}

	/* Find the block */
	l__block = libpmap_find_ubl_block((uintptr_t)mem);
	if (l__block == NULL) 
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		mtx_unlock(&libpmap_pmapheap_mutex);
		return;
	}

	/* Convert it to a free entry */
	l__block->status = LIBPMAP_BLOCKSTAT_FREE;
	libpmap_remove_from_ubl(l__block);

	/* Remove its pages */

	hymk_unmap(0, (void*)l__block->start, l__block->pages, UNMAP_COMPLETE);

	libpmap_add_to_fbl(l__block);

	/* Try to merge the block */
	libpmap_unify_free_entry(l__block);	 

	mtx_unlock(&libpmap_pmapheap_mutex);

	return;
}

/*
 * pmap_mapalloc(size sz)
 *
 * Allocates a memory area of "sz" bytes 
 * within the pmap region. "sz" will rounded
 * automatically to pages. The function will
 * use pmap_alloc to allocate the area.
 *
 * Return value:
 *	Pointer to the allocated area
 *	NULL, if failed.
 *
 */
void* pmap_mapalloc(size_t sz)
{
	void* l__mem = pmap_alloc(sz);
	pmapblock_t *l__block = NULL;
	
	/* Allocation failed? */
	if (l__mem == NULL) return NULL;
	
	/* Allocate the pages into the area */
	l__block = libpmap_find_ubl_block((uintptr_t)l__mem);
	
	hymk_alloc_pages(l__mem, l__block->pages);
	if (*tls_errno) return NULL;
	
	return l__mem;
}
