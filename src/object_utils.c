#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "global_decls.h"

int gen_sym_count = 0;
OBJECT_PTR Symbol;

extern OBJECT_PTR NIL;
extern OBJECT_PTR LET;
extern OBJECT_PTR SET;
extern OBJECT_PTR RETURN;
extern OBJECT_PTR MESSAGE_SEND;
extern OBJECT_PTR LAMBDA;
extern package_t *g_compiler_package;
extern OBJECT_PTR Integer;
extern OBJECT_PTR NiladicBlock;
extern OBJECT_PTR MonadicBlock;

extern OBJECT_PTR Nil;
extern OBJECT_PTR Boolean;

extern char **g_string_literals;
extern unsigned int g_nof_string_literals;

extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

extern OBJECT_PTR Object;

extern OBJECT_PTR g_idclo;

extern stack_type *g_call_chain;

extern OBJECT_PTR Array;

extern OBJECT_PTR INITIALIZE_SELECTOR;

extern binding_env_t *g_top_level;

extern OBJECT_PTR SELF;
extern OBJECT_PTR SUPER;

uintptr_t extract_ptr(OBJECT_PTR obj)
{
  return (obj >> OBJECT_SHIFT) << OBJECT_SHIFT;
}

void set_heap(uintptr_t ptr, unsigned int index, OBJECT_PTR val)
{
  uintptr_t *ptr1 = (uintptr_t *)ptr;
  *(ptr1 + index) = val;
}

uintptr_t object_alloc(int size, int tag)
{
  uintptr_t *ret;

  int err;

  err = GC_posix_memalign((void **)&ret, 16, size * sizeof(OBJECT_PTR));

  if(err)
  {
    error("Unable to allocate memory\n");
    exit(1);
  }
  
  return (uintptr_t)ret;
}

//wrapper to allocate memory for internal objects like class_object_t
int allocate_memory(void **ptr, size_t size)
{
  return GC_posix_memalign(ptr, 16, size);
}

int extract_symbol_index(OBJECT_PTR symbol_obj)
{
  return (symbol_obj >> OBJECT_SHIFT) & TWO_RAISED_TO_SYMBOL_BITS_MINUS_1;
}

char *get_symbol_name(OBJECT_PTR symbol_object)
{
  int symbol_index;

  if(!IS_SYMBOL_OBJECT(symbol_object))
  {
    error("get_symbol_name() passed a non-symbol object\n");
    exit(1);
  }

  symbol_index = extract_symbol_index(symbol_object);
  
  return g_compiler_package->symbols[symbol_index];
}

OBJECT_PTR convert_int_to_object(int i)
{
  long l = i; // i may not be necessary

  uintptr_t ptr;

  ptr = (l << 32) + INTEGER_TAG;

  return ptr;
}

int get_int_value(OBJECT_PTR int_object)
{
  return int_object >> 32;
}

OBJECT_PTR convert_char_to_object(char c)
{
  long l = c; // i may not be necessary

  uintptr_t ptr;

  ptr = (l << 8) + CHARACTER_TAG;

  return ptr;
}

char get_char_value(OBJECT_PTR char_object)
{
  return char_object >> 8;
}

OBJECT_PTR cdr(OBJECT_PTR cons_obj)
{
  if(cons_obj == NIL)
    return NIL;
  else
    return (OBJECT_PTR)*((OBJECT_PTR *)(extract_ptr(cons_obj))+1);
}

OBJECT_PTR last_cell(OBJECT_PTR lst)
{
  OBJECT_PTR rest = lst;

  while(cdr(rest) != NIL)
    rest = (OBJECT_PTR)*((OBJECT_PTR *)(  extract_ptr(rest) )+1);
  
  return rest;
}

