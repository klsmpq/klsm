#ifndef __LINDEN_H
#define __LINDEN_H

#include <cstdint>

namespace kpq
{

struct linden_t;

class Linden
{
public:
    Linden(const int max_offset);
    virtual ~Linden();

    void insert(const uint32_t v);
    bool delete_min(uint32_t &v);

    constexpr static int DEFAULT_OFFSET = 32;

private:
    linden_t *m_q;
};

}

#endif /* __LINDEN_H */
