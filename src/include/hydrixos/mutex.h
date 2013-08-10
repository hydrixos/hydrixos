/*
 *
 * mutex.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * User-level Mutexes
 *
 */ 
#ifndef _MUTEX_H
#define _MUTEX_H

#include <hydrixos/types.h>
#include <hydrixos/hysys.h>

#ifdef HYDRIXOS_x86
/* Mutex data type */
typedef struct mtx_st {
	volatile int	mutex;
	volatile int	latency;
}mtx_t;
#endif

/* Mutex control functions */
int mtx_trylock(mtx_t *mutex);
int mtx_lock(mtx_t *mutex, long timeout);
void mtx_unlock(mtx_t *mutex);

#define MTX_NEW()	((struct mtx_st){0, 0});
#define MTX_DEFINE()	{0, 0}

#define MTX_NO_TIMEOUT		0
#define MTX_UNLIMITED		-1

#endif
