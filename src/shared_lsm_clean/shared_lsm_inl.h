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
//    version_t global_version;
//    typename block<K, V>::peek_t best;
//    do {
//        refresh_local_array_copy();
//        best = m_local_array_copy.get()->peek();
//        global_version = m_block_array.load(std::memory_order_relaxed)->version();
//    } while (global_version != m_local_array_copy.get()->version());

//    if (best.m_item == nullptr) {
//        return false;  /* We did our best, give up. */
//    }

//    return best.m_item->take(best.m_version, val);
    return false;
}
