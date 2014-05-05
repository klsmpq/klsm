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

#include <random>

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
