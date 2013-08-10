/*
 *
 * sync.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying').
 *
 * Thread synchronization
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


/*
 * ksync_interrupt_other(other)
 *
 * Awakes the other side of the synchronization
 * caused by the destruction of the current thread.
 *
 */
void ksync_interrupt_other(uint32_t *other)
{
	if (other[THRTAB_THRSTAT_FLAGS] & THRSTAT_TIMEOUT)
	{
		/* Let the timeout end */
		ksched_del_timeout(other);
		other[THRTAB_THRSTAT_FLAGS] &= (~THRSTAT_TIMEOUT);
	}
	
	other[THRTAB_SYNC_SID] = SID_PLACEHOLDER_INVALID;

	/* Restart it */
	ksched_start_thread(other);
		
	return;
}

/*
 * ksync_removefrom_waitqueue_error(other, me)
 *
 * Removes the SID of 'me' from the waitqueue of 'other'
 * during the destruction of a thread.
 *
 */
void ksync_removefrom_waitqueue_error(uint32_t *other, uint32_t *me)
{
	uint32_t *l__ent = NULL;
	
	/* Are we at the begin of the list? */
	if ((uintptr_t)me == other[THRTAB_OWN_SYNC_QUEUE_BEGIN])
	{
		/* Remove us, set the next thread of the list to the begin */
		other[THRTAB_OWN_SYNC_QUEUE_BEGIN] = (uintptr_t)
		          me[THRTAB_CUR_SYNC_QUEUE_NEXT];
		
		l__ent = (void*)(uintptr_t)
			me[THRTAB_CUR_SYNC_QUEUE_NEXT];
		
		if (l__ent != NULL)
			l__ent[THRTAB_CUR_SYNC_QUEUE_PREV] = 0;
	}
	 else
	{
		/* Remove us from the list */
		if (me[THRTAB_CUR_SYNC_QUEUE_NEXT] != 0)
		{
			l__ent = (void*)(uintptr_t)
				me[THRTAB_CUR_SYNC_QUEUE_NEXT];
		
			l__ent[THRTAB_CUR_SYNC_QUEUE_PREV] = (uintptr_t)
				me[THRTAB_CUR_SYNC_QUEUE_PREV];
		}
		
		l__ent = (void*)(uintptr_t)
			me[THRTAB_CUR_SYNC_QUEUE_PREV];
		
		l__ent[THRTAB_CUR_SYNC_QUEUE_NEXT] = (uintptr_t)
				me[THRTAB_CUR_SYNC_QUEUE_NEXT];	
	}
	
	me[THRTAB_CUR_SYNC_QUEUE_NEXT] = 0;
	me[THRTAB_CUR_SYNC_QUEUE_PREV] = 0;

	return;
}

/*
 * ksync_awake_other(other)
 *
 * Awakes the other side of a synchronization 
 *
 */
static inline void ksync_awake_other(sid_t other)
{
	/* If it is alive, just awake it */
	if (THREAD(other, THRTAB_THRSTAT_FLAGS) & THRSTAT_TIMEOUT)
	{
		ksched_del_timeout(&THREAD(other, 0));
	}
		
	THREAD(other, THRTAB_THRSTAT_FLAGS) &= ~(THRSTAT_TIMEOUT | THRSTAT_SYNC);		
	ksched_start_thread(&THREAD(other,  0));
	
	THREAD(other, THRTAB_SYNC_SID) = current_t[THRTAB_SID];

	return;
}

/*
 * ksync_is_waitqueue(other)
 *
 * Test if a SID or a class of SIDs can be resolved to a
 * Thread-SID in the wait queue of the current thread.
 *
 * Return value:
 *	>0 	SID of the waiting thread
 *	==0	No thread that fits to the criteria 'other'
 *
 */
static inline sid_t ksync_is_waitqueue(sid_t other)
{
	uint32_t *l__ent = (void*)(uintptr_t)
				current_t[THRTAB_OWN_SYNC_QUEUE_BEGIN];
	
	/* No thread in queue */
	if (l__ent == NULL) return 0;
	
	/* Search within the queue */
	do
	{
		/* Does it fit to our criteria? */
		if (    (l__ent[THRTAB_SID] == other)
		     || (l__ent[THRTAB_PROCESS_SID] == other)
		     || (     (PROCESS(l__ent[THRTAB_PROCESS_SID],
		     		       PRCTAB_IS_ROOT
				      )
			        == 1
			      )
		          &&  (other == SID_USER_ROOT)
			)
		     || (other == SID_USER_EVERYBODY)
		        /* PageD-only */
		     || (    (other == SID_PLACEHOLDER_KERNEL)
		          && (   l__ent[THRTAB_THRSTAT_FLAGS] 
			       & THRSTAT_WAIT_HYPAGED
			     )
			  && (    current_t[THRTAB_SID] 
			       == paged_thr_sid
			     )
			)
		   )
		{
			/* Yes... */
			return l__ent[THRTAB_SID];
		}
	}while(   (l__ent = (void*)(uintptr_t)
	                      l__ent[THRTAB_CUR_SYNC_QUEUE_NEXT]
	          ) 
	       != NULL
	      );

	/* No thread found */
	return 0;
}

