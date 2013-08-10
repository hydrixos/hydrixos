/*
 *
 * errno.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * System error codes
 *
 */
#ifndef _ERRNO_H
#define _ERRNO_H

/*
 * Exceptions
 *
 */
#define EXC_DIVISION_BY_ZERO		1
#define EXC_PROTECTION_FAULT		2
#define EXC_INVALID_INSTRUCTION		3
#define EXC_TRAP			4
#define EXC_OVERFLOW			5
#define EXC_STACK_FAULT			6
#define EXC_INVALID_PAGE		7
#define EXC_SPECIFIC			8
#define EXC_FLOATING_POINT		9

/*
 * System call errors
 *
 */
#define ERR_NO_ERROR			0x0
#define ERR_NOT_ROOT			0x1
#define ERR_ACCESS_DENIED		0x2
#define ERR_RESOURCE_BUSY		0x3
#define ERR_TIMED_OUT			0x4
#define ERR_INVALID_ARGUMENT		0x5
#define ERR_INVALID_SID			0x6
#define ERR_INVALID_ADDRESS		0x7
#define ERR_NOT_ENOUGH_MEMORY		0x8
#define ERR_PAGES_LOCKED		0x9
#define ERR_PAGING_DAEMON		0xA
#define ERR_SYSCALL_RESTRICTED		0xB

/*
 * i386 Exception codes
 *
 */
#define EXC_X86_DIVISION_BY_ZERO		0
#define EXC_X86_DEBUG_EXCEPTION			1
#define EXC_X86_NOT_MASKABLE_INTERRUPT		2
#define EXC_X86_BREAKPOINT			3
#define EXC_X86_OVERFLOW_EXCEPTION		4
#define EXC_X86_BOUND_RANGE_EXCEEDED		5
#define EXC_X86_INVALID_OPCODE			6
#define EXC_X86_DEVICE_NOT_AVAILABLE		7
#define EXC_X86_DOUBLE_FAULT			8
#define EXC_X86_COPROCESSOR_SEGMENT_OVERRUN	9
#define EXC_X86_INVALID_TSS			10
#define EXC_X86_SEGMENT_NOT_PRESENT		11
#define EXC_X86_STACK_FAULT			12
#define EXC_X86_GENERAL_PROTECTION_FAULT	13
#define EXC_X86_PAGE_FAULT			14
#define EXC_X86_UNKNOWN				15
#define EXC_X86_FLOATING_POINT_ERROR		16
#define EXC_X86_ALIGNMENT_CHECK			17
#define EXC_X86_MACHINE_CHECK			18
#define EXC_X86_SIMD_FLOATING_POINT_ERROR	19

#define EXC_LAST_VALID_X86_EXCEPTION		19

/*
 * hyAPI error codes
 *
 */
#define ERR_IMPLEMENTATION_ERROR		0x100
#define ERR_MEMORY_CORRUPTED			0x101
#define ERR_SYNTAX_ERROR			0x102
#define ERR_OUT_OF_DATA				0x103

#endif
