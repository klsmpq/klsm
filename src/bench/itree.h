#ifndef __ITREE_H
#define __ITREE_H

#include <stdint.h>

#define ITREE_MAX_DEPTH (8 * sizeof(uint64_t))

/**
 * An AVL tree with closed, mutually disjunct uint64_t intervals as keys and
 * uint64_t values representing the index count in the right subtree (the
 * interval [5, 7] counts as 3 indices).
 *
 * For further information, see the Tree of Holes in Almasi, Cascaval and
 * Padua. Calculating stack distances efficiently. SIG-PLAN Not. 38, 2
 * supplement (June 2002), 37-43.
 */

typedef struct __itree_t {
    struct __itree_t *l, *r;    /**< The left and right child nodes. */
    uint64_t k1, k2;            /**< The key interval [k1, k2]. */
    uint64_t v;                 /**< The # of elements in the right subtree. */
    uint8_t h;                  /**< The height of this node. height(node without
                                 *   children) == 0. */
} itree_t;

/**
 * Inserts a new index into the tree and writes the # of indices in the
 * tree larger than the new index into holes. Returns 0 on success, < 0
 * on error.
 *
 * Preconditions:
 *  * Index must not be in the tree.
 *  * holes != NULL.
 *
 * Postconditions:
 *  * Index is in the tree.
 */
int
itree_insert(const uint64_t index,
             itree_t **root,
             uint64_t *holes);

void
itree_print(const itree_t *root);

void
itree_free(itree_t *root);


struct __itree_iter_t;
typedef struct __itree_iter_t itree_iter_t;

/* Sets up the iterator to traverse the tree pointed to by root
 * inorder. Returns NULL on error.
 *
 * Precondition:
 *  * The tree must have a depth <= ITREE_MAX_DEPTH.
 *  * root != NULL.
 *
 * Postconditions:
 *  * The iterator is ready to use.
 */
itree_iter_t *
itree_iter_init(const itree_t *root);

/**
 * Returns the next node of the tree, or NULL if the end has been
 * reached.
 *
 * Precondition:
 *  * iter != NULL.
 */
const itree_t *
itree_iter_next(itree_iter_t *iter);

void
itree_iter_free(itree_iter_t *iter);

#endif /*  __ITREE_H */
