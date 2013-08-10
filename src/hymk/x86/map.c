/*
 *
 * map.c
 *
 * (C)2004 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying').
 *
 * Mapping functions
 *
 */
#include <hydrixos/types.h>
#include <stdio.h>
#include <mem.h>
#include <info.h>
#include <error.h>
#include <setup.h>
#include <sched.h>
#include <current.h>
#include <sysc.h>
#include <page.h>

/*
 * sysc_allow(dest_sid, src_sid, dest_adr, pages, flags)
 *
 * (Implementation of the "allow" system call)
 *
 * Allows another thread or a group of SIDs to execute
 * a memory operation on the current thread. Root thread
 * are allowed to execute the allow-Operation on other
 * threads.
 *
 * Parameters:
 *	dest_sid	SID that will be allowed to
 *			execute a memory operation
 *				INVALID	Use the access list
 *				NULL	Lock the access completely
 *	src_sid		SID of the thread that is affected
 *			by the allow operation (use INVALID
 *			to select the current thread)
 *	dest_adr	The destination area that can be used
 *			by the memory operation
 *	pages		The number of pages that may be affected
 *	flags		Flags:
 *				ALLOW_MAP	Allow the map operation
 *				ALLOW_UNMAP	Allow the unmap operation
 *
 */
void sysc_allow(sid_t dest_sid, 
		sid_t src_sid, 
		uintptr_t dest_adr,
		unsigned long pages,
		unsigned flags
	       )
{
	/* Transform src_sid == invalid if needed */
	if (    (src_sid == SID_PLACEHOLDER_INVALID)
	     || (src_sid == SID_PLACEHOLDER_NULL)
	     || (src_sid == 0)
	   )
	{
		src_sid = current_t[THRTAB_SID];
	}
	 else
	{
		if (!kinfo_isthrd(src_sid))
		{
			SET_ERROR(ERR_INVALID_SID);
			return;
		}
	}
	
	/* Is the calling thread root or the same thread? */
	if (   (!current_p[PRCTAB_IS_ROOT])
	    && (current_p[PRCTAB_SID] != THREAD(src_sid, THRTAB_PROCESS_SID))
	   )
	{
		SET_ERROR(ERR_ACCESS_DENIED);
		return;
	}
		
	/* Round up the destination address */
	dest_adr &= (~0x3ff);
	
	/* It is not allowed to make the kernel address space accessable */
	if (	(((dest_adr + (pages * 4096)) <= dest_adr) && (dest_adr > 0) && (pages > 0))
	     || ((dest_adr + (pages * 4096)) >= VAS_KERNEL_START)
	   )
	{
		SET_ERROR(ERR_INVALID_ADDRESS);
		return;
	}
		
	/* Set the flags only if they are correct */
	if (    (flags & ALLOW_MAP)
	     || (flags & ALLOW_UNMAP)
	     || (flags & ALLOW_REVERSE)
	     || (flags == 0)
	   )
	{	
		THREAD(src_sid, THRTAB_MEMORY_OP_SID) = dest_sid;
		THREAD(src_sid, THRTAB_MEMORY_OP_DESTADR) = dest_adr & (~0xFFFu);
		THREAD(src_sid, THRTAB_MEMORY_OP_MAXSIZE) = pages;
		THREAD(src_sid, THRTAB_MEMORY_OP_ALLOWED) = flags;
		
		return;
	}
	 
	SET_ERROR(ERR_INVALID_ARGUMENT);
	return;
}

/*
 * sysc_map(dest_sid, src_adr, pages, flags)
 *
 * (Implementation of the "map" system call)
 *
 * Maps a memory area of the current virtual address space
 * into an enabled memory area of the virtual address space 
 * of another thread.
 *
 * Parameters:
 *	dest_sid	SID of the affected thread
 *	src_adr		The start address of the memory area
 *			within the current address space
 *	pages		Number of pages that should be mapped
 *	flags		Flags:
 *				MAP_READ	The area will be readable
 *				MAP_WRITE	The area will be writeable
 *				MAP_EXECUTE	The area will be executable
 *				MAP_COPYONWRITE The area will be selcted for
 *						copy-on-write
 *
 */
