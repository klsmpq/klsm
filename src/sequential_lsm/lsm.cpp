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

#include "lsm.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

namespace kpq
{

template <class T>
class LSMElem
{
public:
    LSMElem() :
        m_used(false)
    {
    }

    T peek() const
    {
        assert(m_used);
        return m_elem;
    }

    T pop()
    {
        assert(m_used);
        m_used = false;
        return m_elem;
    }

    bool used() const
    {
        return m_used;
    }

    void put(const T &v)
    {
        assert(!m_used);
        m_used = true;
        m_elem = v;
    }

private:
    T m_elem;
    bool m_used;
};

template <class T>
class LSMBlock
{
public:
    LSMBlock(const int n) :
        m_prev(nullptr),
        m_next(nullptr),
        m_elems(n),
        m_first(0),
        m_size(0),
        m_capacity(n),
        m_used(false)
    {
    }

    void put(const T &v)
    {
        assert(m_capacity == 1);
        assert(m_size == 0);
        assert(!m_used);

        m_first = 0;
        m_size = 1;
        m_elems[0].put(v);
        m_used = true;
        m_prev = m_next = nullptr;
    }

    void merge(LSMBlock<T> *lhs,
               LSMBlock<T> *rhs)
    {
        assert(m_capacity == lhs->capacity() * 2);
        assert(lhs->capacity() == rhs->capacity());
        assert(!m_used);

        m_first = 0;
        m_size = lhs->size() + rhs->size();
        m_used = true;
        m_prev = m_next = nullptr;

        /* Merge. */

        size_t l = 0, r = 0, dst = 0;

        while (l < lhs->capacity() && r < rhs->capacity()) {
            auto &lelem = lhs->m_elems[l];
            auto &relem = rhs->m_elems[r];

            if (!lelem.used()) {
                l++;
                continue;
            }

            if (!relem.used()) {
                r++;
                continue;
            }

            if (lelem.peek() < relem.peek()) {
                m_elems[dst++] = lelem;
                lelem.pop();
                l++;
            } else {
                m_elems[dst++] = relem;
                relem.pop();
                r++;
            }
        }

        while (l < lhs->capacity()) {
            auto &lelem = lhs->m_elems[l];
            if (!lelem.used()) {
                l++;
                continue;
            }
            m_elems[dst++] = lelem;
            lelem.pop();
            l++;
        }

        while (r < rhs->capacity()) {
            auto &relem = rhs->m_elems[r];
            if (!relem.used()) {
                r++;
                continue;
            }
            m_elems[dst++] = relem;
            relem.pop();
            r++;
        }

        lhs->set_unused();
        rhs->set_unused();
    }

    void shrink(LSMBlock<T> *that)
    {
        assert(m_size == 0);
        assert(m_capacity == that->capacity() / 2);
        assert(!m_used);

        uint32_t j = 0;
        for (uint32_t i = that->m_first; i < that->m_capacity; i++) {
            if (that->m_elems[i].used()) {
                m_elems[j++] = that->m_elems[i];
                that->m_elems[i].pop();
            }
        }

        m_size = that->size();
        m_first = 0;
        m_used = true;
        m_prev = m_next = nullptr;

        that->set_unused();
    }

    bool peek(T &v) const
    {
        if (m_size == 0) {
            return false;
        }

        v = m_elems[m_first].peek();
        return true;
    }

    bool pop(T &v)
    {
        if (m_size == 0) {
            return false;
        }

        v = m_elems[m_first].pop();
        m_size--;
        m_first++;
        return true;
    }

    size_t size() const
    {
        return m_size;
    }

    size_t capacity() const
    {
        return m_capacity;
    }

    bool used() const
    {
        return m_used;
    }

    void set_unused()
    {
        assert(m_used);

        m_used = false;
        m_first = 0;
        m_size = 0;
    }

