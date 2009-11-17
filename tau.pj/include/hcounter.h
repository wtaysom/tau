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

#ifndef _HCOUNTER_H_
#define _HCOUNTER_H_ 1

void *init_hcounter (unsigned num_counters);
void *hcounter     (void *hc, unsigned x);
void stat_hcounter (void *hc);
void dump_counters (void *hc);
void dump_buckets  (void *hc);
void dump_hcounter (void *hc);

#endif
