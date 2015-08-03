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

template <class K, class V, int Relaxation>
shared_lsm<K, V, Relaxation>::shared_lsm()
{
}

template <class K, class V, int Relaxation>
void
shared_lsm<K, V, Relaxation>::insert(const K &key)
{
    insert(key, key);
}

template <class K, class V, int Relaxation>
void
shared_lsm<K, V, Relaxation>::insert(const K &key,
                                     const V &val)
{
    auto local = m_local_component.get();
    local->insert(key, val, m_global_array);
}

template <class K, class V, int Relaxation>
void
shared_lsm<K, V, Relaxation>::insert(block<K, V> *b)
{
    auto local = m_local_component.get();
    local->insert(b, m_global_array);
}

template <class K, class V, int Relaxation>
bool
shared_lsm<K, V, Relaxation>::delete_min(V &val)
{
    auto local = m_local_component.get();
    return local->delete_min(val, m_global_array);
}
