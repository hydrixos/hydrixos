###########################################################################
#
#
# HydrixOS x86 low-level system call handlers
#
# (C)2004 by Friedrich Gräter
#
# This file is distributed under the terms of
# the GNU General Public License, Version 2. You
# should have received a copy of this license 
# (e.g. in the file 'copying'). 
#
#
###########################################################################

.arch i386
#
# External symbols
#
.extern i386_do_context_switch
.extern i386_saved_last_block

.extern sysc_error

.extern i386_emptyint_handler

.extern current_t

#
# System call exports
#
.global i386_sysc_alloc_pages

.global i386_sysc_create_thread
.global i386_sysc_create_process
.global i386_sysc_set_controller
.global i386_sysc_destroy_subject

.global i386_sysc_chg_root

.global i386_sysc_freeze_subject
.global i386_sysc_awake_subject
.global i386_sysc_yield_thread
.global i386_sysc_set_priority

.global i386_sysc_allow
.global i386_sysc_map
.global i386_sysc_unmap
.global i386_sysc_move

.global i386_sysc_sync

.global i386_sysc_io_allow
.global i386_sysc_io_alloc
.global i386_sysc_recv_irq

.global i386_sysc_recv_softints
.global i386_sysc_decv_softints
.global i386_sysc_read_regs
.global i386_sysc_write_regs

.global i386_sysc_set_paged
.global i386_sysc_test_page

#
# System call impotrs
#
.extern sysc_alloc_pages

.extern sysc_create_thread
.extern sysc_create_process
.extern sysc_set_controller
.extern sysc_destroy_subject

.extern sysc_chg_root

.extern sysc_freeze_subject
.extern sysc_awake_subject
.extern sysc_yield_thread
.extern sysc_set_priority

.extern sysc_allow
.extern sysc_map
.extern sysc_unmap
.extern sysc_move

.extern sysc_sync

.extern sysc_io_allow
.extern sysc_io_alloc
.extern sysc_recv_irq

.extern sysc_recv_softints
.extern sysc_decv_softints
.extern sysc_read_regs
.extern sysc_write_regs

.extern sysc_set_paged

.code32
.text

######################################################
#
#
# System call
#
#	Low Level Handler
#
#
######################################################

#
# sysc_alloc_pages
# 
# ISR:	0xC0
#
# In:
#	EAX	Start address of the mapping
#	EBX	Count of pages
#
# Out:
#	EAX	Error code
#
i386_sysc_alloc_pages:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_alloc_pages_norm	# Normal system call
	
	# Redirect it
	pushal
	pushl	$0xC0
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_alloc_pages_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_alloc_pages_norm:				
	popl	%ebp
	popl	%eax
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ebx
	pushl	%eax
	call	sysc_alloc_pages
	addl	$8, %esp

	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error	
	
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_create_thread
# 
# ISR:	0xC1
#
# In:
#	EAX	Start address of the new thread
#	EBX	The ESP of the new thread
#
# Out:
#	EAX	Error code
#	EBX	The SID of the new thread
#
i386_sysc_create_thread:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_create_thread_norm	# Normal system call
	
	# Redirect it
	pushal
	pushl	$0xC1
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_create_thread_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_create_thread_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ebx
	pushl	%eax
	call	sysc_create_thread
	addl	$8, %esp
	
	#
	# Writing the return value (EAX register) to EBX
	#
	movl	%eax, 16(%esp)
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
	
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_create_process
# 
# ISR:	0xC2
#
# In:
#	EAX	Start address of the new thread
#	EBX	The ESP of the new thread
#
# Out:
#	EAX	Error code
#	EBX	The SID of the new process
#
i386_sysc_create_process:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_create_process_norm	# Normal system call
	
	# Redirect it
	pushal
	pushl	$0xC2
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_create_process_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_create_process_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ebx
	pushl	%eax
	call	sysc_create_process
	addl	$8, %esp
	
	#
	# Writing the return value (EAX register) to EBX
	#
	movl	%eax, 16(%esp)
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_set_controller
#
# ISR:	0xC3
#
# In:
#	EAX	SID of the new controller thread
#
# Out:
#	EAX	Error code
#
i386_sysc_set_controller:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_set_controller_norm
	
	# Redirect it
	pushal
	pushl	$0xC3
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_set_controller_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_set_controller_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%eax
	call	sysc_set_controller
	addl	$4, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch


#
# sysc_destroy_subject
#
# ISR:	0xC4
#
# In:
#	EAX	SID of the affected thread or process
#
# Out:
#	EAX	Error code
#
i386_sysc_destroy_subject:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_destroy_subject_norm
	
	# Redirect it
	pushal
	pushl	$0xC4
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_destroy_subject_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_destroy_subject_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%eax
	call	sysc_destroy_subject
	addl	$4, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_chg_root
