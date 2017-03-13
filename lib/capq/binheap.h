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

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    unsigned long key;
    unsigned long value;
} node_t;

#define MAX_HEAP_SIZE 8192

typedef struct {
    int len;
    int size;
    node_t nodes[MAX_HEAP_SIZE];
} heap_t;

bool push(heap_t *h, unsigned long key, unsigned long value)
{
    if (h->len + 1 >= h->size) {
        return false;
    }
    int i = h->len + 1;
    int j = i / 2;
    while (i > 1 && h->nodes[j].key > key) {
        h->nodes[i] = h->nodes[j];
        i = j;
        j = j / 2;
    }
    h->nodes[i].key = key;
    h->nodes[i].value = value;
    h->len++;
    return true;
}

bool peek(heap_t *h, unsigned long *key)
{
    if (h->len == 0) {
        return false;
    }
    *key = h->nodes[1].key;
    return true;
}


bool pop(heap_t *h, unsigned long *key, unsigned long *value)
{
    int i, j, k;
    if (!h->len) {
        return false;
    }
    h->size--;
    *key = h->nodes[1].key;
    *value = h->nodes[1].value;
    h->nodes[1] = h->nodes[h->len];
    h->len--;
    i = 1;
    while (1) {
        k = i;
        j = 2 * i;
        if (j <= h->len && h->nodes[j].key < h->nodes[k].key) {
            k = j;
        }
        if (j + 1 <= h->len && h->nodes[j + 1].key < h->nodes[k].key) {
            k = j + 1;
        }
        if (k == i) {
            break;
        }
        h->nodes[i] = h->nodes[k];
        i = k;
    }
    h->nodes[i] = h->nodes[h->len + 1];
    return true;
}


