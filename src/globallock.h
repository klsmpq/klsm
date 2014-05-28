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

#ifndef __GLOBALLOCK_H
#define __GLOBALLOCK_H

#include <mutex>
#include <queue>

namespace kpq
{

template <class K, class V>
class GlobalLock
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
    void clear();

    void print() const;

private:
    typedef std::priority_queue<entry_t, std::vector<entry_t>, std::greater<entry_t>> pq_t;

    std::mutex m_mutex;
    pq_t m_q;
};

template <class K, class V>
bool
GlobalLock<K, V>::delete_min(V &value)
{
    std::lock_guard<std::mutex> g(m_mutex);

    if (m_q.empty()) {
        return false;
    }

    entry_t entry = m_q.top();
    m_q.pop();

    value = entry.value;

    return true;
}

template <class K, class V>
void
GlobalLock<K, V>::insert(const K &key,
                         const V &value)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_q.push(entry_t { key, value });
}

template <class K, class V>
void
GlobalLock<K, V>::clear()
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_q = pq_t();
}

template <class K, class V>
void GlobalLock<K, V>::print() const
{
    /* NOP */
}

}

#endif /* __GLOBALLOCK_H */
