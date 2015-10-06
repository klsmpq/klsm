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

#ifndef __MULTI_LSM_H
#define __MULTI_LSM_H

#include "dist_lsm/dist_lsm_local.h"
#include "util/counters.h"
#include "util/thread_local_ptr.h"

namespace kpq {

template <class K, class V, int C = 4>
class multi_lsm {
public:
    multi_lsm(const size_t num_threads);
    virtual ~multi_lsm();

    void insert(const K &key);
    void insert(const K &key,
                const V &val);

    bool delete_min(V &val);

    void init_thread(const size_t) const { set_tid(); }
    constexpr static bool supports_concurrency() { return true; }

private:
    /** Relaxation is meaningless when there is no slsm. */
    static constexpr int DUMMY_RELAXATION = (1 << 20);

    dist_lsm_local<K, V, DUMMY_RELAXATION> *random_local_queue() const;
    dist_lsm_local<K, V, DUMMY_RELAXATION> *random_queue() const;

private:
    const size_t m_num_threads;
    const size_t m_num_queues;

    dist_lsm_local<K, V, DUMMY_RELAXATION> *m_dist;
};

#include "multi_lsm_inl.h"

}

#endif /* __MULTI_LSM_H */
