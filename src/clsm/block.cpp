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

template <class T>
block<T>::block(const size_t power_of_2) :
    m_power_of_2(power_of_2),
    m_capacity(1 << power_of_2),
    m_used(false)
{
}

template <class T>
block<T>::~block()
{
}

template <class T>
bool
block<T>::used() const
{
    return m_used;
}

template <class T>
void
block<T>::set_unused()
{
    assert(m_used);
    m_used = false;
}

template class block<uint32_t>;

}
