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

#ifndef __MULTIQ_H
#define __MULTIQ_H

#include <cstring>
#include <mutex>

namespace kpqbench
{

/**
 * Reimplementation of a multiqueue as described in Rihani, Sanders, Dementiev:
 * "MultiQueues: Simpler, Faster, and Better Relaxed Concurrent Priority Queues".
 * C is a tuning parameter specifying the number of internal queues per thread.
 */
template <class K, class V, int C = 4>
class multiq
{
public:
    multiq();
    virtual ~multiq();

    void insert(const K &key, const V &value);
    bool delete_min(V &value);
    void clear();

    void print() const;

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return true; }

private:
};

#include "multiq_inl.h"

}

#endif /* __MULTIQ_H */
