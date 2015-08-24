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

#ifndef __LSM_H
#define __LSM_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace kpq
{

template <class T>
class LSMBlock;

template <class T>
class LSM
{
public:
    LSM();
    ~LSM();

    void insert(const T &k, const T &v);
    bool delete_min(T &v);
    void clear();

    void print() const;

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return false; }

private:
    /** Returns an unused block of size n == 2^i. */
    LSMBlock<T> *unused_block(const int n);
    void prune_last_block();

    void insert_between(LSMBlock<T> *new_block,
                        LSMBlock<T> *prev,
                        LSMBlock<T> *next);

private:
    LSMBlock<T> *m_head; /**< The smallest block in the list. */

    /** A list of all allocated blocks. The two blocks of size 2^i
     *  are stored in m_blocks[i]. */
    std::vector<std::pair<LSMBlock<T> *, LSMBlock<T> *>> m_blocks;
};

}

#endif /* __LSM_H */
