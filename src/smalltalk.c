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

OBJECT_PTR apply_lisp_transforms(OBJECT_PTR);
void *compile_to_c(OBJECT_PTR);
nativefn get_function(void *, const char *);
char *extract_variable_string(OBJECT_PTR, BOOLEAN);
OBJECT_PTR convert_native_fn_to_object(nativefn);

OBJECT_PTR create_closure(OBJECT_PTR, nativefn);
nativefn extract_native_fn(OBJECT_PTR);
OBJECT_PTR convert_int_to_object(int);
OBJECT_PTR identity_function(OBJECT_PTR, ...);

OBJECT_PTR new_object(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR get_binding_val(binding_env_t *, OBJECT_PTR);

extern OBJECT_PTR NIL;
extern OBJECT_PTR MESSAGE_SEND;
extern OBJECT_PTR SELF;
extern OBJECT_PTR Integer;

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

OBJECT_PTR create_class(OBJECT_PTR class, OBJECT_PTR parent_class)
{
  //TODO: what to do if the class to be created
  //already exists?
  
  assert(IS_SYMBOL_OBJECT(class));
  assert(IS_SYMBOL_OBJECT(parent_class));

  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(parent_class);
  
  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_parent_class_name = substring(s2, 1, strlen(s2)-1);

#ifdef DEBUG  
  printf("%s %s\n", stripped_class_name, stripped_parent_class_name);
#endif
  
  assert(exists_in_top_level(get_symbol(stripped_parent_class_name)));

  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_Integer: Unable to allocate memory\n");
    exit(1);
  }

  //TODO: initialize cls_obj->parent_class_object to Object
  //once we define Object
  cls_obj->name = GC_strdup(stripped_class_name);

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
  cls_obj->class_methods->bindings[0].val = cons(convert_native_fn_to_object((nativefn)new_object), NIL);

  OBJECT_PTR class_object = convert_class_object_to_object_ptr(cls_obj);
  
  add_binding_to_top_level(get_symbol(stripped_class_name), cons(class_object, NIL));
  
  return class_object;
}

void add_instance_var(OBJECT_PTR class, OBJECT_PTR var)
{
  //TODO: add a default value for the variable?
  
  assert(IS_SYMBOL_OBJECT(class));
  assert(IS_SYMBOL_OBJECT(var));

  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(var);

  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_instance_var_name = substring(s2, 1, strlen(s2)-1);

#ifdef DEBUG  
  printf("%s %s\n", stripped_class_name, stripped_instance_var_name);
#endif
  
  OBJECT_PTR var_sym = get_symbol(stripped_instance_var_name);
  
  assert(exists_in_top_level(get_symbol(stripped_class_name)));

  OBJECT_PTR class_object_val, class_object;

  assert(get_top_level_val(get_symbol(stripped_class_name), &class_object_val));
  class_object = car(class_object_val);
  assert(IS_CLASS_OBJECT(class_object));

  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_object);

  unsigned int i, n;

  //check that the instance variable doesn't already exist
  n = cls_obj->nof_instance_vars;
  for(i=0; i<n; i++)
    if(cls_obj->inst_vars[i] == var_sym)
    {
      printf("Instance variable %s already exists\n", stripped_instance_var_name);
      return;
    }

  //check that the instance variable doesn't conflict with an existing class variable
  n = cls_obj->shared_vars->count;
  for(i=0; i<n; i++)
    if(cls_obj->shared_vars->bindings[i].key == var_sym)
    {
      printf("Instance variable %s conflicts with an existing class variable\n", stripped_instance_var_name);
      return;
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
}

void add_class_var(OBJECT_PTR class, OBJECT_PTR var)
{
  //TODO: add a default value for the variable?
  
  assert(IS_SYMBOL_OBJECT(class));
  assert(IS_SYMBOL_OBJECT(var));

  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(var);
  
  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_class_var_name = substring(s2, 1, strlen(s2)-1);

#ifdef DEBUG  
  printf("%s %s\n", stripped_class_name, stripped_class_var_name);
#endif
  
  assert(exists_in_top_level(get_symbol(stripped_class_name)));

  OBJECT_PTR var_sym = get_symbol(stripped_class_var_name);
  
  OBJECT_PTR class_object_val, class_object;

  assert(get_top_level_val(get_symbol(stripped_class_name), &class_object_val));

  class_object = car(class_object_val);
  
  assert(IS_CLASS_OBJECT(class_object));

  class_object_t *cls_obj = (class_object_t *)extract_ptr(class_object);

  //check that the class variable doesn't already exist
  unsigned int i, n;
  n = cls_obj->shared_vars->count;
  for(i=0; i<n; i++)
    if(cls_obj->shared_vars->bindings[i].key == var_sym)
    {
      printf("Class variable %s already exists\n", stripped_class_var_name);
      return;
    }
  //check that the class variable doesn't conflict with an existing instance variable
  n = cls_obj->nof_instance_vars;
  for(i=0; i<n; i++)
    if(cls_obj->inst_vars[i] == var_sym)
    {
      printf("Class variable %s conflicts with an existing instance variable\n", stripped_class_var_name);
      return;
    }
  
  if(!cls_obj->shared_vars->bindings)
  {
    cls_obj->shared_vars->bindings = (binding_t *)GC_MALLOC(sizeof(binding_t));
    cls_obj->shared_vars->count = 1;
  }
  else
    cls_obj->shared_vars->count++;
  
  cls_obj->shared_vars->bindings[cls_obj->shared_vars->count-1].key = var_sym;
  cls_obj->shared_vars->bindings[cls_obj->shared_vars->count-1].val = NIL;
  
}

