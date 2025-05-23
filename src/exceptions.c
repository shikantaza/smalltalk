#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "global_decls.h"
#include "util.h"
#include "stack.h"

//void show_debug_window(BOOLEAN, OBJECT_PTR, char *);
void show_error_dialog(char *);

//stack of exception handlers;
//an exception handler is a list of tuples,
//each tuple comprising:
// a) the protected_block to which the handler is attached
// b) the exception selector which the handler handles
// c) the action to be performed by the handler for the exception
// d) the exception environment existing at the time of execution of the on:do:
// e) the continuation to call on successful execution of the protected block
stack_type *g_exception_environment;

stack_type *g_signalling_environment;

OBJECT_PTR Exception;
OBJECT_PTR Error;
OBJECT_PTR MessageNotUnderstood;
OBJECT_PTR ZeroDivide;
OBJECT_PTR InvalidArgument;
OBJECT_PTR IndexOutofBounds;

//global object that captures the exception
//environment that was in play when the exception
//handler was created by the execution of an on:do:
//method
stack_type *g_handler_environment;

exception_handler_t *g_active_handler;

stack_type *g_exception_contexts;

extern binding_env_t *g_top_level;

extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;
extern OBJECT_PTR Object;
extern stack_type *g_call_chain;
extern OBJECT_PTR g_idclo;
extern OBJECT_PTR g_msg_snd_closure;
extern OBJECT_PTR Nil;

extern OBJECT_PTR VALUE_SELECTOR;
extern OBJECT_PTR VALUE1_SELECTOR;
extern OBJECT_PTR SIGNAL_SELECTOR;
extern OBJECT_PTR ON_DO_SELECTOR;

extern char **g_string_literals;

extern OBJECT_PTR Smalltalk;

extern OBJECT_PTR g_last_eval_result;

extern BOOLEAN g_debug_in_progress;

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

void invoke_curtailed_blocks(OBJECT_PTR cont)
{
  assert(!stack_is_empty(g_call_chain));

  call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
  int count = stack_count(g_call_chain);
  //printf("count = %d\n", count);
  int i = count - 1;

  while(i >= 0)
  {
    //printf("i = %d\n",i);
    call_chain_entry_t *entry = entries[i];

    if(entry->cont == cont)
      break;
    
    OBJECT_PTR termination_blk = entry->termination_blk_closure;

    if(termination_blk != NIL &&
       entry->termination_blk_invoked == false)
    {
      OBJECT_PTR discarded_ret = message_send(g_msg_snd_closure,
					      termination_blk,
					      VALUE_SELECTOR,
					      convert_int_to_object(0),
					      g_idclo);
      entry->termination_blk_invoked = true;
    }
    
    i--;
  }
 
}

OBJECT_PTR exception_user_intervention(OBJECT_PTR cont)
{
  g_debug_in_progress = true;
  show_debug_window(true, cont);

  while(g_debug_in_progress)
    ; //loop till the debug window returns control

  return g_last_eval_result;
}

