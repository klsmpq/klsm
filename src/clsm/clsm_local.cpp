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

#include "clsm_local.h"

namespace kpq
{

template <class K, class V>
clsm_local<K, V>::clsm_local() :
    m_head(nullptr),
    m_tail(nullptr)
{
}

template <class K, class V>
clsm_local<K, V>::~clsm_local()
{
    /* Blocks and items are managed by, respectively,
     * block_storage and item_allocator. */
}

template <class K, class V>
void
clsm_local<K, V>::insert(const K &,
                         const V &)
{
}

template <class K, class V>
bool
clsm_local<K, V>::delete_min(V &)
{
    return false;
}

template class clsm_local<uint32_t, uint32_t>;

}