#
# ISR:	0xC5
#
# In:
#	EAX	SID of the affected process
#	EBX	Changing access rights
#
# Out:
#	EAX	Error code
#
i386_sysc_chg_root:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_chg_root_norm
	
	# Redirect it
	pushal
	pushl	$0xC5
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_chg_root_norm		# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_chg_root_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ebx
	pushl	%eax
	call	sysc_chg_root
	addl	$8, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_freeze_subject
#
# ISR:	0xC6
#
# In:
#	EAX	SID of the subject that should be freezed
#
# Out:
#	EAX	Error code
#
i386_sysc_freeze_subject:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_freeze_subject_norm
	
	# Redirect it
	pushal
	pushl	$0xC6
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_freeze_subject_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_freeze_subject_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%eax
	call	sysc_freeze_subject
	addl	$4, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_awake_subject
#
# ISR:	0xC7
#
# In:
#	EAX	SID of the subject that should be awaked
#
# Out:
#	EAX	Error code
#
i386_sysc_awake_subject:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_awake_subject_norm
	
	# Redirect it
	pushal
	pushl	$0xC7
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_awake_subject_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_awake_subject_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%eax
	call	sysc_awake_subject
	addl	$4, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_yield_thread
#
# ISR:	0xC8
#
# In:
#	EAX	SID of the thread that should receive the rest of the priority
#
# Out:
#	EAX	Error code
#
i386_sysc_yield_thread:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_yield_thread_norm
	
	# Redirect it
	pushal
	pushl	$0xC8
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_yield_thread_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_yield_thread_norm:				
	popl	%ebp
	popl	%eax
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%eax
	call	sysc_yield_thread
	addl	$4, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_set_priority
#
# ISR:	0xC9
#
# In:
#	EAX	SID of the affected thread
#	EBX	New priority
#	ECX	New scheduling class
#
# Out:
#	EAX	Error code
#
i386_sysc_set_priority:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_set_priority_norm
	
	# Redirect it
	pushal
	pushl	$0xC9
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_set_priority_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_set_priority_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_set_priority
	addl	$12, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_allow
#
# ISR:	0xCA
#
# In:
#	EAX	SID of the other side
#	EBX	SID that is affected by the allow instruction
#	ECX	Start address
#	EDX	Number of pages
#	EDI	Allowed operations
#
# Out:
#	EAX	Error code
#
i386_sysc_allow:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_allow_norm
	
	# Redirect it
	pushal
	pushl	$0xCA
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_allow_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_allow_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%edi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_allow
	addl	$20, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_map
#
# ISR:	0xCB
#
# In:
#	EAX	SID of the other side
#	EBX	Start address
#	ECX	Number of pages
#	EDX	Flags
#	EDI	Offset in the destination area
#
# Out:
#	EAX	Error code
#
i386_sysc_map:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_map_norm
	
	# Redirect it
	pushal
	pushl	$0xCB
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_map_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_map_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%edi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_map
	addl	$20, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_unmap
#
# ISR:	0xCC
#
# In:
#	EAX	SID of the other side
#	EBX	Start address
#	ECX	Number of pages
#	EDX	Flags
#
# Out:
#	EAX	Error code
#
i386_sysc_unmap:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_unmap_norm
	
	# Redirect it
	pushal
	pushl	$0xCC
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_unmap_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_unmap_norm:				
	popl	%ebp
	popl	%eax	
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_unmap
	addl	$16, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_move
#
# ISR:	0xCD
#
# In:
#	EAX	SID of the other side
#	EBX	Start address
#	ECX	Number of pages
#	EDX	Flags
#	EDI	Offset in the destination area
#
# Out:
#	EAX	Error code
#
i386_sysc_move:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_move_norm
	
	# Redirect it
	pushal
	pushl	$0xCD
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_move_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_move_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%edi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_move
	addl	$20, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_sync
#
# ISR:	0xCE
#
# In:
#	EAX	SID of the other side
#	EBX	Duration of timeout
#	ECX	Count of resync operations
#
# Out:
#	EAX	Error code
#	EBX	SID of the other side
#
i386_sysc_sync:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_sync_norm
	
	# Redirect it
	pushal
	pushl	$0xCE
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_sync_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_sync_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_sync
	addl	$12, %esp
	
	#
	# Writing the return value (EAX register) to EBX
	#
	movl	%eax, 16(%esp)
		
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_io_allow
#
# ISR:	0xCF
#
# In:
#	EAX	SID of the affected process
#	EBX	Flags
#
# Out:
#	EAX	Error code
#
i386_sysc_io_allow:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_io_allow_norm
	
	# Redirect it
	pushal
	pushl	$0xCF
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_io_allow_norm		# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_io_allow_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ebx
	pushl	%eax
	call	sysc_io_allow
	addl	$8, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_io_alloc
