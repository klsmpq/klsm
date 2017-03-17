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

#ifndef __ITEM_H
#define __ITEM_H

#include <atomic>
#include <cassert>
#include <cstddef>

namespace kpq
{

typedef uint32_t version_t;

template <class K, class V>
class item
{
public:
    item();

    void initialize(const K &key,
                    const V &val);
    bool take(const version_t version,
              V &val);
    bool take(const version_t version,
              K &key, V &val);

    K key() const;
    V val() const;

    version_t version() const;
    bool used() const;

    class reuse
    {
    public:
        bool operator()(const item<K, V> &item) const
        {
            return !item.used();
        }
    };

private:
    /** Even versions are reusable, odd versions are in use. */
    std::atomic<version_t> m_version;
    K m_key;
    V m_val;
};

#include "item_inl.h"

}

#endif /* __ITEM_H */
