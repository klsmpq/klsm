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
#include "binheap.h"

int main()
{
    heap_t *h = malloc(sizeof(heap_t));
    h-> len = 0;
    h->size = MAX_HEAP_SIZE;
    push(h, 3, 3);
    push(h, 4, 4);
    push(h, 5, 5);
    push(h, 1, 1);
    push(h, 2, 2);
    int i;
    for (i = 0; i < 5; i++) {
        unsigned long key, value;
        pop(h, &key, &value);
        printf("%lu %lu\n", key, value);
    }
    return 0;
}
