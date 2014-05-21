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

#ifndef __LOCKFREE_VECTOR_H
#define __LOCKFREE_VECTOR_H

#include <atomic>
#include <cassert>

namespace kpq
{

/** A lock-free vector of dynamic size, capable of holding up to 2^bucket_count
 *  elements. Allocated memory is freed only on destruction. The amount of memory
 *  used is determined by the highest n passed to get(). */

template <class T>
class lockfree_vector
{
public:
    static constexpr int bucket_count = 32;

    lockfree_vector()
    {
        for (int i = 0; i < bucket_count; i++) {
            m_buckets[i] = nullptr;
        }
    }

    virtual ~lockfree_vector()
    {
        for (int i = 0; i < bucket_count; i++) {
            if (m_buckets[i] != nullptr) {
                delete[] m_buckets[i];
            }
        }
    }

    T *get(const int n)
    {
        const int i = index_of(n);

        T *bucket = m_buckets[i].load(std::memory_order_relaxed);
        if (bucket == nullptr) {
            bucket = new T[1 << i];
            T *expected = nullptr;
            if (!m_buckets[i].compare_exchange_strong(expected, bucket)) {
                delete[] bucket;
                bucket = expected;
                assert(bucket != nullptr);
            }
        }

        return &bucket[n + 1 - (1 << i)];
    }

private:
    static int index_of(const int n)
    {
        /* We could optimize this for 64/32 bit ints. */

        int i = n + 1;
        int log = 0;

        while (i > 1) {
            i >>= 1;
            log++;
        }

        assert(log < bucket_count);

        return log;
    }

private:
    std::atomic<T *> m_buckets[bucket_count];
};

}

#endif /* __LOCKFREE_VECTOR_H */
