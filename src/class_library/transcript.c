#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

OBJECT_PTR Transcript;

extern binding_env_t *g_top_level;
extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Object;

extern stack_type *g_call_chain;

OBJECT_PTR transcript_show(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to transcript_show()\n");
#endif
  
  assert(IS_CLOSURE_OBJECT(cont));
  
  print_object(arg);

  pop_if_top(entry);

  return invoke_cont_on_val(cont, NIL);
}

OBJECT_PTR transcript_cr(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  assert(IS_CLOSURE_OBJECT(cont));
  
  printf("\n");
  
  pop_if_top(entry);

  return invoke_cont_on_val(cont, NIL);
}

void create_Transcript()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Transcript(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("Transcript");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  
  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 0;    
  cls_obj->instance_methods->bindings = NULL;

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 2;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));

  cls_obj->class_methods->bindings[0].key = get_symbol("_show:");
  cls_obj->class_methods->bindings[0].val = list(3,
						 convert_native_fn_to_object((nativefn)transcript_show),
						 NIL,
						 convert_int_to_object(1));				

  cls_obj->class_methods->bindings[1].key = get_symbol("_cr");
  cls_obj->class_methods->bindings[1].val = list(3,
						 convert_native_fn_to_object((nativefn)transcript_cr),
						 NIL,
						 convert_int_to_object(0));
  
  Transcript =  convert_class_object_to_object_ptr(cls_obj);
}