/*
 * ksync_addto_waitqueue(other)
 *
 * Adds the current thread to the waitqueue of 'other'.
 *
 * Return value:
 *	0	Wait queue full
 *	1	OKAY
 *
 */
static inline int ksync_addto_waitqueue(sid_t other)
{
	uint32_t *l__ent = (void*)(uintptr_t)
		THREAD(other, THRTAB_OWN_SYNC_QUEUE_BEGIN);
	
	/* Empty list */
	if (l__ent == NULL)
	{
		THREAD(other, THRTAB_OWN_SYNC_QUEUE_BEGIN) =
			(uintptr_t)current_t;
		current_t[THRTAB_CUR_SYNC_QUEUE_NEXT] = 0;
		current_t[THRTAB_CUR_SYNC_QUEUE_PREV] = 0;
		
		return 1;
	}
		
	/* Find the end of the list */
	while (l__ent[THRTAB_CUR_SYNC_QUEUE_NEXT] != NULL)
		l__ent = (void*)(uintptr_t)l__ent[THRTAB_CUR_SYNC_QUEUE_NEXT];
		
	/* Add to list */	
	l__ent[THRTAB_CUR_SYNC_QUEUE_NEXT] = (uintptr_t)current_t;
	current_t[THRTAB_CUR_SYNC_QUEUE_NEXT] = 0;
	current_t[THRTAB_CUR_SYNC_QUEUE_PREV] = (uintptr_t)l__ent;
	
	return 1;	
}

/*
 * ksync_removefrom_waitqueue(other)
 *
 * Removes our SID from the waitqueue of 'other'.
 *
 */
static inline void ksync_removefrom_waitqueue(sid_t other)
{
	uint32_t *l__ent = NULL;
		
	/* Are we at the begin of the list? */
	if ((uintptr_t)current_t == THREAD(other, THRTAB_OWN_SYNC_QUEUE_BEGIN))
	{
		/* Remove us, set the next thread of the list to the begin */
		THREAD(other, THRTAB_OWN_SYNC_QUEUE_BEGIN) = (uintptr_t)
		          current_t[THRTAB_CUR_SYNC_QUEUE_NEXT];
		
		l__ent = (void*)(uintptr_t)
			current_t[THRTAB_CUR_SYNC_QUEUE_NEXT];
		
		if (l__ent != NULL)
			l__ent[THRTAB_CUR_SYNC_QUEUE_PREV] = 0;
	}
	 else
	{
		/* Remove us from the list */
		if (current_t[THRTAB_CUR_SYNC_QUEUE_NEXT] != 0)
		{
			l__ent = (void*)(uintptr_t)
				current_t[THRTAB_CUR_SYNC_QUEUE_NEXT];
		
			l__ent[THRTAB_CUR_SYNC_QUEUE_PREV] = (uintptr_t)
				current_t[THRTAB_CUR_SYNC_QUEUE_PREV];
		}
		
		l__ent = (void*)(uintptr_t)
			current_t[THRTAB_CUR_SYNC_QUEUE_PREV];
		
		l__ent[THRTAB_CUR_SYNC_QUEUE_NEXT] = (uintptr_t)
				current_t[THRTAB_CUR_SYNC_QUEUE_NEXT];	
	}
	
	current_t[THRTAB_CUR_SYNC_QUEUE_NEXT] = 0;
	current_t[THRTAB_CUR_SYNC_QUEUE_PREV] = 0;
	
	return;
}

/*
 * ksync_wait_for(other, timeout)
 *
 * The thread waits for 'other' until 'timeout' ends
 * and sets/clears the THRTAB_SYNC_SID entry.
 *
 * Return value:
 *	The SID of the other thread. If it awakes us,
 *	it has to change our THRTAB_SYNC_SID! 
 *
 *	0	Time out/error (ERROR VALUE SET !)
 *	>0	SID of the other thread
 *
 */
