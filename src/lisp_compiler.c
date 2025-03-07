#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "gc.h"

#include "global_decls.h"

//forward declarations
OBJECT_PTR assignment_conversion(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR expand_body(OBJECT_PTR);
OBJECT_PTR expand_bodies(OBJECT_PTR);
OBJECT_PTR map(OBJECT_PTR (*f)(OBJECT_PTR), OBJECT_PTR lst);
OBJECT_PTR union1(unsigned int, ...);
OBJECT_PTR difference(OBJECT_PTR, OBJECT_PTR);
BOOLEAN primop(OBJECT_PTR);
OBJECT_PTR translate_to_il(OBJECT_PTR);
OBJECT_PTR desugar_il(OBJECT_PTR);
OBJECT_PTR ren_transform(OBJECT_PTR, binding_env_t *);
OBJECT_PTR closure_conv_transform(OBJECT_PTR);

OBJECT_PTR convert_int_to_object(int);
int get_int_value(OBJECT_PTR);

OBJECT_PTR free_ids_il(OBJECT_PTR);
OBJECT_PTR simplify_il(OBJECT_PTR);
OBJECT_PTR convert_message_sends(OBJECT_PTR);
OBJECT_PTR mcps_transform(OBJECT_PTR);
OBJECT_PTR lift_transform(OBJECT_PTR, OBJECT_PTR);

extern OBJECT_PTR NIL;
extern OBJECT_PTR LET;
extern OBJECT_PTR LAMBDA;
extern OBJECT_PTR CONS;
extern OBJECT_PTR SET;
extern OBJECT_PTR MESSAGE_SEND;
extern OBJECT_PTR CAR;
extern OBJECT_PTR SETCAR;
extern OBJECT_PTR LET1;
extern OBJECT_PTR NTH;
extern OBJECT_PTR SAVE_CONTINUATION;
extern OBJECT_PTR CREATE_CLOSURE;
extern OBJECT_PTR EXTRACT_NATIVE_FN;
extern OBJECT_PTR METHOD_LOOKUP;
extern OBJECT_PTR RETURN_FROM;
extern OBJECT_PTR GET_CONTINUATION;

extern OBJECT_PTR SUPER;
extern OBJECT_PTR MESSAGE_SEND_SUPER;

OBJECT_PTR get_top_level_symbols()
{
  //TODO: global objects to be added
  return NIL;
}

OBJECT_PTR get_free_variables(OBJECT_PTR exp)
{
  return difference(free_ids_il(exp),
                    list(4, LET, SET, LAMBDA, NIL));
}

binding_env_t *create_binding_env()
{
  binding_env_t *env = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  env->count = 0;
  env->bindings = NULL;

  return env;
}

//this is to be used only within the compiler,
//another version (where missing bindings should be
//handled differently) should be used in places
//like method, variable lookups
OBJECT_PTR get_binding_val(binding_env_t *env, OBJECT_PTR key)
{
  int i;
  for(i=0; i<env->count; i++)
    if(env->bindings[i].key == key)
      return env->bindings[i].val;

  return key; //TODO: is this correct?
}

void put_binding_val(binding_env_t *env, OBJECT_PTR key, OBJECT_PTR val)
{
  int i;

  BOOLEAN found = false;

  for(i=0;i<env->count;i++)
  {
    if(env->bindings[i].key == key)
    {
      env->bindings[i].val = val;
      found = true;
      break;
    }
  }

  if(!found)
  {
    env->count++;

    binding_t *temp = (binding_t *)GC_REALLOC(env->bindings, env->count * sizeof(binding_t));

    assert(temp);

    env->bindings = temp;

    env->bindings[env->count-1].key = key;
    env->bindings[env->count-1].val = val;
  }
}

OBJECT_PTR message_selectors;

//Return the message selectors in the given expression.
//Used for filtering selectors from
//the free variables list in the downstream
//compiler passes.
OBJECT_PTR build_selectors_list(OBJECT_PTR res)
{
  if(is_atom(res) || res == NIL)
    return NIL;

  if(car(res) == MESSAGE_SEND)
    return concat(3,
                  build_selectors_list(second(res)),
                  list(1, third(res)),
                  build_selectors_list(CDDDR(res)));
  else if(car(res) == RETURN_FROM)
    return concat(2,
		  list(1, second(res)),
		  build_selectors_list(cdr(cdr(res))));

  return concat(2,
                build_selectors_list(car(res)),
                build_selectors_list(cdr(res)));
}

//add an underscore to message selectors to distinguish
//them from instance/class variables of the same name.
//otherwise these variables get filtered while building
//the free variables list
OBJECT_PTR decorate_message_selectors(OBJECT_PTR res)
{
  if(is_atom(res))
    return res;
  else if(car(res) == MESSAGE_SEND)
    return concat(2,
		  list(3,
		       MESSAGE_SEND,
		       decorate_message_selectors(second(res)),
		       get_symbol(append_char(get_symbol_name(third(res)),'_'))),
		  map(decorate_message_selectors,CDDDR(res)));
  else
    return map(decorate_message_selectors, res);
}

OBJECT_PTR apply_lisp_transforms(OBJECT_PTR obj)
{
  OBJECT_PTR res;

  //obj = decorate_message_selectors(obj1);

#ifdef DEBUG
  print_object(obj); printf(" is returned by decorate_message_selectors()\n");
#endif
  
  message_selectors = build_selectors_list(obj);
  
  //res = list(3, first(res), second(res), expand_body(CDDR(obj)));
  res = expand_bodies(obj);

#ifdef DEBUG
  print_object(res); printf(" is returned by expand_bodies()\n");
#endif
  
  res = convert_message_sends(res);

#ifdef DEBUG
  print_object(res); printf(" is returned by convert_message_sends()\n");
#endif
  
  res = assignment_conversion(res, concat(2,
                                          get_top_level_symbols(),
                                          get_free_variables(res)));
#ifdef DEBUG
  print_object(res); printf(" is returned by assignment_conversion()\n");
#endif
  
  res = translate_to_il(res);

#ifdef DEBUG
  print_object(res); printf(" is returned by translate_to_il()\n");
#endif
  
  res = desugar_il(res);

#ifdef DEBUG
  print_object(res); printf(" is returned by desugar_il()\n");
#endif
  
  res = ren_transform(res, create_binding_env());

#ifdef DEBUG
  print_object(res); printf(" is returned by ren_transform()\n");
#endif
  
  res = simplify_il(res);

#ifdef DEBUG
  print_object(res); printf(" is returned by simplify_il()\n");
#endif
  
  //res = convert_message_sends(res);
  
  res = mcps_transform(res);

#ifdef DEBUG
  print_object(res); printf(" is returned by mcps_transform()\n");
#endif

  res = closure_conv_transform(res);

#ifdef DEBUG
  print_object(res); printf(" is returned by closure_conv_transform()\n");
#endif
  
  res = lift_transform(res, NIL);

#ifdef DEBUG
  print_object(res); printf(" is returned by lift_transform()\n");
#endif
  
  return res;
}

OBJECT_PTR expand_body(OBJECT_PTR body)
{
  if(body == NIL)
    return NIL;
  else if(cons_length(body) == 1)
    return expand_bodies(car(body));
  else
    return list(3,
                LET,
                list(1, list(2, gensym(), expand_bodies(car(body)))),
                expand_body(cdr(body)));
}

OBJECT_PTR expand_bodies(OBJECT_PTR exp)
{
  if(is_atom(exp))
    return exp;
  else if(car(exp) == LAMBDA || car(exp) == LET)
    return list(3, first(exp), second(exp), expand_body(CDDR(exp)));
  else
    return cons(expand_bodies(car(exp)),
                expand_bodies(cdr(exp)));
}

BOOLEAN exists(OBJECT_PTR obj, OBJECT_PTR lst)
{
  OBJECT_PTR rest = lst;

  while(rest != NIL)
  {
    if(IS_SYMBOL_OBJECT(obj) && IS_SYMBOL_OBJECT(car(rest)) && obj == car(rest))
      return true;
    else if(obj == *((OBJECT_PTR *)extract_ptr(rest))) 
      return true;

    rest = *((OBJECT_PTR *)(  extract_ptr(rest)  ) + 1);
  }

  return false;
}

OBJECT_PTR maybe_cell(OBJECT_PTR id, OBJECT_PTR ids, OBJECT_PTR exp)
{
  if(exists(id, ids))
     return list(3, CONS, exp, NIL);
  else
    return exp;
}

OBJECT_PTR temp4(OBJECT_PTR x, OBJECT_PTR v1, OBJECT_PTR v2)
{
  return list(2,
              first(x),
              maybe_cell(first(x),
                         v1,
                         assignment_conversion(second(x), v2)));
}

OBJECT_PTR union_single_list(OBJECT_PTR lst)
{
  OBJECT_PTR ret, rest1, rest2;

  ret = NIL;

  rest1 = lst;

  while(rest1 != NIL)
  {
    rest2 = car(rest1);

    while(rest2 != NIL)
    {
      OBJECT_PTR obj = car(rest2);
      if(!exists(obj, ret))
      {
        if(ret == NIL)
          ret = cons(clone_object(obj), NIL);
        else
        {
          uintptr_t ptr = extract_ptr(last_cell(ret));
          set_heap(ptr, 1, cons(clone_object(obj), NIL));
        }
      }

      rest2 = cdr(rest2);
    }

    rest1 = cdr(rest1);
  }

  return ret;
}

OBJECT_PTR intersection(OBJECT_PTR lst1, OBJECT_PTR lst2)
{
  OBJECT_PTR ret = NIL, rest = lst1;

  while(rest != NIL)
  {
    OBJECT_PTR obj = car(rest);
    if(exists(obj, lst2))
    {
      if(ret == NIL)
        ret = cons(clone_object(obj), NIL);
      else
      {
        uintptr_t ptr = extract_ptr(last_cell(ret));
        set_heap(ptr, 1, cons(clone_object(obj), NIL));        
      }
    }

    rest = cdr(rest);
  }

  return ret;
}

OBJECT_PTR subexps(OBJECT_PTR exp)
{
  OBJECT_PTR car_exp = NIL;

  if(IS_CONS_OBJECT(exp))
    car_exp = car(exp);

  if(is_atom(exp))
    return NIL;
  else if(car_exp == SET || car_exp == LAMBDA)
    return list(1, third(exp));
  else if(primop(car_exp))
    return cdr(exp);
  else if(car_exp == LET)
  {
    return concat(map(CADR, second(exp)),
                  list(1, third(exp)));
  }
  else
    return exp;
}

OBJECT_PTR mutating_ids(OBJECT_PTR exp)
{
  OBJECT_PTR car_exp = NIL;

  if(IS_CONS_OBJECT(exp))
    car_exp = car(exp);

  if(is_atom(exp))
    return NIL;
  else if(car_exp == SET)
    return union1(2,
                  list(1, second(exp)),
                  mutating_ids(third(exp)));
  else if(car_exp == LAMBDA)
    return difference(mutating_ids(third(exp)),
                      second(exp));
  else if(car_exp == LET)
  {
    return union1(2,
                  union_single_list(map(mutating_ids,
                                        map(CADR, second(exp)))),
                  difference(mutating_ids(third(exp)),
                             map(car, second(exp))));
  }
  else
    return union_single_list(map(mutating_ids, subexps(exp)));
}

OBJECT_PTR partition(OBJECT_PTR ids, OBJECT_PTR exps)
{
  OBJECT_PTR mids = union_single_list(map(mutating_ids, exps));

  OBJECT_PTR ret = cons(intersection(ids, mids),
                        difference(ids, mids));
  return ret;
}

OBJECT_PTR map(OBJECT_PTR (*f)(OBJECT_PTR), OBJECT_PTR lst)
{
  assert(lst == NIL || IS_CONS_OBJECT(lst));

  OBJECT_PTR ret = NIL, rest = lst;

  while(rest != NIL)
  {
    OBJECT_PTR val = f(car(rest));

    if(ret == NIL)
      ret = cons(val, NIL);
    else
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(val, NIL));        
    }

    rest = cdr(rest);
  }

  return ret;  
}

