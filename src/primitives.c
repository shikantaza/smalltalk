#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "smalltalk.h"

BOOLEAN IS_NATIVE_FN_OBJECT(OBJECT_PTR);
BOOLEAN IS_CLOSURE_OBJECT(OBJECT_PTR);
uintptr_t extract_ptr(OBJECT_PTR);
int get_int_value(OBJECT_PTR);
void set_heap(uintptr_t, unsigned int, OBJECT_PTR);
uintptr_t object_alloc(int, int);
OBJECT_PTR last_cell(OBJECT_PTR);

extern OBJECT_PTR NIL;

extern OBJECT_PTR method_call_stack;

/* OBJECT_PTR message_send(OBJECT_PTR receiver, ...) */
/* { */
/*   //TODO: */
/*   return NIL; */
/* } */

void save_continuation(OBJECT_PTR cont)
{
  //TODO
  //printf("pushing: "); print_object(car(cont)); printf("\n");
}

nativefn extract_native_fn(OBJECT_PTR);

void set_most_recent_closure(OBJECT_PTR clo)
{
  //TODO
#ifdef DEBUG  
  printf("popping: %p\n", extract_native_fn(clo));
#endif
}

nativefn extract_native_fn(OBJECT_PTR closure)
{
#ifdef DEBUG  
  print_object(closure); printf(" is passed to extract_native_fn\n");
#endif  
  assert(IS_CLOSURE_OBJECT(closure));

  //TODO: is an unmet dependencies check
  //needed here?
         
  OBJECT_PTR nativefn_obj = car(closure);  
  assert(IS_NATIVE_FN_OBJECT(nativefn_obj));

  native_fn_obj_t *nfobj = (native_fn_obj_t *)extract_ptr(nativefn_obj);

  nativefn nf = nfobj->nf;

  assert(nf);

#ifdef DEBUG  
  printf("returing from extract_native_fn\n");
#endif
  
  return nf;  
}

nativefn extract_native_fn1(OBJECT_PTR closure)
{
  assert(IS_CLOSURE_OBJECT(closure));

  printf("extract_native_fn is passed "); print_object(closure); printf("\n");
  print_object(extract_ptr(closure) + CONS_TAG); printf(" is the CONS form\n");  
  
  //TODO: is an unmet dependencies check
  //needed here?
         
  //OBJECT_PTR nativefn_obj = (OBJECT_PTR)*((OBJECT_PTR *)extract_ptr(closure));
  OBJECT_PTR nativefn_obj = car(closure);
  
  assert(IS_NATIVE_FN_OBJECT(nativefn_obj));

  uintptr_t p = extract_ptr(nativefn_obj);  
  printf("extract_ptr returns %lu\n", p);
  
  nativefn nf = *((nativefn *)p);
  
  printf("nf is %p\n", nf);
  
  assert(nf);

  //printf("returning from extract_native_fn\n");
  
  return nf;
}

OBJECT_PTR get_exception_handler()
{
  //TODO
  return NIL;
}

OBJECT_PTR get_continuation(OBJECT_PTR selector)
{
  OBJECT_PTR rest = method_call_stack;
  OBJECT_PTR cont = NIL;

  while(rest != NIL)
  {
    if(car(car(rest)) == selector)
    {
       cont = cdr(car(rest));
       method_call_stack = cdr(rest);
       break;
    }
    rest = cdr(rest);
  }

  assert(cont != NIL);
  assert(IS_CLOSURE_OBJECT(cont));

  return cont;
}

int in_error_condition()
{
  //TODO
  return 0;
}

//needn't return an OBJECT_PTR; kept so
//since we do it that way in pLisp
OBJECT_PTR save_cont_to_resume(OBJECT_PTR cont)
{
  //TODO
  return NIL;
}

//needn't return an OBJECT_PTR; kept so
//since we do it that way in pLisp
OBJECT_PTR handle_exception()
{
  //TODO
  return NIL;
}

