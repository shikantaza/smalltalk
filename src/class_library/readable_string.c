#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

//workaround for variadic function arguments
//getting clobbered in ARM64
typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR ReadableString;

extern OBJECT_PTR InvalidArgument;
extern binding_env_t *g_top_level;

extern OBJECT_PTR NIL;
extern OBJECT_PTR SELF;
extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

extern OBJECT_PTR Object;

extern stack_type *g_call_chain;

extern char **g_string_literals;

extern OBJECT_PTR g_msg_snd_closure;
extern OBJECT_PTR g_idclo;
extern OBJECT_PTR VALUE1_SELECTOR;
extern OBJECT_PTR VALUE_SELECTOR;

extern OBJECT_PTR Error;

OBJECT_PTR readable_string_size(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  pop_if_top(entry);

  return invoke_cont_on_val(cont, convert_int_to_object(strlen(g_string_literals[receiver >> OBJECT_SHIFT])));
}

OBJECT_PTR readable_string_is_empty(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  pop_if_top(entry);

  return invoke_cont_on_val(cont, strlen(g_string_literals[receiver >> OBJECT_SHIFT]) == 0 ? TRUE : FALSE);
}

OBJECT_PTR readable_string_not_empty(OBJECT_PTR closure, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  pop_if_top(entry);

  return invoke_cont_on_val(cont, strlen(g_string_literals[receiver >> OBJECT_SHIFT]) == 0 ? FALSE : TRUE);
}

OBJECT_PTR readable_string_do(OBJECT_PTR closure, OBJECT_PTR operation, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  OBJECT_PTR ret;

  for(i=0; i<size; i++)
  {
    ret = message_send(g_msg_snd_closure,
		       operation,
		       NIL,
		       VALUE1_SELECTOR,
		       convert_int_to_object(1),
		       convert_char_to_object(str[i]),
		       g_idclo);

    if(call_chain_entry_exists(entry))
      continue;
    else
      break;
  }

  if(call_chain_entry_exists(entry))
  {
    pop_if_top(entry);
    return invoke_cont_on_val(cont, receiver);
  }
  else
    return ret;
}

OBJECT_PTR readable_string_do_separated_by(OBJECT_PTR closure, OBJECT_PTR operation, OBJECT_PTR separator, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  OBJECT_PTR ret1, ret2;

  BOOLEAN ret_from_do, ret_from_separated_by;

  for(i=0; i<size; i++)
  {
    ret1 = message_send(g_msg_snd_closure,
			operation,
			NIL,
			VALUE1_SELECTOR,
			convert_int_to_object(1),
			convert_char_to_object(str[i]),
			g_idclo);

    if(!call_chain_entry_exists(entry))
    {
      ret_from_do = true;
      break;
    }

    if(i < size - 1)
    {
      ret2 = message_send(g_msg_snd_closure,
			  separator,
			  NIL,
			  VALUE_SELECTOR,
			  convert_int_to_object(0),
			  g_idclo);

      if(!call_chain_entry_exists(entry))
      {
	ret_from_separated_by = true;
	break;
      }
    }
  }

  if(call_chain_entry_exists(entry))
  {
    pop_if_top(entry);
    return invoke_cont_on_val(cont, receiver);
  }
  else
  {
    if(ret_from_do)
      return ret1;
    else if(ret_from_separated_by)
      return ret2;
    else
      assert(false);
  }
}

OBJECT_PTR readable_string_select(OBJECT_PTR closure, OBJECT_PTR discriminator, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  unsigned int count = 0;

  //this is inefficient: we loop twice
  //through the string elements. but
  //this obviates the complexity of
  //dynamically expanding the return string

  for(i=0; i<size; i++)
  {
    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  discriminator,
				  NIL,
				  VALUE1_SELECTOR,
				  convert_int_to_object(1),
				  convert_char_to_object(str[i]),
				  g_idclo);

    if(ret == TRUE)
      count++;
  }

  char *ret_str = (char *)GC_MALLOC((count+1) * sizeof(char));

  int j = 0;

  for(i=0; i<size; i++)
  {
    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  discriminator,
				  NIL,
				  VALUE1_SELECTOR,
				  convert_int_to_object(1),
				  convert_char_to_object(str[i]),
				  g_idclo);

    if(ret == TRUE)
    {
      ret_str[j] = str[i];
      j++;
    }
  }

  ret_str[count] = '\0';

  pop_if_top(entry);

  return invoke_cont_on_val(cont, get_string_obj(ret_str));
}