    void print() const
    {
        printf("size: %d, capacity: %d, first: %d\n", m_size, m_capacity, m_first);
        for (size_t i = 0; i < m_capacity; i++) {
            T v = 0;
            if (m_elems[i].used()) {
                v = m_elems[i].peek();
            }
            printf("%zu: { %d, %d }\n", i, v, m_elems[i].used());
        }
    }

public:
    LSMBlock<T> *m_prev, *m_next;

private:
    std::vector<LSMElem<T>> m_elems;
    uint32_t m_first, m_size;
    const uint32_t m_capacity;
    bool m_used;
};

template <class T>
LSM<T>::LSM() :
    m_head(nullptr)
{
}

template <class T>
LSM<T>::~LSM()
{
    clear();
}

template <class T>
void
LSM<T>::insert(const T &key,
               const T & /* Unused */)
{
    auto new_block = unused_block(1);
    new_block->put(key);

    while (m_head != nullptr && m_head->capacity() == new_block->capacity()) {
        const auto merged_block = unused_block(new_block->capacity() * 2);
        merged_block->merge(m_head, new_block);
        new_block = merged_block;

        m_head = m_head->m_next;
    }

    insert_between(new_block, nullptr, m_head);

#ifdef DEBUG
    printf("after insert(%d)\n", v);
    print();
#endif
}

template <class T>
bool
LSM<T>::delete_min(T &v)
{
    if (m_head == nullptr) {
        return false;
    }

    auto best = m_head;
    for (auto l = best->m_next; l != nullptr; l = l->m_next) {
        T lhs = T(), rhs = T();
        const bool l_has_elems __attribute__((unused)) = l->peek(lhs);
        const bool r_has_elems __attribute__((unused)) = best->peek(rhs);

        assert(l_has_elems);
        assert(r_has_elems);

        if (lhs < rhs) {
            best = l;
        }
    }

    const bool has_elems __attribute__((unused)) = best->pop(v);
    assert(has_elems);

    if (best->size() == 0 && m_head == best) {
        /* Unlink empty blocks of capacity 1 or 2. */

        assert(best->m_prev == nullptr);

        m_head = best->m_next;
        if (m_head != nullptr) {
            m_head->m_prev = nullptr;
        }

        best->set_unused();
    } else if (best->size() < best->capacity() / 2) {
        /* Merge with previous block. */

        /* Whether last block should be pruned. */
        bool prune_last = (best->m_next == nullptr);

        const auto shrunk_block = unused_block(best->capacity() / 2);
        shrunk_block->shrink(best);
        insert_between(shrunk_block, best->m_prev, best->m_next);
        best = shrunk_block;

        const auto lhs = best->m_prev;
        const auto rhs = best;

        if (lhs != nullptr && lhs->capacity() == rhs->capacity()) {
            prune_last = false;

            const auto merged_block = unused_block(lhs->capacity() * 2);
            merged_block->merge(lhs, rhs);
            insert_between(merged_block, lhs->m_prev, rhs->m_next);
        }

        if (prune_last) {
            prune_last_block();
        }
    }

#ifdef DEBUG
    printf("after delete_min(%d)\n", v);
    print();
#endif

    return true;
}

template <class T>
void
LSM<T>::clear()
{
    m_head = nullptr;

    for (auto &block_pair : m_blocks) {
        delete block_pair.first;
        delete block_pair.second;
    }

    m_blocks.clear();
}

template <class T>
void
LSM<T>::print() const
{
    for (auto block = m_head; block != nullptr; block = block->m_next) {
        block->print();
    }
}

template <class T>
LSMBlock<T> *
LSM<T>::unused_block(const int n)
{
    /* TODO: Ugly hack. */
    size_t i = 0, m = n;
    while (m > 1) {
        m >>= 1;
        i++;
    }

    if (i >= m_blocks.size()) {
        /* Alloc new blocks. */
        assert(m_blocks.size() == i);
        m_blocks.push_back({ new LSMBlock<T>(n), new LSMBlock<T>(n) });
    }

    if (m_blocks[i].first->used()) {
        assert(!m_blocks[i].second->used());
        return m_blocks[i].second;
    } else {
        return m_blocks[i].first;
    }
}

template <class T>
void
LSM<T>::prune_last_block()
{
    const int last = m_blocks.size() - 1;

    assert(m_blocks[last - 1].first->used() || m_blocks[last - 1].second->used());

    delete m_blocks[last].first;
    delete m_blocks[last].second;

    m_blocks.erase(m_blocks.begin() + last);
}

template <class T>
void
LSM<T>::insert_between(LSMBlock<T> *new_block,
                       LSMBlock<T> *prev,
                       LSMBlock<T> *next)
{
    if (prev != nullptr) {
        prev->m_next = new_block;
    } else {
        m_head = new_block;
    }

    if (next != nullptr) {
        next->m_prev = new_block;
    }

    new_block->m_prev = prev;
    new_block->m_next = next;
}

template class LSM<uint32_t>;

}
