#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

OBJECT_PTR Boolean;

extern binding_env_t *g_top_level;
extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;
extern OBJECT_PTR Object;
extern OBJECT_PTR MESSAGE_SEND;

extern OBJECT_PTR VALUE_SELECTOR;

OBJECT_PTR boolean_and(OBJECT_PTR closure, OBJECT_PTR operand, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  OBJECT_PTR ret;

  if(receiver == TRUE)
    if(operand == TRUE)
      ret = TRUE;
    else
      ret = FALSE;
  else
    ret = FALSE;

  return invoke_cont_on_val(cont, ret);
}

OBJECT_PTR boolean_or(OBJECT_PTR closure, OBJECT_PTR operand, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  OBJECT_PTR ret;

  if(receiver == TRUE)
    ret = TRUE;
  else
    if(operand == TRUE)
      ret = TRUE;
    else
      ret = FALSE;

  return invoke_cont_on_val(cont, ret);
}    

OBJECT_PTR boolean_short_circuit_and(OBJECT_PTR closure, OBJECT_PTR operand, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  if(receiver == FALSE)
    return invoke_cont_on_val(cont, FALSE);
  else
  {
    OBJECT_PTR msg_send = car(get_binding_val(g_top_level, MESSAGE_SEND));
    nativefn nf1 = (nativefn)extract_native_fn(msg_send);
    return nf1(msg_send, operand, VALUE_SELECTOR, convert_int_to_object(0), cont);
  }
}

OBJECT_PTR boolean_equiv(OBJECT_PTR closure, OBJECT_PTR operand, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  if(receiver == operand)
    return invoke_cont_on_val(cont, TRUE);
  else
    return invoke_cont_on_val(cont, FALSE);
}

OBJECT_PTR boolean_if_false(OBJECT_PTR closure, OBJECT_PTR operand, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  if(receiver == FALSE)
  {
    OBJECT_PTR msg_send = car(get_binding_val(g_top_level, MESSAGE_SEND));
    nativefn nf1 = (nativefn)extract_native_fn(msg_send);
    return nf1(msg_send, operand, VALUE_SELECTOR, convert_int_to_object(0), cont);
  }
  else
    return invoke_cont_on_val(cont, NIL);
}

OBJECT_PTR boolean_if_false_if_true(OBJECT_PTR closure,
				    OBJECT_PTR false_operand,
				    OBJECT_PTR true_operand,
				    OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  if(receiver == FALSE)
  {
    OBJECT_PTR msg_send = car(get_binding_val(g_top_level, MESSAGE_SEND));
    nativefn nf1 = (nativefn)extract_native_fn(msg_send);
    return nf1(msg_send, false_operand, VALUE_SELECTOR, convert_int_to_object(0), cont);
  }
  else
  {
    OBJECT_PTR msg_send = car(get_binding_val(g_top_level, MESSAGE_SEND));
    nativefn nf1 = (nativefn)extract_native_fn(msg_send);
    return nf1(msg_send, true_operand, VALUE_SELECTOR, convert_int_to_object(0), cont);
  }
}

OBJECT_PTR boolean_if_true(OBJECT_PTR closure, OBJECT_PTR operand, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  if(receiver == TRUE)
  {
    OBJECT_PTR msg_send = car(get_binding_val(g_top_level, MESSAGE_SEND));
    nativefn nf1 = (nativefn)extract_native_fn(msg_send);
    return nf1(msg_send, operand, VALUE_SELECTOR, convert_int_to_object(0), cont);
  }
  else
    return invoke_cont_on_val(cont, NIL);
}

OBJECT_PTR boolean_if_true_if_false(OBJECT_PTR closure,
				    OBJECT_PTR true_operand,
				    OBJECT_PTR false_operand,
				    OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  if(receiver == TRUE)
  {
    OBJECT_PTR msg_send = car(get_binding_val(g_top_level, MESSAGE_SEND));
    nativefn nf1 = (nativefn)extract_native_fn(msg_send);
    return nf1(msg_send, true_operand, VALUE_SELECTOR, convert_int_to_object(0), cont);
  }
  else
  {
    OBJECT_PTR msg_send = car(get_binding_val(g_top_level, MESSAGE_SEND));
    nativefn nf1 = (nativefn)extract_native_fn(msg_send);
    return nf1(msg_send, false_operand, VALUE_SELECTOR, convert_int_to_object(0), cont);
  }
}

