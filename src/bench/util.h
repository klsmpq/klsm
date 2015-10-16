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

#ifndef __UTIL_H
#define __UTIL_H

#include <ctime>
#include <cstdint>
#include <vector>

class hwloc_wrapper_private;

class hwloc_wrapper
{
public:
    hwloc_wrapper();
    virtual ~hwloc_wrapper();

    void pin_to_core(const int id);

private:
    hwloc_wrapper_private *m_p;
};

double
timediff_in_s(const struct timespec &start,
              const struct timespec &end);

std::vector<uint32_t>
random_array(const size_t n,
             const int seed);

uint64_t
rdtsc();

#endif /* __UTIL_H */
