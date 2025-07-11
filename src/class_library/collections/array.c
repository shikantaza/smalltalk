#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "../../global_decls.h"
#include "../../util.h"

typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR Array;

extern OBJECT_PTR g_msg_snd_closure;
extern binding_env_t *g_top_level;
extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Object;
extern OBJECT_PTR Nil;
extern OBJECT_PTR InvalidArgument;
extern OBJECT_PTR IndexOutofBounds;
extern OBJECT_PTR g_idclo;

extern OBJECT_PTR VALUE_SELECTOR;
extern OBJECT_PTR VALUE1_SELECTOR;

extern stack_type *g_call_chain;

extern BOOLEAN g_eval_aborted;

OBJECT_PTR array_new(OBJECT_PTR closure, OBJECT_PTR size, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  assert(receiver == Array);

  if(!IS_INTEGER_OBJECT(size))
    return create_and_signal_exception(InvalidArgument, cont);

  int count = get_int_value(size);

  if(count <= 0)
    return create_and_signal_exception(InvalidArgument, cont);

  //array_object_t *obj = (array_object_t *)GC_MALLOC(sizeof(array_object_t));
  array_object_t *obj;

  if(allocate_memory((void **)&obj, sizeof(array_object_t)))
  {
    printf("array_new(): Unable to allocate memory\n");
    exit(1);
  }

  //assert(obj);

  obj->nof_elements = count;
  obj->elements = (OBJECT_PTR *)GC_MALLOC(count * sizeof(OBJECT_PTR));
  assert(obj->elements);

  int i;

  for(i=0; i<count; i++)
    obj->elements[i] = NIL;

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, convert_array_object_to_object_ptr(obj));
}

//TODO: move this to superclass
OBJECT_PTR array_at_put(OBJECT_PTR closure, OBJECT_PTR index, OBJECT_PTR val, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  if(!IS_INTEGER_OBJECT(index))
    return create_and_signal_exception(InvalidArgument, cont);

  int idx = get_int_value(index);

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  if(idx <= 0 || idx > obj->nof_elements)
    return create_and_signal_exception(IndexOutofBounds, cont);

  obj->elements[idx-1] = val;

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, receiver);
}

//TODO: move this to superclass
OBJECT_PTR array_at(OBJECT_PTR closure, OBJECT_PTR index, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  if(!IS_INTEGER_OBJECT(index))
    return create_and_signal_exception(InvalidArgument, cont);

  int idx = get_int_value(index);

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  if(idx <= 0 || idx > obj->nof_elements)
    return create_and_signal_exception(IndexOutofBounds, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, obj->elements[idx-1]);
}

OBJECT_PTR array_size(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, convert_int_to_object((int)(obj->nof_elements)));
}

OBJECT_PTR array_do(OBJECT_PTR closure, OBJECT_PTR operation, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  if(!IS_CLOSURE_OBJECT(operation))
    return create_and_signal_exception(InvalidArgument, cont);

  unsigned int size = obj->nof_elements;

  int i;

  OBJECT_PTR ret;

  for(i=0; i<size; i++)
  {
    ret = message_send(g_msg_snd_closure,
		       operation,
		       NIL,
		       VALUE1_SELECTOR,
		       convert_int_to_object(1),
		       obj->elements[i],
		       g_idclo);

    if(call_chain_entry_exists(entry) && !g_eval_aborted)
      continue;
    else
      break;
  }

  assert(IS_CLOSURE_OBJECT(cont));

  if(call_chain_entry_exists(entry) && !g_eval_aborted)
  {
    pop_if_top(entry);
    return invoke_cont_on_val(cont, receiver);
  }
  else
    return ret;
}

OBJECT_PTR array_do_separated_by(OBJECT_PTR closure, OBJECT_PTR operation, OBJECT_PTR separator, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  if(!IS_CLOSURE_OBJECT(operation))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_CLOSURE_OBJECT(separator))
    return create_and_signal_exception(InvalidArgument, cont);

  unsigned int size = obj->nof_elements;

  int i;

  OBJECT_PTR ret1, ret2;

  BOOLEAN ret_from_do, ret_from_separated_by;

  for(i=0; i<size; i++)
  {
    ret1 = message_send(g_msg_snd_closure,
			operation,
			NIL,
			VALUE1_SELECTOR,
			convert_int_to_object(1),
			obj->elements[i],
			g_idclo);

    if(!call_chain_entry_exists(entry))
    {
      ret_from_do = true;
      break;
    }

    if(i < size - 1)
    {
      ret2 = message_send(g_msg_snd_closure,
			  separator,
			  NIL,
			  VALUE_SELECTOR,
			  convert_int_to_object(0),
			  g_idclo);

      if(!call_chain_entry_exists(entry))
      {
	ret_from_separated_by = true;
	break;
      }
    }
  }

  assert(IS_CLOSURE_OBJECT(cont));

  if(call_chain_entry_exists(entry))
  {
    pop_if_top(entry);
    return invoke_cont_on_val(cont, receiver);
  }
  else
  {
    if(ret_from_do)
      return ret1;
    else if(ret_from_separated_by)
      return ret2;
    else
      assert(false);
  }
}

void create_Array()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Array(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("Array");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;

  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;

  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 5;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_at:put:");
  cls_obj->instance_methods->bindings[0].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)array_at_put),
						    NIL, NIL,
						    2, NIL, NULL);

  cls_obj->instance_methods->bindings[1].key = get_symbol("_at:");
  cls_obj->instance_methods->bindings[1].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)array_at),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[2].key = get_symbol("_size");
  cls_obj->instance_methods->bindings[2].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)array_size),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[3].key = get_symbol("_do:");
  cls_obj->instance_methods->bindings[3].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)array_do),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[4].key = get_symbol("_do:separatedBy:");
  cls_obj->instance_methods->bindings[4].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)array_do_separated_by),
						    NIL, NIL,
						    2, NIL, NULL);

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 1;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));

  cls_obj->class_methods->bindings[0].key = get_symbol("_new:");
  cls_obj->class_methods->bindings[0].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)array_new),
						 NIL, NIL,
						 1, NIL, NULL);

  Array =  convert_class_object_to_object_ptr(cls_obj);
}
