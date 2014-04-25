#ifndef __LSM_H
#define __LSM_H

#include <cstdint>

namespace kpq
{

template <class T>
class LSMBlock;

template <class T>
class LSM
{
public:
    LSM();
    ~LSM();

    void insert(const T v);
    bool delete_min(T &v);
    void clear();

private:
    LSMBlock<T> *m_head; /**< The smallest block in the list. */
};

}

#endif /* __LSM_H */
