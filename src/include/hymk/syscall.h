/*
 *
 * hymk/syscall.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * System call prototypes
 *
 */
#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <hydrixos/types.h>

/* Memory managment system calls */
void hymk_alloc_pages(void* start, unsigned pages);

/* Subject managment system calls */
sid_t hymk_create_thread(void* eip, void* esp);
sid_t hymk_create_process(void* eip, void* esp);

void hymk_set_controller(sid_t sid);

void hymk_destroy_subject(sid_t sid);

/* Security system calls */
#define CHGROOT_LEAVE		0u
#define CHGROOT_ENTER		1u

void hymk_chg_root(sid_t subj, unsigned mode);

/* Scheduler */
void hymk_freeze_subject(sid_t subj);
void hymk_awake_subject(sid_t subj);

void hymk_yield_thread(sid_t dest);
void hymk_set_priority(sid_t thrd, unsigned priority, unsigned policy);

/* Memory sharing */
void hymk_allow(sid_t dest_sid, 
		sid_t src_sid, 
		void* dest_adr,
		unsigned pages,
		unsigned flags
	       );
#define ALLOW_MAP		1u
#define ALLOW_UNMAP		2u	
#define ALLOW_REVERSE		(ALLOW_MAP | 4u)       
	       
void hymk_map(sid_t dest_sid,
	      void* src_adr,
	      unsigned pages,
	      unsigned flags,
	      uintptr_t dest_offset
	     );
#define MAP_READ		1u
#define MAP_WRITE		2u
#define MAP_EXECUTABLE		4u
#define MAP_COPYONWRITE		8u
#define MAP_PAGED		16u
#define MAP_REVERSE		32u

void hymk_unmap(sid_t dest_sid,
		void* dest_adr,
		unsigned pages,
		unsigned flags
	       );
#define UNMAP_COMPLETE		0u
#define UNMAP_AVAILABLE		1u
#define UNMAP_WRITE		2u
#define UNMAP_EXECUTE		4u
 
	     
void hymk_move(sid_t dest_sid,
	       void* src_adr,
	       unsigned pages,
	       unsigned flags,
	       uintptr_t dest_offset
	      );

/* Synchronization */
sid_t hymk_sync(sid_t other, unsigned timeout, unsigned resyncs);
    
/* Input / Output */
void hymk_io_allow(sid_t dest, unsigned flags);
#define IO_ALLOW_IRQ		1u
#define IO_ALLOW_PORTS		2u

void hymk_io_alloc(uintptr_t src, 
		  void* dest, 
		  unsigned pages, 
		  unsigned flags
		 );
#define IOMAP_READ		1u
#define IOMAP_WRITE		2u
#define IOMAP_EXECUTE		4u
#define IOMAP_WITH_CACHE	8u

void hymk_recv_irq(irq_t irq);
#define RECV_IRQ_NONE		0xFFFFFFFFu		 

/* Remote control of threads */
unsigned hymk_recv_softints(sid_t sid, unsigned timeout, unsigned flags);
#define RECV_AWAKE_OTHER	1u
#define RECV_TRACE_SYSCALL	2u

/* x86: Count of registers readable with the read_regs system call */
#define SYSTEM_REGREAD_COUNT	4u

/* x86: Register structure */
typedef struct 
{
	uint32_t regs[SYSTEM_REGREAD_COUNT];	/* Register values */
}reg_t;

reg_t hymk_read_regs(sid_t subj, unsigned tp);
void hymk_write_regs(sid_t subj, unsigned tp, reg_t rv);
#define REGS_X86_GENERIC	0u
#define REGS_X86_INDEX		1u
#define REGS_X86_POINTERS	2u
#define REGS_X86_EFLAGS		3u
#define REGS_X86_BREAKPOINTS	4u
#define REGS_X86_DEBUG_SETUP	5u

/* Paging Daemon */
void hymk_set_paged(void);
unsigned hymk_test_page(uintptr_t adr, sid_t sid);

#define PGA_READ		1u
#define PGA_WRITE		2u
#define PGA_EXECUTE		4u

#endif
