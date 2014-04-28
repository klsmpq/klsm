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

#ifndef __MM_H
#define __MM_H

#include <cstddef>

namespace kpq
{

/**
 * The wait-free memory management scheme by Wimmer (www.pheet.org).
 */

template <class T>
class item_allocator_item
{
public:
    T m_item;
    item_allocator_item<T> *m_next;
};

template <class T, class ReuseCheck>
class item_allocator
{
public:
    typedef T              value_type;
    typedef T             *pointer;
    typedef const T       *const_pointer;
    typedef T             &reference;
    typedef const T       &const_reference;
    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;

    item_allocator() :
        is_reusable()
    {
        m_head = m_tail = new item_allocator_item<T>();
        m_head->m_next = m_head;
    }

    virtual ~item_allocator()
    {
        const auto tail = m_head;
        do {
            auto next = m_head->m_next;
            delete m_head;
            m_head = next;
        } while (m_head != tail);
    }

    pointer acquire()
    {
        while (m_head->m_next != m_tail) {
            if (is_reusable(m_head->m_next->m_item)) {
                m_head = m_head->m_next;

                m_tail = m_tail->m_next;
                if (m_tail != m_head) {
                    m_tail = m_tail->m_next;
                }

                return &m_head->m_item;
            }
            m_head = m_head->m_next;
        }

        m_head->m_next = new item_allocator_item<T>();
        m_head->m_next->m_next = m_tail;
        m_head = m_head->m_next;
        m_tail = m_tail->m_next;

        return &m_head->m_item;
    }

private:
    item_allocator_item<T> *m_head;
    item_allocator_item<T> *m_tail;

    const ReuseCheck is_reusable;
};

}

#endif /* __MM_H */
