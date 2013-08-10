/*
 *
 * schedule.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying').
 *
 * Includes the "schedule" system call and the
 * kernel scheduler function.
 *
 */
#include <hydrixos/types.h>
#include <stdio.h>
#include <mem.h>
#include <info.h>
#include <error.h>
#include <setup.h>
#include <sched.h>
#include <current.h>
#include <sysc.h>

/* Count of threads that are ready for execution*/
long ksched_active_threads = 0;

/* Signalizes the need of a thread switch */
long ksched_change_thread = 0;	

/* The "idle" kernel thread */
uint32_t *ksched_idle_thread;

/*
 * ksched_start_thread(thrd)
 *
 * Adds the thread that is described by the descriptor 'thrd'
 * to the runqueue of the scheduler and removes its 'THRSTAT_BUSY'
 * status flag.
 *
 * Return value:
 *	== 0	Successful
 *	!= 0	Error
 *
 */
int ksched_start_thread(uint32_t *thrd)
{
	uint32_t *l__current_t = current_t;
	
	/* Is the thread already active? */
	if (    (thrd[THRTAB_RUNQUEUE_PREV] != NULL)
	     || (thrd[THRTAB_RUNQUEUE_NEXT] != NULL)
	   )
	{
		return 0;
	}
	
	/* Never start an defunced thread */
	if (thrd[THRTAB_THRSTAT_FLAGS] & THRSTAT_PROC_DEFUNC)
		return 0;
	
	/* Also: Never start a freezed thread */
	if (thrd[THRTAB_FREEZE_COUNTER])
		return 0;
	
	/*
	 * The effective priority consists of 
	 * a quarter of the unused effective 
	 * priority of the last execution and 
	 * the static priority. 
	 *
	 */
	thrd[THRTAB_EFFECTIVE_PRIORITY] = 
		(
		    (thrd[THRTAB_EFFECTIVE_PRIORITY] / 4)
		  + thrd[THRTAB_STATIC_PRIORITY]
		);
	
	/* Recalculate it to clock ticks */
	thrd[THRTAB_EFFECTIVE_PRIORITY] *= 2;
			
	/*
	 * If the effective priority of the new thread will
	 * be greater than or equal to the priority of the current 
	 * thread the current thread will be preempted and the new
	 * thread will be executed.
	 *
	 * If the current thread is the 'idle thread' however
	 * this operation won't be executed.
	 *
	 */
	if (    (    thrd[THRTAB_EFFECTIVE_PRIORITY] 
		  >= l__current_t[THRTAB_EFFECTIVE_PRIORITY]
		) 
     	     && (l__current_t != ksched_idle_thread)
   	   )
	{
		/*
		 * Add the new thread into the runqueue
		 * between the current thread and its
		 * following thread
		 *
		 */
		thrd[THRTAB_RUNQUEUE_PREV] = (uintptr_t)l__current_t;
		thrd[THRTAB_RUNQUEUE_NEXT] = (uintptr_t)l__current_t[THRTAB_RUNQUEUE_NEXT];
		
		l__current_t[THRTAB_RUNQUEUE_NEXT] = (uintptr_t)thrd;
		if (thrd[THRTAB_RUNQUEUE_NEXT] != (uintptr_t)NULL)
		{
			uint32_t *l__thrd_next = (void*)(uintptr_t)thrd[THRTAB_RUNQUEUE_NEXT];
			
			l__thrd_next[THRTAB_RUNQUEUE_PREV] = (uintptr_t)thrd;			
		}
		
		/* Change threads after leaving the kernel mode */
		ksched_change_thread = true;
	}
	 else
	{
		/*
		 * If not, the new thread will be executed after all
		 * other threads of the run queue were executed. So
		 * we put it into the runqueue after the entry of the
		 * idle thread
		 *
		 */
		thrd[THRTAB_RUNQUEUE_NEXT] =
			(uintptr_t)ksched_idle_thread[THRTAB_RUNQUEUE_NEXT];
		thrd[THRTAB_RUNQUEUE_PREV] = 
			(uintptr_t)ksched_idle_thread;
		ksched_idle_thread[THRTAB_RUNQUEUE_NEXT] = 
			(uintptr_t)thrd;
		
		if (thrd[THRTAB_RUNQUEUE_NEXT] != (uintptr_t)NULL) 
		{
			uint32_t *l__thrd_next = (void*)(uintptr_t)thrd[THRTAB_RUNQUEUE_NEXT];
			
			l__thrd_next[THRTAB_RUNQUEUE_PREV] = (uintptr_t)thrd;			
		}		
	}	

	ksched_active_threads ++;
	/*
	 * We remove the THRSTAT_BUSY flag here. However
	 * the flag that described the reason of the blocking
	 * of our new thread have to be removed by the function
	 * that called ksched_start_thread()!
	 *
	 */
	thrd[THRTAB_THRSTAT_FLAGS] &= (~THRSTAT_BUSY);
		
	return 0;
}