OBJECT_PTR temp2(OBJECT_PTR x)
{
  return list(2, x, list(3, CONS, x, NIL));
}

OBJECT_PTR wrap_cells(OBJECT_PTR ids, OBJECT_PTR exp)
{
  if(ids == NIL)
    return exp;
  else
    return list(3, LET, map(temp2, ids), exp);
}

OBJECT_PTR difference(OBJECT_PTR lst1, OBJECT_PTR lst2)
{
  OBJECT_PTR ret = NIL, rest = lst1;

  OBJECT_PTR last = NIL;

  while(rest != NIL)
  {
    OBJECT_PTR obj = car(rest);
    if(!exists(obj, lst2))
    {
      if(ret == NIL)
      {
        ret = cons(clone_object(obj), NIL);
        last = ret;
      }
      else
      {
        uintptr_t ptr = extract_ptr(last);
        OBJECT_PTR temp = cons(clone_object(obj), NIL);
        set_heap(ptr, 1, temp);
        last = temp;
      }
    }

    rest = cdr(rest);
  }

  return ret;
}

OBJECT_PTR union1(unsigned int count, ...)
{
  va_list ap;
  OBJECT_PTR ret, rest;
  int i;

  if(!count)
    return NIL;

  ret = NIL;

  va_start(ap, count);

  for(i=0; i<count; i++)
  {
    rest = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);

    while(rest != NIL)
    {
      OBJECT_PTR obj = car(rest);

      if(!exists(obj, ret))
        ret = cons(clone_object(obj), ret);

      rest = cdr(rest);
    }
  }

  va_end(ap);

  return ret;
}

