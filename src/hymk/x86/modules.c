/*
 *
 * modules.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * User mode modules managment
 *
 */
#include <hydrixos/types.h>
#include <stdio.h>
#include <kcon.h>
#include <setup.h>
#include <mem.h>

grub_module_t *grub_modules_list = NULL;
long grub_modules_num = 0;

#ifdef DEBUG_MODE
static void kmod_dump_modules_list(void)
{
	long l__i = grub_modules_num;
	
	kprintf("DUMPING GRUB MODULES LIST\n");
	kprintf("-------------------------\n");
	kprintf("List: 0x%X\n", (uintptr_t)grub_modules_list);
	kprintf("Number of mods: %i\n", grub_modules_num);
	kprintf("-------------------------\n");
		
	while(l__i --)
	{
		kprintf("Module: %i\n", grub_modules_num - l__i);
		kprintf("Name: %s\n\n", grub_modules_list[l__i].ascii);
		kprintf("Start: 0x%X\n", grub_modules_list[l__i].start);
		kprintf("End: 0x%X\n", grub_modules_list[l__i].end);
		kprintf("Size: %i Bytes = %i KiB\n",   
			(
			     grub_modules_list[l__i].end 
			  -  grub_modules_list[l__i].start
			),
			(
			     grub_modules_list[l__i].end 
			  -  grub_modules_list[l__i].start
			) / 1024
		       );
		kprintf("-------------------------\n\n");
	}

}
#endif 

/* 
 * kmod_init
 *
 * Initializes the GRUB modules list and
 * all kernel data structures associated with
 * it.
 *
 * Return:
 *
 *	0	Successfull
 *	>0	Error
 *
 */
int kmod_init(void)
{
	long l__i;
	
	/* Load the modules list */
	if (!(    i386_boot_info[GRUB_BOOTINFO_FLAGS] 
	       && GRUB_BOOTFLAG_MODULELIST_PRESENT
	   ))
	 {
	 	return -1;
	 }
	
	grub_modules_num = i386_boot_info[GRUB_BOOTINFO_MODULES_NUMBER];
	grub_modules_list =
	  (void*)(uintptr_t)i386_boot_info[GRUB_BOOTINFO_MODULES_INFO_ADDRESS];
	
	l__i = grub_modules_num;
	  
	while (l__i --)
	{
		uintptr_t l__tmp = grub_modules_list[l__i].end;
		
		if (l__tmp > mem_start) mem_start = l__tmp;
	}
	  
	#ifdef DEBUG_MODE
		kmod_dump_modules_list();
	#endif    
	
	/* Initialize the memory size data */
	if (!(    i386_boot_info[GRUB_BOOTINFO_FLAGS] 
	       && GRUB_BOOTFLAG_MEMSIZE_PRESENT
	   ))
	{
		return -2;
	}
	
	/* Align mem_start */
	mem_start += 4096;
	mem_start &= 0xFFFFF000;
	
	i386_boot_info[GRUB_BOOTINFO_MEM_UPPER] -= 1;
	total_mem_size = i386_boot_info[GRUB_BOOTINFO_MEM_UPPER] * 1024;
	normal_mem_free = total_mem_size;
	total_mem_size += 1024*1024;
	
	/* Calculating normal and high zone size */
	normal_mem_free -= (mem_start - (1024*1024));
	if (total_mem_size > (895 * 1024 * 1024))
	{	
		high_mem_free = (total_mem_size - (895 * 1024 * 1024));
		normal_mem_free -= high_mem_free; 
	}
	 else
	{
		high_mem_free = 0;
	}
	
	#ifdef DEBUG_MODE

		kprintf("Size of lower memory: %i Bytes = %i KiB\n", 
			i386_boot_info[GRUB_BOOTINFO_MEM_LOWER] * 1024, 
			i386_boot_info[GRUB_BOOTINFO_MEM_LOWER]
		       );		
		kprintf("Size of Upper memory: %i Bytes = %i MiB\n", 
			total_mem_size - (1024*1024), 
			(total_mem_size / (1024*1024)) - 1
		       );
		kprintf("Size of normal zone: %i Bytes = %i MiB\n",
			normal_mem_free,
			normal_mem_free / (1024*1024)
		       );
		if (high_mem_free > 0)
		{
			kprintf("Upper memory > 895 MiB. Using high zone.\n");
			kprintf("Size of high zone: %i Bytes = %i MiB\n",
				high_mem_free,
				high_mem_free / (1024*1024)
		       	       );
		}
		 else
		{
			kprintf("Upper memory < 895 MiB. Using normal zone only.\n");
		}
		
		kprintf("Start of the system memory 0x%X (@ %i KiB)\n", 
			mem_start, 
			(mem_start / 1024)
	       	       );
	#endif

	 
	return 0;
}

