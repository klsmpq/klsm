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

#ifndef __SHAREDLSM_H
#define __SHAREDLSM_H

#include <atomic>

#include "block_array.h"
#include "mm.h"
#include "sharedlsm_block_pool.h"
#include "thread_local_ptr.h"

namespace kpq {

template <class K, class V>
class shared_lsm {
public:
    shared_lsm();
    virtual ~shared_lsm() { }

    void insert(const K &key);
    void insert(const K &key,
                const V &val);
    void insert(block<K, V> *b);

    bool delete_min(V &val);

private:
    void refresh_local_array_copy();

private:
    std::atomic<block_array<K, V> *> m_block_array;

    /* ---- Item memory management. ---- */

    thread_local_ptr<item_allocator<item<K, V>, typename item<K, V>::reuse>> m_item_allocators;

    /* ---- Block memory management. ---- */

    thread_local_ptr<shared_lsm_block_pool<K, V>> m_block_pool;

    /* ---- Block array memory management. ---- */

    /** Contains a copy of the global block array, updated regularly. */
    thread_local_ptr<block_array<K, V>> m_local_array_copy;

    /** Local memory pools for use by block arrays. */
    thread_local_ptr<block_array<K, V>> m_array_pool_odds;
    thread_local_ptr<block_array<K, V>> m_array_pool_evens;

    /** The block array used to initialize the global pointer. */
    block_array<K, V> m_array_pool_initial;
};

#include "sharedlsm_inl.h"

}

#endif /* __SHAREDLSM_H */
