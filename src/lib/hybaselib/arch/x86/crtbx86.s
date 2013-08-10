###########################################################################
#
#
# HydrixOS x86
#
# Startup code for GRUB modules
#
# (C)2005 by Friedrich Gräter
#
# This file is distributed under the terms of
# the GNU Lesser General Public License, Version 2. You
# should have received a copy of this license (e.g.
# in the file 'copying.library'). 
#
###########################################################################

.arch i386

.global start
.global _start
.extern crt_entry

.global lib_grub_module_start
.global lib_grub_module_pages

.code32
.text

_start:       
start:
	#
	# Save the kernel boot informations
	#
	movl	%eax, lib_grub_module_start
	movl	%ebx, lib_grub_module_pages
	
	#
	# Enter the generic start up code
	#
	jmp	crt_entry

#
# REMEMBER: This is only a temporary solution !
#
lib_grub_module_start:
	.long	0
lib_grub_module_pages:
	.long	0
