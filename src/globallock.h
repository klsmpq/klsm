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

#ifndef __GLOBALLOCK_H
#define __GLOBALLOCK_H

#include <mutex>
#include <queue>

namespace kpq
{

class GlobalLock
{
public:
    void insert(const uint32_t v);
    bool delete_min(uint32_t &v);
    void clear();

    void print() const;

private:
    typedef std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> pq_t;

    std::mutex m_mutex;
    pq_t m_q;
};

}

#endif /* __GLOBALLOCK_H */
