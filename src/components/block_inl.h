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
    peek_t p = peek_t::EMPTY();

    while (m_next < m_last) {
        const auto &item = m_block_items[m_next++];

        p.m_version = item.m_version;
        p.m_item    = item.m_item;
        p.m_key     = item.m_item->key();

        if (item.m_version != p.m_version) {
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
    m_block_items(new block_item[m_capacity]),
    m_used(false)
{
}

template <class K, class V>
block<K, V>::~block()
{
    delete[] m_block_items;
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

    m_block_items[m_last].m_item  = it;
    m_block_items[m_last].m_version = version;
    m_block_items[m_last].m_key = it->key();

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
    constexpr static int NONE = 0;
    constexpr static int L = 1;
    constexpr static int R = 2;

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
    int last_updated = NONE;
    K lhs_key, rhs_key;
    lhs_key = rhs_key = std::numeric_limits<K>::min();

    while (l < lhs->m_last && r < rhs->m_last) {
        if (last_updated != L) {
            rhs_key = rhs->m_block_items[r].m_key;
        }

        if (last_updated != R) {
            lhs_key = lhs->m_block_items[l].m_key;
        }

        if (lhs_key < rhs_key) {
            m_block_items[dst++] = lhs->m_block_items[l];
            l++;
            last_updated = L;
        } else {
            m_block_items[dst++] = rhs->m_block_items[r];
            r++;
            last_updated = R;
        }
    }

    while (l < lhs->m_last) {
        auto &lelem = lhs->m_block_items[l];
        m_block_items[dst++] = lelem;
        l++;
    }

    while (r < rhs->m_last) {
        auto &relem = rhs->m_block_items[r];
        m_block_items[dst++] = relem;
        r++;
    }

    const size_t size = lhs->m_last + rhs->m_last - lhs_first - rhs_first;
    const size_t skipped_prunes = std::max(lhs->m_skipped_prunes, rhs->m_skipped_prunes);
    if (skipped_prunes > MAX_SKIPPED_PRUNES) {
        size_t dst = 0;
        for (size_t src = 0; src < size; src++) {
            if (!item_owned(m_block_items[src])) {
                continue;
            } else if (src != dst) {
                m_block_items[dst] = m_block_items[src];
            }
            dst++;
        }

        m_last = dst;
        m_skipped_prunes = 0;
    } else {
        m_last = size;
        m_skipped_prunes = skipped_prunes + 1;
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
        auto &elem = that->m_block_items[i];
        if (!item_owned(elem)) {
            continue;
        }

        m_block_items[dst++] = elem;
    }

    m_last = dst;
}

template <class K, class V>
typename block<K, V>::peek_t
block<K, V>::peek()
{
    size_t ix;
    return peek(ix);
}

template <class K, class V>
typename block<K, V>::peek_t
block<K, V>::peek(size_t &ix)
{
    const bool called_by_owner = (tid() == m_owner_tid);
    peek_t p = peek_t::EMPTY();
    for (ix = m_first; ix < m_last; ix++) {
        p.m_item    = m_block_items[ix].m_item;
        p.m_key     = m_block_items[ix].m_key;
        p.m_version = m_block_items[ix].m_version;

        if (item_owned(m_block_items[ix])) {
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
            m_first = ix + 1;
        }
    }

    p.m_item = nullptr;
    return p;
}

template <class K, class V>
const typename block<K, V>::block_item *
block<K, V>::peek_nth(const size_t n) const
{
    assert(n < m_capacity);
    return &m_block_items[n];
}

template <class K, class V>
bool
block<K, V>::peek_tail(K &key)
{
    for (int i = (int)m_last - 1; i >= (int)m_first; i--) {
        key = m_block_items[i].m_key;
        if (item_owned(m_block_items[i])) {
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

    it.m_block_items = m_block_items;
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
block<K, V>::item_owned(const block_item &block_item)
{
    const auto *item_ptr = block_item.m_item;
    return (item_ptr != nullptr && item_ptr->version() == block_item.m_version);
}
