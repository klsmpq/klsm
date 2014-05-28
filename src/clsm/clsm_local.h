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

#ifndef __CLSM_LOCAL_H
#define __CLSM_LOCAL_H

#include <atomic>
#include <random>

#include "block_storage.h"
#include "item.h"
#include "mm.h"
#include "thread_local_ptr.h"

namespace kpq
{

template <class K, class V>
class clsm;

template <class K, class V>
class clsm_local
{
public:
    clsm_local();
    virtual ~clsm_local();

    void insert(const K &key,
                const V &val);
    bool delete_min(clsm<K, V> *parent,
                    V &val);

    /** Attempts to copy items from a random other thread's local clsm,
     *  and returns the number of items copied. */
    int spy(clsm<K, V> *parent);

    void print() const;

private:
    /** The internal insertion, used both in the public insert() and in spy(). */
    void insert(item<K, V> *it,
                const version_t version);

    /** Iterates through local items and returns the best one found.
     *  In the process of finding the minimal item, unowned items
     *  in each block are removed and block merges are performed if possible.
     *  Used internally by delete_min(). */
    void peek(typename block<K, V>::peek_t &best);

    /**
     * Inserts new_block into the linked list of blocks, merging with
     * same size blocks until no two blocks in the list have the same size.
     */
    void merge_insert(block<K, V> *const new_block);

private:
    std::atomic<block<K, V> *> m_head; /**< The largest  block. */
    block<K, V>               *m_tail; /**< The smallest block. */

    block_storage<K, V> m_block_storage;
    item_allocator<item<K, V>, typename item<K, V>::reuse> m_item_allocator;

