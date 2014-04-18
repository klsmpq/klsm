#include "lsm.h"

#include <algorithm>

namespace kpq
{

template <class T>
struct LSMElem {
    LSMElem() :
        m_used(false)
    {
    }

    bool operator<(const LSMElem<T> &that) const
    {
        if (!this->m_used && that.m_used) {
            return false;
        }

        if (this->m_used && !that.m_used) {
            return true;
        }

        return this->m_elem < that.m_elem;
    }

    T m_elem;
    bool m_used;
};

template <class T>
struct LSMBlock {
    LSMBlock(const uint32_t n) :
        m_elems(new LSMElem<T>[n]),
        m_n(n),
        m_next(nullptr)
    {
    }

    ~LSMBlock()
    {
        delete[] m_elems;
    }

    T peek() const
    {
        for (uint32_t i = 0; i < m_n; i++) {
            if (m_elems[i].m_used) {
                return m_elems[i].m_elem;
            }
        }

        return 0; /* FIXME */
    }

    T pop()
    {
        for (uint32_t i = 0; i < m_n; i++) {
            if (m_elems[i].m_used) {
                m_elems[i].m_used = false;
                return m_elems[i].m_elem;
            }
        }

        return 0; /* FIXME */
    }

    void shrink()
    {
        auto new_elems = new LSMElem<T>[m_n / 2];

        uint32_t dst = 0;
        for (uint32_t src = 0; src < m_n; src++) {
            if (m_elems[src].m_used) {
                new_elems[dst++] = m_elems[src];
            }
        }

        delete m_elems;
        m_elems = new_elems;

        m_n /= 2;
    }

    size_t size() const
    {
        size_t s = 0;
        for (uint32_t i = 0; i < m_n; i++) {
            if (m_elems[i].m_used) {
                s++;
            }
        }
        return s;
    }

    LSMElem<T> *m_elems;
    uint32_t m_n;
    LSMBlock<T> *m_next;
};

template <class T>
LSM<T>::LSM() :
    m_head(nullptr)
{
}

template <class T>
LSM<T>::~LSM()
{
    while (m_head != nullptr) {
        LSMBlock<T> *next = m_head->m_next;
        delete m_head;
        m_head = next;
    }
}

template <class T>
void
LSM<T>::insert(const T v)
{
    auto new_block = new LSMBlock<T>(1);
    new_block->m_elems[0].m_elem = v;
    new_block->m_elems[0].m_used = true;

    while (m_head != nullptr && m_head->m_n == new_block->m_n) {
        auto merged_block = new LSMBlock<T>(new_block->m_n * 2);

        std::merge(m_head->m_elems, m_head->m_elems + m_head->m_n,
                   new_block->m_elems, new_block->m_elems + new_block->m_n,
                   merged_block->m_elems);

        auto old_head = m_head;

        m_head = m_head->m_next;

        delete old_head;
        delete new_block;

        new_block = merged_block;
    }

    new_block->m_next = m_head;
    m_head = new_block;
}

template <class T>
bool
LSM<T>::delete_min(T &v)
{
    auto best = m_head;

    for (auto l = best->m_next; l != nullptr; l = l->m_next) {
        if (l->peek() < best->peek()) {
            best = l;
        }
    }

    if (best == nullptr || best->size() == 0) {
        return false;
    }

    v = best->pop();

    if (best->size() < best->m_n / 2) {
        best->shrink();

        auto lhs = best;
        auto rhs = best->m_next;

        if (rhs != nullptr && lhs->m_n == rhs->m_n) {
            auto merged_block = new LSMBlock<T>(lhs->m_n * 2);

            std::merge(lhs->m_elems, lhs->m_elems + lhs->m_n,
                       rhs->m_elems, rhs->m_elems + rhs->m_n,
                       merged_block->m_elems);

            /* Cheat here (since we have a singly linked list) and overwrite best struct. */

            std::swap(best->m_elems, merged_block->m_elems);
            std::swap(best->m_n, merged_block->m_n);

            best->m_next = rhs->m_next;

            delete rhs;
        }
    }

    return true;
}

template class LSM<uint32_t>;

}
