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

#include "thread_local_ptr.h"

namespace kpq
{

/** An artificial thread id which can be used to index into the array of items. */
static constexpr int32_t TID_UNSET = -1;
static thread_local int32_t m_tid = TID_UNSET;
static std::atomic<int32_t> m_max_tid(0);

void
set_tid()
{
    if (m_tid == TID_UNSET) {
        m_tid = m_max_tid.fetch_add(1, std::memory_order_relaxed);
    }
}

int32_t
tid()
{
    return m_tid;
}

int32_t
max_tid()
{
    return m_max_tid.load(std::memory_order_relaxed);
}

}