OBJECT_PTR readable_string_reject(OBJECT_PTR closure, OBJECT_PTR discriminator, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  unsigned int count = 0;

  //this is inefficient: we loop twice
  //through the string elements. but
  //this obviates the complexity of
  //dynamically expanding the return string

  for(i=0; i<size; i++)
  {
    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  discriminator,
				  NIL,
				  VALUE1_SELECTOR,
				  convert_int_to_object(1),
				  convert_char_to_object(str[i]),
				  g_idclo);

    if(ret == FALSE)
      count++;
  }

  char *ret_str = (char *)GC_MALLOC((count+1) * sizeof(char));

  int j = 0;
  
  for(i=0; i<size; i++)
  {
    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  discriminator,
				  NIL,
				  VALUE1_SELECTOR,
				  convert_int_to_object(1),
				  convert_char_to_object(str[i]),
				  g_idclo);

    if(ret == FALSE)
    {
      ret_str[j] = str[i];
      j++;
    }
  }

  ret_str[count] = '\0';
  
  pop_if_top(entry);

  return invoke_cont_on_val(cont, get_string_obj(ret_str));
}

OBJECT_PTR readable_string_occurrences_of(OBJECT_PTR closure, OBJECT_PTR target, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  int count = 0;

  for(i=0; i<size; i++)
  {
    if(str[i] == get_char_value(target))
      count++;
  }

  pop_if_top(entry);

  return invoke_cont_on_val(cont, convert_int_to_object(count));
}

OBJECT_PTR readable_string_includes(OBJECT_PTR closure, OBJECT_PTR target, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  OBJECT_PTR ret = FALSE;

  for(i=0; i<size; i++)
  {
    if(str[i] == get_char_value(target))
    {
      ret = TRUE;
      break;
    }
  }

  pop_if_top(entry);

  return invoke_cont_on_val(cont, ret);
}

OBJECT_PTR readable_string_detect_if_none(OBJECT_PTR closure,
					  OBJECT_PTR discriminator,
					  OBJECT_PTR exception_handler,
					  OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  for(i=0; i<size; i++)
  {
    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  discriminator,
				  NIL,
				  VALUE1_SELECTOR,
				  convert_int_to_object(1),
				  convert_char_to_object(str[i]),
				  g_idclo);

    if(ret == TRUE)
    {
      pop_if_top(entry);
      return invoke_cont_on_val(cont, convert_char_to_object(str[i]));
    }
  }

  OBJECT_PTR ret1 = message_send(g_msg_snd_closure,
				 exception_handler,
				 NIL,
				 VALUE_SELECTOR,
				 convert_int_to_object(0),
				 g_idclo);

  pop_if_top(entry);

  return invoke_cont_on_val(cont, ret1);
}

OBJECT_PTR readable_string_detect(OBJECT_PTR closure,
				  OBJECT_PTR discriminator,
				  OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  for(i=0; i<size; i++)
  {
    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  discriminator,
				  NIL,
				  VALUE1_SELECTOR,
				  convert_int_to_object(1),
				  convert_char_to_object(str[i]),
				  g_idclo);

    if(ret == TRUE)
    {
      pop_if_top(entry);
      return invoke_cont_on_val(cont, convert_char_to_object(str[i]));
    }
  }

  pop_if_top(entry);

  return invoke_cont_on_val(cont, NIL);
}

OBJECT_PTR readable_string_collect(OBJECT_PTR closure, OBJECT_PTR transformer, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  int i;

  char *ret_str = (char *)GC_MALLOC((size+1) * sizeof(char));

  for(i=0; i<size; i++)
  {
    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  transformer,
				  NIL,
				  VALUE1_SELECTOR,
				  convert_int_to_object(1),
				  convert_char_to_object(str[i]),
				  g_idclo);

    if(!(IS_CHARACTER_OBJECT(ret)))
      return create_and_signal_exception_with_text(Error, get_string_obj("Transformer returns non-character"), cont);

    ret_str[i] = get_char_value(ret);
  }

  ret_str[size] = '\0';

  pop_if_top(entry);

  return invoke_cont_on_val(cont, get_string_obj(ret_str));
}

