/*
 *
 * timeout.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying').
 *
 * Time out operations
 *
 */
#include <hydrixos/types.h>
#include <stdio.h>
#include <mem.h>
#include <info.h>
#include <error.h>
#include <setup.h>
#include <sched.h>
#include <sysc.h>
#include <current.h>
#include <string.h>

uint32_t *timeout_queue = NULL;		/* First thread of the timeout queue */
unsigned int timeout_num = 0;		/* Number of threads within the timeout queue */
uint64_t timeout_next = 0;		/* Time of the next timeout */

/*
 * ksched_add_timeout(thr, to)
 *
 * Adds a thread 'thr' (pointer to the descriptor) to 
 * the timeout queue and sets the timeout of 'to' ms.
 * This function won't stop the thread and won't also
 * change its THRSTAT_TIMEOUT-Flag. This will 
 * be the object of the calling function.
 *
 */
void ksched_add_timeout(uint32_t *thr, uint32_t to)
{
	uint64_t l__time = to + (*kinfo_rtc_ctr);
	uint32_t l__n = timeout_num;
	uint32_t *l__queue;
	uint32_t *l__queue_prev;
	
	/* Set time */
	thr[THRTAB_TIMEOUT_LOW] = (uint32_t)l__time;
	thr[THRTAB_TIMEOUT_HIGH] = (uint32_t)(l__time >> 32);
	
	/* No timeouts ? */
	if (timeout_queue == NULL)
	{
		timeout_queue = thr;
		thr[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)NULL;
		thr[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)NULL;
		timeout_next = l__time;
		timeout_num = 1;
		
		return;
	}
	
	/* Sooner than the next time out? */
	if (timeout_next >= l__time)
	{
		timeout_queue[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)thr;
		thr[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)timeout_queue;
		thr[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)NULL;
		
		timeout_queue = thr;
		timeout_next = l__time;
		timeout_num ++;
	
		return;
	}
	
	/* Else ... */
	l__queue = timeout_queue;
	l__queue_prev = NULL;
	
	/* Timeouts given */
	while(l__n --)
	{
		uint64_t l__testtime =    (uint64_t)l__queue[THRTAB_TIMEOUT_LOW]
					| ((uint64_t)l__queue[THRTAB_TIMEOUT_HIGH] << 32);
		
		/* Test the next member of the queue */
		if (l__testtime >= l__time)
		{
			thr[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)l__queue_prev;
			thr[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)l__queue;
			l__queue[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)thr;
			l__queue_prev[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)thr;
			
			timeout_num ++;
			return;
		}
		
		l__queue_prev = l__queue;
		l__queue = (void*)(uintptr_t)l__queue[THRTAB_TIMEOUT_QUEUE_NEXT];
		
		if (l__queue == NULL) break;
	}

	l__queue_prev[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)thr;
	thr[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)NULL;
	thr[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)l__queue_prev;

	timeout_num ++;
	
	return;
}

/*
 * ksched_del_timeout(thr)
 *
 * Removes a thread 'thr' from the timeout list
 * and clears the timeout. The function won't start
 * the thread and won't change the THRSTAT_TIMEOUT
 * flag. This will be the object of the calling function.
 *
 */
void ksched_del_timeout(uint32_t *thr)
{
	/* Do it fast, if we are the first member of the list */
	if (thr == timeout_queue)
	{
		timeout_queue = (void*)(uintptr_t)thr[THRTAB_TIMEOUT_QUEUE_NEXT];
			
		/* Last member of the list? */
		if (timeout_queue == NULL)
		{
			timeout_next = 0;
			timeout_num = 0;
		}
		 else
		{
			timeout_next =    ((uint64_t)timeout_queue[THRTAB_TIMEOUT_LOW])
					| ((uint64_t)timeout_queue[THRTAB_TIMEOUT_HIGH] << 32);
							
			timeout_queue[THRTAB_TIMEOUT_QUEUE_PREV] = NULL;
			
			timeout_num --;
		}
	}
	 else
	{
		/* Okay, we are somewhere in the list */
		uint32_t *l__prev = (void*)(uintptr_t)thr[THRTAB_TIMEOUT_QUEUE_PREV];
		uint32_t *l__next = (void*)(uintptr_t)thr[THRTAB_TIMEOUT_QUEUE_NEXT];
		
		l__prev[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)l__next;
		if (l__next != NULL)
		{
			l__next[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)l__prev;
		}
		timeout_num --;
	}
	thr[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)NULL;	
	thr[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)NULL;
	thr[THRTAB_TIMEOUT_LOW] = 0;
	thr[THRTAB_TIMEOUT_HIGH] = 0;

	return;
}

