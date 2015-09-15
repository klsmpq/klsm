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

#ifndef __BLOCK_STORAGE_H
#define __BLOCK_STORAGE_H

#include <cassert>

#include "block.h"

namespace kpq
{

/**
 * Maintains N-tuples of memory blocks of size 2^i.
 */

template <class K, class V, int N>
class block_storage
{
private:
    static constexpr size_t MAX_BLOCKS = 32;  // TODO: Global setting.

    struct block_tuple {
        block<K, V> *xs[N];
    };

public:
    block_storage() : m_blocks { { nullptr } }, m_size(0) { }
    virtual ~block_storage();

    /**
     * Returns an unused block of size 2^i. If such a block does not exist,
     * a new N-tuple of size 2^i is allocated.
     */
    block<K, V> *get_block(const size_t i);

    block<K, V> *get_largest_block();

    void print() const;

private:
    block_tuple m_blocks[MAX_BLOCKS];
    size_t m_size;
};

#include "block_storage_inl.h"

}

#endif /* __BLOCK_STORAGE_H */
