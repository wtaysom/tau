	/*
	 * Save registers before calling assembly functions. This avoids
	 * disturbance of register allocation in some inline assembly constructs.
	 * Copyright 2001,2002 by Andi Kleen, SuSE Labs.
	 * Subject to the GNU public license, v.2. No warranty of any kind.
	 * $Id: thunk.S,v 1.2 2002/03/13 20:06:58 ak Exp $
	 */

	#include <linux/config.h>
	#include <linux/linkage.h>
	#include <asm/dwarf2.h>
	#include <asm/calling.h>			
	#include <asm/rwlock.h>
		
	/* rdi:	arg1 ... normal C conventions. rax is saved/restored. */ 	
	.macro thunk name,func
	.globl \name
\name:	
	CFI_STARTPROC
	SAVE_ARGS
	call \func
	jmp  restore
	CFI_ENDPROC
	.endm


/* Support for read/write spinlocks. */
	.text
/* rax:	pointer to rwlock_t */	
ENTRY(__write_lock_failed)
	lock
	addl $RW_LOCK_BIAS,(%rax)
1:	rep
	nop
	cmpl $RW_LOCK_BIAS,(%rax)
	jne 1b
	lock 
	subl $RW_LOCK_BIAS,(%rax)
	jnz  __write_lock_failed
	ret

/* rax:	pointer to rwlock_t */	
ENTRY(__read_lock_failed)
	lock
	incl (%rax)
1:	rep
	nop
	cmpl $1,(%rax)
	js 1b
	lock
	decl (%rax)
	js __read_lock_failed
	ret
