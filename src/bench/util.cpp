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

#include "util.h"

#include <hwloc.h>
#include <random>
#include <x86intrin.h>

class hwloc_wrapper_private
{
public:
    hwloc_topology_t m_topology;
};

hwloc_wrapper::hwloc_wrapper()
{
    m_p = new hwloc_wrapper_private();

    hwloc_topology_init(&m_p->m_topology);
    hwloc_topology_load(m_p->m_topology);
}

hwloc_wrapper::~hwloc_wrapper()
{
    hwloc_topology_destroy(m_p->m_topology);
    delete m_p;
}

void
hwloc_wrapper::pin_to_core(const int id)
{
    const int depth = hwloc_get_type_or_below_depth(m_p->m_topology, HWLOC_OBJ_CORE);
    const int ncores = hwloc_get_nbobjs_by_depth(m_p->m_topology, depth);

    const hwloc_obj_t obj = hwloc_get_obj_by_depth(m_p->m_topology, depth, id % ncores);

    hwloc_cpuset_t cpuset = hwloc_bitmap_dup(obj->cpuset);
    hwloc_bitmap_singlify(cpuset);

    if (hwloc_set_cpubind(m_p->m_topology, cpuset, HWLOC_CPUBIND_THREAD) != 0) {
        fprintf(stderr, "Could not bind to core: %s\n", strerror(errno));
    }

    hwloc_bitmap_free(cpuset);
}

double
timediff_in_s(const struct timespec &start,
              const struct timespec &end)
{
    struct timespec tmp;
    if (end.tv_nsec < start.tv_nsec) {
        tmp.tv_sec = end.tv_sec - start.tv_sec - 1;
        tmp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        tmp.tv_sec = end.tv_sec - start.tv_sec;
        tmp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    return tmp.tv_sec + (double)tmp.tv_nsec / 1000000000.0;
}

uint64_t
rdtsc()
{
    return __rdtsc();
}

std::vector<uint32_t>
random_array(const size_t n,
             const int seed)
{
    std::vector<uint32_t> xs;
    xs.reserve(n);

    std::mt19937 gen(seed);
    std::uniform_int_distribution<> rand_int;

    for (size_t i = 0; i < n; i++) {
        xs.push_back(rand_int(gen));
    }

    return xs;
}
