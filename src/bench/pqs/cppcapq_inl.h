#include "cppcapq.h"

extern "C" {
#include "capq.h"
#include "capq/gc/gc.h"
}



struct capq_t {
    char pad1[64 - sizeof(CAPQ *)];
    CAPQ *pq;
    char pad2[64];
};

static inline void
qdcatree_insert(CAPQ *pq,
                const uint32_t k,
                const uint32_t v)
{
    capq_put(pq,
             (unsigned long) k,
             (unsigned long) v);
}

template <bool remove_min_relax, bool put_relax, bool catree_adapt>
CPPCAPQ<remove_min_relax, put_relax, catree_adapt>::CPPCAPQ()
{
    _init_gc_subsystem();
    init_thread(1);
    m_q = new capq_t;
    m_q->pq = capq_new();
}

template <bool remove_min_relax, bool put_relax, bool catree_adapt>
CPPCAPQ<remove_min_relax, put_relax, catree_adapt>::~CPPCAPQ()
{

}

template <bool remove_min_relax, bool put_relax, bool catree_adapt>
void
CPPCAPQ<remove_min_relax, put_relax, catree_adapt>::init_thread(const size_t nthreads)
{
    (void)nthreads;
}

template <bool remove_min_relax, bool put_relax, bool catree_adapt>
void
CPPCAPQ<remove_min_relax, put_relax, catree_adapt>::insert(const uint32_t &key,
        const uint32_t &value)
{
    qdcatree_insert(m_q->pq, key, value);
}

template <bool remove_min_relax, bool put_relax, bool catree_adapt>
void
CPPCAPQ<remove_min_relax, put_relax, catree_adapt>::insert(const size_t &key,
        const size_t &value)
{
    capq_put_param(m_q->pq,
                   (unsigned long) key,
                   (unsigned long) value,
                   catree_adapt);
}

template <bool remove_min_relax, bool put_relax, bool catree_adapt>
void
CPPCAPQ<remove_min_relax, put_relax, catree_adapt>::flush_insert_cache()
{
}

template <bool remove_min_relax, bool put_relax, bool catree_adapt>
bool
CPPCAPQ<remove_min_relax, put_relax, catree_adapt>::delete_min(uint32_t &v)
{
    unsigned long key_write_back;
    v = (uint32_t)capq_remove_min(m_q->pq, &key_write_back);
    return key_write_back != ((unsigned long) - 1);
}

template <bool remove_min_relax, bool put_relax, bool catree_adapt>
bool
CPPCAPQ<remove_min_relax, put_relax, catree_adapt>::delete_min(size_t &k, size_t &v)
{
    unsigned long key_write_back;
    v = (size_t)capq_remove_min_param(m_q->pq, &key_write_back, remove_min_relax, put_relax,
                                      catree_adapt);
    k = (size_t)key_write_back;
    return key_write_back != ((unsigned long) - 1);
}
