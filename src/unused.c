/*

This file contains the code that was written to
try to avoid the special processing for
Smalltalk>>addInstaceMethod and Smalltalk>>addClassMethod.

Due to the inability of the compiler pipeline to handle
the instance and class variables (they are tagged as free
variables by the assignment conversion pass, but are not
really free, as they are defined in the class), this code
does not work. Keeping the code in this file just in case.
  
*/


typedef struct free_vars_mapping_entry
{
  OBJECT_PTR class;
  OBJECT_PTR method_selector;
  OBJECT_PTR free_vars_array;
} free_vars_mapping_entry_t;

typedef struct free_vars_mapping
{
  unsigned int count;
  free_vars_mapping_entry_t *entries;
} free_vars_mapping_t;

//the free variables in the code block
//in all the calls to Smalltalk>>addInstanceMethod
//and Smalltalk>>addCLassMethod in a REPL expression
free_vars_mapping_t *g_free_vars_mapping;


OBJECT_PTR transform_return_statements(OBJECT_PTR exp)
{
  //the assumption is that decorate_message_selectors()
  //has already been invoked on the expression by repl()

  if(is_atom(exp))
    return exp;
  else if(first(exp) == MESSAGE_SEND &&
	  second(exp) == get_symbol("Smalltalk") &&
	  (third(exp) == get_symbol("addInstanceMethod1:toClass:withBody:_") ||
	   third(exp) == get_symbol("addClassMethod1:toClass:withBody:_")))
  {
    OBJECT_PTR arg_count   = fourth(exp);
    OBJECT_PTR selector    = fifth(exp);
    OBJECT_PTR class       = sixth(exp);
    OBJECT_PTR lambda_form = seventh(exp);

    OBJECT_PTR decorated_selector_smalltalk_symbol = get_smalltalk_symbol(append_char(get_smalltalk_symbol_name(selector),'_'));
    OBJECT_PTR decorated_selector_lisp_symbol = get_symbol(append_char(get_smalltalk_symbol_name(selector),'_'));
    
    //OBJECT_PTR decorated_selector = get_symbol(append_char(get_smalltalk_symbol_name(selector),'_'));
    //OBJECT_PTR decorated_selector = get_smalltalk_symbol(append_char(get_smalltalk_symbol_name(selector),'_'));
    //OBJECT_PTR decorated_selector = selector;

    OBJECT_PTR new_lambda_form = replace_method_selector(lambda_form, decorated_selector_lisp_symbol);

    setcdr(last_cell(third(new_lambda_form)), list(1, list(3, RETURN_FROM, decorated_selector_lisp_symbol, SELF)));

    OBJECT_PTR free_vars_lst = free_ids(new_lambda_form);
    print_object(free_vars_lst); printf(" is the free vars list\n");
    add_free_vars_mapping(class, decorated_selector_smalltalk_symbol, convert_list_to_array(free_vars_lst));
    
    return list(8,
		MESSAGE_SEND,
		get_symbol("Smalltalk"),
		third(exp),
		convert_int_to_object(get_int_value(arg_count)+1),
		decorated_selector_smalltalk_symbol,
		class,
		new_lambda_form,
                convert_int_to_object(cons_length(second(third(new_lambda_form)))));
  }
  else
    return map(transform_return_statements, exp);
}

void initialize_free_vars_mapping()
{
  g_free_vars_mapping = (free_vars_mapping_t *)GC_MALLOC(sizeof(free_vars_mapping_t));
  g_free_vars_mapping->count = 0;
  g_free_vars_mapping->entries = NULL;
}

void add_free_vars_mapping(OBJECT_PTR class, OBJECT_PTR method_selector, OBJECT_PTR free_vars_array)
{
  assert(g_free_vars_mapping);

  if(!g_free_vars_mapping->entries)
  {
    g_free_vars_mapping->count = 1;
    g_free_vars_mapping->entries = (free_vars_mapping_entry_t *)GC_MALLOC(sizeof(free_vars_mapping_entry_t));
  }
  else
  {
    g_free_vars_mapping->count++;
    g_free_vars_mapping->entries =
      (free_vars_mapping_entry_t *)GC_REALLOC(g_free_vars_mapping->entries,
					      g_free_vars_mapping->count * sizeof(free_vars_mapping_entry_t));
  }

  g_free_vars_mapping->entries[g_free_vars_mapping->count-1].class           = class;
  g_free_vars_mapping->entries[g_free_vars_mapping->count-1].method_selector = method_selector;
  g_free_vars_mapping->entries[g_free_vars_mapping->count-1].free_vars_array = free_vars_array;
}

OBJECT_PTR retrieve_free_vars_mapping(OBJECT_PTR class, OBJECT_PTR method_selector)
{
  unsigned int i, count;

  count = g_free_vars_mapping->count;

  for(i=0; i<count; i++)
  {
    if(g_free_vars_mapping->entries[i].class          == class &&
       g_free_vars_mapping->entries[i].method_selector == method_selector)
      return g_free_vars_mapping->entries[i].free_vars_array;
  }

  return NIL;
}

