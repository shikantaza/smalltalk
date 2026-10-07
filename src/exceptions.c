/**
  Copyright 2026 Rajesh Jayaprakash <rajesh.jayaprakash@protonmail.com>

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  It is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this file.  If not, see <http://www.gnu.org/licenses/>.
**/

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

extern enum DebugAction g_debug_action;
extern BOOLEAN g_eval_aborted;

extern enum UIMode g_ui_mode;

void invoke_curtailed_blocks(OBJECT_PTR cont)
{
  assert(!stack_is_empty(g_call_chain));

  call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
  int count = stack_count(g_call_chain);

  int i = count - 1;

  while(i >= 0)
  {
    call_chain_entry_t *entry = entries[i];

    if(entry->cont == cont)
      break;
    
    OBJECT_PTR termination_blk = entry->termination_blk_closure;

    if(termination_blk != NIL &&
       entry->termination_blk_invoked == false)
    {
      message_send(g_msg_snd_closure,
                   termination_blk,
                   NIL,
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
  //for the cases where workspace expression
  //results in an exception without any call chain
  //existent (e.g., unbound variables)
  if(stack_count(g_call_chain) == 0)
    return NIL;

  g_debug_in_progress = true;
  show_debug_window(true, cont);

  while(g_debug_in_progress)
    ; //loop till the debug window returns control

  if(g_debug_action == ABORT)
    g_eval_aborted = true;

  return g_last_eval_result;
}

OBJECT_PTR exception_user_intervention_cli(OBJECT_PTR cont)
{
  //for the cases where workspace expression
  //results in an exception without any call chain
  //existent (e.g., unbound variables)
  if(stack_count(g_call_chain) == 0)
    return NIL;

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
				 entry->exp_ptr,
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
				  NIL,
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

    OBJECT_PTR ret1 = message_send(g_msg_snd_closure,
                                   handler->selector,
                                   NIL,
                                   get_symbol("_handles:"),
                                   convert_int_to_object(1),
                                   exception,
                                   g_idclo);

    //if(cls_obj == handler->selector || is_super_class(handler->selector, cls_obj))
    if(ret1 == TRUE && handler != g_active_handler)
    {
      g_active_handler = handler;

      OBJECT_PTR action = handler->exception_action;
      
      assert(IS_CLOSURE_OBJECT(action)); //MonadicBlock

      g_handler_environment = handler->exception_environment;
      
      //pop the handler (and all later handlers (is ths correct?))
      //stack_pop(g_exception_environment); //don't think the env should be popped

      ret = message_send(g_msg_snd_closure,
			 action,
			 NIL,
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

  if(g_ui_mode == GUI)
  {
    show_error_dialog(buf);
    return exception_user_intervention(cont);
  }
  else if(g_ui_mode == CLI)
  {
    printf("%s\n", buf);
    print_call_chain();
    return exception_user_intervention_cli(cont);
  }
  else
    assert(false);
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
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR handler_cont = g_active_handler->cont;
  assert(IS_CLOSURE_OBJECT(handler_cont));

  invoke_curtailed_blocks(handler_cont);

  OBJECT_PTR protected_block = g_active_handler->protected_block;

  return message_send(g_msg_snd_closure,
		      protected_block,
		      NIL,
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
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(another_protected_blk));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR handler_cont = g_active_handler->cont;
  assert(IS_CLOSURE_OBJECT(handler_cont));

  invoke_curtailed_blocks(handler_cont);

  return message_send(g_msg_snd_closure,
		      another_protected_blk,
		      NIL,
		      ON_DO_SELECTOR,
		      convert_int_to_object(2),
		      g_active_handler->selector,
		      g_active_handler->exception_action,
		      g_active_handler->cont);
}

OBJECT_PTR exception_resume(OBJECT_PTR closure, OBJECT_PTR cont)
{
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  if(g_active_handler)
    invoke_curtailed_blocks(g_active_handler->cont);

  assert(!stack_is_empty(g_exception_contexts));
  OBJECT_PTR exception_context = (OBJECT_PTR)stack_pop(g_exception_contexts);

  assert(IS_CLOSURE_OBJECT(exception_context));

  //TODO: check if the exception object is resumable,
  //if it is, return the default resumption value

  g_active_handler = NULL;

  return invoke_cont_on_val(exception_context, NIL);
}

OBJECT_PTR exception_resume_with_val(OBJECT_PTR closure, OBJECT_PTR val, OBJECT_PTR cont)
{
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  if(g_active_handler)
    invoke_curtailed_blocks(g_active_handler->cont);

  assert(!stack_is_empty(g_exception_contexts));
  OBJECT_PTR exception_context = (OBJECT_PTR)stack_pop(g_exception_contexts);

  assert(IS_CLOSURE_OBJECT(exception_context));

  g_active_handler = NULL;

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

    OBJECT_PTR ret1 = message_send(g_msg_snd_closure,
                                   handler->selector,
                                   NIL,
                                   get_symbol("_handles:"),
                                   convert_int_to_object(1),
                                   receiver,
                                   g_idclo);

    //if((cls_obj == handler->selector || is_super_class(handler->selector, cls_obj)) && handler != g_active_handler)
    if(ret1 == TRUE && handler != g_active_handler)
    {
      g_active_handler = handler;

      OBJECT_PTR action = handler->exception_action;

      assert(IS_CLOSURE_OBJECT(action)); //MonadicBlock

      g_handler_environment = handler->exception_environment;

      //pop the handler (and all later handlers (is ths correct?))
      //stack_pop(g_exception_environment); //don't think the env should be popped

      ret = message_send(g_msg_snd_closure,
			 action,
			 NIL,
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

  sprintf(buf, "Unhandled exception: %s\n", cls_obj_int->name);

  if(g_ui_mode == GUI)
  {
    show_error_dialog(buf);
    return exception_user_intervention(cont);
  }
  else if(g_ui_mode == CLI)
  {
    printf("%s\n", buf);
    print_call_chain();
    return exception_user_intervention_cli(cont);
  }
  else
    assert(false);
}

OBJECT_PTR exception_outer(OBJECT_PTR closure, OBJECT_PTR cont)
{
  stack_push(g_exception_contexts, (void *)cont);

  return exception_pass(closure, cont);
}

OBJECT_PTR exception_resignal_as(OBJECT_PTR closure, OBJECT_PTR new_exception, OBJECT_PTR cont)
{
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
  cls_obj->shared_vars->bindings = NULL;
  
  cls_obj->instance_methods = (method_binding_env_t *)GC_MALLOC(sizeof(method_binding_env_t));
  cls_obj->instance_methods->count = 11;
  cls_obj->instance_methods->bindings = (method_binding_t **)GC_MALLOC(cls_obj->instance_methods->count * sizeof(method_binding_t *));

  cls_obj->instance_methods->bindings[0] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[0]->key = get_symbol("_return");
  cls_obj->instance_methods->bindings[0]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_return),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[1] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[1]->key = get_symbol("_return:");
  cls_obj->instance_methods->bindings[1]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_return_val),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[2] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[2]->key = get_symbol("_retry");
  cls_obj->instance_methods->bindings[2]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_retry),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[3] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[3]->key = get_symbol("_retryUsing:");
  cls_obj->instance_methods->bindings[3]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_retry_using),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[4] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[4]->key = get_symbol("_resume");
  cls_obj->instance_methods->bindings[4]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_resume),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[5] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[5]->key = get_symbol("_resume:");
  cls_obj->instance_methods->bindings[5]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_resume_with_val),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[6] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[6]->key = get_symbol("_pass");
  cls_obj->instance_methods->bindings[6]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_pass),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[7] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[7]->key = get_symbol("_outer");
  cls_obj->instance_methods->bindings[7]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_outer),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[8] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[8]->key = get_symbol("_signal");
  cls_obj->instance_methods->bindings[8]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_signal),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[9] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[9]->key = get_symbol("_resignalAs:");
  cls_obj->instance_methods->bindings[9]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_resignal_as),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[10] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->instance_methods->bindings[10]->key = get_symbol("_signal:");
  cls_obj->instance_methods->bindings[10]->val = create_method(convert_class_object_to_object_ptr(cls_obj), false,
						    convert_native_fn_to_object((nativefn)exception_signal_with_text),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->class_methods = (method_binding_env_t *)GC_MALLOC(sizeof(method_binding_env_t));
  cls_obj->class_methods->count = 1;
  cls_obj->class_methods->bindings = (method_binding_t **)GC_MALLOC(cls_obj->class_methods->count * sizeof(method_binding_t *));;

  cls_obj->class_methods->bindings[0] = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));
  cls_obj->class_methods->bindings[0]->key = get_symbol("_new");
  cls_obj->class_methods->bindings[0]->val = create_method(convert_class_object_to_object_ptr(cls_obj), true,
						 convert_native_fn_to_object((nativefn)new_object),
						 NIL, NIL,
						 0, NIL, NULL);
  
  cls_obj->nof_aliases = 0;
  cls_obj->aliases = NULL;

  Exception =  convert_class_object_to_object_ptr(cls_obj);
}

//call this function to signal exceptions with a signalled text string
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
		      NIL,
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
		      NIL,
		      SIGNAL_SELECTOR,
		      convert_int_to_object(0),
		      excp_cont);
}