OBJECT_PTR exception_user_intervention_cli(OBJECT_PTR cont)
{
  int choice = 0;

  while(choice < 1 || choice > 5)
  {
    printf("Enter 1 to abort, 2 to retry, 3 to resume with nil, 4 to resume with a value, 5 to ignore exception: ");
    scanf("%d", &choice);
  }

  if(choice == 1)
    return NIL;
  else if(choice == 2)
  {
    stack_pop(g_call_chain); //Exception>>signal

    call_chain_entry_t *entry = stack_pop(g_call_chain); //the method that signalled the exception

    int i;
    int n = entry->nof_args;

    OBJECT_PTR *args = (OBJECT_PTR *)GC_MALLOC((n+1) * sizeof(OBJECT_PTR));

    for(i=0; i<n; i++)
      args[i] = entry->args[i];

    args[n] = entry->cont;

    return message_send_internal(entry->super,
				 entry->receiver,
				 entry->selector,
				 convert_int_to_object(n),
				 args);
  }
  else if(choice == 3)
  {
    if(!stack_is_empty(g_call_chain) && g_active_handler != NULL)
      invoke_curtailed_blocks(g_active_handler->cont);

    OBJECT_PTR exception_context;

    if(!stack_is_empty(g_exception_contexts))
      exception_context = (OBJECT_PTR)stack_pop(g_exception_contexts);
    else
      exception_context = g_idclo;

    assert(IS_CLOSURE_OBJECT(exception_context));

    //TODO: check if the exception object is resumable,
    //if it is, return the default resumption value

    //two pops, the frame corresponding to Exception>>signal
    //and the frame corresponding to the method that signalled
    //the exception
    if(!stack_is_empty(g_call_chain))
      stack_pop(g_call_chain);

    if(!stack_is_empty(g_call_chain))
      stack_pop(g_call_chain);

    return invoke_cont_on_val(exception_context, NIL);
  }
  else if(choice == 4)
  {
    char buf[200];

    printf("Enter expression to evaluate and resume with: ");
    scanf("%s", buf);

    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  Smalltalk,
				  get_symbol("_eval:"),
				  convert_int_to_object(1),
				  get_string_obj(buf),
				  g_idclo);

    if(ret == NIL)
      printf("Error evaluating the given resumption value, resuming with nil\n");

    if(!stack_is_empty(g_call_chain))
      invoke_curtailed_blocks(g_active_handler->cont);

    OBJECT_PTR exception_context;

    if(!stack_is_empty(g_exception_contexts))
      exception_context = (OBJECT_PTR)stack_pop(g_exception_contexts);
    else
      exception_context = g_idclo;

    assert(IS_CLOSURE_OBJECT(exception_context));

    //two pops, the frame corresponding to Exception>>signal
    //and the frame corresponding to the method that signalled
    //the exception
    if(!stack_is_empty(g_call_chain))
      stack_pop(g_call_chain);

    if(!stack_is_empty(g_call_chain))
      stack_pop(g_call_chain);

    return invoke_cont_on_val(exception_context, ret);
  }
  else if(choice == 5)
    return invoke_cont_on_val(cont, NIL);
  else
    assert(false); //execution should not reach here
}

OBJECT_PTR signal_exception_with_text(OBJECT_PTR exception, OBJECT_PTR signalerText, OBJECT_PTR cont)
{
  assert(IS_STRING_LITERAL_OBJECT(signalerText) || signalerText == NIL);

  g_signalling_environment = g_exception_environment;

  exception_handler_t **entries = (exception_handler_t **)stack_data(g_exception_environment);
  int count = stack_count(g_exception_environment);
  int i = count - 1;

  OBJECT_PTR cls_obj = get_class_object(exception);
  class_object_t *cls_obj_int = (class_object_t *)extract_ptr((cls_obj));

  while(i >= 0)
  {
    exception_handler_t *handler = entries[i];

    OBJECT_PTR ret;

    if(cls_obj == handler->selector || is_super_class(handler->selector, cls_obj))
    {
      g_active_handler = handler;

      OBJECT_PTR action = handler->exception_action;
      
      assert(IS_CLOSURE_OBJECT(action)); //MonadicBlock

      g_handler_environment = handler->exception_environment;
      
      //pop the handler (and all later handlers (is ths correct?))
      //stack_pop(g_exception_environment); //don't think the env should be popped

      ret = message_send(g_msg_snd_closure,
			 action,
			 VALUE1_SELECTOR,
			 convert_int_to_object(1),
			 exception,
			 handler->cont);

      return ret;
    }

    i--;
  }  

  char buf[200];
  memset(buf, '\0', 200);

  if(signalerText != NIL)
    //printf("Unhandled exception: %s (%s)\n", cls_obj_int->name, g_string_literals[signalerText >> OBJECT_SHIFT]);
    sprintf(buf, "Unhandled exception: %s (%s)\n", cls_obj_int->name, g_string_literals[signalerText >> OBJECT_SHIFT]);
  else
    //printf("Unhandled exception: %s\n", cls_obj_int->name);
    sprintf(buf, "Unhandled exception: %s\n", cls_obj_int->name);

  show_error_dialog(buf);

  //print_call_chain();

  return exception_user_intervention(cont);
}

OBJECT_PTR signal_exception(OBJECT_PTR exception, OBJECT_PTR cont)
{
  return signal_exception_with_text(exception, NIL, cont);
}

OBJECT_PTR exception_signal(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  return signal_exception(receiver, cont);
}

OBJECT_PTR exception_signal_with_text(OBJECT_PTR closure, OBJECT_PTR signalerText, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));

  if(!IS_STRING_LITERAL_OBJECT(signalerText) || signalerText == NIL)
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  return signal_exception_with_text(receiver, signalerText, cont);
}

