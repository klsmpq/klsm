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

#include "util/mm.h"
#include "util/thread_local_ptr.h"
#include "block_array.h"
#include "block_pool.h"
#include "shared_lsm_local.h"
#include "versioned_array_ptr.h"

namespace kpq {

/**
 * The shared lsm is a relaxed priority queue which is based on maintaining
 * a single global array of blocks.
 *
 * TODO: Local ordering semantics using bloom filters.
 * TODO: Logical (instead of physical) shrinking of blocks.
 */

template <class K, class V, int Rlx>
class shared_lsm {
public:
    shared_lsm();
    virtual ~shared_lsm() { }

    void insert(const K &key);
    void insert(const K &key,
                const V &val);
    void insert(block<K, V> *b);

    bool delete_min(V &val);
    void find_min(typename block<K, V>::peek_t &best);

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return true; }

private:
    versioned_array_ptr<K, V, Rlx> m_global_array;
    thread_local_ptr<shared_lsm_local<K, V, Rlx>> m_local_component;
};

#include "shared_lsm_inl.h"

}

#endif /* __SHARED_LSM_H */
