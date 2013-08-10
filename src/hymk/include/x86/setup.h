/*
 *
 * setup.h
 *
 * (C)2004 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Kernel settings
 *
 */
#ifndef _SETUP_H
#define _SETUP_H

/*
 * Write kernel debug messages to screen
 *
 */
#define	DEBUG_MODE

/*
 * Stop execution if a user mode exception occurs
 *
 */
//#define STRONG_DEBUG_MODE

#ifdef STRONG_DEBUG_MODE
	#ifndef DEBUG_MODE
		#define DEBUG_MODE
	#endif
#endif

/*
 * Current HydrixOS Version
 *
 */
#define HYMK_VERSION_MAJOR		0
#define HYMK_VERSION_MINOR		0
#define HYMK_VERSION_REVISION		2

/*
 * CPU-Type: at least a Pentium
 *
 */
#define HYMK_VERSION_CPUID		0x80586

/*
 * Memory geometry 
 *
 */
#define NORMAL_ZONE_END			(896 * 1024 * 1024)
#define HIGH_ZONE_END			(4096 * 1024 * 1024)

#define MEM_MAX_PAGE_OP_NUM		((8 * 1024 * 1024) / 4096)

#define PST_PERCENT			20
/*
 * Size of a Kernel stack
 *
 */
#define KERNEL_STACK_PAGES		(1)
#define KERNEL_STACK_SIZE		(KERNEL_STACK_PAGES * 4096)

/*
 * Scheduling priorities and classes
 *
 */
#define SCHED_PRIORITY_MAX		40
#define SCHED_PRIORITY_MIN		0
#define SCHED_CLASS_MAX			0
#define SCHED_CLASS_MIN			0

/*
 * Frequency of the timer in Hz 
 *
 * At moment we try to work with 1 kHz.
 * 
 */
#define TIMER_FREQUENCY				1000

/* IRQ THREAD PRIORITY */
#define IRQ_THREAD_PRIORITY			1000

#endif
