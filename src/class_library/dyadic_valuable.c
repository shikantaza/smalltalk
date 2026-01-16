#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

OBJECT_PTR DyadicValuable;

extern binding_env_t *g_top_level;
extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Object;

OBJECT_PTR dyadic_valuable_value(OBJECT_PTR closure, OBJECT_PTR arg1, OBJECT_PTR arg2, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));

  nativefn nf = (nativefn)extract_native_fn(receiver);

  return nf(receiver, arg1, arg2, cont);
}

void create_DyadicValuable()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_DyadicValuable(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("DyadicValuable");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;

  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;

  cls_obj->instance_methods = (method_binding_env_t *)GC_MALLOC(sizeof(method_binding_env_t));
  cls_obj->instance_methods->count = 1;
  cls_obj->instance_methods->bindings = (method_binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(method_binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_value:value:");
  cls_obj->instance_methods->bindings[0].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)dyadic_valuable_value),
						    NIL, NIL,
						    2, NIL, NULL);

  cls_obj->class_methods = (method_binding_env_t *)GC_MALLOC(sizeof(method_binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  DyadicValuable =  convert_class_object_to_object_ptr(cls_obj);
}
