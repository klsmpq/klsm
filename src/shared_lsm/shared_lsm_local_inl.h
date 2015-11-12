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

template <class K, class V, int Rlx>
shared_lsm_local<K, V, Rlx>::shared_lsm_local() :
    m_cached_best(block<K, V>::peek_t::EMPTY())
{
}

template <class K, class V, int Rlx>
void
shared_lsm_local<K, V, Rlx>::insert(
        const K &key,
        const V &val,
        versioned_array_ptr<K, V, Rlx> &global_array)
{
    auto i = m_item_pool.acquire();
    i->initialize(key, val);

    /* TODO: Allocate larger blocks as an optimization. Find a more elegant
     * way of handling the special case of the initially allocated block. */
    auto b = m_block_pool.get_block(1);
    b->insert(i, i->version());

    insert_block(b, global_array);
}

template <class K, class V, int Rlx>
void
shared_lsm_local<K, V, Rlx>::insert(
        block<K, V> *b,
        versioned_array_ptr<K, V, Rlx> &global_array)
{
    assert(!m_block_pool.contains(b)), "Not called with a dist lsm block";

    auto c = m_block_pool.get_block(b->power_of_2());
    c->copy(b);

    insert_block(c, global_array);
}

template <class K, class V, int Rlx>
void
shared_lsm_local<K, V, Rlx>::insert_block(
        block<K, V> *b,
        versioned_array_ptr<K, V, Rlx> &global_array)
{
    assert(m_block_pool.contains(b)), "Given block not allocated by shared lsm";
    COUNT_INC(slsm_inserts);

    while (true) {
        /* Fetch a consistent copy of the global array. */

        block_array<K, V, Rlx> *observed_packed;
        version_t observed_version;
        refresh_local_array_copy(observed_packed, observed_version, global_array);

        /* Create a new version which we will attempt to push globally. */

        auto &new_blocks = (observed_version & 1)
                ? m_array_pool_evens
                : m_array_pool_odds;
        auto new_blocks_ptr = new_blocks.ptr();
        new_blocks_ptr->copy_from(&m_local_array_copy);
        new_blocks_ptr->increment_version();
        new_blocks_ptr->insert(b, &m_block_pool);

        /* Try to update the global array. */

        if (observed_version == global_array.version()
                && global_array.compare_exchange_strong(observed_packed,
                                                        new_blocks)) {
            m_block_pool.publish(new_blocks_ptr->m_blocks,
                                 new_blocks_ptr->m_size,
                                 new_blocks_ptr->version());
            m_block_pool.free_local();
            break;
        }

        COUNT_INC(slsm_insert_retries);
        m_block_pool.free_local_except(b);
    }
}

template <class K, class V, int Rlx>
bool
shared_lsm_local<K, V, Rlx>::delete_min(
        V &val,
        versioned_array_ptr<K, V, Rlx> &global_array)
{
    typename block<K, V>::peek_t best = block<K, V>::peek_t::EMPTY();
    peek(best, global_array);

    if (best.m_item == nullptr) {
        return false;  /* We did our best, give up. */
    }

    return best.m_item->take(best.m_version, val);
}

template <class K, class V, int Rlx>
void
shared_lsm_local<K, V, Rlx>::peek(typename block<K, V>::peek_t &best,
                                  versioned_array_ptr<K, V, Rlx> &global_array)
{
    if (local_array_copy_is_fresh(global_array)
            && !m_cached_best.empty()
            && !m_cached_best.taken()) {
        COUNT_INC(slsm_peek_cache_hit);
        best = m_cached_best;
        return;
    }

    block_array<K, V, Rlx> *observed_packed;
    version_t observed_version;

    COUNT_INC(slsm_peeks_performed);
    do {
        refresh_local_array_copy(observed_packed, observed_version, global_array);
        best = m_cached_best = m_local_array_copy.peek();
        COUNT_INC(slsm_peek_attempts);
    } while (global_array.version() != observed_version);
}

template <class K, class V, int Rlx>
bool
shared_lsm_local<K, V, Rlx>::local_array_copy_is_fresh(
        versioned_array_ptr<K, V, Rlx> &global_array) const
{
    return (m_local_array_copy.version() == global_array.version());
}

template <class K, class V, int Rlx>
void
shared_lsm_local<K, V, Rlx>::refresh_local_array_copy(
        block_array<K, V, Rlx> *&observed_packed,
        version_t &observed_version,
        versioned_array_ptr<K, V, Rlx> &global_array)
{
    observed_packed = global_array.load_packed();
    auto observed_unpacked = global_array.unpack(observed_packed);
    observed_version = observed_unpacked->version();

    if (local_array_copy_is_fresh(global_array)) {
        return;
    }

    while (true) {
        observed_packed = global_array.load_packed();
        observed_unpacked = global_array.unpack(observed_packed);
        observed_version = observed_unpacked->version();

        if (!versioned_array_ptr<K, V, Rlx>::matches(observed_packed,
                                                            observed_version)) {
            continue;
        }

        m_local_array_copy.copy_from(observed_unpacked);

        if (global_array.version() == observed_version
                && observed_version == m_local_array_copy.version()) {
            break;
        }
    }

#ifndef NDEBUG
    for (size_t i = 0; i < m_local_array_copy.m_size; i++) {
        auto b = m_local_array_copy.m_blocks[i];
        assert(b != nullptr);
    }
#endif
}
