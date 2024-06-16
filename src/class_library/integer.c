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
OBJECT_PTR create_closure(OBJECT_PTR, nativefn, ...);
OBJECT_PTR convert_class_object_to_object_ptr(class_object_t *);
OBJECT_PTR convert_native_fn_to_object(nativefn);

OBJECT_PTR get_binding_val(binding_env_t *, OBJECT_PTR);

extern binding_env_t *top_level;

extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;

OBJECT_PTR Integer;

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

OBJECT_PTR minus(OBJECT_PTR closure, ...)
{
  va_list ap;
  
  va_start(ap, closure);

  OBJECT_PTR receiver = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_INTEGER_OBJECT(receiver));

  OBJECT_PTR arg = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_INTEGER_OBJECT(arg));

  OBJECT_PTR cont = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_CLOSURE_OBJECT(cont));
  
  va_end(ap);

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_int_to_object(get_int_value(receiver) - get_int_value(arg)));
}

OBJECT_PTR times(OBJECT_PTR closure, ...)
{
  va_list ap;
  
  va_start(ap, closure);

  OBJECT_PTR receiver = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_INTEGER_OBJECT(receiver));

  OBJECT_PTR arg = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_INTEGER_OBJECT(arg));

  OBJECT_PTR cont = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_CLOSURE_OBJECT(cont));
  
  va_end(ap);

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_int_to_object(get_int_value(receiver) * get_int_value(arg)));
}

OBJECT_PTR divided_by(OBJECT_PTR closure, ...)
{
  va_list ap;
  
  va_start(ap, closure);

  OBJECT_PTR receiver = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_INTEGER_OBJECT(receiver));

  OBJECT_PTR arg = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_INTEGER_OBJECT(arg));

  OBJECT_PTR cont = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  assert(IS_CLOSURE_OBJECT(cont));
  
  va_end(ap);

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  return nf(cont, convert_int_to_object(get_int_value(receiver) / get_int_value(arg)));
}

void create_Integer()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Integer: Unable to allocate memory\n");
    exit(1);
  }

  //TODO: initialize cls_obj->parent_class_object to Object
  //once we define Object
  cls_obj->name = GC_strdup("Integer");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->nof_shared_vars = 0;
  cls_obj->shared_vars = NULL;
  
  cls_obj->nof_instance_methods = 4;    
  cls_obj->instance_methods = (binding_t *)GC_MALLOC(cls_obj->nof_instance_methods *
                                                     sizeof(binding_t));

  cls_obj->instance_methods[0].key = get_symbol("+");
  cls_obj->instance_methods[0].val = cons(convert_native_fn_to_object((nativefn)plus), NIL);

  cls_obj->instance_methods[1].key = get_symbol("-");
  cls_obj->instance_methods[1].val = cons(convert_native_fn_to_object((nativefn)minus), NIL);

  cls_obj->instance_methods[2].key = get_symbol("*");
  cls_obj->instance_methods[2].val = cons(convert_native_fn_to_object((nativefn)times), NIL);

  cls_obj->instance_methods[3].key = get_symbol("/");
  cls_obj->instance_methods[3].val = cons(convert_native_fn_to_object((nativefn)divided_by), NIL);
  
  cls_obj->nof_class_methods = 0;
  cls_obj->class_methods = NULL;

  Integer =  convert_class_object_to_object_ptr(cls_obj);
}
