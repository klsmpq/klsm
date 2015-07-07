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

#ifndef __SHAREDLSM_H
#define __SHAREDLSM_H

#include <atomic>

#include "block_array.h"
#include "mm.h"
#include "thread_local_ptr.h"

namespace kpq {

template <class K, class V>
class shared_lsm {
public:
    shared_lsm();
    virtual ~shared_lsm() { }

    void insert(const K &key);
    void insert(const K &key,
                const V &val);
    void insert(block<K, V> *b);

    bool delete_min(V &val);

private:
    std::atomic<block_array<K, V> *> m_blocks;

    thread_local_ptr<item_allocator<item<K, V>, typename item<K, V>::reuse>> m_item_allocators;
};

#include "sharedlsm_inl.h"

}

#endif /* __SHAREDLSM_H */
