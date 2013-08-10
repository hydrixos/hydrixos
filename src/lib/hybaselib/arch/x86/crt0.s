###########################################################################
#
#
# HydrixOS x86
#
# Generic startup code
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

.extern lib_init_hybaselib
.extern tls_my_thread
.extern main
.global crt_entry

.extern blthr_init
.extern blthr_finish
.global blthr_init_arch

.code32
.text

#
# The entry code of a hyBaseLib program
#
crt_entry:
	#
	# At first we need a small, initial stack
	# for our new process to get the things running.
	# We just use the local TLS as our initial stack.
	# It has 2 KiB - this should be enough, because
	# lib_init_hybaselib won't do any complex things.
	#
	movl $0xBfffffff, %esp		# Change stack pointer
	
	#
	# lib_init_hybaselib
	#
	# This function will initialize the hyBaseLib. The
	# return value of this function contains our new stack
	# pointer.
	#
	call lib_init_hybaselib		# Initialize the hyBaseLib
	cmpl $0, %eax			# NULL?
	je   tmp_exit			# TODO: This is temporary
	
	movl %eax, %esp			# Change the stack pointer

	#
	# Enter the function "main"
	#
	call main			
	
	#
	# There is currently no exit()-function
	#
tmp_exit:
	int $0xB0			# Call an inactive interrupt, 
					# to give a signal, that we
					# have left init.

tmpa:	movl $0, %eax
	int  $0xC8			# yield_thread
	
	jmp tmpa
	
#
# blthr_init_arch
#
# Initializes a BlThread
# (Architecture-dependend part)
#
blthr_init_arch:
	call	blthr_init		# Call the non-architecture-dependend initialization
	
	call	blthr_finish		# Finish, if return.
	