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

#ifndef READER_GROUPS_READ_INDICATOR_H
#define READER_GROUPS_READ_INDICATOR_H

#include "misc/thread_includes.h"
#include "misc/padded_types.h"
#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available

/* Read Indicator */

#ifndef MRQD_LOCK_NUMBER_OF_READER_GROUPS
#    define MRQD_LOCK_NUMBER_OF_READER_GROUPS 8
#endif

//Warning this will not work when using more threads than reader groups
// #define MRQD_LOCK_READER_GROUP_PER_THREAD 1

typedef struct {
    LLPaddedUInt readerGroups[MRQD_LOCK_NUMBER_OF_READER_GROUPS];
} ReaderGroupsReadIndicator;

typedef union {
    int value;
    char pad[CACHE_LINE_SIZE];
} RGRIGetThreadIDVarWrapper;



extern volatile atomic_int rgri_get_thread_id_counter;

extern
_Alignas(CACHE_LINE_SIZE)
_Thread_local RGRIGetThreadIDVarWrapper rgri_get_thread_id_var;


static inline
void reader_groups_initialize(ReaderGroupsReadIndicator * readIndicator){
    for(int i = 0; i < MRQD_LOCK_NUMBER_OF_READER_GROUPS; i++){
        atomic_store(&readIndicator->readerGroups[i].value, 0);
    }
}

static inline
int rgri_get_thread_id(){
    if(rgri_get_thread_id_var.value > -1) {
        return rgri_get_thread_id_var.value;
    } else {
        rgri_get_thread_id_var.value = atomic_fetch_add(&rgri_get_thread_id_counter, 1);
        return rgri_get_thread_id_var.value;
    }
}

static inline
void rgri_arrive(ReaderGroupsReadIndicator * indicator){
    int index = rgri_get_thread_id() % MRQD_LOCK_NUMBER_OF_READER_GROUPS;
#ifdef MRQD_LOCK_READER_GROUP_PER_THREAD
    atomic_store(&indicator->readerGroups[index].value, 1);
#else
    atomic_fetch_add(&indicator->readerGroups[index].value, 1);
#endif
}

static inline
void rgri_depart(ReaderGroupsReadIndicator * indicator){
    int index = rgri_get_thread_id() % MRQD_LOCK_NUMBER_OF_READER_GROUPS;
#ifdef MRQD_LOCK_READER_GROUP_PER_THREAD
    atomic_store_explicit(&indicator->readerGroups[index].value, 0, memory_order_release);
#else
    atomic_fetch_sub_explicit(&indicator->readerGroups[index].value, 1, memory_order_release);
#endif
}

static inline
void rgri_wait_all_readers_gone(ReaderGroupsReadIndicator * indicator){
    atomic_thread_fence(memory_order_seq_cst);
    for(int i = 0; i < MRQD_LOCK_NUMBER_OF_READER_GROUPS; i++){
        while(0 < atomic_load_explicit(&indicator->readerGroups[i].value, memory_order_acquire)){
            thread_yield();
        }
    }
}

#endif
