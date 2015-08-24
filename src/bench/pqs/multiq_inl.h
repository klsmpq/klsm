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

template <class K, class V, int C>
multiq<K, V, C>::multiq()
{
}

template <class K, class V, int C>
multiq<K, V, C>::~multiq()
{
}

template <class K, class V, int C>
bool
multiq<K, V, C>::delete_min(V &value)
{
    return false;
}

template <class K, class V, int C>
void
multiq<K, V, C>::insert(const K &key,
                     const V &value)
{
}

template <class K, class V, int C>
void multiq<K, V, C>::print() const
{
    /* NOP */
}
