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

#ifndef __SEQUENCE_HEAP_H
#define __SEQUENCE_HEAP_H

#include <cstddef>

template <class K, class V>
class KNHeap;

namespace kpqbench
{

template <class T>
class sequence_heap
{
public:
    sequence_heap();

    void insert(const T &k, const T &v);
    bool delete_min(T &v);
    void clear();

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return false; }

private:
    KNHeap<T, T> *m_pq;
};

}

#endif /* __SEQUENCE_HEAP_H */
