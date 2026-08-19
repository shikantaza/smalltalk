/**
  Copyright 2026 Rajesh Jayaprakash <rajesh.jayaprakash@protonmail.com>

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  It is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this file.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <stdint.h>

typedef uintptr_t OBJECT_PTR;
typedef int BOOLEAN;

char *convert_to_lower_case(char *);
char *convert_identifier(char *);
char *replace_hyphens(char *);
BOOLEAN is_gensym(OBJECT_PTR);

void sort(char *);
char *substring(const char*, size_t, size_t); 
char *strip_last_colon(char *);
