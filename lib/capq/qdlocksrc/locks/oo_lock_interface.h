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

#ifndef OO_LOCK_INTERFACE_H
#define OO_LOCK_INTERFACE_H

#include <stdbool.h>
#include <stdlib.h>
#include "misc/padded_types.h"

typedef struct {
    void (*free)(void*);
    void (*lock)(void*);
    void (*unlock)(void*);
    bool (*is_locked)(void*);
    bool (*try_lock)(void*);
    void (*rlock)(void*);
    void (*runlock)(void*);
    void (*delegate)(void*,
                     void (*funPtr)(unsigned int, void *),
                     unsigned int messageSize,
                     void * messageAddress);
    void (*delegate_wait)(void*,
                          void (*funPtr)(unsigned int, void *),
                          unsigned int messageSize,
                          void * messageAddress);
    void * (*delegate_or_lock)(void* lock,
                               unsigned int messageSize);
    void (*close_delegate_buffer)(void * buffer,
                                  void (*funPtr)(unsigned int, void *));
    void (*delegate_unlock)(void* lock);
    char pad[CACHE_LINE_SIZE -  (8 * sizeof(void*)) % CACHE_LINE_SIZE];
} OOLockMethodTable;

typedef struct {
    OOLockMethodTable * m;
    void * lock;
    char pad[CACHE_LINE_SIZE - (2 * sizeof(void*)) % CACHE_LINE_SIZE];
} OOLock;

static inline void oolock_free(OOLock * lock){
    lock->m->free(lock->lock);
    free(lock);
}

#endif
