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

template <class K, class V, int N>
block_storage<K, V, N>::~block_storage()
{
    for (auto &block : m_blocks) {
        for (int i = 0; i < N; i++) {
            delete block.xs[i];
        }
    }

    m_blocks.clear();
}

template <class K, class V, int N>
block<K, V> *
block_storage<K, V, N>::get_block(const size_t i)
{
    if (i >= m_blocks.size()) {
        assert(m_blocks.size() == i);

        /* Alloc new blocks. */
        block_tuple t;
        for (int j = 0; j < N; j++) {
            t.xs[j] = new block<K, V>(i);
        }
        m_blocks.push_back(t);
    }

    block<K, V> *block = m_blocks[i].xs[N - 1];
    for (int j = 0; j < N - 1; j++) {
        if (!m_blocks[i].xs[j]->used()) {
            block = m_blocks[i].xs[j];
            break;
        }
    }

    block->set_used();
    return block;
}

template <class K, class V, int N>
block<K, V> *
block_storage<K, V, N>::get_largest_block()
{
    const size_t size = m_blocks.size();
    return get_block((size == 0) ? 0 : size - 1);
}

template <class K, class V, int N>
void
block_storage<K, V, N>::print() const
{
    for (size_t i = 0; i < m_blocks.size(); i++) {
        printf("%zu: {%d", i, m_blocks[i].xs[0]->used());
        for (int j = 1; j < N; j++) {
            printf(", %d", m_blocks[i].xs[j]->used());
        }
        printf("}, ");
    }
    printf("\n");
}
