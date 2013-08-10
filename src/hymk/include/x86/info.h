/*
 *
 * info.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Info page and descriptor managment functions
 *
 */
#ifndef _INFO_H
#define _INFO_H

#include <hydrixos/types.h>
#include <hymk/sysinfo.h>
#include <hydrixos/sid.h>

/* 
 * Address of the page frames of the main info page and
 * the empty subject descriptor
 *
 */
extern uintptr_t real_main_info;
extern uintptr_t real_empty_info;

/*
 * Pointer to the mapped info page areas
 *
 */
extern uint32_t *main_info;
extern uint32_t *process_tab;
extern uint32_t *thread_tab;

extern uint32_t *current_p;		/* current process descr */
extern uint32_t *current_t;		/* current thread descr */
extern uint32_t *kinfo_eff_prior;	/* buffer of the currnt effctv priority*/
extern uint64_t *kinfo_rtc_ctr;		/* buffer of the current RTC ctr */
extern uint32_t *kinfo_io_map;		/* current I/O permission map */

/* Initialization of the info page area */
int kinfo_init_x86_cpu(void);
int kinfo_init_387_fpu(void);

int kinfo_init(void);

#include <hymk/sysinfo.h>

/*
 * CPU features
 *
 */
extern long i386_do_pge;

/*
 * Descriptor managment
 *
 */
#define PROCESS_MAX		4096
#define THREAD_MAX		4096

uint32_t* kinfo_new_descr(sid_t sid);
void kinfo_del_descr(sid_t sid);

/*
 * Use these macros to access to the
 * process or thread table. Please test
 * the process or thread SID with kinfo_isproc
 * resp. kinfo_isthrd before using PROCESS
 * or THREAD.
 *
 */
#define PROCESS(___sid, ___entry)		\
	(process_tab[(((___sid) & 0xFFF) * 1024 * 2) + (___entry)])

#define THREAD(___sid, ___entry)		\
	(thread_tab[(((___sid) & 0xFFF) * 1024 * 2) + (___entry)])

/*
 * kinfo_isproc(sid)
 *
 * Tests if a SID defines a valid process
 *
 * Return:
 *	!= 0	Valid
 *	== 0	Invalid
 *
 */
static inline int kinfo_isproc(sid_t sid)
{
	/* Is it a valid process-SID? */
	if ((sid & SID_TYPE_MASK) != SIDTYPE_PROCESS) return 0;
	if ((sid & SID_DATA_MASK) > PROCESS_MAX) return 0;
	
	/* Is the selected process active? */
	if (!PROCESS(sid, PRCTAB_IS_USED)) return 0;
	
	/* It seems to be a valid process */
	return 1;
}

/*
 * kinfo_isthrd(sid)
 *
 * Tests if a SID defines a valid thread
 *
 * Return:
 *	!= 0	Valid
 *	== 0	Invalid
 *
 */
static inline int kinfo_isthrd(sid_t sid)
{
	/* Is it a valid thread-SID? */
	if ((sid & SID_TYPE_MASK) != SIDTYPE_THREAD) return 0;
	if ((sid & SID_DATA_MASK) > THREAD_MAX) return 0;
	
	/* Is the selected thread existing? */
	if (!THREAD(sid, THRTAB_IS_USED)) return 0;
	
	/* It seems to be a valid thread */
	return 1;
}

#endif