/*
 * ksched_stop_thread(thrd)
 *
 * Removes the thread 'thrd' from the runque and sets
 * its THRSTAT_BUSY flag.
 *
 * !! Important: ksched_stop_thread won't change 
 * !!            ksched_change_thread if it should
 * !!		 stop the current thread. This is
 * !!		 the object of the calling function!
 *
 * Return value:
 *	== 0	Successful
 *	!= 0	Error
 *
 */ 
int ksched_stop_thread(uint32_t *thrd)
{
	/* Remove the thread from the run queue */
	if (thrd[THRTAB_RUNQUEUE_PREV] != (uintptr_t)NULL) 
	{
		uint32_t *l__thr_prev = (void*)(uintptr_t)thrd[THRTAB_RUNQUEUE_PREV];
		l__thr_prev[THRTAB_RUNQUEUE_NEXT] = thrd[THRTAB_RUNQUEUE_NEXT];
	}
	
	if (thrd[THRTAB_RUNQUEUE_NEXT] != (uintptr_t)NULL) 
	{
		uint32_t *l__thr_next = (void*)(uintptr_t)thrd[THRTAB_RUNQUEUE_NEXT];
		l__thr_next[THRTAB_RUNQUEUE_PREV] = thrd[THRTAB_RUNQUEUE_PREV];
	}
	
	/* Change active thread count only if really removed from list! */
	if (    (thrd[THRTAB_RUNQUEUE_PREV] != NULL)
	     || (thrd[THRTAB_RUNQUEUE_NEXT] != NULL)
	   )
	{
		ksched_active_threads --;
		
		/* Remove from runque */
		thrd[THRTAB_RUNQUEUE_PREV] = (uintptr_t)NULL;	
		thrd[THRTAB_RUNQUEUE_NEXT] = (uintptr_t)NULL;	
		/* 
	 	 * Set the thread's busy flag
	 	 *
	 	 */
		thrd[THRTAB_THRSTAT_FLAGS] |= THRSTAT_BUSY;
	}
	
	return 0;
}

/*
 * sysc_set_priority(thrd, priority, policy)
 *
 * (Implementation of the "set_priority" system call)
 *
 * Changes the priority and the scheduling policy of
 * a thread. If the thread isn't part of a root process
 * it may only reduce its priority or scheduling policy
 * value. 
 *
 * Parameters:
 *	thrd		SID of the affected thread
 *	priority	New scheduling priority
 *	policy		New scheduling policy
 *
 */
void sysc_set_priority(sid_t thrd, unsigned priority, unsigned policy)
{
	int l__isroot = 0;
	
	/* Is it a valid thread? */
	if (!kinfo_isthrd(thrd))
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
	
	/* Is it a thread of a root process? */
	l__isroot = PROCESS(
			    THREAD(thrd, THRTAB_PROCESS_SID),
			    PRCTAB_IS_ROOT
			   );
	
	/* Only root processes may increase their priority */
	if (!l__isroot)
	{
		if (    (THREAD(thrd, THRTAB_STATIC_PRIORITY) < priority)
		     || (THREAD(thrd, THRTAB_SCHEDULING_CLASS) < policy)
		   )
		{
			SET_ERROR(ERR_ACCESS_DENIED);
			return;
		}
	}   
	
	/* Is the wanted priority or scheduling class valid? */		   
	if (    (priority > SCHED_PRIORITY_MAX)
	     || (policy > SCHED_CLASS_MAX)
	   )
	{
		SET_ERROR(ERR_INVALID_ARGUMENT);
		return;
	}
	
	/* Change the priority */
	THREAD(thrd, THRTAB_STATIC_PRIORITY) = priority;
	THREAD(thrd, THRTAB_SCHEDULING_CLASS) = policy;
	
	return;
}

/*
 * sysc_awake_subject(subj)
 *
 * (Implementation of the "awake_subject" system call)
 *
 * Awakes a thread subject. 
 *
 * A thread will enter the run queue if its freeze counter
 * has the value 0.
 *
 * Parameters:
 *	subj	SID of the affected thread
 *
 */
void sysc_awake_subject(sid_t subj)
{
	if ((subj & SID_TYPE_MASK) == SIDTYPE_THREAD)
	{
		/* Is the used thread SID valid? */
		if (    (!kinfo_isthrd(subj))
		     || ((subj == 0x1000000) && (current_p != &process_tab[0]))
		   )
		{
			SET_ERROR(ERR_INVALID_SID);
			return;
		}

		/* Is it already active? */		
		if (THREAD(subj, THRTAB_FREEZE_COUNTER) == 0)
		{
			SET_ERROR(ERR_INVALID_ARGUMENT);
			return;
		}
				
		/* Reduce its freeze counter */
		THREAD(subj, THRTAB_FREEZE_COUNTER) --;
		
		THREAD(subj, THRTAB_THRSTAT_FLAGS) &= (~THRSTAT_FREEZED);
		    
		/* Can we put it into the run queue? (only if it is not still selected as busy!) */
		if (    
			 (THREAD(subj, THRTAB_FREEZE_COUNTER) == 0)
		     &&  (!(THREAD(subj, THRTAB_THRSTAT_FLAGS) & THRSTAT_OTHER_FREEZE))
		   )
		{
			ksched_start_thread(&(THREAD(subj, 0)));
			
			/* If needed, yield the current thread */
			KSCHED_TRY_RESCHED();
		}
		
		return;
	}
	
	/* Invalid SID */
	SET_ERROR(ERR_INVALID_SID);
	return;	
}

