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

#include "globallock.h"

namespace kpq
{

bool
GlobalLock::delete_min(uint32_t &v)
{
    std::lock_guard<std::mutex> g(m_mutex);

    if (m_q.empty()) {
        return false;
    }

    v = m_q.top();
    m_q.pop();

    return true;
}

void
GlobalLock::insert(const uint32_t v)
{
    std::lock_guard<std::mutex> g(m_mutex);

    m_q.push(v);
}

void
GlobalLock::clear()
{
    std::lock_guard<std::mutex> g(m_mutex);

    m_q = pq_t();
}

}
