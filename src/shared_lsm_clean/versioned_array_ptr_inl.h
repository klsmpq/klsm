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

template <class K, class V, int Alignment>
versioned_array_ptr<K, V, Alignment>::versioned_array_ptr()
{
    m_ptr = packed_ptr(m_initial_value.ptr());
}

template <class K, class V, int Alignment>
versioned_array_ptr<K, V, Alignment>::~versioned_array_ptr()
{
}

template <class K, class V, int Alignment>
bool
versioned_array_ptr<K, V, Alignment>::matches(
        block_array<K, V> *ptr,
        version_t version)
{
    return ((intptr_t)ptr & MASK) == (version & MASK);
}

template <class K, class V, int Alignment>
block_array<K, V> *
versioned_array_ptr<K, V, Alignment>::packed_ptr(block_array<K, V> *ptr)
{
    const intptr_t intptr = (intptr_t)ptr;
    assert((intptr & MASK) == 0);
    return (block_array<K, V> *)(intptr | (ptr->version() & MASK));
}

template <class K, class V, int Alignment>
block_array<K, V> *
versioned_array_ptr<K, V, Alignment>::unpacked_ptr(block_array<K, V> *ptr)
{
    const intptr_t intptr = (intptr_t)ptr;
    return (block_array<K, V> *)(intptr & ~MASK);
}

template <class K, class V, int Alignment>
block_array<K, V> *
versioned_array_ptr<K, V, Alignment>::load()
{
    return unpacked_ptr(m_ptr.load());
}

template <class K, class V, int Alignment>
block_array<K, V> *
versioned_array_ptr<K, V, Alignment>::load_packed()
{
    return m_ptr.load();
}

template <class K, class V, int Alignment>
bool
versioned_array_ptr<K, V, Alignment>::compare_exchange_strong(
        block_array<K, V> *&expected_packed,
        aligned_block_array<K, V, Alignment> &desired)
{
    return m_ptr.compare_exchange_strong(expected_packed,
                                         packed_ptr(desired.ptr()));
}
