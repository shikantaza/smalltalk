#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "smalltalk.h"
#include "util.h"

//workaround for variadic function arguments
//getting clobbered in ARM64
typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

binding_env_t *top_level;

OBJECT_PTR create_message_send_closure();
char *get_symbol_name(OBJECT_PTR);
OBJECT_PTR get_symbol(char *);
OBJECT_PTR convert_class_object_to_object_ptr(class_object_t *);

BOOLEAN IS_CLASS_OBJECT(OBJECT_PTR);
BOOLEAN IS_SMALLTALK_SYMBOL_OBJECT(OBJECT_PTR);

OBJECT_PTR apply_lisp_transforms(OBJECT_PTR);
void *compile_to_c(OBJECT_PTR);
nativefn get_function(void *, const char *);
char *extract_variable_string(OBJECT_PTR, BOOLEAN);
OBJECT_PTR convert_native_fn_to_object(nativefn);

OBJECT_PTR create_closure(OBJECT_PTR, OBJECT_PTR, nativefn);
nativefn extract_native_fn(OBJECT_PTR);
OBJECT_PTR convert_int_to_object(int);
OBJECT_PTR identity_function(OBJECT_PTR, ...);

OBJECT_PTR new_object(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR get_binding_val(binding_env_t *, OBJECT_PTR);

char *get_smalltalk_symbol_name(OBJECT_PTR);
OBJECT_PTR get_smalltalk_symbol(char *);

OBJECT_PTR extract_arity(OBJECT_PTR);

OBJECT_PTR Object;
OBJECT_PTR Smalltalk;

char **string_literals = NULL;
unsigned int nof_string_literals = 0;

extern OBJECT_PTR NIL;
extern OBJECT_PTR MESSAGE_SEND;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Integer;
extern OBJECT_PTR Transcript;
extern OBJECT_PTR NiladicBlock;
extern OBJECT_PTR Boolean;

extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

void add_binding_to_top_level(OBJECT_PTR sym, OBJECT_PTR val)
{
  top_level->count++;

  top_level->bindings = (binding_t *)GC_REALLOC(top_level->bindings,
                                                top_level->count * sizeof(binding_t));

  top_level->bindings[top_level->count - 1].key = sym;
  top_level->bindings[top_level->count - 1].val = val;
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
  return get_binding_val_regular(top_level, sym, ret);
}

void initialize_top_level()
{
  top_level = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  top_level->count = 0;

  add_binding_to_top_level(MESSAGE_SEND, cons(create_message_send_closure(), NIL));
  add_binding_to_top_level(get_symbol("Integer"), Integer); //TODO: shouldn't this be cons(Integer, NIL)?
  add_binding_to_top_level(SELF, cons(NIL, NIL));
  add_binding_to_top_level(get_symbol("Object"), cons(Object, NIL));
  add_binding_to_top_level(get_symbol("Smalltalk"), cons(Smalltalk, NIL));
  add_binding_to_top_level(get_symbol("Transcript"), cons(Transcript, NIL));
  add_binding_to_top_level(get_symbol("NiladicBlock"), cons(NiladicBlock, NIL));
  add_binding_to_top_level(get_symbol("Boolean"), cons(Boolean, NIL));

  add_binding_to_top_level(get_symbol("true"), cons(TRUE, NIL));
  add_binding_to_top_level(get_symbol("false"), cons(FALSE, NIL));
}

BOOLEAN exists_in_top_level(OBJECT_PTR sym)
{
  assert(IS_SYMBOL_OBJECT(sym));

  unsigned int i, n = top_level->count;

  for(i=0; i<n; i++)
    if(top_level->bindings[i].key == sym)
      return true;

  return false;
}

OBJECT_PTR create_class(OBJECT_PTR closure,
			OBJECT_PTR class_sym,
			OBJECT_PTR parent_class_sym,
			OBJECT_PTR cont)
{
  //TODO: what to do if the class to be created
  //already exists?

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(class_sym));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(parent_class_sym));
  assert(IS_CLOSURE_OBJECT(cont));

  /*
  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(parent_class);
  
  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_parent_class_name = substring(s2, 1, strlen(s2)-1);
  */
  
#ifdef DEBUG  
  /* printf("%s %s\n", stripped_class_name, stripped_parent_class_name); */
