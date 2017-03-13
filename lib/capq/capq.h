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

/*
  This file is the header file for an implementation of the contention
  avoiding priority queue (CA-PQ). CA-PQ is described in the paper
  "The Contention Avoiding Concurrent Priority Queue" which is
  published in the 29th International Workshop on Languages and
  Compilers for Parallel Computing (LCPC 2016).
*/

#ifndef CAPQ_H
#define CAPQ_H
#include <stdbool.h>

#ifndef SLCATREE_MALLOC
#      define SLCATREE_MALLOC(size) malloc(size)
#endif

#ifndef SLCATREE_FREE
#      define SLCATREE_FREE(data) free(data)
#endif

typedef struct fpasl_catree_set CAPQ;

void capq_put(CAPQ *set,
              unsigned long key,
              unsigned long value);
void capq_put_param(CAPQ *set,
                    unsigned long key,
                    unsigned long value,
                    bool catree_adapt);
unsigned long capq_remove_min(CAPQ *set, unsigned long *key_write_back);
unsigned long capq_remove_min_param(CAPQ *set,
                                    unsigned long *key_write_back,
                                    bool remove_min_relax,
                                    bool put_relax,
                                    bool catree_adapt);
void capq_delete(CAPQ *setParam);
CAPQ *capq_new();

#endif
