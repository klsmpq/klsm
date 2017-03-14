#include "linden.h"

extern "C" {
#include "spraylist_linden/gc/gc.h"
#include "spraylist_linden/linden.h"
}

namespace kpqbench
{

struct linden_t {
    pq_t *pq;
};

static inline void
linden_insert(pq_t *pq,
              const uint32_t k,
              const uint32_t v)
{
    insert(pq, k, v);
}

Linden::Linden(const int max_offset)
{
    _init_gc_subsystem();
    m_q = new linden_t;
    m_q->pq = pq_init(max_offset);
}

Linden::~Linden()
{
    pq_destroy(m_q->pq);
    delete m_q;
    _destroy_gc_subsystem();
}

void
Linden::insert(const uint32_t &key,
               const uint32_t &value)
{
    /*Add 1 to key to support 0 keys*/
    linden_insert(m_q->pq, key+1, value);
}

void
Linden::insert(const size_t &key,
               const size_t &value)
{
    /*Add 1 to key to support 0 keys*/
    linden_insert(m_q->pq, key+1, value);
}

bool
Linden::delete_min(size_t &k, size_t &v)
{
    unsigned long k_ret;
    v = deletemin_key(m_q->pq, &k_ret);
    k = k_ret -1;
    return k_ret != -1;
}

bool
Linden::delete_min(uint32_t &v)
{
    size_t k_ret;
    size_t v_ret;
    bool ret = delete_min(k_ret, v_ret);
    v = (uint32_t)v_ret;
    return ret;
}

}

