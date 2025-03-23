#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

OBJECT_PTR MonadicBlock;

extern binding_env_t *g_top_level;
extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Object;

OBJECT_PTR monadic_block_value(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));

  nativefn nf = (nativefn)extract_native_fn(receiver);

  return nf(receiver, arg, cont);
}

void create_MonadicBlock()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_MonadicBlock(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("MonadicBlock");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;

  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;

  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 1;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_value:");
  cls_obj->instance_methods->bindings[0].val = list(3,
						    convert_native_fn_to_object((nativefn)monadic_block_value),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  MonadicBlock =  convert_class_object_to_object_ptr(cls_obj);
}
