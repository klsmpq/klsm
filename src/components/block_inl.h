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

template <class K, class V>
typename block<K, V>::peek_t
block<K, V>::spying_iterator::next()
{
    peek_t p;

    while (m_next < m_last) {
        const auto &item = m_item_pairs[m_next++];

        p.m_version = item.second;
        p.m_item    = item.first;
        p.m_key     = item.first->key();

        if (item.second != p.m_version) {
            p.m_item = nullptr;
        } else {
            break;
        }
    }

    return p;
}

template <class K, class V>
block<K, V>::block(const size_t power_of_2) :
    m_next(nullptr),
    m_prev(nullptr),
    m_first(0),
    m_last(0),
    m_power_of_2(power_of_2),
    m_capacity(1 << power_of_2),
    m_owner_tid(tid()),
    m_item_pairs(new item_pair_t[m_capacity]),
    m_used(false)
{
}

template <class K, class V>
block<K, V>::~block()
{
    delete[] m_item_pairs;
}

template <class K, class V>
void
block<K, V>::insert(item<K, V> *it,
                    const version_t version)
{
    assert(m_first == 0);
    assert(m_last == 0);

    insert_tail(it, version);
}

template <class K, class V>
void
block<K, V>::insert_tail(item<K, V> *it,
                         const version_t version)
{
    assert(m_used);
    assert(m_last < m_capacity);

    m_item_pairs[m_last].first  = it;
    m_item_pairs[m_last].second = version;

    m_last++;
}

template <class K, class V>
void
block<K, V>::merge(const block<K, V> *lhs,
                   const block<K, V> *rhs)
{
    merge(lhs, lhs->m_first, rhs, rhs->m_first);
}

template <class K, class V>
void
block<K, V>::merge(const block<K, V> *lhs,
                   const size_t lhs_first,
                   const block<K, V> *rhs,
                   const size_t rhs_first)
{
    /* The following assertions are no longer valid since we now sometimes merge blocks
     * of different capacities.
    assert(m_power_of_2 == lhs->power_of_2() + 1);
    assert(lhs->power_of_2() == rhs->power_of_2());
     */
    assert(m_used);
    assert(m_first == 0);
    assert(m_last == 0);

    /* Merge. */

    size_t l = lhs_first, r = rhs_first, dst = 0;

    while (l < lhs->m_last && r < rhs->m_last) {
        auto &lelem = lhs->m_item_pairs[l];
        auto &relem = rhs->m_item_pairs[r];

        if (!item_owned(lelem)) {
            l++;
            continue;
        }

        if (!item_owned(relem)) {
            r++;
            continue;
        }

        if (lelem.first->key() < relem.first->key()) {
            m_item_pairs[dst++] = lelem;
            l++;
        } else {
            m_item_pairs[dst++] = relem;
            r++;
        }
    }

    while (l < lhs->m_last) {
        auto &lelem = lhs->m_item_pairs[l];
        if (!item_owned(lelem)) {
            l++;
            continue;
        }
        m_item_pairs[dst++] = lelem;
        l++;
    }

    while (r < rhs->m_last) {
        auto &relem = rhs->m_item_pairs[r];
        if (!item_owned(relem)) {
            r++;
            continue;
        }
        m_item_pairs[dst++] = relem;
        r++;
    }

    m_last = dst;

    assert(m_last <= m_capacity);
}

template <class K, class V>
void
block<K, V>::copy(const block<K, V> *that)
{
    assert(m_used);
    assert(m_first == 0);
    assert(m_last == 0);

    size_t dst = 0;
    for (size_t i = that->m_first; i < that->m_last; i++) {
        if (dst >= m_capacity) {
            /* Can happen when a block is reused during the shared lsm's
             * insert(). In that case simply abort, the version compare exchange
             * will fail once this thread tries to publish it's array. */
            return;
        }
        auto &elem = that->m_item_pairs[i];
        if (!item_owned(elem)) {
            continue;
        }

        m_item_pairs[dst++] = elem;
    }

    m_last = dst;
}

template <class K, class V>
typename block<K, V>::peek_t
block<K, V>::peek()
{
    const bool called_by_owner = (tid() == m_owner_tid);
    peek_t p;
    for (size_t i = m_first; i < m_last; i++) {
        p.m_item    = m_item_pairs[i].first;
        p.m_key     = m_item_pairs[i].first->key();
        p.m_index   = i;
        p.m_version = m_item_pairs[i].second;

        if (item_owned(m_item_pairs[i])) {
            return p;
        }

        /* Move initial sequence of unowned item references out
         * of active scope. Only the owner thread may modify m_first
         * in order to avoid conflicts through reused blocks.
         * TODO: This is not acceptable long-term, since it prevents us
         * from giving any decent bounds on delete-min: if peek is never
         * called by the owning thread, we potentially need to check every
         * item in the block before returning the last one. */
        if (called_by_owner) {
            m_first = i + 1;
        }
    }

    p.m_item = nullptr;
    return p;
}

template <class K, class V>
typename block<K, V>::peek_t
block<K, V>::peek_nth(const size_t n)
{
    assert(n < m_capacity);

    peek_t p;

    if (m_item_pairs[n].first == nullptr) {
        return p;
    } else if (!item_owned(m_item_pairs[n])) {
        return p;
    }

    p.m_key     = m_item_pairs[n].first->key();
    p.m_item    = m_item_pairs[n].first;
    p.m_index   = n;
    p.m_version = m_item_pairs[n].second;

    return p;
}

template <class K, class V>
bool
block<K, V>::peek_tail(K &key)
{
    for (int i = (int)m_last - 1; i >= (int)m_first; i--) {
        key = m_item_pairs[i].first->key();
        if (item_owned(m_item_pairs[i])) {
            return true;
        }
        /* Last item is not owned by us anymore, clean it up. */
        m_last--;
    }
    return false;
}

template <class K, class V>
typename block<K, V>::spying_iterator
block<K, V>::iterator()
{
    typename block<K, V>::spying_iterator it;

    it.m_item_pairs = m_item_pairs;
    it.m_next = m_first;
    it.m_last = m_last;

    return it;
}

template <class K, class V>
size_t
block<K, V>::first() const
{
    return m_first;
}

template <class K, class V>
size_t
block<K, V>::last() const
{
    return m_last;
}

template <class K, class V>
size_t
block<K, V>::size() const
{
    return m_last - m_first;
}

template <class K, class V>
size_t
block<K, V>::power_of_2() const
{
    return m_power_of_2;
}

template <class K, class V>
size_t
block<K, V>::capacity() const
{
    return m_capacity;
}

template <class K, class V>
bool
block<K, V>::used() const
{
    return m_used;
}

template <class K, class V>
void
block<K, V>::set_unused()
{
    assert(m_used);
    m_used  = false;

    clear();
}

template <class K, class V>
void
block<K, V>::clear()
{
    m_first = 0;
    m_last  = 0;

    m_next.store(nullptr, std::memory_order_relaxed);
    m_prev = nullptr;
}

template <class K, class V>
void
block<K, V>::set_used()
{
    assert(!m_used);
    m_used = true;
}

template <class K, class V>
bool
block<K, V>::item_owned(const item_pair_t &item_pair)
{
    return (item_pair.first != nullptr && item_pair.first->version() == item_pair.second);
}
