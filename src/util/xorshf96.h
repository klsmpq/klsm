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

/**
 * Fast Marsaglia xorshf random number generator.
 */

class xorshf96
{
public:
    typedef uint64_t result_type;
    static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }

    xorshf96()
    {
        const auto d = std::chrono::high_resolution_clock::now().time_since_epoch();
        x = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    }

    xorshf96(const uint64_t seed)
    {
        x = seed;
    }

    uint64_t operator()()
    {
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
    uint64_t x = 123456789;
    uint64_t y = 362436069;
    uint64_t z = 521288629;
};

}

#endif /* __XORSHF96_H */
