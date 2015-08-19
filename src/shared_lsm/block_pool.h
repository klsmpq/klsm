/*
 *  Copyright 2015 Jakob Gruber
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

#ifndef __BLOCK_POOL_H
#define __BLOCK_POOL_H

#include "components/block.h"

#include <algorithm>

namespace kpq {

template <class K, class V>
class block_pool {
private:
    static constexpr int MAX_POWER_OF_2   = 48;
    static constexpr int BLOCKS_PER_LEVEL = 4;
    static constexpr int BLOCKS_IN_POOL   = MAX_POWER_OF_2 * BLOCKS_PER_LEVEL;

    enum block_status {
        BLOCK_FREE,
        BLOCK_LOCAL,
        BLOCK_GLOBAL,
    };

public:
    block_pool() :
        m_pool { nullptr },
        m_status { BLOCK_FREE },
        m_version { 0 },
        m_local_ixs_size { 0 }
    {
    }

    virtual ~block_pool() {
        for (int i = 0; i < BLOCKS_IN_POOL; i++) {
            delete m_pool[i];
        }
    }

    /** Given the current version of the global array, returns a block of capacity 2^i. */
    block<K, V> *get_block(const size_t i)
    {
        /* Find the maximum version of globally allocated blocks.
         * It is safe to reallocate any but the most recent global block.
         * We could optimize this loop out in the future. */
        int max_global_version = -1;
        for (int j = ix(i); j < ix(i + 1); j++) {
            if (m_status[j] == BLOCK_GLOBAL) {
                max_global_version = std::max(max_global_version, (int)m_version[j]);
            }
        }

        for (int j = ix(i); j < ix(i + 1); j++) {
            if (m_status[j] == BLOCK_FREE
                    || (m_status[j] == BLOCK_GLOBAL
                        && (int)m_version[j] != max_global_version)) {
                m_status[j] = BLOCK_LOCAL;
                m_local_ixs[m_local_ixs_size++] = j;
                if (m_pool[j] == nullptr) {
                    /* Lazy block creation.
                     * 'used' is not needed for the shared lsm. Figure out a way for both
                     * mechanisms to interact when integrating shared & dist lsm's. */
                    m_pool[j] = new block<K, V>(i);
                    m_pool[j]->set_used();
                } else {
                    m_pool[j]->clear();
                }
                return m_pool[j];
            }
        }
        assert(false), "A free block should always exist";
        return nullptr;
    }

    void publish(block<K, V> **blocks,
                 const size_t nblocks,
                 const version_t version)
    {
        for (size_t i = 0; i < nblocks; i++) {
            auto block = blocks[i];
            if (block == nullptr) {
                continue;
            }

            /* TODO: Optimize out find(). */
            int ix = find(block);
            if (ix != -1) {
                m_status[ix] = BLOCK_GLOBAL;
                m_version[ix] = version;
            }
        }
    }

    void free_local()
    {
        free_local_except(nullptr);
    }

    void free_local_except(block<K, V> *that)
    {
        int that_ix = -1;
        for (size_t i = 0; i < m_local_ixs_size; i++) {
            int ix = m_local_ixs[i];
            if (m_pool[ix] == that) {
                assert(that_ix == -1);
                that_ix = ix;
            } else if (m_status[ix] == BLOCK_LOCAL) {
                m_status[ix] = BLOCK_FREE;
            }
        }

        m_local_ixs_size = 0;
        if (that_ix != -1) {
            m_local_ixs[m_local_ixs_size++] = that_ix;
        }
    }

    bool contains(const block<K, V> *block) const {
        return (find(block) != -1);
    }

private:
    int find(const block<K, V> *block) const
    {
        const int pow = block->power_of_2();
        for (int j = ix(pow); j < ix(pow + 1); j++) {
            if (m_pool[j] == block) {
                return j;
            }
        }
        return -1;
    }

    static constexpr int ix(const size_t i)
    {
        return BLOCKS_PER_LEVEL * i;
    }

private:
    block<K, V> *m_pool[BLOCKS_IN_POOL];
    block_status m_status[BLOCKS_IN_POOL];
    version_t    m_version[BLOCKS_IN_POOL];

    /** Stores indexes allocated during the current insert iteration.
     *  Valid stati are LOCAL and GLOBAL (if publish() has been called,
     *  i.e. the current insert has been successfully completed).
     *  All other indexes are guaranteed not to be set to LOCAL. */
    size_t m_local_ixs_size;
    int m_local_ixs[BLOCKS_IN_POOL];
};

}

#endif /* __BLOCK_POOL_H */
