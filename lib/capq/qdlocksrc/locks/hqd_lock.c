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

#include "hqd_lock.h"


_Alignas(CACHE_LINE_SIZE)
__thread HQDPaddedUnsigned hqd_thread_numa_node = {.value = 1000001};

_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable HQD_LOCK_METHOD_TABLE = 
{
     .free = &free,
     .lock = &hqd_lock,
     .unlock = &hqd_unlock,
     .is_locked = &hqd_is_locked,
     .try_lock = &hqd_try_lock,
     .rlock = &hqd_lock,
     .runlock = &hqd_unlock,
     .delegate = &hqd_delegate,
     .delegate_wait = &hqd_delegate_wait,
     .delegate_or_lock = &hqd_delegate_or_lock,
     .close_delegate_buffer = &hqd_close_delegate_buffer,
     .delegate_unlock = &hqd_delegate_unlock
};



void hqd_initialize(HQDLock * lock){
    //    printf("HQD INIT %p\n", lock);
    ticket_initialize(&lock->globalLock);
    for(int i = 0; i < HQD_NUMBER_OF_NUMA_NODES; i++){
        qd_initialize(&lock->qdlocks[i]);
    }
}

static inline unsigned hqd_get_numa_node(){
#ifdef USE_HQD_LOCK
    if(hqd_thread_numa_node.value == 1000001){
        int cpu = 0;
        if(-1 == (cpu = sched_getcpu())){
            fprintf(stderr, "An error occured when requesting NUMA node\n");
            exit(0);
        }else{
#ifdef BULLDOZER
            int nodeCoresArray[8][8] = {  
                {0,4,8,12,16,20,24,28},
                {32,36,40,44,48,52,56,60},
                {2,6,10,14,18,22,26,30},
                {34,38,42,46,50,54,58,62},
                {3,7,11,15,19,23,27,31},
                {35,39,43,47,51,55,59,63},
                {1,5,9,13,17,21,25,29},
                {33,37,41,45,49,53,57,61}
            };
            for(int node = 0; node < 8; node++){
                for(int core = 0; core < 8; core++){
                    if(nodeCoresArray[node][core]==cpu){
                        hqd_thread_numa_node.value = node;
                    }
                }
            }
#else
            hqd_thread_numa_node.value = cpu % HQD_NUMBER_OF_NUMA_NODES;
#endif
        }
    }
    return hqd_thread_numa_node.value;
#else
    return 0;
#endif
}

void hqd_lock(void * lock) {
    HQDLock *l = (HQDLock*)lock;
    qd_lock(&l->qdlocks[hqd_get_numa_node()]);
    ticket_lock(&l->globalLock);
    //printf("HQDLOCK %p %d\n", l, pthread_self());
}


void hqd_unlock(void * lock) {
    HQDLock *l = (HQDLock*)lock;
    //printf("UNLOCK\n");
    ticket_unlock(&l->globalLock);
    qd_unlock(&l->qdlocks[hqd_get_numa_node()]);
    //printf("HQD UNLOCK %p %d \n", l, pthread_self());
}


bool hqd_try_lock(void * lock) {
    HQDLock *l = (HQDLock*)lock;
    if(ticket_try_lock(&l->globalLock)){
        if(qd_try_lock(&l->qdlocks[hqd_get_numa_node()])){
            //printf("TRY LOCK SUCCESS %p %d\n", l, pthread_self());
            return true;
        }
        ticket_unlock(&l->globalLock);
        //printf("TRY LOCK FAIL %p %d\n", l, pthread_self());
    }  
    return false;
}


void hqd_delegate(void* lock,
                 void (*funPtr)(unsigned int, void *), 
                 unsigned int messageSize,
                 void * messageAddress) {
    HQDLock *h = (HQDLock*)lock;
    QDLock *l = &h->qdlocks[hqd_get_numa_node()];
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            ticket_lock(&h->globalLock);
            qdq_open(&l->queue);
            funPtr(messageSize, messageAddress);
            qdq_flush(&l->queue);
            ticket_unlock(&h->globalLock);
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

void * hqd_delegate_or_lock_extra(void* lock,
                                 unsigned int messageSize,
                                 bool openQueue) {
    HQDLock *h = (HQDLock*)lock;
    QDLock *l = &h->qdlocks[hqd_get_numa_node()];
    void * buffer;
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            ticket_lock(&h->globalLock);
            if(openQueue)qdq_open(&l->queue);
            //printf("DELEGATE OR LOCK GOT LOCK %d\n", pthread_self());
            return NULL;
        } else if(NULL != (buffer = qdq_enqueue_get_buffer(&l->queue,
                                                           messageSize))){
            //printf("DELEGATE OR LOCK DELEGATE %d\n", pthread_self());
            return buffer;
        }
        thread_yield();
    }
}

void * hqd_delegate_or_lock(void* lock,
                           unsigned int messageSize) {
    return hqd_delegate_or_lock_extra(lock,
                                      messageSize,
                                      true);
}

void hqd_open_delegation_queue(void* lock){
    HQDLock *h = (HQDLock*)lock;
    QDLock *l = &h->qdlocks[hqd_get_numa_node()];
    qdq_open(&l->queue);
}

void hqd_flush_delegation_queue(void* lock){
    HQDLock *h = (HQDLock*)lock;
    QDLock *l = &h->qdlocks[hqd_get_numa_node()];
    qdq_flush(&l->queue);
}

void hqd_close_delegate_buffer(void * buffer,
                              void (*funPtr)(unsigned int, void *)){
    qdq_enqueue_close_buffer(buffer, funPtr);
}

 
void hqd_delegate_unlock(void* lock) {
    HQDLock *h = (HQDLock*)lock;
    QDLock *l = &h->qdlocks[hqd_get_numa_node()];
    qdq_flush(&l->queue);
    //printf("DELEGATE UNLOCK %d\n", pthread_self());
    ticket_unlock(&h->globalLock);
    tatas_unlock(&l->mutexLock);
}

void hqd_executeAndWaitCS(unsigned int size, void * data){
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


void hqd_delegate_wait(void* lock,
                      void (*funPtr)(unsigned int, void *), 
                      unsigned int messageSize,
                      void * messageAddress) {
    volatile atomic_int waitVar = ATOMIC_VAR_INIT(1);
    unsigned int metaDataSize = sizeof(volatile atomic_int *) + 
        sizeof(void (*)(unsigned int, void *));
    char * buff = hqd_delegate_or_lock(lock,
                                      metaDataSize + messageSize);
    if(buff==NULL){
        funPtr(messageSize, messageAddress);
        hqd_delegate_unlock(lock);
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
        hqd_close_delegate_buffer((void *)buff, hqd_executeAndWaitCS);
        while(atomic_load_explicit(&waitVar, memory_order_acquire)){
            thread_yield();
        }
    }
}


HQDLock * plain_hqd_create(){
    HQDLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(HQDLock));
    hqd_initialize(l);
    return l;
}

OOLock * oo_hqd_create(){
    HQDLock * l = plain_hqd_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &HQD_LOCK_METHOD_TABLE;
    return ool;
}
