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

#include "clsm_local.h"

namespace kpq
{

template <class K, class V>
clsm_local<K, V>::clsm_local() :
    m_head(nullptr),
    m_tail(nullptr)
{
}

template <class K, class V>
clsm_local<K, V>::~clsm_local()
{
    /* Blocks and items are managed by, respectively,
     * block_storage and item_allocator. */
}

template <class K, class V>
void
clsm_local<K, V>::insert(const K &key,
                         const V &val)
{
    item<K, V> *it = m_item_allocator.acquire();
    it->initialize(key, val);

    /* TODO: Add to existing block optimization. */
    /* TODO: Allocate biggest possible array optimization. */

    block<K, V> *new_block = m_block_storage.get_block(0);
    new_block->insert(it);

    merge_insert(new_block);
}

template <class K, class V>
void
clsm_local<K, V>::merge_insert(block<K, V> *const new_block)
{
    block<K, V> *insert_block = new_block;
    block<K, V> *other_block  = m_tail;
    block<K, V> *delete_block = nullptr;

    /* Merge as long as the prev block is of the same size as the new block. */
    while (other_block != nullptr && insert_block->capacity() == other_block->capacity()) {
        auto merged_block = m_block_storage.get_block(insert_block->power_of_2() + 1);
        merged_block->merge(insert_block, other_block);

        insert_block->set_unused();
        insert_block = merged_block;
        delete_block = other_block;
        other_block  = other_block->m_prev;
    }

    /* Insert the new block into the list. */
    insert_block->m_prev = other_block;
    if (other_block != nullptr) {
        other_block->m_next.store(insert_block, std::memory_order_relaxed);
    } else {
        m_head.store(insert_block, std::memory_order_relaxed);
    }
    m_tail = insert_block;

    /* Remove merged blocks from the list. */
    while (delete_block != nullptr) {
        auto next_block = delete_block->m_next.load(std::memory_order_relaxed);
        delete_block->set_unused();
        delete_block = next_block;
    }
}

template <class K, class V>
bool
clsm_local<K, V>::delete_min(V &)
{
    return false;
}

template class clsm_local<uint32_t, uint32_t>;

}
