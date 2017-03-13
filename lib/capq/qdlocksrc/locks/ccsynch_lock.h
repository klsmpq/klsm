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

#ifndef CCSYNCH_LOCK_H
#define CCSYNCH_LOCK_H

// Implementation of the CC-Synch algorithm as described in the paper:
// Revisiting the combining synchronization technique
// by Panagiota Fatourou and Nikolaos D. Kallimanis
// PPoPP '12 Proceedings of the 17th ACM SIGPLAN symposium on 
// Principles and Practice of Parallel Programming

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>

#include "misc/padded_types.h"
#include "locks/oo_lock_interface.h"

#define CCSYNCH_BUFFER_SIZE 512
#define CCSYNCH_HAND_OFF_LIMIT 512

typedef struct {
    volatile atomic_uintptr_t next;
    void (*requestFunction)(unsigned int, void *);
    unsigned int messageSize;
    unsigned char * buffer;
    bool completed;
    volatile atomic_int wait;
    char pad2[CACHE_LINE_SIZE_PAD(sizeof(void *)*2 + 
                                  sizeof(unsigned int) +
                                  CCSYNCH_BUFFER_SIZE +
                                  sizeof(bool) +
                                  sizeof(atomic_int))];
    unsigned char tempBuffer[CACHE_LINE_SIZE*8]; //used in ccsynch_delegate_or_lock 
} CCSynchLockNode;

typedef struct {
    LLPaddedPointer tailPtr;
} CCSynchLock;


// Public interface

void ccsynch_initialize(CCSynchLock * l);
void ccsynch_lock(void * lock);
void ccsynch_unlock(void * lock);
bool ccsynch_is_locked(void * lock);
bool ccsynch_try_lock(void * lock);
void ccsynch_delegate(void* lock,
                      void (*funPtr)(unsigned int, void *), 
                      unsigned int messageSize,
                      void * messageAddress);
void * ccsynch_delegate_or_lock(void* lock,
                                unsigned int messageSize);
void ccsynch_close_delegate_buffer(void * buffer,
                                   void (*funPtr)(unsigned int, void *));
void ccsynch_delegate_unlock(void* lock);
CCSynchLock * plain_ccsynch_create();
OOLock * oo_ccsynch_create();

#endif
