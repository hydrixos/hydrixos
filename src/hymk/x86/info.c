/*
 *
 * info.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Info page managment
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/sid.h>
#include <setup.h>
#include <mem.h>
#include <stdio.h>
#include <page.h>
#include <info.h>

uintptr_t real_main_info = 0;
uintptr_t real_empty_info = 0;

uint32_t *main_info = NULL;
uint32_t *process_tab = NULL;
uint32_t *thread_tab = NULL;

uint32_t *current_p = NULL;
uint32_t *current_t = NULL;
uint32_t *kinfo_eff_prior = NULL;
uint64_t *kinfo_rtc_ctr = NULL;
uint32_t *kinfo_io_map = NULL;

sid_t	paged_pid = SID_PLACEHOLDER_NULL;

errno_t sysc_error = 0;

static struct {
	int	cpuid_available;	/* Is CPUID available */
	long	max_cpuid;		/* No. of CPUID parameters */
	long	name[3];		/* Name of the current CPU */
	
	int	type;
	int	family;
	int	model;
	int	stepping_id;
	
	uint32_t	features;
	
}i386_cpuid_s;
long i387_fsave = 0;
long i386_do_pge = 0;

/*
 * kinfo_init_x86_cpu()
 *
 * Initializes the x86 CPU by using the CPUID instruction
 *
 */
int kinfo_init_x86_cpu(void)
{
	long l__cpuid_infos[4];
	long l__cpuid_test_a, l__cpuid_test_b;
	
	/* Test if CPU-ID is available */
	asm volatile ("pushfl\n\t"
		      "pushfl\n\t"
		      "popl %%eax\n\t"
		      "xorl $0x200000, %%eax\n\t"
		      "pushl %%eax\n\t"
		      "popfl \n\t"
		      "pushfl \n\t"
		      "popl %%eax\n\t"
		      "popl %%ebx\n\t"
		      "pushl %%ebx\n\t"
		      "popfl\n\t"
		      :"=a" (l__cpuid_test_a),
		       "=b" (l__cpuid_test_b)
		      :
		      :"memory"
		     );
	i386_cpuid_s.cpuid_available = 
		((l__cpuid_test_a ^ l__cpuid_test_b) & 0x200000);
	if (i386_cpuid_s.cpuid_available == false) return -1;
	
	/* Get the number of possible CPU-ID parameters and the CPU name*/	
	asm volatile ("movl $0, %%eax\n\t"
	    	      "cpuid\n\t"
	    	      :"=a" (l__cpuid_infos[0]), "=b" (l__cpuid_infos[1]), 
		       "=c" (l__cpuid_infos[2]), "=d" (l__cpuid_infos[3])
	    	      :
	    	      :"memory"
	    	     );
		     
	i386_cpuid_s.max_cpuid = l__cpuid_infos[0];
	i386_cpuid_s.name[0] = l__cpuid_infos[1];
	i386_cpuid_s.name[1] = l__cpuid_infos[3];
	i386_cpuid_s.name[2] = l__cpuid_infos[2];
	i386_cpuid_s.name[12] = 0;
	
	if (i386_cpuid_s.max_cpuid == 0) return -2; 

	/* Get the CPU features etc. */
	asm volatile ("movl $1, %%eax\n\t"
	    	      "cpuid\n\t"
	    	      :"=a" (l__cpuid_infos[0]), "=b" (l__cpuid_infos[1]), 
		       "=c" (l__cpuid_infos[2]), "=d" (l__cpuid_infos[3])
	    	      :
	    	      :"memory"
	    	     );		
	
	i386_cpuid_s.type = (l__cpuid_infos[0] & 0x3000) >> 12;
	i386_cpuid_s.family = (l__cpuid_infos[0] & 0xF00) >> 8;
	i386_cpuid_s.model = (l__cpuid_infos[0] & 0xF0) >> 4;
	i386_cpuid_s.stepping_id = (l__cpuid_infos[0] & 0xF);
	
	i386_cpuid_s.features = l__cpuid_infos[3];
	
	/* Use PGE if available */
	if (i386_cpuid_s.features & 16384) 
	{
		i386_do_pge = 1;
	
		#ifdef DEBUG_MODE
			kprintf("Using PTE global extension (PGE).\n");
		#endif
	}
	 else
	{
		i386_do_pge = 0;
	
		#ifdef DEBUG_MODE
			kprintf("Disabling PTE global extension (PGE).\n");
		#endif
	}	
	
	return 0;
}

/*
 * kinfo_init_387_fpu
 *
 * Initializes the FPU and tests whether
 * FSTOR or FXSTOR should be used.
 *
 */
int kinfo_init_387_fpu()
{
	long l__tmp = 0;
	
	if (!(i386_cpuid_s.features & 0x1)) 
	{
		i387_fsave = 0;
		/* No FPU supported */
		return -1;
	}
	
	if (i386_cpuid_s.features & 0x1000000)
	{
		/* Use FXSave */
		i387_fsave = 2;
	}
	 else
	{
		/* Use FSAVE */
		i387_fsave = 1;
	}
		
	asm volatile ("movl %%cr0, %%eax\n\t"
		      : "=a" (l__tmp)
		     );
	l__tmp &= (~0xC);
	l__tmp |= 0x22;
	
	asm volatile (
	     	      "movl %%eax, %%cr0\n\t"
		      "finit\n\t"
	     	      :: "a" (l__tmp)
	     	     );
		     
	return 0;
}