#
# ISR:	0xD0
#
# In:
#	EAX	Source address
#	EBX	Destination address
#	ECX	Number of pages
#	EDX	Flags
#
# Out:
#	EAX	Error code
#
i386_sysc_io_alloc:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
		
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_io_alloc_norm
	
	# Redirect it
	pushal
	pushl	$0xD0
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_io_alloc_norm		# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_io_alloc_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_io_alloc
	addl	$16, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_recv_irq
#
# ISR:	0xD1
#
# In:
#	EAX	Number of IRQ to observate
#
# Out:
#	EAX	Error code
#
i386_sysc_recv_irq:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_recv_irq_norm
	
	# Redirect it
	pushal
	pushl	$0xD1
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_recv_irq_norm		# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_recv_irq_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%eax
	call	sysc_recv_irq
	addl	$4, %esp
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_recv_softints
#
# ISR:	0xD2
#
# In:
#	EAX	SID of the affected thread
#	EBX	Timeout
#	ECX	Flags
#		RECV_AWAKE_OTHER	1
#
# Out:
#	EAX	Error code
#	EBX	Occured softint
#
i386_sysc_recv_softints:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_recv_softints_norm
	
	# Redirect it
	pushal
	pushl	$0xD2
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_recv_softints_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_recv_softints_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_recv_softints
	addl	$12, %esp
	
	#
	# Writing the return value (EAX register) to EBX
	#
	movl	%eax, 16(%esp)
		
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_read_regs
#
# ISR:	0xD3
#
# In:
#	EAX	SID of the affected thread
#	EBX	Type of the register block
#
# Out:
#	EAX	Error code
#	EBX	(1)
#	ECX	(2)
#	EDX	(3)
#	ESI	(4)
#
i386_sysc_read_regs:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	#movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_read_regs_norm
	
	# Redirect it
	pushal
	pushl	$0xD3
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_read_regs_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_read_regs_norm:
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ebx
	pushl	%eax
	call	sysc_read_regs
	addl	$8, %esp
		
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_write_regs
#
# ISR:	0xD4
#
# In:
#	EAX	SID of the affected thread
#	EBX	Type of the register block
#	ECX	(1)
#	EDX	(2)
#	ESI	(3)
#	EDI	(4)
#
# Out:
#	EAX	Error code
#
i386_sysc_write_regs:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	#movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_write_regs_norm
	
	# Redirect it
	pushal
	pushl	$0xD4
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_write_regs_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_write_regs_norm:
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	sysc_write_regs
	addl	$24, %esp
		
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# sysc_set_paged
#
# ISR:	0xD5
#
# In:
#	None.
#
# Out:
#	EAX	Error code
#
i386_sysc_set_paged:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	#movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_set_paged_norm
	
	# Redirect it
	pushal
	pushl	$0xD5
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_set_paged_norm		# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_set_paged_norm:
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	call	sysc_set_paged
	
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	
#
# sysc_test_page
#
# ISR:	0xD6
#
# In:
#	EAX	Address of the page which is to test
#	EBX	SID of the address space which contains that page
#
# Out:
#	EAX	Error code
#	EBX	Access rights
#
i386_sysc_test_page:
	#
	# Save all registers to stack
	#
	cli
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
		
	pushal
	
	#
	# Load the kernel segment descriptor
	#
	pushw	%ax
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	
	popw	%ax

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	##movl	%esp, i386_saved_last_block
	
	#
	# Redirect the system call if we are selected for
	# recv_softints
	#
	pushl	%eax
	pushl	%ebp
	movl	current_t, %ebp
	addl	$60, %ebp		# THRTAB_SOFTINT_LISTENER_SID
	movl	(%ebp), %eax
	cmpl	$0, %eax
	je	i386_sysc_test_page_norm
	
	# Redirect it
	pushal
	pushl	$0xD6
	call	kremote_received
	addl	$4, %esp
	cmpl	$0, %eax
	popal
	jne	i386_sysc_test_page_norm	# Normal execution, if Trace-Only
	
	# Return to user mode
	popl	%ebp
	popl	%eax
	jmp	i386_do_context_switch

i386_sysc_test_page_norm:				
	popl	%ebp
	popl	%eax		
	
	#
	# Call system call handler (push and pop
	# additional parameters and return values)
	#
	pushl	%ebx
	pushl	%eax
	call	sysc_test_page
	addl	$8, %esp
	
	#
	# Writing the return value (EAX register) to EBX
	#
	movl	%eax, 16(%esp)
		
	#
	# Write the error status to the eax register of the
	# returning thread
	#
	movl	sysc_error, %eax
	movl	%eax, 28(%esp)
	movl	$0, %eax
	movl	%eax, sysc_error
		
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
	