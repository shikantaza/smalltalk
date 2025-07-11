#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "global_decls.h"

#include "util.h"
#include "stack.h"

//workaround for variadic function arguments
//getting clobbered in ARM64
typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

void add_to_autocomplete_list(char *word);

binding_env_t *g_top_level;

OBJECT_PTR Object;
OBJECT_PTR Smalltalk;
OBJECT_PTR Nil;
OBJECT_PTR Compiler;
OBJECT_PTR CompileError;

OBJECT_PTR g_compile_time_method_selector;

int smalltalk_gensym_count = 0;

OBJECT_PTR CompileError;

OBJECT_PTR Package;

stack_type *g_breakpointed_methods = NULL;

extern OBJECT_PTR Array;
extern OBJECT_PTR InvalidArgument;
extern OBJECT_PTR NIL;
extern OBJECT_PTR MESSAGE_SEND;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Integer;
extern OBJECT_PTR Float;
extern OBJECT_PTR Transcript;
extern OBJECT_PTR NiladicBlock;
extern OBJECT_PTR MonadicBlock;
extern OBJECT_PTR Boolean;

extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

extern OBJECT_PTR THIS_CONTEXT;
extern OBJECT_PTR RETURN_FROM;

extern OBJECT_PTR Exception;

extern stack_type *g_call_chain;

extern OBJECT_PTR g_idclo;
extern OBJECT_PTR g_msg_snd_closure;
extern OBJECT_PTR g_msg_snd_super_closure;

extern OBJECT_PTR MESSAGE_SEND_SUPER;

extern stack_type *g_exception_contexts;

extern OBJECT_PTR OrderedCollection;

extern char **g_string_literals;
extern executable_code_t *g_exp;

extern OBJECT_PTR g_message_selector;

extern OBJECT_PTR MNU_SYMBOL;

extern OBJECT_PTR Error;

extern BOOLEAN g_system_initialized;

extern OBJECT_PTR ReadableString;

extern OBJECT_PTR DyadicValuable;
extern OBJECT_PTR Character;

void add_binding_to_top_level(OBJECT_PTR sym, OBJECT_PTR val)
{
  g_top_level->count++;

  if(!g_top_level->bindings)
    g_top_level->bindings = (binding_t *)GC_MALLOC(sizeof(binding_t));
  else
  {
    binding_t *temp = (binding_t *)GC_REALLOC(g_top_level->bindings,
					      g_top_level->count * sizeof(binding_t));
    assert(temp);
    g_top_level->bindings = temp;
  }

  g_top_level->bindings[g_top_level->count - 1].key = sym;
  g_top_level->bindings[g_top_level->count - 1].val = val;
}

//there is a similar function in lisp_compiler.c
//(get_binding_val()), that has a slightly different behaviour
BOOLEAN get_binding_val_regular(binding_env_t *env, OBJECT_PTR sym, OBJECT_PTR *ret)
{
  assert(IS_SYMBOL_OBJECT(sym));

  unsigned int i, n = env->count;

  for(i=0; i<n; i++)
  {
    if(env->bindings[i].key == sym)
    {
      *ret = env->bindings[i].val;
      return true;
    }
  }

  *ret = NIL;
  return false;
  
}

BOOLEAN get_top_level_val(OBJECT_PTR sym, OBJECT_PTR *ret)
{
  return get_binding_val_regular(g_top_level, sym, ret);
}

void initialize_top_level()
{
  g_top_level = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  g_top_level->count = 0;
  g_top_level->bindings = NULL;

  add_binding_to_top_level(MESSAGE_SEND, cons(g_msg_snd_closure, NIL));
  add_binding_to_top_level(MESSAGE_SEND_SUPER, cons(g_msg_snd_super_closure, NIL));
  add_binding_to_top_level(get_symbol("Integer"), cons(Integer, NIL));
  add_binding_to_top_level(get_symbol("Float"), cons(Float, NIL));
  add_binding_to_top_level(SELF, cons(NIL, NIL));
  add_binding_to_top_level(get_symbol("Object"), cons(Object, NIL));
  add_binding_to_top_level(get_symbol("Smalltalk"), cons(Smalltalk, NIL));
  add_binding_to_top_level(get_symbol("Nil"), cons(Nil, NIL));
  add_binding_to_top_level(get_symbol("nil"), cons(NIL, NIL));
  add_binding_to_top_level(get_symbol("Transcript"), cons(Transcript, NIL));
  add_binding_to_top_level(get_symbol("NiladicBlock"), cons(NiladicBlock, NIL));
  add_binding_to_top_level(get_symbol("MonadicBlock"), cons(MonadicBlock, NIL));
  add_binding_to_top_level(get_symbol("Boolean"), cons(Boolean, NIL));

  add_binding_to_top_level(get_symbol("true"), cons(TRUE, NIL));
  add_binding_to_top_level(get_symbol("false"), cons(FALSE, NIL));

  add_binding_to_top_level(THIS_CONTEXT, cons(NIL, NIL)); //g_idclo will be set at each repl loop start

  add_binding_to_top_level(get_symbol("Exception"), cons(Exception, NIL));

  add_binding_to_top_level(get_symbol("Array"), cons(Array, NIL));
  add_binding_to_top_level(get_symbol("OrderedCollection"), cons(OrderedCollection, NIL));
  add_binding_to_top_level(get_symbol("Compiler"), cons(Compiler, NIL));

  add_binding_to_top_level(get_symbol("ReadableString"), cons(ReadableString, NIL));
  add_binding_to_top_level(get_symbol("DyadicValuable"), cons(DyadicValuable, NIL));
  add_binding_to_top_level(get_symbol("Character"), cons(Character, NIL));
}

BOOLEAN exists_in_top_level(OBJECT_PTR sym)
{
  assert(IS_SYMBOL_OBJECT(sym));

  unsigned int i, n = g_top_level->count;

  for(i=0; i<n; i++)
    if(g_top_level->bindings[i].key == sym)
      return true;

  return false;
}

