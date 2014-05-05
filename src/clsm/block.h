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

#ifndef __BLOCK_H
#define __BLOCK_H

#include <utility>

#include "item.h"

namespace kpq
{

template <class K, class V>
class block
{
public:
    block(const size_t power_of_2);
    virtual ~block();

    bool used() const;
    void set_unused();

private:
    typedef std::pair<item<K, V>, version_t> item_pair_t;

    const size_t m_power_of_2;
    const size_t m_capacity;
    item_pair_t *m_item_pairs;
    bool m_used;
};

}

#endif /* __BLOCK_H */
