#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "../../global_decls.h"
#include "../../util.h"

typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR OrderedCollection;
OBJECT_PTR EmptyCollection;

unsigned int DEFAULT_COLLECTION_SIZE = 500;

extern binding_env_t *g_top_level;
extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Object;

extern OBJECT_PTR InvalidArgument;
extern OBJECT_PTR IndexOutofBounds;

extern stack_type *g_call_chain;

extern OBJECT_PTR g_msg_snd_closure;
extern OBJECT_PTR g_idclo;
extern OBJECT_PTR VALUE1_SELECTOR;

OBJECT_PTR ordered_collection_new(OBJECT_PTR closure, OBJECT_PTR cont)
{
  return new_object_internal(OrderedCollection, convert_fn_to_closure((nativefn)new_object_internal), cont);
}

OBJECT_PTR ordered_collection_initialize(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  object_t *coll_obj = (object_t *)extract_ptr(receiver);

  assert(coll_obj->class_object == OrderedCollection);

  //array_object_t *obj = (array_object_t *)GC_MALLOC(sizeof(array_object_t));
  array_object_t *obj;

  if(allocate_memory((void **)&obj, sizeof(array_object_t)))
  {
    printf("ordered_collection_initialize(): Unable to allocate memory\n");
    exit(1);
  }

  assert(obj);

  obj->nof_elements = DEFAULT_COLLECTION_SIZE;
  obj->elements = (OBJECT_PTR *)GC_MALLOC(obj->nof_elements * sizeof(OBJECT_PTR));

  int i;

  for(i=0; i<DEFAULT_COLLECTION_SIZE; i++)
    obj->elements[i] = NIL;

  update_binding(coll_obj->instance_vars, get_symbol("arr"), cons(convert_array_object_to_object_ptr(obj), NIL));
  update_binding(coll_obj->instance_vars, get_symbol("size"), cons(convert_int_to_object(0), NIL));

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, receiver);
}

OBJECT_PTR ordered_collection_size(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  object_t *coll_obj = (object_t *)extract_ptr(receiver);

  assert(coll_obj->class_object == OrderedCollection);

  OBJECT_PTR size = car(get_binding(coll_obj->instance_vars, get_symbol("size")));
  assert(size);
  assert(IS_INTEGER_OBJECT(size));

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, size);
}

OBJECT_PTR ordered_collection_add(OBJECT_PTR closure, OBJECT_PTR elem, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  object_t *coll_obj = (object_t *)extract_ptr(receiver);

  assert(coll_obj->class_object == OrderedCollection);

  OBJECT_PTR arr = car(get_binding(coll_obj->instance_vars, get_symbol("arr")));

  assert(arr); //get_binding returns NULL if the binding is not found
  assert(IS_ARRAY_OBJECT(arr));

  array_object_t *obj = (array_object_t *)extract_ptr(arr);

  OBJECT_PTR size = car(get_binding(coll_obj->instance_vars, get_symbol("size")));
  assert(size);
  assert(IS_INTEGER_OBJECT(size));

  int size_val = get_int_value(size);

  if(size_val == obj->nof_elements)
  {
    obj->nof_elements += DEFAULT_COLLECTION_SIZE;
    obj->elements = (OBJECT_PTR *)GC_REALLOC(obj->elements, obj->nof_elements * sizeof(OBJECT_PTR));
  }

  obj->elements[size_val] = elem;

  update_binding(coll_obj->instance_vars, get_symbol("size"), cons(convert_int_to_object(size_val+1), NIL));

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, receiver);
}

//TODO: move this to superclass (see also Array class)
OBJECT_PTR ordered_collection_at(OBJECT_PTR closure, OBJECT_PTR index, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  object_t *coll_obj = (object_t *)extract_ptr(receiver);

  assert(coll_obj->class_object == OrderedCollection);

  OBJECT_PTR arr = car(get_binding(coll_obj->instance_vars, get_symbol("arr")));

  assert(arr); //get_binding returns NULL if the binding is not found
  assert(IS_ARRAY_OBJECT(arr));

  array_object_t *obj = (array_object_t *)extract_ptr(arr);

  if(!IS_INTEGER_OBJECT(index))
    return create_and_signal_exception(InvalidArgument, cont);

  OBJECT_PTR size = car(get_binding(coll_obj->instance_vars, get_symbol("size")));
  assert(size);
  assert(IS_INTEGER_OBJECT(size));

  unsigned int size_val = get_int_value(size);

  int idx = get_int_value(index);

  if(idx <= 0 || idx > size_val)
    return create_and_signal_exception(IndexOutofBounds, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, obj->elements[idx-1]);
}

