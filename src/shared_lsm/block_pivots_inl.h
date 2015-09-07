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
    m_maximal_pivot(std::numeric_limits<K>::min()),
    m_count { 0 },
    m_count_for_size { INVALID_COUNT_FOR_SIZE }
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

    m_count = that.m_count;
    m_count_for_size = that.m_count_for_size;

    return *this;
}

template <class K, class V, int Rlx, int MaxBlocks>
size_t
block_pivots<K, V, Rlx, MaxBlocks>::shrink(block<K, V> **blocks,
                                           const size_t size)
{
    /* Find the minimal element and initially set pivots s.t. it is the only
     * element in the pivot set. */

    typename block<K, V>::peek_t best = block<K, V>::peek_t::EMPTY();
    int best_block_ix = -1;
    int best_item_ix = -1;
    for (size_t i = 0; i < size; i++) {
        auto b = blocks[i];
        size_t candidate_ix;
        auto candidate  = b->peek(candidate_ix);

        m_first_in_block[i] = m_pivots[i] = b->first();

        if ((best.empty() && !candidate.empty()) ||
                (!best.empty() && !candidate.empty() && candidate.m_key < best.m_key)) {
            best = candidate;
            best_block_ix = i;
            best_item_ix = candidate_ix;
        }
    }

    if (best_block_ix == -1) {
        /* All blocks are empty. */
        return 0;
    }

    m_pivots[best_block_ix] = best_item_ix + 1;

    m_count = 1;
    m_count_for_size = size;

    return resize(1,
                  best.m_key,
                  m_maximal_pivot,
                  blocks,
                  size);
}

template <class K, class V, int Rlx, int MaxBlocks>
size_t
block_pivots<K, V, Rlx, MaxBlocks>::grow(const int initial_range_size,
                                         block<K, V> **blocks,
                                         const size_t size)
{
    return resize(initial_range_size,
                  m_maximal_pivot,
                  std::numeric_limits<K>::max(),
                  blocks,
                  size);
}

template <class K, class V, int Rlx, int MaxBlocks>
size_t
block_pivots<K, V, Rlx, MaxBlocks>::resize(const int initial_range_size,
                                           const K initial_lower_bound,
                                           const K initial_upper_bound,
                                           block<K, V> **blocks,
                                           const size_t size)
{
    /* During iterative improvement of pivots, we may repeatedly go beyond legal
     * limits and must backtrack the previous solution. For that purpose, we
     * create a second pivot vector and pointers to the currently legal solution
     * and the in-progress solution. */
    int temp_array[MaxBlocks];
    int *pivots = m_pivots;
    int *tentative_pivots = temp_array;

    // We could skip owned() checks by keeping a bitmap around - once an item is taken,
    // it is taken forever.

    /* Initially, only the minimal element is within the pivot range. */
    int elements_in_range = initial_range_size;

    K lower_bound = initial_lower_bound;
    K upper_bound = initial_upper_bound;
    K mid;

    int elements_in_tentative_range;
    while (true) {
        if (upper_bound < lower_bound) {
            return elements_in_range;
        }

        mid = lower_bound + (upper_bound - lower_bound) / 2;

        // Used to obtain better bounds for next iteration.
        K maximal_key = std::numeric_limits<K>::min();

        elements_in_tentative_range = elements_in_range;
        for (size_t block_ix = 0; block_ix < size; block_ix++) {
            auto b = blocks[block_ix];
            const int last = b->last();

            for (tentative_pivots[block_ix] = pivots[block_ix];
                 tentative_pivots[block_ix] < last;
                 tentative_pivots[block_ix]++) {
                auto peeked_item = b->peek_nth(tentative_pivots[block_ix]);
                if (peeked_item->taken()) {
                    continue;
                } else if (peeked_item->m_key > mid) {
                    break;
                } else {
                    elements_in_tentative_range++;
                    maximal_key = std::max(maximal_key, peeked_item->m_key);
                }

                if (elements_in_tentative_range > Rlx + 1) {
                    goto outer;
                }
            }
        }

outer:
        if (elements_in_tentative_range > Rlx + 1) {
            if (upper_bound == mid) {
                return elements_in_range;
            }
            upper_bound = std::min(mid, maximal_key);
        } else if (elements_in_tentative_range < Rlx / 2) {
            if (lower_bound == mid) {
                break;  // Could not improve solution further.
            }
            if (elements_in_tentative_range > elements_in_range) {
                // Keep partial solution.
                elements_in_range = elements_in_tentative_range;
                std::swap(pivots, tentative_pivots);
            }
            lower_bound = std::max(mid, maximal_key);
        } else {
            /* Successfully found range. */
            break;
        }
    }

    /* Return the valid solution. */
    m_maximal_pivot = mid;
    if (m_pivots != tentative_pivots) {
        memcpy(m_pivots, tentative_pivots, sizeof(m_pivots));
    }

    m_count_for_size = INVALID_COUNT_FOR_SIZE;
    m_count = count(size);

    return elements_in_tentative_range;
}

