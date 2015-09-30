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

#include <algorithm>
#include <limits>

template <class K, class V, int Rlx>
block_array<K, V, Rlx>::block_array() :
    m_size(0),
    m_version(0),
#ifndef NDEBUG
    m_gen(0)
#else
    m_gen()
#endif
{
}

template <class K, class V, int Rlx>
block_array<K, V, Rlx>::~block_array()
{
}

template <class K, class V, int Rlx>
void
block_array<K, V, Rlx>::block_insert(const size_t block_ix,
                                     block<K, V> *block)
{
    memmove(&m_blocks[block_ix + 1],
            &m_blocks[block_ix],
            sizeof(m_blocks[0]) * (m_size - block_ix));
    m_blocks[block_ix] = block;
    m_pivots.insert(block_ix, m_size, block->first(), m_pivots.pivot_of(block));
}

template <class K, class V, int Rlx>
void
block_array<K, V, Rlx>::block_set(const size_t block_ix,
                                  block<K, V> *block)
{
    // TODO: More efficient pivot recalculation.
    m_blocks[block_ix] = block;
    m_pivots.set(block_ix, block->first(), m_pivots.pivot_of(block));
}

template <class K, class V, int Rlx>
void
block_array<K, V, Rlx>::insert(block<K, V> *new_block,
                               block_pool<K, V> *pool)
{
    if (m_size == 0) {
        block_set(0, new_block);
    } else {
        size_t i;
        for (i = 0; i < m_size; i++) {
            auto b = m_blocks[i];
            if (b->capacity() < new_block->capacity()) {
                break;
            }
        }

        assert(i < MAX_BLOCKS);

        /* Merge with equal capacity blocks until strictly descending invariant is preserved.*/

        auto insert_block = new_block;
        for (; i > 0; i--) {
            auto other_block = m_blocks[i - 1];
            if (other_block == nullptr) {
                continue;
            } else if (other_block->capacity() > insert_block->capacity()) {
                break;
            } else {
                assert(other_block->capacity() == insert_block->capacity());
                const size_t other_first = m_pivots.nth_ix_in(0, i - 1);

                auto merged_block = pool->get_block(insert_block->power_of_2() + 1);
                merged_block->merge(insert_block, insert_block->first(),
                                    other_block, other_first);

                insert_block = merged_block;
                m_blocks[i - 1] = nullptr;
            }
        }
        block_insert(i, insert_block);
    }

    m_size++;
    compact(pool);

    /* If the number of elements within the pivot range is smaller than our lower bound,
     * attempt to improve pivots. */

    const size_t ncandidates = m_pivots.count(m_size);
    if (ncandidates > Rlx + 1) {
        // TODO: Possibly a more efficient reset mechanism which uses knowledge of existing
        // pivots.
        m_pivots.shrink(m_blocks, m_size);
    } else if (ncandidates < Rlx / 2) {
        m_pivots.grow(ncandidates, m_blocks, m_size);
    }
}

template <class K, class V, int Rlx>
void
block_array<K, V, Rlx>::compact(block_pool<K, V> *pool)
{
    remove_null_blocks();

    /* Shrink blocks. */

    for (size_t i = 0; i < m_size; i++) {
        auto b = m_blocks[i];

        if (b == nullptr) {
            continue;
        }

        const size_t size = b->last() - m_pivots.nth_ix_in(0, i);
        if (size < b->capacity() / 2) {
           /* TODO: Improve shrinking. Ideally, we'd be able to shrink
            * to an arbitrary level and without physically copying blocks. */
           int shrunk_power_of_2 = b->power_of_2() - 1;

            auto shrunk = pool->get_block(shrunk_power_of_2);
            shrunk->copy(b);
            block_set(i, shrunk);

            COUNT_INC(block_shrinks);
        }
    }

    /* Merge blocks. */

    for (int i = m_size - 2; i >= 0; i--) {
        auto big_block = m_blocks[i];
        auto small_block = m_blocks[i + 1];

        const size_t big_first = m_pivots.nth_ix_in(0, i);
        const size_t small_first = m_pivots.nth_ix_in(0, i + 1);

        const size_t big_pow = big_block->power_of_2();
        const size_t small_pow = small_block->power_of_2();

        if (big_pow > small_pow) {
            continue;
        }

        int merge_pow = std::max(big_pow, small_pow) + 1;

        auto merge_block = pool->get_block(merge_pow);
        merge_block->merge(big_block, big_first, small_block, small_first);

        m_blocks[i + 1] = nullptr;
        block_set(i, merge_block);
    }

    remove_null_blocks();
}