OBJECT_PTR ordered_collection_add_last(OBJECT_PTR closure, OBJECT_PTR elem, OBJECT_PTR cont)
{
  return ordered_collection_add(closure, elem, cont);
}

OBJECT_PTR ordered_collection_remove_last(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  object_t *coll_obj = (object_t *)extract_ptr(receiver);

  assert(coll_obj->class_object == OrderedCollection);

  OBJECT_PTR arr = car(get_binding(coll_obj->instance_vars, get_symbol("arr")));

  assert(arr); //get_binding returns NULL if the binding is not found
  assert(IS_ARRAY_OBJECT(arr));

  array_object_t *obj = (array_object_t *)extract_ptr(arr);

  OBJECT_PTR size = car(get_binding(coll_obj->instance_vars, get_symbol("size")));
  assert(size);
  assert(IS_INTEGER_OBJECT(size));

  unsigned int size_val = get_int_value(size);

  if(size_val == 0)
    return create_and_signal_exception(EmptyCollection, cont);

  update_binding(coll_obj->instance_vars, get_symbol("size"), cons(convert_int_to_object(size_val-1), NIL));

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, obj->elements[size_val-1]);
}

OBJECT_PTR ordered_collection_do(OBJECT_PTR closure, OBJECT_PTR operation, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  object_t *coll_obj = (object_t *)extract_ptr(receiver);

  assert(coll_obj->class_object == OrderedCollection);

  OBJECT_PTR arr = car(get_binding(coll_obj->instance_vars, get_symbol("arr")));

  assert(arr); //get_binding returns NULL if the binding is not found
  assert(IS_ARRAY_OBJECT(arr));

  array_object_t *obj = (array_object_t *)extract_ptr(arr);

  if(!IS_CLOSURE_OBJECT(operation))
    return create_and_signal_exception(InvalidArgument, cont);

  OBJECT_PTR size = car(get_binding(coll_obj->instance_vars, get_symbol("size")));
  assert(size);
  assert(IS_INTEGER_OBJECT(size));

  unsigned int size_val = get_int_value(size);

  int i;

  OBJECT_PTR ret;

  for(i=0; i<size_val; i++)
  {
    ret = message_send(g_msg_snd_closure,
		       operation,
		       NIL,
		       VALUE1_SELECTOR,
		       convert_int_to_object(1),
		       obj->elements[i],
		       g_idclo);

    if(call_chain_entry_exists(entry))
      continue;
    else
      break;
  }

  assert(IS_CLOSURE_OBJECT(cont));

  if(call_chain_entry_exists(entry))
  {
    pop_if_top(entry);
    return invoke_cont_on_val(cont, receiver);
  }
  else
    return ret;
}

void create_OrderedCollection()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_OrderedCollection(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("OrderedCollection");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;

  cls_obj->nof_instance_vars = 2;
  cls_obj->inst_vars = (OBJECT_PTR *)GC_MALLOC(cls_obj->nof_instance_vars * sizeof(OBJECT_PTR));

  cls_obj->inst_vars[0] = get_symbol("arr");
  cls_obj->inst_vars[1] = get_symbol("size");

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;

  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 7;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_initialize");
  cls_obj->instance_methods->bindings[0].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)ordered_collection_initialize),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[1].key = get_symbol("_size");
  cls_obj->instance_methods->bindings[1].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)ordered_collection_size),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[2].key = get_symbol("_add:");
  cls_obj->instance_methods->bindings[2].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)ordered_collection_add),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[3].key = get_symbol("_at:");
  cls_obj->instance_methods->bindings[3].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)ordered_collection_at),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[4].key = get_symbol("_addLast:");
  cls_obj->instance_methods->bindings[4].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)ordered_collection_add_last),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[5].key = get_symbol("_removeLast");
  cls_obj->instance_methods->bindings[5].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)ordered_collection_remove_last),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[6].key = get_symbol("_do:");
  cls_obj->instance_methods->bindings[6].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)ordered_collection_do),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 1;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));

  cls_obj->class_methods->bindings[0].key = get_symbol("_new");
  cls_obj->class_methods->bindings[0].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)ordered_collection_new),
						 NIL, NIL,
						 0, NIL, NULL);

  OrderedCollection =  convert_class_object_to_object_ptr(cls_obj);
}
