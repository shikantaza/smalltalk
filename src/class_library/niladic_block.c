#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../global_decls.h"
#include "../stack.h"

OBJECT_PTR NiladicBlock;

extern binding_env_t *g_top_level;
extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Object;
extern stack_type *g_exception_environment;
extern stack_type *g_call_chain;
extern OBJECT_PTR g_idclo;
extern OBJECT_PTR g_msg_snd_closure;

extern OBJECT_PTR VALUE_SELECTOR;
/*

These two methods to be included in
the class library (smalltalk.st)

Smalltalk addInstanceMethod: #ensure: withBody:
  [ :ensureBlock |
    | result |
    result := self ifCurtailed: ensureBlock.
    ensureBlock value.
    ^result ]
  toClass: #NiladicBlock

Smalltalk addInstanceMethod: #ifCurtailed: withBody:
  [ :curtailBlock |
    | result curtailed |
    curtailed := true.
    [result := self value. curtailed := false] ensure: [curtailed ifTrue: [curtailBlock value]].
    ^result]
  toClass: #NiladicBlock

 */

OBJECT_PTR niladic_block_arg_count(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));
  
  return invoke_cont_on_val(cont, convert_int_to_object(0));
}

OBJECT_PTR niladic_block_value(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));

  return invoke_cont_on_val(receiver, cont);
}

OBJECT_PTR niladic_block_on_do(OBJECT_PTR closure,
			       OBJECT_PTR exception_selector,
			       OBJECT_PTR exception_action,
			       OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLASS_OBJECT(exception_selector));
  assert(IS_CLOSURE_OBJECT(exception_action));
  assert(IS_CLOSURE_OBJECT(cont));

  exception_handler_t *eh = create_exception_handler(receiver,
						     exception_selector,
						     exception_action,
						     g_exception_environment,
						     cont);
  stack_push(g_exception_environment, eh);

  return niladic_block_value(closure, cont);
}

call_chain_entry_t *is_termination_block_not_invoked(OBJECT_PTR termination_block)
{
  call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
  int count = stack_count(g_call_chain);

  int i = count - 1;

  while(i >= 0)
  {
    call_chain_entry_t *entry = entries[i];

    if(entry->termination_blk_closure == termination_block &&
       entry->termination_blk_invoked == false)
      return entry;

    i--;
  }
  return NULL;
}

OBJECT_PTR niladic_block_ensure(OBJECT_PTR closure,
				OBJECT_PTR ensure_block,
				OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(ensure_block));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR ret = message_send(g_msg_snd_closure,
				receiver,
				get_symbol("_ifCurtailed:"),
				convert_int_to_object(1),
				ensure_block,
				g_idclo);

  //if the ensure: block has already been invoked because
  //of an exception unwinding, don't invoke it again

  call_chain_entry_t *e = is_termination_block_not_invoked(ensure_block);

  if(e)
  {
    e->termination_blk_invoked = true;
    OBJECT_PTR discarded_ret = message_send(g_msg_snd_closure,
					    ensure_block,
					    VALUE_SELECTOR,
					    convert_int_to_object(0),
					    g_idclo);
  }

  nativefn nf3 = (nativefn)extract_native_fn(cont);
  return nf3(cont, ret);
}

OBJECT_PTR niladic_block_ifcurtailed(OBJECT_PTR closure,
				     OBJECT_PTR curtailed_block,
				     OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(curtailed_block));
  assert(IS_CLOSURE_OBJECT(cont));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);
  entry->termination_blk_closure = curtailed_block;
  entry->termination_blk_invoked = false;
  
  //nativefn nf1 = (nativefn)extract_native_fn(receiver);
  //OBJECT_PTR ret = nf1(receiver, g_idclo);
  OBJECT_PTR ret = message_send(g_msg_snd_closure,
				receiver,
				VALUE_SELECTOR,
				convert_int_to_object(0),
				cont);
  
  //nativefn nf2 = (nativefn)extract_native_fn(cont);
  //return nf2(cont, ret);
  return ret;
}

void create_NiladicBlock()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_NiladicBlock(): Unable to allocate memory\n");
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
  cls_obj->instance_methods->count = 5;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("argumentCount");
  cls_obj->instance_methods->bindings[0].val = list(3,
						    convert_native_fn_to_object((nativefn)niladic_block_arg_count),
						    NIL,
						    convert_int_to_object(0));

  cls_obj->instance_methods->bindings[1].key = get_symbol("_value");
  cls_obj->instance_methods->bindings[1].val = list(3,
						    convert_native_fn_to_object((nativefn)niladic_block_value),
						    NIL,
						    convert_int_to_object(0));

  cls_obj->instance_methods->bindings[2].key = get_symbol("_on:do:");
  cls_obj->instance_methods->bindings[2].val = list(3,
						    convert_native_fn_to_object((nativefn)niladic_block_on_do),
						    NIL,
						    convert_int_to_object(2));
  
  cls_obj->instance_methods->bindings[3].key = get_symbol("_ensure:");
  cls_obj->instance_methods->bindings[3].val = list(3,
						    convert_native_fn_to_object((nativefn)niladic_block_ensure),
						    NIL,
						    convert_int_to_object(1));
  
  cls_obj->instance_methods->bindings[4].key = get_symbol("_ifCurtailed:");
  cls_obj->instance_methods->bindings[4].val = list(3,
						    convert_native_fn_to_object((nativefn)niladic_block_ifcurtailed),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  NiladicBlock =  convert_class_object_to_object_ptr(cls_obj);
}
