#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "gc.h"
#include "smalltalk.h"
#include "util.h"

//list of exception handlers;
//an exception handler is a list of
//a) exception selector, b) exception action,
//and c) closure object that is the continuation
//that the protected block (which is mapped to
//the exception handler) is to call when it
//exits normally
OBJECT_PTR exception_environment;

OBJECT_PTR curtailed_blocks_list;

OBJECT_PTR get_symbol(char *);
char *get_smalltalk_symbol_name(OBJECT_PTR);
nativefn extract_native_fn(OBJECT_PTR);
OBJECT_PTR convert_int_to_object(int);
OBJECT_PTR new_object(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR convert_class_object_to_object_ptr(class_object_t *);
OBJECT_PTR convert_native_fn_to_object(nativefn);
OBJECT_PTR get_binding_val(binding_env_t *, OBJECT_PTR);

extern binding_env_t *top_level;

extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

extern OBJECT_PTR Object;
OBJECT_PTR Exception;

//global object that captures the exception
//environment that was in play when the exception
//handler was created by the execution of an on:do:
//method
OBJECT_PTR handler_environment;

OBJECT_PTR create_closure(OBJECT_PTR, OBJECT_PTR, nativefn, ...);
OBJECT_PTR identity_function(OBJECT_PTR, ...);

/* code below this point is earlier code; will be
   incoporated if found relevant */

/* these protocols need to be implemented for the
   first cut implementation:

   <exceptionDescription>
     defaultAction
     description
     isResumable
     messageText
     tag

   <signaledException>
     isNested
     outer
     pass
     resignalAs:
     resume
     resume:
     retry
     retryUsing:
     return
     return:
   
   <exceptionSignaler>
     signal
     signal:

   <exceptionBuilder>
     messageText:

   <Exception>
     <no messages, is an abstract class (applications will define subclasses of Exception)

   <Error>
     defaultAction
     isResumable

   <MessageNotUnderstood>
     message
     isResumable
     receiver

   <ZeroDivide>
     dividend
     isResumable

   <failedMessage>
     arguments
     selector

*/

/** commenting out earlier code **/

/*

OBJECT_PTR zero_divide_dividend(OBJECT_PTR receiver)
{
  return get_inst_var(receiver, get_symbol("dividend"));  
}

OBJECT_PTR zero_divide_is_resumable(OBJECT_PTR receiver)
{
  return TRUE;  
}

OBJECT_PTR Zero_Divide_dividend(OBJECT_PTR receiver, OBJECT_PTR argument)
{
  object_t *zd_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  zd_obj->nof_instance_vars = 1;
  zd_obj->instance_vars = (binding_t *)GC_MALLOC(sizeof(binding_t));
  zd_obj->instance_vars[0].key = get_symbol("dividend");
  zd_obj->instance_vars[0].val = argument;

  return (OBJECT_PTR)(zd_obj + OBJECT_TAG);
}

OBJECT_PTR failed_msg_arguments(OBJECT_PTR receiver)
{
  return get_inst_var(receiver, get_symbol("arguments"));
}

OBJECT_PTR failed_msg_selector(OBJECT_PTR receiver)
{
  return get_inst_var(receiver, get_symbol("selector"));
}

OBJECT_PTR create_FailedMessage()
{
  class_object_t *cls_obj = (class_object_t *)GC_MALLOC(sizeof(class_object_t));
  cls_obj->name = GC_strdup("FailedMessage");
  cls_obj->nof_shared_vars = 0;
  cls_obj->shared_vars = NULL;
  cls_obj->nof_instance_methods = 2;  
  
  cls_obj->instance_methods = (binding_t *)GC_MALLOC(cls_obj->nof_instance_methods *
                                                     sizeof(binding_t));

  cls_obj->instance_methods[0].key = get_symbol("arguments");
  cls_obj->instance_methods[1].key = get_symbol("selector");

  cls_obj->instance_methods[0].val = convert_native_fn_to_object(failed_msg_arguments);
  cls_obj->instance_methods[1].val = convert_native_fn_to_object(failed_msg_selector);

  return cls_obj;
}

/-* <exceptionDescription protocol *-/
//defaultAction is implemented in the MessageNotUnderstood class

OBJECT_PTR mnu_description(OBJECT_PTR receiver)
{
  char desc[200];
  memset(desc, '\0', 200);

  char *class_name = (((object_t *)extract_ptr(receiver))->class_object)->name;
  
  sprintf(desc, "MessageNotUnderstood: Objects of class %s do not understand the message %s",
          class_name, get_symbol_name(failed_msg_selector(mnu_message(receiver))));

  return create_string_object(desc);
  
}

//isResumable is implemented in the MessageNotunderstood class

OBJECT_PTR mnu_message_text(OBJECT_PTR receiver)
{
  //no need to incorporate any message provided by the exception signaler,
  //as this is an exception that is signalled internally
  return mnu_description(receiver);
}

OBJECT_PTR mnu_tag(OBJECT_PTR receiver)
{
  //no need to incorporate any tag provided by the exception signaler,
  //as this is an exception that is signalled internally
  return mnu_description(receiver);
}
/-* end of <exceptionDescription protocol *-/

/-* <signaledException> protocol *-/

/-* end of <signaledException> protocol *-/

/-* <exceptionBuilder> protocol *-/
OBJECT_PTR mnu_message_text(OBJECT_PTR receiver, OBJECT_PTR signaler_text)
{
  set_inst_var(receiver, get_symbol("messageText"), signaler_text);
  return receiver;
}
/-* end of <exceptionBuilder protocol *-/

/-* <exceptionSignaler> protocol *-/
OBJECT_PTR mnu_signal(OBJECT_PTR receiver)
{
  
}
/-* end of <exceptionSignaler> protocol *-/

/-* <Error> protocol *-/
OBJECT_PTR mnu_default_action(OBJECT_PTR receiver)
{
  printf("Error: MessageNotUnderstood: %s\n", get_symbol_name(failed_msg_selector(mnu_message(receiver))));
  return NIL;
}

// isResumable is defined in the MessageNotunderstood subclass
/-* end of <Error> protocol *-/

/-* <MessageNotUnderstood> protocol *-/
OBJECT_PTR mnu_message(OBJECT_PTR receiver)
{
  //TODO: ensure that the receiver, i.e., instance of MessageNotUnderstood,
  //has an instance variable called "failedMessage" when the instance  is constructed
  return get_inst_var(receiver, get_symbol("failedMessage"));
}

OBJECT_PTR mnu_is_resumable(OBJECT_PTR receiver)
{
  return TRUE;
}

OBJECT_PTR mnu_receiver(OBJECT_PTR receiver)
{
  //TODO: ensure that the receiver, i.e., instance of MessageNotUnderstood,
  //has an instance variable called "receiver" when the instance  is constructed  
  return get_inst_var(receiver, get_symbol("receiver"));
}

OBJECT_PTR create_mnu_object()
{
  object_t *mnu_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  mnu_obj->nof_instance_vars = 1;
  mnu_obj->instance_vars = (binding_t *)GC_MALLOC(sizeof(binding_t));
  mnu_obj->instance_vars[0].key = get_symbol("messageText");
  zd_obj->instance_vars[0].val = NIL;

  return (OBJECT_PTR)(mnu_obj + OBJECT_TAG);  
}

OBJECT_PTR create_MessageNotUnderstood()
{
  class_object_t *cls_obj = (class_object_t *)GC_MALLOC(sizeof(class_object_t));
  cls_obj->name = GC_strdup("MessageNotUnderstood");
  cls_obj->nof_shared_vars = 0;
  cls_obj->shared_vars = NULL;
  cls_obj->nof_instance_methods = 3;  
  
  cls_obj->instance_methods = (binding_t *)GC_MALLOC(cls_obj->nof_instance_methods *
                                                     sizeof(binding_t));

  cls_obj->instance_methods[0].key = get_symbol("message");
  cls_obj->instance_methods[1].key = get_symbol("isResumable");
  cls_obj->instance_methods[2].key = get_symbol("receiver");

  cls_obj->instance_methods[0].val = convert_native_fn_to_object(mnu_message);
  cls_obj->instance_methods[1].val = convert_native_fn_to_object(mnu_is_resumable);
  cls_obj->instance_methods[2].val = convert_native_fn_to_object(mnu_receiver);

  return cls_obj;
}
/-* end of <MessageNotunderstood> protocol *-/

*/

/** end of commented out earlier code **/

/* return type is void because we
   discard the return value of the curtailed
   blocks' execution */
void invoke_curtailed_blocks()
{
  OBJECT_PTR rest = curtailed_blocks_list;
  OBJECT_PTR blk;
  
  while(rest != NIL)
  {
    blk = car(rest);

    assert(IS_CLOSURE_OBJECT(blk));

    //TODO: a global idclo object
    OBJECT_PTR idclo = create_closure(convert_int_to_object(1),
				      convert_int_to_object(0),
				      (nativefn)identity_function);
  
    nativefn nf = (nativefn)extract_native_fn(blk);
    OBJECT_PTR discarded_ret = nf(blk, idclo);
    
    rest = cdr(rest);
  }

  curtailed_blocks_list = NIL;
}

OBJECT_PTR handle_exception(OBJECT_PTR exception)
{
  //TODO: exception is just a selector for the time
  //being, this will be converted to a full exception
  //object later

  OBJECT_PTR env = exception_environment;

  while(env != NIL)
  {
    OBJECT_PTR handler = car(env);

    OBJECT_PTR ret;
    
#ifdef DEBUG
    print_object(handler); printf(" is the handler\n");
    print_object(car(handler)); printf(" is the exception name\n");
#endif
    
    if(car(handler) == exception)
    {   
      OBJECT_PTR action = second(handler);
      assert(IS_CLOSURE_OBJECT(action)); //MonadicBlock

      handler_environment = third(handler);
      
      nativefn nf = (nativefn)extract_native_fn(action);

      //pop the handler (and all later handlers (is ths correct?))
      exception_environment = cdr(env);

      ret =  nf(action, exception, fourth(handler));

      invoke_curtailed_blocks();

      return ret;
    }

    env = cdr(env);
  }  

  
  printf("Unhandled exception: %s\n", get_smalltalk_symbol_name(exception));

  invoke_curtailed_blocks();
  
  return NIL;
}

OBJECT_PTR exception_is_nested(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));
  
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR env = handler_environment;

  nativefn nf = (nativefn)extract_native_fn(cont);
  
  while(env != NIL)
  {
    OBJECT_PTR handler = car(env);
    
    if(car(handler) == receiver)
      return nf(cont, TRUE);
    else
      return nf(cont, FALSE);
  }

  return nf(cont, FALSE);
}

void create_Exception()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Exception: Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object; //TODO: check whether this is OK
  cls_obj->name = GC_strdup("Exception");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  
  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 0;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  /* cls_obj->instance_methods->bindings[0].key = get_symbol("nested"); */
  /* cls_obj->instance_methods->bindings[0].val = list(3, */
  /* 						    convert_native_fn_to_object((nativefn)exception_is_nested), */
  /* 						    NIL, */
  /* 						    convert_int_to_object(0)); */

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 1;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));;

  cls_obj->class_methods->bindings[0].key = get_symbol("new");
  cls_obj->class_methods->bindings[0].val = list(3,
						 convert_native_fn_to_object((nativefn)new_object),
						 NIL,
						 convert_int_to_object(0));
  
  Exception =  convert_class_object_to_object_ptr(cls_obj);
}
