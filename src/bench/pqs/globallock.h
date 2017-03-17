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

#ifndef __GLOBALLOCK_H
#define __GLOBALLOCK_H

#include <cstring>
#include <mutex>

namespace kpqbench
{

/**
 * A custom heap implementation as used in klsm benchmarks. Never shrinks.
 */
template <class K, class V>
class GlobalLock
{
private:
    class entry_t
    {
    public:
        K key;
        V value;

        bool operator<(const entry_t &that) const
        {
            return this->key < that.key;
        }

        bool operator<=(const entry_t &that) const
        {
            return this->key <= that.key;
        }

        bool operator>=(const entry_t &that) const
        {
            return !operator<(that);
        }
    };

public:
    GlobalLock();
    virtual ~GlobalLock();

    void insert(const K &key, const V &value);
    bool delete_min(V &value);
    bool delete_min(K &key, V &value);
    void clear();

    void print() const;

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return true; }

private:
    void grow();
    void bubble_up(const size_t ix);
    void bubble_down(const size_t ix);

private:
    constexpr static size_t INITIAL_CAPACITY = 64;

    std::mutex m_mutex;

    entry_t *m_array;
    size_t m_length;
    size_t m_capacity;
};

template <class K, class V>
GlobalLock<K, V>::GlobalLock() :
    m_array(new entry_t[INITIAL_CAPACITY]),
    m_length(0),
    m_capacity(INITIAL_CAPACITY)
{
}

template <class K, class V>
GlobalLock<K, V>::~GlobalLock()
{
    delete[] m_array;
}

template <class K, class V>
bool
GlobalLock<K, V>::delete_min(K &key, V &value)
{
    std::lock_guard<std::mutex> g(m_mutex);

    if (m_length == 0) {
        return false;
    }

    key = m_array[0].key;
    value = m_array[0].value;
    m_array[0] = m_array[m_length - 1];
    bubble_down(0);
    m_length--;

    return true;
}

template <class K, class V>
bool
GlobalLock<K, V>::delete_min(V &value)
{
    K key;
    return delete_min(key, value);
}

template <class K, class V>
void
GlobalLock<K, V>::bubble_down(const size_t ix)
{
    size_t i = ix;
    size_t j = (i << 1) + 1;
    while (j < m_length) {
        const size_t k = j + 1;
        if (k < m_length && m_array[k] < m_array[j]) {
            j = k;
        }
        if (m_array[i] <= m_array[j]) {
            break;
        }
        std::swap(m_array[j], m_array[i]);
        i = j;
        j = (i << 1) + 1;
    }
}

template <class K, class V>
void
GlobalLock<K, V>::insert(const K &key,
                         const V &value)
{
    std::lock_guard<std::mutex> g(m_mutex);

    if (m_length == m_capacity) {
        grow();
    }

    m_array[m_length] = { key, value };
    bubble_up(m_length);
    m_length++;
}

template <class K, class V>
void
GlobalLock<K, V>::grow()
{
    const size_t new_capacity = m_capacity << 1;
    entry_t *new_array = new entry_t[new_capacity];

    memcpy(new_array, m_array, sizeof(m_array[0]) * m_capacity);
    delete[] m_array;

    m_array = new_array;
    m_capacity = new_capacity;
}

template <class K, class V>
void
GlobalLock<K, V>::bubble_up(const size_t ix)
{
    size_t i = ix;
    size_t j = (i - 1) >> 1;
    while (i > 0 && m_array[i] < m_array[j]) {
        std::swap(m_array[j], m_array[i]);
        i = j;
        j = (i - 1) >> 1;
    }
}

template <class K, class V>
void
GlobalLock<K, V>::clear()
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_length = 0;
}

template <class K, class V>
void GlobalLock<K, V>::print() const
{
    /* NOP */
}

}

#endif /* __GLOBALLOCK_H */