/*
 * sysc_freeze_subject(subj)
 *
 * (Implementation of the "freeze_subject" system call)
 *
 * Freezes a thread subject. It is allowed to freeze
 * the current thread.
 *
 * A thread will leave the run queue if its freeze counter
 * changes from 0 to 1.
 *
 * Parameters:
 *	subj	SID of the affected thread (null or invalid for 
 *		current thread)
 *
 */
void sysc_freeze_subject(sid_t subj)
{
	long l__testsec = 0;
	
	/* Are we allowed to do that */
	if (!current_p[PRCTAB_IS_ROOT])
	{
		l__testsec = 1;			
	}
	
	/* Invalid or null for current thread */
	if ((subj == SID_PLACEHOLDER_INVALID) || (subj == SID_PLACEHOLDER_NULL))
	{
		subj = current_t[THRTAB_SID];
	}

	/* Awake a thread */
	if ((subj & SID_TYPE_MASK) == SIDTYPE_THREAD)
	{
		/* Is the used thread SID valid? */
		if (    (!kinfo_isthrd(subj))
		     || ((subj == 0x1000000) && (current_p != &process_tab[0]))
		   )
		{
			SET_ERROR(ERR_INVALID_SID);
			return;
		}
	
		/* Are we allowed to do that */
		if (l__testsec == 1)
		{
			if (   THREAD(subj, THRTAB_PROCESS_SID) 
			    != current_p[PRCTAB_SID]
			   )
			{
				SET_ERROR(ERR_ACCESS_DENIED);
				return;
			}
		}
		
		/* Is its freeze counter at the last possible value? */
		if (THREAD(subj, THRTAB_FREEZE_COUNTER) == 0xFFFFFFFF)
		{
			/* Just return. It's cold enough. */
			return;
		}
			
		/* Increase its freeze counter */
		THREAD(subj, THRTAB_FREEZE_COUNTER) ++;
		THREAD(subj, THRTAB_THRSTAT_FLAGS) |= THRSTAT_FREEZED;
				
		/* Should we remove it from the run queue? (Only if it is not already in busy mode!) */
		if (    (THREAD(subj, THRTAB_FREEZE_COUNTER) == 1)
		     && (!(THREAD(subj, THRTAB_THRSTAT_FLAGS) & THRSTAT_OTHER_FREEZE))
		   )
		{
			ksched_stop_thread(&(THREAD(subj, 0)));
						
			if (current_t[THRTAB_SID] == THREAD(subj, THRTAB_SID))
			{
				/* Yield the current thread */
				ksched_change_thread = true;
				ksched_next_thread();
			}
			
		}
		
		return;
	}

	/* Invalid SID */
	SET_ERROR(ERR_INVALID_SID);
	return;	
}

/*
 * sysc_yield_thread(subj)
 *
 * (Implementation of the "yield_thread" system call)
 *
 * Emits the effective priority of the current thread, 
 * so the thread will lose the control over the CPU. It 
 * can pass the rest of its current effective priority
 * to another thread.
 *
 * Parameters:
 *	subj	SID of a thread that should receive
 *		the current effective priority
 *
 */
void sysc_yield_thread(sid_t subj)
{
	/* 
	 * If another thread should receive the rest of the
	 * current thread's effective priority
	 *
	 */
	if (   (subj != SID_PLACEHOLDER_NULL)
	    && (subj != SID_PLACEHOLDER_INVALID)
	    && (subj != 0x1000000)
	   )
	{
		if (!kinfo_isthrd(subj))
		{
			SET_ERROR(ERR_INVALID_SID);
			return;
		}
		
		/* Increase the effective priority of the selected thread */
		THREAD(subj, THRTAB_EFFECTIVE_PRIORITY) 
			+= current_t[THRTAB_EFFECTIVE_PRIORITY];
	}

	/* The current thread loses its effective priority */	
	current_t[THRTAB_EFFECTIVE_PRIORITY] = 0;	
		
	/* Yield the current thread */
	ksched_change_thread = true;
	ksched_next_thread();
	
	/* Return to user mode */
	return;
}

/*
 * ksched_idle_loop
 *
 * This function contains the loop of the idle thread.
 *
 */
void ksched_idle_loop(void)
{
	while(1)
	{
		/* Reduce our effective priority to 0 */
		current_t[THRTAB_EFFECTIVE_PRIORITY] = 0;
		/* 
		 * Activate IRQs and sleep until 
		 * new IRQs are arriving
		 *
		 */
		__asm__ __volatile__(
				     "STI\n"
		    		     "HLT\n"
		   		    );
		
	}
}