OBJECT_PTR concat(unsigned int count, ...)
{
  va_list ap;
  OBJECT_PTR lst, ret, rest;
  int i, start = 1;

  if(!count)
    return NIL;

  va_start(ap, count);

  lst = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);

  if(!IS_CONS_OBJECT(lst) && lst != NIL)
  {
    print_object(lst);printf("\n");
    assert(false);
  }

  //to skip NILs
  while(lst == NIL)
  {
    start++;

    if(start > count)
      return NIL;

    lst = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);

    if(!IS_CONS_OBJECT(lst) && lst != NIL)
    {
      print_object(lst);
      assert(false);
    }
  }

  ret = clone_object(lst);

  for(i=start; i<count; i++)
  {
    lst = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);

    if(lst == NIL)
      continue;

    if(!IS_CONS_OBJECT(lst))
    {
      //print_object(lst);
      assert(false);
    }

    rest = lst;

    while(rest != NIL)
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(clone_object(car(rest)), NIL));

      rest = cdr(rest);
    }
  }

  va_end(ap);

  return ret;
}

OBJECT_PTR concat_old(OBJECT_PTR lst1, OBJECT_PTR lst2)
{
  //note: the lst1 gets appended with lst2,
  //i.e., a fresh list doesn't get created
  uintptr_t ptr = extract_ptr(last_cell(lst1));
  set_heap(ptr, 1, lst2);

  return lst1;
}

OBJECT_PTR cons(OBJECT_PTR car, OBJECT_PTR cdr)
{
  uintptr_t ptr = object_alloc(2, CONS_TAG);

  set_heap(ptr, 0, car);
  set_heap(ptr, 1, cdr);

  return ptr + CONS_TAG;
}

OBJECT_PTR build_cons(OBJECT_PTR local_var_index)
{
  assert(IS_INTEGER_OBJECT(local_var_index));

  OBJECT_PTR ret = cons(NIL, NIL);

  assert(!stack_is_empty(g_call_chain));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  entry->local_vars_list = cons(ret, entry->local_vars_list);

  return ret;
}

OBJECT_PTR car(OBJECT_PTR cons_obj)
{
  if(cons_obj == NIL)
    return NIL;
  else
    return (OBJECT_PTR)*((OBJECT_PTR *)(extract_ptr(cons_obj)));
}

OBJECT_PTR setcar(OBJECT_PTR obj, OBJECT_PTR val)
{
  set_heap(extract_ptr(obj), 0, val);
  return val;
}

OBJECT_PTR setcdr(OBJECT_PTR obj, OBJECT_PTR val)
{
  set_heap(extract_ptr(obj), 1, val);
  return val;
}

OBJECT_PTR reverse(OBJECT_PTR lst)
{
  OBJECT_PTR ret = NIL, rest = lst;

  while(rest != NIL)
  {
    ret = cons(car(rest), ret);

    rest = cdr(rest);
  }

  return ret;
}

OBJECT_PTR list(int count, ...)
{
  va_list ap;
  OBJECT_PTR ret;
  int i;

  if(!count)
    return NIL;

  va_start(ap, count);

  ret = cons((OBJECT_PTR)va_arg(ap, OBJECT_PTR), NIL);

  for(i=1; i<count; i++)
  {
    OBJECT_PTR val = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
    uintptr_t ptr = extract_ptr(last_cell(ret));
    set_heap(ptr, 1, cons(val, NIL));
  }

  va_end(ap);

  return ret;
}

void print_cons_object(OBJECT_PTR obj)
{
  OBJECT_PTR car_obj = car(obj);
  OBJECT_PTR cdr_obj = cdr(obj);

  fflush(stdout);

  if(is_atom(cdr_obj) 
     && cdr_obj != NIL)
  {
    fprintf(stdout, "(");
    print_object(car_obj);
    fprintf(stdout, " . ");
    print_object(cdr_obj);
    fprintf(stdout, ")");
  }
  else
  {
    OBJECT_PTR rest = obj;

    fprintf(stdout, "(");

    while(rest != NIL && 
          !is_atom(rest))
    {
      print_object(car(rest));
      fprintf(stdout, " ");
      rest = cdr(rest);
    }

    if(is_atom(rest)
       && rest != NIL)
    {
      fprintf(stdout, ". ");
      print_object(rest);
      fprintf(stdout, ")");
    }
    else
      fprintf(stdout, "\b)");

    //fprintf(stdout, "\b\b)");
  }
}

