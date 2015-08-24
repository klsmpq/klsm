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

#ifndef __CHEAP_H
#define __CHEAP_H

#include <queue>

#include "util/thread_local_ptr.h"

using kpq::thread_local_ptr;

namespace kpqbench
{

/**
 * 'cheap' stands for concurrent heap, and simply consists of a thread-local
 * std::priority_queue. Unlike the CLSM, there is no spy() operation, and threads
 * may only access elements they added themselves.
 */
template <class K, class V>
class cheap
{
private:
    class entry_t
    {
    public:
        K key;
        V value;

        bool operator>(const entry_t &that) const
        {
            return this->key > that.key;
        }
    };

public:
    void insert(const K &key, const V &value);
    bool delete_min(V &value);

    void print() const;

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return true; }

private:
    typedef std::priority_queue<entry_t, std::vector<entry_t>, std::greater<entry_t>> pq_t;

    thread_local_ptr<pq_t> m_q;
};

template <class K, class V>
bool
cheap<K, V>::delete_min(V &value)
{
    pq_t *pq = m_q.get();
    if (pq->empty()) {
        return false;
    }

    entry_t entry = pq->top();
    pq->pop();

    value = entry.value;

    return true;
}

template <class K, class V>
void
cheap<K, V>::insert(const K &key,
                    const V &value)
{
    pq_t *pq = m_q.get();
    pq->push(entry_t { key, value });
}

template <class K, class V>
void cheap<K, V>::print() const
{
    /* NOP */
}

}

#endif /* __CHEAP_H */
