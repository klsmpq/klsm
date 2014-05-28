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

#ifndef __BLOCK_H
#define __BLOCK_H

#include <atomic>
#include <cassert>
#include <utility>

#include "item.h"

namespace kpq
{

/**
 * A block stores references to items together with their expected version.
 * An item is owned by this block if its version is equal to the expected version,
 * otherwise it has been processed by another thread and possibly reused.
 *
 * A block is always of capacity 2^i, i \in N_0. For all owned items, if the index i < j
 * then i.key < j.key.
 */

template <class K, class V>
class block
{
private:
    typedef std::pair<item<K, V> *, version_t> item_pair_t;

public:
    struct peek_t {
        peek_t() : m_item(nullptr), m_version(0) { }

        K m_key;
        item<K, V> *m_item;
        version_t m_version;
    };

    class spying_iterator
    {
        friend class block<K, V>;
    public:
        peek_t next();

    private:
        item_pair_t *m_item_pairs;
        size_t m_last, m_next;
    };

public:
    block(const size_t power_of_2);
    virtual ~block();

    void insert(item<K, V> *it,
                const version_t version);
    void insert_tail(item<K, V> *it,
                     const version_t version);
    void merge(const block<K, V> *lhs,
               const block<K, V> *rhs);
    void copy(const block<K, V> *that);

    /** Returns null if the block is empty, and a peek_t struct of the minimal item
     *  otherwise. Removes observed unowned items from the current block. */
    peek_t peek();

    /** Iterates the block from last to first and sets key to the first key it finds.
     *  If none are found, returns false. */
    bool peek_tail(K &key);

    spying_iterator iterator();

    size_t last() const;
    size_t size() const;
    size_t power_of_2() const;
    size_t capacity() const;

    bool used() const;
    void set_unused();
    void set_used();

public:
    /** Next pointers may be used by all threads. */
    std::atomic<block<K, V> *> m_next;
    /** Prev pointers may be used only by the owning thread. */
    block<K, V> *m_prev;

private:
    static bool item_owned(const item_pair_t &item_pair);

private:
    /** Points to the lowest known filled index. */
    size_t m_first;

    /** Points to the highest known filled index + 1.
     *  Since the CLSM is concurrent and other threads can take items without the owning
     *  thread knowing about it, size if not an exact value. Instead, it counts the number
     *  of elements that were written into the local list of items by the owning thread,
     *  even if those items currently aren't active anymore. */
    size_t m_last;

    /** The capacity stored as a power of 2. */
    const size_t m_power_of_2;
    const size_t m_capacity;

    item_pair_t *m_item_pairs;

