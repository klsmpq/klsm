#include "lsm.h"

#include <algorithm>

namespace kpq
{

template <class T>
struct LSMElem {
    LSMElem() :
        m_deleted(false)
    {
    }

    bool operator<(const LSMElem<T> &that) const
    {
        return this->m_elem < that.m_elem;
    }

    T m_elem;
    bool m_deleted;
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
            if (!m_elems[i].m_deleted) {
                return m_elems[i].m_elem;
            }
        }

        return 0; /* FIXME */
    }

    T pop()
    {
        for (uint32_t i = 0; i < m_n; i++) {
            if (!m_elems[i].m_deleted) {
                m_elems[i].m_deleted = true;
                return m_elems[i].m_elem;
            }
        }

        return 0; /* FIXME */
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

    if (best == nullptr) {
        return false;
    }

    v = best->pop();

    /* TODO: Resizing. */

    return true;
}

template class LSM<uint32_t>;

}
