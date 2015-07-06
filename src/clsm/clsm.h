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

#ifndef __CLSM_H
#define __CLSM_H

#include "clsm_local.h"

namespace kpq
{

template <class K, class V>
class clsm
{
    friend int clsm_local<K, V>::spy(clsm<K, V> *parent);

public:

    /**
     * Inserts a new item into the local LSM.
     */
    void insert(const K &key);
    void insert(const K &key,
                const V &val);

    /**
     * Attempts to remove the locally (i.e. on the current thread) minimal item.
     * If the local LSM is empty, we try to copy items from another active thread.
     * In case the local LSM is still empty, false is returned.
     * If a locally minimal element is successfully found and removed, true is returned.
     */
    bool delete_min(V &val);

    void print();

private:
    thread_local_ptr<clsm_local<K, V>> m_local;
};

#include "clsm_inl.h"

}

#endif /* __CLSM_H */