OBJECT_PTR map2(OBJECT_PTR (*f)(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR), 
                OBJECT_PTR v1, 
                OBJECT_PTR v2, 
                OBJECT_PTR lst)
{
  OBJECT_PTR ret = NIL, rest = lst;

  while(rest != NIL)
  {
    OBJECT_PTR val = f(car(rest), v1, v2);

    if(ret == NIL)
      ret = cons(val, NIL);
    else
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(val, NIL));        
    }

    rest = cdr(rest);
  }

  return ret;  
}

OBJECT_PTR map2_fn(OBJECT_PTR (*f)(OBJECT_PTR, 
                                   OBJECT_PTR (*)(OBJECT_PTR), 
                                   OBJECT_PTR), 
                   OBJECT_PTR (*f1)(OBJECT_PTR), 
                   OBJECT_PTR v2, 
                   OBJECT_PTR lst)
{
  OBJECT_PTR ret = NIL, rest = lst;

  while(rest != NIL)
  {
    OBJECT_PTR val = f(car(rest), f1, v2);

    if(ret == NIL)
      ret = cons(val, NIL);
    else
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(val, NIL));        
    }

    rest = cdr(rest);
  }

  return ret;  
}

BOOLEAN is_vararg_primop(OBJECT_PTR sym)
{
  return sym == MESSAGE_SEND || sym == MESSAGE_SEND_SUPER;
  //return false; //TODO: to be confirmed whether MESSAGE_SEND can be considered non-vararg
}

BOOLEAN primop(OBJECT_PTR sym)
{
  return sym == SAVE_CONTINUATION || sym == CAR || sym == SETCAR || sym == CONS || sym == RETURN_FROM || sym == GET_CONTINUATION;
}

