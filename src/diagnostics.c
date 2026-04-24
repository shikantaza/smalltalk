#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "global_decls.h"

extern OBJECT_PTR           g_message_selector;
extern unsigned int         g_nof_string_literals;
extern OBJECT_PTR           g_msg_snd_closure;
extern OBJECT_PTR           g_msg_snd_super_closure;
extern OBJECT_PTR           g_compile_time_method_selector;
extern OBJECT_PTR           g_run_till_cont;
extern enum DebugAction     g_debug_action;
extern package_t           *g_smalltalk_symbols;
extern package_t           *g_compiler_package;
extern char               **g_string_literals;
extern stack_type          *g_exception_environment;
extern stack_type          *g_call_chain;
extern stack_type          *g_exception_contexts;
extern stack_type          *g_breakpointed_methods;
extern BOOLEAN              g_loading_core_library;
extern BOOLEAN              g_running_tests;
extern OBJECT_PTR           g_method_call_stack;
extern OBJECT_PTR           g_last_eval_result;
extern BOOLEAN              g_system_initialized;
extern enum UIMode          g_ui_mode;
extern BOOLEAN              g_eval_aborted;
extern executable_code_t   *g_exp;
extern binding_env_t       *g_top_level;
extern BOOLEAN              g_debugger_invoked_for_exception;
extern exception_handler_t *g_active_handler;
extern BOOLEAN              g_debug_in_progress;
extern OBJECT_PTR           g_debug_cont;
extern stack_type          *g_handler_environment;
extern stack_type          *g_signalling_environment;
extern unsigned int         g_nof_compiler_states;

void print_object_to_file(OBJECT_PTR obj, FILE *fp)
{
  char buf[10000];
  memset(buf,'\0',10000);
  print_object_to_string(obj, buf);
  fprintf(fp, "%s", buf);
}

void print_binding(binding_t *binding, FILE*fp)
{
  fprintf(fp, "key: "); print_object_to_file(binding->key, fp);fprintf(fp, "\n");
  fprintf(fp, "val: "); print_object_to_file(binding->val, fp);fprintf(fp, "\n");
}

void print_binding_env(binding_env_t *env, FILE *fp)
{
  fprintf(fp, "binding_env:\n");
  int i;
  fprintf(fp, "count = %d\n", env->count);
  for(i=0; i<env->count; i++)
  {
    print_binding(env->bindings+i, fp);
    fprintf(fp, "\n");
  }
}

//forward declaration
void print_class_object(class_object_t *, FILE *);

void print_method(method_t *m, FILE *fp)
{
  fprintf(fp, "method: \n");
  print_class_object(m->cls_obj, fp);
  fprintf(fp, "class_method: %s\n", m->class_method ? "true" : "false");
  fprintf(fp, "nativefn_obj: "); print_object_to_file(m->nativefn_obj, fp); fprintf(fp, "\n");
  fprintf(fp, "closed_syms: ");  print_object_to_file(m->closed_syms, fp); fprintf(fp, "\n");
  fprintf(fp, "temporaries: ");  print_object_to_file(m->temporaries, fp); fprintf(fp, "\n");
  fprintf(fp, "arity: %d\n", m->arity);
  fprintf(fp, "code_str: ");     print_object_to_file(m->code_str, fp); fprintf(fp, "\n");
  print_executable_code(fp, m->exec_code);
  fprintf(fp, "breakpointed: %s\n", m->breakpointed ? "true" : "false");
}

void print_method_binding(method_binding_t *binding, FILE*fp)
{
  fprintf(fp, "key: "); print_object_to_file(binding->key, fp);fprintf(fp, "\n");
  fprintf(fp, "val:\n");
  print_method(binding->val, fp);
}

void print_method_binding_env(method_binding_env_t *env, FILE *fp)
{
  fprintf(fp, "method_binding_env:\n");
  int i;
  fprintf(fp, "count = %d\n", env->count);
  for(i=0; i<env->count; i++)
  {
    print_method_binding(env->bindings+i, fp);
    fprintf(fp, "\n");
  }
}

