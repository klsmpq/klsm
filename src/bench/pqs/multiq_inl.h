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

template <class K, class V, int C>
multiq<K, V, C>::multiq(const size_t num_threads) :
    m_num_threads(num_threads)
{
    m_queues = new multiq_local[num_threads * C]();
}

template <class K, class V, int C>
multiq<K, V, C>::~multiq()
{
    delete[] m_queues;
}

template <class K, class V, int C>
bool
multiq<K, V, C>::delete_min(V &value)
{
    auto &gen = *m_local_gen.get();

    /* Peek at two random queues and lock the one with the minimal item. */

    std::uniform_int_distribution<size_t> dist(0, num_queues() - 1);
    size_t i, j;

    while (true) {
        do {
            i = dist(gen);
            j = dist(gen);

            auto ith_top = m_queues[i].m_pq.top();
            auto jth_top = m_queues[j].m_pq.top();

            if (ith_top > jth_top) {
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
    auto gen = m_local_gen.get();

    /* Lock a random priority queue and insert into it. */

    std::uniform_int_distribution<size_t> dist(0, num_queues() - 1);
    size_t i;

    do {
        i = dist(*gen);
    } while (!lock(i));

    m_queues[i].m_pq.push({key, value});

    unlock(i);
}

template <class K, class V, int C>
bool
multiq<K, V, C>::lock(const size_t ix)
{
    bool expected = false;
    return m_queues[ix].m_is_locked.compare_exchange_strong(
            expected, true, std::memory_order_relaxed);
}

template <class K, class V, int C>
void
multiq<K, V, C>::unlock(const size_t ix)
{
    bool expected = true;
    const bool succeeded = m_queues[ix].m_is_locked.compare_exchange_strong(
                    expected, false, std::memory_order_relaxed);
    assert(succeeded), (void)succeeded;
}

template <class K, class V, int C>
void multiq<K, V, C>::print() const
{
    /* NOP */
}
