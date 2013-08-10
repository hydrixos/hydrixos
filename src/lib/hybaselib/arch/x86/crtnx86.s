###########################################################################
#
#
# HydrixOS x86
#
# Startup code for normal programs
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

.code32
.text

_start:       
start:
	#
	# Enter the generic start up code
	#
	jmp	crt_entry
