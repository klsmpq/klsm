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

#include "ccsynch_lock.h"

_Alignas(CACHE_LINE_SIZE)
__thread CCSynchLockNode * ccsynchNextLocalNode = NULL;


_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable CCSYNCH_LOCK_METHOD_TABLE = 
{
     .free = &free,
     .lock = &ccsynch_lock,
     .unlock = &ccsynch_unlock,
     .is_locked = &ccsynch_is_locked,
     .try_lock = &ccsynch_try_lock,
     .rlock = &ccsynch_lock,
     .runlock = &ccsynch_unlock,
     .delegate = &ccsynch_delegate,
     .delegate_wait = &ccsynch_delegate,
     .delegate_or_lock = &ccsynch_delegate_or_lock,
     .close_delegate_buffer = &ccsynch_close_delegate_buffer,
     .delegate_unlock = &ccsynch_delegate_unlock
};


static
void ccsynchlock_initNode(CCSynchLockNode * node){
    node->requestFunction = NULL;
    node->messageSize = CCSYNCH_BUFFER_SIZE + 1;
    atomic_store_explicit(&node->wait, 0, memory_order_relaxed);
    node->completed = false;
    volatile atomic_uintptr_t tmp = ATOMIC_VAR_INIT((uintptr_t)NULL);
    node->next = tmp;
}

static inline
void ccsynchlock_initLocalIfNeeded(){
    if(ccsynchNextLocalNode == NULL){
        ccsynchNextLocalNode = aligned_alloc(CACHE_LINE_SIZE, sizeof(CCSynchLockNode));
        ccsynchlock_initNode(ccsynchNextLocalNode);
    }
}

void ccsynch_initialize(CCSynchLock * l){
    CCSynchLockNode * dummyNode = aligned_alloc(CACHE_LINE_SIZE, sizeof(CCSynchLockNode));
    ccsynchlock_initNode(dummyNode);
    atomic_store_explicit(&l->tailPtr.value, (uintptr_t)dummyNode, memory_order_release);
}

void ccsynch_lock(void * lock) {
    CCSynchLock *l = (CCSynchLock*)lock;
    CCSynchLockNode *nextNode;
    CCSynchLockNode *curNode;
    ccsynchlock_initLocalIfNeeded();
    nextNode = ccsynchNextLocalNode;
    atomic_store_explicit(&nextNode->next, (uintptr_t)NULL, memory_order_relaxed);
    atomic_store_explicit(&nextNode->wait, 1, memory_order_relaxed);
    nextNode->completed = false;
    curNode = (CCSynchLockNode *)atomic_exchange_explicit( &l->tailPtr.value, (uintptr_t)nextNode, memory_order_release);
    curNode->requestFunction = NULL;
    atomic_store_explicit(&curNode->next, (uintptr_t)nextNode, memory_order_release);
    ccsynchNextLocalNode = curNode;
    while (atomic_load_explicit(&curNode->wait, memory_order_acquire) == 1){
        thread_yield();
    }
}

void ccsynch_unlock(void * lock) {
    (void)lock;
    CCSynchLockNode *tmpNode;
    void (*tmpFunPtr)(unsigned int, void *);
    CCSynchLockNode *tmpNodeNext;
    int counter = 0;
    tmpNode = ccsynchNextLocalNode;
    while ((tmpNodeNext=(CCSynchLockNode *)atomic_load_explicit(&tmpNode->next, memory_order_acquire)) != NULL && counter < CCSYNCH_HAND_OFF_LIMIT) {
        counter = counter + 1;
        tmpFunPtr = tmpNode->requestFunction;
        if(tmpFunPtr==NULL){
            tmpNode = tmpNodeNext;
            break;
        }
        tmpFunPtr(tmpNode->messageSize, tmpNode->buffer);
        tmpNode->completed = true;
        atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
        tmpNode = tmpNodeNext;
    }
    atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
}

bool ccsynch_is_locked(void * lock) {
    CCSynchLock *l = (CCSynchLock*)lock;
    CCSynchLockNode * tail =  (CCSynchLockNode *)atomic_load_explicit(&l->tailPtr.value, memory_order_acquire);
    return atomic_load_explicit(&tail->wait, memory_order_acquire) == 1;
}


bool ccsynch_try_lock(void * lock) {
    if(ccsynch_is_locked(lock)){
        return false;
    }else{
        ccsynch_lock(lock);
        return true;
    }
}


