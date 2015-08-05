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

#ifndef __ALIGNED_BLOCK_ARRAY_H
#define __ALIGNED_BLOCK_ARRAY_H

#include <memory>

#include "block_array.h"

namespace kpq {

constexpr static int DEFAULT_ALIGNMENT = 2048;

/**
 * Wraps allocation of a block array instance aligned to a specific amount.
 * This is needed since we partially pack an array's version into its pointer
 * in order to avoid the ABA problem when we compare and swap the global array
 * pointer.
 * Algn must be a power of two.
 */
template <class K, class V, int Rlx, int Algn = DEFAULT_ALIGNMENT>
class aligned_block_array {
public:
    aligned_block_array();
    virtual ~aligned_block_array();

    block_array<K, V, Rlx> *ptr() const { return m_ptr; }

private:
    constexpr static size_t ARRAY_SIZE   = sizeof(block_array<K, V, Rlx>);
    constexpr static size_t BUFFER_SIZE = Algn + ARRAY_SIZE;

    block_array<K, V, Rlx> *m_ptr;
    uint8_t m_buffer[BUFFER_SIZE];
};

#include "aligned_block_array_inl.h"

}

#endif /* __ALIGNED_BLOCK_ARRAY_H */