OBJECT_PTR readable_string_substring(OBJECT_PTR closure, OBJECT_PTR start, OBJECT_PTR end, OBJECT_PTR cont)
{
  OBJECT_PTR receiver = car(get_binding_val(g_top_level, SELF));

  call_chain_entry_t *entry = (call_chain_entry_t *)stack_top(g_call_chain);

  char *str = g_string_literals[receiver >> OBJECT_SHIFT];

  unsigned int size = strlen(str);

  if(!IS_INTEGER_OBJECT(start) || !IS_INTEGER_OBJECT(end))
    create_and_signal_exception(InvalidArgument, cont);

  int start_int, end_int;
  start_int = get_int_value(start);
  end_int = get_int_value(end);

  if(end < start)
    create_and_signal_exception_with_text(Error, get_string_obj("End index less than start index"), cont);

  unsigned int substr_size = end_int - start_int + 1;

  char *ret_str = (char *)GC_MALLOC((substr_size+1) * sizeof(char));

  int i;

  for(i=0; i<substr_size; i++)
    ret_str[i] = str[i+start_int-1];

  ret_str[substr_size] = '\0';

  return invoke_cont_on_val(cont, get_string_obj(ret_str));
}

void create_ReadableString()
{
  class_object_t *cls_obj;

  if(allocate_memory((void **)&cls_obj, sizeof(class_object_t)))
  {
    printf("create_ReadableString: Unable to allocate memory\n");
    exit(1);
  }

  cls_obj->parent_class_object = Object;
  cls_obj->name = GC_strdup("ReadableString");

  cls_obj->nof_instances = 0;
  cls_obj->instances = NULL;

  cls_obj->nof_instance_vars = 0;
  cls_obj->inst_vars = NULL;

  cls_obj->shared_vars = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->shared_vars->count = 0;

  cls_obj->instance_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_t));
  cls_obj->instance_methods->count = 13;
  cls_obj->instance_methods->bindings = (binding_t *)GC_MALLOC(cls_obj->instance_methods->count * sizeof(binding_t));

  cls_obj->instance_methods->bindings[0].key = get_symbol("_size");
  cls_obj->instance_methods->bindings[0].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_size),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[1].key = get_symbol("_isEmpty");
  cls_obj->instance_methods->bindings[1].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_is_empty),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[2].key = get_symbol("_notEmpty");
  cls_obj->instance_methods->bindings[2].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_not_empty),
						    NIL, NIL,
						    0, NIL, NULL);

  cls_obj->instance_methods->bindings[3].key = get_symbol("_do:");
  cls_obj->instance_methods->bindings[3].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_do),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[4].key = get_symbol("_do:separatedBy:");
  cls_obj->instance_methods->bindings[4].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_do_separated_by),
						    NIL, NIL,
						    2, NIL, NULL);

  cls_obj->instance_methods->bindings[5].key = get_symbol("_select:");
  cls_obj->instance_methods->bindings[5].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_select),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[6].key = get_symbol("_reject:");
  cls_obj->instance_methods->bindings[6].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_reject),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[7].key = get_symbol("_occurrencesOf:");
  cls_obj->instance_methods->bindings[7].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_occurrences_of),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[8].key = get_symbol("_includes:");
  cls_obj->instance_methods->bindings[8].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_includes),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[9].key = get_symbol("_detect:ifNone:");
  cls_obj->instance_methods->bindings[9].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_detect_if_none),
						    NIL, NIL,
						    2, NIL, NULL);

  cls_obj->instance_methods->bindings[10].key = get_symbol("_detect:");
  cls_obj->instance_methods->bindings[10].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_detect),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[11].key = get_symbol("_collect:");
  cls_obj->instance_methods->bindings[11].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_collect),
						    NIL, NIL,
						    1, NIL, NULL);

  cls_obj->instance_methods->bindings[12].key = get_symbol("_substringFrom:to:");
  cls_obj->instance_methods->bindings[12].val = create_method(cls_obj, false,
						    convert_native_fn_to_object((nativefn)readable_string_substring),
						    NIL, NIL,
						    2, NIL, NULL);

  cls_obj->class_methods = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));
  cls_obj->class_methods->count = 0;
  cls_obj->class_methods->bindings = NULL;

  ReadableString =  convert_class_object_to_object_ptr(cls_obj);
}