static inline sid_t ksync_wait_for(sid_t other, unsigned timeout)
{
	sid_t l__retval;
	
	if (timeout == 0)
	{
		SET_ERROR(ERR_TIMED_OUT);
		return 0;
	}
	
	current_t[THRTAB_SYNC_SID] = other;
	current_t[THRTAB_THRSTAT_FLAGS] |= THRSTAT_SYNC;
	
	if (timeout != 0xFFFFFFFFu)
	{
		ksched_add_timeout(current_t, timeout);
		current_t[THRTAB_THRSTAT_FLAGS] |= THRSTAT_TIMEOUT;
	}

	ksched_stop_thread(current_t);
	ksched_change_thread = true;
	ksched_next_thread();
	i386_yield_kernel_thread();
	
	MSYNC();
	
	l__retval = current_t[THRTAB_SYNC_SID];
	current_t[THRTAB_SYNC_SID] = 0;
	
	/* Timed out */
	if (current_t[THRTAB_THRSTAT_FLAGS] & THRSTAT_SYNC)
	{
		current_t[THRTAB_THRSTAT_FLAGS] &= (~THRSTAT_SYNC);
		l__retval = 0;
		SET_ERROR(ERR_TIMED_OUT);
	}
		
	return l__retval;
}

/*
 * sysc_sync(other, timeout, resyncs)
 *
 * (Implementation of the "sync" system call)
 *
 * Synchronizes this thread with another thread that called
 * the sync system call with the SID of the caller or a SID
 * that fits to the caller. If the other side is not able to
 * synchronize or is synchronizing with another thread or
 * to a SID that fits not to the caller, the function will
 * wait until the timeout ends or the other side will be
 * ready to synchronize.
 *
 * Parameters:
 *	other		SID of the other thread / the SIDs
 *			that will be allowed to synchronize:
 *				- A special thread (Thread-SID)
 *				- Any thread of a special process (Process-SID)
 *				- Any thread of a process in root mode (ROOT)
 *				- Any thread (EVERYBODY)
 *				- Kernel (only for Paged)
 *
 *	timeout		Timeout of the operation in ms (0 = no waiting, 
 *							0xFFFFFFFF unlimited
 *						       )
 *	resyncs		Number of resyncs
 *
 * Return value:
 *	SID		SID of the thread that have been synchronized
 *			with the calling thread	
 *
 */
sid_t sysc_sync(sid_t other,
	        unsigned timeout,
	        unsigned resync
	       )
{
	sid_t l__retval = 0;
	
	/* Ignore self-thread-syncs */
	if (other == current_t[THRTAB_SID])
		return other;
	
	do
	{
		/* Sync with a thread subject */
		if (other & SIDTYPE_THREAD)
		{
			sid_t l__other_want = 0;
			
			if (!kinfo_isthrd(other))
			{
				SET_ERROR(ERR_INVALID_SID);
				return 0;
			}
			
			l__other_want = THREAD(other, THRTAB_SYNC_SID);
			
			/* Is it waiting for us? */
			if (    (   (l__other_want == current_t[THRTAB_SID])
				 || (l__other_want == current_p[PRCTAB_SID])
				 || (   (l__other_want == SID_USER_ROOT)
				     && (current_p[PRCTAB_IS_ROOT] == 1)
				    )
				 || (l__other_want == SID_USER_EVERYBODY)
				 /* PageD-Operations only */
				 || (   (    l__other_want 
				         ==  SID_PLACEHOLDER_KERNEL
					)
				     && (  current_t[THRTAB_THRSTAT_FLAGS]
				         & THRSTAT_WAIT_HYPAGED
					)
				    )
				)
			     && (   THREAD(other, THRTAB_THRSTAT_FLAGS) 
			         &  THRSTAT_SYNC
				)
			   )
			{
				/* Awake it */
				ksync_awake_other(other);
				l__retval = other;
			}
			 else
			{
				/* Wait for it */
				if (!ksync_addto_waitqueue(other))
				{
					SET_ERROR(ERR_RESOURCE_BUSY);
					return 0;
				}
				
				l__retval = ksync_wait_for(other, timeout);
								
			    	ksync_removefrom_waitqueue(other);
								
				MSYNC();
			}
		}
		 else /* Sync with a non-thread subject */
		{
			/* Is it already a part of our waitqueue? */
			l__retval = ksync_is_waitqueue(other);
			
			if (l__retval)
			{
				/* Awake it */
				ksync_awake_other(l__retval);
			}
			 else
			{
				l__retval = 0;
				/* Wait for it */
				l__retval = ksync_wait_for(other, timeout);

				MSYNC();
			}
		}
		
		/* Error or time out */
		if (l__retval == 0) break;
		
		/* TODO: Resync only with the last sync'ing thread */
	}while (resync --);
		
	/* Probably we've to resched */
	KSCHED_TRY_RESCHED();
	 
	return l__retval;
}