OBJECT_PTR create_class(OBJECT_PTR closure,
			OBJECT_PTR class_sym,
			OBJECT_PTR parent_class_object,
			OBJECT_PTR cont)
{
  //TODO: what to do if the class to be created
  //already exists?

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  assert(IS_CLOSURE_OBJECT(closure));

  if(!IS_SMALLTALK_SYMBOL_OBJECT(class_sym))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_CLASS_OBJECT(parent_class_object))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_class(): Unable to allocate memory\n");
    exit(1);
  }
  
  cls_obj->parent_class_object = parent_class_object;
  cls_obj->name = GC_strdup(get_smalltalk_symbol_name(class_sym));

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  cls_obj->shared_vars->bindings = NULL;

  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->instance_methods->count = 0;
  cls_obj->instance_methods->bindings = NULL;
  
  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 1;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));
  
  cls_obj->class_methods->bindings[0].key = get_symbol("_new");
  cls_obj->class_methods->bindings[0].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)new_object),
						 NIL, NIL,
						 0, NIL, NULL);

  OBJECT_PTR class_object = convert_class_object_to_object_ptr(cls_obj);
  
  add_binding_to_top_level(get_symbol(get_smalltalk_symbol_name(class_sym)), cons(class_object, NIL));

  add_to_autocomplete_list(cls_obj->name);

  pop_if_top(entry);

  return invoke_cont_on_val(cont, class_object);
}

OBJECT_PTR create_class_no_parent_class(OBJECT_PTR closure,
					OBJECT_PTR class_sym,
					OBJECT_PTR cont)
{
  return create_class(closure, class_sym, Object, cont);
}

OBJECT_PTR add_instance_var(OBJECT_PTR closure,
			    OBJECT_PTR var,
			    OBJECT_PTR class_obj,
			    OBJECT_PTR cont)
{
  //TODO: add a default value for the variable?

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  assert(IS_CLOSURE_OBJECT(closure));

  if(!IS_CLASS_OBJECT(class_obj))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_SMALLTALK_SYMBOL_OBJECT(var))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR var_sym = get_symbol(get_smalltalk_symbol_name(var));
  
  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_obj);

  unsigned int i, n;

  //check that the instance variable doesn't already exist
  n = cls_obj->nof_instance_vars;
  for(i=0; i<n; i++)
    if(cls_obj->inst_vars[i] == var_sym)
    {
      printf("Instance variable %s already exists\n", get_smalltalk_symbol_name(var));
      return NIL;
    }

  //check that the instance variable doesn't conflict with an existing class variable
  n = cls_obj->shared_vars->count;
  for(i=0; i<n; i++)
    if(cls_obj->shared_vars->bindings[i].key == var_sym)
    {
      printf("Instance variable %s conflicts with an existing class variable\n", get_smalltalk_symbol_name(var));
      return NIL;
    }
  
  cls_obj->nof_instance_vars++;

  if(!cls_obj->inst_vars)
    cls_obj->inst_vars = (OBJECT_PTR *)GC_MALLOC(cls_obj->nof_instance_vars * sizeof(OBJECT_PTR));
  else
  {
    OBJECT_PTR *temp = (OBJECT_PTR *)GC_REALLOC(cls_obj->inst_vars,
						cls_obj->nof_instance_vars * sizeof(OBJECT_PTR));
    assert(temp);
    cls_obj->inst_vars = temp;
  }

  cls_obj->inst_vars[cls_obj->nof_instance_vars-1] = var_sym;

  //add the instance variable to all the existing instances of the class

  n = cls_obj->nof_instances;
  
  for(i=0; i<n; i++)
  {
    object_t *inst = (object_t *)extract_ptr(cls_obj->instances[i]);

    assert(inst);
    
    if(!inst->instance_vars)
    {
      inst->instance_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
      inst->instance_vars->count = 0;
    }
    else
      inst->instance_vars->count++;
    
    inst->instance_vars->bindings[inst->instance_vars->count].key = var_sym;
    inst->instance_vars->bindings[inst->instance_vars->count].val = cons(NIL, NIL);
  }

  pop_if_top(entry);

  return invoke_cont_on_val(cont, class_obj);
}

OBJECT_PTR add_class_var(OBJECT_PTR closure,
			 OBJECT_PTR var,
			 OBJECT_PTR class_obj,
			 OBJECT_PTR cont)
{
  //TODO: add a default value for the variable?

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  assert(IS_CLOSURE_OBJECT(closure));

  if(!IS_CLASS_OBJECT(class_obj))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_SMALLTALK_SYMBOL_OBJECT(var))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

#ifdef DEBUG  
  printf("%s\n", get_smalltalk_symbol_name(var));
#endif
  
  OBJECT_PTR var_sym = get_symbol(get_smalltalk_symbol_name(var));
  
  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_obj);

  //check that the class variable doesn't already exist
  unsigned int i, n;
  n = cls_obj->shared_vars->count;
  for(i=0; i<n; i++)
    if(cls_obj->shared_vars->bindings[i].key == var_sym)
    {
      printf("Class variable %s already exists\n", get_smalltalk_symbol_name(var));
      return NIL;
    }
  //check that the class variable doesn't conflict with an existing instance variable
  n = cls_obj->nof_instance_vars;
  for(i=0; i<n; i++)
    if(cls_obj->inst_vars[i] == var_sym)
    {
      printf("Class variable %s conflicts with an existing instance variable\n", get_smalltalk_symbol_name(var));
      return NIL;
    }
  
  if(!cls_obj->shared_vars->bindings)
  {
    cls_obj->shared_vars->bindings = (binding_t *)GC_MALLOC(sizeof(binding_t));
    cls_obj->shared_vars->count = 1;
  }
  else
  {
    cls_obj->shared_vars->count++;
    binding_t *temp = (binding_t *)GC_REALLOC(cls_obj->shared_vars->bindings,
					      cls_obj->shared_vars->count * sizeof(binding_t));
    assert(temp);
    cls_obj->shared_vars->bindings = temp;
  }
  
  cls_obj->shared_vars->bindings[cls_obj->shared_vars->count-1].key = var_sym;
  cls_obj->shared_vars->bindings[cls_obj->shared_vars->count-1].val = cons(NIL, NIL);

  pop_if_top(entry);

  return invoke_cont_on_val(cont, class_obj);
}

