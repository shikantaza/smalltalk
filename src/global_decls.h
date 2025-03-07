/**
  Copyright 2011-2024 Rajesh Jayaprakash <rajesh.jayaprakash@pm.me>

  This file is part of <smalltalk>.

  <smalltalk> is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  <smalltalk> is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with <smalltalk>.  If not, see <http://www.gnu.org/licenses/>.
**/

//TODO: move all function declarations here

#ifndef GLOBAL_DECLS_H
#define GLOBAL_DECLS_H

#include "smalltalk.h"

BOOLEAN get_top_level_val(OBJECT_PTR, OBJECT_PTR *);
OBJECT_PTR new_object_internal(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR convert_fn_to_closure(nativefn);
OBJECT_PTR create_and_signal_exception(OBJECT_PTR, OBJECT_PTR);

void initialize_pass2();

#endif
