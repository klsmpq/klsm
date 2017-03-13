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

#ifndef __FAT_SKIPLIST_H__
#define __FAT_SKIPLIST_H__

#include <stdbool.h>

#ifndef SKIPLIST_MALLOC
#define SKIPLIST_MALLOC(size) malloc(size)
#endif

#ifndef SKIPLIST_FREE
#define SKIPLIST_FREE free
#endif


//Max 255
#define SKIPLIST_MAX_VALUSES_IN_NODE 90

#define SKIPLIST_NUM_OF_LEVELS 21

typedef struct key_value_item {
    unsigned long key;
    unsigned long value;
} KeyValueItem;

typedef struct skiplist_node {
    //contains information about if it is a boarder point
    unsigned char info;
    bool sorted;
    unsigned char num_of_levels;
    unsigned char first_key_value_pos;
    unsigned long max_key;
    KeyValueItem *key_values;  //Points to region after lower_lists
    struct skiplist_node *lower_lists[];
} SkiplistNode;

typedef struct skiplist Skiplist;
void skiplist_put(Skiplist *skiplist, unsigned long key, unsigned long value);
unsigned long  skiplist_remove_min(Skiplist *skiplist, unsigned long *key_write_back);
Skiplist *new_skiplist();
void skiplist_delete(Skiplist *skiplist);
bool skiplist_is_empty(Skiplist *skiplist);
bool skiplist_more_than_two_keys(Skiplist *skiplist);
unsigned long skiplist_max_key(Skiplist *skiplist);
/* This function assumes that the input skiplist has at least two different keys */
unsigned long skiplist_split(Skiplist *skiplist,
                             Skiplist **left_writeback,
                             Skiplist **right_writeback);
Skiplist *skiplist_join(Skiplist *left_skiplist,
                        Skiplist *right_skiplist);
SkiplistNode *skiplist_remove_head_nodes(Skiplist *skiplist, int number_of_nodes);

#endif
