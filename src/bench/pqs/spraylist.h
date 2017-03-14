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

#ifndef __SPRAYLIST_H
#define __SPRAYLIST_H

#include <iostream>

struct sl_intset;
typedef sl_intset sl_intset_t;

namespace kpqbench {

class spraylist
{
public:
    spraylist(const size_t nthreads);
    virtual ~spraylist();

    void init_thread(const size_t nthreads);

    void insert(const uint32_t &k, const uint32_t &v);
    void insert(const size_t &k, const size_t &v);
    bool delete_min(uint32_t &v);
    bool delete_min(size_t &k, size_t &v);

    static void print_name() { std::cout << "spraylist"; }
    constexpr static bool supports_concurrency() { return true; }

private:
    typedef sl_intset_t pq_t;

    pq_t *m_q;
};

}

#endif /* __SPRAYLIST_H */
