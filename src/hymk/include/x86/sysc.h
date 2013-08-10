/*
 *
 * sysc.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * System call prototypes, macros and helper functions 
 *
 */
#ifndef _SYSC_H
#define _SYSC_H

#include <hydrixos/types.h>
#include <sched.h>

#define KFIRST_SYSCALL			0xC0u
#define KLAST_SYSCALL			0xD5u

extern errno_t syscall_error;

/* Memory managment system calls */
void sysc_alloc_pages(uintptr_t start, unsigned long pages);

/* Subject managment system calls */
sid_t sysc_create_thread(uintptr_t eip, uintptr_t esp);
sid_t sysc_create_process(uintptr_t eip, uintptr_t esp);

int ksysc_create_init(void);

void sysc_set_controller(sid_t sid);

void sysc_destroy_subject(sid_t sid);

/* Security system calls */
#define CHGROOT_LEAVE		0u
#define CHGROOT_ENTER		1u

void sysc_chg_access(sid_t proc, int op);

/* Scheduler */
void sysc_freeze_subject(sid_t subj);
void sysc_awake_subject(sid_t subj);

void sysc_yield_thread(sid_t dest);
void sysc_set_priority(sid_t thrd, unsigned priority, unsigned policy);

/* Memory sharing */
void sysc_allow(sid_t dest_sid, 
		sid_t src_sid, 
		uintptr_t dest_adr,
		unsigned long pages,
		unsigned flags
	       );
#define ALLOW_MAP		1u
#define ALLOW_UNMAP		2u
#define ALLOW_REVERSE		(ALLOW_MAP | 4u)	       
	       
void sysc_map(sid_t dest_sid,
	      uintptr_t src_adr,
	      unsigned long pages,
	      unsigned flags,
	      uintptr_t dest_offset
	     );
#define MAP_READ		1u
#define MAP_WRITE		2u
#define MAP_EXECUTABLE		4u
#define MAP_COPYONWRITE		8u
#define MAP_PAGED		16u
#define MAP_REVERSE		32u

void sysc_unmap(sid_t dest_sid,
		uintptr_t dest_adr,
		unsigned long pages,
		unsigned flags
	       );
#define UNMAP_COMPLETE		0u
#define UNMAP_AVAILABLE		1u
#define UNMAP_WRITE		2u
#define UNMAP_EXECUTE		4u
 
	     
void sysc_move(sid_t dest_sid,
	       uintptr_t src_adr,
	       unsigned long pages,
	       unsigned flags,
	       uintptr_t dest_offset
	      );

/* Synchronization */
sid_t sysc_sync(sid_t other, unsigned timeout, unsigned resyncs);

/* Security */
void sysc_chg_root(sid_t proc, int op);
#define CHGROOT_ENTER		1u
#define CHGROOT_LEAVE		0u

 
/* Input / Output */
void sysc_io_allow(sid_t dest, unsigned flags);
#define IO_ALLOW_IRQ		1u
#define IO_ALLOW_PORTS		2u

void sysc_io_alloc(uintptr_t src, 
		  uintptr_t dest, 
		  unsigned long pages, 
		  unsigned flags
		 );
#define IOMAP_READ		1u
#define IOMAP_WRITE		2u
#define IOMAP_EXECUTE		4u
#define IOMAP_WITH_CACHE	8u

void sysc_recv_irq(irq_t irq);
#define RECV_IRQ_NONE		0xFFFFFFFFu		 

/* Remote control of threads */
uint32_t sysc_recv_softints(sid_t sid, unsigned timeout, int flags);
#define RECV_AWAKE_OTHER	1u
#define RECV_TRACE_SYSCALL	2u

/*
 * These functions will directly manipulate the registers
 * of the calling thread to pass their return values.
 *
 */
void sysc_read_regs(sid_t sid, unsigned regtype);
void sysc_write_regs(sid_t sid, 
		     unsigned regtype, 
		     uint32_t reg_a,
		     uint32_t reg_b,
		     uint32_t reg_c,
		     uint32_t reg_d
		    );
#define REGS_X86_GENERIC	0u
#define REGS_X86_INDEX		1u
#define REGS_X86_POINTERS	2u
#define REGS_X86_EFLAGS		3u
#define REGS_X86_BREAKPOINTS	4u
#define REGS_X86_DEBUG_SETUP	5u

/* Paging Daemon */
void sysc_set_paged(void);
unsigned sysc_test_page(uintptr_t adr, sid_t sid);

#define PGA_READ		1u
#define PGA_WRITE		2u
#define PGA_EXECUTE		4u

/*
 * Low-Level implementations of the system calls
 *
 */
void i386_sysc_alloc_pages(void);

void i386_sysc_create_thread(void);
void i386_sysc_create_process(void);
void i386_sysc_set_controller(void);
void i386_sysc_destroy_subject(void);

void i386_sysc_chg_root(void);

void i386_sysc_freeze_subject(void);
void i386_sysc_awake_subject(void);
void i386_sysc_yield_thread(void);
void i386_sysc_set_priority(void);

void i386_sysc_allow(void);
void i386_sysc_map(void);
void i386_sysc_unmap(void);
void i386_sysc_move(void);

void i386_sysc_sync(void);

void i386_sysc_io_allow(void);
void i386_sysc_io_alloc(void);
void i386_sysc_recv_irq(void);

void i386_sysc_recv_softints(void);
void i386_sysc_decv_softints(void);
void i386_sysc_read_regs(void);
void i386_sysc_write_regs(void);

void i386_sysc_set_paged(void);
void i386_sysc_test_page(void);


#endif

