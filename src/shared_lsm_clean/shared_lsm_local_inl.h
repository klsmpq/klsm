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

template <class K, class V, int Relaxation>
shared_lsm_local<K, V, Relaxation>::shared_lsm_local()
{
}

template <class K, class V, int Relaxation>
void
shared_lsm_local<K, V, Relaxation>::insert(const K &key,
                                           const V &val,
                                           versioned_array_ptr<K, V> &global_array)
{
    auto i = m_item_pool.acquire();
    i->initialize(key, val);

    /* TODO: Allocate larger blocks as an optimization. Find a more elegant
     * way of handling the special case of the initially allocated block. */
    auto b = m_block_pool.get_block(1);
    b->insert(i, i->version());

    insert(b, global_array);
}

template <class K, class V, int Relaxation>
void
shared_lsm_local<K, V, Relaxation>::insert(
        block<K, V> *b,
        versioned_array_ptr<K, V> &global_array)
{
    while (true) {
        /* Fetch a consistent copy of the global array. */

        block_array<K, V> *observed_packed;
        version_t observed_version;
        do {
            observed_packed = global_array.load_packed();
            auto observed_unpacked = global_array.unpack(observed_packed);
            observed_version = observed_unpacked->version();

            if (m_local_array_copy.version() == observed_version) {
                break;
            }

            m_local_array_copy.copy_from(observed_unpacked);
        } while (global_array.load()->version() == observed_version);

        /* Create a new version which we will attempt to push globally. */

        auto &new_blocks = (observed_version & 1)
                ? m_array_pool_evens
                : m_array_pool_odds;
        auto new_blocks_ptr = new_blocks.ptr();
        new_blocks_ptr->copy_from(&m_local_array_copy);
        new_blocks_ptr->increment_version();
        new_blocks_ptr->insert(b, &m_block_pool);

        /* Try to update the global array. */

        if (global_array.compare_exchange_strong(observed_packed,
                                                 new_blocks)) {
            m_block_pool.publish(new_blocks_ptr->m_blocks,
                                 new_blocks_ptr->version());
            m_block_pool.free_local();
            break;
        }

        m_block_pool.free_local_except(b);
    }
}
