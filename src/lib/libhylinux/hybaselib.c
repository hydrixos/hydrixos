/*
 *
 * hybaselib.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Emulation of the hyBaseLib
 *
 */
#include "types.h"

#include <hydrixos/hysys.h>
#undef HYDRIXOS_USE_STDFUN_ARCH
#include <hydrixos/sid.h>
#include <hydrixos/errno.h>
#include <hydrixos/blthr.h>
#include <hydrixos/hymk.h>
#include <hydrixos/hythr.h>
#include <hydrixos/mem.h>
#include <hydrixos/mutex.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/tls.h>
#include <hydrixos/vfsmem.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOT_A_FUNCTION	{printf("%s can not be emulated.", __FUNCTION__); exit(1);}

errno_t myerrno = 0;
errno_t *tls_errno = &myerrno;


/*
 * Basic library Threads 
 *
 * "blthread"
 *
 */
/* Create a BlThread */
thread_t* blthr_create(void (*start)(thread_t *thr), size_t stack)
{
	NOT_A_FUNCTION;
	return NULL;
}


/* Handler functions of the thread package */
void blthr_cleanup(thread_t *thr)
{
	NOT_A_FUNCTION;
}

void blthr_kill(thread_t *thr)
{
	NOT_A_FUNCTION;
}

void blthr_init(thread_t *thr)
{
	NOT_A_FUNCTION;
}

void blthr_init_arch(void)
{
	NOT_A_FUNCTION;
}

void blthr_awake(thread_t *thr)
{
	NOT_A_FUNCTION;
}

void blthr_freeze(thread_t *thr)
{
	NOT_A_FUNCTION;
}

void blthr_finish(void)
{
	NOT_A_FUNCTION;
}

void blthr_atexit(struct thread_st *thr, void (*func)(struct thread_st *thr))
{
	NOT_A_FUNCTION;
}


/*
 * HyMK System calls
 *
 */
void hymk_alloc_pages(void* adr, int pages)
{
	NOT_A_FUNCTION;
	return;
}

sid_t hymk_create_thread(void* ip, void* sp)
{
	NOT_A_FUNCTION;
	return 0;
}

sid_t hymk_create_process(void* ip, void* sp)
{
	NOT_A_FUNCTION;
	return 0;
}

void hymk_set_controller(sid_t ctl)
{
	NOT_A_FUNCTION;
}

void hymk_destroy_subject(sid_t subj)
{
	NOT_A_FUNCTION;
}

void hymk_chg_root(sid_t subj, int mode)
{
	NOT_A_FUNCTION;
}

void hymk_freeze_subject(sid_t subj)
{
	NOT_A_FUNCTION;
}

void hymk_awake_subject(sid_t subj)
{
	NOT_A_FUNCTION;
}

void hymk_yield_thread(sid_t recv)
{
	NOT_A_FUNCTION;
}

void hymk_set_priority(sid_t subj, int prior, int cls)
{
	NOT_A_FUNCTION;
}

void hymk_allow(sid_t subj, sid_t me, void* adr, int pages, int op)
{
	NOT_A_FUNCTION;
}

void hymk_map(sid_t subj, void* adr, int pages, int flags)
{
	NOT_A_FUNCTION;
}

void hymk_unmap(sid_t subj, void* adr, int pages, int flags)
{
	NOT_A_FUNCTION;
}

void hymk_move(sid_t subj, void* adr, int pages, int flags)
{
	NOT_A_FUNCTION;
}

sid_t hymk_sync(sid_t subj, long tm, int resync)
{
	NOT_A_FUNCTION;
}

void hymk_io_allow(sid_t subj, int flags)
{
	NOT_A_FUNCTION;
}

void hymk_io_alloc(uintptr_t phys, void* adr, int pages, int flags)
{
	NOT_A_FUNCTION;
}

void hymk_recv_irq(int irqn)
{
	NOT_A_FUNCTION;
}

int hymk_recv_softints(sid_t subj, long tm, int flags)
{
	NOT_A_FUNCTION;
}

reg_t hymk_read_regs(sid_t subj, int tp)
{
	NOT_A_FUNCTION;
}

void hymk_write_regs(sid_t subj, int tp, reg_t rv)
{
	NOT_A_FUNCTION;
}

void hymk_set_paged(void)
{
	NOT_A_FUNCTION;
}

/*
 * Region managment
 *
 */
region_t* reg_create(region_t region)
{
	NOT_A_FUNCTION;
	return NULL;
}


