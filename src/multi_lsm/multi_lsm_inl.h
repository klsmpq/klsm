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

static thread_local xorshf96 multiq_local_rng;

template <class K, class V, int C>
multi_lsm<K, V, C>::multi_lsm(const size_t num_threads) :
    m_num_threads(num_threads),
    m_num_queues(num_threads * C)
{
    m_dist = new dist_lsm_local<K, V, DUMMY_RELAXATION>[m_num_queues]();
}

template <class K, class V, int C>
multi_lsm<K, V, C>::~multi_lsm()
{
    delete[] m_dist;
}

template <class K, class V, int C>
void
multi_lsm<K, V, C>::insert(const K &key)
{
    insert(key, key);
}

template <class K, class V, int C>
void
multi_lsm<K, V, C>::insert(const K &key,
                           const V &val)
{
}

template <class K, class V, int C>
bool
multi_lsm<K, V, C>::delete_min(V &val)
{
    return false;
}
