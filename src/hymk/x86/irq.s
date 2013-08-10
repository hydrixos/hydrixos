###########################################################################
#
#
# HydrixOS x86 interrupt managing and handling
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
# Informations for thread switching
#
.global i386_new_stack_pointer
.global i386_old_stack_pointer
.extern ksched_change_thread
.extern i386_esp0_ret
.extern ksched_debug_stack

.extern kinfo_io_map

.global i386_do_context_switch
.global i386_yield_kernel_thread

#
# Informations for IRQ handling
#
.extern ksched_handle_irq
.extern kinfo_eff_prior
.extern kinfo_rtc_ctr

#
# Informations for exception handling
#
.global i386_saved_error_code
.global i386_saved_error_cs
.global i386_saved_error_eip
.global i386_saved_error_esp
.global i386_saved_error_num
.global	i386_error_kernel_esp
.global i386_saved_last_block

#
# Information for Softint handling
#
.global i386_emptyint_handler
.extern i386_handle_emptyint


.code32
.text

#
# Thread switch via IRET
#
# Saved ESP
i386_saved_esp:
	.long	0

# Pointer to the buffer of the new ESP
i386_new_stack_pointer:
	.long	0
		
# Pointer to the buffer of the old ESP
i386_old_stack_pointer:
	.long	0

# Position of the ESP after last kernel entrance
i386_saved_last_block:
	.long 	0
	
#
# Exception handling
#
# Exception error code
i386_saved_error_code:
	.long	0

# CS during the exception
i386_saved_error_cs:
	.long	0

# EIP during the exception
i386_saved_error_eip:
	.long	0
	
# ESP during the exception
i386_saved_error_esp:
	.long	0

# Occured exception	
i386_saved_error_num:
	.long	0

# Kernel-ESP during exception
i386_error_kernel_esp:
	.long	0
	
###########################################################################
#
# i386_do_context_switch
#
# Return from kernel mode after handling
#
#	- Unused Softing
#	- System call
#	- IRQ
#	- User-Mode Exception
#
###########################################################################
i386_do_context_switch:
	#
	# Do we have to perform a task switch ?
	#
	cmpl	$1, ksched_change_thread	# ksched_change_tread signals it
	jb	i386_do_context_switch_ret	# No, just return to the interrupted thread

i386_do_context_switch_1:
	# Yes...	
	movl	$0, ksched_change_thread	# Reset ksched_change_thread
	
	#
	# Save the user mode ESP of the interrupted thread to its
	# ESP buffer
	#		
	movl	i386_old_stack_pointer, %eax
	movl	%esp, (%eax)	
	
	#
	# Load the user mode ESP of the destination thread that
	# should started after leaving the INT handler
	#
	movl	i386_new_stack_pointer, %eax
	movl	(%eax), %esp	
	
i386_do_context_switch_ret:
	#
	# Set returning kernel stack
	#
	
	# Load current kernel stack
	movl	%esp, %eax
	addl	$68, %eax	
	
	# Save the kernel stack pointer to the TSS
	movl	i386_esp0_ret, %ebx
	movl	%eax, (%ebx)
		
	#
	# Restore the I/O-Permission map
	#
	movl	kinfo_io_map, %eax		# Get the I/O permission map
	movl	(%eax), %ebx
	
	# If port access is allowed
	andl	$2, %ebx
	jz	i386_do_context_switch_noio
	
	# Set IOPL = 3
	movl	%esp, %ebx
	movl	56(%ebx), %eax
	orl	$0x3000, %eax
	movl	%eax, 56(%ebx)

	jmp	i386_do_context_switch_lastret

	#
	# Set IOPL = 0
	#
i386_do_context_switch_noio:
	movl	%esp, %ebx
	movl	56(%ebx), %eax
	andl	$0xFFFFCFFF, %eax
	movl	%eax, 56(%ebx)
	
i386_do_context_switch_lastret:		
	#
	# Restore the registers of the interrupted thread
	#
	
	/*call	ksched_debug_stack	# <- Activate for stack debugging*/
	
	popal
		
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
			
	#
	# Return to it
	#
	iretl
	
###########################################################################
#
# i386_awake_kernel_thread
#
# Return to the system call that yielded this thread in kernel mode.
#
###########################################################################
i386_awake_kernel_thread:
	#
	# It is easy, isn't it?
	#
	ret	
	
###########################################################################
#
# i386_yield_kernel_thread
#
# The current system call gives the CPU to the control of another thread.
#
###########################################################################
i386_yield_kernel_thread:
	#
	# Create a return address
	#
	pushfl
	pushl	%cs
	pushl	$i386_awake_kernel_thread	# Just to return to the right point
	#
	# Save registers
	#
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	pushal
	
	#
	# Return to the next thread
	#
	jmp	i386_do_context_switch
	
###########################################################################
#
# MIRQ
#
# i386_irhandleasm_X
#
# Contains the standard low-level IRQ handler as GAS macro
#
# The macro itselfs has the name "MIRQ"
#
# The macro parameter 'M_exnum' will define the prefix of the function
# that is created by the macro. The number 'M_pnum' defines whether
# a master (0x20) or a slave (0xa0) IRQ should be handled by the function
# builded out of this macro.
#
###########################################################################
.macro MIRQ M_irqnum, M_pnum