void add_instance_method(OBJECT_PTR class, OBJECT_PTR selector, OBJECT_PTR code)
{
  assert(IS_SYMBOL_OBJECT(class));
  assert(IS_SYMBOL_OBJECT(selector));

  assert(IS_CONS_OBJECT(code)); //TODO: maybe some stronger checks?

  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(selector);
  
  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_selector_name = substring(s2, 1, strlen(s2)-1);

  assert(exists_in_top_level(get_symbol(stripped_class_name)));

  OBJECT_PTR selector_sym = get_symbol(stripped_selector_name);
  
  OBJECT_PTR res = apply_lisp_transforms(code);

  void *state = compile_to_c(res);

  char *fname = extract_variable_string(third(first(res)), true);

  nativefn nf = get_function(state, fname);

  assert(nf);
  
  OBJECT_PTR closed_vals = CDDDR(first(res));

  //closed vals are only the variable names
  //and don't have any significance here as 
  //they will be deferenced only when the method
  //is invoked, at which time the correct values
  //would have been populated
  OBJECT_PTR lst_form = concat(2, list(1, convert_native_fn_to_object(nf)), closed_vals);
  OBJECT_PTR closure_form = extract_ptr(lst_form) + CLOSURE_TAG;

  OBJECT_PTR idclo = create_closure(convert_int_to_object(0),
                                    (nativefn)identity_function);

  assert(IS_CLOSURE_OBJECT(idclo));
  
  nativefn1 nf1 = (nativefn1)nf;
    
  OBJECT_PTR result_closure = nf1(closure_form, idclo);

  assert(IS_CLOSURE_OBJECT(result_closure));
  
  OBJECT_PTR nfo = convert_native_fn_to_object(extract_native_fn(result_closure));

  print_object(closed_vals); printf("\n");

  OBJECT_PTR class_object_val, class_object;

  assert(get_top_level_val(get_symbol(stripped_class_name), &class_object_val));

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
      cls_obj->instance_methods->bindings[i].val = concat(2, list(1, nfo), closed_vals);
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
    cls_obj->instance_methods->bindings[cls_obj->instance_methods->count - 1].val = concat(2, list(1, nfo), closed_vals);;
  }
}

void add_class_method(OBJECT_PTR class, OBJECT_PTR selector, OBJECT_PTR code)
{
  assert(IS_SYMBOL_OBJECT(class));
  assert(IS_SYMBOL_OBJECT(selector));

  assert(IS_CONS_OBJECT(code)); //TODO: maybe some stronger checks?

  char *s1 = get_symbol_name(class);
  char *s2 = get_symbol_name(selector);
  
  char *stripped_class_name = substring(s1, 1, strlen(s1)-1);
  char *stripped_selector_name = substring(s2, 1, strlen(s2)-1);

  assert(exists_in_top_level(get_symbol(stripped_class_name)));

  OBJECT_PTR selector_sym = get_symbol(stripped_selector_name);

#ifdef DEBUG
  print_object(code); printf(" is the code for the method\n");
#endif
    
  OBJECT_PTR res = apply_lisp_transforms(code);

#ifdef DEBUG  
  print_object(res); printf("\n");
#endif
  
  void *state = compile_to_c(res);

  char *fname = extract_variable_string(third(first(res)), true);

  nativefn nf = get_function(state, fname);

  assert(nf);

  OBJECT_PTR closed_vals = CDDDR(first(res));

  //closed vals are only the variable names
  //and don't have any significance here as 
  //they will be deferenced only when the method
  //is invoked, at which time the correct values
  //would have been populated
  OBJECT_PTR lst_form = concat(2, list(1, convert_native_fn_to_object(nf)), closed_vals);
  OBJECT_PTR closure_form = extract_ptr(lst_form) + CLOSURE_TAG;

  OBJECT_PTR idclo = create_closure(convert_int_to_object(0),
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

  assert(get_top_level_val(get_symbol(stripped_class_name), &class_object_val));

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
      cls_obj->class_methods->bindings[i].val = concat(2, list(1, nfo), closed_vals);
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
    cls_obj->class_methods->bindings[cls_obj->class_methods->count - 1].val = concat(2, list(1, nfo), closed_vals);;
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

  class_object_t *cls_obj = (class_object_t *)extract_ptr(receiver);

  unsigned int n = cls_obj->nof_instance_vars;
  unsigned int i;

  obj->class_object = receiver;
  obj->instance_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));

  if(n > 0)
  {
    obj->instance_vars->count = n;
    obj->instance_vars->bindings = (binding_t *)GC_MALLOC(obj->instance_vars->count * sizeof(binding_t));

    for(i=0; i<n; i++)
    {
      obj->instance_vars->bindings[i].key = cls_obj->inst_vars[i];
      obj->instance_vars->bindings[i].val = cons(convert_int_to_object(42), NIL); //just for testing, revert to NIL
    }
  }

  //TODO: call 'initialize' method if present
  //(defined by the user)
  
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
  