void reg_destroy(region_t *region)
{
	NOT_A_FUNCTION;
	return;
}

region_t* reg_alloc(int pages)
{
	NOT_A_FUNCTION;
	return NULL;
}

/*
 * Heap managment
 *
 */
void mem_heap_inc(int pages)
{
	NOT_A_FUNCTION;
}

void mem_heap_dec(int pages)
{
	NOT_A_FUNCTION;
}

/* Heap managment API */
void* mem_alloc(size_t sz)
{
	return malloc(sz);
}

void* mem_realloc(void* mem, size_t nsz)
{
	return realloc(mem, nsz);
}

void mem_free(void* mem)
{
	free(mem);
}


/* Stack managment API */
void* mem_stack_alloc(size_t sz)
{
	return mem_alloc(sz);
}

void mem_stack_free(void* stack)
{
	mem_free(stack);
	return;
}

/* Mutex functions */
int mtx_trylock(mtx_t *mutex)
{
	if (mutex != 0) return 0;
	*mutex = 1;
	return 1;
}

int mtx_lock(mtx_t *mutex, long timeout)
{
	if (mutex != 0)
	{
		printf("HydrixOS API emulation doesn't support multithreading.\n");
		printf("Deadlock possible.\n");
		exit(1);
	}
	return mtx_trylock(mutex);
}

void mtx_unlock(mtx_t *mutex)
{
	if (mutex == 0)
	{
		printf("WARNING: wrong mutex initialization possible.\n");
	}
	
	*mutex = 0;
	
	return;
}

/*
 * TLS functions
 *
 */
void** tls_global_alloc(void)
{
	void** l__retval = malloc(sizeof(void*));
	
	return l__retval;
}

void** tls_local_alloc(void)
{
	void** l__retval = malloc(sizeof(void*));
	
	return l__retval;
}

/*
 * buf_copy(dest, src, num)
 *
 * Copies "num" bytes of the content of the buffer "src" to the
 * buffer "dest".
 *
 * Return value:
 *	Number of the written bytes.
 *
 */
#ifndef HAVE_STDFUN_ARCH_BUF_COPY
size_t buf_copy(void* dest, const void* src, size_t num)
{
	size_t l__num = num;
	uint8_t *l__dest = dest;
	const uint8_t *l__src = src;
	
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return 0;
	}
	
	while (l__num --) *l__dest ++ = *l__src ++;	
	
	return num;
}
#endif

/*
 * buf_fill(dest, num, val)
 *
 * Fills the buffer "dest" with the value "val". The function will repeat 
 * this filling operation until "num" bytes have been written.
 *
 * Return value:
 *	Number of the written bytes.
 */
#ifndef HAVE_STDFUN_ARCH_BUF_FILL
size_t buf_fill(void* dest, size_t num, uint8_t val)
{
	uint8_t *l__dest = dest;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return 0;
	}
	
	while (num --)
	{
		*l__dest ++= val;
	}
	
	return num;
}
#endif

/*
 * buf_compare(dest, src, num)
 *
 * Compares the buffers "dest" and "src" until "num" bytes
 * have been compared or the content of "dest" and "src"
 * differs. 
 *
 * Return value:
 *	"dest" is graeter (>0), smaller (<0) or equal to src (= 0)
 *
 */
#ifndef HAVE_STDFUN_ARCH_BUF_COMPARE
int buf_compare(const void* dest, const void* src, size_t num)
{
	uint8_t *l__dest = (uint8_t*)dest;
	uint8_t *l__src = (uint8_t*)src;
	
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return 0;
	}	
	
	while (num --)
	{
		int l__retval = *l__dest ++ - *l__src ++;
		
		if (l__retval) return l__retval;
	}
	
	return 0;
}
#endif

/*
 * buf_find_uint8(dest, num, val)
 *
 * Searches the byte "val" within the first "num" bytes of
 * the buffer "dest".
 *
 * Return value:
 *	Address of the first occurence of "val" in "buf"
 *	NULL if failed.
 *
 */
#ifndef HAVE_STDFUN_ARCH_BUF_FIND_UINT8
void* buf_find_uint8(const void* dest, size_t num, uint8_t val)
{
	const uint8_t *l__dest = dest;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
		
	while ((num--) && (*l__dest != val)) l__dest ++;
	
	if ((num == -1) && (*l__dest != val)) l__dest = NULL;
	
	return (void*)l__dest;
}
#endif

