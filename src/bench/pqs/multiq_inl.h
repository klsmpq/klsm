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

static thread_local kpq::xorshf96 local_rng;

template <class K, class V, int C>
multiq<K, V, C>::multiq(const size_t num_threads) :
    m_num_threads(num_threads)
{
    m_queues = new local_queue[num_queues()]();
    m_locks = new local_lock[num_queues()]();
}

template <class K, class V, int C>
multiq<K, V, C>::~multiq()
{
    delete[] m_queues;
    delete[] m_locks;
}

template <class K, class V, int C>
bool
multiq<K, V, C>::delete_min(V &value)
{
    /* Peek at two random queues and lock the one with the minimal item. */

    const int nqueues = num_queues();
    size_t i, j;

    while (true) {
        do {
            i = local_rng() % nqueues;
            j = local_rng() % nqueues;

            if (m_queues[i].m_top > m_queues[j].m_top) {
                std::swap(i, j);
            }
        } while (!lock(i));

        auto &pq = m_queues[i].m_pq;
        const auto item = pq.top();

        if (item.key == SENTINEL_KEY) {
            // Empty queue, retry indefinitely.
            // TODO: Not a permanent solution, but original data structure does the same.
            unlock(i);
            return false;
        } else {
            value = item.value;
            pq.pop();
            unlock(i);
            return true;
        }
    }

    assert(false);  // Never reached.
    return false;
}

template <class K, class V, int C>
void
multiq<K, V, C>::insert(const K &key,
                        const V &value)
{
    /* Lock a random priority queue and insert into it. */

    const int nqueues = num_queues();
    size_t i;

    do {
        i = local_rng() % nqueues;
    } while (!lock(i));

    m_queues[i].m_pq.emplace(key, value);
    m_queues[i].m_top = m_queues[i].m_pq.top().key;

    unlock(i);
}

template <class K, class V, int C>
bool
multiq<K, V, C>::lock(const size_t ix)
{
    bool expected = false;
    return m_locks[ix].m_is_locked.compare_exchange_strong(
            expected, true, std::memory_order_seq_cst);
}

template <class K, class V, int C>
void
multiq<K, V, C>::unlock(const size_t ix)
{
    bool expected = true;
    const bool succeeded = m_locks[ix].m_is_locked.compare_exchange_strong(
                    expected, false, std::memory_order_seq_cst);
    assert(succeeded), (void)succeeded;
}

template <class K, class V, int C>
void multiq<K, V, C>::print() const
{
    /* NOP */
}
