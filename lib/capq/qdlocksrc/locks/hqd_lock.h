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

#ifndef HQD_LOCK_H
#define HQD_LOCK_H


#include <stdbool.h>
#ifdef USE_HQD_LOCK
#    include <sched.h>
#endif
#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include "misc/padded_types.h"
#include "locks/tatas_lock.h"
#include "locks/ticket_lock.h"
#include "locks/qd_lock.h"
#include "qd_queues/qd_queue.h"

#ifdef BULLDOZER
#define HQD_NUMBER_OF_NUMA_NODES 8
#else
#define HQD_NUMBER_OF_NUMA_NODES 4
#endif
/* Hierarchical Queue Delegation Lock */

typedef struct {
    TicketLock globalLock;
    QDLock qdlocks[HQD_NUMBER_OF_NUMA_NODES]; 
} HQDLock;

typedef union {
    volatile unsigned value;
    char padding[CACHE_LINE_SIZE];
} HQDPaddedUnsigned;

void hqd_initialize(HQDLock * lock);
void hqd_lock(void * lock);
void hqd_unlock(void * lock);
static inline
bool hqd_is_locked(void * lock){
    HQDLock *l = (HQDLock*)lock;
    return ticket_is_locked(&l->globalLock);
}
bool hqd_try_lock(void * lock);
void hqd_delegate(void* lock,
                 void (*funPtr)(unsigned int, void *), 
                 unsigned int messageSize,
                 void * messageAddress);
void * hqd_delegate_or_lock(void* lock,
                           unsigned int messageSize);
void * hqd_delegate_or_lock_extra(void* lock,
                                  unsigned int messageSize,
                                  bool openQueue);
void hqd_open_delegation_queue(void* lock);

void hqd_flush_delegation_queue(void* lock);
void hqd_close_delegate_buffer(void * buffer,
                              void (*funPtr)(unsigned int, void *));
void hqd_delegate_unlock(void* lock);
void hqd_delegate_wait(void* lock,
                      void (*funPtr)(unsigned int, void *), 
                      unsigned int messageSize,
                      void * messageAddress);
HQDLock * plain_hqd_create();
OOLock * oo_hqd_create();

#endif