OBJECT_PTR add_method_str_internal(OBJECT_PTR class_obj,
				   OBJECT_PTR selector,
				   OBJECT_PTR code_str,
				   OBJECT_PTR cont,
				   BOOLEAN instance_method)
{
  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  OBJECT_PTR ret;

  executable_code_t *prev_exp = g_exp;

  char *buf = GC_strdup(g_string_literals[code_str >> OBJECT_SHIFT]);

  yy_scan_string(buf);

  if(!yyparse())
  {
    OBJECT_PTR exp = convert_exec_code_to_lisp(g_exp);

    exp = decorate_message_selectors(exp);

    //g_exp = prev_exp;

    if(instance_method)
      ret = add_instance_method(class_obj, selector, exp, code_str, g_exp);
    else
      ret = add_class_method(class_obj, selector, exp, code_str, g_exp);

    g_exp = prev_exp;

    pop_if_top(entry);

    return invoke_cont_on_val(cont, ret);
  }
  else
  {
    g_exp = prev_exp;

    return create_and_signal_exception(CompileError, cont);
  }
}

OBJECT_PTR add_instance_method_str(OBJECT_PTR closure,
				   OBJECT_PTR selector,
				   OBJECT_PTR class_obj,
				   OBJECT_PTR code_str,
				   OBJECT_PTR cont)
{
  if(!IS_CLASS_OBJECT(class_obj))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_SMALLTALK_SYMBOL_OBJECT(selector))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_STRING_LITERAL_OBJECT(code_str))
    return create_and_signal_exception(InvalidArgument, cont);

  return add_method_str_internal(class_obj, selector, code_str, cont, true);
}

OBJECT_PTR add_class_method_str(OBJECT_PTR closure,
				OBJECT_PTR selector,
				OBJECT_PTR class_obj,
				OBJECT_PTR code_str,
				OBJECT_PTR cont)
{
  if(!IS_CLASS_OBJECT(class_obj))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_SMALLTALK_SYMBOL_OBJECT(selector))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_STRING_LITERAL_OBJECT(code_str))
    return create_and_signal_exception(InvalidArgument, cont);

  return add_method_str_internal(class_obj, selector, code_str, cont, false);
}

OBJECT_PTR create_method(class_object_t *cls_obj,
			 BOOLEAN class_method,
			 OBJECT_PTR nfo,
			 OBJECT_PTR closed_syms,
			 OBJECT_PTR temporaries,
			 unsigned int arity,
			 OBJECT_PTR code_str,
			 executable_code_t *exec_code)
{
  method_t *m;

  if(allocate_memory((void **)&m, sizeof(method_t)))
  {
    printf("create_method(): Unable to allocate memory\n");
    exit(1);
  }

  m->cls_obj      = cls_obj;
  m->class_method = class_method;
  m->nativefn_obj = nfo;
  m->closed_syms  = closed_syms;
  m->temporaries  = temporaries;
  m->arity        = arity;
  m->code_str     = code_str;
  m->exec_code    = exec_code;
  m->breakpointed = false;

  return (uintptr_t)m + OBJECT_TAG;
}

OBJECT_PTR add_instance_method(OBJECT_PTR class_obj,
			       OBJECT_PTR selector,
			       OBJECT_PTR code,
			       OBJECT_PTR code_str,
			       executable_code_t *exec_code)
{
  //these asserts are redundant since the checks
  //are being done in repl() now
  assert(IS_CLASS_OBJECT(class_obj));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(selector));
  assert(IS_CONS_OBJECT(code)); //TODO: maybe some stronger checks?

  g_compile_time_method_selector = get_symbol(prepend_char(get_smalltalk_symbol_name(selector),'_'));

  OBJECT_PTR selector_sym = g_compile_time_method_selector;

  OBJECT_PTR code1 = replace_method_selector(code, selector_sym);

  OBJECT_PTR last_stmt = last_cell(third(third(code1)));
  //if(car(last_stmt) != RETURN_FROM)
    setcdr(last_stmt, cons(list(3, RETURN_FROM, selector_sym, SELF), NIL));

  OBJECT_PTR res = apply_lisp_transforms(code1);

  void *state = compile_to_c(res);

  char *fname = extract_variable_string(fourth(first(res)), true);

  nativefn nf = get_function(state, fname);

  assert(nf);
  
  OBJECT_PTR closed_vals = cdr(CDDDR(first(res)));

  //closed vals are only the variable names
  //and don't have any significance here as 
  //they will be dereferenced only when the method
  //is invoked, at which time the correct values
  //would have been populated
  OBJECT_PTR lst_form = list(3, convert_native_fn_to_object(nf), closed_vals, second(first(res)));
  OBJECT_PTR closure_form = extract_ptr(lst_form) + CLOSURE_TAG;

  put_binding_val(g_top_level, THIS_CONTEXT, cons(g_idclo, NIL));
  
  OBJECT_PTR result_closure = invoke_cont_on_val(closure_form, g_idclo);

  assert(IS_CLOSURE_OBJECT(result_closure));
  
  OBJECT_PTR nfo = convert_native_fn_to_object(extract_native_fn(result_closure));

#ifdef DEBUG  
  print_object(closed_vals); printf("\n");
#endif
  
  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_obj);
  
  unsigned int i, n;
  n = cls_obj->instance_methods->count;

  BOOLEAN existing_method = false;
  
  for(i=0; i<n; i++)
    if(cls_obj->instance_methods->bindings[i].key == selector_sym)
    {
      existing_method = true;
      cls_obj->instance_methods->bindings[i].val = create_method(cls_obj,
								 false,
								 nfo,
								 closed_vals,
								 capture_local_var_names(code1),
								 cons_length(second(third(code1))),
								 code_str,
								 exec_code);
      break;
    }

  if(existing_method == false)
  {
    if(!cls_obj->instance_methods->bindings)
    {
      cls_obj->instance_methods->count = 1;
      cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));
    }
    else
    {
      cls_obj->instance_methods->count++;
      binding_t *temp = (binding_t *)GC_REALLOC(cls_obj->instance_methods->bindings,
						cls_obj->instance_methods->count * sizeof(binding_t));
      assert(temp);
      cls_obj->instance_methods->bindings = temp;
    }
    
    cls_obj->instance_methods->bindings[cls_obj->instance_methods->count - 1].key = selector_sym;
    cls_obj->instance_methods->bindings[cls_obj->instance_methods->count - 1].val =
      create_method(cls_obj,
		    false,
		    nfo,
		    closed_vals,
		    capture_local_var_names(code1),
		    cons_length(second(third(code1))),
		    code_str,
		    exec_code);
  }

  add_to_autocomplete_list(get_smalltalk_symbol_name(selector));

  return class_obj;
}

