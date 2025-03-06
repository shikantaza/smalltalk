#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "../../smalltalk.h"
#include "../../util.h"

nativefn extract_native_fn(OBJECT_PTR);
OBJECT_PTR convert_int_to_object(int);
int get_int_value(OBJECT_PTR);

extern OBJECT_PTR msg_snd_closure;

extern stack_type *exception_contexts;

OBJECT_PTR new_object_internal(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR convert_fn_to_closure(nativefn fn);

OBJECT_PTR get_binding_val(binding_env_t *, OBJECT_PTR);

extern binding_env_t *top_level;

extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;

extern OBJECT_PTR Object;
extern OBJECT_PTR nil;

OBJECT_PTR signal_exception(OBJECT_PTR);
OBJECT_PTR get_smalltalk_symbol(char *);

BOOLEAN get_top_level_val(OBJECT_PTR, OBJECT_PTR *);

OBJECT_PTR message_send(OBJECT_PTR,
			OBJECT_PTR,
			OBJECT_PTR,
			OBJECT_PTR,
			...);

OBJECT_PTR get_symbol(char *);
OBJECT_PTR convert_class_object_to_object_ptr(class_object_t *);
OBJECT_PTR convert_native_fn_to_object(nativefn);

typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

extern OBJECT_PTR idclo;

OBJECT_PTR Array;

OBJECT_PTR convert_array_object_to_object_ptr(array_object_t *);

OBJECT_PTR array_new(OBJECT_PTR closure, OBJECT_PTR size, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));

  assert(receiver == Array);

  if(!IS_INTEGER_OBJECT(size))
  {
    stack_push(exception_contexts, (void *)cont);

    OBJECT_PTR ret;
    assert(get_top_level_val(get_symbol("InvalidArgument"), &ret));

    OBJECT_PTR invalid_arg_class_obj = car(ret);

    OBJECT_PTR invalid_arg_excp_obj = new_object_internal(invalid_arg_class_obj,
							  convert_fn_to_closure((nativefn)new_object_internal),
							  idclo);
    return message_send(msg_snd_closure,
			invalid_arg_excp_obj,
			get_symbol("signal_"),
			convert_int_to_object(0),
			cont);
  }

  int count = get_int_value(size);

  if(count <= 0)
  {
    stack_push(exception_contexts, (void *)cont);

    OBJECT_PTR ret;
    assert(get_top_level_val(get_symbol("InvalidArgument"), &ret));

    OBJECT_PTR invalid_arg_class_obj = car(ret);

    OBJECT_PTR invalid_arg_excp_obj = new_object_internal(invalid_arg_class_obj,
							  convert_fn_to_closure((nativefn)new_object_internal),
							  idclo);
    return message_send(msg_snd_closure,
			invalid_arg_excp_obj,
			get_symbol("signal_"),
			convert_int_to_object(0),
			cont);
  }

  array_object_t *obj = (array_object_t *)GC_MALLOC(sizeof(array_object_t));

  assert(obj);

  obj->nof_elements = count;
  obj->elements = (OBJECT_PTR *)GC_MALLOC(count * sizeof(OBJECT_PTR));
  assert(obj->elements);

  int i;

  for(i=0; i<count; i++)
    obj->elements[i] = nil;

  assert(IS_CLOSURE_OBJECT(cont));

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_array_object_to_object_ptr(obj));
}

OBJECT_PTR array_at_put(OBJECT_PTR closure, OBJECT_PTR index, OBJECT_PTR val, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));

  if(!IS_INTEGER_OBJECT(index))
  {
    stack_push(exception_contexts, (void *)cont);

    OBJECT_PTR ret;
    assert(get_top_level_val(get_symbol("InvalidArgument"), &ret));

    OBJECT_PTR invalid_arg_class_obj = car(ret);

    OBJECT_PTR invalid_arg_excp_obj = new_object_internal(invalid_arg_class_obj,
							  convert_fn_to_closure((nativefn)new_object_internal),
							  idclo);
    return message_send(msg_snd_closure,
			invalid_arg_excp_obj,
			get_symbol("signal_"),
			convert_int_to_object(0),
			cont);
  }

  int idx = get_int_value(index);

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  if(idx <= 0 || idx > obj->nof_elements)
  {
    stack_push(exception_contexts, (void *)cont);

    OBJECT_PTR ret;
    assert(get_top_level_val(get_symbol("IndexOutofBounds"), &ret));

    OBJECT_PTR invalid_arg_class_obj = car(ret);

    OBJECT_PTR invalid_arg_excp_obj = new_object_internal(invalid_arg_class_obj,
							  convert_fn_to_closure((nativefn)new_object_internal),
							  idclo);
    return message_send(msg_snd_closure,
			invalid_arg_excp_obj,
			get_symbol("signal_"),
			convert_int_to_object(0),
			cont);
  }

  obj->elements[idx-1] = val;

  assert(IS_CLOSURE_OBJECT(cont));

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, receiver);
}

