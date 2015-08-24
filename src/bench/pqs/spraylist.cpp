#include "spraylist.h"

extern "C" {
#include "spraylist/include/random.h"
#include "spraylist/intset.h"
#include "spraylist/linden.h"
#include "spraylist/pqueue.h"
#include "ssalloc.h"
}

__thread unsigned long *seeds;

namespace kpqbench {

constexpr unsigned int INITIAL_SIZE = 1 << 15;

/** See documentation of --elasticity in spraylist/test.c. */
#define READ_ADD_REM_ELASTIC_TX (4)

static __thread bool initialized = false;
static __thread thread_data_t *d;


spraylist::spraylist()
{
    init_thread(1);
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
        ssalloc_init();
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
    sl_add(m_q, v, READ_ADD_REM_ELASTIC_TX);
}

bool
spraylist::delete_min(uint32_t &v)
{
    const int ret = spray_delete_min(m_q, &v, d);
    return (ret == 1);
}

}
