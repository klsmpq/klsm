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

#ifndef __BLOCK_ARRAY_H
#define __BLOCK_ARRAY_H

#include <atomic>
#include <cstring>
#include <vector>

#include "components/block.h"
#include "util/counters.h"
#include "util/xorshf96.h"
#include "block_pivots.h"
#include "block_pool.h"

namespace kpq {

template <class K, class V, int Rlx>
class block_array {
    /* For access to blocks during publishing. */
    template <class X, class Y, int Z>
    friend class shared_lsm_local;
public:
    static constexpr size_t MAX_BLOCKS = 32;

    block_array();
    virtual ~block_array();

    /** May only be called when this block is not visible to other threads. */
    void insert(block<K, V> *block,
                block_pool<K, V> *pool);

    /** Callable from other threads. */
    bool delete_min(V &val);
    typename block<K, V>::peek_t peek();

    /** Copies the given block array into the current instance.
      * The copy is shallow, i.e. only block pointers are copied. */
    void copy_from(const block_array<K, V, Rlx> *that);

    version_t version() const { return m_version.load(std::memory_order_relaxed); }
    void increment_version() { m_version.fetch_add(1, std::memory_order_relaxed); }

private:
    /** May only be called when this block is not visible to other threads. */
    void compact(block_pool<K, V> *pool);
    void remove_null_blocks();

    /** Utility functions for mutating blocks together with pivots. */
    void block_insert(const size_t block_ix, block<K, V> *block);
    void block_set(const size_t block_ix, block<K, V> *block);

private:

    /** Stores block pointers from largest to smallest (to stay consistent with
     *  clsm_local). The usual invariants (block size strictly descending, only
     *  one block of each size in array) are preserved while the block array is
     *  visible to other threads.
     */
    block<K, V> *m_blocks[MAX_BLOCKS];
    size_t m_size;

    block_pivots<K, V, Rlx, MAX_BLOCKS> m_pivots;

    std::atomic<version_t> m_version;

    xorshf96 m_gen;
};

#include "block_array_inl.h"

}

#endif /* __BLOCK_ARRAY_H */
