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
    m_dist.insert(key, val);
}

template <class K, class V, int Rlx>
bool
k_lsm<K, V, Rlx>::delete_min(V &val)
{
    return m_dist.delete_min(val);
}