#endif
  
  //assert(exists_in_top_level(get_symbol(stripped_parent_class_name)));
  assert(exists_in_top_level(get_symbol(get_smalltalk_symbol_name(parent_class_sym))));

  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_class(): Unable to allocate memory\n");
    exit(1);
  }

  OBJECT_PTR parent_class_object;
  //assert(get_top_level_val(get_symbol(stripped_parent_class_name), &parent_class_object));
  assert(get_top_level_val(get_symbol(get_smalltalk_symbol_name(parent_class_sym)), &parent_class_object));
  
  cls_obj->parent_class_object = car(parent_class_object);
  //cls_obj->name = GC_strdup(stripped_class_name);
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
  
  cls_obj->class_methods->bindings[0].key = get_symbol("new");
  cls_obj->class_methods->bindings[0].val = list(3,
						 convert_native_fn_to_object((nativefn)new_object),
						 NIL,
						 convert_int_to_object(0));

  OBJECT_PTR class_object = convert_class_object_to_object_ptr(cls_obj);
  
  add_binding_to_top_level(get_symbol(get_smalltalk_symbol_name(class_sym)), cons(class_object, NIL));

  nativefn nf = (nativefn)extract_native_fn(cont);
  
  return nf(cont, class_object);
}

OBJECT_PTR create_class_no_parent_class(OBJECT_PTR closure,
					OBJECT_PTR class_sym,
					OBJECT_PTR cont)
{
  return create_class(closure, class_sym, get_smalltalk_symbol("Object"), cont);
}

OBJECT_PTR add_instance_var(OBJECT_PTR closure,
			    OBJECT_PTR var,
			    OBJECT_PTR class_sym,
			    OBJECT_PTR cont)
{
  //TODO: add a default value for the variable?

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(class_sym));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(var));
  assert(IS_CLOSURE_OBJECT(cont));

  /*
  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(var);

  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_instance_var_name = substring(s2, 1, strlen(s2)-1);
  */
  
#ifdef DEBUG  
  printf("%s %s\n", get_smalltalk_symbol_name(class_sym), get_smalltalk_symbol_name(var)); 
#endif
  
  //OBJECT_PTR var_sym = get_symbol(stripped_instance_var_name);
  OBJECT_PTR var_sym = get_symbol(get_smalltalk_symbol_name(var));
  
  //assert(exists_in_top_level(get_symbol(stripped_class_name)));
  assert(exists_in_top_level(get_symbol(get_smalltalk_symbol_name(class_sym))));

  OBJECT_PTR class_object_val, class_object;

  //assert(get_top_level_val(get_symbol(stripped_class_name), &class_object_val));
  assert(get_top_level_val(get_symbol(get_smalltalk_symbol_name(class_sym)), &class_object_val));
    
  class_object = car(class_object_val);
  assert(IS_CLASS_OBJECT(class_object));

  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_object);

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
    cls_obj->inst_vars = (OBJECT_PTR *)GC_REALLOC(cls_obj->inst_vars,
                                                  cls_obj->nof_instance_vars * sizeof(OBJECT_PTR));

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
    inst->instance_vars->bindings[inst->instance_vars->count].val = NIL;
  }

  nativefn nf = (nativefn)extract_native_fn(cont);
  
  return nf(cont, NIL);
}

OBJECT_PTR add_class_var(OBJECT_PTR closure,
			 OBJECT_PTR var,
			 OBJECT_PTR class_sym,
			 OBJECT_PTR cont)
{
  //TODO: add a default value for the variable?

  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(class_sym));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(var));
  assert(IS_CLOSURE_OBJECT(cont));

  /*
  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(var);
  
  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_class_var_name = substring(s2, 1, strlen(s2)-1);
  */
  
#ifdef DEBUG  
  printf("%s %s\n", get_smalltalk_symbol_name(class_sym), get_smalltalk_symbol_name(var));
