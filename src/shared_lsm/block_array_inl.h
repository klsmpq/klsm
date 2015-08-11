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
block_array<K, V, Rlx>::insert(block<K, V> *new_block,
                               block_pool<K, V> *pool)
{
    if (m_size == 0) {
        m_blocks[0] = new_block;
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
                auto merged_block = pool->get_block(insert_block->power_of_2() + 1);
                merged_block->merge(insert_block, other_block);

                insert_block = merged_block;
                m_blocks[i - 1] = nullptr;
            }
        }
        memmove(&m_blocks[i + 1], &m_blocks[i], sizeof(block<K, V> *) * (m_size - i));
        m_blocks[i] = insert_block;
    }

    m_size++;
    compact(pool);
    reset_pivots();
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

        if (b->size() < b->capacity() / 2) {
           /* TODO: Improve shrinking. Ideally, we'd be able to shrink
            * to an arbitrary level and without physically copying blocks. */
           int shrunk_power_of_2 = b->power_of_2() - 1;

            auto shrunk = pool->get_block(shrunk_power_of_2);
            shrunk->copy(b);
            b = m_blocks[i] = shrunk;
        }
    }

    /* Merge blocks. */

    for (int i = m_size - 2; i >= 0; i--) {
        auto big_block = m_blocks[i];
        auto small_block = m_blocks[i + 1];

        size_t big_pow = big_block->power_of_2();
        size_t small_pow = small_block->power_of_2();

        if (big_pow > small_pow) {
            continue;
        }

        int merge_pow = std::max(big_pow, small_pow) + 1;

        auto merge_block = pool->get_block(merge_pow);
        merge_block->merge(big_block, small_block);

        m_blocks[i + 1] = nullptr;
        m_blocks[i] = merge_block;
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
        m_blocks[dst++] = b;

#ifndef NDEBUG
        assert(b->capacity() < prev_capacity);
        prev_capacity = b->capacity();
#endif
    }

    m_size = dst;
}

template <class K, class V, int Rlx>
void
block_array<K, V, Rlx>::reset_pivots()
{
    /* Find the minimal element and initially set pivots s.t. it is the only
     * element in the pivot set. */

    typename block<K, V>::peek_t best;
    int best_block_ix = -1;
    for (size_t i = 0; i < m_size; i++) {
        auto b = m_blocks[i];
        auto candidate  = b->peek();

        m_first_in_block[i] = m_pivots[i] = b->first();

        if ((best.empty() && !candidate.empty()) ||
                (!best.empty() && !candidate.empty() && candidate.m_key < best.m_key)) {
            best = candidate;
            best_block_ix = i;
        }
    }

    if (best_block_ix == -1) {
        /* All blocks are empty. */
        return;
    }

    m_pivots[best_block_ix] = best.m_index + 1;

    improve_pivots(1);
}

