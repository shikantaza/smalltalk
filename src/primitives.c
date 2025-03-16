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

extern OBJECT_PTR VALUE_SELECTOR;

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

  //assert(!stack_is_empty(g_call_chain));
  //TODO: this is a hack to a) handle re-execution of ensure blocks
  //b) neutralize the system-added '^ self' messages at the
  //end of method bodies (for the case when the method body
  //has already returned). needs fixing if there are
  //repurcussions elsewhere
  if(stack_is_empty(g_call_chain))
    return g_idclo;

  //1. Scan the call chain and identify the continuation object
  //   corresponding to the passed selector.
  //2. Retrieve all the to-be-executed termination blocks (upto the call chain
  //   entry corresponding to the selector) as a list.

  call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
  int count = stack_count(g_call_chain);

  int i = count - 1;

  OBJECT_PTR termination_blk_lst = NIL;

  while(entries[i]->selector != selector && i >= 1)
  {
    call_chain_entry_t *entry = entries[i];

    OBJECT_PTR termination_blk = entry->termination_blk_closure;

    if(termination_blk != NIL &&
       entry->termination_blk_invoked == false)
    {
      termination_blk_lst = cons(termination_blk, termination_blk_lst);

      //we are setting this to true preemptively,
      //should not be an issue hopefully
      entry->termination_blk_invoked = true;
    }
    i--;
  }

  //we should have hit the method's selector before
  //running out of stack
  //assert(i != 0);

  OBJECT_PTR cont = entries[i]->cont;

  //since the termination blocks should be executed on LIFO basis
  termination_blk_lst = reverse(termination_blk_lst);

  //3. Loop through this list and execute the blocks (with explicit "value"
  //   messages). Note that the processing of the termination blocks could alter the
  //  stack downstream of the entry we are interested in

  int n = cons_length(termination_blk_lst);

  for(i=0; i<n; i++)
  {
    OBJECT_PTR discarded_ret = message_send(g_msg_snd_closure,
					    nth(convert_int_to_object(i),termination_blk_lst),
					    VALUE_SELECTOR,
					    convert_int_to_object(0),
					    g_idclo);
    
  }

  //4. Pop all the call chain stack items up to and including the one corresponding
  //   to the selector.

  if(!stack_is_empty(g_call_chain))
  {
    call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

    while(entry->selector != selector && !stack_is_empty(g_call_chain))
      entry = (call_chain_entry_t *)stack_pop(g_call_chain);
  }

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