#endif
  
  //assert(exists_in_top_level(get_symbol(stripped_class_name)));
  assert(exists_in_top_level(get_symbol(get_smalltalk_symbol_name(class_sym))));

  //OBJECT_PTR var_sym = get_symbol(stripped_class_var_name);
  OBJECT_PTR var_sym = get_symbol(get_smalltalk_symbol_name(var));
  
  OBJECT_PTR class_object_val, class_object;

  //assert(get_top_level_val(get_symbol(stripped_class_name), &class_object_val));
  assert(get_top_level_val(get_symbol(get_smalltalk_symbol_name(class_sym)), &class_object_val));

  class_object = car(class_object_val);
  
  assert(IS_CLASS_OBJECT(class_object));

  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_object);

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
    cls_obj->shared_vars->count++;
  
  cls_obj->shared_vars->bindings[cls_obj->shared_vars->count-1].key = var_sym;
  cls_obj->shared_vars->bindings[cls_obj->shared_vars->count-1].val = cons(NIL, NIL);

  nativefn nf = (nativefn)extract_native_fn(cont);
  
  return nf(cont, NIL);
}

void add_instance_method(OBJECT_PTR class_sym, OBJECT_PTR selector, OBJECT_PTR code)
{
  assert(IS_SMALLTALK_SYMBOL_OBJECT(class_sym));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(selector));

  assert(IS_CONS_OBJECT(code)); //TODO: maybe some stronger checks?

  /*
  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(selector);
  
  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_selector_name = substring(s2, 1, strlen(s2)-1);
  */
  
  //assert(exists_in_top_level(get_symbol(stripped_class_name)));
  assert(exists_in_top_level(get_symbol(get_smalltalk_symbol_name(class_sym))));

  //OBJECT_PTR selector_sym = get_symbol(stripped_selector_name);
  OBJECT_PTR selector_sym = get_symbol(get_smalltalk_symbol_name(selector));
  
  OBJECT_PTR res = apply_lisp_transforms(code);

  void *state = compile_to_c(res);

  char *fname = extract_variable_string(fourth(first(res)), true);

  nativefn nf = get_function(state, fname);

  assert(nf);
  
  OBJECT_PTR closed_vals = cdr(CDDDR(first(res)));

  //closed vals are only the variable names
  //and don't have any significance here as 
  //they will be deferenced only when the method
  //is invoked, at which time the correct values
  //would have been populated
  OBJECT_PTR lst_form = list(3, convert_native_fn_to_object(nf), closed_vals, second(first(res)));
  OBJECT_PTR closure_form = extract_ptr(lst_form) + CLOSURE_TAG;

  OBJECT_PTR idclo = create_closure(convert_int_to_object(1),
                                    convert_int_to_object(0),
                                    (nativefn)identity_function);

  assert(IS_CLOSURE_OBJECT(idclo));
  
  nativefn1 nf1 = (nativefn1)nf;
    
  OBJECT_PTR result_closure = nf1(closure_form, idclo);

  assert(IS_CLOSURE_OBJECT(result_closure));
  
  OBJECT_PTR nfo = convert_native_fn_to_object(extract_native_fn(result_closure));

#ifdef DEBUG  
  print_object(closed_vals); printf("\n");
#endif
  
  OBJECT_PTR class_object_val, class_object;

  //assert(get_top_level_val(get_symbol(stripped_class_name), &class_object_val));
  assert(get_top_level_val(get_symbol(get_smalltalk_symbol_name(class_sym)), &class_object_val));

  class_object = car(class_object_val);
  
  assert(IS_CLASS_OBJECT(class_object));

  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_object);
  
  unsigned int i, n;
  n = cls_obj->instance_methods->count;

  BOOLEAN existing_method = false;
  
  for(i=0; i<n; i++)
    if(cls_obj->instance_methods->bindings[i].key == selector_sym)
    {
      existing_method = true;
      cls_obj->instance_methods->bindings[i].val = list(3,
							nfo,
							closed_vals,
							second(first(res)));
      //list(1, convert_int_to_object(cons_length(second(third(code))))));
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
      cls_obj->instance_methods->bindings = (binding_t *)GC_REALLOC(cls_obj->instance_methods->bindings,
								    cls_obj->instance_methods->count * sizeof(binding_t));
    }
    
    cls_obj->instance_methods->bindings[cls_obj->instance_methods->count - 1].key = selector_sym;
    cls_obj->instance_methods->bindings[cls_obj->instance_methods->count - 1].val =
      list(3,
	   nfo,
	   closed_vals,
	   second(first(res)));
    //list(1, convert_int_to_object(cons_length(second(third(code))))));
  }
}