template <class K, class V, int Rlx, int MaxBlocks>
size_t
block_pivots<K, V, Rlx, MaxBlocks>::count(const size_t size)
{
    if (m_count_for_size == size) {
        return m_count;
    }

    m_count = 0;
    for (m_count_for_size = 0; m_count_for_size < size; m_count_for_size++) {
        m_count += count_in(m_count_for_size);
    }
    return m_count;
}

template <class K, class V, int Rlx, int MaxBlocks>
size_t
block_pivots<K, V, Rlx, MaxBlocks>::count_in(const size_t block_ix) const
{
    assert(block_ix < MaxBlocks);
    assert(m_first_in_block[block_ix] <= m_pivots[block_ix]);
    return m_pivots[block_ix] - m_first_in_block[block_ix];
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
block_pivots<K, V, Rlx, MaxBlocks>::mark_first_taken_in(const size_t block_ix)
{
    assert(block_ix < MaxBlocks);
    m_first_in_block[block_ix]++;
    if (block_ix < m_count_for_size) {
        m_count--;
    }
}

template <class K, class V, int Rlx, int MaxBlocks>
int
block_pivots<K, V, Rlx, MaxBlocks>::pivot_of(block<K, V> *block) const
{
    const size_t first = block->first();
    const size_t upper_bound = std::min(first + Rlx + 1, block->last());
    for (size_t i = first; i < upper_bound; i++) {
        auto p = block->peek_nth(i);
        if (!p->taken() && p->m_key > m_maximal_pivot) {
            return i;
        }
    }
    return upper_bound;
}

template <class K, class V, int Rlx, int MaxBlocks>
void
block_pivots<K, V, Rlx, MaxBlocks>::insert(const size_t block_ix,
                                           const size_t size,
                                           const int first_in_block,
                                           const int pivot)
{
    memmove(&m_pivots[block_ix + 1],
            &m_pivots[block_ix],
            sizeof(m_pivots[0]) * (size - block_ix));
    memmove(&m_first_in_block[block_ix + 1],
            &m_first_in_block[block_ix],
            sizeof(m_first_in_block[0]) * (size - block_ix));
    set(block_ix, first_in_block, pivot);
}

template <class K, class V, int Rlx, int MaxBlocks>
void
block_pivots<K, V, Rlx, MaxBlocks>::set(const size_t block_ix,
                                        const int first_in_block,
                                        const int pivot)
{
    m_first_in_block[block_ix] = first_in_block;
    m_pivots[block_ix] = pivot;
    m_count_for_size = INVALID_COUNT_FOR_SIZE;
}

template <class K, class V, int Rlx, int MaxBlocks>
void
block_pivots<K, V, Rlx, MaxBlocks>::copy(const size_t src_ix,
                                         const size_t dst_ix)
{
    if (src_ix == dst_ix) {
        return;
    }

    m_first_in_block[dst_ix] = m_first_in_block[src_ix];
    m_pivots[dst_ix] = m_pivots[src_ix];
    m_count_for_size = INVALID_COUNT_FOR_SIZE;
}
