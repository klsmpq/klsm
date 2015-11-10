#include "itree.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace kpqbench {

/* itree defines. */

#define INDENT (2)

/* itree definitions. */

itree::itree() :
    m_root(nullptr),
    m_size(0)
{
}

itree::~itree()
{
    itree_free(m_root);
}

int
itree::insert(const elem_t &index)
{
    uint64_t holes = 0;
    int ret = _itree_insert(index, &m_root, &holes);
    if (ret == 0) {
        m_size++;
    }
    return ret;
}

int
itree::erase(const elem_t &index,
             uint64_t *rank)
{
    int ret = _itree_erase(index, &m_root, rank);
    if (ret == 0) {
        *rank = m_size - *rank;
        m_size--;
    }
    return 0;
}

void
itree::print()
{
    _itree_print(m_root, 0);
}

void
itree::_itree_print(const itree_t *root,
                    uint8_t level)
{
    if (root == NULL) {
        return;
    }

    printf("%*s{ k: %llu, v: %llu, h: %d }\n", level * INDENT, "",
           (long long unsigned)root->k.key,
           (long long unsigned)root->v,
           root->h);

    _itree_print(root->l, level + 1);
    _itree_print(root->r, level + 1);
}

int
itree::_itree_new_node(const elem_t &index,
                       itree_t **root)
{
    itree_t *droot = (itree_t *)calloc(1, sizeof(itree_t));
    if (droot == NULL) {
        perror("calloc");
        return -1;
    }
    droot->k = index;
    *root = droot;
    return 0;
}

inline int8_t
itree::_itree_height(const itree_t *node)
{
    return (node == NULL) ? -1 : node->h;
}

inline void
itree::_itree_set_height(itree_t *node)
{
    node->h = std::max(_itree_height(node->l), _itree_height(node->r)) + 1;
}

/**
 * Returns the count of elements in the tree.
 */
inline uint64_t
itree::_itree_count(const itree_t *root)
{
    if (root == NULL) {
        return 0;
    }
    return 1 + root->v + _itree_count(root->l);
}

/**
 * Rebalances the subtree.
 *
 * Precondition:
 *  * root != NULL.
 *  * The subtrees root->l and root->r are balanced.
 *
 * Postcondition:
 *  * The subtree is balanced but otherwise unchanged.
 */
void
itree::_itree_rebalance(itree_t **root)
{
    itree_t *droot = *root;

    const int lh = _itree_height(droot->l);
    const int rh = _itree_height(droot->r);

    if (abs(lh - rh) < 2) {
        /* No rebalancing required. */
        return;
    }

    if (lh < rh) {
        itree_t *r = droot->r;

        const int rlh = _itree_height(r->l);
        const int rrh = _itree_height(r->r);

        /* Right-left case. */
        if (rlh > rrh) {
            droot->r = r->l;
            r->l = droot->r->r;
            droot->r->r = r;

            droot->r->v += r->v + 1;

            _itree_set_height(r);

            r = droot->r;
        }

        /* Right-right case. */

        droot->r = r->l;
        r->l = droot;
        *root = r;

        droot->v = _itree_count(droot->r);

        _itree_set_height(droot);
        _itree_set_height(r);
    } else {
        itree_t *l = droot->l;

        const int llh = _itree_height(l->l);
        const int lrh = _itree_height(l->r);

        /* Left-right case. */
        if (lrh > llh) {
            droot->l = l->r;
            l->r = droot->l->l;
            droot->l->l = l;

            l->v = _itree_count(l->r);

            _itree_set_height(l);

            l = droot->l;
        }

        /* Left-left case. */

        droot->l = l->r;
        l->r = droot;
        *root = l;

        l->v += droot->v + 1;

        _itree_set_height(droot);
        _itree_set_height(l);
    }
}

int
itree::_itree_descend_l(const elem_t &index,
                        itree_t **root,
                        uint64_t *holes)
{
    itree_t *droot = *root;

    *holes += droot->v + 1;

    int ret = _itree_insert(index, &droot->l, holes);
    if (ret != 0) { return ret; }

    return 0;
}

