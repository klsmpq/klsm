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

#ifndef __SHARED_LSM_H
#define __SHARED_LSM_H

#include <atomic>

#include "util/mm.h"
#include "util/thread_local_ptr.h"
#include "block_array.h"
#include "block_pool.h"
#include "shared_lsm_local.h"

namespace kpq {

template <class K, class V, int Relaxation>
class shared_lsm {
public:
    shared_lsm();
    virtual ~shared_lsm() { }

    void insert(const K &key);
    void insert(const K &key,
                const V &val);
    void insert(block<K, V> *b);

    bool delete_min(V &val);

private:
    void refresh_local_array_copy();

private:
    std::atomic<block_array<K, V> *> m_block_array;

    thread_local_ptr<shared_lsm_local<K, V, Relaxation>> m_local_component;

    /** The block array used to initialize the global pointer. */
    block_array<K, V> m_array_pool_initial;
};

#include "shared_lsm_inl.h"

}

#endif /* __SHARED_LSM_H */
