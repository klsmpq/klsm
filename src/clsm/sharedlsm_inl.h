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

template <class K, class V>
shared_lsm<K, V>::shared_lsm() :
    m_block_array(&m_array_pool_initial)
{
}

template <class K, class V>
void
shared_lsm<K, V>::insert(const K &key)
{
    insert(key, key);
}

template <class K, class V>
void
shared_lsm<K, V>::insert(const K &key,
                         const V &val)
{
    auto i = m_item_allocators.get()->acquire();
    i->initialize(key, val);

    /* TODO: Allocate larger blocks as an optimization. Find a more elegant
     * way of handling the special case of the initially allocated block. */
    auto b = m_block_pool.get()->get_block(1);
    b->insert(i, i->version());

    insert(b);
}

template <class K, class V>
void
shared_lsm<K, V>::insert(block<K, V> *b)
{
    auto pool = m_block_pool.get();
    while (true) {
        auto old_blocks = m_block_array.load(std::memory_order_relaxed);
        refresh_local_array_copy();

        auto new_blocks = (m_local_array_copy.get()->version() & 1)
                ? m_array_pool_evens.get()
                : m_array_pool_odds.get();
        new_blocks->copy_from(m_local_array_copy.get());
        new_blocks->increment_version();
        new_blocks->insert(b, pool);

        /* TODO: This is a problem now that we reuse blocks... */
        if (m_block_array.compare_exchange_strong(old_blocks,
                                                  new_blocks)) {
            pool->publish(new_blocks->m_blocks, new_blocks->version());
            pool->free_local();
            break;
        }

        pool->free_local_except(b);
    }
}

template <class K, class V>
bool
shared_lsm<K, V>::delete_min(V &val)
{
    version_t global_version;
    typename block<K, V>::peek_t best;
    do {
        refresh_local_array_copy();
        best = m_local_array_copy.get()->peek();
        global_version = m_block_array.load(std::memory_order_relaxed)->version();
    } while (global_version != m_local_array_copy.get()->version());

    if (best.m_item == nullptr) {
        return false;  /* We did our best, give up. */
    }

    return best.m_item->take(best.m_version, val);
}

template <class K, class V>
void
shared_lsm<K, V>::refresh_local_array_copy()
{
    auto observed = m_block_array.load(std::memory_order_relaxed);
    auto observed_version = observed->version();

    /* Our copy is up to date, nothing to do. */
    if (m_local_array_copy.get()->version() == observed_version) {
        return;
    }

    do {
        observed = m_block_array.load(std::memory_order_relaxed);
        observed_version = observed->version();

        m_local_array_copy.get()->copy_from(observed);
    } while (m_block_array.load(std::memory_order_relaxed)->version() != observed_version);
}