int
itree::_itree_descend_r(const elem_t &index,
                        itree_t **root,
                        uint64_t *holes)
{
    itree_t *droot = *root;

    /* Index was added as a new descendant node. */
    droot->v++;

    int ret = _itree_insert(index, &droot->r, holes);
    if (ret != 0) { return ret; }

    return 0;
}

/**
 * The workhorse for itree_insert.
 * Util keeps track of several internal variables needed for merging nodes.
 */
int
itree::_itree_insert(const elem_t &index,
                     itree_t **root,
                     uint64_t *holes)
{
    itree_t *droot = *root;
    int ret = 0;

    /* New node. */
    if (droot == NULL) {
        return _itree_new_node(index, root);
    }

    /* Descend into left or right subtree. */
    if (droot->k > index) {
        if ((ret = _itree_descend_l(index, root, holes)) != 0) {
            return ret;
        }
    } else if (index > droot->k) {
        if ((ret = _itree_descend_r(index, root, holes)) != 0) {
            return ret;
        }
    } else {
        fprintf(stderr, "Index %u is already in tree\n", index.key);
        return -1;
    }

    /* Rebalance if necessary. */
    _itree_rebalance(root);

    _itree_set_height(droot);

    return ret;
}

int
itree::_itree_erase(const elem_t &index,
                    itree_t **root,
                    uint64_t *rank)
{

    itree_t *droot = *root;
    int ret = 0;

    if (droot == nullptr) {
        fprintf(stderr, "Key %u not found\n", index.key);
        return -1;
    } else if (droot->k > index) {
        if ((ret = _itree_erase(index, &droot->l, rank)) != 0) {
            return ret;
        }
        *rank += droot->v + 1;
    } else if (index > droot->k) {
        if ((ret = _itree_erase(index, &droot->r, rank)) != 0) {
            return ret;
        }
        droot->v--;
    } else { /* index == k */
        *rank = droot->v;

        if (droot->l == nullptr && droot->r == nullptr) {
            *root = nullptr;
            free(droot);
        } else if (droot->l == nullptr) {
            *root = droot->r;
            free(droot);
        } else if (droot->r == nullptr) {
            *root = droot->l;
            free(droot);
        } else {
            /* Two child nodes exist. Replace the current node with its successor,
             * and remove the successor. */

            itree_iter_t *iter = itree_iter_init(droot->r);
            itree_t *succ = itree_iter_next(iter);
            itree_iter_free(iter);
            assert(succ != nullptr);

            droot->k = succ->k;
            droot->v--;

            uint64_t dummy = 0;
            _itree_erase(succ->k, &droot->r, &dummy);
        }
    }

    /* Rebalance if necessary. */
    if (*root != nullptr) {
        _itree_rebalance(root);
        _itree_set_height(*root);
    }

    return ret;
}

void
itree::itree_free(itree_t *root)
{
    if (root == NULL) {
        return;
    }

    itree_free(root->l);
    itree_free(root->r);

    free(root);
}


/* itree_iter defines. */

#define ITER_PUSH(iter, val) do { iter->stack[iter->top++] = val; } while(0);
#define ITER_POP(iter) (iter->stack[--iter->top])


/* itree_iter definitions. */

itree::itree_iter_t *
itree::itree_iter_init(itree_t *root)
{
    itree_iter_t *iter = (itree_iter_t *)malloc(sizeof(itree_iter_t));
    if (iter == NULL) {
        return NULL;
    }

    memset(iter, 0, sizeof(itree_iter_t));
    iter->root = root;

    itree_t *n = root;
    do {
        ITER_PUSH(iter, n);
        n = n->l;
    } while (n != NULL);

    return iter;
}

itree::itree_t *
itree::itree_iter_next(itree_iter_t *iter)
{
    if (iter->top == 0) {
        return NULL;
    }

    itree_t *next = ITER_POP(iter);

    itree_t *n = next->r;
    while (n != NULL) {
        ITER_PUSH(iter, n);
        n = n->l;
    }

    return next;
}

void
itree::itree_iter_free(itree_iter_t *iter)
{
    free(iter);
}

}
