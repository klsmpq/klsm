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
    m_block_array(new block_array<K, V>())
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
    auto b = new block<K, V>(2);

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
        auto new_blocks = old_blocks->copy();
        new_blocks->insert(b);
        if (m_block_array.compare_exchange_strong(old_blocks,
                                             new_blocks)) {
            // TODO: Mark old_blocks as reusable.
            break;
        }
    }
}

template <class K, class V>
bool
shared_lsm<K, V>::delete_min(V &val)
{
    auto observed = m_block_array.load(std::memory_order_relaxed);
    return observed->delete_min(val);
}
