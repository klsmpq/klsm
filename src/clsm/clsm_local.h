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

#ifndef __CLSM_LOCAL_H
#define __CLSM_LOCAL_H

#include "block_storage.h"
#include "item.h"
#include "mm.h"
#include "thread_local_ptr.h"

namespace kpq
{

template <class K, class V>
class clsm_local
{
public:
    clsm_local();
    virtual ~clsm_local();

    void insert(const K &key,
                const V &val);
    bool delete_min(V &val);

private:
    block<K, V> *m_head; /**< The largest  block. */
    block<K, V> *m_tail; /**< The smallest block. */

    block_storage<K, V> m_block_storage;
    item_allocator<item<K, V>, typename item<K, V>::reuse> m_item_allocator;
};

}

#endif /* __CLSM_LOCAL_H */
