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

#ifndef __VERSIONED_ARRAY_PTR_H
#define __VERSIONED_ARRAY_PTR_H

#include <atomic>

#include "aligned_block_array.h"
#include "block_array.h"

namespace kpq {

template <class K, class V, int Relaxation, int Alignment = DEFAULT_ALIGNMENT>
class versioned_array_ptr {
public:
    versioned_array_ptr();
    virtual ~versioned_array_ptr();

    /* Interface subject to change. */
    block_array<K, V, Relaxation> *load();
    block_array<K, V, Relaxation> *load_packed();
    bool compare_exchange_strong(
            block_array<K, V, Relaxation> *&expected_packed,
            aligned_block_array<K, V, Relaxation, Alignment> &desired);

    block_array<K, V, Relaxation> *unpack(block_array<K, V, Relaxation> *ptr)
    {
        return unpacked_ptr(ptr);
    }

    /** Returns true, iff the packed version in ptr possibly matches the
     *  given version. */
    static bool matches(block_array<K, V, Relaxation> *ptr,
                        version_t version);

private:
    static block_array<K, V, Relaxation> *packed_ptr(block_array<K, V, Relaxation> *ptr);
    static block_array<K, V, Relaxation> *unpacked_ptr(block_array<K, V, Relaxation> *ptr);

private:
    constexpr static int MASK = Alignment - 1;

    std::atomic<block_array<K, V, Relaxation> *> m_ptr;

    /** The block array used to initialize the global pointer. */
    aligned_block_array<K, V, Relaxation, Alignment> m_initial_value;
};

#include "versioned_array_ptr_inl.h"

}

#endif /* __VERSIONED_ARRAY_PTR_H */