void add_class_method(OBJECT_PTR class_sym, OBJECT_PTR selector, OBJECT_PTR code)
{
  assert(IS_SMALLTALK_SYMBOL_OBJECT(class_sym));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(selector));

  assert(IS_CONS_OBJECT(code)); //TODO: maybe some stronger checks?

  /*
  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(selector);
  
  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_selector_name = substring(s2, 1, strlen(s2)-1);
  */
  
  //assert(exists_in_top_level(get_symbol(stripped_class_name)));
  assert(exists_in_top_level(get_symbol(get_smalltalk_symbol_name(class_sym))));

  //OBJECT_PTR selector_sym = get_symbol(stripped_selector_name);
  OBJECT_PTR selector_sym = get_symbol(get_smalltalk_symbol_name(selector));

#ifdef DEBUG
  print_object(code); printf(" is the code for the method\n");
#endif
    
  OBJECT_PTR res = apply_lisp_transforms(code);

#ifdef DEBUG  
  print_object(res); printf("\n");
#endif
  
  void *state = compile_to_c(res);

  char *fname = extract_variable_string(fourth(first(res)), true);

  nativefn nf = get_function(state, fname);

  assert(nf);

  OBJECT_PTR closed_vals = cdr(CDDDR(first(res)));

  //closed vals are only the variable names
  //and don't have any significance here as 
  //they will be deferenced only when the method
  //is invoked, at which time the correct values
  //would have been populated
  OBJECT_PTR lst_form = list(3, convert_native_fn_to_object(nf), closed_vals, second(first(res)));
  OBJECT_PTR closure_form = extract_ptr(lst_form) + CLOSURE_TAG;

  OBJECT_PTR idclo = create_closure(convert_int_to_object(1),
                                    convert_int_to_object(0),
                                    (nativefn)identity_function);

  assert(IS_CLOSURE_OBJECT(idclo));
  
  nativefn1 nf1 = (nativefn1)nf;
    
  OBJECT_PTR result_closure = nf1(closure_form, idclo);

  assert(IS_CLOSURE_OBJECT(result_closure));
  
  OBJECT_PTR nfo = convert_native_fn_to_object(extract_native_fn(result_closure));

#ifdef DEBUG  
  print_object(closed_vals); printf("\n");
#endif
  
  OBJECT_PTR class_object_val, class_object;

  //assert(get_top_level_val(get_symbol(stripped_class_name), &class_object_val));
  assert(get_top_level_val(get_symbol(get_smalltalk_symbol_name(class_sym)), &class_object_val));

  class_object = car(class_object_val);
  
  assert(IS_CLASS_OBJECT(class_object));

  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_object);
  
  unsigned int i, n;
  n = cls_obj->class_methods->count;

  BOOLEAN existing_method = false;
  
  for(i=0; i<n; i++)
    if(cls_obj->class_methods->bindings[i].key == selector_sym)
    {
      existing_method = true;
      cls_obj->class_methods->bindings[i].val = list(3,
						     nfo,
						     closed_vals,
						     second(first(res)));
      //list(1, convert_int_to_object(cons_length(second(third(code))))));
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
      cls_obj->class_methods->bindings = (binding_t *)GC_REALLOC(cls_obj->class_methods->bindings,
								 cls_obj->class_methods->count * sizeof(binding_t));
    }

    cls_obj->class_methods->bindings[cls_obj->class_methods->count - 1].key = selector_sym;
    cls_obj->class_methods->bindings[cls_obj->class_methods->count - 1].val =
      list(3,
	   nfo,
	   closed_vals,
	   second(first(res)));
    //list(1, convert_int_to_object(cons_length(second(third(code))))));
  }
}

