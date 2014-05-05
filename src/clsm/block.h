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
    block(const size_t power_of_2);
    virtual ~block();

    void insert(item<K, V> *it);
    void merge(const block<K, V> *lhs,
               const block<K, V> *rhs);

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
    const size_t m_power_of_2;
    const size_t m_capacity;

    item_pair_t *m_item_pairs;

    bool m_used;
};

}

#endif /* __BLOCK_H */