    /** Specifies whether the block is currently in use. */
    bool m_used;
};

template <class K, class V>
typename block<K, V>::peek_t
block<K, V>::spying_iterator::next()
{
    peek_t p;

    while (m_next < m_last) {
        const auto &item = m_item_pairs[m_next++];

        p.m_version = item.second;
        p.m_item    = item.first;
        p.m_key     = item.first->key();

        if (item.second != p.m_version) {
            p.m_item = nullptr;
        } else {
            break;
        }
    }

    return p;
}

template <class K, class V>
block<K, V>::block(const size_t power_of_2) :
    m_next(nullptr),
    m_prev(nullptr),
    m_first(0),
    m_last(0),
    m_power_of_2(power_of_2),
    m_capacity(1 << power_of_2),
    m_item_pairs(new item_pair_t[m_capacity]),
    m_used(false)
{
}

template <class K, class V>
block<K, V>::~block()
{
    delete[] m_item_pairs;
}

template <class K, class V>
void
block<K, V>::insert(item<K, V> *it,
                    const version_t version)
{
    assert(m_first == 0);
    assert(m_last == 0);

    insert_tail(it, version);
}

template <class K, class V>
void
block<K, V>::insert_tail(item<K, V> *it,
                         const version_t version)
{
    assert(m_used);
    assert(m_last < m_capacity);

    m_item_pairs[m_last].first  = it;
    m_item_pairs[m_last].second = version;

    m_last++;
}

template <class K, class V>
void
block<K, V>::merge(const block<K, V> *lhs,
                   const block<K, V> *rhs)
{
    /* The following assertions are no longer valid since we now sometimes merge blocks
     * of different capacities.
    assert(m_power_of_2 == lhs->power_of_2() + 1);
    assert(lhs->power_of_2() == rhs->power_of_2());
     */
    assert(m_used);
    assert(m_first == 0);
    assert(m_last == 0);

    /* Merge. */

    size_t l = lhs->m_first, r = rhs->m_first, dst = 0;

    while (l < lhs->m_last && r < rhs->m_last) {
        auto &lelem = lhs->m_item_pairs[l];
        auto &relem = rhs->m_item_pairs[r];

        if (!item_owned(lelem)) {
            l++;
            continue;
        }

        if (!item_owned(relem)) {
            r++;
            continue;
        }

        if (lelem.first->key() < relem.first->key()) {
            m_item_pairs[dst++] = lelem;
            l++;
        } else {
            m_item_pairs[dst++] = relem;
            r++;
        }
    }

    while (l < lhs->m_last) {
        auto &lelem = lhs->m_item_pairs[l];
        if (!item_owned(lelem)) {
            l++;
            continue;
        }
        m_item_pairs[dst++] = lelem;
        l++;
    }

    while (r < rhs->m_last) {
        auto &relem = rhs->m_item_pairs[r];
        if (!item_owned(relem)) {
            r++;
            continue;
        }
        m_item_pairs[dst++] = relem;
        r++;
    }

    m_last = dst;
}

template <class K, class V>
void
block<K, V>::copy(const block<K, V> *that)
{
    assert(m_power_of_2 == that->power_of_2() - 1);
    assert(m_used);
    assert(m_first == 0);
    assert(m_last == 0);

    size_t dst = 0;
    for (size_t i = that->m_first; i < that->m_last; i++) {
        auto &elem = that->m_item_pairs[i];
        if (!item_owned(elem)) {
            continue;
        }

        m_item_pairs[dst++] = elem;
    }

    m_last = dst;
}

template <class K, class V>
typename block<K, V>::peek_t
block<K, V>::peek()
{
    peek_t p;
    for (size_t i = m_first; i < m_last; i++) {
        p.m_item    = m_item_pairs[i].first;
        p.m_key     = m_item_pairs[i].first->key();
        p.m_version = m_item_pairs[i].second;

        if (item_owned(m_item_pairs[i])) {
            return p;
        }

        /* Move initial sequence of unowned item references out
         * of active scope. */
        if (i == m_first) {
            m_first++;
        }
    }

    p.m_item = nullptr;
    return p;
}

template <class K, class V>
bool
block<K, V>::peek_tail(K &key)
{
    for (int i = (int)m_last - 1; i >= (int)m_first; i--) {
        key = m_item_pairs[i].first->key();
        if (item_owned(m_item_pairs[i])) {
            return true;
        }
        /* Last item is not owned by us anymore, clean it up. */
        m_last--;
    }
    return false;
}

template <class K, class V>
typename block<K, V>::spying_iterator
block<K, V>::iterator()
{
    typename block<K, V>::spying_iterator it;

    it.m_item_pairs = m_item_pairs;
    it.m_next = m_first;
    it.m_last = m_last;

    return it;
}

template <class K, class V>
size_t
block<K, V>::last() const
{
    return m_last;
}

template <class K, class V>
size_t
block<K, V>::size() const
{
    return m_last - m_first;
}

template <class K, class V>
size_t
block<K, V>::power_of_2() const
{
    return m_power_of_2;
}

template <class K, class V>
size_t
block<K, V>::capacity() const
{
    return m_capacity;
}

template <class K, class V>
bool
block<K, V>::used() const
{
    return m_used;
}

template <class K, class V>
void
block<K, V>::set_unused()
{
    assert(m_used);
    m_used  = false;
    m_first = 0;
    m_last  = 0;

    m_next.store(nullptr, std::memory_order_relaxed);
    m_prev = nullptr;
}

template <class K, class V>
void
block<K, V>::set_used()
{
    assert(!m_used);
    m_used = true;
}

template <class K, class V>
bool
block<K, V>::item_owned(const item_pair_t &item_pair)
{
    return (item_pair.first != nullptr && item_pair.first->version() == item_pair.second);
}

}

#endif /* __BLOCK_H */
