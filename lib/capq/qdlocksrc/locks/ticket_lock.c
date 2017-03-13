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

#include "ticket_lock.h"

_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable Ticket_LOCK_METHOD_TABLE = 
{
     .free = &free,
     .lock = &ticket_lock,
     .unlock = &ticket_unlock,
     .is_locked = &ticket_is_locked,
     .try_lock = &ticket_try_lock,
     .rlock = &ticket_lock,
     .runlock = &ticket_unlock,
     .delegate = &ticket_delegate,
     .delegate_wait = &ticket_delegate,
     .delegate_or_lock = &ticket_delegate_or_lock,
     .close_delegate_buffer = NULL, /* Should never be called */
     .delegate_unlock = &ticket_unlock
};



void ticket_initialize(TicketLock * lock){
    //    printf("TICKET LOCK inti %p\n", lock);
    atomic_init( &lock->ingress, 0 );
    atomic_init( &lock->egress, 0 );
}

void ticket_delegate(void * lock,
                    void (*funPtr)(unsigned int, void *), 
                    unsigned int messageSize,
                    void * messageAddress){
    TicketLock *l = (TicketLock*)lock;
    ticket_lock(l);
    funPtr(messageSize, messageAddress);
    ticket_unlock(l);
}


void * ticket_delegate_or_lock(void * lock, unsigned int messageSize){
    (void)messageSize;
    TicketLock *l = (TicketLock*)lock;
    ticket_lock(l);
    return NULL;
}



TicketLock * plain_ticket_create(){
    TicketLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(TicketLock));
    ticket_initialize(l);
    return l;
}


OOLock * oo_ticket_create(){
    TicketLock * l = plain_ticket_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &Ticket_LOCK_METHOD_TABLE;
    return ool;
}
