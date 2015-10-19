/*
 *  Copyright 2014 Jakob Gruber
 *
 *  This file is part of kpqueue.
 *
 *  kpqueue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  kpqueue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with kpqueue.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __COUNTERS_H
#define __COUNTERS_H

#include <cstddef>
#include <cstdio>
#include <cstring>

#define V(D) \
    D(inserts) \
    D(successful_deletes) \
    D(failed_deletes) \
    D(slsm_inserts) /* Block inserts into shared lsm. */ \
    D(slsm_insert_retries) /* Block insert retries through concurrent modification. */ \
    D(slsm_deletes) \
    D(dlsm_deletes) \
    D(slsm_peek_cache_hit) /* Number of times the cached item is returned by the slsm. */ \
    D(slsm_peeks_performed) /* Number of times we got past the cached item. */ \
    D(slsm_peek_attempts) /* Number of actual block array peek() calls. */ \
    D(block_shrinks) \
    D(pivot_shrinks) \
    D(pivot_grows) \
    D(successful_peeks) \
    D(failed_peeks) \
    D(requested_spies) \
    D(aborted_spies)

namespace kpq
{

/**
 * Very simple performance counters.
 */

struct counters
{
    counters()
    {
        memset(this, 0, sizeof(*this));
    }

    counters &operator+=(const counters &that)
    {
#define D_OP_ADD(C) C += that.C;
        V(D_OP_ADD)
#undef D_OP_ADD

        return *this;
    }

    size_t operations() const {
        return inserts + successful_deletes + failed_deletes;
    }

    void print() const {
#define D_PRINT_FORMAT(C) #C ": %lu\n"
#define D_PRINT_ARGS(C) C,
        printf(V(D_PRINT_FORMAT) "%s",
               V(D_PRINT_ARGS) "");
#undef D_PRINT_ARGS
#undef D_PRINT_FORMAT
    }

#define D_DECL(C) size_t C;
    V(D_DECL)
#undef D_DECL

#ifdef ENABLE_QUALITY
    /** Two special-case members which are used to store thread-local sequences
     *  of insertions and deletions for the quality benchmark. */
    void *insertion_sequence;
    void *deletion_sequence;
#endif
};

thread_local counters COUNTERS;

#define ENABLE_COUNTERS 1

#ifndef ENABLE_COUNTERS
#define COUNT_INC(C)
#else
#define COUNT_INC(C) kpq::COUNTERS.C++
#endif

}

#endif /* __COUNTERS_H */