void sysc_map(sid_t dest_sid,
	      uintptr_t src_adr,
	      unsigned long pages,
	      unsigned flags,
	      uintptr_t dest_offset
	     )
{
	uint32_t *l__dest = NULL;	/* Dest Thread */
	uint32_t *l__pdir_s = NULL;	/* Source PDIR */
	uint32_t *l__pdir_d = NULL;	/* Dest PDIR */
	uintptr_t l__dest_adr = 0;	/* Dest address */
	uint32_t *l__pstat_d = NULL;	/* Dest PSTAT */	
	uint16_t l__flags = 0;		/* Calculated flags */
	sid_t l__src_psid = current_t[THRTAB_PROCESS_SID];	/* Process of the source address space */
	sid_t l__dest_psid;					/* Process of the dest. address space */
		
	dest_offset &= (~0xfff);	/* Rounding offset address to pages */
		
	/* dest_sid = "null", means current thread */
	if ((dest_sid == 0) || (dest_sid == SID_PLACEHOLDER_INVALID))
	{
		dest_sid = current_t[THRTAB_SID];
	}
	
	/*
	 * Testing & preparing
	 *
	 */
	/* Test the execution restriction */
	if (pages > MEM_MAX_PAGE_OP_NUM)	
	{
		SET_ERROR(ERR_SYSCALL_RESTRICTED);
		return;
	}	
	
	/* Round up the source address */
	src_adr &= (~0xFFFu);
	 
	/* Is the used source address valid? */
	if (	((src_adr + (pages * 4096)) <= src_adr)
	     || ((src_adr + (pages * 4096)) >= VAS_KERNEL_START)
	   )
	{
		SET_ERROR(ERR_INVALID_ADDRESS);
		return;
	}
	
	/* Are the selected flags valid? */
	if (flags & (~(unsigned)(MAP_READ | MAP_WRITE | MAP_EXECUTABLE | MAP_COPYONWRITE | MAP_REVERSE | MAP_PAGED)))
	{
		SET_ERROR(ERR_INVALID_ARGUMENT);
		return;
	}
	
	/* Is the destination SID valid? */
	if (!kinfo_isthrd(dest_sid))
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
	
	l__dest = &THREAD(dest_sid, 0);
	l__dest_psid = current_t[THRTAB_PROCESS_SID];
	
	/* Are we allowed to map into this address space? */
	if (    (l__dest[THRTAB_MEMORY_OP_SID] != current_t[THRTAB_SID])
	     && (l__dest[THRTAB_MEMORY_OP_SID] != current_p[PRCTAB_SID])
	     && (l__dest[THRTAB_MEMORY_OP_SID] != SID_USER_EVERYBODY)
	     && (    (l__dest[THRTAB_MEMORY_OP_SID] == SID_USER_ROOT)
	          && (current_p[PRCTAB_IS_ROOT] == 0)
		)
	     && (l__dest[THRTAB_SID] != current_t[THRTAB_SID])
	    )
	{
		SET_ERROR(ERR_ACCESS_DENIED);
		return;
	}
	    
	/* Is the destination area big enough? */
	if (    ((l__dest[THRTAB_MEMORY_OP_MAXSIZE]) < (pages + (dest_offset / 4096)))
	     || ((pages + (dest_offset / 4096)) < pages)
	   )
	{
		SET_ERROR(ERR_ACCESS_DENIED);
		return;
	}
	
	/* Is the wanted operation allowed? */
	if (    (!(l__dest[THRTAB_MEMORY_OP_ALLOWED] & ALLOW_MAP))
	     || ((!(l__dest[THRTAB_MEMORY_OP_ALLOWED] & ALLOW_REVERSE)) && (flags & MAP_REVERSE))
	   )
	{
		SET_ERROR(ERR_ACCESS_DENIED);
		return;
	}
	
	/* Get the page directories */
	l__pdir_s = (void*)(uintptr_t)
			current_p[PRCTAB_PAGEDIR_PHYSICAL_ADDR];
	l__pdir_d = (void*)(uintptr_t)
			PROCESS(l__dest[THRTAB_PROCESS_SID],
				PRCTAB_PAGEDIR_PHYSICAL_ADDR
			       );
	l__pstat_d = &PROCESS(l__dest[THRTAB_PROCESS_SID],
			      PRCTAB_X86_MMTABLE
			     );
			       
	l__dest_adr = l__dest[THRTAB_MEMORY_OP_DESTADR] + dest_offset;

	/* Setup flags */
	l__flags = GENFLAG_PRESENT | GENFLAG_USER_MODE;
	
	if (flags & MAP_READ) l__flags |= GENFLAG_READABLE;
	if (flags & MAP_WRITE) l__flags |= GENFLAG_WRITABLE;
	if (flags & MAP_EXECUTABLE) l__flags |= GENFLAG_EXECUTABLE;
	if (flags & MAP_PAGED) l__flags |= GENFLAG_PAGED_PROTECTED;
				       
	/* Setting MAP_PAGED is only allowed for the PageD */
	if ((flags & MAP_PAGED) && (!current_p[PRCTAB_IS_PAGED]))
	{
		SET_ERROR(ERR_PAGING_DAEMON);
		return;
	}
	
	/* Reverse mapping operation */
	if (flags & MAP_REVERSE)
	{
		/* Switch dest and source */
		uint32_t *l__pdir_tmp_s = l__pdir_s;
		uintptr_t l__dest_tmp_adr = l__dest_adr;
		l__pstat_d = &current_p[PRCTAB_X86_MMTABLE];
		sid_t l__dest_psid_tmp = l__dest_psid;
		l__dest_psid = l__src_psid;
		l__src_psid = l__dest_psid_tmp;
		
		l__pdir_s = l__pdir_d;
		l__pdir_d = l__pdir_tmp_s;
		l__dest_adr = src_adr;
		src_adr = l__dest_tmp_adr;
		
		
	}	
	
	/*
	 * Page mapping operation
	 *
	 */
	/* Initialize some variables */
	uint32_t l__pd_offs_s = 0xFFFFFFFF;	/* Local page directory offset (src) */
	uint32_t l__pd_offs_d = 0xFFFFFFFF;	/* Local page directory offset (dest) */
	
	uint32_t *l__ptab_s = NULL;	/* Page table of the source process */
	uint32_t *l__ptab_d = NULL;	/* Page table of the dest process */

	while (pages --)
	{
		/* 
		 * Reload PDIR offsets, Page table ptrs etc. 
		 *
		 */
		/* Source space */
		if ((src_adr / (4096 * 1024)) != l__pd_offs_s)
		{
			l__pd_offs_s = (src_adr / (4096 * 1024));
			
			l__ptab_s = (void*)(uintptr_t)
				(l__pdir_s[l__pd_offs_s] & (~0xFFFu));
			
			/* Empty page table, override 1024 pages */
			if (l__ptab_s == NULL)
			{
				if (pages < 1024)
				{
					break;
				}
				 else
				{
					pages -= 1024;
					src_adr += 1024 * 4096;
					l__dest_adr += 1024 * 4096;
					continue;
				}
					
			}
		
		}		 
		 
		/* Dest space */
		if ((l__dest_adr / (4096 * 1024)) != l__pd_offs_d)
		{
			l__pd_offs_d = (l__dest_adr / (4096 * 1024));
			
			l__ptab_d = kmem_get_table(l__pdir_d, 
				    		   l__dest_adr,
				       		   1
				      	    	  );
		}
	

		/*
		 * Map the pages 
		 *
		 */	
		uint32_t l__ptb_offs_s = (src_adr / 4096) & 0x3FF;
		uint32_t l__ptb_offs_d = (l__dest_adr / 4096) & 0x3FF;

		/* Map only from active pages; Map only to inactive pages */
		if (	(l__ptab_s[l__ptb_offs_s] != 0) 
		     && (l__ptab_d[l__ptb_offs_d] == 0)
		   )
		{
			uint32_t l__entry = l__ptab_s[l__ptb_offs_s];
				
			/* 
			 * Is this page marked for copy on write,
			 * but MAP_COPYONWRITE is not set? 
			 */
			if ((l__entry & GENFLAG_DO_COPYONWRITE) && (!(flags & MAP_COPYONWRITE)))
			{
				/* This means that we have to copy the page before sharing it */
				if (kmem_do_copy_on_write(l__pdir_s, l__src_psid, src_adr))
				{
					MSYNC();
					SET_ERROR(ERR_PAGES_LOCKED);
					return;
				}
				
				/* Refresh l__entry now */
				MSYNC();
				l__entry = l__ptab_s[l__ptb_offs_s];
			}
				
			/* 
			 * This will effect, that no missing flags of the page 
			 * can be set using map 
			 */
			l__entry =   (l__entry & (~0xfffu))
				   | (l__entry & l__flags);
		
			/* But root is able to set such missing flags */
			if (current_p[PRCTAB_IS_ROOT])		
			{
				l__entry |= l__flags;
			}

			uintptr_t l__padr = l__entry & (~0xfffu);				
			
			if (    (l__entry & GENFLAG_PRESENT)
			     && (l__padr > 1024*1024)
			     && (l__padr < (page_buffer_start + page_buffer_sz))
			   )
			{

				/* Get a new PST entry */
				sidlist_t *l__pst = page_share_table_free;
				if (l__pst == NULL)
				{
					SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
					return;
				}
				page_share_table_free = page_share_table_free->nxt;
				
				/* Add the new user as page frame owner */
				if (PAGE_BUF_TAB(l__padr).usage == 1)
				{
					/* Create a new list, so we need 
					 * another free PST entry first
					 */
					sidlist_t *l__sndpst = page_share_table_free;
					if (l__sndpst == NULL)
					{
						page_share_table_free = l__pst;
						SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
						return;
					}
					page_share_table_free = page_share_table_free->nxt;
					
					/* Copy datas from the single-entry */
					l__sndpst->user.pid = PAGE_BUF_TAB(l__padr).owner.single.pid;
					l__sndpst->user.u_adr = PAGE_BUF_TAB(l__padr).owner.single.u_adr;
					l__sndpst->nxt = l__pst;
					
					/* Setup the list */
					PAGE_BUF_TAB(l__padr).owner.multi = l__sndpst;
				}
				
				/* Just create the new element */
				l__pst->user.pid = l__dest_psid;
				l__pst->user.u_adr = l__dest_adr;
				l__pst->nxt = PAGE_BUF_TAB(l__padr).owner.multi;
				PAGE_BUF_TAB(l__padr).owner.multi = l__pst;		
						
				/* Change the usage counter */
				PAGE_BUF_TAB(l__padr).usage ++;
			}

			/*
			 * Change mapping informations
			 *
			 */
			if (    (!(l__ptab_d[l__ptb_offs_d] & GENFLAG_PRESENT))
			     && (l__entry & GENFLAG_PRESENT)
			   )
			{
				/* Change the page usage statistic */
				l__pstat_d[l__pd_offs_d] ++;
			}			
			
			/* Select page frame for copy-on-write */						
			if (flags & MAP_COPYONWRITE)
			{
				l__entry |= GENFLAG_DO_COPYONWRITE;
				l__entry &= (~GENFLAG_WRITABLE);	
						
				/* COW will also affect the source frame */
				l__ptab_s[l__ptb_offs_s] = l__entry;

				INVLPG(src_adr);
			}
						
			/* Map the page frame */
			l__ptab_d[l__ptb_offs_d] = l__entry;
			
			/* Invalidate TLB, if needed */
			if (l__pdir_d == i386_current_pdir) INVLPG(l__dest_adr);
		}
				
		l__dest_adr += 4096;
		src_adr += 4096;
	}
	 
	return;
}

