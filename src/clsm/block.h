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
#include <utility>

#include "item.h"

namespace kpq
{

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

public:
    block(const size_t power_of_2);
    virtual ~block();

    void insert(item<K, V> *it,
                const version_t version);
    void merge(const block<K, V> *lhs,
               const block<K, V> *rhs);

    /** Returns null if the block is empty, and a peek_t struct of the minimal item
     *  otherwise. Removes observed unowned items from the current block. */
    peek_t peek();
    peek_t spy_at(const size_t i);

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
    /** Since the CLSM is concurrent and other threads can take items without the owning
     *  thread knowing about it, size if not an exact value. Instead, it counts the number
     *  of elements that were written into the local list of items by the owning thread,
     *  even if those items currently aren't active anymore. */
    size_t m_size;
    const size_t m_power_of_2;
    const size_t m_capacity;

    item_pair_t *m_item_pairs;

    bool m_used;
};

}

#endif /* __BLOCK_H */
