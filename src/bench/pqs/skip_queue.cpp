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

#include "skip_queue.h"

namespace kpqbench
{

template <class T>
void
skip_queue<T>::insert(const T &key,
                      const T & /* Unused */)
{
    m_pq.insert(key);
}

template <class T>
bool
skip_queue<T>::delete_min(T &v)
{
    if (m_pq.empty()) {
        return false;
    }

    v = m_pq.front();
    m_pq.erase(m_pq.begin());

    return true;
}

template <class T>
void
skip_queue<T>::clear()
{
    m_pq.clear();
}

template class skip_queue<uint32_t>;

}
