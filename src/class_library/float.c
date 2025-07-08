#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

//workaround for variadic function arguments
//getting clobbered in ARM64
typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR Float;

extern OBJECT_PTR ZeroDivide;
extern OBJECT_PTR InvalidArgument;
extern binding_env_t *g_top_level;

extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

extern OBJECT_PTR Object;

extern stack_type *g_call_chain;

OBJECT_PTR float_plus(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  assert(IS_FLOAT_OBJECT(receiver));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to plus\n");
#endif
  
  if(!IS_FLOAT_OBJECT(arg) && !IS_INTEGER_OBJECT(arg))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont,
			    convert_float_to_object(get_float_value(receiver) +
						    (IS_INTEGER_OBJECT(arg) ? get_int_value(arg) : get_float_value(arg))));
}

OBJECT_PTR float_minus(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  assert(IS_FLOAT_OBJECT(receiver));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to minus\n");
#endif
  
  if(!IS_FLOAT_OBJECT(arg) && !IS_INTEGER_OBJECT(arg))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));
  
  pop_if_top(entry);

  return invoke_cont_on_val(cont,
			    convert_float_to_object(get_float_value(receiver) -
						    (IS_INTEGER_OBJECT(arg) ? get_int_value(arg) : get_float_value(arg))));

}

OBJECT_PTR float_times(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  assert(IS_FLOAT_OBJECT(receiver));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to times\n");
#endif
  
  if(!IS_FLOAT_OBJECT(arg) && !IS_INTEGER_OBJECT(arg))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));
  
  pop_if_top(entry);

  return invoke_cont_on_val(cont,
			    convert_float_to_object(get_float_value(receiver) *
						    (IS_INTEGER_OBJECT(arg) ? get_int_value(arg) : get_float_value(arg))));
  
}

OBJECT_PTR float_divided_by(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  assert(IS_FLOAT_OBJECT(receiver));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to divide_by\n");
#endif
  
  if(!IS_FLOAT_OBJECT(arg) && !IS_INTEGER_OBJECT(arg))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  if(IS_FLOAT_OBJECT(arg))
    if(get_float_value(arg) == 0)
      return create_and_signal_exception(ZeroDivide, cont);

  if(IS_INTEGER_OBJECT(arg))
    if(get_int_value(arg) == 0)
      return create_and_signal_exception(ZeroDivide, cont);
  
  pop_if_top(entry);

  return invoke_cont_on_val(cont,
			    convert_float_to_object(get_float_value(receiver) /
						    (IS_INTEGER_OBJECT(arg) ? get_int_value(arg) : get_float_value(arg))));

}

//TODO: this can be subsumed by Object>>=
OBJECT_PTR float_eq(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_FLOAT_OBJECT(receiver));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG
  print_object(arg); printf(" is the arg passed to eq\n");
#endif

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  if(IS_FLOAT_OBJECT(arg))
    return invoke_cont_on_val(cont, get_float_value(receiver) == get_float_value(arg) ? TRUE : FALSE);
  else if(IS_INTEGER_OBJECT(arg))
    return invoke_cont_on_val(cont, get_float_value(receiver) == get_int_value(arg) ? TRUE : FALSE);
  else
    return invoke_cont_on_val(cont, receiver == arg ? TRUE : FALSE);
}

OBJECT_PTR float_lt(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  if(!IS_FLOAT_OBJECT(arg) && !IS_INTEGER_OBJECT(arg))
    return create_and_signal_exception(InvalidArgument, cont);

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG
  print_object(arg); printf(" is the arg passed to lt\n");
#endif

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, (get_float_value(receiver) <
				   (IS_INTEGER_OBJECT(arg) ? get_int_value(arg) : get_float_value(arg))) ? TRUE : FALSE);
}

OBJECT_PTR float_gt(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  if(!IS_FLOAT_OBJECT(arg) && !IS_INTEGER_OBJECT(arg))
    return create_and_signal_exception(InvalidArgument, cont);

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG
  print_object(arg); printf(" is the arg passed to gt\n");
#endif

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, (get_float_value(receiver) >
				   (IS_INTEGER_OBJECT(arg) ? get_int_value(arg) : get_float_value(arg))) ? TRUE : FALSE);
}

void create_Float()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Float: Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("Float");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  
  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 7;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_+");
  cls_obj->instance_methods->bindings[0].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)float_plus),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[1].key = get_symbol("_-");
  cls_obj->instance_methods->bindings[1].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)float_minus),
						    NIL, NIL,
						    1, NIL, NULL);
					
  
  cls_obj->instance_methods->bindings[2].key = get_symbol("_*");
  cls_obj->instance_methods->bindings[2].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)float_times),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[3].key = get_symbol("_/");
  cls_obj->instance_methods->bindings[3].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)float_divided_by),
						    NIL, NIL,
						    1, NIL, NULL);
  
  cls_obj->instance_methods->bindings[4].key = get_symbol("_=");
  cls_obj->instance_methods->bindings[4].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)float_eq),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[5].key = get_symbol("_<");
  cls_obj->instance_methods->bindings[5].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)float_lt),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[6].key = get_symbol("_>");
  cls_obj->instance_methods->bindings[6].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)float_gt),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  Float =  convert_class_object_to_object_ptr(cls_obj);
}
