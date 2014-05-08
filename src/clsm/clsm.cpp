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

#include "clsm.h"

namespace kpq
{

template <class K, class V>
void
clsm<K, V>::insert(const K &key)
{
    insert(key, key);
}

template <class K, class V>
void
clsm<K, V>::insert(const K &key,
                   const V &val)
{
    m_local.get()->insert(key, val);
}

template <class K, class V>
bool
clsm<K, V>::delete_min(V &val)
{
    return m_local.get()->delete_min(this, val);
}

template <class K, class V>
void
clsm<K, V>::print()
{
    for (size_t i = 0; i < m_local.num_threads(); i++) {
        m_local.get(i)->print();
    }
}

template class clsm<uint32_t, uint32_t>;

}