int kinfo_init(void)
{
	uint32_t *l__ptab = ikp_start + 1024;
	long l__n = 0;
	
	/* Allocate the page frame of the main info page */
	real_main_info = (uintptr_t)kmem_alloc_kernel_pageframe();
	if (real_main_info == NULL) return -1;
	
	/* Allocate the page frame of the empty info page */
	real_empty_info = (uintptr_t)kmem_alloc_kernel_pageframe();
	if (real_empty_info == NULL) return -2;
		
	/* Map the main info page into the kernel virtual address space */
	l__ptab[(0xF8000 - 0xc0000)] =   real_main_info 
	                               | PFLAG_PRESENT
			               | PFLAG_GLOBAL
				       | PFLAG_USER
			              ;
	/* Invalidate TLB */			   
	INVLPG(0xF8000000);

	/*
	 * Map the empty info page into the process and thread table area of the
	 * kernel virtual address space
	 *
	 */
	for(l__n = (0xFE001 - 0xc0000);
	    l__n >= (0xF8001 - 0xc0000);
	    l__n --
	   )
	{
		l__ptab[l__n] = real_empty_info
				| PFLAG_PRESENT
				| PFLAG_GLOBAL
				| PFLAG_USER
				 ;
		INVLPG(((l__n +0xc0000) * 4096));

	}	
	
	#ifdef DEBUG_MODE
		kprintf("Info page and process/thread table have been installed.\n");
	#endif
	
	/* Set the pointers to the tables */
	/* They are *not* linear addresses but kernel internal addresses, so
	 * we have to subtract 0xC0000000.
	 */
	main_info = (void*)(uintptr_t)(0xF8000000 - 0xC0000000);
	process_tab = (void*)(uintptr_t)(0xF8001000 - 0xC0000000);
	thread_tab = (void*)(uintptr_t)(0xFB001000 - 0xC0000000);
	
	/*
	 * Write static informations into the info page
	 *
	 */
	main_info[MAININFO_KERNEL_MAJOR] = HYMK_VERSION_MAJOR;
	main_info[MAININFO_KERNEL_MINOR] = HYMK_VERSION_MINOR;
	main_info[MAININFO_KERNEL_REVISION] = HYMK_VERSION_REVISION;
	main_info[MAININFO_RTC_COUNTER_LOW] = 0;  		   
	main_info[MAININFO_RTC_COUNTER_HIGH] = 0;  		   
	main_info[MAININFO_CPU_ID_CODE] = HYMK_VERSION_CPUID;
	main_info[MAININFO_PAGE_SIZE] = 4096;
	main_info[MAININFO_MAX_PAGE_OPERATION] = MEM_MAX_PAGE_OP_NUM;
		
	main_info[MAININFO_X86_CPU_NAME_PART_1] = i386_cpuid_s.name[0];
	main_info[MAININFO_X86_CPU_NAME_PART_2] = i386_cpuid_s.name[1];
	main_info[MAININFO_X86_CPU_NAME_PART_3] = i386_cpuid_s.name[2];
	
	main_info[MAININFO_X86_CPU_TYPE] = i386_cpuid_s.type;
	main_info[MAININFO_X86_CPU_FAMILY] = i386_cpuid_s.family;
	main_info[MAININFO_X86_CPU_MODEL] = i386_cpuid_s.model;
	main_info[MAININFO_X86_CPU_STEPPING] = i386_cpuid_s.stepping_id;
	main_info[MAININFO_X86_RAM_SIZE] = total_mem_size - (1024*1024);
	
	#ifdef DEBUG_MODE
		long l__cpustr[4];
		
		l__cpustr[0] = main_info[MAININFO_X86_CPU_NAME_PART_1];
		l__cpustr[1] = main_info[MAININFO_X86_CPU_NAME_PART_2];
		l__cpustr[2] = main_info[MAININFO_X86_CPU_NAME_PART_3];
		l__cpustr[3] = 0;
	
		kprintf("\n");
		kprintf("HydrixOS Kernel: %i.%i.%i\n", 
			main_info[MAININFO_KERNEL_MAJOR],
			main_info[MAININFO_KERNEL_MINOR],
			main_info[MAININFO_KERNEL_REVISION]
		       );
		kprintf("Detected CPU: Family %i Model %i\n",
			main_info[MAININFO_X86_CPU_FAMILY],
			main_info[MAININFO_X86_CPU_MODEL]
	       	       );
		kprintf("CPU Name: %s\n", (char*)&(l__cpustr[0]));
		kprintf("%i MiB upper memory free. (%i Bytes)\n", 
			main_info[MAININFO_X86_RAM_SIZE] / (1024*1024),
			main_info[MAININFO_X86_RAM_SIZE]
		       );

		kprintf("\n");
	#endif

	kinfo_rtc_ctr = (uint64_t*)&main_info[MAININFO_RTC_COUNTER_LOW];
	*kinfo_rtc_ctr = 0;
	
	return 0;	
}

