#ifndef __LSM_H
#define __LSM_H

#include <cstdint>
#include <vector>

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
    void print() const;

    /** Returns an unused block of size n == 2^i. */
    LSMBlock<T> *unused_block(const int n);
    void prune_last_block();

private:
    LSMBlock<T> *m_head; /**< The smallest block in the list. */

    /** A list of all allocated blocks. The two blocks of size 2^i
     *  are stored in m_blocks[i]. */
    std::vector<std::pair<LSMBlock<T> *, LSMBlock<T> *>> m_blocks;
};

}

#endif /* __LSM_H */
