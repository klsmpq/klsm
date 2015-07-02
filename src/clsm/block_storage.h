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
#include <vector>

#include "block.h"

namespace kpq
{

/**
 * Maintains 3-tuples of memory blocks of size 2^i.
 */

template <class K, class V>
class block_storage
{
private:
    struct block_tuple {
        block<K, V> *fst, *snd, *thd;
    };

public:
    virtual ~block_storage();

    /**
     * Returns an unused block of size 2^i. If such a block does not exist,
     * a new 3-tuple of size 2^i is allocated.
     */
    block<K, V> *get_block(const size_t i);

    block<K, V> *get_largest_block();

    void print() const;

private:
    std::vector<block_tuple> m_blocks;
};

template <class K, class V>
block_storage<K, V>::~block_storage()
{
    for (auto &block : m_blocks) {
        delete block.fst;
        delete block.snd;
        delete block.thd;
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
        m_blocks.push_back({ new block<K, V>(i)
                           , new block<K, V>(i)
                           , new block<K, V>(i)
                           });
    }

    block<K, V> *block;
    if (!m_blocks[i].fst->used()) {
        block = m_blocks[i].fst;
    } else if (!m_blocks[i].snd->used()) {
        block = m_blocks[i].snd;
    } else {
        block = m_blocks[i].thd;
    }

    block->set_used();
    return block;
}

template <class K, class V>
block<K, V> *
block_storage<K, V>::get_largest_block()
{
    const size_t size = m_blocks.size();
    return get_block((size == 0) ? 0 : size - 1);
}

template <class K, class V>
void
block_storage<K, V>::print() const
{
    for (size_t i = 0; i < m_blocks.size(); i++) {
        printf("%zu: {%d, %d, %d}, ", i,
               m_blocks[i].fst->used(),
               m_blocks[i].snd->used(),
               m_blocks[i].thd->used());
    }
    printf("\n");
}

}

#endif /* __BLOCK_STORAGE_H */
