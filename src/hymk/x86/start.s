###########################################################################
#
#
# HydrixOS x86 kernel startup code
#
# (C)2004 by Friedrich Gräter
#
# This file is distributed under the terms of
# the GNU General Public License, Version 2. You
# should have received a copy of this license (e.g.
# in the file 'copying'). 
#
###########################################################################

.global start
.global _start
.global i386_idt_des_new
.global i386_idt_s
.global i386_gdt_des_new
.global i386_gdt_s
.global i386_boot_info

.extern main

CS_SELECTOR		=	0x08
DS_SELECTOR		=	0x10
CS_KERNEL		=	0x18
DS_KERNEL		=	0x20
CS_USER			=	0x28
DS_USER			=	0x30
TSS_SELECTOR		=	0x38

#
# -------------------------------------------------------------------------
#
#   32-bit PROTECTED MODE WORLD
#
# -------------------------------------------------------------------------
#

.code32
.text

_start:       
start:
	jmp     i386_entry_code
     
	#
	# GNU Hurd Multiboot header
	#
	#
	.align  4
i386_multiboot_hdr:
	.long   0x1BADB002				# Magic Code
	.long   0x00000003				# Flags
	.long   -(0x1BADB002 + 0x00000003)		# Checksum

#
# i386 Entry code
#	     
i386_entry_code:
	cmpl	$0x2BADB002, %eax
	jne	die
		
	#
	# basic setup
	#
	movw	$0x10, %ax			# setup DS,ES,SS to 0x10
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs
   	movl	$0x90000, %esp

	movl	%ebx, i386_boot_info
	#
	# Clear unwanted flags
	#
	pushl	$0
	popfl

	#
	# Load GDT + IDT again
	#
	lgdt	i386_gdt_des
	lidt	i386_idt_des

	# 
	# Set GS to the userspace data segment
	#
	movw	$0x2b, %ax
	movw	%ax, %gs

	#
	# okay that was easy :). The rest is done by the kernel
	#
	pushl	$0
	pushl	$0

	jmp main

die:
	jmp die
	
	#
	# The descriptor tables
	#
	.align 16
i386_gdt_s:
	.word	0,0,0,0			# dummy

	#
	# Flat memory
	#
	.word	0xFFFF			# 4 GB of memory 
	.word	0			# base address = 0
	.word	0x9A00			# code read/exec
	.word	0x00CF			# 4 KB Segs. 386 sys

	.word	0xFFFF			# 4 GB of memory
	.word	0			# base address = 0
	.word	0x9200			# data r/w
	.word	0x00CF			# 4 KB Segs. 386 sys

	# -----------------------------------------
	#
	# Direct user <-> kernel transfer for 
	# 32 bit programs
	#
	# -----------------------------------------
	
	#
	# Kernel
	#
	.word	0x0000			# 1 GiB of memory
	.word	0			# base address = 3 GiB
	.word	0x9A00			# code read/exec
	.word	0xC0C4			# 4 KiB Segs. 386 sys

	.word	0x0000			# 1 GiB of memory
	.word	0			# base address = 3 GiB
	.word	0x9200			# data read/write
	.word	0xC0C4			# 4 KiB Segs. 386 sys
	
	#
	# User
	#
	.word	0xFFFF			# 4 GiB of memory
	.word	0			# base address = 0
	.word	0xFA00	     		# code read/exec
	.word	0x00CF	     		# 4 KiB Segs. 386 CPL3

	.word	0xFFFF			# 4 GiB of memory
	.word	0			# base address = 0
	.word	0xF200	     		# data read/write
	.word	0x00CF	     		# 4 KiB Segs. 386 CPL3	
		
	# -----------------------------------------
	#
	# TSS descriptor
	#
	# -----------------------------------------
		
	.word	200			# Size 100 Byte
	.word	0			# Set Base later...
	.word	0x8900			# Task, inactive.
	.word	0x0000			# 
	

#
# Normal IDT
#	
	.align 16
i386_idt_s:
	.fill 256,8,0			# IDT is uninitialized

	#
	# The pseudo-descriptors
	#
	.align 2
	.word 0
i386_gdt_des:
	.word	64
	.long	i386_gdt_s
	
i386_gdt_des_new:
	.word	64
	.long	i386_gdt_s + (3 * 1024 * 1024 * 1024)


.align 16

i386_idt_des:
	.word	2048
	.long	i386_idt_s

i386_idt_des_new:
	.word	2048
	.long	i386_idt_s + (3 * 1024 * 1024 * 1024)
	
i386_boot_info:
	.long	0