OBJECT_PTR exception_is_nested(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  stack_type *env = g_handler_environment;

  exception_handler_t **entries = (exception_handler_t **)stack_data(g_handler_environment);
  int count = stack_count(g_handler_environment);
  int i = count - 1;

  OBJECT_PTR cls_obj = get_class_object(receiver);
  class_object_t *cls_obj_int = (class_object_t *)extract_ptr((cls_obj));
  
  while(i >= 0)
  {
    exception_handler_t *handler = entries[i];

    char *exception_name = get_smalltalk_symbol_name(handler->selector);
    
    if(!strcmp(exception_name, cls_obj_int->name))
      return invoke_cont_on_val(cont, TRUE);
    else
      return invoke_cont_on_val(cont, FALSE);

    i--;
  }

  return invoke_cont_on_val(cont, FALSE);
}

OBJECT_PTR exception_return(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR handler_cont = g_active_handler->cont;

  assert(IS_CLOSURE_OBJECT(handler_cont));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  invoke_curtailed_blocks(handler_cont);

  assert(!stack_is_empty(g_exception_contexts));
  stack_pop(g_exception_contexts);

  pop_if_top(entry);

  return invoke_cont_on_val(handler_cont, NIL);
}

OBJECT_PTR exception_return_val(OBJECT_PTR closure, OBJECT_PTR val, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR handler_cont = g_active_handler->cont;

  assert(IS_CLOSURE_OBJECT(handler_cont));

  invoke_curtailed_blocks(handler_cont);

  assert(!stack_is_empty(g_exception_contexts));
  stack_pop(g_exception_contexts);

  return invoke_cont_on_val(handler_cont, val);
}

OBJECT_PTR exception_retry(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR handler_cont = g_active_handler->cont;
  assert(IS_CLOSURE_OBJECT(handler_cont));

  invoke_curtailed_blocks(handler_cont);

  OBJECT_PTR protected_block = g_active_handler->protected_block;

  return message_send(g_msg_snd_closure,
		      protected_block,
		      ON_DO_SELECTOR,
		      convert_int_to_object(2),
		      g_active_handler->selector,
		      g_active_handler->exception_action,
		      g_active_handler->cont);
}

OBJECT_PTR exception_retry_using(OBJECT_PTR closure,
				 OBJECT_PTR another_protected_blk,
				 OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(another_protected_blk));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR handler_cont = g_active_handler->cont;
  assert(IS_CLOSURE_OBJECT(handler_cont));

  invoke_curtailed_blocks(handler_cont);

  return message_send(g_msg_snd_closure,
		      another_protected_blk,
		      ON_DO_SELECTOR,
		      convert_int_to_object(2),
		      g_active_handler->selector,
		      g_active_handler->exception_action,
		      g_active_handler->cont);
}

OBJECT_PTR exception_resume(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  invoke_curtailed_blocks(g_active_handler->cont);

  assert(!stack_is_empty(g_exception_contexts));
  OBJECT_PTR exception_context = (OBJECT_PTR)stack_pop(g_exception_contexts);

  assert(IS_CLOSURE_OBJECT(exception_context));

  //TODO: check if the exception object is resumable,
  //if it is, return the default resumption value

  return invoke_cont_on_val(exception_context, NIL);
}

OBJECT_PTR exception_resume_with_val(OBJECT_PTR closure, OBJECT_PTR val, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  invoke_curtailed_blocks(g_active_handler->cont);

  assert(!stack_is_empty(g_exception_contexts));
  OBJECT_PTR exception_context = (OBJECT_PTR)stack_pop(g_exception_contexts);

  assert(IS_CLOSURE_OBJECT(exception_context));

  return invoke_cont_on_val(exception_context, val);
}

OBJECT_PTR exception_pass(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  exception_handler_t **entries = (exception_handler_t **)stack_data(g_exception_environment);
  int count = stack_count(g_exception_environment);
  int i = count - 1;

  OBJECT_PTR cls_obj = get_class_object(receiver);
  class_object_t *cls_obj_int = (class_object_t *)extract_ptr((cls_obj));

  while(i >= 0)
  {
    exception_handler_t *handler = entries[i];

    OBJECT_PTR ret;

    if((cls_obj == handler->selector || is_super_class(handler->selector, cls_obj)) && handler != g_active_handler)
    {
      g_active_handler = handler;

      OBJECT_PTR action = handler->exception_action;

      assert(IS_CLOSURE_OBJECT(action)); //MonadicBlock

      g_handler_environment = handler->exception_environment;

      //pop the handler (and all later handlers (is ths correct?))
      //stack_pop(g_exception_environment); //don't think the env should be popped

      ret = message_send(g_msg_snd_closure,
			 action,
			 VALUE1_SELECTOR,
			 convert_int_to_object(1),
			 receiver,
			 handler->cont);

      return ret;
    }

    i--;
  }

  //TODO: incorporate exception message text once this is
  //added as an instance variable to the exception

  char buf[200];
  memset(buf, '\0', 200);

  //printf("Unhandled exception: %s\n", cls_obj_int->name);
  sprintf(buf, "Unhandled exception: %s\n", cls_obj_int->name);

  show_error_dialog(buf);

  //print_call_chain();

  return exception_user_intervention(cont);
}