OBJECT_PTR array_at(OBJECT_PTR closure, OBJECT_PTR index, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));

  if(!IS_INTEGER_OBJECT(index))
  {
    stack_push(exception_contexts, (void *)cont);

    OBJECT_PTR ret;
    assert(get_top_level_val(get_symbol("InvalidArgument"), &ret));

    OBJECT_PTR invalid_arg_class_obj = car(ret);

    OBJECT_PTR invalid_arg_excp_obj = new_object_internal(invalid_arg_class_obj,
							  convert_fn_to_closure((nativefn)new_object_internal),
							  idclo);
    return message_send(msg_snd_closure,
			invalid_arg_excp_obj,
			get_symbol("signal_"),
			convert_int_to_object(0),
			cont);
  }

  int idx = get_int_value(index);

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  if(idx <= 0 || idx > obj->nof_elements)
  {
    stack_push(exception_contexts, (void *)cont);

    OBJECT_PTR ret;
    assert(get_top_level_val(get_symbol("IndexOutofBounds"), &ret));

    OBJECT_PTR invalid_arg_class_obj = car(ret);

    OBJECT_PTR invalid_arg_excp_obj = new_object_internal(invalid_arg_class_obj,
							  convert_fn_to_closure((nativefn)new_object_internal),
							  idclo);
    return message_send(msg_snd_closure,
			invalid_arg_excp_obj,
			get_symbol("signal_"),
			convert_int_to_object(0),
			cont);
  }

  assert(IS_CLOSURE_OBJECT(cont));

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, obj->elements[idx-1]);
}

OBJECT_PTR array_size(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  assert(IS_CLOSURE_OBJECT(cont));

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_int_to_object((int)(obj->nof_elements)));
}

OBJECT_PTR array_do(OBJECT_PTR closure, OBJECT_PTR operation, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));

  array_object_t *obj = (array_object_t *)extract_ptr(receiver);

  if(!IS_CLOSURE_OBJECT(operation))
  {
    stack_push(exception_contexts, (void *)cont);

    OBJECT_PTR ret;
    assert(get_top_level_val(get_symbol("InvalidArgument"), &ret));

    OBJECT_PTR invalid_arg_class_obj = car(ret);

    OBJECT_PTR invalid_arg_excp_obj = new_object_internal(invalid_arg_class_obj,
							  convert_fn_to_closure((nativefn)new_object_internal),
							  idclo);
    return message_send(msg_snd_closure,
			invalid_arg_excp_obj,
			get_symbol("signal_"),
			convert_int_to_object(0),
			cont);
  }

  unsigned int size = obj->nof_elements;

  int i;

  for(i=0; i<size; i++)
  {
    message_send(msg_snd_closure,
                 operation,
		 get_symbol("value:_"),
		 convert_int_to_object(1),
		 obj->elements[i],
		 idclo);
  }

  assert(IS_CLOSURE_OBJECT(cont));

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, receiver);
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
  cls_obj->instance_methods->count = 4;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("at:put:_");
  cls_obj->instance_methods->bindings[0].val = list(3,
						    convert_native_fn_to_object((nativefn)array_at_put),
						    NIL,
						    convert_int_to_object(2));

  cls_obj->instance_methods->bindings[1].key = get_symbol("at:_");
  cls_obj->instance_methods->bindings[1].val = list(3,
						    convert_native_fn_to_object((nativefn)array_at),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->instance_methods->bindings[2].key = get_symbol("size_");
  cls_obj->instance_methods->bindings[2].val = list(3,
						    convert_native_fn_to_object((nativefn)array_size),
						    NIL,
						    convert_int_to_object(0));

  cls_obj->instance_methods->bindings[3].key = get_symbol("do:_");
  cls_obj->instance_methods->bindings[3].val = list(3,
						    convert_native_fn_to_object((nativefn)array_do),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 1;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));

  cls_obj->class_methods->bindings[0].key = get_symbol("new:_");
  cls_obj->class_methods->bindings[0].val = list(3,
						 convert_native_fn_to_object((nativefn)array_new),
						 NIL,
						 convert_int_to_object(1));

  Array =  convert_class_object_to_object_ptr(cls_obj);
}