.global i386_irqhandleasm_\M_irqnum

i386_irqhandleasm_\M_irqnum:
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
	# Set the kernel segment selectors, so
	# we can access the kernel memory
	#
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs
	
	#
	# Save the last ESP position for different purposes
	#        
	#movl	%esp, %eax
	#movl	%eax, i386_saved_last_block
	
	#
        # Call the kernel low-level handler
        #
        pushl	$\M_pnum
	pushl	%eax
	pushl	$\M_irqnum
	call	ksched_handle_irq
        addl	$12, %esp			# Never reached

	#
	# Set IRQ = Handled
	#
	movb	$0x20, %al
        outb	%al, $\M_pnum
	
	#
        # Return to the current thread
        #
	jmp i386_do_context_switch
        
.endm

###########################################################################
#
# MEX
#
# i386_exhandleasm_X
#
# The kernel low-level handler for exceptions as GAS macro. The macro has
# the name 'MEX'.
#
# The macro parameter 'M_exnum' will define the prefix of the function
# that is created by the macro. 'M_errcodepop' will define whether
# the excpetion handler should search an error code or not.
#
###########################################################################

.macro MEX M_exnum, M_errcodepop

.global i386_exhandleasm_\M_exnum

i386_exhandleasm_\M_exnum:
	cli

	#
	# Save old DS and EAX
	#
	pushl	%ds
	pushl	%eax
				
	#
	# Set the kernel segment selector to DS, to
	# make access to kernel memory possible
	#
	movw	$0x20, %ax
	movw	%ax, %ds
	
	#
	# Save the error informations
	#
	movl	20 + \M_errcodepop(%esp), %eax
	movl	%eax, i386_saved_error_esp
	movl	%esp, i386_error_kernel_esp
		
	movl	12 + \M_errcodepop(%esp), %eax
	movl	%eax, i386_saved_error_cs
	movl	8 + \M_errcodepop(%esp), %eax
	movl	%eax, i386_saved_error_eip
	movl	4 + \M_errcodepop(%esp), %eax
	movl	%eax, i386_saved_error_code
	
	movl	$\M_exnum, i386_saved_error_num

	popl	%eax
	popl	%ds

	#
	# Remove error code from stack
	#	
	add	$\M_errcodepop, %esp

	#
	# Save old status
	#
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs	

	pushal
	
	#
	# Load kernel data segment
	#
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs			
	
	#
	# Set error code to 0 if there is no error code
	# (it is depending on the macro parameters)
	#
	.ifc \M_errcodepop, no
		movl	$0, i386_saved_error_code
	.endif

        #
        # Call kernel handler
        #
	pushl	%ebp
        call	ksched_handle_except
	popl	%ebp
	
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch
.endm

###########################################################################
#
# i386_emptyint_handler
#
# Handles an unused software interrupt
#
###########################################################################
.global i386_emptyint_handler

i386_emptyint_handler:
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
	# Read the number of the last executed
	# INT instruction to find out which soft
	# int was called
	#
	
	# Read EIP from stack
	movl	%esp, %eax
	addl	$48, %eax	
	movl	%ss:(%eax), %ebx
	
	# load the parameter of the INT instruction
	xorl	%edx, %edx
	subl	$1, %ebx
	movb	%ds:(%ebx), %dl
	
	#
	# Load the kernel segment descriptor
	#
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	$0x2b, %ax
	movw	%ax, %gs	

	#
	# Save the current kernel ESP for different
	# purposes
	#	
	movl	%esp, i386_saved_last_block
	
	#
	# Call handler for empty software interrupts
	#
	pushl	%edx
	call	i386_handle_emptyint	
	popl	%edx
	
        #
        # Return to the current thread
        #
	jmp i386_do_context_switch

#
# IRQ handler function symbols "created" by calling the MIRQ macro
#
MIRQ 0, 0x20
MIRQ 1, 0x20
MIRQ 2, 0x20
MIRQ 3, 0x20
MIRQ 4, 0x20
MIRQ 5, 0x20
MIRQ 6, 0x20
MIRQ 7, 0x20
MIRQ 8, 0xA0
MIRQ 9, 0xA0
MIRQ 10, 0xA0
MIRQ 11, 0xA0
MIRQ 12, 0xA0
MIRQ 13, 0xA0
MIRQ 14, 0xA0
MIRQ 15, 0xA0

#
# Exception handler symbols "created" by calling the MEX macro
#
MEX 0, 0
MEX 1, 0
MEX 2, 0
MEX 3, 0
MEX 4, 0
MEX 5, 0
MEX 6, 0
MEX 7, 0
MEX 8, 4
MEX 9, 0
MEX 10, 4
MEX 11, 4
MEX 12, 4
MEX 13, 4
MEX 14, 4
MEX 15, 0
MEX 16, 0
MEX 17, 4
MEX 18, 0
MEX 19, 0
MEX 20, 0
MEX 21, 0
MEX 22, 0
MEX 23, 0
MEX 24, 0
MEX 25, 0
MEX 26, 0
MEX 27, 0
MEX 28, 0
MEX 29, 0
MEX 30, 0
MEX 31, 0