void print_object(OBJECT_PTR obj_ptr)
{
  if(obj_ptr == NIL)
    fprintf(stdout, "nil");
  else if(IS_SYMBOL_OBJECT(obj_ptr))
    fprintf(stdout, "%s", get_symbol_name(obj_ptr));
  else if(IS_SMALLTALK_SYMBOL_OBJECT(obj_ptr))
    fprintf(stdout, "#%s", get_smalltalk_symbol_name(obj_ptr));
  else if(IS_INTEGER_OBJECT(obj_ptr))
    fprintf(stdout, "%d", get_int_value(obj_ptr));
  else if(IS_CONS_OBJECT(obj_ptr))
    print_cons_object(obj_ptr);
  else if(IS_CLOSURE_OBJECT(obj_ptr))
  {
    OBJECT_PTR lst_form = extract_ptr(obj_ptr) + CONS_TAG;
    int arity = get_int_value(car(reverse(lst_form)));
    if(arity == 0)
      fprintf(stdout, "#<CLOSURE %p> (a NiladicBlock)", (void *)obj_ptr);
    else if(arity == 1)
      fprintf(stdout, "#<CLOSURE %p> (a MonadicBlock)", (void *)obj_ptr);
    else
      fprintf(stdout, "#<CLOSURE %p> (arity = %d)", (void *)obj_ptr, arity);
  }
  else if(IS_CLASS_OBJECT(obj_ptr))
    fprintf(stdout, "#<CLASS %p> (%s)", (void *)obj_ptr, ((class_object_t *)extract_ptr(obj_ptr))->name);
  //fprintf(stdout, "#<CLASS %p>", (void *)obj_ptr);
  else if(IS_NATIVE_FN_OBJECT(obj_ptr))
    fprintf(stdout, "#<NATIVEFN %p> ", (void *)obj_ptr);
  else if(IS_OBJECT_OBJECT(obj_ptr))
  {
    object_t *obj = (object_t *)extract_ptr(obj_ptr);
    OBJECT_PTR cls_obj = obj->class_object;
    fprintf(stdout, "#<OBJECT %p> (instance of %s)", (void *)obj_ptr, ((class_object_t *)extract_ptr(cls_obj))->name);
  }
  else if(IS_CHARACTER_OBJECT(obj_ptr))
    fprintf(stdout, "$%c", get_char_value(obj_ptr));
  else if(IS_STRING_LITERAL_OBJECT(obj_ptr))
    fprintf(stdout, "%s", g_string_literals[obj_ptr >> OBJECT_SHIFT]);
  else if(IS_TRUE_OBJECT(obj_ptr) || IS_FALSE_OBJECT(obj_ptr))
    fprintf(stdout, "%s", obj_ptr == TRUE ? "true" : "false");
  else if(IS_ARRAY_OBJECT(obj_ptr))
    fprintf(stdout, "#<OBJECT %p> (instance of Array)", (void *)obj_ptr);
  else
    error("<invalid object %p>", (void *)obj_ptr);

  fflush(stdout);
}

int cons_length(OBJECT_PTR cons_obj)
{
  OBJECT_PTR rest;
  int l = 0;

  if(cons_obj == NIL)
    return 0;

  if(!IS_CONS_OBJECT(cons_obj))
    assert(false);

  rest = cons_obj;

  while(rest != NIL)
  {
    l++;
    rest = cdr(rest);
  }

  return l;
}

OBJECT_PTR build_symbol_object(int symbol_index)
{
  return (OBJECT_PTR)(((OBJECT_PTR)0 << (SYMBOL_BITS + OBJECT_SHIFT)) + (symbol_index << OBJECT_SHIFT) + SYMBOL_TAG);
}

