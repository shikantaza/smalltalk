#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "gc.h"

#include "global_decls.h"
#include "util.h"
#include "stack.h"

extern OBJECT_PTR g_idclo;
extern OBJECT_PTR THIS_CONTEXT;
extern OBJECT_PTR g_method_call_stack;
extern stack_type *g_call_chain;
extern OBJECT_PTR NIL;
extern OBJECT_PTR Integer;
extern OBJECT_PTR SELF;
extern OBJECT_PTR SUPER;
extern OBJECT_PTR Object;
extern binding_env_t *g_top_level;
extern OBJECT_PTR Nil;

extern OBJECT_PTR MNU_SYMBOL;

call_chain_entry_t *create_call_chain_entry(OBJECT_PTR receiver,
					    OBJECT_PTR selector,
					    OBJECT_PTR method,
					    OBJECT_PTR closure,
					    unsigned int nof_args,
					    OBJECT_PTR *args,
					    OBJECT_PTR cont,
					    OBJECT_PTR termination_blk_closure,
					    BOOLEAN termination_blk_invoked)
{
  call_chain_entry_t *entry = (call_chain_entry_t *)GC_MALLOC(sizeof(call_chain_entry_t));

  entry->receiver                = receiver;
  entry->selector                = selector;
  entry->method                  = method;
  entry->closure                 = closure;
  entry->nof_args                = nof_args;

  entry->args = (OBJECT_PTR *)GC_MALLOC(nof_args * sizeof(OBJECT_PTR));

  unsigned int i;
  for(i=0; i< nof_args; i++)
    entry->args[i] = args[i];

  entry->local_vars_list         = NIL;
  entry->cont                    = cont;
  entry->termination_blk_closure = termination_blk_closure;
  entry->termination_blk_invoked = termination_blk_invoked;

  return entry;
}

OBJECT_PTR method_lookup(BOOLEAN super, OBJECT_PTR obj, OBJECT_PTR selector)
{
  OBJECT_PTR cls_obj;
  BOOLEAN is_class_object;

#ifdef DEBUG
  print_object(obj); printf(" is the receiver passed to method_lookup()\n");
  print_object(selector); printf(" is the selector passed to method_lookup()\n");
#endif
  
  if(!IS_CLASS_OBJECT(obj))
  {
    is_class_object = false;
    cls_obj = get_class_object(obj);
  }
  else
  {
    is_class_object = true;
    cls_obj = obj;
  }

#ifdef DEBUG
  printf("method_lookup(): is_class_object = %s\n", is_class_object ? "true" : "false");
#endif  

  BOOLEAN method_found = false;
  OBJECT_PTR method;

  unsigned int i, n;

  //OBJECT_PTR class = cls_obj;
  OBJECT_PTR class;

  if(super)
    class = get_parent_class(cls_obj);
  else
    class = cls_obj;
  
  while(!method_found && class != NIL)
  {
    class_object_t *cls_obj_int = (class_object_t *)extract_ptr(class);

    if(is_class_object)
    {
      n = cls_obj_int->class_methods->count;
  
      for(i=0; i<n; i++)
      {
	if(cls_obj_int->class_methods->bindings[i].key == selector)
	{
	  method_found = true;
	  method = cls_obj_int->class_methods->bindings[i].val;
	  break;
	}
      }    
    }
    else
    {
      n = cls_obj_int->instance_methods->count;
  
      for(i=0; i<n; i++)
      {
	if(cls_obj_int->instance_methods->bindings[i].key == selector)
	{
	  method_found = true;
	  method = cls_obj_int->instance_methods->bindings[i].val;
	  break;
	}
      }
    }
    
    class = cls_obj_int->parent_class_object;
  }
  
  //assert(method_found);

#ifdef DEBUG
  printf("returning from method_lookup\n");
#endif

  if(method_found)
    return method;
  else
    return NIL;
}

