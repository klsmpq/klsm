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

template <class T, size_t BlockSize>
class item_allocator_item
{
public:
    T m_items[BlockSize];
    item_allocator_item<T, BlockSize> *m_next;
};

template <class T, class ReuseCheck, size_t BlockSize = 1024>
class item_allocator
{
    static constexpr size_t AMORTIZATION = 1;
public:
    typedef T              value_type;
    typedef T             *pointer;
    typedef const T       *const_pointer;
    typedef T             &reference;
    typedef const T       &const_reference;
    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;

    item_allocator() :
        m_offset(0),
        m_amortized(0),
        m_total_size(BlockSize),
        m_new_block(true),
        is_reusable()
    {
        m_head = new item_allocator_item<T, BlockSize>();
        m_head->m_next = m_head;
    }

    virtual ~item_allocator()
    {
        auto next = m_head->m_next;
        while (next != m_head) {
            auto nnext = next->m_next;
            delete next;
            next = nnext;
        }
        delete m_head;
    }

    pointer acquire()
    {
        while (true) {
            while (m_offset < BlockSize) {
                auto item = &m_head->m_items[m_offset++];
                if (m_new_block || is_reusable(*item)) {
                    m_amortized += 1 + AMORTIZATION;
                    return item;
                }
            }

            if (m_amortized < BlockSize) {
                auto new_block = new item_allocator_item<T, BlockSize>();
                new_block->m_next = m_head->m_next;
                m_head->m_next = new_block;
                m_head = new_block;
                m_total_size += BlockSize;
                m_new_block = true;
            } else {
                m_amortized = std::min(m_amortized, m_total_size * AMORTIZATION);
                m_amortized -= BlockSize;
                m_head = m_head->m_next;
                m_new_block = false;
            }
            m_offset = 0;
        }
    }

private:
    item_allocator_item<T, BlockSize> *m_head;

    size_t m_offset;
    size_t m_amortized;
    size_t m_total_size;
    bool m_new_block;

    const ReuseCheck is_reusable;
};

}

#endif /* __MM_H */
