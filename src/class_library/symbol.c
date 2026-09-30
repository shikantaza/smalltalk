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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

OBJECT_PTR Symbol;

extern binding_env_t *g_top_level;
extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Object;
extern OBJECT_PTR ReadableString;

extern stack_type *g_call_chain;

OBJECT_PTR symbol_as_string(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);
  
  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, IS_SMALLTALK_SYMBOL_OBJECT(receiver) ? get_string_obj(get_smalltalk_symbol_name(receiver)) :
                                                                         get_string_obj(get_symbol_name(receiver)));
}

OBJECT_PTR symbol_as_symbol(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, receiver);
}

void create_Symbol()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Symbol(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = ReadableString;
  cls_obj->name = GC_strdup("Symbol");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  cls_obj->shared_vars->bindings = NULL;
  
  cls_obj->instance_methods = (method_binding_env_t *)GC_MALLOC(sizeof(method_binding_env_t));
  cls_obj->instance_methods->count = 2;
  cls_obj->instance_methods->bindings = (method_binding_t **)GC_MALLOC(cls_obj->instance_methods->count * sizeof(method_binding_t *));

  cls_obj->class_methods = (method_binding_env_t *)GC_MALLOC(sizeof(method_binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  cls_obj->instance_methods->bindings[0] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[0]->key = get_symbol("_asString");
  cls_obj->instance_methods->bindings[0]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						 convert_native_fn_to_object((nativefn)symbol_as_string),
						 NIL, NIL,
						 0, NIL, NULL);

  cls_obj->instance_methods->bindings[1] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[1]->key = get_symbol("_asSymbol");
  cls_obj->instance_methods->bindings[1]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						 convert_native_fn_to_object((nativefn)symbol_as_symbol),
						 NIL, NIL,
						 0, NIL, NULL);
  
  cls_obj->nof_aliases = 0;
  cls_obj->aliases = NULL;

  Symbol =  convert_class_object_to_object_ptr(cls_obj);
}