OBJECT_PTR temp3(OBJECT_PTR x, OBJECT_PTR v1, OBJECT_PTR v2)
{
  return list(2, 
              first(x),
              maybe_cell(first(x),
                         v1,
                         assignment_conversion(second(x), v2)));
}

OBJECT_PTR temp7(OBJECT_PTR x, OBJECT_PTR v, OBJECT_PTR dummy)
{
  return assignment_conversion(x, v);
}

OBJECT_PTR map2_fn1(OBJECT_PTR (*f)(OBJECT_PTR, 
                                    OBJECT_PTR (*)(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR), 
                                   OBJECT_PTR), 
                    OBJECT_PTR (*f1)(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR), 
                    OBJECT_PTR v2, 
                    OBJECT_PTR lst)
{
  OBJECT_PTR ret = NIL, rest = lst;

  while(rest != NIL)
  {
    OBJECT_PTR val = f(car(rest), f1, v2);

    if(ret == NIL)
      ret = cons(val, NIL);
    else
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(val, NIL));        
    }

    rest = cdr(rest);
  }

  return ret;  
}

OBJECT_PTR temp6(OBJECT_PTR x, 
                 OBJECT_PTR (*f)(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR),
                 OBJECT_PTR v)
{
  return list(2,
              first(x),
              f(second(x), v, NIL));
}

OBJECT_PTR mapsub1(OBJECT_PTR exp, 
                   OBJECT_PTR (*tf)(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR),
                   OBJECT_PTR v)
{
  OBJECT_PTR car_exp = NIL;

  if(IS_CONS_OBJECT(exp))
    car_exp = car(exp);

  if(is_atom(exp))
    return exp;
  else if(car_exp == SET)
    return list(3, SET, second(exp), tf(third(exp),v,NIL));
  else if(car_exp == LAMBDA)
    return list(3, LAMBDA, second(exp), tf(third(exp),v,NIL));
  else if(primop(car_exp))
    return cons(first(exp),
                map2(tf, v, NIL, cdr(exp)));
  else if(car_exp == LET)
    return list(3,
                car_exp,
                map2_fn1(temp6, tf, v, second(exp)),
                tf(third(exp),v,NIL));
  else
    return map2(tf, v, NIL, exp);
}

OBJECT_PTR assignment_conversion(OBJECT_PTR exp, OBJECT_PTR ids)
{
  OBJECT_PTR first_exp = NIL;

  if(IS_CONS_OBJECT(exp))
    first_exp = first(exp);

  if(IS_SYMBOL_OBJECT(exp))
  {
    if(exists(exp, ids))
      return list(2, CAR, exp);
    else
      return exp;
  }
  else if(is_atom(exp))
    return exp;
  else if(first_exp == SET)
    return list(3, SETCAR, second(exp), assignment_conversion(third(exp),
                                                              ids));
  else if(first_exp == LAMBDA)
  {
    OBJECT_PTR pids = partition(second(exp),
                                list(1, third(exp)));
    OBJECT_PTR mids = car(pids);
    OBJECT_PTR uids = cdr(pids);
    
    return list(3, 
                LAMBDA, 
                second(exp),
                wrap_cells(mids,
                           assignment_conversion(third(exp),
                                                 difference(union1(2, ids, mids),
                                                            uids))));
  }
  else if(first_exp == LET)
  {
    OBJECT_PTR pids = partition(map(car, second(exp)),
                                list(1, third(exp)));

    OBJECT_PTR mids = car(pids);
    OBJECT_PTR uids = cdr(pids);

    return list(3, 
                LET, 
                map2(temp3, mids, ids, second(exp)),
                assignment_conversion(third(exp),
                                      difference(union1(2, ids, mids),
                                                 uids)));
  }
  else
    return mapsub1(exp, temp7, ids);
}

BOOLEAN equal(OBJECT_PTR obj1, OBJECT_PTR obj2)
{
  BOOLEAN ret = false;

  //TODO: extend this for array objects (any others?)

  if(obj1 == obj2) //integer objects and symbol objects will be covered here
    ret = true;
  else
  {
    if(IS_CONS_OBJECT(obj1) && IS_CONS_OBJECT(obj2))
      ret = (equal(car(obj1), car(obj2)) && 
	     equal(cdr(obj1), cdr(obj2)));
    else
      ret = false; //TODO: modify this when we incorporate other object types
  }
  
  return ret;
}

OBJECT_PTR subst(OBJECT_PTR exp1,
                 OBJECT_PTR exp2,
                 OBJECT_PTR exp)
{
  if(exp == NIL)
    return NIL;
  else if(is_atom(exp))
  {
    if(equal(exp,exp2))
      return exp1;
    else
      return exp;
  }
  else
  {
    return cons(subst(exp1, exp2, car(exp)),
                subst(exp1, exp2, cdr(exp)));
  }
}

OBJECT_PTR msubst(OBJECT_PTR ids, OBJECT_PTR exp)
{
  OBJECT_PTR res = clone_object(exp);

  OBJECT_PTR rest = ids;

  while(rest != NIL)
  {
    res = subst(list(2, CAR, car(rest)),
                car(rest),
                res);

    rest = cdr(rest);
  }

  return res;
}

OBJECT_PTR temp8(OBJECT_PTR x)
{
  return list(2, first(x), list(3, CONS, NIL, NIL));
}