//replaces the first parameter to RETURN_FROM with the correct
//method selector (from add_class_method() and add_instance_method()).
//needed because the method definition/compilation (via these
//methods) happens after the conversion to Lisp code in repl2()
OBJECT_PTR replace_method_selector(OBJECT_PTR code, OBJECT_PTR selector)
{
  if(is_atom(code))
    return code;
  else
  {
    if(car(code) == RETURN_FROM)
      return concat(2, list(2, RETURN_FROM, selector), replace_method_selector(cdr(cdr(code)), selector));
    else
      return cons(replace_method_selector(car(code), selector),
		  replace_method_selector(cdr(code), selector));
  }
}

OBJECT_PTR add_class_method(OBJECT_PTR class_obj,
			    OBJECT_PTR selector,
			    OBJECT_PTR code,
			    OBJECT_PTR code_str,
			    executable_code_t *exec_code)
{
  //these asserts are redundant since the checks
  //are being done in repl() now
  assert(IS_CLASS_OBJECT(class_obj));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(selector));
  assert(IS_CONS_OBJECT(code)); //TODO: maybe some stronger checks?

  g_compile_time_method_selector = get_symbol(prepend_char(get_smalltalk_symbol_name(selector),'_'));
  
  OBJECT_PTR selector_sym = g_compile_time_method_selector;

  OBJECT_PTR code1 = replace_method_selector(code, selector_sym);

  OBJECT_PTR last_stmt = last_cell(third(third(code1)));
  //if(car(last_stmt) != RETURN_FROM)
    setcdr(last_stmt, cons(list(3, RETURN_FROM, selector_sym, SELF), NIL));

  OBJECT_PTR res = apply_lisp_transforms(code1);

  void *state = compile_to_c(res);

  char *fname = extract_variable_string(fourth(first(res)), true);

  nativefn nf = get_function(state, fname);

  assert(nf);

  OBJECT_PTR closed_vals = cdr(CDDDR(first(res)));

  //closed vals are only the variable names
  //and don't have any significance here as 
  //they will be dereferenced only when the method
  //is invoked, at which time the correct values
  //would have been populated
  OBJECT_PTR lst_form = list(3, convert_native_fn_to_object(nf), closed_vals, second(first(res)));
  OBJECT_PTR closure_form = extract_ptr(lst_form) + CLOSURE_TAG;

  put_binding_val(g_top_level, THIS_CONTEXT, cons(g_idclo, NIL));
  
  OBJECT_PTR result_closure = invoke_cont_on_val(closure_form, g_idclo);

  assert(IS_CLOSURE_OBJECT(result_closure));
  
  OBJECT_PTR nfo = convert_native_fn_to_object(extract_native_fn(result_closure));

#ifdef DEBUG  
  print_object(closed_vals); printf("\n");
#endif
  
  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_obj);
  
  unsigned int i, n;
  n = cls_obj->class_methods->count;

  BOOLEAN existing_method = false;
  
  for(i=0; i<n; i++)
    if(cls_obj->class_methods->bindings[i].key == selector_sym)
    {
      existing_method = true;
      cls_obj->class_methods->bindings[i].val = create_method(cls_obj,
							      true,
							      nfo,
							      closed_vals,
							      capture_local_var_names(code),
							      cons_length(second(third(code))),
							      code_str,
							      exec_code);
      break;
    }

  if(existing_method == false)
  {
    if(!cls_obj->class_methods->bindings)
    {
      cls_obj->class_methods->count = 1;
      cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));
    }
    else
    {
      cls_obj->class_methods->count++;
      binding_t *temp = (binding_t *)GC_REALLOC(cls_obj->class_methods->bindings,
						cls_obj->class_methods->count * sizeof(binding_t));
      assert(temp);
      cls_obj->class_methods->bindings = temp;
    }

    cls_obj->class_methods->bindings[cls_obj->class_methods->count - 1].key = selector_sym;
    cls_obj->class_methods->bindings[cls_obj->class_methods->count - 1].val =
      create_method(cls_obj,
		    true,
		    nfo,
		    closed_vals,
		    capture_local_var_names(code),
		    cons_length(second(third(code))),
		    code_str,
		    exec_code);
  }

  add_to_autocomplete_list(get_smalltalk_symbol_name(selector));

  return class_obj;
}