OBJECT_PTR boolean_not(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  if(receiver == TRUE)
    return invoke_cont_on_val(cont, FALSE);
  else
    return invoke_cont_on_val(cont, TRUE);
}

OBJECT_PTR boolean_short_circuit_or(OBJECT_PTR closure, OBJECT_PTR operand, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  if(receiver == TRUE)
    return invoke_cont_on_val(cont, TRUE);
  else
  {
    OBJECT_PTR msg_send = car(get_binding_val(g_top_level, MESSAGE_SEND));
    nativefn nf1 = (nativefn)extract_native_fn(msg_send);
    return nf1(msg_send, operand, VALUE_SELECTOR, convert_int_to_object(0), cont);
  }
}

OBJECT_PTR boolean_xor(OBJECT_PTR closure, OBJECT_PTR operand, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  OBJECT_PTR ret;

  if(receiver == TRUE)
    if(operand == TRUE)
      ret = FALSE;
    else
      ret = TRUE;
  else
    if(operand == TRUE)
      ret = TRUE;
    else
      ret = FALSE;

  return invoke_cont_on_val(cont, ret);
}

OBJECT_PTR boolean_print_string(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_TRUE_OBJECT(receiver) || IS_FALSE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));

  if(receiver == TRUE)
    return invoke_cont_on_val(cont, get_string_obj("true"));
  else
    return invoke_cont_on_val(cont,get_string_obj("false"));
}

void create_Boolean()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Boolean(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("Boolean");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  
  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 12;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_&");
  cls_obj->instance_methods->bindings[0].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_and),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->instance_methods->bindings[1].key = get_symbol("_|");
  cls_obj->instance_methods->bindings[1].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_or),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->instance_methods->bindings[2].key = get_symbol("_and:");
  cls_obj->instance_methods->bindings[2].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_short_circuit_and),
						    NIL,
						    convert_int_to_object(1));
  
  cls_obj->instance_methods->bindings[3].key = get_symbol("_eqv:");
  cls_obj->instance_methods->bindings[3].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_equiv),
						    NIL,
						    convert_int_to_object(1));
  
  cls_obj->instance_methods->bindings[4].key = get_symbol("_ifFalse:");
  cls_obj->instance_methods->bindings[4].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_if_false),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->instance_methods->bindings[5].key = get_symbol("_ifFalse:ifTrue:");
  cls_obj->instance_methods->bindings[5].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_if_false_if_true),
						    NIL,
						    convert_int_to_object(2));
  
  cls_obj->instance_methods->bindings[6].key = get_symbol("_ifTrue:");
  cls_obj->instance_methods->bindings[6].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_if_true),
						    NIL,
						    convert_int_to_object(1));
  
  cls_obj->instance_methods->bindings[7].key = get_symbol("_ifTrue:ifFalse:");
  cls_obj->instance_methods->bindings[7].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_if_true_if_false),
						    NIL,
						    convert_int_to_object(2));
  
  cls_obj->instance_methods->bindings[8].key = get_symbol("_not");
  cls_obj->instance_methods->bindings[8].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_not),
						    NIL,
						    convert_int_to_object(0));

  cls_obj->instance_methods->bindings[9].key = get_symbol("_or:");
  cls_obj->instance_methods->bindings[9].val = list(3,
						    convert_native_fn_to_object((nativefn)boolean_short_circuit_or),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->instance_methods->bindings[10].key = get_symbol("_xor:");
  cls_obj->instance_methods->bindings[10].val = list(3,
						     convert_native_fn_to_object((nativefn)boolean_xor),
						     NIL,
						     convert_int_to_object(1));

  cls_obj->instance_methods->bindings[11].key = get_symbol("_printString");
  cls_obj->instance_methods->bindings[11].val = list(3,
						     convert_native_fn_to_object((nativefn)boolean_print_string),
						     NIL,
						     convert_int_to_object(0));

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  Boolean = convert_class_object_to_object_ptr(cls_obj);
}