OBJECT_PTR temp9(OBJECT_PTR x,
                 OBJECT_PTR v1,
                 OBJECT_PTR v2)
{
  return list(2,
              gensym(),
              list(3, 
                   SETCAR, 
                   first(x),
                   msubst(map(car, second(v1)),
                          translate_to_il(second(x)))));
}

/* TODO: is this pass even required,
   in the absence of the LETREC construct
   that made it necessary for pLisp? */
OBJECT_PTR translate_to_il(OBJECT_PTR exp)
{
  if(is_atom(exp))
    return exp;
  else
    return cons(translate_to_il(car(exp)),
                translate_to_il(cdr(exp)));
}

OBJECT_PTR desugar_il(OBJECT_PTR exp)
{
  if(second(exp) == NIL)
    return third(exp);
  else
    return list(3,
                LET,
                list(1, first(second(exp))),
                desugar_il(list(3,
                                LET1,
                                cdr(second(exp)),
                                third(exp))));
}

OBJECT_PTR temp91(OBJECT_PTR x, binding_env_t *env, OBJECT_PTR v)
{
  return list(2,
              first(x),
              ren_transform(second(second(x)), env));
}

OBJECT_PTR map2_for_ren_transform(OBJECT_PTR (*f)(OBJECT_PTR, binding_env_t *, OBJECT_PTR),
                                  binding_env_t *env,
                                  OBJECT_PTR v2,
                                  OBJECT_PTR lst)
{
  OBJECT_PTR ret = NIL, rest = lst;

  while(rest != NIL)
  {
    OBJECT_PTR val = f(car(rest), env, v2);

    if(ret == NIL)
      ret = cons(val, NIL);
    else
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(val, NIL));        
    }

    rest = cdr(rest);
  }

  return ret;    
}

OBJECT_PTR temp13(OBJECT_PTR x)
{
  return gensym();
}

OBJECT_PTR range(int start, int end, int step)
{
  OBJECT_PTR ret = NIL;

  int i = start;

  while(i <= end)
  {
    OBJECT_PTR val = convert_int_to_object(i);

    if(ret == NIL)
      ret = cons(val, NIL);
    else
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(val, NIL));        
    }

    i = i + step;
  }

  return ret;    
}

OBJECT_PTR generate_fresh_ids(unsigned int count)
{
  return map(temp13, range(1,count,1));
}

OBJECT_PTR mapcar(OBJECT_PTR (*f)(OBJECT_PTR,OBJECT_PTR),
                  OBJECT_PTR list1,
                  OBJECT_PTR list2)
{
  OBJECT_PTR ret = NIL, rest1 = list1, rest2 = list2;

  while(rest1 != NIL || rest2 != NIL)
  {
    OBJECT_PTR val = f(car(rest1), car(rest2));

    if(ret == NIL)
      ret = cons(val, NIL);
    else
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(val, NIL));        
    }

    rest1 = cdr(rest1);
    rest2 = cdr(rest2);
  }

  return ret;    
}


OBJECT_PTR pair( OBJECT_PTR list1,
                 OBJECT_PTR list2)
{
  OBJECT_PTR ret = NIL, rest1 = list1, rest2 = list2;

  while(rest1 != NIL || rest2 != NIL)
  {
    OBJECT_PTR val = list(2, car(rest1), car(rest2));

    if(ret == NIL)
      ret = cons(val, NIL);
    else
    {
      uintptr_t ptr = extract_ptr(last_cell(ret));
      set_heap(ptr, 1, cons(val, NIL));        
    }

    rest1 = cdr(rest1);
    rest2 = cdr(rest2);
  }

  return ret;    
}

OBJECT_PTR ren_transform(OBJECT_PTR exp, binding_env_t *env)
{
  if(exp == NIL)
    return NIL;
  else if(IS_SYMBOL_OBJECT(exp))
    return get_binding_val(env, exp);
  else if(is_atom(exp))
    return exp;
  else if(car(exp) == LAMBDA)
  {
    OBJECT_PTR fresh_ids = generate_fresh_ids(cons_length(second(exp)));

    OBJECT_PTR new_bindings = mapcar(cons, second(exp), fresh_ids);

    OBJECT_PTR rest = new_bindings;

    while(rest != NIL)
    {
      put_binding_val(env, car(car(rest)), cdr(car(rest)));
      rest = cdr(rest);
    }

    return list(3,
                LAMBDA,
                fresh_ids,
                ren_transform(third(exp), env));
  }
  else if(car(exp) == LET)
  {
    OBJECT_PTR fresh_ids = generate_fresh_ids(cons_length(second(exp)));

    OBJECT_PTR new_bindings = mapcar(cons,
                                     map(car, second(exp)),
                                     fresh_ids);

    OBJECT_PTR rest = new_bindings;

    while(rest != NIL)
    {
      put_binding_val(env, car(car(rest)), cdr(car(rest)));
      rest = cdr(rest);
    }

    return list(3,
                LET,
                map2_for_ren_transform(temp91, env, NIL, pair(fresh_ids, second(exp))),
                ren_transform(third(exp), env));
  }
  else
    return cons(ren_transform(car(exp), env),
                ren_transform(cdr(exp), env));
}