BOOLEAN is_atom(OBJECT_PTR obj)
{
  //TODO: add other atomic objects
  return IS_SYMBOL_OBJECT(obj)      ||
    IS_INTEGER_OBJECT(obj)          ||
    IS_CLASS_OBJECT(obj)            ||
    IS_OBJECT_OBJECT(obj)           ||
    IS_SMALLTALK_SYMBOL_OBJECT(obj) ||
    IS_CHARACTER_OBJECT(obj)        ||
    IS_STRING_LITERAL_OBJECT(obj)   ||
    IS_ARRAY_OBJECT(obj)            ||
    IS_TRUE_OBJECT(obj)             ||
    IS_FALSE_OBJECT(obj);
}

OBJECT_PTR clone_object(OBJECT_PTR obj)
{
  OBJECT_PTR ret;

  if(is_atom(obj) || IS_NATIVE_FN_OBJECT(obj) || IS_CLOSURE_OBJECT(obj))
    ret = obj; //atoms are immutable and are reused
  else
  {
    if(IS_CONS_OBJECT(obj))
      ret = cons(clone_object(car(obj)), clone_object(cdr(obj)));
    else
      assert(false);
    //TODO: delete array code below after confirming it's not
    //needed for the compiler
    /*
    else if(IS_ARRAY_OBJECT(obj))
    {
      uintptr_t ptr = extract_ptr(obj);

      unsigned int len = *((OBJECT_PTR *)ptr);

      uintptr_t *raw_ptr;

      uintptr_t new_obj;

      int i;

      new_obj = object_alloc(len+1, ARRAY_TAG);
      
      *((OBJECT_PTR *)new_obj) = len;

      for(i=1; i<=len; i++)
	set_heap(new_obj, i, clone_object(get_heap(ptr, i)));

      ret = new_obj + ARRAY_TAG;
    }
    */
  }

  return ret;
}

int add_symbol(char *sym)
{
  g_compiler_package->nof_symbols++;
  
  g_compiler_package->symbols = (char **)GC_REALLOC(g_compiler_package->symbols,
						    g_compiler_package->nof_symbols * sizeof(char *));

  g_compiler_package->symbols[g_compiler_package->nof_symbols - 1] = GC_strdup(sym);

  return g_compiler_package->nof_symbols - 1;
}

OBJECT_PTR gensym()
{
  char sym[20];

  gen_sym_count++;

  sprintf(sym, "#:G%d", gen_sym_count);

  return build_symbol_object(add_symbol(sym));
}

OBJECT_PTR convert_native_fn_to_object(nativefn nf)
{
  uintptr_t ptr = object_alloc(1, NATIVE_FN_TAG);

  native_fn_obj_t *nfobj = (native_fn_obj_t *)GC_MALLOC(sizeof(native_fn_obj_t));
  nfobj->nf = nf;
  *((native_fn_obj_t *)ptr) = *nfobj;

  return ptr + NATIVE_FN_TAG;
}

OBJECT_PTR convert_class_object_to_object_ptr(class_object_t *cls_obj)
{
  return (uintptr_t)cls_obj + CLASS_OBJECT_TAG;
}

OBJECT_PTR convert_array_object_to_object_ptr(array_object_t *arr_obj)
{
  uintptr_t ptr = object_alloc(1, ARRAY_TAG);

  *((array_object_t *)ptr) = *arr_obj;

  return ptr + ARRAY_TAG;  
}

OBJECT_PTR selector(char *s)
{
  return get_symbol(s);
}

OBJECT_PTR identity_function(OBJECT_PTR closure, ...)
{
  va_list ap;
  va_start(ap, closure);
  OBJECT_PTR ret = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
  va_end(ap);
  
  return ret;
}

