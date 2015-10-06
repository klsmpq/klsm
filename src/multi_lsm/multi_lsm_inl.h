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

static thread_local xorshf96 mlsm_local_rng;

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
    /* Insert into a random local queue. */

    auto q = random_local_queue();
    q->insert(key, val, nullptr);
}

template <class K, class V, int C>
bool
multi_lsm<K, V, C>::delete_min(V &val)
{
    /* Delete the better item from two random queues. */

    auto q1 = random_local_queue();
    auto q2 = random_queue();

    typename block<K, V>::peek_t it1 = block<K, V>::peek_t::EMPTY();
    typename block<K, V>::peek_t it2 = block<K, V>::peek_t::EMPTY();

    q1->peek(it1);
    q2->safe_peek(it2);

    const bool it1_empty = it1.empty();
    const bool it2_empty = it2.empty();

    if (it1_empty && it2_empty) {
        return false;
    } else if (it1_empty) {
        return it2.take(val);
    } else if (it2_empty) {
        return it1.take(val);
    } else {
        return (it1.m_key < it2.m_key) ? it1.take(val) : it2.take(val);
    }

    return false;
}

template <class K, class V, int C>
dist_lsm_local<K, V, multi_lsm<K, V, C>::DUMMY_RELAXATION> *
multi_lsm<K, V, C>::random_local_queue() const
{
    const int id = tid();
    const size_t ix = (C * id) + (mlsm_local_rng() % C);
    assert(ix < m_num_queues);
    return &m_dist[ix];
}

template <class K, class V, int C>
dist_lsm_local<K, V, multi_lsm<K, V, C>::DUMMY_RELAXATION> *
multi_lsm<K, V, C>::random_queue() const
{
    return &m_dist[mlsm_local_rng() % m_num_queues];
}
