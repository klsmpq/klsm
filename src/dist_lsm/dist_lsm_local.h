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

#ifndef __DIST_LSM_LOCAL_H
#define __DIST_LSM_LOCAL_H

#include <atomic>
#include <random>

#include "components/block_storage.h"
#include "components/item.h"
#include "util/mm.h"
#include "util/thread_local_ptr.h"

namespace kpq
{

template <class K, class V>
class dist_lsm;

template <class K, class V>
class dist_lsm_local
{
public:
    dist_lsm_local();
    virtual ~dist_lsm_local();

    void insert(const K &key,
                const V &val);
    bool delete_min(dist_lsm<K, V> *parent,
                    V &val);

    /** Attempts to copy items from a random other thread's local clsm,
     *  and returns the number of items copied. */
    int spy(class dist_lsm<K, V> *parent);

    void print() const;

private:
    /** The internal insertion, used both in the public insert() and in spy(). */
    void insert(item<K, V> *it,
                const version_t version);

    /** Iterates through local items and returns the best one found.
     *  In the process of finding the minimal item, unowned items
     *  in each block are removed and block merges are performed if possible.
     *  Used internally by delete_min(). */
    void peek(typename block<K, V>::peek_t &best);

    /**
     * Inserts new_block into the linked list of blocks, merging with
     * same size blocks until no two blocks in the list have the same size.
     */
    void merge_insert(block<K, V> *const new_block);

private:
    std::atomic<block<K, V> *> m_head; /**< The largest  block. */
    block<K, V>               *m_tail; /**< The smallest block. */

    block_storage<K, V, 3> m_block_storage;
    item_allocator<item<K, V>, typename item<K, V>::reuse> m_item_allocator;

    std::mt19937 m_gen;
};

#include "dist_lsm_local_inl.h"

}

#endif /* __DIST_LSM_LOCAL_H */
