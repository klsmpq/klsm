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
shared_lsm<K, V, Rlx>::shared_lsm()
{
}

template <class K, class V, int Rlx>
void
shared_lsm<K, V, Rlx>::insert(const K &key)
{
    insert(key, key);
}

template <class K, class V, int Rlx>
void
shared_lsm<K, V, Rlx>::insert(const K &key,
                              const V &val)
{
    auto local = m_local_component.get();
    local->insert(key, val, m_global_array);
}

template <class K, class V, int Rlx>
void
shared_lsm<K, V, Rlx>::insert(block<K, V> *b)
{
    auto local = m_local_component.get();
    local->insert(b, m_global_array);
}

template <class K, class V, int Rlx>
bool
shared_lsm<K, V, Rlx>::delete_min(V &val)
{
    auto local = m_local_component.get();
    return local->delete_min(val, m_global_array);
}

template <class K, class V, int Rlx>
void
shared_lsm<K, V, Rlx>::find_min(typename block<K, V>::peek_t &best)
{
    auto local = m_local_component.get();
    local->peek(best, m_global_array);
}