OBJECT_PTR new_object(OBJECT_PTR closure,
		      OBJECT_PTR cont)
{
#ifdef DEBUG
  printf("Entering new_object()\n");
#endif

  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));
  
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_CLASS_OBJECT(receiver));
  assert(IS_CLOSURE_OBJECT(cont));

  object_t *obj = (object_t *)GC_MALLOC(sizeof(object_t));
  
  obj->class_object = receiver;
  obj->instance_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));

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
	obj->instance_vars->bindings = (binding_t *)GC_REALLOC(obj->instance_vars->bindings,
							       obj->instance_vars->count * sizeof(binding_t));
    
      for(i=prev_count; i<prev_count + n; i++)
      {
	obj->instance_vars->bindings[i].key = curr_cls_obj->inst_vars[prev_count - i];
	obj->instance_vars->bindings[i].val = cons(NIL, NIL);
      }
    }

    //TODO: call 'initialize' method for parent_class if present
    //(defined by the user)
    
    current_parent = curr_cls_obj->parent_class_object;
  }

  //add the new instance to the instances mapped to the class
  cls_obj->nof_instances++;

  if(!cls_obj->instances)
    cls_obj->instances = (OBJECT_PTR *)GC_MALLOC(cls_obj->nof_instances * sizeof(OBJECT_PTR));
  else
    cls_obj->instances = (OBJECT_PTR *)GC_REALLOC(cls_obj->instances, cls_obj->nof_instances * sizeof(OBJECT_PTR));

  cls_obj->instances[cls_obj->nof_instances-1] = (uintptr_t)obj + OBJECT_TAG;
  
  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  OBJECT_PTR ret = nf(cont, (uintptr_t)obj + OBJECT_TAG);
  
#ifdef DEBUG
  printf("Exiting new_object()\n");
#endif

  return ret;
}
  
void create_Object()
{
  class_object_t *cls_obj = (class_object_t *)GC_MALLOC(sizeof(class_object_t));
  
  cls_obj->parent_class_object = NIL;
  cls_obj->name = GC_strdup("Object");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;
  
  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;
  
  cls_obj->shared_vars = NULL;
  cls_obj->instance_methods = NULL;
  cls_obj->class_methods = NULL;

  Object = (uintptr_t)cls_obj + CLASS_OBJECT_TAG;
}

OBJECT_PTR create_global(OBJECT_PTR closure,
			 OBJECT_PTR global_sym,
			 OBJECT_PTR global_val,
			 OBJECT_PTR cont)
{
  assert(IS_CLOSURE_OBJECT(closure));
  assert(IS_SMALLTALK_SYMBOL_OBJECT(global_sym));
  assert(IS_CLOSURE_OBJECT(cont));
  
  add_binding_to_top_level(get_symbol(get_smalltalk_symbol_name(global_sym)), cons(global_val, NIL));

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  OBJECT_PTR ret = nf(cont, global_val);

  return ret;
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
  cls_obj->class_methods->count = 5;
  cls_obj->class_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->class_methods->count * sizeof(binding_t));

  //addInstanceMethod and addClassMethod cannot be brought into
  //the Smalltalk class because we don't have a way to 'quote' blocks
  
  cls_obj->class_methods->bindings[0].key = get_symbol("createClass:parentClass:");
  cls_obj->class_methods->bindings[0].val = list(3,
						 convert_native_fn_to_object((nativefn)create_class),
						 NIL,
						 convert_int_to_object(2));

  cls_obj->class_methods->bindings[1].key = get_symbol("createClass:");
  cls_obj->class_methods->bindings[1].val = list(3,
						 convert_native_fn_to_object((nativefn)create_class_no_parent_class),
						 NIL,
						 convert_int_to_object(1));
  
  cls_obj->class_methods->bindings[2].key = get_symbol("addInstanceVariable:toClass:");
  cls_obj->class_methods->bindings[2].val = list(3,
						 convert_native_fn_to_object((nativefn)add_instance_var),
						 NIL,
						 convert_int_to_object(2));

  cls_obj->class_methods->bindings[3].key = get_symbol("addClassVariable:toClass:");
  cls_obj->class_methods->bindings[3].val = list(3,
						 convert_native_fn_to_object((nativefn)add_class_var),
						 NIL,
						 convert_int_to_object(2));

  cls_obj->class_methods->bindings[4].key = get_symbol("createGlobal:valued:");
  cls_obj->class_methods->bindings[4].val = list(3,
						 convert_native_fn_to_object((nativefn)create_global),
						 NIL,
						 convert_int_to_object(2));
  
  Smalltalk =  convert_class_object_to_object_ptr(cls_obj);
}