OBJECT_PTR free_ids(OBJECT_PTR exp)
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
    return difference(union1(2, free_ids(third(exp)), free_ids(fourth(exp))),
                      second(exp));
  else if(car_exp == LET)
    return union1(2,
                  flatten(map(free_ids,
                              map(CADR, second(exp)))),
                  difference(flatten(difference(map(free_ids, CDDR(exp)),
						map(car, second(exp)))),
			     list(9, NIL, LET, SET, LAMBDA, RETURN_FROM, SELF, SUPER, MESSAGE_SEND, MESSAGE_SEND_SUPER)));
  //We don't want message selectors to be processed as free variables.
  //Also, MESSAGE_SEND itself is going to be a top level closure.
  else if(car_exp == MESSAGE_SEND)
    return concat(2, list(1, MESSAGE_SEND), difference(free_ids(cdr(exp)), list(1, third(exp))));
  else if(car_exp == MESSAGE_SEND_SUPER)
    return concat(2, list(1, MESSAGE_SEND_SUPER), difference(free_ids(cdr(exp)), list(1, third(exp))));
  else if(car_exp == RETURN_FROM)
    return difference(free_ids(cdr(cdr(exp))), list(1, second(exp)));
  else
    return flatten(cons(free_ids(car(exp)),
                        free_ids(cdr(exp))));
}

OBJECT_PTR convert_list_to_array(OBJECT_PTR lst)
{
  assert(lst == NIL || IS_CONS_OBJECT(lst));

  array_object_t *array_obj = (array_object_t *)GC_MALLOC(sizeof(array_object_t));

  int i;
  
  int len = cons_length(lst);

  array_obj->nof_elements = (unsigned int)len;
  array_obj->elements = (OBJECT_PTR *)GC_MALLOC(len * sizeof(OBJECT_PTR));
  
  for(i=0; i<len; i++)
    array_obj->elements[i] = nth(convert_int_to_object(i), lst);
    
  return convert_array_object_to_object_ptr(array_obj);
}

OBJECT_PTR convert_array_to_list(OBJECT_PTR array)
{
  assert(IS_ARRAY_OBJECT(array));

  array_object_t *array_obj = (array_object_t *)extract_ptr(array);

  unsigned int count = array_obj->nof_elements;

  unsigned int i;

  OBJECT_PTR ret = NIL;

  for(i=0; i<count; i++)
    ret = cons(array_obj->elements[i], ret);

  return reverse(ret);
}

OBJECT_PTR add_instance_method1(OBJECT_PTR closure,
				OBJECT_PTR selector,
				OBJECT_PTR class_obj,
				OBJECT_PTR method_closure,
				OBJECT_PTR arity,
				OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));

#ifdef DEBUG  
  print_object(closure);printf("\n");
  print_object(selector);printf("\n");
  print_object(class_obj);printf("\n");
  print_object(method_closure);printf("\n");
#endif
  
  class_object_t *cls_obj_int = (class_object_t *)extract_ptr(class_obj);
  
  OBJECT_PTR free_vars_array = retrieve_free_vars_mapping(get_symbol(cls_obj_int->name), selector);

#ifdef DEBUG  
  if(free_vars_array != NIL)
    message_send(msg_snd_closure,
		 free_vars_array,
		 get_symbol("printString_"),
		 convert_int_to_object(0),
		 idclo);
  else
    printf("No free vars\n");
#endif

//
  //OBJECT_PTR selector_sym = get_symbol(append_char(get_smalltalk_symbol_name(selector),'_'));
  OBJECT_PTR selector_sym = get_symbol(get_smalltalk_symbol_name(selector));
  //OBJECT_PTR selector_sym = selector;

  OBJECT_PTR closed_vals = convert_array_to_list(free_vars_array);

  OBJECT_PTR nfo = convert_native_fn_to_object(extract_native_fn(method_closure));

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
      cls_obj->instance_methods->bindings[i].val = list(3, nfo, closed_vals, arity);
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
    cls_obj->instance_methods->bindings[cls_obj->instance_methods->count - 1].val = list(3, nfo, closed_vals, arity);
  }
//  
  nativefn1 nf2 = (nativefn1)extract_native_fn(cont);

  OBJECT_PTR ret = nf2(cont, class_obj);

  return ret;
}

OBJECT_PTR add_class_method1(OBJECT_PTR closure,
			     OBJECT_PTR selector,
			     OBJECT_PTR class_obj,
			     OBJECT_PTR method_closure,
			     OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(top_level, SELF));
  
  print_object(closure);printf("\n");
  print_object(selector);printf("\n");
  print_object(class_obj);printf("\n");
  print_object(method_closure);printf("\n");

  OBJECT_PTR free_vars_array = retrieve_free_vars_mapping(class_obj, selector);

  if(free_vars_array != NIL)
    message_send(msg_snd_closure,
		 free_vars_array,
		 get_symbol("printString_"),
		 convert_int_to_object(0),
		 idclo);
  else
    printf("No free vars\n");

  nativefn1 nf = (nativefn1)extract_native_fn(cont);

  OBJECT_PTR ret = nf(cont, class_obj);

  return ret;
}

