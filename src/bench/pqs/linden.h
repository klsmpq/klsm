#ifndef __LINDEN_H
#define __LINDEN_H

#include <cstddef>
#include <cstdint>

namespace kpqbench
{

struct linden_t;

class Linden
{
public:
    constexpr static int DEFAULT_OFFSET = 32;

    Linden(const int max_offset = DEFAULT_OFFSET);
    virtual ~Linden();

    void insert(const uint32_t &key, const uint32_t &value);
    void insert(const size_t &key, const size_t &value);
    bool delete_min(uint32_t &v);
    bool delete_min(size_t &k, size_t &v);

    void init_thread(const size_t) const { }
    constexpr static bool supports_concurrency() { return true; }

private:
    linden_t *m_q;
};

}

#endif /* __LINDEN_H */
