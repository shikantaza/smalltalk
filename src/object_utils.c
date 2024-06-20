#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "smalltalk.h"

OBJECT_PTR clone_object(OBJECT_PTR);
OBJECT_PTR get_symbol(char *);
BOOLEAN IS_CLOSURE_OBJECT(OBJECT_PTR);
BOOLEAN IS_NATIVE_FN_OBJECT(OBJECT_PTR);
BOOLEAN IS_TRUE_OBJECT(OBJECT_PTR);
BOOLEAN IS_FALSE_OBJECT(OBJECT_PTR);

BOOLEAN IS_CLASS_OBJECT(OBJECT_PTR);

extern OBJECT_PTR NIL;
extern OBJECT_PTR LET;
extern OBJECT_PTR SET;
extern OBJECT_PTR RETURN;
extern OBJECT_PTR MESSAGE_SEND;
extern OBJECT_PTR LAMBDA;

extern package_t *compiler_package;

extern OBJECT_PTR Integer;

OBJECT_PTR Symbol;
OBJECT_PTR Boolean;

int gen_sym_count = 0;

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
  
  return compiler_package->symbols[symbol_index];
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

OBJECT_PTR car(OBJECT_PTR cons_obj)
{
  if(cons_obj == NIL)
    return NIL;
  else
    return (OBJECT_PTR)*((OBJECT_PTR *)(extract_ptr(cons_obj)));
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
      fprintf(stdout, ")");

    //fprintf(stdout, "\b\b)");
  }
}

void print_object(OBJECT_PTR obj_ptr)
{
  if(IS_SYMBOL_OBJECT(obj_ptr))
    fprintf(stdout, "%s", get_symbol_name(obj_ptr));
  else if(IS_INTEGER_OBJECT(obj_ptr))
    fprintf(stdout, "%d", get_int_value(obj_ptr));
  else if(IS_CONS_OBJECT(obj_ptr))
    print_cons_object(obj_ptr);
  else if(IS_CLOSURE_OBJECT(obj_ptr))
    fprintf(stdout, "#<CLOSURE %p> ", (void *)obj_ptr);
  else if(IS_CLASS_OBJECT(obj_ptr))
    fprintf(stdout, "#<CLASS %p> (%s)", (void *)obj_ptr, ((class_object_t *)extract_ptr(obj_ptr))->name);
  //fprintf(stdout, "#<CLASS %p>", (void *)obj_ptr);
  else if(IS_NATIVE_FN_OBJECT(obj_ptr))
    fprintf(stdout, "#<NATIVEFN %p> ", (void *)obj_ptr);
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
  return IS_SYMBOL_OBJECT(obj) || IS_INTEGER_OBJECT(obj) || IS_CLASS_OBJECT(obj);
}

OBJECT_PTR clone_object(OBJECT_PTR obj)
{
  OBJECT_PTR ret;

#ifdef DEEP_DEBUG
  print_object(obj);
  fprintf(stdout, "\n");
#endif
  
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
  compiler_package->nof_symbols++;
  
  compiler_package->symbols = (char **)GC_REALLOC(compiler_package->symbols,
                                                  compiler_package->nof_symbols * sizeof(char *));

  compiler_package->symbols[compiler_package->nof_symbols - 1] = GC_strdup(sym);

  return compiler_package->nof_symbols - 1;
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

  //*((nativefn *)ptr) = nf;
  native_fn_obj_t *nfobj = (native_fn_obj_t *)GC_MALLOC(sizeof(native_fn_obj_t));
  nfobj->nf = nf;
  *((native_fn_obj_t *)ptr) = *nfobj;

  return ptr + NATIVE_FN_TAG;
}

OBJECT_PTR convert_class_object_to_object_ptr(class_object_t *cls_obj)
{
  //uintptr_t ptr = object_alloc(1, CLASS_OBJECT_TAG);
  //*((class_object_t *)ptr) = *cls_obj;
  //return ptr + CLASS_OBJECT_TAG;

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

OBJECT_PTR create_closure(OBJECT_PTR count1, nativefn fn, ...)
{
  va_list ap;

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

  ret = cons(fnobj, NIL);

  for(i=0; i<count; i++)
  {
    closed_object = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);
    //print_object(closed_object); printf("^^^^^\n");
    uintptr_t ptr = extract_ptr(last_cell(ret));
    set_heap(ptr, 1, cons(closed_object, NIL));
  }

  ret = extract_ptr(ret) + CLOSURE_TAG;

  assert(IS_CLOSURE_OBJECT(ret));

  va_end(ap);

  //print_object(ret); printf(" is returned by create_closure()\n");

  return ret;  
}

OBJECT_PTR get_class_object(OBJECT_PTR obj)
{
  if(IS_SYMBOL_OBJECT(obj))
    return Symbol;
  else if(IS_INTEGER_OBJECT(obj))
    return Integer;
  else if(IS_TRUE_OBJECT(obj))
    return Boolean;
  else if(IS_FALSE_OBJECT(obj))
    return Boolean;
  else
    return ((object_t *)extract_ptr(obj))->class_object;
}