OBJECT_PTR temp11(OBJECT_PTR v1, OBJECT_PTR v2)
{
  return list(2, v1, v2);
}

OBJECT_PTR flatten(OBJECT_PTR lst)
{
  OBJECT_PTR ret = NIL, rest = lst;

  while(rest != NIL)
  {
    if(IS_CONS_OBJECT(car(rest)) || car(rest) == NIL)
    {
      OBJECT_PTR rest1 = car(rest);

      while(rest1 != NIL)
      {
        if(ret == NIL)
          ret = cons(car(rest1), NIL);
        else
        {
          uintptr_t ptr = extract_ptr(last_cell(ret));
          set_heap(ptr, 1, cons(car(rest1), NIL));        
        }

        rest1 = cdr(rest1);
      }
    }
    else
    {
      if(ret == NIL)
        ret = cons(car(rest), NIL);
      else
      {
        uintptr_t ptr = extract_ptr(last_cell(ret));
        set_heap(ptr, 1, cons(car(rest), NIL));        
      }
    }

    rest = cdr(rest);
  }

  return ret;  
}

OBJECT_PTR free_ids_il(OBJECT_PTR exp)
{
  OBJECT_PTR car_exp = NIL;

  if(IS_CONS_OBJECT(exp))
    car_exp = car(exp);

  if(exp == NIL)
    return NIL;
  else if(IS_SYMBOL_OBJECT(exp))
  {
    if(primop(exp))
      return NIL;
    else
      return list(1, exp);
  }
  else if(is_atom(exp))
    return NIL;
  else if(car_exp == LAMBDA)
    return difference(union1(2, free_ids_il(third(exp)), free_ids_il(fourth(exp))),
                      second(exp));
  else if(car_exp == LET)
    return union1(2,
                  flatten(map(free_ids_il,
                              map(CADR, second(exp)))),
                  difference(free_ids_il(third(exp)),
                             map(car, second(exp))));
  //We don't want message selectors to be processed as free variables.
  //Also, MESSAGE_SEND itself is going to be a top level closure.
  else if(car_exp == MESSAGE_SEND)
    return concat(2, list(1, MESSAGE_SEND), difference(free_ids_il(cdr(exp)), list(1, third(exp))));
  else if(car_exp == MESSAGE_SEND_SUPER)
    return concat(2, list(1, MESSAGE_SEND_SUPER), difference(free_ids_il(cdr(exp)), list(1, third(exp))));
  else if(car_exp == RETURN_FROM)
    return difference(free_ids_il(cdr(cdr(exp))), list(1, second(exp)));
  else
    return flatten(cons(free_ids_il(car(exp)),
                        free_ids_il(cdr(exp))));
}

OBJECT_PTR simplify_il_empty_let(OBJECT_PTR exp)
{
  if(is_atom(exp))
    return exp;
  else if(first(exp) == LET && second(exp) == NIL)
    return third(exp);
  else
    return cons(simplify_il_empty_let(car(exp)),
                simplify_il_empty_let(cdr(exp)));
}

OBJECT_PTR simplify_il_implicit_let(OBJECT_PTR exp)
{
  if(IS_CONS_OBJECT(exp) &&
     IS_CONS_OBJECT(car(exp)) &&
     CAAR(exp) == LAMBDA)
    return list(3,
                LET,
                mapcar(temp11, second(first(exp)), cdr(exp)),
                third(first(exp)));
  else
    return exp;
}

OBJECT_PTR simplify_il_eta(OBJECT_PTR exp)
{
  if(is_atom(exp))
    return exp;
  else if(car(exp) == LAMBDA &&
          IS_CONS_OBJECT(third(exp)) &&
          !primop(first(third(exp))) &&
          first(third(exp)) != LET &&
          (IS_SYMBOL_OBJECT(first(third(exp))) ||
           (IS_CONS_OBJECT(first(third(exp))) &&
            first(first(third(exp))) == LAMBDA)) &&
          second(exp) == CDDR(third(exp)) &&
          intersection(free_ids_il(first(third(exp))),
                       second(exp)) == NIL)
    return first(third(exp));
  else
    return cons(simplify_il_eta(car(exp)),
                simplify_il_eta(cdr(exp)));
}

OBJECT_PTR simplify_il_copy_prop(OBJECT_PTR exp)
{
  if(is_atom(exp))
    return exp;
  else if(cons_length(exp) == 3                       && // - expression a three-element list
          first(exp) == LET                           && // - first element is LET
          IS_CONS_OBJECT(second(exp))                 && // - second element is a
          cons_length(second(exp)) == 1               && //   one-element list
          IS_CONS_OBJECT(first(second(exp)))          && // - that one element is also a list,
          cons_length(first(second(exp))) == 2        && //   with two elements
          IS_SYMBOL_OBJECT(first(first(second(exp)))) && // - which are both 
          IS_SYMBOL_OBJECT(second(first(second(exp)))))  //   symbols
    return subst(second(first(second(exp))),
                 first(first(second(exp))),
                 simplify_il_copy_prop(third(exp))); //<- TODO: this change has to be propagated to pLisp
  else
    return cons(simplify_il_copy_prop(car(exp)),
                simplify_il_copy_prop(cdr(exp)));
}

