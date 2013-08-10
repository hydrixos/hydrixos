/*
 *
 * hysys.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * Plattform specification
 *
 */
#ifndef _HYSYS_H
#define _HYSYS_H

#define HYDRIXOS_x86

#ifdef HYDRIXOS_x86
/* Plattform-dependend libary features */
#	define HYDRIXOS_USE_STDFUN_ARCH

/* Plattform description */
#	define ARCH_LITTLE_ENDIAN
#	define ARCH_LCHAR_UTF8
#	define ARCH_LSTR_UTF8
#	define ARCH_ICHAR_UTF8
#	define ARCH_ISTR_UTF8

#	define ARCH_PAGE_SIZE		4096u
#	define ARCH_PAGE_OFFSET_MASK	0xfffu
#	define ARCH_MAIN_INFO_PAGE	0xF8000000u
#	define ARCH_MAIN_INFO_SIZE	1024u
#	define ARCH_PROC_TABLE		0xF8001000u
#	define ARCH_PROC_TABLE_SIZE	2048u
#	define ARCH_PROC_TABLE_ENTRIES	4096u
#	define ARCH_THREAD_TABLE	0xFB001000u
#	define ARCH_THREAD_TABLE_SIZE	2048u
#	define ARCH_THREAD_TABLE_ENTRIES	4096u

#	define ARCH_STACK_SIZE		65536u
#endif

#endif
