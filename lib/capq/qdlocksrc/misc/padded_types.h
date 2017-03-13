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

#ifndef PADDED_TYPES_H
#define PADDED_TYPES_H

#include "misc/bsd_stdatomic.h"//Until c11 stdatoic.h is available

#define CACHE_LINE_SIZE 128

#define CACHE_LINE_SIZE_PAD(size) CACHE_LINE_SIZE - (size) % CACHE_LINE_SIZE

#ifndef _ISOC11_SOURCE
#    if _POSIX_C_SOURCE >= 200112
#        include <malloc.h>
#        define aligned_alloc memalign
#    endif
#endif


typedef union {
    volatile atomic_flag value;
    char padding[CACHE_LINE_SIZE];
} LLPaddedFlag;

typedef union {
    volatile atomic_bool value;
    char padding[CACHE_LINE_SIZE];
} LLPaddedBool;

typedef union {
    volatile atomic_int value;
    char padding[CACHE_LINE_SIZE];
} LLPaddedInt;

typedef union {
    volatile atomic_uint value;
    char padding[CACHE_LINE_SIZE];
} LLPaddedUInt;

typedef union {
    volatile atomic_long value;
    char padding[CACHE_LINE_SIZE];
} LLPaddedLong;

typedef union {
    volatile atomic_ulong value;
    char padding[CACHE_LINE_SIZE];
} LLPaddedULong;

typedef union {
    volatile atomic_intptr_t value;
    char padding[CACHE_LINE_SIZE];
} LLPaddedPointer;

typedef union {
    volatile double value;
    char padding[CACHE_LINE_SIZE];
} LLPaddedDouble;

#endif
