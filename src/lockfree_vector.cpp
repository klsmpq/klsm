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

#include "lockfree_vector.h"

#include <cassert>

namespace kpq
{

template <class T>
lockfree_vector<T>::lockfree_vector()
{
    for (int i = 0; i < bucket_count; i++) {
        m_buckets[i] = nullptr;
    }
}

template <class T>
lockfree_vector<T>::~lockfree_vector()
{
    for (int i = 0; i < bucket_count; i++) {
        if (m_buckets[i] != nullptr) {
            delete m_buckets[i];
        }
    }
}

template <class T>
T *
lockfree_vector<T>::get(const int n)
{
    const int i = index_of(n);

    T *bucket = m_buckets[i].load(std::memory_order_relaxed);
    if (bucket == nullptr) {
        bucket = new T[1 << i];
        T *expected = nullptr;
        if (!m_buckets[i].compare_exchange_strong(expected, bucket)) {
            delete bucket;
            bucket = expected;
            assert(bucket != nullptr);
        }
    }

    return &bucket[n + 1 - (1 << i)];
}

template <class T>
int
lockfree_vector<T>::index_of(const int n)
{
    /* We could optimize this for 64/32 bit ints. */

    int i = n + 1;
    int log = 0;

    while (i > 1) {
        i >>= 1;
        log++;
    }

    return log;
}

template class lockfree_vector<uint32_t>;

}