void print_class_object(class_object_t *cls_obj, FILE *fp)
{
  fprintf(fp, "class object:\n");

  fprintf(fp, "parent_class_object: "); print_object_to_file(cls_obj->parent_class_object, fp); fprintf(fp, "\n");
  fprintf(fp, "name: %s\n", cls_obj->name);

  fprintf(fp, "package: "); print_object_to_file(cls_obj->package, fp); fprintf(fp, "\n");

  fprintf(fp, "nof_instances: %d\n", cls_obj->nof_instances);
  fprintf(fp, "instances:\n");
  int i;
  for(i=0; i<cls_obj->nof_instances; i++)
  {
    print_object_to_file(cls_obj->instances[i], fp);
    fprintf(fp, "\n");
  }

  fprintf(fp, "nof_instance_vars: %d\n", cls_obj->nof_instance_vars);
  fprintf(fp, "instance_vars:\n");
  for(i=0; i<cls_obj->nof_instance_vars; i++)
  {
    print_object_to_file(cls_obj->inst_vars[i], fp);
    fprintf(fp, "\n");
  }

  print_binding_env(cls_obj->shared_vars, fp);
  print_method_binding_env(cls_obj->instance_methods, fp);
  print_method_binding_env(cls_obj->class_methods, fp);
}

void print_call_chain_entry(call_chain_entry_t *e, FILE *fp)
{
  fprintf(fp, "exp_ptr: ");                 print_object_to_file(e->exp_ptr, fp);                 fprintf(fp, "\n");

  fprintf(fp, "super: %s\n", e->super ? "true" : "false");

  fprintf(fp, "receiver: ");                print_object_to_file(e->receiver, fp);                fprintf(fp, "\n");
  fprintf(fp, "selector: ");                print_object_to_file(e->selector, fp);                fprintf(fp, "\n");

  print_method(e->method, fp);

  fprintf(fp, "closure: ");                 print_object_to_file(e->closure, fp);                 fprintf(fp, "\n");

  fprintf(fp, "nof_args: %d\n", e->nof_args);
  fprintf(fp, "args: \n");
  int i;
  for(i=0; i< e->nof_args; i++)
  {
    print_object_to_file(e->args[i], fp);
    fprintf(fp, "\n");
  }

  fprintf(fp, "local_vars_list: ");         print_object_to_file(e->local_vars_list, fp);         fprintf(fp, "\n");
  fprintf(fp, "cont: ");                    print_object_to_file(e->cont, fp);                    fprintf(fp, "\n");
  fprintf(fp, "termination_blk_closure: "); print_object_to_file(e->termination_blk_closure, fp); fprintf(fp, "\n");

  fprintf(fp, "termination_blk_invoked: %s\n", e->termination_blk_invoked ? "true" : "false");
}

void print_exception_handler(exception_handler_t *h, FILE *fp)
{
  if(!h)
    return;

  fprintf(fp, "protected_block: ");  print_object_to_file(h->protected_block, fp); fprintf(fp, "\n");
  fprintf(fp, "selector: ");         print_object_to_file(h->selector, fp); fprintf(fp, "\n");
  fprintf(fp, "exception_action: "); print_object_to_file(h->exception_action, fp); fprintf(fp, "\n");

  exception_handler_t **env_handlers = (exception_handler_t **)stack_data(h->exception_environment);
  int i, count;
  count = stack_count(h->exception_environment);

  for(i = count-1; i>=0; i--)
    print_exception_handler(env_handlers[i], fp);

  fprintf(fp, "\n");

  fprintf(fp, "cont: "); print_object_to_file(h->cont, fp); fprintf(fp, "\n");
}