//we need to create objects internally too (not through
//message sends in Smalltalk code -- e.g., exception
//objects)
OBJECT_PTR new_object_internal(OBJECT_PTR receiver,
			       OBJECT_PTR closure,
			       OBJECT_PTR cont)
{
#ifdef DEBUG
  printf("Entering new_object_internal()\n");
#endif

  //OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));
  
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLASS_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));

  //object_t *obj = (object_t *)GC_MALLOC(sizeof(object_t));
  object_t *obj;

  if(allocate_memory((void **)&obj, sizeof(object_t)))
  {
    printf("new_object_internal(): Unable to allocate memory\n");
    exit(1);
  }
  
  obj->class_object = receiver;
  obj->instance_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));

  obj->instance_vars->count = 0;
  obj->instance_vars->bindings = NULL;

  class_object_t *cls_obj = (class_object_t *)extract_ptr(receiver);

  OBJECT_PTR current_parent = receiver;
  
  while(current_parent != Object)
  {
    class_object_t *curr_cls_obj = (class_object_t *)extract_ptr(current_parent);

    unsigned int n = curr_cls_obj->nof_instance_vars;
    unsigned int i;

    if(n > 0)
    {
      unsigned int prev_count = obj->instance_vars->count;
    
      obj->instance_vars->count += n;

      if(!obj->instance_vars->bindings)
	obj->instance_vars->bindings = (binding_t *)GC_MALLOC(obj->instance_vars->count * sizeof(binding_t));
      else
      {
	binding_t *temp = (binding_t *)GC_REALLOC(obj->instance_vars->bindings,
						  obj->instance_vars->count * sizeof(binding_t));
	assert(temp);
	obj->instance_vars->bindings = temp;
      }

      for(i=prev_count; i<prev_count + n; i++)
      {
	obj->instance_vars->bindings[i].key = curr_cls_obj->inst_vars[i - prev_count];
	obj->instance_vars->bindings[i].val = cons(NIL, NIL);
      }
    }

    current_parent = curr_cls_obj->parent_class_object;
  }

  OBJECT_PTR obj_ptr = (uintptr_t)obj + OBJECT_TAG;

  OBJECT_PTR ret1 = initialize_object(obj_ptr);

  if(ret1 == NIL)
    return NIL;

  //add the new instance to the instances mapped to the class
  cls_obj->nof_instances++;

  if(!cls_obj->instances)
    cls_obj->instances = (OBJECT_PTR *)GC_MALLOC(cls_obj->nof_instances * sizeof(OBJECT_PTR));
  else
  {
    OBJECT_PTR *temp = (OBJECT_PTR *)GC_REALLOC(cls_obj->instances, cls_obj->nof_instances * sizeof(OBJECT_PTR));
    assert(temp);
    cls_obj->instances = temp;
  }

  cls_obj->instances[cls_obj->nof_instances-1] = obj_ptr;
  
  OBJECT_PTR ret = invoke_cont_on_val(cont, obj_ptr);
  
#ifdef DEBUG
  printf("Exiting new_object_internal()\n");
#endif

  return ret;
}

OBJECT_PTR new_object(OBJECT_PTR closure,
		      OBJECT_PTR cont)
{
  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  pop_if_top(entry);
  return new_object_internal(car(get_binding_val(g_top_level, SELF)), closure, cont);
}

OBJECT_PTR object_eq(OBJECT_PTR closure, OBJECT_PTR arg, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

#ifdef DEBUG
  print_object(arg); printf(" is the arg passed to eq\n");
#endif

  assert(IS_CLOSURE_OBJECT(cont));

  pop_if_top(entry);

  return invoke_cont_on_val(cont, (receiver == arg) ? TRUE : FALSE );
}

OBJECT_PTR object_message_not_understood(OBJECT_PTR closure, OBJECT_PTR selector, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_SYMBOL_OBJECT(selector));
  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR ret;

  assert(get_top_level_val(MNU_SYMBOL, &ret));

  OBJECT_PTR mnu_class_obj = car(ret);

  //TODO: the exception object's messageText has to be set (after adding it as an instance variable)
  OBJECT_PTR exception_obj = new_object_internal(mnu_class_obj,
						 convert_fn_to_closure((nativefn)new_object_internal),
						 g_idclo);
  char *selector_str = get_symbol_name(selector);
  char buf[300], obj_str[100];
  print_object_to_string(receiver, obj_str);
  sprintf(buf, "Object %s does not understand message '%s'", obj_str, selector_str);

  return signal_exception_with_text(exception_obj, get_string_obj(buf), cont);
}

void create_Object()
{
  //class_object_t *cls_obj = (class_object_t *)GC_MALLOC(sizeof(class_object_t));
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Array(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = NIL;
  cls_obj->name = GC_strdup("Object");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;
  
  cls_obj->shared_vars = NULL;

  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->instance_methods->count = 2;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));
  
  cls_obj->instance_methods->bindings[0].key = get_symbol("_=");
  cls_obj->instance_methods->bindings[0].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)object_eq),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[1].key = get_symbol("_messageNotUnderstood:");
  cls_obj->instance_methods->bindings[1].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)object_message_not_understood),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->class_methods->count = 1;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));

  cls_obj->class_methods->bindings[0].key = get_symbol("_messageNotUnderstood:");
  cls_obj->class_methods->bindings[0].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)object_message_not_understood),
						 NIL, NIL,
						 1, NIL, NULL);

  Object = (uintptr_t)cls_obj + CLASS_OBJECT_TAG;
}

OBJECT_PTR create_global_valued(OBJECT_PTR closure,
				OBJECT_PTR global_sym,
				OBJECT_PTR global_val,
				OBJECT_PTR cont)
{
  assert(IS_CLOSURE_OBJECT(closure));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  if(!IS_SMALLTALK_SYMBOL_OBJECT(global_sym))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  add_binding_to_top_level(get_symbol(get_smalltalk_symbol_name(global_sym)), cons(global_val, NIL));

  add_to_autocomplete_list(get_smalltalk_symbol_name(global_sym));

  pop_if_top(entry);

  OBJECT_PTR ret = invoke_cont_on_val(cont, global_val);

  return ret;
}

OBJECT_PTR create_global(OBJECT_PTR closure,
			 OBJECT_PTR global_sym,
			 OBJECT_PTR cont)
{
  return create_global_valued(closure, global_sym, NIL, cont);
}

OBJECT_PTR smalltalk_gensym(OBJECT_PTR closure,
			    OBJECT_PTR cont)
{
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLOSURE_OBJECT(cont));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char sym[20];

  smalltalk_gensym_count++;

  sprintf(sym, "#:G%d", smalltalk_gensym_count);

  pop_if_top(entry);

  OBJECT_PTR ret = invoke_cont_on_val(cont, get_smalltalk_symbol(sym));

  return ret;
}

OBJECT_PTR smalltalk_eval(OBJECT_PTR closure,
			  OBJECT_PTR source_buffer,
			  OBJECT_PTR cont)
{
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_STRING_LITERAL_OBJECT(source_buffer));
  assert(IS_CLOSURE_OBJECT(cont));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  OBJECT_PTR ret;

  executable_code_t *prev_exp = g_exp;

  char *buf = GC_strdup(g_string_literals[source_buffer >> OBJECT_SHIFT]);

  yy_scan_string(buf);

  if(!yyparse())
  {
    OBJECT_PTR closure_form = repl_common();

    if(closure_form != NIL)
    {
      put_binding_val(g_top_level, THIS_CONTEXT, cons(g_idclo, NIL));

      pop_if_top(entry);

      ret = invoke_cont_on_val(closure_form, g_idclo);
    }
    else
      ret = NIL;

    g_exp = prev_exp;

    pop_if_top(entry);

    return invoke_cont_on_val(cont, ret);
  }
  else
  {
    g_exp = prev_exp;
    return create_and_signal_exception(CompileError, cont);
  }
}

