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

#ifndef Ticket_LOCK_H
#define Ticket_LOCK_H

#include "locks/oo_lock_interface.h"
#include "misc/padded_types.h"

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>


typedef struct TicketLockImpl {
    volatile atomic_ulong ingress;
    volatile atomic_ulong egress;
    char pad[CACHE_LINE_SIZE_PAD(2*sizeof(atomic_ulong))];
} TicketLock;


void ticket_initialize(TicketLock * lock);
static inline
void ticket_lock(void * lock) {
    TicketLock *l = (TicketLock*)lock;
    //printf("TICKET LOCK %p\n", l);
    unsigned long prevValue = atomic_fetch_add_explicit(&l->ingress, 1, memory_order_acquire);
    while(atomic_load_explicit(&l->egress, 
                               memory_order_acquire) != prevValue){
        thread_yield();
    }
}
static inline
void ticket_unlock(void * lock) {
    TicketLock *l = (TicketLock*)lock;
    atomic_fetch_add_explicit(&l->egress, 1, memory_order_release);
}
static inline
bool ticket_is_locked(void * lock){
    TicketLock *l = (TicketLock*)lock;
    return atomic_load(&l->egress) != atomic_load(&l->ingress);
}
static inline
bool ticket_try_lock(void * lock) {
    TicketLock *l = (TicketLock*)lock;
    unsigned long ingress = atomic_load_explicit(&l->ingress, memory_order_acquire);
    if(ingress == atomic_load_explicit(&l->egress, memory_order_acquire)){
       return atomic_compare_exchange_strong( &l->ingress,
                                           &ingress,
                                           ingress + 1);
    }
    return false;
}
void ticket_delegate(void * lock,
                    void (*funPtr)(unsigned int, void *), 
                    unsigned int messageSize,
                    void * messageAddress);
void * ticket_delegate_or_lock(void * lock, unsigned int messageSize);
TicketLock * plain_ticket_create();
OOLock * oo_ticket_create();

#endif