/*
 * kinfo_new_descr(sid)
 *
 * Creates a new descriptor for the process or thread 'sid'
 * (depending on the header value of the SID). 
 *
 * Return Value:
 *
 *	The physical address of the second descriptor page
 *	NULL	  	   - Out of memory
 *	0xFFFFFFFF	   - Invalid SID
 *
 */
uint32_t* kinfo_new_descr(sid_t sid)
{
	uint32_t *l__descr = NULL;
	uint32_t *l__retval = NULL;
	long l__pnum = 0;
	uintptr_t l__adr = 0;
	uint32_t *l__ptab = ikp_start + 1024;
		
	/*
	 * Test whether SID is valid or not
	 *
	 */
	switch (sid & SID_TYPE_MASK)
	{
		case SIDTYPE_PROCESS:
		{
			if ((sid & SID_DATA_MASK) >= PROCESS_MAX)
				return (void*)0xFFFFFFFF;
			else
			{
				l__pnum = 2;	/* Allocate 2 pages (desc, grplst) */
				l__adr = 0xF8001000 + 
					(    ((uint32_t)sid & SID_DATA_MASK) 
					  * 2 * 4096
					);				
			}
			break;
		}
		
		case SIDTYPE_THREAD:
		{
			if ((sid & SID_DATA_MASK) >= THREAD_MAX)
				return (void*)0xFFFFFFFF;
			else
			{
				l__pnum = 2;	/* Allocate 3 pages (desc, acs, tls) */
				l__adr = 0xFB001000 + 
					(    ((uint32_t)sid & SID_DATA_MASK) 
					  * 2 * 4096
					);
			}
			break;
		}
	
		default:
		{
			return (void*)0xFFFFFFFF;	
		}
	}
	
	/*
	 * Create new descriptor
	 *
	 */
	/* Allocate it */
	
	/* Recalculate address */
	l__adr -= 0xC0000000;	/* Kernel memory address */
	l__adr /= 0x1000;	/* IKP entry */

	
	/* Put it into the descriptor table */
	while (l__pnum --)	
	{
		l__descr = kmem_alloc_kernel_pageframe();
		if (l__descr == NULL) return NULL;
	
		if (l__retval == NULL) l__retval = l__descr;
		
		l__ptab[l__adr + l__pnum] =   ((uintptr_t)l__descr)
					    | PFLAG_PRESENT
					    | PFLAG_GLOBAL
					    | PFLAG_USER
			 		    ;		
		INVLPG((((l__adr + l__pnum) + 0xc0000) * 4096));
	}

	l__descr[0] = 1;
		
	return l__retval;
}


/*
 * kinfo_del_descr(sid)
 *
 * Frees the descriptors of the process or thread subject 'SID' and
 * removes its handles from the descriptor tables.
 *
 */
void kinfo_del_descr(sid_t sid)
{
	uint32_t *l__descr = NULL;
	long l__pnum = 0;
	uintptr_t l__adr = 0;
	uint32_t *l__ptab = ikp_start + 1024;
	
	/*
	 * Test whether SID is valid or not
	 *
	 */
	switch (sid & SID_TYPE_MASK)
	{
		case SIDTYPE_PROCESS:
		{
			if ((sid & SID_DATA_MASK) >= PROCESS_MAX)
				return;
			else
			{
				l__pnum = 2;	/* Allocate 2 pages (desc, grplst) */
				l__adr = 0xF8001000 + (((uint32_t)sid & SID_DATA_MASK) * 2 * 4096);
			}
			break;
		}
		
		case SIDTYPE_THREAD:
		{
			if ((sid & SID_DATA_MASK) >= THREAD_MAX)
				return;
			else
			{
				l__pnum = 2;	/* Allocate 3 pages (desc, acs, tls) */
				l__adr = 0xFB001000 + 
					(  ((uint32_t)sid & SID_DATA_MASK) 
					 * 2 * 4096
					);
			}
			break;
		}
	
		default:
		{
			return;
		}
	}
	
	/* Recalculate address */
	l__adr -= 0xC0000000;	/* Kernel memory address */
	l__adr /= 0x1000;	/* IKP entry */

	while (l__pnum --)	
	{
		/*
		 * Get the address of the descriptor page frame
		 * from the page table entry of the descriptor
		 * table 
		 *
		 */
		l__descr = (void*)(uintptr_t)
				((l__ptab[l__adr + l__pnum]) & (~0xFFFU));
		
		/*
		 * Remove the descriptor and set the standard
		 * empty descriptor
		 */
		l__ptab[l__adr + l__pnum] =   real_empty_info
					    | PFLAG_PRESENT
				            | PFLAG_GLOBAL
					    | PFLAG_USER
				           ;
		
		/* Invalidate the TLB of the descriptor page */		 					    
		INVLPG((((l__adr + l__pnum) + 0xc0000) * 4096));
		
		/* Free the descriptor page frame */
		if (l__descr != NULL) kmem_free_kernel_pageframe(l__descr);
	}

	return;
}
