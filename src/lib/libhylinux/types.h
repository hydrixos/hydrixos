/*
 *
 * types.h
 *
 * (C)2001, 2002 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. 
 * in the file 'copying'). 
 *
 * Standard type definitions
 *
 *
 */
#ifndef _TYPES_H
#define _TYPES_H

#include <sys/types.h>
#include <stdint.h>

/*
 * Standard definitions
 *
 */
#define TRUE	(0==0)
#define FALSE	(0==1)
#define true	TRUE
#define false	FALSE

#define NULL	0
#define null	NULL

/*
 * Exact-with types
 * (see ANSI-C99)
 *
 *
typedef signed char			int8_t;
typedef signed short int		int16_t;
typedef signed long int		 	int32_t;
typedef signed long long int		int64_t;

typedef unsigned char			uint8_t;
typedef unsigned short int		uint16_t;
typedef unsigned long int		uint32_t;
typedef unsigned long long int		uint64_t;

typedef int32_t		intptr_t;
typedef uint32_t	uintptr_t;

typedef unsigned long int			size_t;
typedef int32_t		bool;
typedef bool		bool_t;*/

/*
 * Virtual exact-width HydrixOS types
 *
 */
typedef uint64_t		vaddress_t;
typedef uint64_t		vsize_t;

/*
 * Exact-with floating point types
 *
 *
typedef float			float32_t;
typedef double			float64_t;
typedef long double		float80_t;*/	

/*
 * Charracter types
 *
 */
typedef int8_t			utf8_t;

/*
 * System types
 *
 */
typedef uint32_t		sid_t;
typedef uint32_t		errno_t;
typedef uint32_t		irq_t;

#endif

