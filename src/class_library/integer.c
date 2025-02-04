#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../smalltalk.h"

nativefn extract_native_fn(OBJECT_PTR);
OBJECT_PTR convert_int_to_object(int);
int get_int_value(OBJECT_PTR);

OBJECT_PTR get_symbol(char *);
//OBJECT_PTR create_closure(OBJECT_PTR, nativefn, ...);
OBJECT_PTR convert_class_object_to_object_ptr(class_object_t *);
OBJECT_PTR convert_native_fn_to_object(nativefn);

OBJECT_PTR get_binding_val(binding_env_t *, OBJECT_PTR);

extern binding_env_t *top_level;

extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

extern OBJECT_PTR Object;
OBJECT_PTR Integer;

OBJECT_PTR signal_exception(OBJECT_PTR);
OBJECT_PTR get_smalltalk_symbol(char *);

BOOLEAN get_top_level_val(OBJECT_PTR, OBJECT_PTR *);

OBJECT_PTR message_send(OBJECT_PTR,
			OBJECT_PTR,
			OBJECT_PTR,
			OBJECT_PTR,
			...);

OBJECT_PTR create_message_send_closure();

extern OBJECT_PTR idclo;

OBJECT_PTR new_object_internal(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR convert_fn_to_closure(nativefn fn);

//workaround for variadic function arguments
//getting clobbered in ARM64
typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR plus(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));
  
  assert(IS_INTEGER_OBJECT(receiver));

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to plus\n");
#endif
  
  assert(IS_INTEGER_OBJECT(arg));

  assert(IS_CLOSURE_OBJECT(cont));
  
  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_int_to_object(get_int_value(receiver) + get_int_value(arg)));
}

OBJECT_PTR minus(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));
  
  assert(IS_INTEGER_OBJECT(receiver));

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to minus\n");
#endif
  
  assert(IS_INTEGER_OBJECT(arg));

  assert(IS_CLOSURE_OBJECT(cont));
  
  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_int_to_object(get_int_value(receiver) - get_int_value(arg)));
}

OBJECT_PTR times(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));
  
  assert(IS_INTEGER_OBJECT(receiver));

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to times\n");
#endif
  
  assert(IS_INTEGER_OBJECT(arg));

  assert(IS_CLOSURE_OBJECT(cont));
  
  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_int_to_object(get_int_value(receiver) * get_int_value(arg)));
}

OBJECT_PTR divided_by(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));
  
  assert(IS_INTEGER_OBJECT(receiver));

#ifdef DEBUG  
  print_object(arg); printf(" is the arg passed to divide_by\n");
#endif
  
  assert(IS_INTEGER_OBJECT(arg));

  assert(IS_CLOSURE_OBJECT(cont));

  if(get_int_value(arg) == 0)
  {
    OBJECT_PTR ret;
    assert(get_top_level_val(get_symbol("ZeroDivide"), &ret));

    OBJECT_PTR zero_divide_class_obj = car(ret);

    OBJECT_PTR exception_obj = new_object_internal(zero_divide_class_obj,
						   convert_fn_to_closure((nativefn)new_object_internal),
						   idclo);
    return signal_exception(exception_obj);
  }
  
  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_int_to_object(get_int_value(receiver) / get_int_value(arg)));
}

OBJECT_PTR eq(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));

  assert(IS_INTEGER_OBJECT(receiver));

#ifdef DEBUG
  print_object(arg); printf(" is the arg passed to divide_by\n");
#endif

  assert(IS_INTEGER_OBJECT(arg));

  assert(IS_CLOSURE_OBJECT(cont));

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, (receiver == arg) ? TRUE : FALSE );
}

void create_Integer()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Integer: Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("Integer");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  
  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 5;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("+");
  cls_obj->instance_methods->bindings[0].val = list(3,
						    convert_native_fn_to_object((nativefn)plus),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->instance_methods->bindings[1].key = get_symbol("-");
  cls_obj->instance_methods->bindings[1].val = list(3,
						    convert_native_fn_to_object((nativefn)minus),
						    NIL,
						    convert_int_to_object(1));
					
  
  cls_obj->instance_methods->bindings[2].key = get_symbol("*");
  cls_obj->instance_methods->bindings[2].val = list(3,
						    convert_native_fn_to_object((nativefn)times),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->instance_methods->bindings[3].key = get_symbol("/");
  cls_obj->instance_methods->bindings[3].val = list(3,
						    convert_native_fn_to_object((nativefn)divided_by),
						    NIL,
						    convert_int_to_object(1));
  
  cls_obj->instance_methods->bindings[4].key = get_symbol("=");
  cls_obj->instance_methods->bindings[4].val = list(3,
						    convert_native_fn_to_object((nativefn)eq),
						    NIL,
						    convert_int_to_object(1));

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  Integer =  convert_class_object_to_object_ptr(cls_obj);
}
