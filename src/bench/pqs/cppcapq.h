#ifndef __CPPCAPQ_H
#define __CPPCAPQ_H

#include <cstddef>
#include <cstdint>

namespace kpqbench
{

struct capq_t;

template <bool remove_min_relax = true, bool put_relax = true, bool catree_adapt = true>
class CPPCAPQ
{
public:
    CPPCAPQ();
    virtual ~CPPCAPQ();

    void insert(const uint32_t &key, const uint32_t &value);
    void insert(const size_t &key, const size_t &value);
    void flush_insert_cache();
    bool delete_min(uint32_t &v);
    bool delete_min(size_t &k, size_t &v);

    void signal_waste();
    void signal_no_waste();

    void init_thread(const size_t nthreads);
    constexpr static bool supports_concurrency()
    {
        return true;
    }

private:
    capq_t *m_q;
};

#include "cppcapq_inl.h"
}

#endif /* __CPPCAPQ_H */
