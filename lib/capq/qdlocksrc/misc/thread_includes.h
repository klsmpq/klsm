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

#ifndef THREAD_INCLUDES_H
#define THREAD_INCLUDES_H

#include <time.h>
#include <stdlib.h>
#include <pthread.h>//Until c11 threads.h is available
#include <time.h>
#include <sched.h>

#if defined(__x86_64__) || defined(_M_X64)
#define CPUPAUSE asm("pause");
#else
#define CPUPAUSE 
#endif

static inline void thread_yield(){
    //sched_yield();
    //atomic_thread_fence(memory_order_seq_cst);
    CPUPAUSE;
}

#ifndef __clang__
#    define _Thread_local __thread
#endif

#endif
