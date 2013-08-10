/*
 *
 * sid.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * SID definitions
 *
 *
 */
#ifndef _SID_H
#define _SID_H

#include <hydrixos/types.h>

#define SID_TYPE_MASK		0xff000000u
#define SID_DATA_MASK		0x00ffffffu

#define SIDTYPE_PLACEHOLDER	0x00000000u
#define SIDTYPE_THREAD		0x01000000u
#define SIDTYPE_PROCESS		0x02000000u
#define SIDTYPE_PROCESSGROUP	0x04000000u

#define SID_PLACEHOLDER_INVALID		0x00ffffffu
#define SID_PLACEHOLDER_NULL		0x00000000u
#define SID_PLACEHOLDER_KERNEL		0x00000001u

#define SID_INVALID			SID_PLACEHOLDER_INVALID
#define SID_NULL			SID_PLACEHOLDER_NULL
#define SID_KERNEL			SID_PLACEHOLDER_KERNEL

#define SID_USER_EVERYBODY		0x04ffffffu
#define SID_USER_ROOT			0x04000000u

#endif

