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
    // TODO: Memory management. In combined version, will not be necessary for
    // blocks / items (these are handled by the distributed lsm component),
    // but we still need to manage block arrays.
    // We still need to create new blocks during merges though.
    auto b = new block<K, V>(1);

    auto i = m_item_allocators.get()->acquire();
    i->initialize(key, val);

    b->set_used();
    b->insert(i, i->version());

    insert(b);
}

template <class K, class V>
void
shared_lsm<K, V>::insert(block<K, V> *b)
{
    while (true) {
        auto old_blocks = m_block_array.load(std::memory_order_relaxed);
        refresh_local_array_copy();

        auto new_blocks = (m_local_array_copy.get()->version() & 1)
                ? m_array_pool_evens.get()
                : m_array_pool_odds.get();
        new_blocks->copy_from(m_local_array_copy.get());
        new_blocks->increment_version();
        new_blocks->insert(b);

        if (m_block_array.compare_exchange_strong(old_blocks,
                                                  new_blocks)) {
            break;
        }
    }
}

template <class K, class V>
bool
shared_lsm<K, V>::delete_min(V &val)
{
    refresh_local_array_copy();
    return m_local_array_copy.get()->delete_min(val);
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
