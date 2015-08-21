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

template <class K, class V, int Rlx, int Algn>
aligned_block_array<K, V, Rlx, Algn>::aligned_block_array()
{
    void *buf_ptr = m_buffer;

    void *aligned_ptr = (void *)((((intptr_t) buf_ptr) + Algn - 1) & ~(Algn - 1));
    assert(aligned_ptr != nullptr);
    assert(((intptr_t)aligned_ptr & (Algn - 1)) == 0);

    m_ptr = new (aligned_ptr) block_array<K, V, Rlx>();
}

template <class K, class V, int Rlx, int Algn>
aligned_block_array<K, V, Rlx, Algn>::~aligned_block_array()
{
    m_ptr->~block_array();
}
