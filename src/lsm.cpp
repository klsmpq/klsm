#include "lsm.h"

#include <algorithm>
#include <cassert>

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

        int l = 0, r = 0, dst = 0;

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
        for (int i = 0; i < m_capacity; i++) {
            T v = 0;
            if (m_elems[i].used()) {
                v = m_elems[i].peek();
            }
            printf("%d: { %d, %d }\n", i, v, m_elems[i].used());
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
LSM<T>::insert(const T v)
{
    auto new_block = unused_block(1);
    new_block->put(v);

    while (m_head != nullptr && m_head->capacity() == new_block->capacity()) {
        auto merged_block = unused_block(new_block->capacity() * 2);
        merged_block->merge(m_head, new_block);

        m_head = m_head->m_next;

        new_block = merged_block;
    }

    if (m_head != nullptr) {
        m_head->m_prev = new_block;
    }

    new_block->m_next = m_head;
    m_head = new_block;

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

    if (best == nullptr || best->size() == 0) {
        return false;
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

        auto shrunk_block = unused_block(best->capacity() / 2);

        shrunk_block->shrink(best);

        shrunk_block->m_prev = best->m_prev;
        shrunk_block->m_next = best->m_next;

        if (shrunk_block->m_prev != nullptr) {
            shrunk_block->m_prev->m_next = shrunk_block;
        } else {
            m_head = shrunk_block;
        }

        if (shrunk_block->m_next != nullptr) {
            shrunk_block->m_next->m_prev = shrunk_block;
        }

        best = shrunk_block;

        auto lhs = best->m_prev;
        auto rhs = best;

        if (lhs != nullptr && lhs->capacity() == rhs->capacity()) {
            auto merged_block = unused_block(lhs->capacity() * 2);
            merged_block->merge(lhs, rhs);


            /* Reconnect on the left. */

            if (lhs->m_prev != nullptr) {
                lhs->m_prev->m_next = merged_block;
            } else {
                m_head = merged_block;
            }

            /* Reconnect on the right. */

            if (rhs->m_next != nullptr) {
                rhs->m_next->m_prev = merged_block;
            }

            merged_block->m_prev = lhs->m_prev;
            merged_block->m_next = rhs->m_next;

            /* TODO: Free blocks if possible. */
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
    int i = 0, m = n;
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

template class LSM<uint32_t>;

}
