/*
 *  Copyright 2017 Kjell Winblad (http://winsh.me, kjellwinblad@gmail.com)
 *
 *  This file is part of qd_lock_lib.
 *
 *  qd_lock_lib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  qd_lock_lib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with qd_lock_lib.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ERROR_HELP_H
#define ERROR_HELP_H

#include <stdio.h>
#include <stdlib.h>

static inline void LL_error_and_exit_verbose(const char * file, const char * function, int line, char * message){
    printf("ERROR IN FILE: %s, FUNCTION: %s, LINE: %d\n", file, function, line);
    printf("%s\n", message);
    printf("EXITING\n");
    exit(1);
}

#define LL_error_and_exit(message) LL_error_and_exit_verbose(__FILE__, __func__, __LINE__, message)

#endif
