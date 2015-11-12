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

template <class K, class V, int Rlx, int Algn>
versioned_array_ptr<K, V, Rlx, Algn>::versioned_array_ptr()
{
    m_ptr = packed_ptr(m_initial_value.ptr());
}

template <class K, class V, int Rlx, int Algn>
versioned_array_ptr<K, V, Rlx, Algn>::~versioned_array_ptr()
{
}

template <class K, class V, int Rlx, int Algn>
bool
versioned_array_ptr<K, V, Rlx, Algn>::matches(
        block_array<K, V, Rlx> *ptr,
        version_t version)
{
    return ((intptr_t)ptr & MASK) == (version & MASK);
}

template <class K, class V, int Rlx, int Algn>
block_array<K, V, Rlx> *
versioned_array_ptr<K, V, Rlx, Algn>::packed_ptr(
        block_array<K, V, Rlx> *ptr)
{
    const intptr_t intptr = (intptr_t)ptr;
    assert((intptr & MASK) == 0);
    return (block_array<K, V, Rlx> *)(intptr | (ptr->version() & MASK));
}

template <class K, class V, int Rlx, int Algn>
block_array<K, V, Rlx> *
versioned_array_ptr<K, V, Rlx, Algn>::unpacked_ptr(
        block_array<K, V, Rlx> *ptr)
{
    const intptr_t intptr = (intptr_t)ptr;
    return (block_array<K, V, Rlx> *)(intptr & ~MASK);
}

template <class K, class V, int Rlx, int Algn>
block_array<K, V, Rlx> *
versioned_array_ptr<K, V, Rlx, Algn>::load()
{
    return unpacked_ptr(load_packed());
}

template <class K, class V, int Rlx, int Algn>
version_t
versioned_array_ptr<K, V, Rlx, Algn>::version()
{
    return load()->version();
}

template <class K, class V, int Rlx, int Algn>
block_array<K, V, Rlx> *
versioned_array_ptr<K, V, Rlx, Algn>::load_packed()
{
    return m_ptr.load(std::memory_order_relaxed);
}

template <class K, class V, int Rlx, int Algn>
bool
versioned_array_ptr<K, V, Rlx, Algn>::compare_exchange_strong(
        block_array<K, V, Rlx> *&expected_packed,
        aligned_block_array<K, V, Rlx, Algn> &desired)
{
    return m_ptr.compare_exchange_strong(expected_packed,
                                         packed_ptr(desired.ptr()),
                                         std::memory_order_relaxed);
}