OBJECT_PTR message_send_internal(BOOLEAN super,
				 OBJECT_PTR receiver,
				 OBJECT_PTR selector,
				 OBJECT_PTR count1,
				 va_list ap)
{
  //TODO: should we save the previous value of SELF
  //and restore it before returning from message_send?
  put_binding_val(g_top_level, SELF, cons(receiver, NIL));
  put_binding_val(g_top_level, SUPER, cons(receiver, NIL));

  int count;
  
  //va_list ap;
  //va_start(ap, count1); 

#ifdef DEBUG  
  print_object(receiver);printf(" is the receiver; ");
  print_object(selector); printf(" is the selector\n");
#endif

  //TODO: should this be converted to an exception?
  assert(IS_SYMBOL_OBJECT(selector));

  count = get_int_value(count1);

#ifdef DEBUG  
  printf("count = %d\n", count);
#endif
  
  //if the selector has only a colon at the end strip it off
  //OBJECT_PTR stripped_selector = get_symbol(strip_last_colon(get_symbol_name(selector)));
  OBJECT_PTR stripped_selector = selector;

  OBJECT_PTR method = method_lookup(super, receiver, stripped_selector);

  if(method == NIL)
  {
    OBJECT_PTR ret;
#ifdef DEBUG    
    print_object(stripped_selector); printf(" is not understood by "); print_object(receiver); printf("\n");
    getchar();
#endif
    assert(get_top_level_val(MNU_SYMBOL, &ret));

    OBJECT_PTR mnu_class_obj = car(ret);

    //TODO: the exception object's messageText has to be set (after adding it as an instance variable)
    OBJECT_PTR exception_obj = new_object_internal(mnu_class_obj,
						   convert_fn_to_closure((nativefn)new_object_internal),
						   g_idclo);
    char *str = get_symbol_name(stripped_selector);

    return signal_exception_with_text(exception_obj, get_string_obj(substring(str, 1, strlen(str)-1)));
  }

#ifdef DEBUG
  print_object(method); printf(" is returned by method_lookup()\n");
#endif  

  //if(count1 != car(last_cell(cdr(method))))
  if(count1 != third(method))
  {
    assert(false);
  }
  
  native_fn_obj_t *nfobj = (native_fn_obj_t *)extract_ptr(car(method));
  nativefn nf = nfobj->nf;
  assert(nf);

  //OBJECT_PTR closed_vars = cdr(method);
  OBJECT_PTR closed_vars = second(method);
  OBJECT_PTR ret = NIL;
  
  //OBJECT_PTR rest = closed_vars;
  //to discard the arity value which is at the end
  //TODO: replace with a more efficient way
  //OBJECT_PTR rest = reverse(cdr(reverse(closed_vars)));
  OBJECT_PTR rest = closed_vars;
  //OBJECT_PTR rest = cdr(closed_vars);
  
  //binding_env_t *env = NULL;
  binding_env_t *inst_vars = NULL;
  binding_env_t *shared_vars = NULL;
  
  if(IS_CLASS_OBJECT(receiver))
  {
    shared_vars = ((class_object_t *)extract_ptr(receiver))->shared_vars;
  }
  else if(IS_OBJECT_OBJECT(receiver))
  {
    shared_vars = ((class_object_t *)extract_ptr(get_class_object(receiver)))->shared_vars;
    inst_vars = ((object_t *)extract_ptr(receiver))->instance_vars;
  }
  
  while(rest != NIL)
  {
    OBJECT_PTR closed_val_cons;

    if(IS_OBJECT_OBJECT(receiver) && inst_vars)
    {
      if(get_binding_val_regular(inst_vars, car(rest), &closed_val_cons))
      {
	ret = cons(closed_val_cons, ret);
	rest = cdr(rest);
	continue;
      }
    }

    if(IS_OBJECT_OBJECT(receiver) && shared_vars)
    {
      if(get_binding_val_regular(shared_vars, car(rest), &closed_val_cons))
      {
	ret = cons(closed_val_cons, ret);
	rest = cdr(rest);
	continue;
      }
    }

    if(IS_CLASS_OBJECT(receiver) && shared_vars)
    {
      if(get_binding_val_regular(shared_vars, car(rest), &closed_val_cons))
      {
	ret = cons(closed_val_cons, ret);
	rest = cdr(rest);
	continue;
      }
    }

    //TODO: assert should be replaced to raise an exception
    assert(get_top_level_val(car(rest), &closed_val_cons));
    ret = cons(closed_val_cons, ret);
    
    /*
    if(env) //to filter out objects like integers which are not regular objects
    {
      if(get_binding_val_regular(env, car(rest), &closed_val_cons))
      {
	ret = cons(closed_val_cons, ret);
	rest = cdr(rest);
	continue;
      }
      else
      {
	//TODO: assert should be replaced to raise an exception
	assert(get_top_level_val(car(rest), &closed_val_cons));
	ret = cons(closed_val_cons, ret);
      }
    }
    */
    
    rest = cdr(rest);
  }

  OBJECT_PTR cons_form = list(3, car(method), reverse(ret), count1);
  OBJECT_PTR closure_form = extract_ptr(cons_form) + CLOSURE_TAG;

#ifdef DEBUG  
  print_object(cons_form); printf(" is the CONS form of the closure invoked by message_send\n");
  print_object(closure_form); printf(" is the closure form of the closure invoked by message_send\n");
#endif
  
  uintptr_t arg1, arg2, arg3,arg4, arg5;
  uintptr_t cont;

  //order of registers - first to sixth argument
  //%rdi, %rsi, %rdx, %rcx, %r8, %r9

  if(count == 0)
  {
    cont = (uintptr_t)va_arg(ap, uintptr_t);

    g_method_call_stack = cons(cons(selector,cont), g_method_call_stack);

    stack_push(g_call_chain, create_call_chain_entry(receiver, selector, method, closure_form, 0, NULL, cont, NIL, false));

    put_binding_val(g_top_level, THIS_CONTEXT, cons(cont, NIL));

    asm("mov %0, %%rdi\n\t" : : "r"(closure_form) : "%rdi");
    asm("mov %0, %%rsi\n\t" : : "r"(cont) : "%rsi");
    
    asm("call *%0\n\t" : : "m"(nf) : );
  }

  if(count == 1)
  {
    arg1 = (uintptr_t)va_arg(ap, uintptr_t);
    cont = (uintptr_t)va_arg(ap, uintptr_t);

    OBJECT_PTR *args = (OBJECT_PTR *)GC_MALLOC(sizeof(OBJECT_PTR));
    *args = arg1;

    g_method_call_stack = cons(cons(selector,cont), g_method_call_stack);

    stack_push(g_call_chain, create_call_chain_entry(receiver, selector, method, closure_form, 1, args, cont, NIL, false));

    put_binding_val(g_top_level, THIS_CONTEXT, cons(cont, NIL));

    asm("mov %0, %%rdi\n\t" : : "r"(closure_form) : "%rdi");
    asm("mov %0, %%rsi\n\t" : : "r"(arg1) : "%rsi");
    asm("mov %0, %%rdx\n\t" : : "r"(cont) : "%rdx");
    
    asm("call *%0\n\t" : : "m"(nf) : );
  }

  if(count == 2)
  {
    arg1 = (uintptr_t)va_arg(ap, uintptr_t);
    arg2 = (uintptr_t)va_arg(ap, uintptr_t);
    cont = (uintptr_t)va_arg(ap, uintptr_t);
    
    g_method_call_stack = cons(cons(selector,cont), g_method_call_stack);

    OBJECT_PTR *args = (OBJECT_PTR *)GC_MALLOC(2 * sizeof(OBJECT_PTR));
    args[0] = arg1; args[1] = arg2;

    stack_push(g_call_chain, create_call_chain_entry(receiver, selector, method, closure_form, 2, args, cont, NIL, false));

    put_binding_val(g_top_level, THIS_CONTEXT, cons(cont, NIL));

    asm("mov %0, %%rdi\n\t" : : "r"(closure_form) : "%rdi");
    asm("mov %0, %%rsi\n\t" : : "r"(arg1) : "%rsi");
    asm("mov %0, %%rdx\n\t" : : "r"(arg2) : "%rdx");
    asm("mov %0, %%rcx\n\t" : : "r"(cont) : "%rcx");
    
    asm("call *%0\n\t" : : "m"(nf) : );
  }

  if(count == 3)
  {
    arg1 = (uintptr_t)va_arg(ap, uintptr_t);
    arg2 = (uintptr_t)va_arg(ap, uintptr_t);
    arg3 = (uintptr_t)va_arg(ap, uintptr_t);
    cont = (uintptr_t)va_arg(ap, uintptr_t);
    
    g_method_call_stack = cons(cons(selector,cont), g_method_call_stack);

    OBJECT_PTR *args = (OBJECT_PTR *)GC_MALLOC(3 * sizeof(OBJECT_PTR));
    args[0] = arg1; args[1] = arg2; args[2] = arg3;

    stack_push(g_call_chain, create_call_chain_entry(receiver, selector, method, closure_form, 3, args, cont, NIL, false));

    put_binding_val(g_top_level, THIS_CONTEXT, cons(cont, NIL));

    asm("mov %0, %%rdi\n\t" : : "r"(closure_form) : "%rdi");
    asm("mov %0, %%rsi\n\t" : : "r"(arg1) : "%rsi");
    asm("mov %0, %%rdx\n\t" : : "r"(arg2) : "%rdx");
    asm("mov %0, %%rcx\n\t" : : "r"(arg3) : "%rcx");
    asm("mov %0, %%r8\n\t"  : : "r"(cont) : "%r8");
   
    asm("call *%0\n\t" : : "m"(nf) : );
  }

  if(count == 4)
  {
    arg1 = (uintptr_t)va_arg(ap, uintptr_t);
    arg2 = (uintptr_t)va_arg(ap, uintptr_t);
    arg3 = (uintptr_t)va_arg(ap, uintptr_t);
    arg4 = (uintptr_t)va_arg(ap, uintptr_t);
    cont = (uintptr_t)va_arg(ap, uintptr_t);

    g_method_call_stack = cons(cons(selector,cont), g_method_call_stack);

    OBJECT_PTR *args = (OBJECT_PTR *)GC_MALLOC(4 * sizeof(OBJECT_PTR));
    args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4;

    stack_push(g_call_chain, create_call_chain_entry(receiver, selector, method, closure_form, 4, args, cont, NIL, false));

    put_binding_val(g_top_level, THIS_CONTEXT, cons(cont, NIL));

    asm("mov %0, %%rdi\n\t" : : "r"(closure_form) : "%rdi");
    asm("mov %0, %%rsi\n\t" : : "r"(arg1) : "%rsi");
    asm("mov %0, %%rdx\n\t" : : "r"(arg2) : "%rdx");
    asm("mov %0, %%rcx\n\t" : : "r"(arg3) : "%rcx");
    asm("mov %0, %%r8\n\t"  : : "r"(arg4) : "%r8");
    asm("mov %0, %%r9\n\t"  : : "r"(cont) : "%r9");

    asm("call *%0\n\t" : : "m"(nf) : );
  }

  if(count > 4)
  {
    int i, n;
    
    arg1 = (uintptr_t)va_arg(ap, uintptr_t);
    arg2 = (uintptr_t)va_arg(ap, uintptr_t);
    arg3 = (uintptr_t)va_arg(ap, uintptr_t);
    arg4 = (uintptr_t)va_arg(ap, uintptr_t);
    arg5 = (uintptr_t)va_arg(ap, uintptr_t);

#ifdef DEBUG    
    printf("arg[1-5] = %lu %lu %lu %lu %lu\n", arg1, arg2, arg3, arg4, arg5);
#endif
    
    n = count - 4; // no of arguments that should be pushed onto the stack

    uintptr_t *stack_args = (uintptr_t *)GC_MALLOC(n * sizeof(uintptr_t));
    
    for(i=0; i<n; i++)
    {
      stack_args[i] = (uintptr_t)va_arg(ap, uintptr_t);
#ifdef DEBUG      
      printf("stack_args[%d] = %lu\n", i, stack_args[i]);
#endif      
    }

    g_method_call_stack = cons(cons(selector,stack_args[n-1]), g_method_call_stack);

    OBJECT_PTR *args = (OBJECT_PTR *)GC_MALLOC(count * sizeof(OBJECT_PTR));

    args[0] = arg1;
    args[1] = arg2;
    args[2] = arg3;
    args[3] = arg4;
    args[4] = arg5;

    for(i=0; i<n-1; i++)
      args[i+5] = stack_args[i];

    stack_push(g_call_chain, create_call_chain_entry(receiver, selector, method, closure_form, count, args, cont, NIL, false));

    put_binding_val(g_top_level, THIS_CONTEXT, cons(stack_args[n-1], NIL));

    asm("mov %0, %%rdi\n\t" : : "r"(closure_form) : "%rdi");
    asm("mov %0, %%rsi\n\t" : : "r"(arg1) : "%rsi");
    //asm("mov %0, %%rdx\n\t" : : "r"(arg2) : "%rdx");
    asm("mov %0, %%rcx\n\t" : : "r"(arg3) : "%rcx");
    asm("mov %0, %%r8\n\t"  : : "r"(arg4) : "%r8");
    asm("mov %0, %%r9\n\t"  : : "r"(arg5) : "%r9");

    for(i=n-1; i>=0; i--)
      asm("push %0\n\t"       : : "r"(stack_args[i]) : );

    //using a for loop screws up the rdx register.
    //so we populate rdx after the stack push operations
    asm("mov %0, %%rdx\n\t" : : "r"(arg2) : "%rdx");

    asm("call *%0\n\t" : : "m"(nf) : );

    for(i=0; i<n; i++)
      asm("addq $8, %%rsp\n\t" : : : );
  }

  OBJECT_PTR retval = NIL;

  asm("mov %%rax, %0\n\t" : : "r"(retval) : "%rax");

  va_end(ap);

#ifdef DEBUG  
  printf("message_send returning "); print_object(retval); printf("\n");
#endif
  
  return retval;
}

OBJECT_PTR message_send(OBJECT_PTR msg_send_closure,
			OBJECT_PTR receiver,
			OBJECT_PTR selector,
			OBJECT_PTR count1,
			...)
{
  va_list ap;
  va_start(ap, count1);

  OBJECT_PTR ret = message_send_internal(false, receiver, selector, count1, ap);

  va_end(ap);

  return ret;
}

OBJECT_PTR message_send_super(OBJECT_PTR msg_send_closure,
			      OBJECT_PTR receiver,
			      OBJECT_PTR selector,
			      OBJECT_PTR count1,
			      ...)
{
  va_list ap;
  va_start(ap, count1);

  OBJECT_PTR ret = message_send_internal(true, receiver, selector, count1, ap);

  va_end(ap);

  return ret;
}

