#include "spraylist.h"

extern "C" {
#include "spraylist_linden/include/random.h"
#include "spraylist_linden/intset.h"
#include "spraylist_linden/linden.h"
#include "spraylist_linden/pqueue.h"
#include "ssalloc.h"
}

__thread unsigned long *seeds;

namespace kpqbench {

constexpr unsigned int INITIAL_SIZE = 1000000;

/** See documentation of --elasticity in spraylist/test.c. */
#define READ_ADD_REM_ELASTIC_TX (4)

static __thread bool initialized = false;
static __thread thread_data_t *d;


spraylist::spraylist(const size_t nthreads)
{
    init_thread(nthreads);
    *levelmax = floor_log_2(INITIAL_SIZE);
    m_q = sl_set_new();
}

spraylist::~spraylist()
{
    sl_set_delete(m_q);
    delete d;
}

void
spraylist::init_thread(const size_t nthreads)
{
    if (!initialized) {
        ssalloc_init(nthreads);
        seeds = seed_rand();

        d = new thread_data_t;
        d->seed = rand();
        d->seed2 = rand();

        initialized = true;
    }

    d->nb_threads = nthreads;
}

void
spraylist::insert(const uint32_t &k,
                  const uint32_t &v)
{
    sl_add_val(m_q, k, v, TRANSACTIONAL);
}

void
spraylist::insert(const size_t &k,
                  const size_t &v)
{
    sl_add_val(m_q, k, v, TRANSACTIONAL);
}

bool
spraylist::delete_min(size_t &k, size_t &v)
{
    unsigned long k_ret;
    unsigned long v_ret;
    int ret;
    do {
        ret = spray_delete_min_key(m_q, &k_ret, &v_ret, d);
    } while(ret == 0 && k_ret != -1);
    k = k_ret;
    v = v_ret;
    return k_ret != -1;
}

bool
spraylist::delete_min(uint32_t &v)
{
    size_t k_ret;
    size_t v_ret;
    const bool ret = delete_min(k_ret, v_ret);
    v = (uint32_t)v_ret;
    return ret;
}    

}
