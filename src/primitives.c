#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "global_decls.h"
#include "stack.h"

void print_call_chain();

extern OBJECT_PTR NIL;

extern stack_type *g_call_chain;
extern OBJECT_PTR g_idclo;
extern OBJECT_PTR g_msg_snd_closure;

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
         
  OBJECT_PTR nativefn_obj = car(closure);
  
  assert(IS_NATIVE_FN_OBJECT(nativefn_obj));

  uintptr_t p = extract_ptr(nativefn_obj);  
  printf("extract_ptr returns %lu\n", p);
  
  nativefn nf = *((nativefn *)p);
  
  printf("nf is %p\n", nf);
  
  assert(nf);

  return nf;
}

OBJECT_PTR get_exception_handler()
{
  //TODO
  return NIL;
}

OBJECT_PTR get_continuation(OBJECT_PTR selector)
{
  assert(IS_SYMBOL_OBJECT(selector));
  assert(!stack_is_empty(g_call_chain));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  while(entry->selector != selector)
  {
    OBJECT_PTR termination_blk = entry->termination_blk_closure;

    if(termination_blk != NIL &&
       entry->termination_blk_invoked == false)
    {
      nativefn nf = (nativefn)extract_native_fn(termination_blk);
      OBJECT_PTR discarded_ret = message_send(g_msg_snd_closure,
					      termination_blk,
					      get_symbol("value_"),
					      convert_int_to_object(0),
					      g_idclo);
      //this is not really neded
      //as we are popping the entry
      entry->termination_blk_invoked = true;
    }

    //TODO: this assert shouldn't get triggered because
    //the call chain cannot be empty when we are returning
    //from a method. need to convert this into an exception
    //to handle returns from anonymous blocks
    assert(!stack_is_empty(g_call_chain));
    
    entry = (call_chain_entry_t *)stack_pop(g_call_chain);
  }

  return entry->cont;
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

