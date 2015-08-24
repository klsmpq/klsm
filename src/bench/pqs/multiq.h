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
#include <random>

#include "util/thread_local_ptr.h"

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

    struct entry
    {
        K key;
        V value;

        bool operator>(const entry &that) const
        {
            return this->key > that.key;
        }
    };

    typedef std::priority_queue<entry, std::vector<entry>, std::greater<entry>> pq;

    struct multiq_local
    {
        multiq_local() :
            m_is_locked(false)
        {
            m_pq.push({ SENTINEL_KEY, V { } });
        }

        std::atomic<bool> m_is_locked;
        pq m_pq;
    };

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
    kpq::thread_local_ptr<std::default_random_engine> m_local_gen;

    const size_t m_num_threads;
    multiq_local *m_queues;
};

#include "multiq_inl.h"

}

#endif /* __MULTIQ_H */
