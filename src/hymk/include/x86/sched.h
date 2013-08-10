/*
 *
 * sched.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Scheduling helper functions
 *
 */
#ifndef _SCHED_H
#define _SCHED_H

#include <hydrixos/types.h>

/*
 * i386 TSS Managment
 *
 */
/* Structure of a GDT entry */
extern struct task_gdt_s {
	unsigned int	limit_l : 16,
			base_l : 16,
			base_lh : 8,
			type: 8,
			limit_h: 4,
			access: 4,
			base_h:8;
}i386_gdt_s[8];

/* Structure of a TSS */	
extern struct i386_tss_ps {
	uint16_t	backl, backl_e;
	uint32_t	esp0;
	uint16_t	ss0, ss0_e;
	uint32_t	esp1;
	uint16_t	ss1, ss1_e;
	uint32_t	esp2;
	uint16_t	ss2, ss2_e;	
	uint32_t	cr3;
	uint32_t	eip;
	uint32_t	eflags;
	uint32_t	eax;
	uint32_t	ecx;
	uint32_t	edx;
	uint32_t	ebx;
	uint32_t	esp;	
	uint32_t	ebp;
	uint32_t	esi;
	uint32_t	edi;	
	uint16_t	es, es_e;
	uint16_t	cs, cs_e;	
	uint16_t	ss, ss_e;	
	uint16_t	ds, ds_e;
	uint16_t	fs, fs_e;		
	uint16_t	gs, gs_e;
	uint16_t	ldt, ldt_e;		
	uint16_t	t, io_base;
}*i386_tss_struct;

typedef struct {
	unsigned int	offs_l 		:16,
			sel 		:16,
		        wcount		:8,
			access		:8,
			offs_h		:16;
}idt_t;

/* Structure of an IDT entry */
extern idt_t i386_idt_s[256];

int ksched_init_tss();

/*
 * Kernel stack managment
 *
 */
void* ksched_new_stack(void);
void ksched_del_stack(void* frame);

uintptr_t ksched_init_stack(uintptr_t nsp, 
		      	    uintptr_t eip,
		      	    uintptr_t esp,
		      	    int mode
		      	   );
/* Thread is in kernel mode */
#define	KSCHED_KERNEL_MODE	0		
/* Thread is in user mode */
#define	KSCHED_USER_MODE	1		

/*
 * Interrupt managment
 *
 *
 */
/* Kernel IRQ table */
extern struct ksched_irqt_ps {
		sid_t		pid;		/* SID of receiving process */
		sid_t		tid;		/* SID of receiving thread */
		long		used;		/* used = 1; unused = 0; unuseable = -1 */
}ksched_irqt_s[16];

int ksched_init_ints(void);

/* 
 * These functions controls the PIC
 *
 */
void ksched_enable_irq(irq_t irqnr);
void ksched_disable_irq(irq_t irqnr);

/*
 * This assembler function will handle the interrupts:
 *
 */
void i386_irqhandleasm_0(void);
void i386_irqhandleasm_1(void);
void i386_irqhandleasm_2(void);
void i386_irqhandleasm_3(void);
void i386_irqhandleasm_4(void);
void i386_irqhandleasm_5(void);
void i386_irqhandleasm_6(void);
void i386_irqhandleasm_7(void);
void i386_irqhandleasm_8(void);
void i386_irqhandleasm_9(void);
void i386_irqhandleasm_10(void);
void i386_irqhandleasm_11(void);
void i386_irqhandleasm_12(void);
void i386_irqhandleasm_13(void);
void i386_irqhandleasm_14(void);
void i386_irqhandleasm_15(void);

void i386_exhandleasm_0(void);
void i386_exhandleasm_1(void);
void i386_exhandleasm_2(void);
void i386_exhandleasm_3(void);
void i386_exhandleasm_4(void);
void i386_exhandleasm_5(void);
void i386_exhandleasm_6(void);
void i386_exhandleasm_7(void);
void i386_exhandleasm_8(void);
void i386_exhandleasm_9(void);
void i386_exhandleasm_10(void);
void i386_exhandleasm_11(void);
void i386_exhandleasm_12(void);
void i386_exhandleasm_13(void);
void i386_exhandleasm_14(void);
void i386_exhandleasm_15(void);
void i386_exhandleasm_16(void);
void i386_exhandleasm_17(void);
void i386_exhandleasm_18(void);
void i386_exhandleasm_19(void);
void i386_exhandleasm_20(void);
void i386_exhandleasm_21(void);
void i386_exhandleasm_22(void);
void i386_exhandleasm_23(void);
void i386_exhandleasm_24(void);
void i386_exhandleasm_25(void);
void i386_exhandleasm_26(void);
void i386_exhandleasm_27(void);
void i386_exhandleasm_28(void);
void i386_exhandleasm_29(void);
void i386_exhandleasm_30(void);
void i386_exhandleasm_31(void);

void i386_emptyint_handler(void);

/*
 * Higher level IRQ / Exception handlers
 *
 */
void ksched_handle_irq(irq_t irqn);
void ksched_handle_except(void);
void i386_handle_emptyint(unsigned intno);

void ksched_start_irq_handler(irq_t irqn);

void ksched_do_panic(void);

void kio_reenable_irq(irq_t irq);

/* Informations about the current exception */
extern uint32_t i386_saved_error_code;
extern uint32_t i386_saved_error_cs;
extern uint32_t i386_saved_error_eip;
extern uint32_t i386_saved_error_esp;
extern uint32_t i386_saved_error_num;
extern uint32_t i386_error_kernel_esp;

/* Scheduling */
extern uint32_t *i386_new_stack_pointer;
extern uint32_t *i386_old_stack_pointer;

/*
 * Scheduler
 *
 */
#define SCHED_REGULAR			0

/* Count of threads that are ready for execution*/
extern long ksched_active_threads;
extern long ksched_change_thread;	
extern uint32_t *ksched_idle_thread;

int ksched_start_thread(uint32_t *thrd);
int ksched_stop_thread(uint32_t *thrd);

int ksysc_create_idle(void);
void ksched_idle_loop(void);
void ksched_enter_main_loop(void);

/*
 * "Yielding" a thread in kernel mode
 *
 */
void i386_yield_kernel_thread(void);

/*
 * Timeout functions
 *
 */
extern uint32_t *timeout_queue;	/* Timeout queue */
extern uint64_t timeout_next;	/* Next timeout */

void ksched_del_timeout(uint32_t *thr);
void ksched_add_timeout(uint32_t *thr, uint32_t to);

/*
 * Remote access to software interrupts
 *
 */
int kremote_received(uint32_t intr);

/*
 * Thread synchronization
 *
 */
void ksync_interrupt_other(uint32_t *other);
void ksync_removefrom_waitqueue_error(uint32_t *other, uint32_t *me);

#endif