/*
 * buf_find_uint16(dest, num, val)
 *
 * Searches the 16-bit word "val" within the first "num" 
 * bytes of the buffer "dest".
 *
 * Return value:
 *	Address of the first occurence of "val" in "buf"
 *	NULL if failed.
 *
 */
#ifndef HAVE_STDFUN_ARCH_BUF_FIND_UINT16
void* buf_find_uint16(const void* dest, size_t num, uint16_t val)
{
	const uint16_t *l__dest = dest;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	/* Is sizeof(val) a multiplier of num? */
	if (num % sizeof(val))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	num /= sizeof(val);
	
	while ((num--) && (*l__dest != val)) l__dest ++;
	
	if ((num == -1) && (*l__dest != val)) l__dest = NULL;
	
	return (void*)l__dest;
}
#endif

/*
 * buf_find_uint32(dest, num, val)
 *
 * Searches the 16-bit word "val" within the first "num" 
 * bytes of the buffer "dest".
 *
 * Return value:
 *	Address of the first occurence of "val" in "buf"
  *	NULL if failed.
 *
 */
#ifndef HAVE_STDFUN_ARCH_BUF_FIND_UINT32
void* buf_find_uint32(const void* dest, size_t num, uint32_t val)
{
	const uint32_t *l__dest = dest;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	/* Is sizeof(val) a multiplier of num? */
	if (num % sizeof(val))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	num /= sizeof(val);
	
	while ((num--) && (*l__dest != val)) l__dest ++;
	
	if ((num == -1) && (*l__dest != val)) l__dest = NULL;
	
	return (void*)l__dest;
}
#endif

/*
 * buf_find_uint64(dest, num, val)
 *
 * Searches the 16-bit word "val" within the first "num" 
 * bytes of the buffer "dest".
 *
 * Return value:
 *	Address of the first occurence of "val" in "buf"
  *	NULL if failed.
 *
 */
#ifndef HAVE_STDFUN_ARCH_BUF_FIND_UINT64
void* buf_find_uint64(const void* dest, size_t num, uint64_t val)
{
	const uint64_t *l__dest = dest;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	/* Is sizeof(val) a multiplier of num? */
	if (num % sizeof(val))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	num /= sizeof(val);
	
	while ((num--) && (*l__dest != val)) l__dest ++;
	
	if ((num == -1) && (*l__dest != val)) l__dest = NULL;
	
	return (void*)l__dest;
}
#endif

/*
 * buf_find_buf(dest, dnum, src, snum)
 *
 * Searches the buffer "src" within the first "dnum" 
 * bytes of the buffer "dest". The buffer "src" has
 * the size "snum".
 *
 * Return value:
 *	Address of the first occurence of "src" in "dest"
 *	NULL if failed.
 *
 */
#ifndef HAVE_STDFUN_ARCH_BUF_FIND_BUF
void* buf_find_buf(const void* dest, size_t dnum, const void *src, size_t snum)
{
	uint8_t *l__dest = (void*)dest;
	
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	if (snum > dnum)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	if (snum == 0)
	{
		return (void*)dest;
	}
	
	/* We work until "dnum" is greater or equal to "snum" */
	while(dnum >= snum)
	{
		if (!buf_compare(l__dest, src, snum )) return l__dest;
		l__dest ++;
		dnum --;
	}
	
	return NULL;
}
#endif

/*
 * str_len (dest, max)
 *
 * Determine the length of the zero-terminated byte
 * sequence "dest". The maximum size of "dest" will 
 * be "max". Returns ERR_INVALID_ARGUMENT if there
 * was no termination char until "max" found.
 *
 * Return value:
 *	>0 Size of the zero-terminated sequence in Byte
 *	   (without the terminating byte)
 *	-1, if error.
 *
 */
#ifndef HAVE_STDFUN_ARCH_STR_LEN
size_t str_len(const utf8_t* dest, size_t max)
{
	size_t l__retval = 0;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return -1;
	}
	
	while ((max --) && (*dest ++)) l__retval ++;
	
	if ((max == -1) && (*(dest - 1) != 0))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return -1;
	}
	
	return l__retval;
}
#endif

/*
 * str_copy (dest, src, max)
 *
 * Copies the zero-terminated byte sequence "src" into the
 * buffer "dest" (including the terminating charracter). 
 * If there is no terminating character "max" bytes will be
 * copied and the function will quit with an "ERR_INVALID_ARGUMENT".
 *
 * Return value:
 *	>0 Number of copied bytes.
 *	-1 Error.
 *
 */
