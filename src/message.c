#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "gc.h"

#include "smalltalk.h"
#include "util.h"

typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR, va_list*);

OBJECT_PTR convert_int_to_object(int);
int get_int_value(OBJECT_PTR);
OBJECT_PTR get_symbol(char *);
char *get_symbol_name(OBJECT_PTR);
nativefn extract_native_fn(OBJECT_PTR);
OBJECT_PTR create_closure(OBJECT_PTR, OBJECT_PTR, nativefn, ...);
OBJECT_PTR get_class_object(OBJECT_PTR);

BOOLEAN IS_CLASS_OBJECT(OBJECT_PTR);
BOOLEAN IS_NATIVE_FN_OBJECT(OBJECT_PTR);

BOOLEAN get_top_level_val(OBJECT_PTR, OBJECT_PTR *);
OBJECT_PTR get_binding_val_regular(binding_env_t *, OBJECT_PTR, OBJECT_PTR *);
void put_binding_val(binding_env_t *, OBJECT_PTR, OBJECT_PTR);

extern OBJECT_PTR NIL;
extern OBJECT_PTR Integer;
extern OBJECT_PTR SELF;

extern OBJECT_PTR Object;

extern binding_env_t *top_level;

BOOLEAN IS_OBJECT_OBJECT(OBJECT_PTR);

OBJECT_PTR method_lookup(OBJECT_PTR obj, OBJECT_PTR selector)
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

  OBJECT_PTR class = cls_obj;
  
  while(!method_found && class != Object)
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
  
  assert(method_found);

#ifdef DEBUG
  printf("returning from method_lookup\n");
#endif
  
  return method;
}

OBJECT_PTR message_send(OBJECT_PTR mesg_send_closure,
			OBJECT_PTR receiver,
			OBJECT_PTR selector,
			OBJECT_PTR count1,
			...)
{
  //TODO: should we save the previous value of SELF
  //and restore it before returning from message_send?
  put_binding_val(top_level, SELF, cons(receiver, NIL));
  
  int count;
  
  va_list ap;
  va_start(ap, count1); 

#ifdef DEBUG  
  print_object(receiver);printf(" is the receiver\n");
  print_object(selector); printf(" is the selector\n");
#endif
  
  assert(IS_SYMBOL_OBJECT(selector));

  count = get_int_value(count1);

#ifdef DEBUG  
  printf("count = %d\n", count);
#endif
  
  //if the selector has only a colon at the end strip it off
  //OBJECT_PTR stripped_selector = get_symbol(strip_last_colon(get_symbol_name(selector)));
  OBJECT_PTR stripped_selector = selector;

  OBJECT_PTR method = method_lookup(receiver, stripped_selector);

#ifdef DEBUG
  print_object(method); printf(" is returned by method_lookup()\n");
#endif  

  //TODO: arity slot gets incorrectly populated with some
  //compiler-generated symbol during closure_conv_transform().
  //needs to be fixed. Not very critical as arity violations
  //will be caught during parsing itself. We need the arity
  //more importantly to distinguish between NiladicBlock,
  //Monadicblock, etc.
  //if(count1 != car(last_cell(cdr(method))))
  //{
  //  assert(false);
  //}
  
  native_fn_obj_t *nfobj = (native_fn_obj_t *)extract_ptr(car(method));
  nativefn nf = nfobj->nf;
  assert(nf);

  OBJECT_PTR closed_vars = cdr(method);
  OBJECT_PTR ret = NIL;
  
  //OBJECT_PTR rest = closed_vars;
  //to discard the arity value which is at the end
  //TODO: replace with a more efficient way
  //OBJECT_PTR rest = reverse(cdr(reverse(closed_vars)));
  //OBJECT_PTR rest = closed_vars;
  OBJECT_PTR rest = cdr(closed_vars);
  
  binding_env_t *env = NULL;
  
  if(IS_CLASS_OBJECT(receiver))
    env = ((class_object_t *)extract_ptr(receiver))->shared_vars;
  else if(IS_OBJECT_OBJECT(receiver))
    env = ((object_t *)extract_ptr(receiver))->instance_vars;
  
  while(rest != NIL)
  {
    OBJECT_PTR closed_val_cons;

    //TODO: the receiver's super's instance/class vars
    //should also be scanned for the free vars' values
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
    rest = cdr(rest);
  }

  OBJECT_PTR cons_form = cons(car(method), reverse(ret));
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
    
    asm("mov %0, %%rdi\n\t" : : "r"(closure_form) : "%rdi");
    asm("mov %0, %%rsi\n\t" : : "r"(cont) : "%rsi");
    
    asm("call *%0\n\t" : : "m"(nf) : );
  }

  if(count == 1)
  {
    arg1 = (uintptr_t)va_arg(ap, uintptr_t);
    cont = (uintptr_t)va_arg(ap, uintptr_t);

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

    asm("mov %0, %%rdi\n\t" : : "r"(closure_form) : "%rdi");
    asm("mov %0, %%rsi\n\t" : : "r"(arg1) : "%rsi");
    //asm("mov %0, %%rdx\n\t" : : "r"(arg2) : "%rdx");
    asm("mov %0, %%rcx\n\t" : : "r"(arg3) : "%rcx");
    asm("mov %0, %%r8\n\t"  : : "r"(arg4) : "%r8");
    asm("mov %0, %%r9\n\t"  : : "r"(arg5) : "%r9");

    for(i=n-1; i>=0; i--)
      asm("push %0\n\t"       : : "r"(stack_args[i]) : );
    //asm("push %0\n\t"       : : "r"(stack_args[1]) : );
    //asm("push %0\n\t"       : : "r"(stack_args[0]) : );

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

  return retval;
}

OBJECT_PTR create_message_send_closure()
{
  //first parameter is a dummy value, not needed
  //(we do not know the arity of message_send)
  return create_closure(convert_int_to_object(0),
			convert_int_to_object(0), (nativefn)message_send);
}

