###########################################################################
#
#
# HydrixOS x86
#
# Startup code for init forks
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


.extern main
.extern initfork_stack_buf
.extern initfork_thread_buf
.extern initfork_libinit

.global initfork_entry

.code32
.text

#
# The entry code of a hyBaseLib program
#
initfork_entry:
	#
	# Okay, we already have a working stack and 
	# everything needed to build a valid TLS.
	# So we just do it :).
	#
	movl initfork_stack_buf, %eax		# Load our stack pointer
	movl %eax, %esp
	
	call initfork_libinit			# Re-initialize the library
	
	call main				# Start main
	
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
		
	