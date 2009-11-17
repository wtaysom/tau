/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kallsyms.h>

#include <style.h>
#include <tau/debug.h>
#include <tau_fs.h>

#define SPINLOCK_DEBUG	ENABLE

bool Save_irq = FALSE;

void show_stack (struct task_struct *task, unsigned long *sp)
{
	static void (*fp)(struct task_struct *task, unsigned long *sp) = NULL;
	char	*symbol = "show_stack";

	if (!fp) {
		fp = (void *)kallsyms_lookup_name(symbol);
		if (!fp) {
			printk(KERN_ALERT "ERR: Symbol[%s] unavailable.\n", symbol);
			return;
		}
	}
	fp(task, sp);
}

void tau_init_lock (tau_spinlock_s *lock, const char *name)
{
	zero(*lock);
	spin_lock_init( &lock->lk_spinlock);
	lock->lk_where = WHERE;
	lock->lk_name  = name;
}

static void spinlock_err (tau_spinlock_s *lock)
{
	printk("<1>---------------- Stack dump %p ----------------\n",
		lock->lk_owner);
	show_stack((struct task_struct *)lock->lk_owner, NULL);
	printk("<1>---------------- Stack end  %p ----------------\n",
		lock->lk_owner);
	BUG();
}

void tau_lock (tau_spinlock_s *lock, const char *where)
{
	if (lock->lk_owner == current) {
		 printk("<1>" "Current<%p> already has spinlock %s at %s\n",
			current, lock->lk_name, lock->lk_where);
		BUG();
	}
	if (Save_irq) {
		spin_lock_irqsave( &lock->lk_spinlock, lock->lk_flags);
	} else {
#if SPINLOCK_DEBUG IS_ENABLED
		static int dead = FALSE;
		int	n = 20*1000*1000;

		if (dead) bug("======tau spinlock problem already detected=====");
		for (;;) {
			if (spin_trylock( &lock->lk_spinlock)) break;
			if (!n--) {
				dead = TRUE;
				spinlock_err(lock);
			}
		}
#else
		spin_lock( &lock->lk_spinlock);
#endif
	}
	lock->lk_owner = current;
	lock->lk_where = where;
}

void tau_unlock (tau_spinlock_s *lock, const char *where)
{
	if (lock->lk_owner != current) {
		printk("<1>" "Current<%p> shouldn't be holding spinlock %s at %s\n",
			current, lock->lk_name, lock->lk_where);
		BUG();
	}
	lock->lk_owner = 0;
	lock->lk_where = where;
	if (Save_irq) {
		spin_unlock_irqrestore( &lock->lk_spinlock, lock->lk_flags);
	} else {
		spin_unlock( &lock->lk_spinlock);
	}
}

void tau_have_lock (tau_spinlock_s *lock, const char *where)
{
	if (lock->lk_owner != current) {
		printk("<1>" "Current<%p> doesn't hold spinlock %s at %s\n",
			current, lock->lk_name, lock->lk_where);
 		BUG();
	}
}

void tau_dont_have_lock (tau_spinlock_s *lock, const char *where)
{
	if (lock->lk_owner == current) {
		printk("<1>" "Current<%p> shouldn't be holding spinlock %s at %s\n",
			current, lock->lk_name, lock->lk_where);
		BUG();
	}
}
