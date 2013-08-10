/*
 *
 * io.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying').
 *
 * Input/Output system calls
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
#include <page.h>
#include <sysc.h>

int kio_current_io = 0xFFFFFFFF;
uint32_t *kio_last_thread = NULL;

/*
 * IRQ Handlers structure
 *
 */
struct {
	sid_t	tid;
	int	is_handling;
}irq_handlers_s[16] 
= 
	{
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0}
	};

/*
 * sysc_io_allow(dest, flags)
 *
 * (Implementation of the "io_allow" system call)
 *
 * Allows a thread to access on a certain I/O resource
 * managed by the kernel. This system call may be only
 * called by root processes.
 *
 * Parameters:
 *	dest	SID of the affected process
 *	flags	The I/O operations that should be allowed:
 *			IO_ALLOW_IRQ	Allow the handling of IRQs
 *			IO_ALLOW_PORTS	Allow the access on the I/O-Ports
 *
 */
void sysc_io_allow(sid_t dest, unsigned flags)
{
	/* Is the current process a root process? */
	if (!current_p[PRCTAB_IS_ROOT])
	{
		SET_ERROR(ERR_NOT_ROOT);
		return;
	}
	
	/* Does the destination SID define a valid process? */
	if (!kinfo_isproc(dest))
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
	
	/* Proceed only, if the flags are valid */
	if (    (flags & IO_ALLOW_IRQ)
	     || (flags & IO_ALLOW_PORTS)
	   )
	{
		/* Add the flags */
		PROCESS(dest, PRCTAB_IO_ACCESS_RIGHTS) |= flags;
		
		return;
	}
	
	/* Invalid flags */
	SET_ERROR(ERR_INVALID_ARGUMENT);
	return;
}

/*
 * sysc_io_alloc(src, dest, pages, flags)
 *
 * (Implementation of the "io_alloc" system call)
 *
 * Maps a defined hardware memory area into the current
 * virtual address space. This system call may be only called
 * by root processes.
 *
 * Parameters:
 *	src	Physical address of the hardware memory area
 *	dest	Virtual address of the mapping
 *	pages	Number of pages to map
 *	flags	Controlling flags:
 *			IOMAP_READ	It is allowed to read from the area
 *			IOMAP_WRITE	It is allowed to write to the area
 *			IOMAP_EXECUTE	It is allowed to execute the area
 *			IOMAP_WITH_CACHE The CPU-Cache is activated for this
 *					 area.
 *
 */
void sysc_io_alloc(uintptr_t src, 
		  uintptr_t dest, 
		  unsigned long pages, 
		  unsigned flags
		 )
{
	unsigned l__flags = GENFLAG_PRESENT | GENFLAG_USER_MODE;
	uintptr_t l__psrc = src + (pages * 4096);
	
	/* Test the execution restriction */
	if (pages > MEM_MAX_PAGE_OP_NUM)	
	{
		SET_ERROR(ERR_SYSCALL_RESTRICTED);
		return;
	}		
	
	/* Is it a root process */
	if (!current_p[PRCTAB_IS_ROOT])
	{
		SET_ERROR(ERR_NOT_ROOT);
		return;
	}

	/* Is the source area between the "middle part" of the RAM ? */
	if (    (src > 0x1FFFFF)
	     || ((l__psrc) > 0x1FFFFF)
	   )
	{
		if (    (src < (total_mem_size + 0x200000))
		     || ((l__psrc) < (total_mem_size + 0x200000))
		   )
		{
			SET_ERROR(ERR_INVALID_ADDRESS);
			return;
		}
	}
	
	/* Calculate the flags */
	if (flags & IOMAP_READ) l__flags |= GENFLAG_READABLE;
	if (flags & IOMAP_WRITE) l__flags |= GENFLAG_WRITABLE;
	if (!(flags & IOMAP_WITH_CACHE)) l__flags |= GENFLAG_NOT_CACHEABLE;
	
	flags |= GENFLAG_DONT_OVERWRITE_SETTINGS;
	
	/* Map the area */
	kmem_map_page_frame_cur(src, 
		  	    	dest, 
			    	pages,
			    	l__flags
			       );
			       			      
	return;
}

/*
 * kio_reenable_irq(irq)
 *
 * Reenables an IRQ that was handled by a thread and
 * shouldn't be handled again.
 *
 */
void kio_reenable_irq(irq_t irq)
{
	/*
	 * Disable all IRQs after handling
	 *
	 * (However we don't want to disable
	 *  the RTC IRQ, because we need it)
	 *
	 */
	if (irq != 0) 
		ksched_disable_irq(irq);
		
	/* Exit IRQ handling by releasing the IRQ */	
	current_t[THRTAB_IRQ_RECV_NUMBER] = 0xFFFFFFFF;
		
	irq_handlers_s[irq].tid = 0;
	irq_handlers_s[irq].is_handling = 0;
		
	return ;
}
	
