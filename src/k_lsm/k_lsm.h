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

#ifndef __K_LSM_H
#define __K_LSM_H

#include "dist_lsm/dist_lsm.h"
#include "shared_lsm/shared_lsm.h"
#include "util/counters.h"

namespace kpq {

/**
 * The k-lsm combines the distributed- and shared lsm data structures
 * in order to emphasize their respective strenghts. Items are initially
 * inserted into (thread-local) distributed lsm's until the relaxation
 * limit is reached, at which point the contained item's are inserted
 * into the shared lsm component.
 *
 * As always, K, V and Rlx denote, respectively, the key, value classes
 * and the relaxation parameter.
 */

template <class K, class V, int Rlx>
class k_lsm {
public:
    k_lsm();
    virtual ~k_lsm() { }

    void insert(const K &key);
    void insert(const K &key,
                const V &val);

    bool delete_min(V &val);
    bool delete_min(K &key, V &val);

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return true; }

private:
    dist_lsm<K, V, Rlx>   m_dist;
    shared_lsm<K, V, Rlx> m_shared;
};

#include "k_lsm_inl.h"

}

#endif /* __K_LSM_H */
