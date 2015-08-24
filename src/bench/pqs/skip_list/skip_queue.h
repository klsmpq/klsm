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

#ifndef __SKIP_QUEUE_H
#define __SKIP_QUEUE_H

#include "skip_list.h"

namespace kpqbench
{

template <class T>
class skip_queue
{
public:
    void insert(const T &key, const T &value);
    bool delete_min(T &v);
    void clear();

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return false; }

private:
    goodliffe::multi_skip_list<T> m_pq;
};

}

#endif /* __SKIP_QUEUE_H */
