/*
 *
 * mem.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Kernel memory managment functions
 *
 */
#ifndef _MEM_H
#define _MEM_H

#include <hydrixos/types.h>

/*
 * GDT entries
 *
 */
#define CS_SELECTOR			0x08
#define DS_SELECTOR			0x10
#define CS_KERNEL			0x18
#define DS_KERNEL			0x20
#define CS_USER				0x28
#define DS_USER				0x30
#define TSS_SELECTOR			0x38	

/*
 * GRUB Bootinfo
 * (According to the GRUB multiboot specifications)
 *
 */
/* GRUB_BOOTFLAG */
#define	GRUB_BOOTINFO_FLAGS			0
/* Size of lower memory (< 640 KiB) */
#define GRUB_BOOTINFO_MEM_LOWER			1
/* Size of upper memory (> 1 MiB)*/
#define GRUB_BOOTINFO_MEM_UPPER			2
/* Boot device info */
#define GRUB_BOOTINFO_BOOT_DEVICE		3
/* Kernel command line */
#define GRUB_BOOTINFO_CMDLINE			4
/* Number of loaded modules and adress of the modules info block */
#define GRUB_BOOTINFO_MODULES_NUMBER		5
#define GRUB_BOOTINFO_MODULES_INFO_ADDRESS	6
/* ELF header entries */
#define GRUB_BOOTINFO_SYMS_ELF_SHDR_NUM		7
#define GRUB_BOOTINFO_SYMS_ELF_SHDR_SIZE	8
#define GRUB_BOOTINFO_SYMS_ELF_SHDR_ADDR	9
#define GRUB_BOOTINFO_SYMS_ELF_SHDR_SHNDX	10
/* BIOS memory mapping informations */
#define GRUB_BOOTINFO_MMAP_LENGTH		11
#define GRUB_BOOTINFO_MMAP_ADDRESS		12
/* BIOS Drive informations */
#define GRUB_BOOTINFO_DRIVES_LENGTH		13
#define GRUB_BOOTINFO_DRIVES_ADDRESS		14
/* BIOS Configuration table */
#define GRUB_BOOTINFO_CONFIG_TABLE		15
/* Boot loader name */
#define GRUB_BOOTINFO_LOADER_NAME		16
/* APM table */
#define GRUB_BOOTINFO_APM_TABLE			17

extern uint32_t *i386_boot_info;

/* GRUB_BOOTFLAG */
#define GRUB_BOOTFLAG_MEMSIZE_PRESENT		1
#define GRUB_BOOTFLAG_BOOTDEVICE_PRESENT	2
#define GRUB_BOOTFLAG_COMMANDLINE_PRESENT	4
#define GRUB_BOOTFLAG_MODULELIST_PRESENT	8
#define GRUB_BOOTFLAG_SYMS_AOUT_PRESENT		16
#define GRUB_BOOTFLAG_SYMS_ELF_PRESENT		32
#define GRUB_BOOTFLAG_MMAP_PRESENT		64
#define GRUB_BOOTFLAG_DRIVES_PRESENT		128
#define GRUB_BOOTFLAG_CONFIGTABLE_PRESENT	256
#define GRUB_BOOTFLAG_BOOTLOADERNAME_PRESENT	512
#define GRUB_BOOTFLAG_APMTABLE_PRESENT		1024


/*
 * GRUB Modules
 *
 */
typedef struct 
{
	uintptr_t	start;		/* Start address of the module */
	uintptr_t	end;		/* End address of the module */
	char		*ascii;		/* Name & parameters of the module */
}grub_module_t;

extern grub_module_t *grub_modules_list;
extern long grub_modules_num;

/*
 * Initializes the GRUB modules list
 * and memory size structures.
 *
 */
int kmod_init(void);

/*
 * General paging functions and macros
 *
 */
/* x86 paging flags */
#define PFLAG_PRESENT		1
#define PFLAG_READWRITE		2
#define PFLAG_USER		4
#define PFLAG_WRITE_THROUGH	8
#define PFLAG_CACHE_DISABLED	16
#define PFLAG_ACCESSED		32
#define PFLAG_GLOBAL		256

/* 
 * PFLAG_AVAILABLE_0:  Page is selected for Copy-on-write.
 * PFLAG_AVAILABLE_1:  Page is protected by the PageD.
 * PFLAG_AVAILABLE_2:  Currently unused (reserved for later versions).
 *
 */
#define PFLAG_AVAILABLE_0	512	
#define PFLAG_AVAILABLE_1	1024
#define PFLAG_AVAILABLE_2	2048

#define PFLAG_COPYONWRITE	PFLAG_AVAILABLE_0
#define PFLAG_PAGED_PROTECTED	PFLAG_AVAILABLE_1

/* generalized paging flags */
#define GENFLAG_PRESENT		PFLAG_PRESENT
#define GENFLAG_READABLE	0
#define GENFLAG_WRITABLE	PFLAG_READWRITE
#define GENFLAG_EXECUTABLE	0
#define GENFLAG_USER_MODE	PFLAG_USER
#define GENFLAG_NOT_CACHEABLE	PFLAG_CACHE_DISABLED
#define GENFLAG_GLOBAL		PFLAG_GLOBAL
#define GENFLAG_DO_COPYONWRITE	PFLAG_AVAILABLE_0
#define GENFLAG_PAGED_PROTECTED	PFLAG_AVAILABLE_1

#define GENFLAG_DONT_OVERWRITE_SETTINGS		0x1000
			 		      		      
/*
 * Kernel address space
 *
 */
/* The current page directory */
extern uint32_t *i386_current_pdir;

int kmem_init_krnl_spc(void);		/* Initializes the kernel address space */
int kmem_switch_krnl_spc(void);		/* Switches into the kernel address space */


