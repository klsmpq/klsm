/*
 *  Copyright 2017 Kjell Winblad (http://winsh.me, kjellwinblad@gmail.com)
 *
 *  This file is part of kpqueue.
 *
 *  kpqueue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  kpqueue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with kpqueue.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>
#include "fat_skiplist.h"

/*
 * ========================
 * Internal data structures
 * ========================
 */

#define SKIPLIST_NORMAL_NODE 1
#define SKIPLIST_LEFT_BORDER_NODE 1 << 2
#define SKIPLIST_RIGHT_BORDER_NODE 1 << 1

typedef struct skiplist {
    SkiplistNode head_node;
} Skiplist;

struct one_level_find_result {
    SkiplistNode *neigbour_before;
    SkiplistNode *neigbour_after;
};

struct find_result {
    SkiplistNode *neigbours_before[SKIPLIST_NUM_OF_LEVELS];
    SkiplistNode *neigbours_after[SKIPLIST_NUM_OF_LEVELS];
};

struct thread_local_rand_state {
    char pad1[56];
    unsigned int rand;
    char pad2[64];
};

__thread struct thread_local_rand_state local_random_state = {.rand = 0};

#define SKIPLIST_USE_LOCAL_RANDOM 1

/*
 * ==================
 * Internal functions
 * ==================
 */

