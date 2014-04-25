#include "globallock.h"

namespace kpq
{

bool
GlobalLock::delete_min(uint32_t &v)
{
    std::lock_guard<std::mutex> g(m_mutex);

    if (m_q.empty()) {
        return false;
    }

    v = m_q.top();
    m_q.pop();

    return true;
}

void
GlobalLock::insert(const uint32_t v)
{
    std::lock_guard<std::mutex> g(m_mutex);

    m_q.push(v);
}

void
GlobalLock::clear()
{
    std::lock_guard<std::mutex> g(m_mutex);

    m_q = pq_t();
}

}
