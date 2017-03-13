/*
 *  Copyright 2017 Kjell Winblad (http://winsh.me, kjellwinblad@gmail.com)
 *
 *  This file is part of qd_lock_lib.
 *
 *  qd_lock_lib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  qd_lock_lib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with qd_lock_lib.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DRMCS_LOCK_H
#define DRMCS_LOCK_H

#include "locks/oo_lock_interface.h"
#include "locks/mcs_lock.h"
#include "misc/padded_types.h"
#include "read_indicators/reader_groups_read_indicator.h"

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>

#define DRMCS_READ_PATIENCE_LIMIT 130000

typedef struct {
    MCSLock lock;
    LLPaddedInt writeBarrier;
    ReaderGroupsReadIndicator readIndicator;
    char pad[CACHE_LINE_SIZE];
} DRMCSLock;

extern
_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable DRMCS_LOCK_METHOD_TABLE;


void drmcs_initialize(DRMCSLock * lock);
void drmcs_lock(void * lock);
void drmcs_unlock(void * lock);
bool drmcs_is_locked(void * lock);
bool drmcs_try_lock(void * lock);
void drmcs_rlock(void * lock);
void drmcs_runlock(void * lock);
void drmcs_delegate(void * lock,
                    void (*funPtr)(unsigned int, void *), 
                    unsigned int messageSize,
                    void * messageAddress);
void * drmcs_delegate_or_lock(void * lock, unsigned int messageSize);
DRMCSLock * plain_drmcs_create();
OOLock * oo_drmcs_create();


#endif
