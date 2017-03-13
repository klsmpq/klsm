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

#include "qd_lock.h"


_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable QD_LOCK_METHOD_TABLE = 
{
     .free = &free,
     .lock = &qd_lock,
     .unlock = &qd_unlock,
     .is_locked = &qd_is_locked,
     .try_lock = &qd_try_lock,
     .rlock = &qd_lock,
     .runlock = &qd_unlock,
     .delegate = &qd_delegate,
     .delegate_wait = &qd_delegate_wait,
     .delegate_or_lock = &qd_delegate_or_lock,
     .close_delegate_buffer = &qd_close_delegate_buffer,
     .delegate_unlock = &qd_delegate_unlock
};



void qd_initialize(QDLock * lock){
    tatas_initialize(&lock->mutexLock);
    qdq_initialize(&lock->queue);
}

 void qd_lock(void * lock) {
    QDLock *l = (QDLock*)lock;
    tatas_lock(&l->mutexLock);
}


void qd_unlock(void * lock) {
    QDLock *l = (QDLock*)lock;
    tatas_unlock(&l->mutexLock);
}


bool qd_try_lock(void * lock) {
    QDLock *l = (QDLock*)lock;
    return tatas_try_lock(&l->mutexLock);
}


void qd_delegate(void* lock,
                 void (*funPtr)(unsigned int, void *), 
                 unsigned int messageSize,
                 void * messageAddress) {
    QDLock *l = (QDLock*)lock;
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            qdq_open(&l->queue);
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

void * qd_delegate_or_lock_extra(void* lock,
                                 unsigned int messageSize,
                                 bool openQueue,
				 bool * contendedWriteBack) {
    QDLock *l = (QDLock*)lock;
    void * buffer;
    if(contendedWriteBack != NULL){
      *contendedWriteBack = false;
    }
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            if(openQueue)qdq_open(&l->queue);
            return NULL;
        } else if(NULL != (buffer = qdq_enqueue_get_buffer(&l->queue,
                                                           messageSize))){
	  if(contendedWriteBack != NULL){
	    *contendedWriteBack = true;
	  }
	  return buffer;
        }
	if(contendedWriteBack != NULL){
	    *contendedWriteBack = true;
	}
        thread_yield();
    }
}

void * qd_delegate_or_lock(void* lock,
                           unsigned int messageSize) {
    return qd_delegate_or_lock_extra(lock,
                                     messageSize,
                                     true,
				     NULL);
}

void qd_open_delegation_queue(void* lock){
    QDLock *l = (QDLock*)lock;
    qdq_open(&l->queue);
}

void qd_flush_delegation_queue(void* lock){
    QDLock *l = (QDLock*)lock;
    qdq_flush(&l->queue);
}

void qd_close_delegate_buffer(void * buffer,
                              void (*funPtr)(unsigned int, void *)){
    qdq_enqueue_close_buffer(buffer, funPtr);
}

 
void qd_delegate_unlock(void* lock) {
    QDLock *l = (QDLock*)lock;
    qdq_flush(&l->queue);
    tatas_unlock(&l->mutexLock);
}

void qd_executeAndWaitCS(unsigned int size, void * data){
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


void qd_delegate_wait(void* lock,
                      void (*funPtr)(unsigned int, void *), 
                      unsigned int messageSize,
                      void * messageAddress) {
    volatile atomic_int waitVar = ATOMIC_VAR_INIT(1);
    unsigned int metaDataSize = sizeof(volatile atomic_int *) + 
        sizeof(void (*)(unsigned int, void *));
    char * buff = qd_delegate_or_lock(lock,
                                      metaDataSize + messageSize);
    if(buff==NULL){
        funPtr(messageSize, messageAddress);
        qd_delegate_unlock(lock);
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
        qd_close_delegate_buffer((void *)buff, qd_executeAndWaitCS);
        while(atomic_load_explicit(&waitVar, memory_order_acquire)){
            thread_yield();
        }
    }
}


QDLock * plain_qd_create(){
    QDLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(QDLock));
    qd_initialize(l);
    return l;
}

OOLock * oo_qd_create(){
    QDLock * l = plain_qd_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &QD_LOCK_METHOD_TABLE;
    return ool;
}
