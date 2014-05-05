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

#ifndef __THREAD_LOCAL_PTR_H
#define __THREAD_LOCAL_PTR_H

#include <atomic>
#include <cstddef>

#include "lockfree_vector.h"

namespace kpq
{

void set_tid();
int32_t tid();
int32_t max_tid();

/**
 * A thread-local pointer to an element of type T, based on a dynamically growing
 * array and the current thread id.
 */

template <class T>
class thread_local_ptr
{
public:
    T *get()
    {
        set_tid();
        return get(tid());
    }

    T *get(const int32_t tid)
    {
        assert(tid < max_tid());
        return m_items.get(tid);
    }

    /** Returns the current thread id. */
    static size_t current_thread()
    {
        return tid();
    }

    /** Returns the current thread count. */
    static size_t num_threads()
    {
        return max_tid();
    }

private:
    lockfree_vector<T> m_items;
};

}

#endif /* __THREAD_LOCAL_PTR_H */