OBJECT_PTR simplify_il(OBJECT_PTR exp)
{
  return simplify_il_copy_prop(
    simplify_il_eta(
      simplify_il_implicit_let(
        simplify_il_empty_let(exp))));
}

OBJECT_PTR convert_message_sends(OBJECT_PTR exp)
{
  if(is_atom(exp))
    return exp;
  else if(car(exp) == MESSAGE_SEND && second(exp) == SUPER)
  {
    return concat(2,
		  list(2, MESSAGE_SEND_SUPER, SUPER),
		  map(convert_message_sends,(CDDR(exp))));
  }
  else
  {
    OBJECT_PTR ret = cons(convert_message_sends(car(exp)),
			  convert_message_sends(cdr(exp)));
    return ret;
  }
}

OBJECT_PTR add_ints(OBJECT_PTR count1, ...)
{
  va_list ap;
  OBJECT_PTR arg;
  int i;
  int sum = 0;

  unsigned int count = get_int_value(count1);

  va_start(ap, count1);

  for(i=0; i<count; i++)
  {
    arg = (OBJECT_PTR)va_arg(ap, OBJECT_PTR);

    if(IS_INTEGER_OBJECT(arg))
      sum += get_int_value(arg);
    else
    {
      error("add_ints(): Arguments to ADD should be integers or floats\n");
      return NIL;
    }
  }

  va_end(ap);

  return convert_int_to_object(sum);  
}

OBJECT_PTR nth(OBJECT_PTR n, OBJECT_PTR lst)
{
  assert(IS_CONS_OBJECT(lst) || IS_CLOSURE_OBJECT(lst));

  assert(IS_INTEGER_OBJECT(n));

  OBJECT_PTR lst1 = IS_CONS_OBJECT(lst) ? lst : extract_ptr(lst) + CONS_TAG;
  
  int i_val = get_int_value(n);

  if(i_val < 0 || i_val >= cons_length(lst1))
    return NIL;
  else
  {
    int i = 0;

    OBJECT_PTR rest = lst1;
    
    while(i<i_val)
    {
      rest = cdr(rest);
      i++;
    }

    return car(rest);
  }
}

//used in generated code. the symbol is NTH, but the function
//is nth_closed_val
OBJECT_PTR nth_closed_val(OBJECT_PTR n, OBJECT_PTR closure)
{
  assert(IS_CLOSURE_OBJECT(closure));
  OBJECT_PTR lst_form = extract_ptr(closure) + CONS_TAG;
  return nth(convert_int_to_object(get_int_value(n)-1), second(lst_form));
}


OBJECT_PTR temp12(OBJECT_PTR x, OBJECT_PTR v1, OBJECT_PTR v2)
{
  return list(2,
              nth(x, v1),
              list(3,
                   NTH,
                   add_ints(convert_int_to_object(2), x, convert_int_to_object(1)),
                   v2));
}

OBJECT_PTR closure_conv_transform_abs_cont(OBJECT_PTR exp)
{
  OBJECT_PTR free_ids = difference(free_ids_il(exp), message_selectors);
  OBJECT_PTR iclo = gensym();

  if(free_ids == NIL)
    return concat(4,
                  list(1, CREATE_CLOSURE),
		  list(1, convert_int_to_object(cons_length(second(exp))-1)),
                  list(1, convert_int_to_object(0)),
                  list(1,list(4,
                              LAMBDA,
                              concat(2,
                                     list(1, iclo),
                                     second(exp)),
                              third(exp),
                              closure_conv_transform(fourth(exp)))));
  else
  {
    return concat(5,
                  list(1, CREATE_CLOSURE),
		  list(1, convert_int_to_object(cons_length(second(exp))-1)),
                  list(1, convert_int_to_object(cons_length(free_ids))),
                  list(1,
                       list(4,
                            LAMBDA,
                            concat(2, list(1, iclo), second(exp)),
                            third(exp),
                            list(3,
                                 LET1,
                                 map2(temp12, free_ids, iclo,
                                      range(0, cons_length(free_ids)-1, 1)),
                                 closure_conv_transform(fourth(exp))))),
                  free_ids);
  }
}

OBJECT_PTR closure_conv_transform_abs_no_cont(OBJECT_PTR exp)
{
  OBJECT_PTR free_ids = difference(free_ids_il(exp), message_selectors);
  OBJECT_PTR iclo = gensym();

  if(free_ids == NIL)
    return concat(4,
                  list(1, CREATE_CLOSURE),
		  list(1, convert_int_to_object(cons_length(second(exp))-1)),
                  list(1, convert_int_to_object(0)),
                  list(1,list(3,
                              LAMBDA,
                              concat(2,
                                     list(1, iclo),
                                     second(exp)),
                              closure_conv_transform(third(exp)))));
  else
  {
    return concat(5,
                  list(1, CREATE_CLOSURE),
 		  list(1, convert_int_to_object(cons_length(second(exp))-1)),
		  list(1, convert_int_to_object(cons_length(free_ids))),
                  list(1,
                       list(3,
                            LAMBDA,
                            concat(2, list(1, iclo), second(exp)),
                            list(3,
                                 LET1,
                                 map2(temp12, free_ids, iclo,
                                      range(0, cons_length(free_ids)-1, 1)),
                                 closure_conv_transform(third(exp))))),
                  free_ids);
  }
}

