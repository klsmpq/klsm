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

#ifndef MRQD_LOCK_H
#define MRQD_LOCK_H

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>

#include "misc/padded_types.h"
#include "locks/tatas_lock.h"
#include "qd_queues/qd_queue.h"
#include "read_indicators/reader_groups_read_indicator.h"
#include "locks/oo_lock_interface.h"

/* Multiple Reader Queue Delegation Lock */

#ifndef MRQD_READ_PATIENCE_LIMIT
#    define MRQD_READ_PATIENCE_LIMIT 1000
#endif

typedef struct {
    TATASLock mutexLock;
    QDQueue queue;
    ReaderGroupsReadIndicator readIndicator;
    LLPaddedUInt writeBarrier;
} MRQDLock;

void mrqd_initialize(MRQDLock * lock);
void mrqd_lock(void * lock);
void mrqd_unlock(void * lock);
bool mrqd_is_locked(void * lock);
bool mrqd_try_lock(void * lock);
void mrqd_rlock(void * lock);
void mrqd_runlock(void * lock);
void mrqd_delegate(void* lock,
                   void (*funPtr)(unsigned int, void *), 
                   unsigned int messageSize,
                   void * messageAddress);
void * mrqd_delegate_or_lock(void* lock,
                             unsigned int messageSize);
void mrqd_close_delegate_buffer(void * buffer,
                                void (*funPtr)(unsigned int, void *));
void mrqd_delegate_unlock(void* lock);
void mrqd_executeAndWaitCS(unsigned int size, void * data);
void mrqd_delegate_wait(void* lock,
                        void (*funPtr)(unsigned int, void *), 
                        unsigned int messageSize,
                        void * messageAddress);
MRQDLock * plain_mrqd_create();
OOLock * oo_mrqd_create();

#endif