/*
 * sysc_unmap(dest_sid, dest_adr, pages, flags)
 *
 * (Implementation of the "unmap" system call)
 *
 * Unmaps a memory area that is part of an enabled memory
 * area of another thread or the current thread.
 *
 * Parameters:
 *	dest_sid	SID of the affected thread
 *	dest_adr	The start address of the memory area
 *			within the current address space
 *	pages		Number of pages that should be unmapped
 *	flags		Flags:
 *			UNMAP_COMPLETE		Completly remove the area
 *			UNMAP_WRITE		Make the area Read-Only
 *			UNMAP_EXECUTABLE	Make the area Not-Exec
 *			UNMAP_AVAILABLE		Clear the present flag
 *
 */
void sysc_unmap(sid_t dest_sid,
		uintptr_t dest_adr,
		unsigned long pages,
		unsigned flags
	       )
{
	uint32_t *l__dest = NULL;	/* Dest Thread */
		
	/*
	 * Testing
	 *
	 */
	/* Test the execution restriction */
	if (pages > MEM_MAX_PAGE_OP_NUM)	
	{
		SET_ERROR(ERR_SYSCALL_RESTRICTED);
		return;
	}	 
	 
	/* Are the operation flags valid? */
	if (flags > UNMAP_EXECUTE)
	{
		SET_ERROR(ERR_INVALID_ARGUMENT);
		return;
	}
	
	if (    (dest_sid == SID_PLACEHOLDER_NULL)
	     || (dest_sid == SID_PLACEHOLDER_INVALID)
	   )
	{
		dest_sid = current_t[THRTAB_SID];
	}
	
	/* Is the destination thread valid? */
	if (!kinfo_isthrd(dest_sid))
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
	
	l__dest = &THREAD(dest_sid, 0);
	
	if (l__dest != current_t)
	{
		/*
		 * Unmap to another thread
		 *
		 */
		/* Are we allowed to access, in general? */
		if (    (   l__dest[THRTAB_MEMORY_OP_SID] 
			 != current_t[THRTAB_SID]
			)
	     	     && (   l__dest[THRTAB_MEMORY_OP_SID] 
		     	 != current_p[PRCTAB_SID]
			)
	     	     && (   l__dest[THRTAB_MEMORY_OP_SID] 
		     	 != SID_USER_EVERYBODY)
	     	     && (    (   l__dest[THRTAB_MEMORY_OP_SID] 
		     	      == SID_USER_ROOT
			     )
	          	  && (current_p[PRCTAB_IS_ROOT] == 0)
			)
		    )
		{
			SET_ERROR(ERR_ACCESS_DENIED);
			return;
		}

		/* Are we allowed to unmap? */
		if (!(l__dest[THRTAB_MEMORY_OP_ALLOWED] & ALLOW_UNMAP))
		{
			SET_ERROR(ERR_ACCESS_DENIED);
			return;
		}
		  	
		/* Is the destination address valid? */
		if (    (dest_adr < l__dest[THRTAB_MEMORY_OP_DESTADR])
	     	     || (   (   ((    dest_adr 
		                   -  l__dest[THRTAB_MEMORY_OP_DESTADR]) 
			         / 4096
				)
			     + pages
		            )
		          > l__dest[THRTAB_MEMORY_OP_MAXSIZE]
		        )
	   	   )
		{
			SET_ERROR(ERR_INVALID_ADDRESS);
			return;
		}
	}
	 else
	{
		/* 
		 * Unmap to current thread
		 *
		 */
		/* Is the used destination address valid? */
		if (	((dest_adr + (pages * 4096)) <= dest_adr)
	     	     || ((dest_adr + (pages * 4096)) >= VAS_KERNEL_START)
	   	   )
		{
			SET_ERROR(ERR_INVALID_ADDRESS);
			return;
		}		
	} 
	 
	/*
	 * Preparing
	 *
	 */
	uint32_t *l__pdir_d = NULL;	/* Dest PDIR */
	uint32_t *l__pstat_d = NULL;	/* Dest PSTAT */
	uint32_t l__flagmask = 0;	
	
	l__pdir_d = (void*)(uintptr_t)
			PROCESS(l__dest[THRTAB_PROCESS_SID],
				PRCTAB_PAGEDIR_PHYSICAL_ADDR
			       );
	l__pstat_d = &PROCESS(l__dest[THRTAB_PROCESS_SID],
			      PRCTAB_X86_MMTABLE
			     );
	
	/* Create flag mask */
	if (flags == UNMAP_COMPLETE)
		l__flagmask = 0xFFFFFFFF;
	
	if (flags == UNMAP_AVAILABLE)
		l__flagmask = GENFLAG_READABLE;
	
	if (flags == UNMAP_WRITE)
		l__flagmask = GENFLAG_WRITABLE | GENFLAG_EXECUTABLE;
		
	/*if (flags == UNMAP_EXECUTE)
		l__flagmask = GENFLAG_EXECUTABLE;*/
	
	/* Unmapping GENFLAG_EXECUTABLE makes currently no sense */
	if (flags == UNMAP_EXECUTE)
	{
		return;
	}
			     
	/*
	 * Unmapping
	 *
	 */
	uint32_t l__pd_offs_d = 0xFFFFFFFF;

	uint32_t *l__ptab_d = NULL;
	
	while (pages --)
	{
		/* 
		 * Reload PDIR offsets, Page table ptrs etc. 
		 *
		 */
		 
		/* Dest space */
		if ((dest_adr / (4096 * 1024)) != l__pd_offs_d)
		{
			l__pd_offs_d = (dest_adr / (4096 * 1024));
			
			l__ptab_d = kmem_get_table(l__pdir_d, 
				    		   dest_adr,
				       		   0
				      	    	  );
			
			if (l__ptab_d == NULL)
			{
				if (pages < 1024)
				{
					break;
				}
				 else
				{
					pages -= 1024;
					dest_adr += 1024 * 4096;
					continue;
				}
			}
		}

		/*
		 * Unmaping operation
		 *
		 */	
		uint32_t l__ptb_offs_d = (dest_adr / 4096) & 0x3FF;
				
		/*
		 * Free the page frame, if never needed
		 *
		 */
		if (    (     (flags == UNMAP_COMPLETE)
		           || (flags == UNMAP_AVAILABLE)
		        ) 
		     && (l__ptab_d[l__ptb_offs_d] & GENFLAG_PRESENT)
		   )
		{
			/* Exception if try to unmap a PageD protected page */
			if (    (flags == UNMAP_COMPLETE) 
			     && (l__ptab_d[l__ptb_offs_d] & GENFLAG_PAGED_PROTECTED)
			     && (!current_p[PRCTAB_IS_PAGED])
			   )
			{
				if (kpaged_send_pagefault(0xABCD, 0, 0, dest_adr))
				{
					SET_ERROR(ERR_PAGING_DAEMON);
					return;
				}
			}

			kmem_free_user_pageframe(l__ptab_d[l__ptb_offs_d],
						 l__dest[THRTAB_PROCESS_SID],
						 dest_adr
						);
			
			if (flags == UNMAP_COMPLETE)
				l__ptab_d[l__ptb_offs_d] = 0;
			else
				l__ptab_d[l__ptb_offs_d] &= ~GENFLAG_PRESENT;
			
			/* Actualize page status counter */
			l__pstat_d[dest_adr / (4096 * 1024)] --;
			
			/* Free the page table, if never needed */
			if (l__pstat_d[dest_adr / (4096 * 1024)] == 0)
			{
				kmem_free_kernel_pageframe(l__ptab_d);
				l__pdir_d[dest_adr / (4096 * 1024)] = 0;
			}
		}

		/* Unmap the pages */
		l__ptab_d[l__ptb_offs_d] &= (~l__flagmask);

		/* Invalidate TLB, if needed */		
		if (l__pdir_d == i386_current_pdir) INVLPG(dest_adr);
		
		dest_adr += 4096;
	}		           
	
	return;
}
		
	       
/*
 * sysc_move(dest_sid, src_adr, pages, flags)
 *
 * (Implementation of the "move" system call)
 *
 * Maps an area into another enabled virtual address space and
 * removes it from the current virtual address space.
 *
 * Parameters:
 *	dest_sid	SID of the affected thread
 *	src_adr		The start address of the memory area
 *			within the current address space
 *	pages		Number of pages that should be moved
 *	flags		Flags:
 *				MAP_READ	The area will be readable
 *				MAP_WRITE	The area will be writeable
 *				MAP_EXECUTE	The area will be executable
 *
 */
void sysc_move(sid_t dest_sid,
	       uintptr_t src_adr,
	       unsigned long pages,
	       unsigned flags,
	       uintptr_t dest_offset
	      )
{
	/* Test the execution restriction */
	if (pages > MEM_MAX_PAGE_OP_NUM)	
	{
		SET_ERROR(ERR_SYSCALL_RESTRICTED);
		return;
	}	
	
	sysc_map(dest_sid, src_adr, pages, flags, dest_offset);

	if (sysc_error) return;
		
	sysc_unmap(current_t[THRTAB_SID], src_adr, pages, UNMAP_COMPLETE);
	
	return;
}
