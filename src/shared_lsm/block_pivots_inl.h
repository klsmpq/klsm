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

#include <limits>

template <class K, class V, int Rlx, int MaxBlocks>
block_pivots<K, V, Rlx, MaxBlocks>::block_pivots() :
    m_pivots { 0 },
    m_first_in_block { 0 },
    m_maximal_pivot(std::numeric_limits<K>::min())
{
}

template <class K, class V, int Rlx, int MaxBlocks>
block_pivots<K, V, Rlx, MaxBlocks>::~block_pivots()
{
}

template <class K, class V, int Rlx, int MaxBlocks>
block_pivots<K, V, Rlx, MaxBlocks> &
block_pivots<K, V, Rlx, MaxBlocks>::operator=(const block_pivots<K, V, Rlx, MaxBlocks> &that)
{
    memcpy(m_pivots, that.m_pivots, sizeof(m_pivots));
    memcpy(m_first_in_block, that.m_first_in_block, sizeof(m_first_in_block));
    m_maximal_pivot = that.m_maximal_pivot;

    return *this;
}

template <class K, class V, int Rlx, int MaxBlocks>
void
block_pivots<K, V, Rlx, MaxBlocks>::reset(block<K, V> **blocks,
                                          const size_t size)
{
    /* Find the minimal element and initially set pivots s.t. it is the only
     * element in the pivot set. */

    typename block<K, V>::peek_t best;
    int best_block_ix = -1;
    for (size_t i = 0; i < size; i++) {
        auto b = blocks[i];
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
    m_maximal_pivot = best.m_key;

    improve(1, blocks, size);
}

template <class K, class V, int Rlx, int MaxBlocks>
void
block_pivots<K, V, Rlx, MaxBlocks>::improve(const int initial_range_size,
                                            block<K, V> **blocks,
                                            const size_t size)
{
    /* During iterative improvement of pivots, we may repeatedly go beyond legal
     * limits and must backtrack the previous solution. For that purpose, we
     * create a second pivot vector and pointers to the currently legal solution
     * and the in-progress solution. */
    int pivot_store[MaxBlocks];
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
    while (block_ix < size && elements_in_range < Rlx / 2) {
        auto b = blocks[block_ix];

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
        for (size_t j = block_ix + 1; j < size && Rlx + 1 >= elements_in_tentative_range; j++) {
            // Update the range to include all elements less or equal to the new element.
            // Note that we do not need to check previous blocks, since the newly added element
            // must be less or equal to their next element beyond their pivot limit (otherwise
            // the other block's next element would have been added to the pivot range previously).

            auto b = blocks[j];

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
            m_maximal_pivot = peeked_item.m_key;
            std::swap(pivots, tentative_pivots);
        } else {
            // New item invalidates invariants, revert and set limit.
            upper_bound = peeked_item.m_key;
            tentative_pivots[block_ix] = pivots[block_ix];
        }

        /* Copy the remaining solution over to the final pivot vector. */
        if (pivots != &m_pivots[0]) {
            for (size_t j = block_ix + 1; j < size; j++) {
                m_pivots[j] = pivots[j];
            }
        }
    }
}

template <class K, class V, int Rlx, int MaxBlocks>
size_t
block_pivots<K, V, Rlx, MaxBlocks>::count(const size_t size) const
{
    int count = 0;
    for (size_t i = 0; i < size; i++) {
        count += count_in(i);
    }
    return count;
}

template <class K, class V, int Rlx, int MaxBlocks>
size_t
block_pivots<K, V, Rlx, MaxBlocks>::count_in(const size_t block_ix) const
{
    assert(block_ix < MaxBlocks);
    return std::max(0, m_pivots[block_ix] - m_first_in_block[block_ix]);
}

template <class K, class V, int Rlx, int MaxBlocks>
size_t
block_pivots<K, V, Rlx, MaxBlocks>::nth_ix_in(const size_t relative_element_ix,
                                              const size_t block_ix) const
{
    assert(block_ix < MaxBlocks);
    return m_first_in_block[block_ix] + relative_element_ix;
}

template <class K, class V, int Rlx, int MaxBlocks>
void
block_pivots<K, V, Rlx, MaxBlocks>::take_first_in(const size_t block_ix)
{
    assert(block_ix < MaxBlocks);
    m_first_in_block[block_ix]++;
}