OBJECT_PTR smalltalk_load_file(OBJECT_PTR closure,
			       OBJECT_PTR file_name,
			       OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));

  if(!IS_STRING_LITERAL_OBJECT(file_name))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *file_name_str = GC_strdup(g_string_literals[file_name >> OBJECT_SHIFT]);

  char qualified_file_name[200];
  memset(qualified_file_name, '\0', 200);

  if(g_system_initialized)
    sprintf(qualified_file_name, "%s", file_name_str);
  else
    sprintf(qualified_file_name, "%s/%s", SMALLTALKDATADIR, file_name_str);

  FILE *fp = fopen(qualified_file_name, "r");

  if(!fp)
  {
    char buf[300];
    sprintf(buf, "Unable to open file '%s'", file_name_str);
    return create_and_signal_exception_with_text(Exception, get_string_obj(buf), cont);
  }

  //TODO: handle errors from this
  parse_from_fp(fp);

  fclose(fp);

  pop_if_top(entry);
  return invoke_cont_on_val(cont, receiver);
}

OBJECT_PTR smalltalk_add_remove_breakpoint(OBJECT_PTR closure,
					   OBJECT_PTR selector,
					   OBJECT_PTR class_obj,
					   BOOLEAN add_breakpoint,
					   OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  if(!IS_SMALLTALK_SYMBOL_OBJECT(selector))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_CLASS_OBJECT(class_obj))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR decorated_selector = get_symbol(prepend_char(get_smalltalk_symbol_name(selector),'_'));

  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_obj);

  unsigned int i, n;
  n = cls_obj->instance_methods->count;

  BOOLEAN method_found = false;

  for(i=0; i<n; i++)
  {
    if(cls_obj->instance_methods->bindings[i].key == decorated_selector)
    {
      method_found = true;
      method_t *m = (method_t *)extract_ptr(cls_obj->instance_methods->bindings[i].val);
      m->breakpointed = add_breakpoint;
      break;
    }
  }

  if(!method_found)
  {
    n = cls_obj->class_methods->count;

    for(i=0; i<n; i++)
    {
      if(cls_obj->class_methods->bindings[i].key == decorated_selector)
      {
	method_found = true;
	method_t *m = (method_t *)extract_ptr(cls_obj->class_methods->bindings[i].val);
	m->breakpointed = add_breakpoint;
	break;
      }
    }
  }

  if(!method_found)
  {
    char buf[300];
    sprintf(buf, "Method '%s' does not exist in class %s",
	    get_smalltalk_symbol_name(selector),
	    cls_obj->name);
    return create_and_signal_exception_with_text(Error, get_string_obj(buf), cont);
  }

  pop_if_top(entry);

  OBJECT_PTR ret = invoke_cont_on_val(cont, receiver);

  return ret;
}

OBJECT_PTR smalltalk_add_breakpoint(OBJECT_PTR closure,
				    OBJECT_PTR selector,
				    OBJECT_PTR class_obj,
				    OBJECT_PTR cont)
{
  return smalltalk_add_remove_breakpoint(closure, selector, class_obj, true, cont);
}

OBJECT_PTR smalltalk_remove_breakpoint(OBJECT_PTR closure,
				       OBJECT_PTR selector,
				       OBJECT_PTR class_obj,
				       OBJECT_PTR cont)
{
  return smalltalk_add_remove_breakpoint(closure, selector, class_obj, false, cont);
}

OBJECT_PTR smalltalk_assign_class_to_package(OBJECT_PTR closure,
					     OBJECT_PTR class_obj,
					     OBJECT_PTR pkg_str,
					     OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  assert(IS_CLOSURE_OBJECT(closure));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  if(!IS_CLASS_OBJECT(class_obj))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_STRING_LITERAL_OBJECT(pkg_str))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  OBJECT_PTR pkg = message_send(g_msg_snd_closure,
				Package,
				NIL,
				get_symbol("_get:"),
				convert_int_to_object(1),
				pkg_str,
				g_idclo);

  //the above message send results in an exception
  if(!call_chain_entry_exists(entry))
  {
    pop_if_top(entry);
    return invoke_cont_on_val(cont, receiver);
  }

  class_object_t *cls = (class_object_t *)extract_ptr(class_obj);

  cls->package = pkg;

  pop_if_top(entry);

  return invoke_cont_on_val(cont, receiver);
}

