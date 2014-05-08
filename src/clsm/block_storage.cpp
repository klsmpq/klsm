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

#include "block_storage.h"

#include <cassert>

namespace kpq
{

template <class K, class V>
block_storage<K, V>::~block_storage()
{
    for (auto &block : m_blocks) {
        delete std::get<0>(block);
        delete std::get<1>(block);
        delete std::get<2>(block);
    }

    m_blocks.clear();
}

template <class K, class V>
block<K, V> *
block_storage<K, V>::get_block(const size_t i)
{
    if (i >= m_blocks.size()) {
        assert(m_blocks.size() == i);

        /* Alloc new blocks. */
        m_blocks.push_back(std::make_tuple(new block<K, V>(i),
                                           new block<K, V>(i),
                                           new block<K, V>(i)));
    }

    block<K, V> *block;
    if (!std::get<0>(m_blocks[i])->used()) {
        block = std::get<0>(m_blocks[i]);
    } else if (!std::get<1>(m_blocks[i])->used()) {
        block = std::get<1>(m_blocks[i]);
    } else {
        block = std::get<2>(m_blocks[i]);
    }

    block->set_used();
    return block;
}

template <class K, class V>
void
block_storage<K, V>::print() const
{
    for (size_t i = 0; i < m_blocks.size(); i++) {
        printf("%zu: {%d, %d, %d}, ", i,
               std::get<0>(m_blocks[i])->used(),
               std::get<1>(m_blocks[i])->used(),
               std::get<2>(m_blocks[i])->used());
    }
    printf("\n");
}

template class block_storage<uint32_t, uint32_t>;

}
