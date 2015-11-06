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

#ifndef __BLOCK_PIVOTS_H
#define __BLOCK_PIVOTS_H

namespace kpq {

// TODO: Better naming, pivots is very undescriptive to me. Item range? Index boundaries
// could then be called lower and upper bounds?
// TODO: Store first and #elems per block to avoid having to recalculate it all the time.
// Possibly maintain a count of all elems in the range.
template <class K, class V, int Rlx, int MaxBlocks>
class block_pivots {
public:
    block_pivots();
    virtual ~block_pivots();

    block_pivots &operator=(const block_pivots<K, V, Rlx, MaxBlocks> &that);

    size_t shrink(block<K, V> **blocks,
                  const size_t size);
    size_t grow(const int initial_range_size,
                block<K, V> **blocks,
                const size_t size);

    /** Counts the number of elements within the pivot range. */
    size_t count(const size_t size);
    size_t count_in(const size_t block_ix) const;

    size_t nth_ix_in(const size_t relative_element_ix,
                     const size_t block_ix) const;

    void mark_first_taken_in(const size_t block_ix);

    int pivot_of(block<K, V> *block) const;

    void insert(const size_t block_ix,
                const size_t size,
                const int first_in_block,
                const int pivot);
    void set(const size_t block_ix, const int first_in_block, const int pivot);
    void copy(const size_t src_ix, const size_t dst_ix);

private:
    size_t resize(const int initial_range_size,
                  const K initial_lower_bound,
                  const K initial_upper_bound,
                  block<K, V> **blocks,
                  const size_t size);

private:
    static constexpr size_t INVALID_COUNT_FOR_SIZE = -1;

    /**
     * For each block in the array, stores an index i such that for all indices j < i,
     * block[j] is guaranteed to be within the k smallest keys of the array. These indices
     * are called 'pivots and are required to relax the delete_min operation.
     * Pivots should be absolute indices (not dependent on block's m_first/m_last).
     */
    int m_upper[MaxBlocks];
    int m_lower[MaxBlocks];
    K m_maximal_pivot;

    /**
     * A cache used to reduce the number of required count recalculations. m_count_for_size
     * denotes which block count size was previously cached for. Setting it to MaxBlocks forces
     * a recalculation of size.
     */
    size_t m_count;
    size_t m_count_for_size;
};

#include "block_pivots_inl.h"

}

#endif /* __BLOCK_PIVOTS_H */
