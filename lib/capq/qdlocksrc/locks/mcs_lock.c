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

#include "mcs_lock.h"

_Alignas(CACHE_LINE_SIZE) 
__thread MCSNode myMCSNode[2] = {
    {
        .next.value = ATOMIC_VAR_INIT((intptr_t)NULL),
        .locked.value = ATOMIC_VAR_INIT(0)
    },
    {
        .next.value = ATOMIC_VAR_INIT((intptr_t)NULL),
        .locked.value = ATOMIC_VAR_INIT(0)
    }
};

_Alignas(CACHE_LINE_SIZE)
__thread PaddedCurrentNodeIndex myMCSCurrentNodeIndex = {.index = 0};

_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable MCS_LOCK_METHOD_TABLE = 
{
     .free = &free,
     .lock = &mcs_lock,
     .unlock = &mcs_unlock,
     .is_locked = &mcs_is_locked,
     .try_lock = &mcs_try_lock,
     .rlock = &mcs_lock,
     .runlock = &mcs_unlock,
     .delegate = &mcs_delegate,
     .delegate_wait = &mcs_delegate,
     .delegate_or_lock = &mcs_delegate_or_lock,
     .close_delegate_buffer = NULL, /* Should never be called */
     .delegate_unlock = &mcs_unlock
};


void mcs_initialize(MCSLock * lock){
    volatile atomic_intptr_t tmp = ATOMIC_VAR_INIT((intptr_t)NULL); 
    lock->endOfQueue.value = tmp;
}

bool mcs_lock_status(void * lock) {
    MCSLock * l = lock;
    MCSNode * node = &myMCSNode[myMCSCurrentNodeIndex.index];
    atomic_store_explicit(&node->next.value, (intptr_t)NULL, memory_order_relaxed);
    MCSNode * predecessor = (MCSNode *)atomic_exchange_explicit( &l->endOfQueue.value, (intptr_t)node, memory_order_release);
    if (predecessor != NULL) {
        atomic_store_explicit(&node->locked.value, 1, memory_order_relaxed);
        atomic_store_explicit(&predecessor->next.value, (intptr_t)node, memory_order_release);
        //Wait
        while (atomic_load_explicit(&node->locked.value, memory_order_acquire)) {
            thread_yield();
        }
        myMCSCurrentNodeIndex.index++;
        return true;
    }else{
        myMCSCurrentNodeIndex.index++;
        return false;
    }
}

void mcs_lock(void * lock) {
    mcs_lock_status(lock);
}

void mcs_unlock(void * lock) {
    MCSLock * l = lock;
    myMCSCurrentNodeIndex.index--;
    MCSNode * node = &myMCSNode[myMCSCurrentNodeIndex.index];
    MCSNode * nodeConst = &myMCSNode[myMCSCurrentNodeIndex.index];
    if (NULL == (MCSNode *)atomic_load_explicit(&node->next.value, memory_order_acquire)) {
        if (atomic_compare_exchange_strong(&l->endOfQueue.value,
                                           (intptr_t*)&node,
                                           (intptr_t)NULL)){
            return;
        }
        //wait
        while ((intptr_t)NULL == atomic_load_explicit(&nodeConst->next.value, memory_order_acquire)) {
            thread_yield();
        }
    }
    MCSNode * nextNode = (MCSNode*)atomic_load_explicit(&nodeConst->next.value, memory_order_relaxed);
    atomic_store_explicit(&nextNode->locked.value, 0, memory_order_release);
}

bool mcs_try_lock(void * lock) {
    MCSLock * l = lock;
    MCSNode * node = &myMCSNode[myMCSCurrentNodeIndex.index];
    if(atomic_load_explicit(&l->endOfQueue.value, memory_order_acquire) != (intptr_t)NULL){
        return false;
    }else{
        atomic_store_explicit(&node->next.value, (intptr_t) NULL, memory_order_relaxed);
        intptr_t expected = (intptr_t)NULL;
        bool success = atomic_compare_exchange_strong(&l->endOfQueue.value,
                                                      &expected,
                                                      (intptr_t)node);       
        if(success){
            myMCSCurrentNodeIndex.index++;
            return true; 
        }else{
            return false;
        }
    }
}

void mcs_delegate(void * lock,
                  void (*funPtr)(unsigned int, void *), 
                  unsigned int messageSize,
                  void * messageAddress){
    MCSLock *l = (MCSLock*)lock;
    mcs_lock(l);
    funPtr(messageSize, messageAddress);
    mcs_unlock(l);
}

void * mcs_delegate_or_lock(void * lock, unsigned int messageSize){
    (void)messageSize;
    MCSLock *l = (MCSLock*)lock;
    mcs_lock(l);
    return NULL;
}

MCSLock * plain_mcs_create(){
    MCSLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(MCSLock));
    mcs_initialize(l);
    return l;
}

OOLock * oo_mcs_create(){
    MCSLock * l = plain_mcs_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &MCS_LOCK_METHOD_TABLE;
    return ool;
}