/*
 * sysc_recv_irq(irq)
 *
 * (Implementation of the "recv_irq" system call)
 *
 * Let the current thread wait for the occurence of a selected
 * IRQ. If the IRQ will occure the thread will be started to handle
 * it. After handling it has to call 'recv_irq' again to handle
 * other IRQs. To quit the IRQ handling mode it has to call
 * 'recv_irq' with the invalid IRQ number 0xFFFFFFFF.
 *
 * Parameters:
 *	irq	Number of the IRQ that should be handled
 *
 */
void sysc_recv_irq(irq_t irq)
{
	if (!(current_p[PRCTAB_IO_ACCESS_RIGHTS] & IO_ALLOW_IRQ))
	{
		SET_ERROR(ERR_ACCESS_DENIED);
		return;
	}
		
	/* Exit IRQ handling */
	if (irq == 0xFFFFFFFF) 
	{
		/* No IRQ handled. Just return. */
		if (current_t[THRTAB_IRQ_RECV_NUMBER] == 0xFFFFFFFF) 
			return;

		/* Re-enable IRQ */
		kio_reenable_irq(current_t[THRTAB_IRQ_RECV_NUMBER]);
		
		/* Sleep a moment */
		current_t[THRTAB_EFFECTIVE_PRIORITY] = 0;
		current_t[THRTAB_THRSTAT_FLAGS] &= (~THRSTAT_IRQ_HANDLING);	
		
		KSCHED_TRY_RESCHED();
		return;
	}

	/* Don't handle invalid IRQs */
	if (irq > 15)
	{
		SET_ERROR(ERR_INVALID_ARGUMENT);
		return;
	}

	/* Is the thread already in IRQ handling mode? */
	if (    (current_t[THRTAB_IRQ_RECV_NUMBER] != irq)
	     || (current_t[THRTAB_IRQ_RECV_NUMBER] != 0xFFFFFFFF)
	   )
	{
		/* Exit current IRQ handling */
		current_t[THRTAB_THRSTAT_FLAGS] &= (~THRSTAT_IRQ_HANDLING);
		kio_reenable_irq(current_t[THRTAB_IRQ_RECV_NUMBER]);
	}
	
	/* Is the IRQ already in use? */
	if (
		(irq_handlers_s[irq].tid != 0) 
	     && (irq_handlers_s[irq].tid != current_t[THRTAB_SID])
	   )
	{
		SET_ERROR(ERR_RESOURCE_BUSY);
		return;
	}
		
	/* Enter IRQ handling procedure */		
	irq_handlers_s[irq].tid = current_t[THRTAB_SID];
	irq_handlers_s[irq].is_handling = 0;
	current_t[THRTAB_IRQ_RECV_NUMBER] = irq;
	current_t[THRTAB_EFFECTIVE_PRIORITY] = IRQ_THREAD_PRIORITY;
	
	ksched_stop_thread(current_t);
	current_t[THRTAB_THRSTAT_FLAGS] |= THRSTAT_IRQ;
	
	/* Enable the IRQ */
	ksched_enable_irq(irq);
		
	/* Switch the current thread */
	ksched_change_thread = true;
	ksched_next_thread();
	
	return;	
}

/*
 * ksched_start_irq_handler(irqn)
 *
 * Starts an IRQ handling thread, if an IRQ arrives for it.
 *
 *
 */
void ksched_start_irq_handler(irq_t irqn)
{
	/* Is the IRQ in use? */
	if (irq_handlers_s[irqn].tid == 0) return;
	
	/* Is it allready handled? */
	if (irq_handlers_s[irqn].is_handling == 1)
		return;
	
	/* Handle it */
	irq_handlers_s[irqn].is_handling = 1;
	
	/* Disable the IRQ (staying enabled if RTC-IRQ) */
	if (irqn != 0) ksched_disable_irq(irqn);
	
	/* Handle the IRQ */
	THREAD(irq_handlers_s[irqn].tid, THRTAB_THRSTAT_FLAGS) &= (~THRSTAT_IRQ);
	THREAD(irq_handlers_s[irqn].tid, THRTAB_THRSTAT_FLAGS) |= THRSTAT_IRQ_HANDLING;
	
	ksched_start_thread(&THREAD(irq_handlers_s[irqn].tid, 0));
	
	/* Thread change */
	ksched_change_thread = true;
	return;
}