// From https://rosettacode.org/wiki/Sorting_algorithms/Insertion_sort#C
static inline void insertion_sort(KeyValueItem *a, unsigned int n)
{
    for (unsigned long i = 1; i < n; ++i) {
        KeyValueItem tmp = a[i];
        unsigned int j = i;
        while (j > 0 && tmp.key < a[j - 1].key) {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = tmp;
    }
}

// From http://rosettacode.org/wiki/Sorting_algorithms/Quicksort#C
static inline
void quick_sort(KeyValueItem  *a, int n)
{
    int i, j;
    KeyValueItem  t;
    unsigned long p;
    if (n < 5) {
        insertion_sort(a, n);
        return;
    }
    p = a[n / 2].key;
    for (i = 0, j = n - 1;; i++, j--) {
        while (a[i].key < p) {
            i++;
        }
        while (p < a[j].key) {
            j--;
        }
        if (i >= j) {
            break;
        }
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
    quick_sort(a, i);
    quick_sort(a + i, n - i);
}

static inline
void sort_node(SkiplistNode *node)
{
    if (! node->sorted) {
        node->sorted = true;
        quick_sort(&node->key_values[node->first_key_value_pos],
                   SKIPLIST_MAX_VALUSES_IN_NODE - node->first_key_value_pos);
    }
}

static inline
SkiplistNode *create_skiplist_node(int num_of_levels)
{
    SkiplistNode *skiplist =
        (SkiplistNode *)SKIPLIST_MALLOC(sizeof(SkiplistNode) +
                                        sizeof(SkiplistNode *) * (num_of_levels) +
                                        sizeof(unsigned long) * (SKIPLIST_MAX_VALUSES_IN_NODE * 2));
    skiplist->info = SKIPLIST_NORMAL_NODE;
    skiplist->sorted = false;
    skiplist->num_of_levels = num_of_levels;
    skiplist->first_key_value_pos = SKIPLIST_MAX_VALUSES_IN_NODE;
    skiplist->key_values = (KeyValueItem *)&skiplist->lower_lists[num_of_levels];
    return skiplist;
}

static inline
int random_level(int num_of_levels)
{
#ifdef SKIPLIST_USE_LOCAL_RANDOM
    //From linden.c
    int level = 0;
    if (local_random_state.rand == 0) {
        local_random_state.rand = rand() + 104534169/*+ (unsigned int)((intptr_t)&level + 10453435)*/;
    }
    unsigned int r = local_random_state.rand;
    local_random_state.rand = r * 1103515245 + 82345;
    r &= (1u << (SKIPLIST_NUM_OF_LEVELS - 1)) - 1;
    while (((r >>= 1) & 1) && level < (num_of_levels - 1)) {
        ++level;
    }
    return num_of_levels - level - 1;
#else
    int i;
    long random_number = rand();
    int num = 2;
    for (i = num_of_levels - 1 ; i > 0 ; i--) {
        if (random_number > (RAND_MAX / num)) {
            return i;
        }
        num = num * 2;
    }
#endif
    return 0;
}

static inline
struct one_level_find_result find_neigbours_1_level(SkiplistNode *skiplist,
        unsigned long key,
        int level)
{

    SkiplistNode *skiplist_prev = skiplist;
    int level_pos = level - (SKIPLIST_NUM_OF_LEVELS - skiplist_prev->num_of_levels);
    SkiplistNode *skiplist_next = skiplist_prev->lower_lists[level_pos];
    struct one_level_find_result result;

    while (1) {
        if (skiplist_next->info == SKIPLIST_RIGHT_BORDER_NODE || skiplist_next->max_key >= key) {
            result.neigbour_before = skiplist_prev;
            result.neigbour_after = skiplist_next;
            return result;
        } else {
            level_pos = level - (SKIPLIST_NUM_OF_LEVELS - skiplist_next->num_of_levels);
            skiplist_prev = skiplist_next;
            skiplist_next = skiplist_next->lower_lists[level_pos];
        }
    }

    return result;

}

struct one_level_find_result find_neigbours_of_node_1_level(SkiplistNode *skiplist,
        SkiplistNode *node,
        int level)
{

    unsigned long key = node->max_key;
    SkiplistNode *skiplist_prev = skiplist;
    int level_pos = level - (SKIPLIST_NUM_OF_LEVELS - skiplist_prev->num_of_levels);
    SkiplistNode *skiplist_next = skiplist_prev->lower_lists[level_pos];
    struct one_level_find_result result;
    SkiplistNode *prev_cand = NULL;
    SkiplistNode *next_cand = NULL;
    while (1) {
        if (skiplist_next == node) {
            result.neigbour_before = skiplist_prev;
            level_pos = level - (SKIPLIST_NUM_OF_LEVELS - skiplist_next->num_of_levels);
            result.neigbour_after = skiplist_next->lower_lists[level_pos];
            return result;
        } else if (skiplist_next->info == SKIPLIST_RIGHT_BORDER_NODE || skiplist_next->max_key > key) {
            if (prev_cand != NULL) {
                /*The node is not in level but our neighbour before need to be before our node*/
                result.neigbour_before = prev_cand;
                result.neigbour_after = next_cand;
                return result;
            } else {
                result.neigbour_before = skiplist_prev;
                result.neigbour_after = skiplist_next;
            }
            return result;
        } else {
            if (prev_cand == NULL && skiplist_next->max_key == key) {
                /* Save this for later and continue */
                prev_cand = skiplist_prev;
                next_cand = skiplist_next;
            }
            level_pos = level - (SKIPLIST_NUM_OF_LEVELS - skiplist_next->num_of_levels);
            skiplist_prev = skiplist_next;
            skiplist_next = skiplist_next->lower_lists[level_pos];
        }
    }

    return result;

}

static inline
struct find_result find_neigbours(SkiplistNode *skiplist,
                                  unsigned long key)
{
    int level;
    struct find_result result;
    struct one_level_find_result level_result;
    SkiplistNode *neigbour_before_iter = skiplist;

    for (level = 0; level < SKIPLIST_NUM_OF_LEVELS; level++) {
        level_result =
            find_neigbours_1_level(neigbour_before_iter,
                                   key,
                                   level);
        result.neigbours_before[level] = level_result.neigbour_before;
        result.neigbours_after[level] = level_result.neigbour_after;
        neigbour_before_iter = level_result.neigbour_before;
    }
    return result;

}


struct find_result find_neigbours_of_node(SkiplistNode *skiplist,
        SkiplistNode *node)
{
    int level;
    struct find_result result;
    struct one_level_find_result level_result;
    SkiplistNode *neigbour_before_iter = skiplist;

    for (level = 0; level < SKIPLIST_NUM_OF_LEVELS; level++) {
        level_result =
            find_neigbours_of_node_1_level(neigbour_before_iter,
                                           node,
                                           level);
        result.neigbours_before[level] = level_result.neigbour_before;
        result.neigbours_after[level] = level_result.neigbour_after;
        neigbour_before_iter = level_result.neigbour_before;
    }
    return result;

}

static inline
void set_next_at_level(SkiplistNode *skiplist,
                       SkiplistNode *next_skiplist,
                       int level)
{
    int level_pos = level - (SKIPLIST_NUM_OF_LEVELS - skiplist->num_of_levels);
    skiplist->lower_lists[level_pos] = next_skiplist;
}

static inline
void insert_sublist(SkiplistNode *skiplist,
                    struct find_result neigbours,
                    SkiplistNode *sublist,
                    int level_to_insert_from,
                    int level_to_remove_from)
{
    int link_level;
    for (link_level = level_to_remove_from; link_level < level_to_insert_from; link_level++) {
        set_next_at_level(neigbours.neigbours_before[link_level],
                          neigbours.neigbours_after[link_level],
                          link_level);
    }
    for (link_level = level_to_insert_from; link_level < SKIPLIST_NUM_OF_LEVELS; link_level++) {
        set_next_at_level(neigbours.neigbours_before[link_level],
                          sublist,
                          link_level);
        set_next_at_level(sublist,
                          neigbours.neigbours_after[link_level],
                          link_level);
    }
}

/*
 *================
 * Debug functions
 *================
 */

static inline
void skiplist_print(Skiplist *skiplist)
{
    SkiplistNode *head_node = &(skiplist->head_node);

    SkiplistNode *node_temp = head_node->lower_lists[head_node->num_of_levels - 1];
    SkiplistNode *node_iter = node_temp;
    printf("PRINT SL START ================= %p \n", skiplist);
    /* printf("PRINT FIST NODE  \n"); */
    /* for(int i = 0; i < head_node->num_of_levels; i++){ */
    /*     printf("%d %p \n", i, head_node->lower_lists[i]); */
    /* } */
    /* printf("END PRINT FIRST NODE  \n"); */
    while (node_iter->info &  SKIPLIST_NORMAL_NODE) {
        node_temp = node_iter;
        node_iter = node_iter->lower_lists[node_iter->num_of_levels - 1];
        printf("Min Key: %lu, Max Key: %lu, Size: %d, NUM LEVELS: %d addr %p nexts: ",
               node_temp->key_values[node_temp->first_key_value_pos].key,
               node_temp->max_key, SKIPLIST_MAX_VALUSES_IN_NODE - node_temp->first_key_value_pos,
               node_temp->num_of_levels, node_temp);
        for (int i = node_temp->num_of_levels - 1; i >= 0; i--) {
            printf("%p ", node_temp->lower_lists[i]);
        }
        printf("\n");
    }
    printf("PRINT SL END ==================\n");
}

static inline
void validate(Skiplist *skiplist)
{
    SkiplistNode *head_node = &(skiplist->head_node);

    SkiplistNode *node_temp = head_node->lower_lists[head_node->num_of_levels - 1];
    SkiplistNode *node_iter = node_temp;
    while (node_iter->info &  SKIPLIST_NORMAL_NODE) {
        node_temp = node_iter;
        if (!(node_iter != node_iter->lower_lists[node_iter->num_of_levels - 1])) {
            printf("ASSERTION FAILED !!!!! NOT SO GOOD\n");

        }
        assert(node_iter != node_iter->lower_lists[node_iter->num_of_levels - 1]);
        node_iter = node_iter->lower_lists[node_iter->num_of_levels - 1];
    }
}

/*
 *=================
 * Public interface
 *=================
 */

unsigned long skiplist_remove_min(Skiplist *skiplist, unsigned long *key_write_back)
{
    SkiplistNode *head_node = &(skiplist->head_node);

    SkiplistNode *firstCandidate =
        head_node->lower_lists[head_node->num_of_levels - 1];
    if (firstCandidate->info & SKIPLIST_RIGHT_BORDER_NODE) {
        *key_write_back = -1;
        return 0;
    } else {
        sort_node(firstCandidate);
        *key_write_back = firstCandidate->key_values[firstCandidate->first_key_value_pos].key;
        unsigned long value = firstCandidate->key_values[firstCandidate->first_key_value_pos].value;
        firstCandidate->first_key_value_pos++;
        if (firstCandidate->first_key_value_pos == SKIPLIST_MAX_VALUSES_IN_NODE) {
            int remove_level;
            int remove_from_level =
                SKIPLIST_NUM_OF_LEVELS - firstCandidate->num_of_levels;
            for (remove_level = remove_from_level; remove_level < head_node->num_of_levels; remove_level++) {
                if (head_node->lower_lists[remove_level] == firstCandidate)
                    set_next_at_level(head_node,
                                      firstCandidate->lower_lists[remove_level - remove_from_level],
                                      remove_level);
            }
            SKIPLIST_FREE(firstCandidate);
        }
        return value;
    }
}


SkiplistNode *skiplist_remove_head_nodes(Skiplist *skiplist, int number_of_nodes)
{
    SkiplistNode *head_node = &(skiplist->head_node);
    SkiplistNode *firstCandidate =
        head_node->lower_lists[head_node->num_of_levels - 1];
    if (firstCandidate->info & SKIPLIST_RIGHT_BORDER_NODE) {
        return NULL;
    } else {
        SkiplistNode *current_last_node = firstCandidate;
        SkiplistNode *prev_last_node;
        int current_number_of_nodes_fetched = 1;
        do {
            int remove_level;
            int remove_from_level =
                SKIPLIST_NUM_OF_LEVELS - current_last_node->num_of_levels;
            for (remove_level = remove_from_level; remove_level < head_node->num_of_levels; remove_level++) {
                if (head_node->lower_lists[remove_level] == current_last_node)
                    set_next_at_level(head_node,
                                      current_last_node->lower_lists[remove_level - remove_from_level],
                                      remove_level);
            }
            prev_last_node = current_last_node;
            current_last_node = current_last_node->lower_lists[current_last_node->num_of_levels - 1];
            current_number_of_nodes_fetched++;
        } while (current_number_of_nodes_fetched < number_of_nodes &&
                 current_last_node->info != SKIPLIST_RIGHT_BORDER_NODE);
        prev_last_node->lower_lists[prev_last_node->num_of_levels - 1] = NULL;
        return firstCandidate;
    }
}

static inline
void skiplist_insert_into_non_full_node(SkiplistNode *node, unsigned long key, unsigned long value)
{
    unsigned int current_pos = node->first_key_value_pos - 1;
    node->first_key_value_pos = current_pos;
    while (current_pos < (SKIPLIST_MAX_VALUSES_IN_NODE - 1)
            && key > node->key_values[current_pos + 1].key) {
        node->key_values[current_pos] = node->key_values[current_pos + 1];
        current_pos = current_pos + 1;
    }
    node->key_values[current_pos].key = key;
    node->key_values[current_pos].value = value;
    if (node->first_key_value_pos == (SKIPLIST_MAX_VALUSES_IN_NODE - 1) || key > node->max_key) {
        node->max_key = key;
    }
}

static inline
void skiplist_insert_into_non_full_node_no_sort(SkiplistNode *node,
        unsigned long key,
        unsigned long value)
{
    if (node->sorted) {
        node->sorted = false;
    }
    unsigned int current_pos = node->first_key_value_pos - 1;
    node->first_key_value_pos = current_pos;
    if (node->first_key_value_pos == (SKIPLIST_MAX_VALUSES_IN_NODE - 1) || key > node->max_key) {
        node->max_key = key;
    }
    node->key_values[current_pos].key = key;
    node->key_values[current_pos].value = value;

}


static inline SkiplistNode *insert_new_node(SkiplistNode *head_node,
        struct find_result neigbours)
{
    int level = random_level(head_node->num_of_levels);
    int num_of_elements_in_insert_level = head_node->num_of_levels - level;
    SkiplistNode *new_skiplist_node =
        create_skiplist_node(num_of_elements_in_insert_level);
    insert_sublist(head_node, neigbours, new_skiplist_node, level, level);
    return new_skiplist_node;
}

static inline int find_split_pos(SkiplistNode *node)
{
    if (node->max_key == node->key_values[node->first_key_value_pos].key) {
        return 0;
    } else if (node->key_values[SKIPLIST_MAX_VALUSES_IN_NODE / 2].key == node->max_key) {
        for (int i = SKIPLIST_MAX_VALUSES_IN_NODE / 2 - 1; i >= 0; i--) {
            if (node->key_values[i].key != node->max_key) {
                return i + 1;
            }
        }
        return 0;
    } else {
        unsigned long not_search_for_key = node->key_values[SKIPLIST_MAX_VALUSES_IN_NODE / 2].key;
        for (int i = SKIPLIST_MAX_VALUSES_IN_NODE / 2 + 1; i < SKIPLIST_MAX_VALUSES_IN_NODE; i++) {
            if (node->key_values[i].key != not_search_for_key) {
                return i;
            }
        }
    }
    return 0;
}

static inline void move_items_from_full(SkiplistNode *from, SkiplistNode *to, int number_of_items)
{
    for (int i = 0; i < number_of_items; i++) {
        to->key_values[SKIPLIST_MAX_VALUSES_IN_NODE - number_of_items + i] =
            from->key_values[i];
    }
    from->first_key_value_pos = number_of_items;
    to->first_key_value_pos = SKIPLIST_MAX_VALUSES_IN_NODE - number_of_items;
    to->max_key = to->key_values[SKIPLIST_MAX_VALUSES_IN_NODE - 1].key;
}

static inline void move_items(SkiplistNode *from, SkiplistNode *to, int number_of_items)
{
    for (int i = from->first_key_value_pos; i < from->first_key_value_pos + number_of_items; i++) {
        to->key_values[SKIPLIST_MAX_VALUSES_IN_NODE - number_of_items + (i - from->first_key_value_pos)] =
            from->key_values[i];
    }
    from->first_key_value_pos = from->first_key_value_pos + number_of_items;
    to->first_key_value_pos = SKIPLIST_MAX_VALUSES_IN_NODE - number_of_items;
    to->max_key = to->key_values[SKIPLIST_MAX_VALUSES_IN_NODE - 1].key;
}

static inline void do_split(SkiplistNode *left, SkiplistNode *right,
                            unsigned long key, unsigned long value)
{
    int split_pos = find_split_pos(right);
    if (split_pos != 0) {
        move_items_from_full(right, left, split_pos);
        if (left->max_key >= key) {
            skiplist_insert_into_non_full_node(left, key, value);
        } else {
            skiplist_insert_into_non_full_node(right, key, value);
        }
    } else {
        skiplist_insert_into_non_full_node(left, key, value);
    }
}

void skiplist_put(Skiplist *skiplist, unsigned long key, unsigned long value)
{
    SkiplistNode *head_node = &(skiplist->head_node);
    struct find_result neigbours = find_neigbours(head_node, key);
    SkiplistNode *first_node = neigbours.neigbours_after[SKIPLIST_NUM_OF_LEVELS - 1];
    SkiplistNode *before_node = neigbours.neigbours_before[SKIPLIST_NUM_OF_LEVELS - 1];
    if (first_node->info == SKIPLIST_RIGHT_BORDER_NODE &&
            before_node->info != SKIPLIST_LEFT_BORDER_NODE &&
            before_node->first_key_value_pos != 0) {
        skiplist_insert_into_non_full_node_no_sort(before_node, key, value);
    } else if (first_node->info == SKIPLIST_RIGHT_BORDER_NODE &&
               before_node->info != SKIPLIST_LEFT_BORDER_NODE) {
        /*Need to split the node before*/
        sort_node(before_node);
        SkiplistNode *new_skiplist_node = insert_new_node(head_node, neigbours);
        move_items_from_full(before_node, new_skiplist_node, SKIPLIST_MAX_VALUSES_IN_NODE);
        unsigned long new_key = new_skiplist_node->key_values[SKIPLIST_MAX_VALUSES_IN_NODE - 1].key;
        unsigned long new_value = new_skiplist_node->key_values[SKIPLIST_MAX_VALUSES_IN_NODE - 1].value;
        new_skiplist_node->key_values[SKIPLIST_MAX_VALUSES_IN_NODE - 1].key = key;
        new_skiplist_node->key_values[SKIPLIST_MAX_VALUSES_IN_NODE - 1].value = value;
        new_skiplist_node->max_key = key;
        do_split(before_node, new_skiplist_node,
                 new_key, new_value);
    } else if (first_node->info == SKIPLIST_NORMAL_NODE &&
               first_node->max_key >= key &&
               (first_node->first_key_value_pos != 0)) {
        skiplist_insert_into_non_full_node_no_sort(first_node, key, value);
    } else if (first_node->info == SKIPLIST_RIGHT_BORDER_NODE) {
        /* No node, insert first */
        SkiplistNode *new_skiplist_node = insert_new_node(head_node, neigbours);
        skiplist_insert_into_non_full_node_no_sort(new_skiplist_node, key, value);
    } else {
        /* We need to split */
        sort_node(first_node);
        SkiplistNode *new_skiplist_node = insert_new_node(head_node, neigbours);
        do_split(new_skiplist_node, first_node, key, value);
    }
}

bool skiplist_is_empty(Skiplist *skiplist)
{
    SkiplistNode *head_node = &(skiplist->head_node);
    return head_node->lower_lists[head_node->num_of_levels - 1]->info == SKIPLIST_RIGHT_BORDER_NODE;
}

unsigned long skiplist_max_key(Skiplist *skiplist)
{
    SkiplistNode *head_node = &(skiplist->head_node);
    struct find_result fr = find_neigbours(head_node, ULONG_MAX);
    SkiplistNode *last_node = fr.neigbours_before[SKIPLIST_NUM_OF_LEVELS - 1];
    if (last_node == head_node) {
        return 0;
    }
    return last_node->max_key;
}

bool skiplist_more_than_two_keys(Skiplist *skiplist)
{
    SkiplistNode *head_node = &(skiplist->head_node);
    struct find_result fr = find_neigbours(head_node, ULONG_MAX);
    SkiplistNode *last_node = fr.neigbours_before[SKIPLIST_NUM_OF_LEVELS - 1];
    SkiplistNode *first_node = head_node->lower_lists[SKIPLIST_NUM_OF_LEVELS - 1];
    return (last_node != head_node && last_node != first_node
            && last_node->max_key != first_node->key_values[first_node->first_key_value_pos].key);
}

/* This function assumes that the input skiplist has at least two different keys */
unsigned long skiplist_split(Skiplist *skiplist,
                             Skiplist **left_writeback,
                             Skiplist **right_writeback)
{
    /* Find split key */
    SkiplistNode *head_node = &(skiplist->head_node);
    unsigned long split_key = 0;
    SkiplistNode *split_node = NULL;
    for (int level = 0; level < SKIPLIST_NUM_OF_LEVELS; level++) {
        if (head_node->lower_lists[level]->info != SKIPLIST_RIGHT_BORDER_NODE) {
            split_node = head_node->lower_lists[level];
            split_key = split_node->max_key;
            break;
        }
    }
    /* Make sure that the split is non-empty */
    SkiplistNode *first_node = head_node->lower_lists[head_node->num_of_levels - 1];
    sort_node(first_node);
    if (first_node->key_values[first_node->first_key_value_pos].key == split_key) {
        do {
            split_node = split_node->lower_lists[split_node->num_of_levels - 1];
        } while (split_node->max_key == split_key);
        split_key = split_node->max_key;
    }

    /* Allocate new nodes */

    Skiplist *right_skiplist = SKIPLIST_MALLOC(sizeof(Skiplist) +
                               sizeof(SkiplistNode *) * (SKIPLIST_NUM_OF_LEVELS));
    SkiplistNode *right_skiplist_left_sentinel = (SkiplistNode *) & (right_skiplist->head_node);

    //right_skiplist_left_sentinel->key = 0;
    right_skiplist_left_sentinel->num_of_levels = SKIPLIST_NUM_OF_LEVELS;
    right_skiplist_left_sentinel->info = SKIPLIST_LEFT_BORDER_NODE;

    SkiplistNode *new_left_right_sentinel =
        create_skiplist_node(SKIPLIST_NUM_OF_LEVELS);

    new_left_right_sentinel->info = SKIPLIST_RIGHT_BORDER_NODE;

    /* Create new skip */

    struct find_result split_info = find_neigbours(&skiplist->head_node, split_key);

    SkiplistNode *first_node_in_right = split_info.neigbours_after[SKIPLIST_NUM_OF_LEVELS - 1];
    sort_node(first_node_in_right);
    if (first_node_in_right->key_values[first_node_in_right->first_key_value_pos].key != split_key) {
        /* The first key is not the splt_key, we need to split it */
        for (unsigned char split_node_pos = first_node_in_right->first_key_value_pos + 1;
                split_node_pos < SKIPLIST_MAX_VALUSES_IN_NODE;
                split_node_pos++) {
            if (first_node_in_right->key_values[split_node_pos].key == split_key) {
                SkiplistNode  *new_node = insert_new_node(head_node, split_info);
                move_items(first_node_in_right, new_node,
                           split_node_pos - first_node_in_right->first_key_value_pos);
                split_info = find_neigbours(&skiplist->head_node, split_key);
                break;
            }
        }
    }

    for (int level = 0; level < SKIPLIST_NUM_OF_LEVELS; level++) {
        set_next_at_level(split_info.neigbours_before[level],
                          new_left_right_sentinel,
                          level);
        set_next_at_level(right_skiplist_left_sentinel,
                          split_info.neigbours_after[level],
                          level);
    }

    /* Remove and insert split node at new level */

    struct find_result remove_info = find_neigbours_of_node(&right_skiplist->head_node, split_node);

    for (int link_level = SKIPLIST_NUM_OF_LEVELS - split_node->num_of_levels;
            link_level < SKIPLIST_NUM_OF_LEVELS;
            link_level++) {
        set_next_at_level(remove_info.neigbours_before[link_level],
                          remove_info.neigbours_after[link_level],
                          link_level);
    }
    for (int i = split_node->first_key_value_pos; i < SKIPLIST_MAX_VALUSES_IN_NODE; i++) {
        skiplist_put(right_skiplist, split_node->key_values[i].key, split_node->key_values[i].value);
    }
    SKIPLIST_FREE(split_node);
    *left_writeback = skiplist;
    *right_writeback = right_skiplist;

    return split_key;
}

Skiplist *skiplist_join(Skiplist *left_skiplist,
                        Skiplist *right_skiplist)
{

    /* Find last element on all levels in left_skiplist */
    SkiplistNode *last_nodes[SKIPLIST_NUM_OF_LEVELS];
    SkiplistNode *last_node = &(left_skiplist->head_node);
    SkiplistNode *current_node = last_node->lower_lists[0];
    for (int level = 0; level < SKIPLIST_NUM_OF_LEVELS; level++) {
        while (current_node->info !=  SKIPLIST_RIGHT_BORDER_NODE) {
            last_node = current_node;
            int level_pos = level - (SKIPLIST_NUM_OF_LEVELS - current_node->num_of_levels);
            current_node = current_node->lower_lists[level_pos];
        }
        last_nodes[level] = last_node;
        current_node = last_node;
    }

    /* Save sentinel nodes to be linked out */

    SkiplistNode *left_skiplist_right_sentinel = last_nodes[0]->lower_lists[0];

    /* Link together the skiplists */

    SkiplistNode *right_head = &(right_skiplist->head_node);

    for (int level = 0; level < SKIPLIST_NUM_OF_LEVELS; level++) {
        SkiplistNode *last_node = last_nodes[level];
        int level_pos = level - (SKIPLIST_NUM_OF_LEVELS - last_node->num_of_levels);
        last_node->lower_lists[level_pos] = right_head->lower_lists[level];
    }

    /* Free linked out sentinel nodes */

    SKIPLIST_FREE(left_skiplist_right_sentinel);
    SKIPLIST_FREE(right_skiplist);

    /* Return left skiplist */

    return left_skiplist;
}

Skiplist *new_skiplist()
{
    Skiplist *skiplist = SKIPLIST_MALLOC(sizeof(Skiplist) +
                                         sizeof(SkiplistNode *) * (SKIPLIST_NUM_OF_LEVELS));

    SkiplistNode *rightmost_skiplist =
        create_skiplist_node(SKIPLIST_NUM_OF_LEVELS);

    SkiplistNode *leftmost_skiplist = (SkiplistNode *) & (skiplist->head_node);
    leftmost_skiplist->num_of_levels = SKIPLIST_NUM_OF_LEVELS;

    for (int i = 0 ; i < SKIPLIST_NUM_OF_LEVELS ; i++) {
        leftmost_skiplist->lower_lists[i] =
            rightmost_skiplist;
    }

    leftmost_skiplist->info = SKIPLIST_LEFT_BORDER_NODE;

    rightmost_skiplist->info = SKIPLIST_RIGHT_BORDER_NODE;

    return skiplist;
}

void skiplist_delete(Skiplist *skiplist)
{

    SkiplistNode *head_node = &(skiplist->head_node);

    SkiplistNode *node_temp = head_node->lower_lists[head_node->num_of_levels - 1];
    SkiplistNode *node_iter = node_temp;

    while (node_iter->info &  SKIPLIST_NORMAL_NODE) {
        node_temp = node_iter;
        node_iter = node_iter->lower_lists[node_iter->num_of_levels - 1];
        SKIPLIST_FREE(node_temp);
    }
    SKIPLIST_FREE(node_iter);
    SKIPLIST_FREE(skiplist);

    return;
}

/* int main(int argc, char *argv[]) { */
/*     //ssalloc_init(); */
/*     Skiplist * l = new_skiplist(); */
/*     for(unsigned long val = 10; val >= 0; val--){ */
/*         skiplist_put(l, val, val); */
/*         printf("INSERTED %ul\n", val); */
/*         skiplist_print(l); */
/*         if(val == 0){ */
/*             break; */
/*         } */
/*     } */

/*     for(unsigned long val = 0; val < 8; val++){ */
/*         skiplist_put(l, 2, 2); */
/*         printf("INSERTED %ul\n", val); */
/*         skiplist_print(l); */
/*     } */

/*     for(unsigned long val = 0; val < 20; val++){ */
/*         unsigned long wb; */
/*         printf("REMOVE MIN %ul\n", skiplist_remove_min(l, &wb)); */
/*     } */

/*     for(unsigned long val = 0; val < 3; val++){ */
/*         skiplist_put(l, val, val); */
/*         printf("INSERTED %ul\n", val); */
/*     } */


/*     for(int i = 0; i < 7; i++){ */
/*         unsigned long key = 44; */
/*         unsigned long val = skiplist_remove_min(l, &key); */
/*         printf("RETURNED %ul %ul\n", val, key); */
/*     } */


/*     for(unsigned long val = 0; val < 20; val++){ */
/*         skiplist_put(l, val, val); */
/*         printf("INSERTED %ul\n", val); */
/*     } */

/*     for(unsigned long val = 0; val < 20; val++){ */
/*         skiplist_put(l, val, val); */
/*         printf("INSERTED %ul\n", val); */
/*     } */

/*     Skiplist * l1_temp; */
/*     Skiplist * l2_temp; */
/*     Skiplist * l1; */
/*     Skiplist * l2; */
/*     Skiplist * l3; */
/*     Skiplist * l4; */
/*     printf("BEFORE SPLIT\n"); */
/*     skiplist_print(l); */
/*     printf("SPLIT KEY 1 %lu\n", skiplist_split(l, */
/*                                                &l1_temp, */
/*                                                &l2_temp)); */

/*     printf("SPLIT_RESULT\n"); */
/*     skiplist_print(l1_temp); */
/*     skiplist_print(l2_temp); */

/*     assert(skiplist_more_than_two_keys(l1_temp)); */
/*     assert(skiplist_more_than_two_keys(l2_temp)); */
/*     printf("SPLIT KEY 2 %lu\n", skiplist_split(l1_temp, */
/*                                                &l1, */
/*                                                &l2)); */
/*     printf("SPLIT KEY 3 %lu\n", skiplist_split(l2_temp, */
/*                                                &l3, */
/*                                                &l4)); */
/*     unsigned long dummy = 0; */
/*     unsigned long key; */

/*     l1_temp = skiplist_join(l1, l2); */
/*     l2_temp = skiplist_join(l3, l4); */
/*     l = skiplist_join(l1_temp, l2_temp); */

/*     for(int i = 0; i < 41; i++){ */
/*         unsigned long key = 44; */
/*         unsigned long val = skiplist_remove_min(l, &key); */
/*         printf("RETURNED %ul %ul\n", val, key); */
/*     } */
/*     skiplist_delete(l); */
/*     /\* printf("PRINT PART 1\n"); *\/ */
/*     /\* while((dummy = skiplist_remove_min(l1, &key)+1) && key != -1){ *\/ */
/*     /\*     printf("%lu\n", key); *\/ */
/*     /\* } *\/ */

/*     /\* printf("PRINT PART 2\n"); *\/ */
/*     /\* while((dummy = skiplist_remove_min(l2, &key)+1) && key != -1){ *\/ */
/*     /\*     printf("%lu\n", key); *\/ */
/*     /\* } *\/ */
/*     /\* printf("PRINT PART 3\n"); *\/ */
/*     /\* while((dummy = skiplist_remove_min(l3, &key)+1) && key != -1){ *\/ */
/*     /\*     printf("%lu\n", key); *\/ */
/*     /\* } *\/ */

/*     /\* printf("PRINT PART 4\n"); *\/ */
/*     /\* while((dummy = skiplist_remove_min(l4, &key)+1) && key != -1){ *\/ */
/*     /\*     printf("%lu\n", key); *\/ */
/*     /\* } *\/ */

/*     /\* skiplist_delete(l1); *\/ */
/*     /\* skiplist_delete(l2); *\/ */
/*     /\* skiplist_delete(l3); *\/ */
/*     /\* skiplist_delete(l4); *\/ */
/*     return 0; */
/* } */
