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

#ifndef __BLOCK_STORAGE_H
#define __BLOCK_STORAGE_H

#include <tuple>
#include <vector>

#include "block.h"

namespace kpq
{

/**
 * Maintains 3-tuples of memory blocks of size 2^i.
 */

template <class T>
class block_storage
{
public:

private:
    typedef std::tuple<block<T>, block<T>, block<T>> block_3_tuple;

    std::vector<block_3_tuple> m_blocks;
};

}

#endif /* __BLOCK_STORAGE_H */