OBJECT_PTR create_closure(OBJECT_PTR arg_count, OBJECT_PTR count1, nativefn fn, ...)
{
  va_list ap;

#ifdef DEBUG  
  printf("Entering create_closure()\n");
#endif
  
  va_start(ap, fn);

  OBJECT_PTR fnobj = convert_native_fn_to_object(fn);

  int i;
  OBJECT_PTR closed_object;
  OBJECT_PTR ret;

  assert(IS_INTEGER_OBJECT(count1));

  int count = get_int_value(count1);

  //since count can be zero,
  //disabling this
  //if(!count)
  //  return NIL;

  //ret = cons(fnobj, NIL);
  ret = NIL;

  for(i=0; i<count; i++)
  {
    closed_object = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
    ret = cons(closed_object, ret); 
  }

  ret = list(3,
	     fnobj,
	     reverse(ret),
	     arg_count);

  ret = extract_ptr(ret) + CLOSURE_TAG;

  assert(IS_CLOSURE_OBJECT(ret));

  va_end(ap);

#ifdef DEBUG  
  print_object(ret); printf(" is returned by create_closure()\n");
#endif
  
  return ret;  
}

OBJECT_PTR get_class_object(OBJECT_PTR obj)
{
  if(obj == NIL)
    return Nil;
  else if(IS_SYMBOL_OBJECT(obj))
    return Symbol;
  else if(IS_INTEGER_OBJECT(obj))
    return Integer;
  else if(IS_TRUE_OBJECT(obj))
    return Boolean;
  else if(IS_FALSE_OBJECT(obj))
    return Boolean;
  else if(IS_CLOSURE_OBJECT(obj))
  {
    OBJECT_PTR lst_form = extract_ptr(obj) + CONS_TAG;
    int arity = get_int_value(car(reverse(lst_form)));
    if(arity == 0)
      return NiladicBlock;
    else if(arity == 1)
      return MonadicBlock;
    else
      assert(false);//TODO
  }
  else if(IS_ARRAY_OBJECT(obj))
    return Array;
  else if(IS_CONS_OBJECT(obj)) //TODO: create a proper class for CONS objects.
    return Object;             //CONS objects are returned by Compiler>>compile
  else
    return ((object_t *)extract_ptr(obj))->class_object;
}

OBJECT_PTR get_string_obj(char *s)
{
  unsigned int i;

  for(i=0; i<g_nof_string_literals; i++)
  {
    if(!strcmp(s, g_string_literals[i]))
      return (i << OBJECT_SHIFT) + STRING_LITERAL_TAG;
  }

  g_nof_string_literals++;
  
  if(!g_string_literals)
    g_string_literals = (char **)GC_MALLOC(g_nof_string_literals * sizeof(char *));
  else
    g_string_literals = (char **)GC_REALLOC(g_string_literals, g_nof_string_literals * sizeof(char *));

  g_string_literals[g_nof_string_literals-1] = GC_strdup(s);

  return ((g_nof_string_literals - 1) << OBJECT_SHIFT) + STRING_LITERAL_TAG;
}

OBJECT_PTR get_parent_class(OBJECT_PTR cls)
{
  assert(IS_CLASS_OBJECT(cls));
  return ((class_object_t *)extract_ptr(cls))->parent_class_object;
}

BOOLEAN is_super_class(OBJECT_PTR cls1, OBJECT_PTR cls2)
{
  assert(IS_CLASS_OBJECT(cls1));
  assert(IS_CLASS_OBJECT(cls2));

  if(cls1 == Object)
    return true;
  
  OBJECT_PTR parent_cls = get_parent_class(cls2);

  while(parent_cls != Object)
  {
    if(cls1 == parent_cls)
      return true;

    parent_cls = get_parent_class(parent_cls);
  }

  return false;
}

stack_type *get_class_hierarchy(OBJECT_PTR class_object)
{
  assert(IS_CLASS_OBJECT(class_object));

  stack_type* hier = stack_create();

  //the current class is also part of the hierarchy

  OBJECT_PTR current_parent = class_object;

  while(current_parent != NIL)
  {
    stack_push(hier, (void *)current_parent);

    class_object_t *curr_cls_obj = (class_object_t *)extract_ptr(current_parent);

    current_parent = curr_cls_obj->parent_class_object;
  }

  return hier;
}

