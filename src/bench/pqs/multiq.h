/*
 *  Copyright 2015 Jakob Gruber
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

#ifndef __MULTIQ_H
#define __MULTIQ_H

#include <atomic>
#include <queue>

#include "util/thread_local_ptr.h"
#include "util/xorshf96.h"

namespace kpqbench
{

/**
 * Reimplementation of a multiqueue as described in Rihani, Sanders, Dementiev:
 * "MultiQueues: Simpler, Faster, and Better Relaxed Concurrent Priority Queues".
 * C is a tuning parameter specifying the number of internal queues per thread.
 */
template <class K, class V, int C = 4>
class multiq
{
private:
    constexpr static K SENTINEL_KEY = std::numeric_limits<K>::max();
    constexpr static int CACHE_LINE_SIZE = 64;

    struct entry
    {
        entry(K k, V v) : key(k), value(v) { }

        K key;
        V value;

        bool operator>(const entry &that) const
        {
            return this->key > that.key;
        }
    };

    typedef std::priority_queue<entry, std::vector<entry>, std::greater<entry>> pq;

    struct local_queue
    {
        local_queue()
        {
            m_pq.push({ SENTINEL_KEY, V { } });
            m_top = SENTINEL_KEY;
        }

        pq m_pq;
        K m_top;

        char m_padding[CACHE_LINE_SIZE - sizeof(m_top) - sizeof(m_pq)];
    } __attribute__((aligned(64)));

    struct local_lock
    {
        local_lock() : m_is_locked(false) { }

        std::atomic<bool> m_is_locked;

        char m_padding[CACHE_LINE_SIZE - sizeof(m_is_locked)];
    } __attribute__((aligned(64)));

public:
    multiq(const size_t num_threads);
    virtual ~multiq();

    void insert(const K &key, const V &value);
    bool delete_min(V &value);
    void clear();

    void print() const;

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return true; }

private:
    size_t num_queues() const { return m_num_threads * C; }
    bool lock(const size_t ix);
    void unlock(const size_t ix);

private:
    const size_t m_num_threads;

    local_queue *m_queues;
    local_lock *m_locks;
};

#include "multiq_inl.h"

}

#endif /* __MULTIQ_H */