    std::mt19937 m_gen;
};

template <class K, class V>
clsm_local<K, V>::clsm_local() :
    m_head(nullptr),
    m_tail(nullptr)
{
}

template <class K, class V>
clsm_local<K, V>::~clsm_local()
{
    /* Blocks and items are managed by, respectively,
     * block_storage and item_allocator. */
}

template <class K, class V>
void
clsm_local<K, V>::insert(const K &key,
                         const V &val)
{
    item<K, V> *it = m_item_allocator.acquire();
    it->initialize(key, val);

    insert(it, it->version());
}

template <class K, class V>
void
clsm_local<K, V>::insert(item<K, V> *it,
                         const version_t version)
{
    /* If possible, simply append to the current tail block. */

    if (m_tail != nullptr && m_tail->last() < m_tail->capacity()) {
        K tail_key;
        if (m_tail->peek_tail(tail_key) && tail_key <= it->key()) {
            m_tail->insert_tail(it, version);
            return;
        }
    }

    /* Allocate the biggest possible array. This is an optimization
     * only. For correctness, it is enough to always allocate a new
     * array of capacity 1. */

    block<K, V> *new_block;
    if (m_tail == nullptr) {
        new_block = m_block_storage.get_largest_block();
    } else if (m_tail->power_of_2() > 0) {
        new_block = m_block_storage.get_block(m_tail->power_of_2() - 1);
    } else {
        new_block = m_block_storage.get_block(0);
    }

    new_block->insert(it, version);
    merge_insert(new_block);
}

template <class K, class V>
void
clsm_local<K, V>::merge_insert(block<K, V> *const new_block)
{
    block<K, V> *insert_block = new_block;
    block<K, V> *other_block  = m_tail;
    block<K, V> *delete_block = nullptr;

    /* Merge as long as the prev block is of the same size as the new block. */
    while (other_block != nullptr && insert_block->capacity() == other_block->capacity()) {
        /* Only merge into a larger block if both candidate blocks have enough elements to
         * justify the larger size. This change is necessary to avoid huge blocks containing
         * only a few elements (which actually happens with the 'alloc largest block on insert'
         * optimization. */
        const size_t merged_pow2 =
            (insert_block->size() + other_block->size() <= insert_block->capacity()) ?
            insert_block->power_of_2() : insert_block->power_of_2() + 1;
        auto merged_block = m_block_storage.get_block(merged_pow2);
        merged_block->merge(insert_block, other_block);

        insert_block->set_unused();
        insert_block = merged_block;
        delete_block = other_block;
        other_block  = other_block->m_prev;
    }

    /* Insert the new block into the list. */
    insert_block->m_prev = other_block;
    if (other_block != nullptr) {
        other_block->m_next.store(insert_block, std::memory_order_relaxed);
    } else {
        m_head.store(insert_block, std::memory_order_relaxed);
    }
    m_tail = insert_block;

    /* Remove merged blocks from the list. */
    while (delete_block != nullptr) {
        auto next_block = delete_block->m_next.load(std::memory_order_relaxed);
        delete_block->set_unused();
        delete_block = next_block;
    }
}

template <class K, class V>
bool
clsm_local<K, V>::delete_min(clsm<K, V> *parent,
                             V &val)
{
    typename block<K, V>::peek_t best;
    peek(best);

    if (best.m_item == nullptr && spy(parent) > 0) {
        peek(best); /* Retry once after a successful peek(). */
    }

    if (best.m_item == nullptr) {
        return false; /* We did our best, give up. */
    }

    return best.m_item->take(best.m_version, val);
}

template <class K, class V>
void
clsm_local<K, V>::peek(typename block<K, V>::peek_t &best)
{

    for (auto i = m_head.load(std::memory_order_relaxed);
            i != nullptr;
            i = i->m_next.load(std::memory_order_relaxed)) {

        auto candidate = i->peek();
        while (i->size() <= i->capacity() / 2) {

            /* Simply remove empty blocks. */
            if (i->capacity() == 1) {
                const auto next = i->m_next.load(std::memory_order_relaxed);
                if (i == m_tail) {
                    m_tail = i->m_prev;
                } else {
                    next->m_prev = i->m_prev;
                }

                if (i == m_head.load(std::memory_order_relaxed)) {
                    m_head = next;
                } else {
                    i->m_prev->m_next = next;
                }

                i->set_unused();

                return;
            }

            /* Shrink. */

            block<K, V> *new_block = m_block_storage.get_block(i->power_of_2() - 1);
            new_block->copy(i);

            new_block->m_next.store(i->m_next.load(std::memory_order_relaxed),
                                    std::memory_order_relaxed);
            new_block->m_prev = i->m_prev;

            /* Merge. TODO: Shrink-Merge optimization. */

            auto next = new_block->m_next.load(std::memory_order_relaxed);
            if (next != nullptr && new_block->capacity() == next->capacity()) {
                auto merged_block = m_block_storage.get_block(new_block->power_of_2() + 1);
                merged_block->merge(new_block, next);

                merged_block->m_next = next->m_next.load(std::memory_order_relaxed);
                merged_block->m_prev = new_block->m_prev;

                new_block->set_unused();
                new_block = merged_block;
            }

            /* Insert new block. */

            next = new_block->m_next.load(std::memory_order_relaxed);

            if (next == nullptr) {
                m_tail = new_block;
            } else {
                next->m_prev = new_block;
            }

            if (new_block->m_prev == nullptr) {
                m_head.store(new_block, std::memory_order_relaxed);
            } else {
                new_block->m_prev->m_next.store(new_block, std::memory_order_relaxed);
            }

            /* Bookkeeping and rerun peek(). */

            for (auto j = i; j != nullptr && j != next;) {
                const auto k = j->m_next.load(std::memory_order_relaxed);
                j->set_unused();
                j = k;
            }
            i = new_block;

            candidate = i->peek();
        }

        if (best.m_item == nullptr ||
                (candidate.m_item != nullptr && candidate.m_key < best.m_key)) {
            best = candidate;
        }
    }
}

template <class K, class V>
int
clsm_local<K, V>::spy(clsm<K, V> *parent)
{
    int num_spied = 0;

    const size_t num_threads    = parent->m_local.num_threads();
    const size_t current_thread = parent->m_local.current_thread();

    if (num_threads < 2) {
        return num_spied;
    }

    /* All except current thread, therefore n - 2. */
    std::uniform_int_distribution<> rand_int(0, num_threads - 2);
    size_t victim_id = rand_int(m_gen);
    if (victim_id >= current_thread) {
        victim_id++;
    }

    auto victim = parent->m_local.get(victim_id);
    for (auto i = victim->m_head.load(std::memory_order_relaxed);
            i != nullptr;
            i = i->m_next.load(std::memory_order_relaxed)) {

        auto it = i->iterator();
        for (auto p = it.next(); p.m_item != nullptr; p = it.next()) {
            insert(p.m_item, p.m_version);
            num_spied++;
        }
    }

    return num_spied;
}

template <class K, class V>
void
clsm_local<K, V>::print() const
{
    m_block_storage.print();
}

}

#endif /* __CLSM_LOCAL_H */