#ifndef HAVE_STDFUN_ARCH_STR_COPY
size_t str_copy(utf8_t* dest, const utf8_t* src, size_t max)
{
	size_t l__retval = 0;
	
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return -1;
	}
	
	while (max --)
	{
		l__retval ++;
		*dest ++ = *src;
		if (!(*src ++)) break;
	}
	
	if ((max == -1) && (*(dest - 1) != 0))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return -1;
	}
	
	return l__retval;
}
#endif

/*
 * str_compare (dest, src, max)
 *
 * Compares the zero-terminated byte sequence "src" with the
 * zero-terminated byte sequence "dest". If there is no
 * terminating character "max" bytes will be compared and the
 * function will quit with an "ERR_INVALID_ARGUMENT".
 *
 * Return value:
 *	>0	"dest" is graeter than "src"
 *	<0	"dest" is smaller than "src"
 *	=0	"dest" is equal to "src"
 *
 */
#ifndef HAVE_STDFUN_ARCH_STR_COMPARE
int str_compare(const utf8_t* dest, const utf8_t* src, size_t max)
{
	int l__retval = 0;
	
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return -1;
	}
	
	while ((max --) && (*dest) && (*src) && (l__retval = *dest ++ - *src ++));
	
	if ((max == -1) && ((*(dest - 1) != 0) || (*(src - 1) != 0)))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return -1;
	}
	
	return l__retval;
}
#endif

/*
 * str_char (dest, sgn, max)
 *
 * Searches the zero-terminated byte sequence "dest" for
 * the byte "sgn". If there is no terminating character
 * "max" bytes will be compared and the function will quit
 * with an "ERR_INVALID_ARGUMENT".
 *
 * Return values:
 *	Pointer to the first occurence of the searched byte.
 *	NULL, if failed.
 *
 */
#ifndef HAVE_STDFUN_ARCH_STR_CHAR
utf8_t* str_char(const utf8_t* dest, utf8_t sgn, size_t max)
{
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return (void*)NULL;
	}
	
	while ((max --) && (*dest) && (*dest ++ != sgn)) ;
	
	if ((max == -1) && (*(dest - 1) != 0))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return (void*)NULL;
	}
	
	return (void*)dest - 1;
}
#endif

/*
 * str_find (dest, dmax, src, smax)
 *
 * Searches the first occurence zero-terminated sequence "src"
 * within the zero-terminated sequence "dest". The sequence "src"
 * will have a size of "smax" bytes at least. If the terminating
 * character isn't found within these first "smax" bytes of "src"
 * the function will quit with an "ERR_INVALID_ARGUMENT".
 *
 * "src" will be compared excluding the terminating character.
 *
 * The sequence "dest" will have a size of "dmax" bytes at least. If
 * the terminating character isn't found within the first "dmax"
 * bytes of "dest" the function will quit with an "ERR_INVALID_ARGUMENT".
 *
 * Return value:
 *	Poitner to the first occurence of "src" in "dest".
 *	NULL, if failed.
 *
 */ 
#ifndef HAVE_STDFUN_ARCH_STR_FIND
utf8_t* str_find(const utf8_t* dest, size_t dmax, const utf8_t* src, size_t smax)
{
	utf8_t *l__dest = (void*)dest;
	size_t l__dnum;
	size_t l__snum;
	
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	/* Get the correct size of "dest" and "src" */
	l__dnum = str_len(dest, dmax);
	if (*tls_errno) return NULL;
	
	l__snum = str_len(src, smax);
	if (*tls_errno) return NULL;	
	
	/* Work with them */
	if (l__snum > l__dnum)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	if (l__snum == 0)
	{
		return (void*)dest;
	}
	
	/* We work until "dnum" is greater or equal to "snum" */
	while(l__dnum >= l__snum)
	{
		if (!buf_compare(l__dest, src, l__snum - 1)) return l__dest;
		l__dest ++;
		l__dnum --;
	}
	
	return NULL;
}
#endif


void* pmap_alloc(long sz)
{
	if (sz % 4096) sz += 0x1000;
	sz &= ~0xFFF;
	
	return mem_alloc(sz);
}

void pmap_free(void* ptr)
{
	mem_free(ptr);
}

void* pmap_mapalloc(long sz)
{
	return pmap_alloc(sz);
}
