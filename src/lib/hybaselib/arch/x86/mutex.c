/*
 *
 * mutex.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Implementation of Mutex handling
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/mutex.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/hymk.h>
#include <hydrixos/blthr.h>
#include <hydrixos/system.h>
#include "../../hybaselib.h"

/*
 * mtx_trylock (mutex)
 *
 * Tries to lock the lock "mutex" atomically.
 * 
 * Return value:
 *	0	If the mutex couldn't be locked.
 *	1	If the mutex was locked.
 *
 */
int mtx_trylock(mtx_t *mutex)
{
	volatile int l__a = 0;

	if (mutex == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return 0;
	}

	/*
	 * Compare the value of the mutex with
	 * the value 0 (stored in EAX). If it
	 * is equal "cmpxchgl" will set 'mutex'
	 * to the value 1 atomically.
	 * Otherwise "cmpxchgl" will store
	 * the content of 'mutex' to the register EAX
	 * which will be stored to 'l__a'
	 * 
	 * Of course we have to use the LOCK
	 * prefix for the cmpxchgl instruction
	 * to lock the bus on SMP systems.
	 *
	 */
	__asm__ __volatile__ ("lock cmpxchgl %%edx, (%%ebx)\n\t"
	     	      	      : "=a" (l__a)
	     	      	      : "d" (1), "a" (0), "b" ((uintptr_t)&(mutex->mutex)) 
	     	      	      : "memory"
	     	      	     );

	/* l__a will be set to 1, if the mutex was already locked.
	 * l__a will stay at 0, if the mutex have been locked now.
	 */
	if (l__a)
		return 0;
	else
		return 1;
}
	 
/*
 * mtx_unlock (mutex)
 *
 * Unlocks the Mutex "mutex"
 *
 */
void mtx_unlock(mtx_t *mutex)
{
	int l__latency = mutex->latency;
	mutex->latency = 0;
	
	if (mutex == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return;
	}
	
	/* 
	 * Unlock the mutex atomically by using
	 * the lock instruction prefix
	 *
	 */
	__asm__ __volatile__ ("lock andl $0, (%%ebx)\n\t"
		      	      :
		      	      : "b" ((uintptr_t)&(mutex->mutex))
		      	      : "memory"
		      	     );
		     
	/* Do we have to yield? */
	if (l__latency)
	{
		blthr_yield(0);
	}
	
	return;
}


/*
 * mtx_lock (mutex, timeout)
 *
 * Tries to lock the lock "mutex". If the lock
 * is currently locked, the function tries to
 * wait for the unlocking of the mutex using the
 * thread_yield-Operation until "timeout" tries
 * where failed. If "timeout" is -1 the function
 * will make unlimited tries for locking the mutex.
 *
 * Return value:
 *	1	Mutex locked
 *	0	Mutex coudn't be locked
 *
 */
int mtx_lock(mtx_t *mutex, long timeout)
{
	uint32_t l__starttm = 0;
	
	/* Load our starting time */
	if (timeout != MTX_UNLIMITED)
	{
		l__starttm = hysys_info_read(MAININFO_RTC_COUNTER_LOW);
	}
	
	/* Invalid mutex, exit */
	if (mutex == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return 0;
	}
	
	/* Our mutex access loop */
	while(1)
	{
		if (!mtx_trylock(mutex))
		{
			/* Set up the latency flag */
			mutex->latency = 1;			
			
			blthr_yield(0);
		}
		 else
		{
			/* Mutex was locked successfully */
			return 1;
		}

		/* Not unlimited timeout */
		if (timeout != MTX_UNLIMITED) 
		{
			uint32_t l__now = hysys_info_read(MAININFO_RTC_COUNTER_LOW);
			
			if ((signed)(l__now - l__starttm) >= timeout) 
			{
				*tls_errno = ERR_TIMED_OUT;
				return 0;
			}
		}

	}
	
	
	*tls_errno = ERR_TIMED_OUT;
	return 0;
}
