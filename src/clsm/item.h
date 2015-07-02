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

#ifndef __ITEM_H
#define __ITEM_H

#include <atomic>
#include <cassert>
#include <cstddef>

namespace kpq
{

typedef uint32_t version_t;

template <class K, class V>
class item
{
public:
    item();

    void initialize(const K &key,
                    const V &val);
    bool take(const version_t version,
              V &val);

    K key() const;
    V val() const;

    version_t version() const;
    bool used() const;

    class reuse
    {
    public:
        bool operator()(const item<K, V> &item) const
        {
            return !item.used();
        }
    };

private:
    /** Even versions are reusable, odd versions are in use. */
    std::atomic<version_t> m_version;
    K m_key;
    V m_val;
};

template <class K, class V>
item<K, V>::item() :
    m_version(0)
{
}

template <class K, class V>
void
item<K, V>::initialize(const K &key, const V &val)
{
    assert(!used());

    m_version.fetch_add(1, std::memory_order_relaxed); /* TODO: Really relaxed? */
    m_key = key;
    m_val = val;

    assert(used());
}

template <class K, class V>
bool
item<K, V>::take(const version_t version,
                 V &val)
{
    val = m_val;

    version_t expected = version;
    return m_version.compare_exchange_strong(expected,
                                             expected + 1,
                                             std::memory_order_relaxed);
}


template <class K, class V>
K
item<K, V>::key() const
{
    return m_key;
}

template <class K, class V>
V
item<K, V>::val() const
{
    return m_val;
}

template <class K, class V>
version_t
item<K, V>::version() const
{
    return m_version.load(std::memory_order_relaxed);
}

template <class K, class V>
bool
item<K, V>::used() const
{
    return ((version() & 0x1) == 1);
}

}

#endif /* __ITEM_H */
