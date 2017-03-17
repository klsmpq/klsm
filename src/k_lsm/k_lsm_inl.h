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

template <class K, class V, int Rlx>
k_lsm<K, V, Rlx>::k_lsm()
{
}

template <class K, class V, int Rlx>
void
k_lsm<K, V, Rlx>::insert(const K &key)
{
    insert(key, key);
}

template <class K, class V, int Rlx>
void
k_lsm<K, V, Rlx>::insert(const K &key,
                         const V &val)
{
    /* Insert into the distributed lsm; if the largest block is large enough
     * (i.e. the next-largest block size would exceed the relaxation bounds),
     * insert it into the shared lsm instead.
     *
     * The concept seems simple, but let's see how easy it will be to integrate
     * the different memory management schemes.
     *
     * ---
     *
     * A closer look at dist lsm's insert(): It looks like we can reuse the original
     * behavior up to the point when merges have been fully performed. Then, if the
     * block is large enough, 1) insert it into the shared lsm and 2) clear the dist
     * lsm.
     *
     * Items might be read twice (once from dist, once from shared), but that does
     * not matter since item removal is atomic.
     *
     * The biggest issue might be how to integrate memory management of dist lsm
     * and shared lsm blocks, since they use completely different mechanics: the dist
     * lsm has a pool relying simply on a 'used' flag, while the shared lsm has
     * a pool relying on the published version (of the global array) and a status
     * of each block. Shared blocks (blocks managed by the shared lsm) will never be
     * seen by the dist lsm, and do not need special consideration. Dist blocks on
     * the other hand will be seen by shared.
     *
     * We could either:
     * 1)  Copy the contents of the dist block to a shared block upon insertion.
     *     This adds amortized constant complexity since it's done exactly once
     *     for each item. It would keep the code simple since we don't need special
     *     cases for handling dist blocks in shared.
     * 1a) Instead of copying upon insertion, once we realize the next block will
     *     exceed our bounds, we could ask the shared lsm to allocate a block for our
     *     use instead.
     * 2)  Pass dist blocks into shared. We'd then have ensure they are properly
     *     marked unused once they are removed, and reserve enough space in the
     *     dist block pool to handle the additional blocks floating around.
     *     A problem again is that we cannot guarantee bounds on the number of used
     *     blocks if we set them unused after successfully updating the global array,
     *     since the current thread may stall.
     *
     * It seems best to start with option 1), optimizing to 1a) in the future.
     */

    m_dist.insert(key, val, &m_shared);
}

template <class K, class V, int Rlx>
bool
k_lsm<K, V, Rlx>::delete_min(K &key, V &val)
{
    /* Load the best item from the local distributed lsm, and the (relaxed)
     * best item from the global lsm, and return the best of both.
     * If both are empty, perform spy on the distributed lsm and retry in
     * case it succeeds.
     *
     * ---
     *
     * The changes required for this seem fairly simple: we need to add
     * peek() to the public interface for both dist and shared lsm's, and
     * additionally spy() for the dist lsm. There are no further problematic
     * interactions between memory management here.
     */

    typename block<K, V>::peek_t
            best_dist = block<K, V>::peek_t::EMPTY(),
            best_shared = block<K, V>::peek_t::EMPTY();

    do {
        m_dist.find_min(best_dist);
        m_shared.find_min(best_shared);

        if (!best_dist.empty() && !best_shared.empty()) {
            if (best_dist.m_key <= best_shared.m_key) {
                COUNT_INC(dlsm_deletes);
                return best_dist.take(key, val);
            } else {
                COUNT_INC(slsm_deletes);
                return best_shared.take(key, val);
            }
        }

        if (!best_dist.empty() /* and best_shared is empty */) {
            COUNT_INC(dlsm_deletes);
            return best_dist.take(key, val);
        }

        if (!best_shared.empty() /* and best_dist is empty */) {
            COUNT_INC(slsm_deletes);
            return best_shared.take(key, val);
        }
    } while (m_dist.spy() > 0);

    return false;
}

template <class K, class V, int Rlx>
bool
k_lsm<K, V, Rlx>::delete_min(V &val)
{
    K key;
    return delete_min(key, val);
}