void print_diagnostics()
{
  int i;

  FILE *fp = fopen("diagnostics.txt", "w");

  assert(fp);

  fprintf(fp, "g_message_selector: ");
  print_object_to_file(g_message_selector, fp);
  fprintf(fp, "\n");

  fprintf(fp, "nof_string_literals: %d\n", g_nof_string_literals);

  fprintf(fp, "g_msg_snd_closure: ");
  print_object_to_file(g_msg_snd_closure, fp);
  fprintf(fp, "\n");

  fprintf(fp, "g_msg_snd_super_closure: ");
  print_object_to_file(g_msg_snd_super_closure, fp);
  fprintf(fp, "\n");

  fprintf(fp, "g_compile_time_method_selector: ");
  print_object_to_file(g_compile_time_method_selector, fp);
  fprintf(fp, "\n");

  fprintf(fp, "g_run_till_cont: ");
  print_object_to_file(g_run_till_cont, fp);
  fprintf(fp, "\n");

  fprintf(fp, "debug_action: %d\n", g_debug_action);

  fprintf(fp, "nof_smalltalk_symbols: %d\n", g_smalltalk_symbols->nof_symbols);
  fprintf(fp, "smalltalk_symbols:\n");
  for(i=0; i<g_smalltalk_symbols->nof_symbols; i++)
    fprintf(fp,"  %s\n", g_smalltalk_symbols->symbols[i]);

  fprintf(fp, "nof_compiler_package_symbols: %d\n", g_compiler_package->nof_symbols);
  fprintf(fp, "compiler_package_symbols:\n");
  for(i=0; i<g_compiler_package->nof_symbols; i++)
    fprintf(fp,"  %s\n", g_compiler_package->symbols[i]);

  fprintf(fp, "string_literals:\n");
  for(i=0; i<g_nof_string_literals; i++)
    fprintf(fp,"  %s\n", g_string_literals[i]);

  fprintf(fp, "g_exception_environment:\n");
  exception_handler_t **env_handlers = (exception_handler_t **)stack_data(g_exception_environment);
  int count;
  count = stack_count(g_exception_environment);

  for(i = count-1; i>=0; i--)
  {
    print_exception_handler(env_handlers[i], fp);
    fprintf(fp, "\n");
  }

  fprintf(fp, "g_call_chain:\n");
  call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
  count = stack_count(g_call_chain);

  for(i = count-1; i>=0; i--)
  {
    print_call_chain_entry(entries[i], fp);
    fprintf(fp, "\n");
  }

  fprintf(fp, "g_exception_contexts:\n");
  OBJECT_PTR *contexts = (OBJECT_PTR *)stack_data(g_exception_contexts);
  count = stack_count(g_exception_contexts);

  for(i = count-1; i>=0; i--)
  {
    print_object_to_file(contexts[i], fp);
    fprintf(fp, "\n");
  }

  fprintf(fp, "g_breakpointed_methods:\n");
  method_t **methods = (method_t **)stack_data(g_breakpointed_methods);
  count = stack_count(g_breakpointed_methods);

  for(i = count-1; i>=0; i--)
  {
    print_method(methods[i], fp);
    fprintf(fp, "\n");
  }

  fprintf(fp, "g_loading_core_library: %s\n", g_loading_core_library ? "true" : "false");

  fprintf(fp, "g_running_tests: %s\n", g_running_tests ? "true" : "false");

  fprintf(fp, "g_method_call_stack: ");
  print_object_to_file(g_method_call_stack, fp);
  fprintf(fp, "\n");

  fprintf(fp, "g_last_eval_result: ");
  print_object_to_file(g_last_eval_result, fp);
  fprintf(fp, "\n");

  fprintf(fp, "g_system_initialized: %s\n", g_system_initialized ? "true" : "false");

  fprintf(fp, "g_ui_mode: %d\n", g_ui_mode);

  fprintf(fp, "g_eval_aborted: %s\n", g_eval_aborted ? "true" : "false");

  fprintf(fp, "g_exp:\n");
  print_executable_code(fp, g_exp);
  fprintf(fp, "\n");

  fprintf(fp, "g_top_level:\n");
  print_binding_env(g_top_level, fp);
  fprintf(fp, "\n");

  fprintf(fp, "g_debugger_invoked_for_exception: %s\n", g_debugger_invoked_for_exception ? "true" : "false");;

  fprintf(fp, "g_active_handler:\n");
  print_exception_handler(g_active_handler, fp);
  fprintf(fp, "\n");

  fprintf(fp, "g_debug_in_progress: %s\n", g_debug_in_progress ? "true" : "false");;

  fprintf(fp, "g_debug_cont: ");
  print_object_to_file(g_debug_cont, fp);
  fprintf(fp, "\n");

  fprintf(fp, "g_handler_environment:\n");
  if(g_handler_environment)
  {
    env_handlers = (exception_handler_t **)stack_data(g_handler_environment);
    count = stack_count(g_handler_environment);

    for(i = count-1; i>=0; i--)
    {
      print_exception_handler(env_handlers[i], fp);
      fprintf(fp, "\n");
    }
  }
  else
    fprintf(fp, "\n");

  fprintf(fp, "g_signalling_environment:\n");
  if(g_signalling_environment)
  {
    env_handlers = (exception_handler_t **)stack_data(g_signalling_environment);
    count = stack_count(g_signalling_environment);

    for(i = count-1; i>=0; i--)
    {
      print_exception_handler(env_handlers[i], fp);
      fprintf(fp, "\n");
    }
  }
  else
    fprintf(fp, "\n");

  fprintf(fp, "g_nof_compiler_states: %d\n", g_nof_compiler_states);

  fclose(fp);
}
