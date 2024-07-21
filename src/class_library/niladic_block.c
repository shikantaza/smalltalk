#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../smalltalk.h"

nativefn extract_native_fn(OBJECT_PTR);
OBJECT_PTR convert_int_to_object(int);

OBJECT_PTR get_symbol(char *);
OBJECT_PTR convert_class_object_to_object_ptr(class_object_t *);
OBJECT_PTR convert_native_fn_to_object(nativefn);

OBJECT_PTR get_binding_val(binding_env_t *, OBJECT_PTR);

extern binding_env_t *top_level;

extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;

extern OBJECT_PTR Object;
OBJECT_PTR NiladicBlock;

OBJECT_PTR niladic_block_arg_count(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));

  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  nativefn nf = (nativefn)extract_native_fn(cont);

  return nf(cont, convert_int_to_object(0));
}

OBJECT_PTR niladic_block_value(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));
  
  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));

  nativefn nf = (nativefn)extract_native_fn(receiver);
  
  return nf(receiver, cont);
}

void create_NiladicBlock()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Integer: Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("NiladicBlock");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  
  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 2;    
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("argumentCount");
  cls_obj->instance_methods->bindings[0].val = list(3,
						    convert_native_fn_to_object((nativefn)niladic_block_arg_count),
						    NIL,
						    convert_int_to_object(0));

  cls_obj->instance_methods->bindings[1].key = get_symbol("value");
  cls_obj->instance_methods->bindings[1].val = list(3,
						    convert_native_fn_to_object((nativefn)niladic_block_value),
						    NIL,
						    convert_int_to_object(0));
  
  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  NiladicBlock =  convert_class_object_to_object_ptr(cls_obj);
}
