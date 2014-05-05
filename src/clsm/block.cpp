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

#include "block.h"

#include <cassert>

namespace kpq
{


template <class K, class V>
block<K, V>::block(const size_t power_of_2) :
    m_next(nullptr),
    m_prev(nullptr),
    m_power_of_2(power_of_2),
    m_capacity(1 << power_of_2),
    m_item_pairs(new item_pair_t[m_capacity]),
    m_used(false)
{
}

template <class K, class V>
block<K, V>::~block()
{
    delete[] m_item_pairs;
}

template <class K, class V>
bool
block<K, V>::used() const
{
    return m_used;
}

template <class K, class V>
void
block<K, V>::set_unused()
{
    assert(m_used);
    m_used = false;
}

template class block<uint32_t, uint32_t>;

}
