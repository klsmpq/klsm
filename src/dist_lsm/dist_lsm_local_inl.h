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

template <class K, class V, int Rlx>
dist_lsm_local<K, V, Rlx>::dist_lsm_local() :
    m_head(nullptr),
    m_tail(nullptr),
    m_cached_best(block<K, V>::peek_t::EMPTY())
{
}

template <class K, class V, int Rlx>
dist_lsm_local<K, V, Rlx>::~dist_lsm_local()
{
    /* Blocks and items are managed by, respectively,
     * block_storage and item_allocator. */
}

template <class K, class V, int Rlx>
void
dist_lsm_local<K, V, Rlx>::insert(const K &key,
                                  const V &val,
                                  shared_lsm<K, V, Rlx> *slsm)
{
    item<K, V> *it = m_item_allocator.acquire();
    it->initialize(key, val);

    insert(it, it->version(), slsm);
}

template <class K, class V, int Rlx>
void
dist_lsm_local<K, V, Rlx>::insert(item<K, V> *it,
                                  const version_t version,
                                  shared_lsm<K, V, Rlx> *slsm)
{
    const K it_key = it->key();

    /* Update the cached best item if necessary. */

    if (m_cached_best.empty() || it_key < m_cached_best.m_key) {
        m_cached_best.m_key     = it_key;
        m_cached_best.m_item    = it;
        m_cached_best.m_version = version;
    } else if (m_cached_best.taken()) {
        m_cached_best.m_item    = nullptr;
    }

    /* If possible, simply append to the current tail block. */

    if (m_tail != nullptr && m_tail->last() < m_tail->capacity()) {
        K tail_key;
        if (m_tail->peek_tail(tail_key) && tail_key <= it_key) {
            m_tail->insert_tail(it, version);
            return;
        }
    }

    /* Allocate the biggest possible array. This is an optimization
     * only. For correctness, it is enough to always allocate a new
     * array of capacity 1. */

    block<K, V> *new_block;
    if (m_tail == nullptr) {
        new_block = m_block_storage.get_largest_block();
    } else {
        const size_t tail_size = m_tail->power_of_2();
        new_block = m_block_storage.get_block((tail_size == 0) ? 0 : tail_size - 1);
    }

    new_block->insert(it, version);
    merge_insert(new_block, slsm);
}

template <class K, class V, int Rlx>
void
dist_lsm_local<K, V, Rlx>::merge_insert(block<K, V> *const new_block,
                                        shared_lsm<K, V, Rlx> *slsm)
{
    block<K, V> *insert_block = new_block;
    block<K, V> *other_block  = m_tail;
    block<K, V> *delete_block = nullptr;

    /* Merge as long as the prev block is of the same size as the new block. */
    while (other_block != nullptr && insert_block->capacity() == other_block->capacity()) {
        /* Only merge into a larger block if both candidate blocks have enough elements to
         * justify the larger size. This change is necessary to avoid huge blocks containing
         * only a few elements (which actually happens with the 'alloc largest block on insert'
         * optimization. */
        const size_t merged_pow2 =
            (insert_block->size() + other_block->size() <= insert_block->capacity()) ?
            insert_block->power_of_2() : insert_block->power_of_2() + 1;
        auto merged_block = m_block_storage.get_block(merged_pow2);
        merged_block->merge(insert_block, other_block);

        insert_block->set_unused();
        insert_block = merged_block;
        delete_block = other_block;
        other_block  = other_block->m_prev;
    }

    if (slsm != nullptr && insert_block->size() >= (Rlx + 1) / 2) {
        /* The merged block exceeds relaxation bounds and we have a shared lsm
         * pointer, insert the new block into the shared lsm instead.
         * The shared lsm creates a copy of the passed block, and thus we can set
         * the passed block unused once insertion has completed.
         *
         * TODO: Optimize this by allocating the block from the shared lsm
         * if we are about to merge into a block exceeding the relaxation bound.
         */
        slsm->insert(insert_block);
        insert_block->set_unused();

        if (other_block != nullptr) {
            other_block->m_next.store(nullptr, std::memory_order_relaxed);
        } else {
            m_head.store(nullptr, std::memory_order_relaxed);
        }
        m_tail = other_block;
    } else {
        /* Insert the new block into the list. */
        insert_block->m_prev = other_block;
        if (other_block != nullptr) {
            other_block->m_next.store(insert_block, std::memory_order_relaxed);
        } else {
            m_head.store(insert_block, std::memory_order_relaxed);
        }
        m_tail = insert_block;
    }

    /* Remove merged blocks from the list. */
    while (delete_block != nullptr) {
        auto next_block = delete_block->m_next.load(std::memory_order_relaxed);
        delete_block->set_unused();
        delete_block = next_block;
    }
}