void create_Smalltalk()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Smalltalk(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("Smalltalk");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  
  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 0;    
  cls_obj->instance_methods->bindings = NULL;

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 14;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));

  //addInstanceMethod and addClassMethod cannot be brought into
  //the Smalltalk class because we don't have a way to 'quote' blocks

  cls_obj->class_methods->bindings[0].key = get_symbol("_createClassPreInitialize:parentClass:");
  cls_obj->class_methods->bindings[0].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)create_class),
						 NIL, NIL,
						 2, NIL, NULL);

  cls_obj->class_methods->bindings[1].key = get_symbol("_createClassPreInitialize:");
  cls_obj->class_methods->bindings[1].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)create_class_no_parent_class),
						 NIL, NIL,
						 1, NIL, NULL);

  cls_obj->class_methods->bindings[2].key = get_symbol("_addInstanceVariable:toClass:");
  cls_obj->class_methods->bindings[2].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)add_instance_var),
						 NIL, NIL,
						 2, NIL, NULL);

  cls_obj->class_methods->bindings[3].key = get_symbol("_addClassVariable:toClass:");
  cls_obj->class_methods->bindings[3].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)add_class_var),
						 NIL, NIL,
						 2, NIL, NULL);

  cls_obj->class_methods->bindings[4].key = get_symbol("_createGlobal:valued:");
  cls_obj->class_methods->bindings[4].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)create_global_valued),
						 NIL, NIL,
						 2, NIL, NULL);

  cls_obj->class_methods->bindings[5].key = get_symbol("_createGlobal:");
  cls_obj->class_methods->bindings[5].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)create_global),
						 NIL, NIL,
						 1, NIL, NULL);

  cls_obj->class_methods->bindings[6].key = get_symbol("_genSym");
  cls_obj->class_methods->bindings[6].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)smalltalk_gensym),
						 NIL, NIL,
						 0, NIL, NULL);

  cls_obj->class_methods->bindings[7].key = get_symbol("_addInstanceMethod:toClass:withBody:");
  cls_obj->class_methods->bindings[7].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)add_instance_method_str),
						 NIL, NIL,
						 3, NIL, NULL);

  cls_obj->class_methods->bindings[8].key = get_symbol("_addClassMethod:toClass:withBody:");
  cls_obj->class_methods->bindings[8].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)add_class_method_str),
						 NIL, NIL,
						 3, NIL, NULL);

  cls_obj->class_methods->bindings[9].key = get_symbol("_eval:");
  cls_obj->class_methods->bindings[9].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)smalltalk_eval),
						 NIL, NIL,
						 1, NIL, NULL);

  cls_obj->class_methods->bindings[10].key = get_symbol("_loadFile:");
  cls_obj->class_methods->bindings[10].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)smalltalk_load_file),
						 NIL, NIL,
						 1, NIL, NULL);

  cls_obj->class_methods->bindings[11].key = get_symbol("_addBreakpointTo:ofClass:");
  cls_obj->class_methods->bindings[11].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)smalltalk_add_breakpoint),
						 NIL, NIL,
						 2, NIL, NULL);

  cls_obj->class_methods->bindings[12].key = get_symbol("_removeBreakpointFrom:ofClass:");
  cls_obj->class_methods->bindings[12].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)smalltalk_remove_breakpoint),
						 NIL, NIL,
						 2, NIL, NULL);

  cls_obj->class_methods->bindings[13].key = get_symbol("_assignClass:toPackage:");
  cls_obj->class_methods->bindings[13].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)smalltalk_assign_class_to_package),
						 NIL, NIL,
						 2, NIL, NULL);

  Smalltalk =  convert_class_object_to_object_ptr(cls_obj);
}

void print_g_call_chain()
{
  call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
  unsigned int count = stack_count(g_call_chain);
  int i;

  printf("***\n");
  for(i = 0; i <count; i++)
  {
    print_object(entries[i]->selector); printf(" "); print_object(entries[i]->cont); printf("\n");
  }
  printf("---\n");
}

void print_exception_contexts()
{
  OBJECT_PTR *contexts = (OBJECT_PTR *)stack_data(g_exception_contexts);
  unsigned int count = stack_count(g_exception_contexts);
  int i;

  printf("***\n");
  for(i = 0; i <count; i++)
  {
    print_object(contexts[i]); printf("\n");
  }
  printf("---\n");
}

//utility function to convert zero-arg nativefn's to closures
OBJECT_PTR convert_fn_to_closure(nativefn fn)
{
  OBJECT_PTR lst = list(3,
			convert_native_fn_to_object(fn),
			NIL,
			convert_int_to_object(0));

  return extract_ptr(lst) + CLOSURE_TAG;
}

exception_handler_t *create_exception_handler(OBJECT_PTR protected_block,
					      OBJECT_PTR selector,
					      OBJECT_PTR exception_action,
					      stack_type *env,
					      OBJECT_PTR cont)
{
  exception_handler_t *eh = (exception_handler_t *)GC_MALLOC(sizeof(exception_handler_t));

  eh->protected_block       = protected_block;
  eh->selector              = selector;
  eh->exception_action      = exception_action;
  eh->exception_environment = env;
  eh->cont                  = cont;

  return eh;
}

OBJECT_PTR nil_print_string(OBJECT_PTR closure, OBJECT_PTR cont)
{
  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  //TODO: revisit after addding strings
  printf("nil");
  pop_if_top(entry);

  return invoke_cont_on_val(cont, NIL);
}

void create_Nil()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_nil(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("Nil");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;
  
  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 1;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_printString");
  cls_obj->instance_methods->bindings[0].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)nil_print_string),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  Nil =  convert_class_object_to_object_ptr(cls_obj);
}

void print_call_chain()
{
  call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
  int count = stack_count(g_call_chain);

  int i = count - 1;

  int j;

  printf("\nCall chain:\n");

  while(i >= 0)
  {
    call_chain_entry_t *entry = entries[i];

    printf("receiver: ");
    print_object(entry->receiver);
    printf("; ");
    printf("selector: ");
    char *str = get_symbol_name(entry->selector);
    printf("%s ", substring(str, 1, strlen(str)-1));
    printf("; ");

    if(entry->nof_args > 0)
      printf("args: [");

    for(j=0; j<entry->nof_args; j++)
    {
       print_object(entry->args[j]); printf(" ");
    }

    if(entry->nof_args > 0)
      printf("\b] ");

    method_t *m = (method_t *)extract_ptr(entry->method);

    assert(cons_length(m->temporaries) == cons_length(entry->local_vars_list));

    OBJECT_PTR rest = reverse(entry->local_vars_list);
    OBJECT_PTR rest1 = m->temporaries;

    BOOLEAN local_vars = false;

    if(rest != NIL)
    {
      printf("; local vars: [");
      local_vars = true;
    }

    while(rest != NIL)
    {
      print_object(car(rest1)); printf(": ");
      print_object(car(car(rest))); printf(" ");
      rest = cdr(rest);
      rest1 = cdr(rest1);
    }

    if(local_vars)
      printf("\b] ");

    if(entry->termination_blk_closure != NIL)
    {
      printf("["); print_object(entry->termination_blk_closure);
      printf("(invoked = %s)", entry->termination_blk_invoked == TRUE ? "yes" : "no");
      printf("]");
    }
    printf("\n");
    i--;
  }
  printf("\n");
}

