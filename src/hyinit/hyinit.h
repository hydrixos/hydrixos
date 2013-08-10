/*
 *
 * hyinit.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Global header file of init
 *
 */
#ifndef _HYINIT_H
#define _HYINIT_H

/*
 * Initial console output
 *
 */
int iprintf(const utf8_t* fm, ...);
extern volatile uint8_t* i__screen;
void init_iprintf();
void kclrscr(void);
void init_iprintf(void);

/*
 * Init process managment
 *
 */
extern volatile int init_process_number;

extern sid_t initproc_debugger_sid;

enum {
	INITPROC_MAIN = 101,
	INITPROC_DEBUGGER = 102
};

sid_t init_fork(int pnum);
void init_kill(void);
void initfork_libinit(void);

/*
 * Process entry points
 *
 */
int init_main(void);
int debugger_main(void);

#endif
