#ifndef __LINDEN_H
#define __LINDEN_H

extern "C" {
#include "linden/prioq.h"
}

class Linden
{
public:
    Linden(const int max_offset);
    virtual ~Linden();

    void insert(const uint32_t v);
    bool delete_min(uint32_t &v);

private:
    pq_t *m_q;
};
#endif /* __LINDEN_H */
