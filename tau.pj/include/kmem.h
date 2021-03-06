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

#ifndef _KMEM_H_
#define _KMEM_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

void initkmem (void);
int readkmem  (unsigned long address, void *data, int size);
int writekmem (unsigned long address, void *data, int size);

unsigned long pid2task (unsigned pid);

#ifdef __cplusplus
}
#endif

#endif