void ccsynch_delegate(void* lock,
                      void (*funPtr)(unsigned int, void *), 
                      unsigned int messageSize,
                      void * messageAddress) {
    CCSynchLock *l = (CCSynchLock*)lock;
    unsigned char * messageBuffer =  (unsigned char *) messageAddress;
    CCSynchLockNode *nextNode;
    CCSynchLockNode *curNode;
    CCSynchLockNode *tmpNode;
    void (*tmpFunPtr)(unsigned int, void *);
    CCSynchLockNode *tmpNodeNext;
    int counter = 0;
    ccsynchlock_initLocalIfNeeded();
    nextNode = ccsynchNextLocalNode;
    atomic_store_explicit(&nextNode->next, (uintptr_t)NULL, memory_order_relaxed);
    atomic_store_explicit(&nextNode->wait, 1, memory_order_relaxed);
    nextNode->completed = false;
    curNode = (CCSynchLockNode *)atomic_exchange_explicit(&l->tailPtr.value, (uintptr_t)nextNode, memory_order_release);
    curNode->buffer = messageBuffer;
    curNode->messageSize = messageSize;
    curNode->requestFunction = funPtr;
    atomic_store_explicit(&curNode->next, (uintptr_t)nextNode, memory_order_release);
    ccsynchNextLocalNode = curNode;
    while (atomic_load_explicit(&curNode->wait, memory_order_acquire) == 1){
        thread_yield();
    }
    if(curNode->completed==true){
        return;
    }else{
        funPtr(messageSize, messageBuffer);
    }
    tmpNode = (CCSynchLockNode *)atomic_load_explicit(&curNode->next, memory_order_acquire);
    while ((tmpNodeNext=(CCSynchLockNode *)atomic_load_explicit(&tmpNode->next, memory_order_acquire)) != NULL && counter < CCSYNCH_HAND_OFF_LIMIT) {
        counter = counter + 1;
        tmpFunPtr = tmpNode->requestFunction;
        if(tmpFunPtr==NULL){
            break;
        }
        tmpFunPtr(tmpNode->messageSize, tmpNode->buffer);
        tmpNode->completed = true;
        atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
        tmpNode = tmpNodeNext;
    }
    atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
}


void * ccsynch_delegate_or_lock(void* lock,
                                unsigned int messageSize) {
    CCSynchLock *l = (CCSynchLock*)lock;
    CCSynchLockNode *nextNode;
    CCSynchLockNode *curNode;
    ccsynchlock_initLocalIfNeeded();
    nextNode = ccsynchNextLocalNode;
    atomic_store_explicit(&nextNode->next, (uintptr_t)NULL, memory_order_relaxed);
    atomic_store_explicit(&nextNode->wait, 1, memory_order_relaxed);
    nextNode->completed = false;
    curNode = (CCSynchLockNode *)atomic_exchange_explicit(&l->tailPtr.value, (uintptr_t)nextNode, memory_order_release);
    curNode->messageSize = messageSize;

    curNode->requestFunction = NULL; //Forces helper to stop if it sees this

    atomic_store_explicit(&curNode->next, (uintptr_t)nextNode, memory_order_release);

    ccsynchNextLocalNode = curNode;
    if (atomic_load_explicit(&curNode->wait, memory_order_acquire) == 1){
        //Somone else has the lock delegate
        return curNode->tempBuffer;
    }else{
        //Yey we got the lock
        return NULL;
    }

}

void ccsynch_close_delegate_buffer(void * buffer,
                                   void (*funPtr)(unsigned int, void *)){
    CCSynchLockNode *tmpNode;
    void (*tmpFunPtr)(unsigned int, void *);
    CCSynchLockNode *tmpNodeNext;
    int counter = 0;
    CCSynchLockNode *curNode = ccsynchNextLocalNode;
    curNode->buffer = buffer;
    curNode->requestFunction = funPtr;
    atomic_thread_fence( memory_order_release );
    while (atomic_load_explicit(&curNode->wait, memory_order_acquire) == 1){
        thread_yield();
    }
    if(curNode->completed==true){
        return;
    }else{
        funPtr(curNode->messageSize, buffer);
    }
    tmpNode = (CCSynchLockNode *)atomic_load_explicit(&curNode->next, memory_order_acquire);

    while ((tmpNodeNext=(CCSynchLockNode *)atomic_load_explicit(&tmpNode->next, memory_order_acquire)) != NULL && counter < CCSYNCH_HAND_OFF_LIMIT) {
        counter = counter + 1;
        tmpFunPtr = tmpNode->requestFunction;
        if(tmpFunPtr==NULL){
            break;
        }
        tmpFunPtr(tmpNode->messageSize, tmpNode->buffer);
        tmpNode->completed = true;
        atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
        tmpNode = tmpNodeNext;
    }
    atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
}


void ccsynch_delegate_unlock(void* lock) {
    ccsynch_unlock(lock);
}


CCSynchLock * plain_ccsynch_create(){
    CCSynchLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(CCSynchLock));
    ccsynch_initialize(l);
    return l;
}


OOLock * oo_ccsynch_create(){
    CCSynchLock * l = plain_ccsynch_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &CCSYNCH_LOCK_METHOD_TABLE;
    return ool;
}