/*
 * ===================================
 *
 * Kernel memory managment
 *
 * ===================================
 *
 */
/* Memory size informations */
extern size_t total_mem_size;		/* Total size of RAM (lower, normal and high) */
extern size_t normal_mem_free;		/* Size of free normal-zone RAM */
extern size_t high_mem_free;		/* Size of free high-zone RAM */
extern uintptr_t mem_start;		/* Start of the freely useable memory */

/* Internal page tabeles of the kernel address space */
extern uint32_t *ikp_start;		/* Start address of the IKP */
extern size_t ikp_size;			/* Size of the IKP (should be 1 MiB + 4096 KiB) */

/*
 * Managment of free page frames 
 *
 */
typedef struct {
	uintptr_t frame_stack_start;	/* Start address of free page frame stack */
	size_t	  frame_stack_sz;	/* Size of free page frame stack */
	uint32_t* frame_stack_ptr;	/* Stack pointer of free page frame stack */
	uint32_t  frame_stack_count;	/* Count of pages left in the zone */
}memory_zone_t;

extern memory_zone_t	normal_zone;	/* Descriptor of the normal zone */
extern memory_zone_t	high_zone;	/* Descriptor of the high zone */

/* 
 * Managment of used page frames 
 *
 */
typedef struct {		
	sid_t			pid;	/* Process-SID of page user */
	uintptr_t		u_adr;	/* Adress of page */
}page_user_t;

typedef struct sidlist_st{
	page_user_t		user;	/* Users of this shared page (sid-null = unused )*/
	struct sidlist_st	*nxt;	/* Next list entry */
}sidlist_t;
	
typedef union {
	page_user_t	single;		/* Single use: Process-SID */
	sidlist_t	*multi;		/* Multiple use: SID-List */
}page_usr_u;

typedef struct {
	uint16_t	usage;		/* Usage counter of a page frame */
	page_usr_u	owner;		/* Owner SID / SID-List of a page frame */
}pbtab_t;

/* Page buffer table */
extern uintptr_t page_buf_table_start;	/* Start address of the PBT */
extern size_t page_buf_table_sz;	/* Size of the PBT */
extern pbtab_t* page_buf_table;		/* Pointer to the PBT */

/* Access to the page buffer table */
#define PAGE_BUF_TAB(___padr)		(page_buf_table[(___padr - (1024*1024)) / 4096])

/* Page share table */
extern uintptr_t page_share_table_start;	/* Start address of the PST */
extern long page_share_table_count;		/* Count of entries in the PST */
extern size_t page_share_table_sz;		/* Size of the PST */
extern sidlist_t *page_share_table_free;	/* Pointer to the PST (free entries list) */

/* Page buffer */
extern uintptr_t page_buffer_start;	/* Start address of the Pagebuffer */
extern void* page_buffer;		/* Start address of the Pagebuffer */
extern size_t page_buffer_sz;		/* Size of Page buffer */
extern long page_buffer_num;		/* Number of PBT-Entries */

int kmem_init_tables(void);

void* kmem_alloc_user_pageframe(sid_t pid, uintptr_t u_adr);	/* Normal and high zone */
void* kmem_alloc_kernel_pageframe(void);	/* Normal-zone only */

void kmem_free_kernel_pageframe(void* page);
void kmem_free_user_pageframe(uintptr_t page, sid_t pid, uintptr_t u_adr);

/*
 * ===================================
 *
 * Virtual address space managment
 *
 * ===================================
 *
 */
uint32_t* kmem_create_space(void);
void kmem_destroy_space(uint32_t* pagedir);

/*
 * TLB managment
 *
 */
#define INVLPG(___adr)	 __asm__ __volatile__(\
					      "invlpg %%gs:(%0)\n"\
 		            	      	      :\
  		            	     	      :"r" (((uintptr_t)(___adr)))\
 		            	     	      :"memory"\
  		            	     	     );\
			 

#define INV_TLB_COMPLETE()	({\
					uint32_t ___d;\
					__asm__ __volatile__ (\
							      "movl %%cr3, %0\n"\
							      "movl %0, %%cr3\n"\
							      :"=r" (___d)\
							      :\
							      :"memory"\
							     );\
				})

/*
 * Virtual address space structure
 *
 */
#define VAS_ZERO_PAGE			0
#define VAS_USER_START			0x1000
#define VAS_USER_END			0xBFFFEFFF
#define VAS_THREAD_LOCAL_STORAGE	0xBFFFF000
#define VAS_KERNEL_START		0xC0000000
#define VAS_KERNEL_END			0xFFFFFFFF
#define VAS_MAIN_INFO_PAGE		0xF8000000
#define VAS_PROC_TABLE_START		0xF8001000
#define VAS_THREAD_TABLE_START		0xFB001000
#define VAS_INFO_END			0xFFFDFFFF
#define VAS_USER_MODE_ACCESS_AREA	0xFFFE0000
#define VAS_LAST_PAGE			0xFFFFF000	

/*
 * Memory synchornization
 *
 */
#define MSYNC()			__asm__ __volatile__("add $0, (%%esp)":::"memory")

/*
 * Paging daemon functions
 *
 */
extern sid_t paged_thr_sid;
int kpaged_handle_exception(uint32_t number, uint32_t code, uintptr_t ip);
int kpaged_send_pagefault(uint32_t number, uint32_t code, uint32_t ip, uint32_t adr);

/*
 * Copy-on-write implementation
 *
 */
int kmem_do_copy_on_write(uint32_t* pdir, sid_t sid, uintptr_t usradr);		/* Execution of COW */
int kmem_copy_on_write(void); 							/* Exception handler for COW exceptions */
			  
/*
 * TSS initialization
 *
 */
int ksched_init_tss(void);
			  
#endif