OBJECT_PTR compiler_compile_pass(OBJECT_PTR closure,
				 OBJECT_PTR source_buffer,
				 OBJECT_PTR pass,
				 OBJECT_PTR cont)
{
  assert(IS_CLOSURE_OBJECT(closure));

  if(!IS_STRING_LITERAL_OBJECT(source_buffer))
    return create_and_signal_exception(InvalidArgument, cont);

  if(!IS_SMALLTALK_SYMBOL_OBJECT(pass))
    return create_and_signal_exception(InvalidArgument, cont);

  assert(IS_CLOSURE_OBJECT(cont));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  executable_code_t *prev_exp = g_exp;

  OBJECT_PTR prev_message_selectors =   g_message_selector;

  char *buf = GC_strdup(g_string_literals[source_buffer >> OBJECT_SHIFT]);

  yy_scan_string(buf);

  if(!yyparse())
  {
    OBJECT_PTR exp = convert_exec_code_to_lisp(g_exp);

    if(!strcmp(get_smalltalk_symbol_name(pass), "decorateMessageSelectors"))
      exp = decorate_message_selectors(exp);
    else if(!strcmp(get_smalltalk_symbol_name(pass), "expandBodies"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);
      exp = expand_bodies(exp);
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "convertMessageSends"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);
      exp = convert_message_sends(
              expand_bodies(exp));
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "assignmentConversion"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);

      OBJECT_PTR temp = convert_message_sends(
                          expand_bodies(exp));

      exp = assignment_conversion(temp,
				  concat(2,
					 get_top_level_symbols(),
					 get_free_variables(temp)));
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "translateToIL"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);

      OBJECT_PTR temp = convert_message_sends(
                          expand_bodies(exp));

      exp = assignment_conversion(temp,
				  concat(2,
					 get_top_level_symbols(),
					 get_free_variables(temp)));
      exp = translate_to_il(exp);
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "desugarIL"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);

      OBJECT_PTR temp = convert_message_sends(
                          expand_bodies(exp));

      exp = assignment_conversion(temp,
				  concat(2,
					 get_top_level_symbols(),
					 get_free_variables(temp)));
      exp = desugar_il(
	      translate_to_il(exp));
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "renamingTransform"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);

      OBJECT_PTR temp = convert_message_sends(
                          expand_bodies(exp));

      exp = assignment_conversion(temp,
				  concat(2,
					 get_top_level_symbols(),
					 get_free_variables(temp)));
      exp = ren_transform(desugar_il(
	                    translate_to_il(exp)),
			  create_binding_env());
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "simplifyIL"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);

      OBJECT_PTR temp = convert_message_sends(
                          expand_bodies(exp));

      exp = assignment_conversion(temp,
				  concat(2,
					 get_top_level_symbols(),
					 get_free_variables(temp)));
      exp = ren_transform(desugar_il(
	                    translate_to_il(exp)),
			  create_binding_env());

      exp = simplify_il(exp);
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "cpsTransform"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);

      OBJECT_PTR temp = convert_message_sends(
                          expand_bodies(exp));

      exp = assignment_conversion(temp,
				  concat(2,
					 get_top_level_symbols(),
					 get_free_variables(temp)));
      exp = ren_transform(desugar_il(
	                    translate_to_il(exp)),
			  create_binding_env());

      exp = mcps_transform(
	      simplify_il(exp));
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "closureConvTransform"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);

      OBJECT_PTR temp = convert_message_sends(
                          expand_bodies(exp));

      exp = assignment_conversion(temp,
				  concat(2,
					 get_top_level_symbols(),
					 get_free_variables(temp)));
      exp = ren_transform(desugar_il(
	                    translate_to_il(exp)),
			  create_binding_env());

      exp = closure_conv_transform(
	      mcps_transform(
	        simplify_il(exp)));
    }
    else if(!strcmp(get_smalltalk_symbol_name(pass), "liftTransform"))
    {
      exp = decorate_message_selectors(exp);
      g_message_selector = build_selectors_list(exp);

      OBJECT_PTR temp = convert_message_sends(
                          expand_bodies(exp));

      exp = assignment_conversion(temp,
				  concat(2,
					 get_top_level_symbols(),
					 get_free_variables(temp)));
      exp = ren_transform(desugar_il(
	                    translate_to_il(exp)),
			  create_binding_env());

      exp = closure_conv_transform(
	      mcps_transform(
	        simplify_il(exp)));

      exp = lift_transform(exp, NIL);
    }
    else
      create_and_signal_exception(InvalidArgument, cont);

    g_exp = prev_exp;
    g_message_selector = prev_message_selectors;

    pop_if_top(entry);

    return invoke_cont_on_val(cont, exp);
  }
  else
  {
    g_exp = prev_exp;
    g_message_selector = prev_message_selectors;
    return create_and_signal_exception(CompileError, cont);
  }
}

//Compiler>>compile
OBJECT_PTR compiler_compile(OBJECT_PTR closure,
			    OBJECT_PTR source_buffer,
			    OBJECT_PTR cont)
{
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_STRING_LITERAL_OBJECT(source_buffer));
  assert(IS_CLOSURE_OBJECT(cont));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  executable_code_t *prev_exp = g_exp;

  char *buf = GC_strdup(g_string_literals[source_buffer >> OBJECT_SHIFT]);

  yy_scan_string(buf);

  if(!yyparse())
  {
    OBJECT_PTR exp = convert_exec_code_to_lisp(g_exp);

    exp = decorate_message_selectors(exp);

    OBJECT_PTR res = apply_lisp_transforms(exp);

    g_exp = prev_exp;

    pop_if_top(entry);

    return invoke_cont_on_val(cont, res);
  }
  else
  {
    g_exp = prev_exp;
    return create_and_signal_exception(CompileError, cont);
  }
}

void create_Compiler()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Compiler(): Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("Compiler");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;

  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;

  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 0;
  cls_obj->instance_methods->bindings = NULL;

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 2;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));

  cls_obj->class_methods->bindings[0].key = get_symbol("_compile:");
  cls_obj->class_methods->bindings[0].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)compiler_compile),
						 NIL, NIL,
						 1, NIL, NULL);

  cls_obj->class_methods->bindings[1].key = get_symbol("_compile:pass:");
  cls_obj->class_methods->bindings[1].val = create_method(cls_obj, true,
						 convert_native_fn_to_object((nativefn)compiler_compile_pass),
						 NIL, NIL,
						 2, NIL, NULL);

  Compiler =  convert_class_object_to_object_ptr(cls_obj);
}
