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

#ifndef __SHARED_LSM_LOCAL_H
#define __SHARED_LSM_LOCAL_H

#include <atomic>

#include "util/mm.h"
#include "block_array.h"
#include "block_pool.h"
#include "versioned_array_ptr.h"

namespace kpq {

template <class K, class V, int Rlx>
class shared_lsm_local {
    template <class X, class Y, int Z>
    friend class shared_lsm;
public:
    shared_lsm_local();
    virtual ~shared_lsm_local() { }

    void insert(const K &key,
                const V &val,
                versioned_array_ptr<K, V, Rlx> &global_array);
    void insert(block<K, V> *b,
                versioned_array_ptr<K, V, Rlx> &global_array);

    bool delete_min(V &val,
                    versioned_array_ptr<K, V, Rlx> &global_array);
    void peek(typename block<K, V>::peek_t &best,
              versioned_array_ptr<K, V, Rlx> &global_array);

private:
    /** The internal function responsible for actual insertion. The given
     *  block must have been allocated by the shared lsm. */
    void insert_block(block<K, V> *b,
                      versioned_array_ptr<K, V, Rlx> &global_array);

    /** Refreshes the local array copy and ensures that it is both up to date
     *  and consistent. observed_packed and observed_version are set to the
     *  corresponding values used to perform the copy. */
    void refresh_local_array_copy(block_array<K, V, Rlx> *&observed_packed,
                                  version_t &observed_version,
                                  versioned_array_ptr<K, V, Rlx> &global_array);

    bool local_array_copy_is_fresh(versioned_array_ptr<K, V, Rlx> &global_array) const;

private:
    /** Caches the previously peeked item in case we can short-circuit and simply
     *  return it. */
    typename block<K, V>::peek_t m_cached_best;

    /* ---- Item memory management. ---- */

    item_allocator<item<K, V>, typename item<K, V>::reuse> m_item_pool;

    /* ---- Block memory management. ---- */

    block_pool<K, V> m_block_pool;

    /* ---- Block array memory management. ---- */

    /** Contains a copy of the global block array, updated regularly. */
    block_array<K, V, Rlx> m_local_array_copy;

    /** Local memory pools for use by block arrays. */
    aligned_block_array<K, V, Rlx> m_array_pool_odds;
    aligned_block_array<K, V, Rlx> m_array_pool_evens;
};

#include "shared_lsm_local_inl.h"

}

#endif /* __SHARED_LSM_LOCAL_H */
