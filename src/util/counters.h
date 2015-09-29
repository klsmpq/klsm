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

#ifndef __COUNTERS_H
#define __COUNTERS_H

#include <cstddef>
#include <cstdio>

namespace kpq
{

/**
 * Very simple performance counters.
 */

struct counters
{
    counters()
    {
        memset(this, 0, sizeof(*this));
    }

    counters &operator+=(const counters &that)
    {
        inserts += that.inserts;
        successful_deletes += that.successful_deletes;
        failed_deletes += that.failed_deletes;
        block_shrinks += that.block_shrinks;

        return *this;
    }

    size_t operations() const {
        return inserts + successful_deletes + failed_deletes;
    }

    void print() const {
        printf("inserts: %lu\n"
               "successful_deletes: %lu\n"
               "failed_deletes: %lu\n"
               "block_shrinks: %lu\n",
               inserts, successful_deletes, failed_deletes, block_shrinks);
    }

    size_t inserts;
    size_t successful_deletes;
    size_t failed_deletes;

    size_t block_shrinks;
};

thread_local counters COUNTERS;

}

#endif /* __COUNTERS_H */
