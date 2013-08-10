/*
 *
 * system.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * Encapsulation of plattform-dependend kernel interfaces
 *
 */
#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <hydrixos/types.h>
#include <hydrixos/sid.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/hymk.h>
#include <hymk/sysinfo.h>

/*
 * x86-specific Encapsulation
 *
 */
#ifdef HYDRIXOS_x86

/* Memory synchronization */
#define HYSYS_MSYNC()                  asm volatile("add $0, (%%esp)":::"memory")


/*
 * Info page access functions
 *
 */
static inline uint32_t hysys_info_read(unsigned int num)
{
	uint32_t *l__info = (void*)(uintptr_t)ARCH_MAIN_INFO_PAGE;
	
	/* Is it a valid info page entry? */
	if (num > ARCH_MAIN_INFO_SIZE)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return 0;
	}
	
	return l__info[num];
}

static inline uint32_t hysys_prctab_read(sid_t sid, unsigned int num)
{
	uint32_t *l__ptab = (void*)(uintptr_t)ARCH_PROC_TABLE;
	unsigned int l__proc = sid & SID_DATA_MASK;
	
	/* Is it a valid element of a proc table entry? */
	if (num > ARCH_PROC_TABLE_SIZE)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return 0;
	}
	
	/* Is it a valid SID? */
	if (    (l__proc > ARCH_PROC_TABLE_ENTRIES) 
	     || ((sid & SID_TYPE_MASK) != SIDTYPE_PROCESS)
	   )
	{
		*tls_errno = ERR_INVALID_SID;
		return 0;
	}
	
	
	/* Search the right PTAB entry */
	l__ptab += (ARCH_PROC_TABLE_SIZE * l__proc);	
	
	/* Is it a valid proc table entry? */
	if (l__ptab[PRCTAB_IS_USED] == 0)
	{
		*tls_errno = ERR_INVALID_SID;
		return 0;
	}
	
	return l__ptab[num];
}

static inline uint32_t hysys_thrtab_read(sid_t sid, unsigned int num)
{
	uint32_t *l__ttab = (void*)(uintptr_t)ARCH_THREAD_TABLE;
	unsigned int l__thrd = sid & SID_DATA_MASK;
	
	/* Is it a valid element of a proc table entry? */
	if (num > ARCH_THREAD_TABLE_SIZE)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return 0;
	}
	
	/* Is it a valid SID? */
	if (    (l__thrd > ARCH_THREAD_TABLE_ENTRIES) 
	     || ((sid & SID_TYPE_MASK) != SIDTYPE_THREAD)
	   )
	{
		*tls_errno = ERR_INVALID_SID;
		return 0;
	}
		
	/* Search the right TTAB entry */
	l__ttab += (ARCH_THREAD_TABLE_SIZE * l__thrd);	
		
	/* Is it a valid proc table entry? */
	if (l__ttab[THRTAB_IS_USED] == 0)
	{
		*tls_errno = ERR_INVALID_SID;
		return 0;
	}
	
	return l__ttab[num];
}
#endif

/*
 * Implementation-independend syscall bindings
 *
 */
static inline void hysys_alloc_pages(void* adr, unsigned pages)
{
	/* Get the implementation-specific maximum size */
	unsigned l__maxsz = hysys_info_read(MAININFO_MAX_PAGE_OPERATION);
	unsigned l__cnt = 0;
		
	/* No problem. Do it directly */
	if (pages <= l__maxsz) 
	{
		hymk_alloc_pages(adr, pages);
		return;
	}

	l__cnt = pages / l__maxsz;
	
	while(l__cnt --)
	{
		hymk_alloc_pages(adr, l__maxsz);
		if (*tls_errno) return;
		adr += l__maxsz * ARCH_PAGE_SIZE;
	}
	
	if (pages % l__maxsz) hymk_alloc_pages(adr, pages % l__maxsz);	
	
	return;
}

static inline void hysys_map(sid_t subj, void* adr, unsigned pages, unsigned flags, uintptr_t dest_offset)
{
	/* Get the implementation-specific maximum size */
	unsigned l__maxsz = hysys_info_read(MAININFO_MAX_PAGE_OPERATION);
	unsigned l__cnt = 0;
		
	/* No problem. Do it directly */
	if (pages <= l__maxsz)
	{
		 hymk_map(subj, adr, pages, flags, dest_offset);
	 
		 return;
	}
	
	l__cnt = pages / l__maxsz;
	
	while(l__cnt --)
	{
		hymk_map(subj, adr, l__maxsz, flags, dest_offset);
		if (*tls_errno) return;
		adr += l__maxsz * ARCH_PAGE_SIZE;
		dest_offset += l__maxsz * ARCH_PAGE_SIZE;
	}
	
	if (pages % l__maxsz) hymk_map(subj, adr, pages % l__maxsz, flags, dest_offset);
	
	return;
}

static inline void hysys_unmap(sid_t subj, void* adr, unsigned pages, unsigned flags)
{
	/* Get the implementation-specific maximum size */
	unsigned l__maxsz = hysys_info_read(MAININFO_MAX_PAGE_OPERATION);
	unsigned l__cnt = 0;
		
	/* No problem. Do it directly */
	if (pages <= l__maxsz) 
	{
		hymk_unmap(subj, adr, pages, flags);
		return;
	}
	
	l__cnt = pages / l__maxsz;
	
	while(l__cnt --)
	{
		hymk_unmap(subj, adr, l__maxsz, flags);
		if (*tls_errno) return;
		adr += l__maxsz * ARCH_PAGE_SIZE;
	}
	
	if (pages % l__maxsz) hymk_unmap(subj, adr, pages % l__maxsz, flags);
	
	return;
}

static inline void hysys_move(sid_t subj, void* adr, unsigned pages, unsigned flags, uintptr_t dest_offset)
{
	/* Get the implementation-specific maximum size */
	unsigned l__maxsz = hysys_info_read(MAININFO_MAX_PAGE_OPERATION);
	unsigned l__cnt = 0;
		
	/* No problem. Do it directly */
	if (pages <= l__maxsz)
	{
		hymk_move(subj, adr, pages, flags, dest_offset);
		return;
	}
	
	l__cnt = pages / l__maxsz;
	
	while(l__cnt --)
	{
		hymk_move(subj, adr, l__maxsz, flags, dest_offset);
		if (*tls_errno) return;
		adr += l__maxsz * ARCH_PAGE_SIZE;
		dest_offset += l__maxsz * ARCH_PAGE_SIZE;
	}
	
	if (pages % l__maxsz) hymk_move(subj, adr, pages % l__maxsz, flags, dest_offset);
	
	return;
}

static inline void hysys_io_alloc(uintptr_t phys, void* adr, unsigned pages, unsigned flags)
{
	/* Get the implementation-specific maximum size */
	unsigned l__maxsz = hysys_info_read(MAININFO_MAX_PAGE_OPERATION);
	unsigned l__cnt = 0;
		
	/* No problem. Do it directly */
	if (pages <= l__maxsz)
	{
		hymk_io_alloc(phys, adr, pages, flags);
		return;
	}
	
	l__cnt = pages / l__maxsz;
	
	while(l__cnt --)
	{
		hymk_io_alloc(phys, adr, l__maxsz, flags);
		if (*tls_errno) return;
		adr += l__maxsz * ARCH_PAGE_SIZE;
	}
	
	if (pages % l__maxsz) hymk_io_alloc(phys, adr, pages % l__maxsz, flags);
	
	return;
}

#endif
