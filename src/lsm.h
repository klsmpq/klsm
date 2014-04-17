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

private:
    LSMBlock<T> *m_head;
};

}

#endif /* __LSM_H */
