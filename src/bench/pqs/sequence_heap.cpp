/*
 *  Copyright 2014 Jakob Gruber
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

#include "sequence_heap.h"

#include <limits>

#include "knheap.h"

namespace kpqbench
{

template <class T>
sequence_heap<T>::sequence_heap()
{
    m_pq = new KNHeap<T, T>(std::numeric_limits<uint32_t>::max(),
                            std::numeric_limits<uint32_t>::min());
}

template <class T>
void
sequence_heap<T>::insert(const T &key,
                         const T &value)
{
    m_pq->insert(key, value);
}

template <class T>
bool
sequence_heap<T>::delete_min(T &v)
{
    if (m_pq->getSize() == 0) {
        return false;
    }

    T w;
    m_pq->deleteMin(&w, &v);

    return true;
}

template <class T>
void
sequence_heap<T>::clear()
{
    delete m_pq;
    m_pq = new KNHeap<T, T>(std::numeric_limits<uint32_t>::max(),
                            std::numeric_limits<uint32_t>::min());
}

template class sequence_heap<uint32_t>;

}
