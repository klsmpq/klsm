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


template <class K, class V, int Relaxation, int Alignment>
aligned_block_array<K, V, Relaxation, Alignment>::aligned_block_array()
{
    void *buf_ptr = m_buffer;
    size_t buf_size = BUFFER_SIZE;

    void *aligned_ptr = std::align(Alignment, ARRAY_SIZE, buf_ptr, buf_size);
    assert(aligned_ptr != nullptr);

    m_ptr = new (aligned_ptr) block_array<K, V, Relaxation>();
}

template <class K, class V, int Relaxation, int Alignment>
aligned_block_array<K, V, Relaxation, Alignment>::~aligned_block_array()
{
    m_ptr->~block_array();
}
