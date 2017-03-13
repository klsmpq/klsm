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

#include "mrqd_lock.h"

_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable MRQD_LOCK_METHOD_TABLE = 
{
    .free = &free,
    .lock = &mrqd_lock,
    .unlock = &mrqd_unlock,
    .is_locked = &mrqd_is_locked,
    .try_lock = &mrqd_try_lock,
    .rlock = &mrqd_rlock,
    .runlock = &mrqd_runlock,
    .delegate = &mrqd_delegate,
    .delegate_wait = &mrqd_delegate_wait,
    .delegate_or_lock = &mrqd_delegate_or_lock,
    .close_delegate_buffer = &mrqd_close_delegate_buffer,
    .delegate_unlock = &mrqd_delegate_unlock
};

void mrqd_initialize(MRQDLock * lock){
    tatas_initialize(&lock->mutexLock);
    qdq_initialize(&lock->queue);
    atomic_store(&lock->writeBarrier.value, 0);
    reader_groups_initialize(&lock->readIndicator);
}

void mrqd_lock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    while(atomic_load_explicit(&l->writeBarrier.value, memory_order_seq_cst) > 0){
        thread_yield();
    }
    tatas_lock(&l->mutexLock);
    rgri_wait_all_readers_gone(&l->readIndicator);
}

void mrqd_unlock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    tatas_unlock(&l->mutexLock);
}

bool mrqd_is_locked(void * lock){
    MRQDLock *l = (MRQDLock*)lock;
    return tatas_is_locked(&l->mutexLock);
}

bool mrqd_try_lock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    while(atomic_load_explicit(&l->writeBarrier.value, memory_order_seq_cst) > 0){
        thread_yield();
    }
    if(tatas_try_lock(&l->mutexLock)){
        rgri_wait_all_readers_gone(&l->readIndicator);
        return true;
    }else{
        return false;
    } 
}

void mrqd_rlock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    bool bRaised = false;
    int readPatience = 0;
 start:
    rgri_arrive(&l->readIndicator);
    if(tatas_is_locked(&l->mutexLock)) {
        rgri_depart(&l->readIndicator);
        while(tatas_is_locked(&l->mutexLock)) {
            thread_yield();
            if((readPatience == MRQD_READ_PATIENCE_LIMIT) && !bRaised) {
                atomic_fetch_add_explicit(&l->writeBarrier.value, 1, memory_order_seq_cst);
                bRaised = true;
            }
            readPatience = readPatience + 1;
        }
        goto start;
    }
    if(bRaised) {
        atomic_fetch_sub_explicit(&l->writeBarrier.value, 1, memory_order_seq_cst);
    }
}

void mrqd_runlock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    rgri_depart(&l->readIndicator);
}

void mrqd_delegate(void* lock,
                   void (*funPtr)(unsigned int, void *), 
                   unsigned int messageSize,
                   void * messageAddress) {
    MRQDLock *l = (MRQDLock*)lock;
    while(atomic_load_explicit(&l->writeBarrier.value, memory_order_seq_cst) > 0){
        thread_yield();
    }
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            qdq_open(&l->queue);
            rgri_wait_all_readers_gone(&l->readIndicator);
            funPtr(messageSize, messageAddress);
            qdq_flush(&l->queue);
            tatas_unlock(&l->mutexLock);
            return;
        } else if(qdq_enqueue(&l->queue,
                              funPtr,
                              messageSize,
                              messageAddress)){
            return;
        }
        thread_yield();
    }
}

void * mrqd_delegate_or_lock(void* lock,
                             unsigned int messageSize) {
    MRQDLock *l = (MRQDLock*)lock;
    void * buffer;
    while(atomic_load_explicit(&l->writeBarrier.value, memory_order_seq_cst) > 0){
        thread_yield();
    }
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            qdq_open(&l->queue);
            rgri_wait_all_readers_gone(&l->readIndicator);
            return NULL;
        } else if(NULL != (buffer = qdq_enqueue_get_buffer(&l->queue,
                                                           messageSize))){
            return buffer;
        }
        thread_yield();
    }
}

void mrqd_close_delegate_buffer(void * buffer,
                                void (*funPtr)(unsigned int, void *)){
    qdq_enqueue_close_buffer(buffer, funPtr);
}

void mrqd_delegate_unlock(void* lock) {
    MRQDLock *l = (MRQDLock*)lock;
    qdq_flush(&l->queue);
    tatas_unlock(&l->mutexLock);
}

void mrqd_executeAndWaitCS(unsigned int size, void * data){
    char * buff = data;
    volatile atomic_int * writeBackAddress = *((volatile atomic_int **)buff);
    void (*csFunc)(unsigned int, void *) = 
        *((void (**)(unsigned int, void *))&(buff[sizeof(volatile atomic_int *)]));
    unsigned int metaDataSize = sizeof(volatile atomic_int *) + 
        sizeof(void (*)(unsigned int, void *));
    void * csData = (void*)&(buff[metaDataSize]);
    csFunc(size - metaDataSize, csData);
    atomic_store_explicit(writeBackAddress, 0, memory_order_release);
}

void mrqd_delegate_wait(void* lock,
                      void (*funPtr)(unsigned int, void *), 
                      unsigned int messageSize,
                      void * messageAddress) {
    volatile atomic_int waitVar = ATOMIC_VAR_INIT(1);
    unsigned int metaDataSize = sizeof(volatile atomic_int *) + 
        sizeof(void (*)(unsigned int, void *));
    char * buff = mrqd_delegate_or_lock(lock,
                                        metaDataSize + messageSize);
    if(buff==NULL){
        funPtr(messageSize, messageAddress);
        mrqd_delegate_unlock(lock);
    }else{
        volatile atomic_int ** waitVarPtrAddress = (volatile atomic_int **)buff;
        *waitVarPtrAddress = &waitVar;
        void (**funPtrAdress)(unsigned int, void *) = (void (**)(unsigned int, void *))&buff[sizeof(volatile atomic_int *)];
        *funPtrAdress = funPtr;
        unsigned int metaDataSize = sizeof(volatile atomic_int *) + 
            sizeof(void (*)(unsigned int, void *));
        char * msgBuffer = (char *)messageAddress;
        for(unsigned int i = metaDataSize; i < (messageSize + metaDataSize); i++){
            buff[i] = msgBuffer[i - metaDataSize];
        }
        mrqd_close_delegate_buffer((void *)buff, mrqd_executeAndWaitCS);
        while(atomic_load_explicit(&waitVar, memory_order_acquire)){
            thread_yield();
        }
    }
}

MRQDLock * plain_mrqd_create(){
    MRQDLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(MRQDLock));
    mrqd_initialize(l);
    return l;
}

OOLock * oo_mrqd_create(){
    MRQDLock * l = plain_mrqd_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &MRQD_LOCK_METHOD_TABLE;
    return ool;
}
