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

template <class K, class V, int Rlx>
void
dist_lsm<K, V, Rlx>::insert(const K &key)
{
    insert(key, key);
}

template <class K, class V, int Rlx>
void
dist_lsm<K, V, Rlx>::insert(const K &key,
                            const V &val)
{
    m_local.get()->insert(key, val, nullptr);
}

template <class K, class V, int Rlx>
void
dist_lsm<K, V, Rlx>::insert(const K &key,
                            const V &val,
                            shared_lsm<K, V, Rlx> *slsm)
{
    m_local.get()->insert(key, val, slsm);
}

template <class K, class V, int Rlx>
bool
dist_lsm<K, V, Rlx>::delete_min(V &val)
{
    return m_local.get()->delete_min(this, val);
}

template <class K, class V, int Rlx>
bool
dist_lsm<K, V, Rlx>::delete_min(K &key, V &val)
{
    return m_local.get()->delete_min(this, key, val);
}

template <class K, class V, int Rlx>
void
dist_lsm<K, V, Rlx>::find_min(typename block<K, V>::peek_t &best)
{
    m_local.get()->peek(best);
}

template <class K, class V, int Rlx>
int
dist_lsm<K, V, Rlx>::spy()
{
    return m_local.get()->spy(this);
}

template <class K, class V, int Rlx>
void
dist_lsm<K, V, Rlx>::print()
{
    for (size_t i = 0; i < m_local.num_threads(); i++) {
        m_local.get(i)->print();
    }
}
