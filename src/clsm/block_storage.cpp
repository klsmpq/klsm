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

template <class T>
block_storage<T>::~block_storage()
{
    for (auto &block : m_blocks) {
        delete std::get<0>(block);
        delete std::get<1>(block);
        delete std::get<2>(block);
    }

    m_blocks.clear();
}

template <class T>
block<T> *
block_storage<T>::get_block(const size_t i)
{
    const size_t n = (1 << i);

    if (i >= m_blocks.size()) {
        assert(m_blocks.size() == i);

        /* Alloc new blocks. */
        m_blocks.push_back(std::make_tuple(new block<T>(n),
                                           new block<T>(n),
                                           new block<T>(n)));
    }

    if (!std::get<0>(m_blocks[i])->used()) {
        return std::get<0>(m_blocks[i]);
    } else if (!std::get<1>(m_blocks[i])->used()) {
        return std::get<1>(m_blocks[i]);
    } else {
        assert(!std::get<2>(m_blocks[i])->used());
        return std::get<2>(m_blocks[i]);
    }
}

template class block_storage<uint32_t>;

}