OBJECT_PTR closure_conv_transform_let(OBJECT_PTR exp)
{
  OBJECT_PTR exp1 = closure_conv_transform(second(first(second(exp))));
  OBJECT_PTR icode = gensym();

  return list(3,
              LET1,
              list(2,
                   list(2, icode, fourth(exp1)),
                   list(2, 
                        first(first(second(exp))),
                        concat(2,
                               list(4,
				    CREATE_CLOSURE,
				    convert_int_to_object(cons_length(second(fourth(exp1)))-2),
				    convert_int_to_object(cons_length(cdr(CDDDR(exp1)))),
				    icode),
                               cdr(CDDDR(exp1))))),
              closure_conv_transform(third(exp)));
}

OBJECT_PTR closure_conv_transform_app(OBJECT_PTR exp)
{
  OBJECT_PTR iclo = gensym();
  OBJECT_PTR icode = gensym();

  return list(3,
              LET1,
              list(2,
                   list(2, iclo, closure_conv_transform(first(exp))),
                   list(2, icode, list(2, EXTRACT_NATIVE_FN, iclo))),
              concat(2,
                     list(2, icode, iclo),
                     map(closure_conv_transform, cdr(exp))));
}

OBJECT_PTR temp5(OBJECT_PTR x, 
                 OBJECT_PTR (*f)(OBJECT_PTR),
                 OBJECT_PTR v)
{
  return list(2,
              first(x),
              f(second(x)));
}

OBJECT_PTR mapsub(OBJECT_PTR exp, 
                  OBJECT_PTR (*tf)(OBJECT_PTR))
{
  OBJECT_PTR car_exp = NIL;

  if(IS_CONS_OBJECT(exp))
    car_exp = car(exp);

  if(is_atom(exp))
    return exp;
  else if(car_exp == SET)
    return list(3, SET, second(exp), tf(third(exp)));
  else if(car_exp == LAMBDA)
    return list(3, LAMBDA, second(exp), tf(third(exp)));
  else if(primop(car_exp))
    return cons(first(exp),
                map(tf, cdr(exp)));
  else if(car_exp == LET)
    return list(3,
                car_exp,
                map2_fn(temp5, tf, NIL, second(exp)),
                tf(third(exp)));
  else
    return map(tf, exp);
}

OBJECT_PTR closure_conv_transform(OBJECT_PTR exp)
{
  OBJECT_PTR car_exp = NIL;

  if(IS_CONS_OBJECT(exp))
    car_exp = car(exp);

  if(exp == NIL)
    return NIL;
  else if(is_atom(exp))
    return exp;
  else if(car_exp == LAMBDA)
  {
    if(cons_length(exp) == 3)
      return closure_conv_transform_abs_no_cont(exp);
    else
      return closure_conv_transform_abs_cont(exp);
  }
  else if(car_exp == LET)
  {
    OBJECT_PTR rval = second(first(second(exp)));

    if(IS_CONS_OBJECT(rval) &&
       first(rval) == LAMBDA)
      return closure_conv_transform_let(exp);
    else
      return mapsub(exp, closure_conv_transform);
  }
  else if(primop(car_exp))
    return mapsub(exp, closure_conv_transform);
  else
    return closure_conv_transform_app(exp);
}

OBJECT_PTR lift_transform(OBJECT_PTR exp, OBJECT_PTR bindings)
{
  if(is_atom(exp))
    return cons(exp, bindings);
  else if(car(exp) == LAMBDA)
  {
    OBJECT_PTR sym = gensym();
    OBJECT_PTR res = lift_transform(cons_length(exp) == 3 ? third(exp) : fourth(exp),
                                    bindings);

    return cons(sym,
                concat(2,
                       list(1,
                            list(2,
                                 sym,
                                 cons_length(exp) == 3 ? list(3, LAMBDA, second(exp), car(res)) : list(4, LAMBDA, second(exp), third(exp), car(res)))),
                       cdr(res)));
  }
  else
  {
    OBJECT_PTR car_res = lift_transform(car(exp), bindings);
    OBJECT_PTR cdr_res = lift_transform(cdr(exp), cdr(car_res));

    return cons(cons(car(car_res), car(cdr_res)),
                cdr(cdr_res));
  }
}

//this function takes the result of the
//apply_lisp_transforms() function
//and returns the arity of the native function
//underlying the closure represented
//by the transform. The arity (after conversion
//to an OBJECT_PTR) is appended to the closure
//object (after the closed vals) and is used
//for figuring out what type of block closure
//the closure represents (NiladicBlock, etc.) and
//for validating message sends.
OBJECT_PTR extract_arity(OBJECT_PTR sexp)
{
  OBJECT_PTR closure_sym = third(first(sexp));
  OBJECT_PTR lambdas = cdr(sexp);
  OBJECT_PTR lambda;

  while(lambdas != NIL)
  {
    lambda = car(lambdas);

    if(car(lambda) == closure_sym)
      return convert_int_to_object(cons_length(second(lambda))-2);
    
    lambdas = cdr(lambdas);
  }

  assert(false);

  //TODO: control will not reach here actually. handle this better
  return 0;
}
