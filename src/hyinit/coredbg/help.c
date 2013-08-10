/*
 *
 * help.c
 *
 * (C)2006 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Help screens for the debugger
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/blthr.h>
#include <hydrixos/mem.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>

#include <coredbg/cdebug.h>


#include "../hyinit.h"
#include "coredbg.h"
 
 /*
 * dbg_sh_version()
 *
 * Prints the version of the debugger.
 *
 * Usage:
 *	version [-e]
 *
 *	-v		Detailed information
 *
 */
int dbg_sh_version(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	
	if (dbg_test_par(1, "-v") == -1)
	{
		dbg_iprintf(l__shell->terminal, "HyCoreDbg - HydrixOS Core Debugger 0.0.1\nWrite \'version -v\' for more detailed informations.\n\n");
	}
	 else
	{
		
		dbg_iprintf(l__shell->terminal, "HyCoreDbg - HydrixOS Core Debugger 0.0.1\nCopright (C) 2006 by Friedrich Graeter (graeter@hydrixos.org)\nhttp://www.hydrixos.org/\n\n");
		dbg_iprintf(l__shell->terminal, "This program is distributed under the terms of\nthe GNU Lesser General Public License, Version 2.\nYou should have received a copy of this license\n(e.g. in the file 'copying').\n\n");
	}
	
	return 0;
}

 /*
 * dbg_sh_dbgtest()
 *
 * Simple testing routine of the debugger
 *
 */
int dbg_sh_dbgtest(void)
{
	volatile int l__i = 0;
	dbg_shell_t *l__shell = *dbg_tls_shellptr;

	int silent = 1;
	int yield = 1;

	if (dbg_test_par(1, "-s") == -1)
		silent = 0;
	if (dbg_test_par(1, "-y") == -1)
		yield = 0;

	while(1)
	{
		l__i ++; 

		if (!silent) dbg_iprintf(l__shell->terminal, "%i\n", l__i++);
		if (!yield) blthr_yield(0);
	}

	return 0;
}

/*
 * dbg_sh_help()
 *
 * Manual page of "command".
 *
 * Usage:	
 *	help [command]
 *
 */