template <class K, class V, int Rlx>
bool
dist_lsm_local<K, V, Rlx>::delete_min(dist_lsm<K, V, Rlx> *parent,
                                      V &val)
{
    typename block<K, V>::peek_t best = block<K, V>::peek_t::EMPTY();
    peek(best);

    if (best.m_item == nullptr && spy(parent) > 0) {
        peek(best); /* Retry once after a successful spy(). */
    }

    if (best.m_item == nullptr) {
        return false; /* We did our best, give up. */
    }

    return best.m_item->take(best.m_version, val);
}

template <class K, class V, int Rlx>
void
dist_lsm_local<K, V, Rlx>::peek(typename block<K, V>::peek_t &best)
{
    /* Short-circuit. */
    if (!m_cached_best.empty() && !m_cached_best.taken()) {
        best = m_cached_best;
        return;
    }

    for (auto i = m_head.load(std::memory_order_relaxed);
            i != nullptr;
            i = i->m_next.load(std::memory_order_relaxed)) {

        auto candidate = i->peek();
        while (i->size() <= i->capacity() / 2) {

            /* Simply remove empty blocks. */
            if (i->size() == 0) {
                const auto next = i->m_next.load(std::memory_order_relaxed);
                if (i == m_tail) {
                    m_tail = i->m_prev;
                } else {
                    next->m_prev = i->m_prev;
                }

                if (i == m_head.load(std::memory_order_relaxed)) {
                    m_head = next;
                } else {
                    i->m_prev->m_next = next;
                }

                i->set_unused();

                return;
            }

            /* Shrink. */

            block<K, V> *new_block = m_block_storage.get_block(i->power_of_2() - 1);
            new_block->copy(i);

            new_block->m_next.store(i->m_next.load(std::memory_order_relaxed),
                                    std::memory_order_relaxed);
            new_block->m_prev = i->m_prev;

            /* Merge. TODO: Shrink-Merge optimization. */

            auto next = new_block->m_next.load(std::memory_order_relaxed);
            if (next != nullptr && new_block->capacity() == next->capacity()) {
                auto merged_block = m_block_storage.get_block(new_block->power_of_2() + 1);
                merged_block->merge(new_block, next);

                merged_block->m_next = next->m_next.load(std::memory_order_relaxed);
                merged_block->m_prev = new_block->m_prev;

                new_block->set_unused();
                new_block = merged_block;
            }

            /* Insert new block. */

            next = new_block->m_next.load(std::memory_order_relaxed);

            if (next == nullptr) {
                m_tail = new_block;
            } else {
                next->m_prev = new_block;
            }

            if (new_block->m_prev == nullptr) {
                m_head.store(new_block, std::memory_order_relaxed);
            } else {
                new_block->m_prev->m_next.store(new_block, std::memory_order_relaxed);
            }

            /* Bookkeeping and rerun peek(). */

            for (auto j = i; j != nullptr && j != next;) {
                const auto k = j->m_next.load(std::memory_order_relaxed);
                j->set_unused();
                j = k;
            }
            i = new_block;

            candidate = i->peek();
        }

        if (best.m_item == nullptr ||
                (candidate.m_item != nullptr && candidate.m_key < best.m_key)) {
            best = candidate;
        }
    }

    m_cached_best = best;
}

template <class K, class V, int Rlx>
int
dist_lsm_local<K, V, Rlx>::spy(dist_lsm<K, V, Rlx> *parent)
{
    int num_spied = 0;

    if (m_tail != nullptr) {
        return num_spied;
    }

    const size_t num_threads    = parent->m_local.num_threads();
    const size_t current_thread = parent->m_local.current_thread();

    if (num_threads < 2) {
        return num_spied;
    }

    /* All except current thread, therefore n - 2. */
    size_t victim_id = m_gen() % (num_threads - 1);
    if (victim_id >= current_thread) {
        victim_id++;
    }

    auto victim = parent->m_local.get(victim_id);
    auto spied_block = victim->m_head.load(std::memory_order_relaxed);

    if (spied_block == nullptr) {
        return num_spied;
    }

    /* Got a block, add it to the local lsm. */

    auto insert_block = m_block_storage.get_block(spied_block->power_of_2());
    insert_block->copy(spied_block);

    num_spied = insert_block->size();

    /* TODO: It is not necessarily legal to not pass in the shared lsm here,
     * since spy() may be called even if the local lsm is not empty. */
    merge_insert(insert_block, nullptr);

    return num_spied;
}

template <class K, class V, int Rlx>
void
dist_lsm_local<K, V, Rlx>::print() const
{
    m_block_storage.print();
}
