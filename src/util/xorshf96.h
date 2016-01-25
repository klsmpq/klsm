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

#ifndef __XORSHF96_H
#define __XORSHF96_H

#include <chrono>
#include <cstddef>

namespace kpq
{

static __thread bool __xorshf96_initialized;

/**
 * Fast Marsaglia xorshf random number generator.
 */

class xorshf96
{
public:
    typedef uint64_t result_type;
    static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }

    uint64_t operator()()
    {
        if (!__xorshf96_initialized) {
            __xorshf96_initialized = true;
            x = 123456789 + tid();
            y = 362436069;
            z = 521288629;
        }

        x ^= x << 16;
        x ^= x >> 5;
        x ^= x << 1;

        uint64_t t = x;
        x = y;
        y = z;
        z = t ^ x ^ y;

        return z;
    }

private:
    uint64_t x;
    uint64_t y;
    uint64_t z;
};

}

#endif /* __XORSHF96_H */