template <class K, class V, int Rlx>
void
block_array<K, V, Rlx>::remove_null_blocks() {
#ifndef NDEBUG
    size_t prev_capacity = std::numeric_limits<size_t>::max();
#endif
    size_t dst = 0;
    for (size_t src = 0; src < m_size; src++) {
        auto b = m_blocks[src];
        if (b == nullptr) {
            continue;
        }
        m_blocks[dst] = b;
        m_pivots.copy(src, dst);
        dst++;

#ifndef NDEBUG
        assert(b->capacity() < prev_capacity);
        prev_capacity = b->capacity();
#endif
    }

    m_size = dst;
}

template <class K, class V, int Rlx>
bool
block_array<K, V, Rlx>::delete_min(V &val)
{
    typename block<K, V>::peek_t best = peek();

    if (best.m_item == nullptr) {
        return false; /* We did our best, give up. */
    }

    return best.m_item->take(best.m_version, val);
}

template <class K, class V, int Rlx>
typename block<K, V>::peek_t
block_array<K, V, Rlx>::peek()
{
    /* Random selection of any item within the range given by the pivots.
     * First, calculate the number of items within the range. We need to store
     * the encountered values of first here in order to ensure we use the same
     * values when accessing the selected item below (even though the real value
     * might have changed in the meantime).
     */

    typename block<K, V>::peek_t ret;
    while (true) {
        int ncandidates = m_pivots.count(m_size);

        /* If the range contains too few items, attempt to improve it. */

        if (ncandidates < Rlx / 2) {
            ncandidates = m_pivots.grow(ncandidates, m_blocks, m_size);
        }

        /* Select a random element within the range, find it, and return it. */

        if (ncandidates == 0) {
            return block<K, V>::peek_t::EMPTY();
        }

        int selected_element = m_gen() % ncandidates;

        size_t block_ix;
        block<K, V> *b = nullptr;
        const typename block<K,  V>::block_item *best = nullptr;
        for (block_ix = 0; block_ix < m_size; block_ix++) {
            const int elements_in_range = m_pivots.count_in(block_ix);

            if (selected_element >= elements_in_range) {
                /* Element not in this block. */
                selected_element -= elements_in_range;
                continue;
            }

            b = m_blocks[block_ix];
            best = b->peek_nth(m_pivots.nth_ix_in(selected_element, block_ix));

            // TODO: If the current block is less than half-filled, trigger a shrink.

            break;
        }

        if (best == nullptr) {
            COUNT_INC(failed_peeks);
            continue;
        } else if (!best->taken()) {
            /* Found a valid element, return it. */
            COUNT_INC(successful_peeks);
            ret = *best;
            return ret;
        } else if (block_ix < m_size) {
            /* The selected item has already been taken, fall back to removing
             * the minimal item within the same block (possibly the same item). */

            COUNT_INC(failed_peeks);

            const size_t count_in_block = m_pivots.count_in(block_ix);
            assert(count_in_block > 0);

            const size_t first_in_block = m_pivots.nth_ix_in(0, block_ix);
            best = b->peek_nth(first_in_block);

            for (size_t i = 0; i < count_in_block; i++, best++) {
                if (!best->taken()) {
                    /* Simply taking the first item here would bias peek()
                     * towards the first item in the largest block. Instead,
                     * retry with a random selection.
                     * TODO: A possibly optimization is to retry on level up
                     * the callstack, since we might be doing unnecessary
                     * work if our local array copy is out of date.
                     */
                    break;
                } else {
                    m_pivots.mark_first_taken_in(block_ix);
                }
            }
        }
    }
}

template <class K, class V, int Rlx>
void
block_array<K, V, Rlx>::copy_from(const block_array<K, V, Rlx> *that)
{
    do {
        m_version = that->m_version.load(std::memory_order_acquire);
        m_size = 0;

        m_size = that->m_size;
        memcpy(m_blocks, that->m_blocks, sizeof(m_blocks[0]) * m_size);

        m_pivots = that->m_pivots;
    } while (that->m_version.load() != m_version);
}
