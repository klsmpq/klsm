#ifndef __ITREE_H
#define __ITREE_H

#include <cstdint>
#include <utility>

class itree
{
public:
    struct elem_t {
        uint32_t key;
        uint32_t thread_id;
        uint32_t element_id;
    };

private:
    static constexpr int ITREE_MAX_DEPTH = (8 * sizeof(uint64_t));

    typedef struct __itree_t {
        struct __itree_t *l, *r;    /**< The left and right child nodes. */
        uint64_t k;                   /**< The key. */
        uint64_t v;                 /**< The # of elements in the right subtree. */
        uint8_t h;                  /**< The height of this node. height(node without
                                     *   children) == 0. */
    } itree_t;

    typedef struct {
        itree_t *u, *l;             /**< Upper and lower adjacent nodes. */
    } itree_util_t;

    typedef struct __itree_iter_t {
        const itree_t *root;                    /**< The root of the iterated tree. */
        const itree_t *stack[ITREE_MAX_DEPTH];  /**< The node stack. Tree depth cannot be exceeded
                                                     since keys are uint64_t. */
        int top;                                /**< The current top stack index. */
    } itree_iter_t;

public:
    itree();
    virtual ~itree();

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

private:
    void
    itree_free(itree_t *root);

    void
    _itree_print(const itree_t *root,
                 uint8_t level);
    int
    _itree_insert(const uint64_t index,
                  itree_t **root,
                  uint64_t *holes,
                  itree_util_t *util);
    int
    _itree_new_node(const uint64_t index,
                          itree_t **root);
    void
    _itree_extend_node(const uint64_t index,
                       itree_t *node);
    void
    _itree_merge_nodes(itree_t *upper,
                       itree_t *lower);
    void
    _itree_rebalance(itree_t **root);
    inline int8_t
    _itree_height(const itree_t *node);
    inline void
    _itree_set_height(itree_t *node);
    inline uint64_t
    _itree_count(const itree_t *root);
    int
    _itree_descend_l(const uint64_t index,
                     itree_t **root,
                     uint64_t *holes,
                     itree_util_t *util);
    int
    _itree_descend_r(const uint64_t index,
                     itree_t **root,
                     uint64_t *holes,
                     itree_util_t *util);

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

private:
    itree_t *m_root;
};

#endif /*  __ITREE_H */