template <class K, class V, int Rlx>
void
block_array<K, V, Rlx>::improve_pivots(const int initial_range_size)
{
    /* During iterative improvement of pivots, we may repeatedly go beyond legal
     * limits and must backtrack the previous solution. For that purpose, we
     * create a second pivot vector and pointers to the currently legal solution
     * and the in-progress solution. */
    int pivot_store[MAX_BLOCKS];
    int *pivots = m_pivots;
    int *tentative_pivots = pivot_store;

    memcpy(pivot_store, m_pivots, sizeof(m_pivots));

    /* Initially, only the minimal element is within the pivot range. */
    int elements_in_range = initial_range_size;

    /* Once we've found an element that violates relaxation constraints (i.e. which
     * would cause more than Rlx elements to be within the pivot range), we use an
     * upper bound to limit the keys which may be added to the pivot range. */
    K upper_bound = std::numeric_limits<K>::max();

    size_t block_ix = 0;
    while (block_ix < m_size && elements_in_range < Rlx / 2) {
        auto b = m_blocks[block_ix];

        // Entire current block in pivot range?
        if (pivots[block_ix] >= (int)b->last()) {
            tentative_pivots[block_ix] = pivots[block_ix];
            block_ix++;
            continue;  // With the next block.
        }

        // Check the next potential item in range. If it has been taken, the index
        // range can be enlarged since the taken item may be ignored.
        auto peeked_item = b->peek_nth(pivots[block_ix]);
        if (peeked_item.empty()) {
            pivots[block_ix]++;
            continue;
        }

        // If the current item violates constraints, set the upper bound and move
        // on to the next block.
        if (Rlx + 1 < elements_in_range) {
            if (peeked_item.m_key < upper_bound) {
                upper_bound = peeked_item.m_key;
            }
            tentative_pivots[block_ix] = pivots[block_ix];
            block_ix++;
            continue;
        }

        // The current item is above the upper bound, move on to the next block.
        if (upper_bound <= peeked_item.m_key) {
            tentative_pivots[block_ix] = pivots[block_ix];
            block_ix++;
            continue;
        }

        /* We've now passed initial checks. The next step is to tentatively add
         * the current item to the pivot range, and check if our invariant (all pivots
         * guaranteed to be within the Rlx minimal items) still holds. */

        tentative_pivots[block_ix] = pivots[block_ix] + 1;

        int elements_in_tentative_range = elements_in_range + 1;
        for (size_t j = block_ix + 1; j < m_size && Rlx + 1 >= elements_in_tentative_range; j++) {
            // Update the range to include all elements less or equal to the new element.
            // Note that we do not need to check previous blocks, since the newly added element
            // must be less or equal to their next element beyond their pivot limit (otherwise
            // the other block's next element would have been added to the pivot range previously).

            auto b = m_blocks[j];

            for (int i = pivots[j]; i < pivots[j] + Rlx + 1; i++) {
                if (i >= (int)b->last()) {
                    break;
                }

                auto p = b->peek_nth(i);
                if (!p.empty() && p.m_key > peeked_item.m_key) {
                    /* Got an item, and it's beyond our range. */
                    break;
                } else if (!p.empty()) {
                    /* Got an item within the range. */
                    elements_in_tentative_range++;
                    tentative_pivots[j] = i + 1;
                } else {
                    /* Item was already taken, ignore. */
                    tentative_pivots[j] = i + 1;
                }
            }
        }

        if (Rlx + 1 >= elements_in_tentative_range) {
            // Adding this iteration's element to the pivot range is legal, commit.
            elements_in_range = elements_in_tentative_range;
            std::swap(pivots, tentative_pivots);
        } else {
            // New item invalidates invariants, revert and set limit.
            upper_bound = peeked_item.m_key;
            tentative_pivots[block_ix] = pivots[block_ix];
        }

        /* Copy the remaining solution over to the final pivot vector. */
        if (pivots != &m_pivots[0]) {
            for (size_t j = block_ix + 1; j < m_size; j++) {
                m_pivots[j] = pivots[j];
            }
        }
    }
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
size_t
block_array<K, V, Rlx>::pivot_element_count()
{
    int count = 0;
    for (size_t i = 0; i < m_size; i++) {
        count += std::max(0, m_pivots[i] - m_first_in_block[i]);
    }
    return count;
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

    while (true) {
        int ncandidates = pivot_element_count();

        /* If the range contains too few items, attempt to improve it. */

        if (ncandidates < Rlx / 2) {
            improve_pivots(ncandidates);
            ncandidates = pivot_element_count();
        }

        /* Select a random element within the range, find it, and return it. */

        typename block<K,  V>::peek_t best;
        if (ncandidates == 0) {
            return best;
        }

        std::uniform_int_distribution<int> dist(0, ncandidates - 1);
        int selected_element = dist(m_gen);

        size_t block_ix;
        block<K, V> *b = nullptr;
        for (block_ix = 0; block_ix < m_size; block_ix++) {
            const int elements_in_range =
                    std::max(0, m_pivots[block_ix] - m_first_in_block[block_ix]);

            if (selected_element >= elements_in_range) {
                /* Element not in this block. */
                selected_element -= elements_in_range;
                continue;
            }

            b = m_blocks[block_ix];
            best = b->peek_nth(m_first_in_block[block_ix] + selected_element);

            // Selected first item in block, we can increment our range boundary.
            if (selected_element == 0) {
                m_first_in_block[block_ix]++;
            }

            break;
        }

        /* The selected item has already been taken, fall back to removing
         * the minimal item within the same block (possibly the same item). */

        if (best.empty() && b != nullptr && block_ix < m_size) {
            while (m_first_in_block[block_ix] < m_pivots[block_ix]) {
                best = b->peek_nth(m_first_in_block[block_ix]);
                m_first_in_block[block_ix]++;
                if (!best.empty()) {
                    return best;
                }
            }
        }

        if (!best.empty()) {
            return best;
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

        size_t i;
        for (i = 0; i < that->m_size; i++) {
            m_blocks[i] = that->m_blocks[i];
        }
        m_size = i;

        memcpy(m_pivots, that->m_pivots, sizeof(m_pivots));
        memcpy(m_first_in_block, that->m_first_in_block, sizeof(m_first_in_block));
    } while (that->m_version.load(std::memory_order_release) != m_version);
}