OBJECT_PTR exception_outer(OBJECT_PTR closure, OBJECT_PTR cont)
{
  stack_push(g_exception_contexts, (void *)cont);

  return exception_pass(closure, cont);
}

OBJECT_PTR exception_resignal_as(OBJECT_PTR closure, OBJECT_PTR new_exception, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  g_exception_environment = g_signalling_environment;

  return signal_exception(new_exception, cont);
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
  cls_obj->instance_methods->count = 11;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_return");
  cls_obj->instance_methods->bindings[0].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_return),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[1].key = get_symbol("_return:");
  cls_obj->instance_methods->bindings[1].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_return_val),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[2].key = get_symbol("_retry");
  cls_obj->instance_methods->bindings[2].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_retry),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[3].key = get_symbol("_retryUsing:");
  cls_obj->instance_methods->bindings[3].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_retry_using),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[4].key = get_symbol("_resume");
  cls_obj->instance_methods->bindings[4].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_resume),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[5].key = get_symbol("_resume:");
  cls_obj->instance_methods->bindings[5].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_resume_with_val),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[6].key = get_symbol("_pass");
  cls_obj->instance_methods->bindings[6].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_pass),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[7].key = get_symbol("_outer");
  cls_obj->instance_methods->bindings[7].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_outer),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[8].key = get_symbol("_signal");
  cls_obj->instance_methods->bindings[8].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_signal),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[9].key = get_symbol("_resignalAs:");
  cls_obj->instance_methods->bindings[9].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_resignal_as),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[10].key = get_symbol("_signal:");
  cls_obj->instance_methods->bindings[10].val = create_method(
						    convert_native_fn_to_object((nativefn)exception_signal_with_text),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 1;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));;

  cls_obj->class_methods->bindings[0].key = get_symbol("_new");
  cls_obj->class_methods->bindings[0].val = create_method(
						 convert_native_fn_to_object((nativefn)new_object),
						 NIL, NIL,
						 0, NIL, NULL);
  
  Exception =  convert_class_object_to_object_ptr(cls_obj);
}

//call this function to signal exceptions with a sginalled text string
//from Smalltalk methods implemented as primitives.
OBJECT_PTR create_and_signal_exception_with_text(OBJECT_PTR excp_class_obj,
						 OBJECT_PTR signaller_text,
						 OBJECT_PTR excp_cont)
{
  assert(IS_CLASS_OBJECT(excp_class_obj));
  assert(excp_class_obj == Exception || is_super_class(Exception, excp_class_obj));

  assert(IS_STRING_LITERAL_OBJECT(signaller_text) || signaller_text == NIL);

  assert(IS_CLOSURE_OBJECT(excp_cont));

  stack_push(g_exception_contexts, (void *)excp_cont);

  OBJECT_PTR excp_obj = new_object_internal(excp_class_obj,
					    convert_fn_to_closure((nativefn)new_object_internal),
					    g_idclo);
  return message_send(g_msg_snd_closure,
		      excp_obj,
		      get_symbol("_signal:"), //TODO: replace with constant
		      convert_int_to_object(1),
		      signaller_text,
		      excp_cont);
}

//call this function to signal exceptions from Smalltalk methods
//implemented as primitives.
OBJECT_PTR create_and_signal_exception(OBJECT_PTR excp_class_obj, OBJECT_PTR excp_cont)
{
  assert(IS_CLASS_OBJECT(excp_class_obj));
  assert(excp_class_obj == Exception || is_super_class(Exception, excp_class_obj));

  assert(IS_CLOSURE_OBJECT(excp_cont));

  stack_push(g_exception_contexts, (void *)excp_cont);

  OBJECT_PTR excp_obj = new_object_internal(excp_class_obj,
					    convert_fn_to_closure((nativefn)new_object_internal),
					    g_idclo);
  return message_send(g_msg_snd_closure,
		      excp_obj,
		      SIGNAL_SELECTOR,
		      convert_int_to_object(0),
		      excp_cont);
}