int dbg_sh_help(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	
	if (dbg_test_par(1, "version") != -1)
	{
		dbg_iprintf(l__shell->terminal, "version - Prints informations about the version of the debugger\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tversion [-v]\n");
		dbg_iprintf(l__shell->terminal, "\t-v\tPrint more detailed informations\n");      
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "dbgtest") != -1)
	{
		dbg_iprintf(l__shell->terminal, "dbgtest - Just a simple program to test the debugger\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tdbgtest\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "term") != -1)
	{
		dbg_iprintf(l__shell->terminal, "term - Changes the current terminal\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tterm <number>\n");
		dbg_iprintf(l__shell->terminal, "\tnumber\tThe number of the terminal which should be\n");
		dbg_iprintf(l__shell->terminal, "\t      \tused in future by the current shell (valid 2-12).\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "start") != -1)
	{
		dbg_iprintf(l__shell->terminal, "start - Starts the client thread\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tstart\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "export") != -1)			
	{
		dbg_iprintf(l__shell->terminal, "export - Exports a value to a variable\n");
		dbg_iprintf(l__shell->terminal, "You can use this variable by writing its name with a trailing\n$ sign in a parameter (like $foo).\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\texport <var> <val>\n");
		dbg_iprintf(l__shell->terminal, "\t<var>\tThe name of the variable (a trailing $ will be ignored!)\n");
		dbg_iprintf(l__shell->terminal, "\t<val>\tThe new value of the variable\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "echo") != -1)			
	{
		dbg_iprintf(l__shell->terminal, "echo - Prints a text or a variable to the screen\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\techo <text>\n");
		dbg_iprintf(l__shell->terminal, "\t<text>\tThe text to print (using $ to print a variable)");
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "getreg") != -1)
	{
		dbg_iprintf(l__shell->terminal, "getreg - Returns the content of one or more registers of a thread\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tgetreg [-c <sid>] [[<reg1>] [<reg2>] ... [<regn>]]\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>\tThe command should print the registers of the thread\n");
		dbg_iprintf(l__shell->terminal, "\t        \twhich has the SID <sid>. If not specified the registers\n");
		dbg_iprintf(l__shell->terminal, "\t        \tof the current thread will be shown\n\n");
		dbg_iprintf(l__shell->terminal, "\t<regX>  \tShow the registers \"regX\" this may be:\n");
		dbg_iprintf(l__shell->terminal, "\t        \t\tEAX, EBX, ECX, EDX\n");
		dbg_iprintf(l__shell->terminal, "\t        \t\tESI, EDI, EBP, ESP\n");
		dbg_iprintf(l__shell->terminal, "\t        \t\tEIP, EFLAGS\n");
		dbg_iprintf(l__shell->terminal, "\t        \tYou can specify one or more registers. If\n");
		dbg_iprintf(l__shell->terminal, "\t        \tNo register is specified, all registers are shown.\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "setreg") != -1)
	{
		dbg_iprintf(l__shell->terminal, "setreg - Changes the content of one or more registers of a thread\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tsetreg [-c <sid>] [[<reg1> <val1>] [<reg2> <val2>] ... [<regn> <valn>]]\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe command should change the registers of the thread\n");
		dbg_iprintf(l__shell->terminal, "\t              \twhich has the SID <sid>. If not specified, the registers\n");
		dbg_iprintf(l__shell->terminal, "\t              \tof the current thread will be shown\n\n");
		dbg_iprintf(l__shell->terminal, "\t<regX> <valX> \tChange the value of the registers \"regX\" to the\n");
		dbg_iprintf(l__shell->terminal, "\t              \thex-number \"valx\". \"regX\" may be:\n");
		dbg_iprintf(l__shell->terminal, "\t              \t\tEAX, EBX, ECX, EDX\n");
		dbg_iprintf(l__shell->terminal, "\t              \t\tESI, EDI, EBP, ESP\n");
		dbg_iprintf(l__shell->terminal, "\t              \t\tEIP, EFLAGS\n");
		dbg_iprintf(l__shell->terminal, "\t              \tYou can specify one or more registers.\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
 	 else if (dbg_test_par(1, "readd") != -1)
	{
		dbg_iprintf(l__shell->terminal, "readd - Reads one or more DWORDs from the memory of a client\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\treadd <adr> [-c <sid>] [-n <num>]\n");
		dbg_iprintf(l__shell->terminal, "\t<adr>         \tThe address to start reading from\n\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe command should read from the memory of the thread\n");
		dbg_iprintf(l__shell->terminal, "\t              \twhich has the SID <sid>. If not specified, the memory\n");
		dbg_iprintf(l__shell->terminal, "\t              \tof the current thread will be read.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-n <num>      \tNumber of dwords to read (decimal number). If not\n");
		dbg_iprintf(l__shell->terminal, "\t              \tspecified, 1 will be persumed.\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
 	 else if (dbg_test_par(1, "readb") != -1)
	{
		dbg_iprintf(l__shell->terminal, "readb - Reads one or more bytes from the memory of a client\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\treadb <adr> [-c <sid>] [-n <num>] [-a]\n");
		dbg_iprintf(l__shell->terminal, "\t<adr>         \tThe address to start reading from\n\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe command should read from the memory of the thread\n");
		dbg_iprintf(l__shell->terminal, "\t              \twhich has the SID <sid>. If not specified, the memory\n");
		dbg_iprintf(l__shell->terminal, "\t              \tof the current thread will be read.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-n <num>      \tNumber of bytes to read (decimal number). If not\n");
		dbg_iprintf(l__shell->terminal, "\t              \tspecified, 1 will be persumed.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-a            \tDisplay the memory content as ASCII charracters\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
 	 else if (dbg_test_par(1, "writed") != -1)
	{
		dbg_iprintf(l__shell->terminal, "writed - Writes one or more DWORDs to the memory of a client\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\twrited <adr> [-c <sid>] -d <data-1> [<data-2> ... <data-n>]\n");
		dbg_iprintf(l__shell->terminal, "\t<adr>         \tThe address to start reading from\n\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe command should write to the memory of the thread\n");
		dbg_iprintf(l__shell->terminal, "\t              \twhich has the SID <sid>. If not specified, the memory\n");
		dbg_iprintf(l__shell->terminal, "\t              \tof the current thread will be written.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-d <data-n>   \tThe list of datas which should be written. The data list\n");
		dbg_iprintf(l__shell->terminal, "\t              \thas to be introduced with the -d parameter and has to be\n");
		dbg_iprintf(l__shell->terminal, "\t              \tat the end of the parameter list. Each data block has to\n");
		dbg_iprintf(l__shell->terminal, "\t              \tbe encoded as a hexadecimal value.\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}	
	 else if (dbg_test_par(1, "writeb") != -1)
	{
		dbg_iprintf(l__shell->terminal, "writeb - Writes one or more bytes to the memory of a client\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\twriteb <adr> [-c <sid>] -d <data-1> [<data-2> ... <data-n>]\n");
		dbg_iprintf(l__shell->terminal, "\t<adr>         \tThe address to start reading from\n\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe command should write to the memory of the thread\n");
		dbg_iprintf(l__shell->terminal, "\t              \twhich has the SID <sid>. If not specified, the memory\n");
		dbg_iprintf(l__shell->terminal, "\t              \tof the current thread will be written.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-d <data-n>   \tThe list of datas which should be written. The data list\n");
		dbg_iprintf(l__shell->terminal, "\t              \thas to be introduced with the -d parameter and has to be\n");
		dbg_iprintf(l__shell->terminal, "\t              \tat the end of the parameter list. Each data block has to\n");
		dbg_iprintf(l__shell->terminal, "\t              \tbe encoded as a hexadecimal value.\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}	
         else if (dbg_test_par(1, "writec") != -1)
	{
		dbg_iprintf(l__shell->terminal, "writec - Writes one or more charracters to the memory of a client\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\twrited <adr> [-c <sid>] -d <str>\n");
		dbg_iprintf(l__shell->terminal, "\t<adr>         \tThe address to start reading from\n\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe command should write to the memory of the thread\n");
		dbg_iprintf(l__shell->terminal, "\t              \twhich has the SID <sid>. If not specified, the memory\n");
		dbg_iprintf(l__shell->terminal, "\t              \tof the current thread will be written.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-d <str>      \tThe charracter string which should be written. The string\n");
		dbg_iprintf(l__shell->terminal, "\t              \thas to be introduced with the -d parameter and has to be\n");
		dbg_iprintf(l__shell->terminal, "\t              \tat the end of the parameter list. It can be surrounded by\n");
		dbg_iprintf(l__shell->terminal, "\t              \tquotes if it should contain empty spaces.\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}	
         else if (dbg_test_par(1, "dump") != -1)
	{
		dbg_iprintf(l__shell->terminal, "dump - Dump the content of the stack of a client\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tdump [-n <levels>] [-c <sid>]\n");
		dbg_iprintf(l__shell->terminal, "\t-n <levels>   \tThe number of levels to dump. If not given, 8 levels\n");
		dbg_iprintf(l__shell->terminal, "\t              \twould be printed.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe command should read from the memory of the thread\n");
		dbg_iprintf(l__shell->terminal, "\t              \twhich has the SID <sid>. If not specified, the memory\n");
		dbg_iprintf(l__shell->terminal, "\t              \tof the current thread will be read\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
         else if (dbg_test_par(1, "dumpint") != -1)
	{
		dbg_iprintf(l__shell->terminal, "dumpint - Dump the informations about a software interrupt\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tdumpint <num> [-c <sid>]\n");
		dbg_iprintf(l__shell->terminal, "\t<num>         \tThe number of the software interrupt. Should be a\n");
		dbg_iprintf(l__shell->terminal, "\t              \thexadecimal number between 0x00 and 0xFF.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe interrupt was executed by the client <SID>.\n");
		dbg_iprintf(l__shell->terminal, "\t              \tIf not specified, the SID of the current client\n");
		dbg_iprintf(l__shell->terminal, "\t              \twould be used.\n\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}	
         else if (dbg_test_par(1, "trace") != -1)
	{
		dbg_iprintf(l__shell->terminal, "trace - Retreive or change the traceing mode\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\ttrace [-c <sid>] [+B|-B] [+I|-I] [+D|-D] [+S|-S] [+M|-M]\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe client which trace flags should be changed. If\n");
		dbg_iprintf(l__shell->terminal, "\t              \tnot given, the current thread will be used.\n\n");
		dbg_iprintf(l__shell->terminal, "\t[+B | -B]     \tSet (+B) or delete (-B) breakpoint counting and traceing\n");
		dbg_iprintf(l__shell->terminal, "\t[+I | -I]     \tSet (+I) or delete (-I) tracing of other software interrupts\n");
		dbg_iprintf(l__shell->terminal, "\t[+D | -D]     \tSet (+D) or delete (-D) tracing of debugger calls\n");
		dbg_iprintf(l__shell->terminal, "\t[+S | -S]     \tSet (+S) or delete (-S) tracing of system calls\n");
		dbg_iprintf(l__shell->terminal, "\t[+M | -M]     \tSet (+M) or delete (-M) tracing of executed instructions\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "halton") != -1)
	{
		dbg_iprintf(l__shell->terminal, "halton - Retreive or change the halt conditions\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\thalton [-c <sid>] [+B|-B] [+I|-I] [+D|-D] [+S|-S] [+M|-M]\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe client which trace flags should be changed. If\n");
		dbg_iprintf(l__shell->terminal, "\t              \tnot given, the current thread will be used.\n\n");
		dbg_iprintf(l__shell->terminal, "\t[+B | -B]     \tSet (+B) or delete (-B) stopping on breakpoints\n");
		dbg_iprintf(l__shell->terminal, "\t[+I | -I]     \tSet (+I) or delete (-I) stopping on other software interrupts\n");
		dbg_iprintf(l__shell->terminal, "\t[+D | -D]     \tSet (+D) or delete (-D) stopping on debugger calls\n");
		dbg_iprintf(l__shell->terminal, "\t[+S | -S]     \tSet (+S) or delete (-S) stopping on system calls\n");
		dbg_iprintf(l__shell->terminal, "\t[+M | -M]     \tSet (+M) or delete (-M) stopping on every executed instruction\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
	 else if (dbg_test_par(1, "break") != -1)
	{
		dbg_iprintf(l__shell->terminal, "break - Install, delete or modify a breakpoint\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tbreak {<address>|-n <name>|-l} -o {add|delete|get|inc|reset} [-c client]\n");
		dbg_iprintf(l__shell->terminal, "\t<address>     \tThe address of the breakpoint (hex).\n");
		dbg_iprintf(l__shell->terminal, "\t-n <name>     \tThe name of the breakpoint. At least one parameter has\n");
		dbg_iprintf(l__shell->terminal, "\t              \tto be specified. The operation \"add\" needs at least \"adress\".\n\n");
		dbg_iprintf(l__shell->terminal, "\t-l            \tList all breakpoints of the client (-o will be ignored).\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe client which break points should be modified. If\n");
		dbg_iprintf(l__shell->terminal, "\t              \tnot given, the current thread will be used.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-o <op>       \tThe operation which should be executed. This can be:\n");
		dbg_iprintf(l__shell->terminal, "\t              \t add    -    Add a new breakpoint\n");
		dbg_iprintf(l__shell->terminal, "\t              \t del    -    Delete an existing breakpoint\n");
		dbg_iprintf(l__shell->terminal, "\t              \t get    -    Get the informations about a breakpoint\n");
		dbg_iprintf(l__shell->terminal, "\t              \t inc    -    Increment its usage counter\n");
		dbg_iprintf(l__shell->terminal, "\t              \t reset  -    Reset its usage counter\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
 	 else if (dbg_test_par(1, "hook") != -1)
	{
		dbg_iprintf(l__shell->terminal, "hook - Get the controll over a uncontrolled thread\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\thook <sid> <term>\n");
		dbg_iprintf(l__shell->terminal, "\t<sid>         \tThe SID of the thread (hex).\n");
		dbg_iprintf(l__shell->terminal, "\t<term>        \tThe terminal number (dec).\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
 	 else if (dbg_test_par(1, "exit") != -1)
	{
		dbg_iprintf(l__shell->terminal, "exit - Exit the current debugger session\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\texit [kill]\n");
		dbg_iprintf(l__shell->terminal, "\tkill          \tIf you set the \"kill\" parameter, the client thread\n");
		dbg_iprintf(l__shell->terminal, "\t<term>        \twill be killed before leaving the debugger.\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
 	 else if (dbg_test_par(1, "sysinfo") != -1)
	{
		dbg_iprintf(l__shell->terminal, "sysinfo - Print informations from the main information page\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tsysinfo {-n <name>|-a <num>} [-s]\n");
		dbg_iprintf(l__shell->terminal, "\t-n <name>     \tThe name of the sysinfo table entry (without\n");
		dbg_iprintf(l__shell->terminal, "\t              \tthe prefix \"MAININFO_\")\n");
		dbg_iprintf(l__shell->terminal, "\t-a <number>   \tThe entry number within the sysinfo table (Dec).\n");
		dbg_iprintf(l__shell->terminal, "\t              \tYou have to speceiver either -n or -a.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-s            \tCreate a list of all valid names and numbers.\n");
		dbg_iprintf(l__shell->terminal, "\n");
	}
 	 else if (dbg_test_par(1, "proc") != -1)
	{
		dbg_iprintf(l__shell->terminal, "proc - Print informations about a certain process\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tproc {-n <name>|-a <num>} [-s] [-c <sid>]\n");
		dbg_iprintf(l__shell->terminal, "\t-n <name>     \tThe name of the process table entry field (without\n");
		dbg_iprintf(l__shell->terminal, "\t              \tthe prefix \"PRCTAB_\")\n");
		dbg_iprintf(l__shell->terminal, "\t-a <number>   \tThe entry number within the process table (Dec).\n");
		dbg_iprintf(l__shell->terminal, "\t              \tYou have to speceiver either -n or -a.\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe SID of the process which informations should\n");
		dbg_iprintf(l__shell->terminal, "\t              \tbe shown. If the SID of a thread is given the SID\n");
		dbg_iprintf(l__shell->terminal, "\t              \tof its process will be used. If no SID is given\n");
		dbg_iprintf(l__shell->terminal, "\t              \tthe SID of the current client will be used.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-s            \tCreate a list of all valid names and numbers.\n");
		
		dbg_iprintf(l__shell->terminal, "\n");
	}
 	 else if (dbg_test_par(1, "thrd") != -1)
	{
		dbg_iprintf(l__shell->terminal, "thrd - Print informations about a certain thread\n\n");
		dbg_iprintf(l__shell->terminal, "Usage:\n\tproc {-n <name>|-a <num>} [-s] [-c <sid>]\n");
		dbg_iprintf(l__shell->terminal, "\t-n <name>     \tThe name of the thread table entry field (without\n");
		dbg_iprintf(l__shell->terminal, "\t              \tthe prefix \"THRTAB_\")\n");
		dbg_iprintf(l__shell->terminal, "\t-a <number>   \tThe entry number within the thread table (Dec).\n");
		dbg_iprintf(l__shell->terminal, "\t              \tYou have to speceiver either -n or -a.\n");
		dbg_iprintf(l__shell->terminal, "\t-c <sid>      \tThe SID of the thread which informations should\n");
		dbg_iprintf(l__shell->terminal, "\t              \tbe shown. If no SID is given the SID of\n");
		dbg_iprintf(l__shell->terminal, "\t              \tthe current client will be used.\n\n");
		dbg_iprintf(l__shell->terminal, "\t-s            \tCreate a list of all valid names and numbers.\n");
		
		dbg_iprintf(l__shell->terminal, "\n");
	}														
	 else
	{
		dbg_iprintf(l__shell->terminal, "Command list:\n");
		
		dbg_iprintf(l__shell->terminal, "\thelp   \t- This help screen.\n");
		dbg_iprintf(l__shell->terminal, "\tversion\t- Prints informations about the version of the debugger.\n");
		dbg_iprintf(l__shell->terminal, "\tdbgtest\t- Testing program for the debugger.\n");
		dbg_iprintf(l__shell->terminal, "\tterm   \t- Changes the current terminal.\n");
		dbg_iprintf(l__shell->terminal, "\n");
		dbg_iprintf(l__shell->terminal, "\texport \t- Exports a value to a variable.\n");
		dbg_iprintf(l__shell->terminal, "\techo   \t- Prints a text (or a variable) to the screen.\n");
		dbg_iprintf(l__shell->terminal, "\n");
		dbg_iprintf(l__shell->terminal, "\tstart  \t- Start the execution of a thread.\n");
		dbg_iprintf(l__shell->terminal, "\thook   \t- Get the controll over an uncontrolled client.\n");
		dbg_iprintf(l__shell->terminal, "\texit   \t- Exit the debugger session.\n");
		dbg_iprintf(l__shell->terminal, "\n");
		dbg_iprintf(l__shell->terminal, "\tsetreg \t- Get the content of one or more registers.\n");
		dbg_iprintf(l__shell->terminal, "\tgetreg \t- Get the content of one or more registers.\n");
		dbg_iprintf(l__shell->terminal, "\n");
		dbg_pause(l__shell->terminal);
		dbg_iprintf(l__shell->terminal, "\treadd  \t- Read one or more DWORDs from client's memory.\n");
		dbg_iprintf(l__shell->terminal, "\treadb  \t- Read one or more bytes from client's memory.\n");
		dbg_iprintf(l__shell->terminal, "\twrited \t- Write one or more DWORDs to the client's memory.\n");
		dbg_iprintf(l__shell->terminal, "\twriteb \t- Write one or more bytes to the client's memory.\n");
		dbg_iprintf(l__shell->terminal, "\twritec \t- Write one or more charracters to the client's memory.\n");
		dbg_iprintf(l__shell->terminal, "\n");
		dbg_iprintf(l__shell->terminal, "\tdump   \t- Dump the content of the stack.\n");
		dbg_iprintf(l__shell->terminal, "\tdumpint\t- Dump the informations about an software interrupt.\n");
		dbg_iprintf(l__shell->terminal, "\n");
		dbg_iprintf(l__shell->terminal, "\ttrace  \t- Retreive or change the traceing mode.\n");
		dbg_iprintf(l__shell->terminal, "\thalton \t- Retreive or change the halt conditions.\n");
		dbg_iprintf(l__shell->terminal, "\n");
		dbg_iprintf(l__shell->terminal, "\tbreak  \t- Install, delete or modify a breakpoint.\n");
		dbg_pause(l__shell->terminal);
		dbg_iprintf(l__shell->terminal, "\tsysinfo\t- Print informations from the main information page.\n");
		dbg_iprintf(l__shell->terminal, "\tproc   \t- Print informations about a certain process.\n");
		dbg_iprintf(l__shell->terminal, "\tthrd   \t- Print informations about a certain thread.\n");
				
		dbg_iprintf(l__shell->terminal, "\n\nEnter help <command> for more detailed informations about the selected command.\n\n");
	}
	
	
	
	return 0;
}