OBJECT_PTR initialize_object(OBJECT_PTR obj)
{
  stack_type *hier = get_class_hierarchy(get_class_object(obj));

  OBJECT_PTR *classes = (OBJECT_PTR *)stack_data(hier);
  int count = stack_count(hier);
  int i;

  unsigned int j, n;

  OBJECT_PTR cls, method;

  OBJECT_PTR selector = INITIALIZE_SELECTOR;

  //we are iterating in FIFO so that more
  //'senior' classes are processed first
  for(i=0; i<count; i++)
  {
    cls = classes[i];

    class_object_t *cls_obj_int = (class_object_t *)extract_ptr(cls);

    n = cls_obj_int->instance_methods->count;

    for(j=0; j<n; j++)
    {
      if(cls_obj_int->instance_methods->bindings[j].key == selector)
      {
	method = cls_obj_int->instance_methods->bindings[j].val;

	native_fn_obj_t *nfobj = (native_fn_obj_t *)extract_ptr(car(method));
	nativefn nf = nfobj->nf;
	assert(nf);

	OBJECT_PTR closed_vars = second(method);
	OBJECT_PTR ret = NIL;

	OBJECT_PTR rest = closed_vars;
  
	binding_env_t *inst_vars = NULL;
	binding_env_t *shared_vars = NULL;

	shared_vars = ((class_object_t *)extract_ptr(get_class_object(obj)))->shared_vars;
	inst_vars = ((object_t *)extract_ptr(obj))->instance_vars;

	while(rest != NIL)
	{
	  OBJECT_PTR closed_val_cons;

	  if(IS_OBJECT_OBJECT(obj) && inst_vars)
	  {
	    if(get_binding_val_regular(inst_vars, car(rest), &closed_val_cons))
	    {
	      ret = cons(closed_val_cons, ret);
	      rest = cdr(rest);
	      continue;
	    }
	  }

	  if(IS_OBJECT_OBJECT(obj) && shared_vars)
	  {
	    if(get_binding_val_regular(shared_vars, car(rest), &closed_val_cons))
	    {
	      ret = cons(closed_val_cons, ret);
	      rest = cdr(rest);
	      continue;
	    }
	  }

	  if(IS_CLASS_OBJECT(obj) && shared_vars)
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

	  rest = cdr(rest);
	}

	OBJECT_PTR cons_form = list(3, car(method), reverse(ret), convert_int_to_object(0));
	OBJECT_PTR closure_form = extract_ptr(cons_form) + CLOSURE_TAG;

	//TODO: this should be converted into a call to message_send(),
	//but the class lookup hierachy used by message_send() is the
	//opposite of what is required for 'initialize'
	put_binding_val(g_top_level, SELF, cons(obj, NIL));
	put_binding_val(g_top_level, SUPER, cons(obj, NIL));

	stack_push(g_call_chain, create_call_chain_entry(false, obj, selector, method, closure_form, 0, NULL, g_idclo, NIL, false));

	OBJECT_PTR ret1 = nf(closure_form, g_idclo);

	if(ret1 == NIL)
	  return NIL;

	break;
      }
    }
  }

  return obj;
}

void update_binding(binding_env_t *env, OBJECT_PTR key, OBJECT_PTR val)
{
  unsigned int i, n;

  n = env->count;

  for(i=0; i<n; i++)
  {
    if(env->bindings[i].key == key)
    {
      env->bindings[i].val = val;
      return;
    }
  }

  assert(false);
}

OBJECT_PTR get_binding(binding_env_t *env, OBJECT_PTR key)
{
  int i;
  for(i=0; i<env->count; i++)
    if(env->bindings[i].key == key)
      return env->bindings[i].val;

  return 0;
}
