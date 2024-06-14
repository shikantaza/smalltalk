#include <string.h>

#include "../smalltalk.h"

extern OBJECT_PTR NIL;
extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

//functions for implementing the <symbol> protocol

//methods from <collection> protocol

OBJECT_PTR all_satisfy(OBJECT_PTR receiver, OBJECT_PTR discriminator)
{
  char *s = get_symbol_name(receiver);

  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
    //TODO: handle exception from message_send
    if(message_send(discriminator, selector("value"), convert_char_to_object(s[i])) == FALSE)
      return FALSE;

  return TRUE;
}

OBJECT_PTR any_satisfy(OBJECT_PTR receiver, OBJECT_PTR discriminator)
{
  char *s = get_symbol_name(receiver);

  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
    //TODO: handle exception from message_send    
    if(message_send(discriminator, selector("value"), convert_char_to_object(s[i])) == TRUE)
      return TRUE;

  return FALSE;
}

OBJECT_PTR as_array(OBJECT_PTR receiver)
{
  char *s = get_symbol_name(receiver);
  size_t len = strlen(s);
  size_t i;

  array_object_t *arr = (array_object_ *)GC_MALLOC(sizeof(array_object_t));
  arr->nof_elements = len;
  arr->elements = (OBJECT_PTR *)GC_MALLOC(arr_nof_elements * sizeof(OBJECT_PTR));

  for(i=0; i<n; i++)
    arr->elements[i] = convert_char_to_object(s[i]);

  return convert_array_object_to_object_ptr(arr);
}

OBJECT_PTR as_bag(OBJECT_PTR receiver)
{
  //we represent bags internally as array objects, with the <Bag>
  //protocol implementation using the array_object_t structure.
  //it will be possible to exploit the structure to do non-Bag
  //things, but this shouldn't be a concern.

  object_t *bag_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  bag_obj->class_object = Bag;
  bag_obj->nof_instance_vars = 1;
  bag_obj->instance_vars = (binding_t *)GC_MALLOC(bag_obj->nof_instance_vars * sizeof(binding_t));

  //not needed, as we are not expected to refer to the instance object by name,
  //but object equivalence will fail otherwise
  bag_obj->instance_vars[0].key = NIL;
  
  bag_obj->instance_vars[0].val = as_array(receiver);
  
  return bag_obj;
}

OBJECT_PTR as_byte_array(OBJECT_PTR receiver)
{
  return as_array(receiver);
}

OBJECT_PTR as_ordered_collection(OBJECT_PTR receiver)
{
  //repurposing array objects for this
  
  object_t *ordered_coll_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  ordered_coll_obj->class_object = OrderedCollection;
  ordered_coll_obj->nof_instance_vars = 1;
  ordered_coll_obj->instance_vars = (binding_t *)GC_MALLOC(ordered_coll_obj->nof_instance_vars *
                                                           sizeof(binding_t));

  //not needed, as we are not expected to refer to the instance object by name,
  //but object equivalence will fail otherwise
  ordered_coll_obj->instance_vars[0].key = NIL;

  ordered_coll_obj->instance_vars[0].val = as_array(receiver);
  
  return ordered_coll_obj;
}

OBJECT_PTR as_set(OBJECT_PTR receiver)
{
  //we are using a list to represent the set's contents
  
  OBJECT_PTR ret = NIL;

  char *s = get_symbol_name(receiver);
  size_t len = strlen(s);
  size_t i, j;

  for(i=0; i<len; i++)
  {
    for(j=0; j<i; j++)
      if(s[j] == s[i])
        break;

    if(i == j)
      ret = cons(convert_char_to_object(s[j]), ret);
  }

  object_t *set_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  set_obj->class_object = Set;
  set_obj->nof_instance_vars = 1;
  set_obj->instance_vars = (binding_t *)GC_MALLOC(set_obj->nof_instance_vars * sizeof(binding_t));

  //not needed, as we are not expected to refer to the instance object by name,
  //but object equivalence will fail otherwise
  set_obj->instance_vars[0].key = NIL;

  set_obj->instance_vars[0].val = reverse(ret);
  
  return set_obj;
}

OBJECT_PTR as_sorted_collection(OBJECT_PTR receiver)
{
  char *s1 = GC_strdup(get_symbol_name(receiver));

  sort(s1);

  object_t *sorted_coll_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  sorted_coll_obj->class_object = OrderedCollection;
  sorted_coll_obj->nof_instance_vars = 1;
  sorted_coll_obj->instance_vars = (binding_t *)GC_MALLOC(sorted_coll_obj->nof_instance_vars *
                                                          sizeof(binding_t));

  //not needed, as we are not expected to refer to the instance object by name,
  //but object equivalence will fail otherwise
  sorted_coll_obj->instance_vars[0].key = NIL;

  sorteded_coll_obj->instance_vars[0].val = as_array(get_symbol(s1));
  
  return sorted_coll_obj;
}

OBJECT_PTR as_sorted_collection_with_sort_block(OBJECT_PTR receiver, OBJECT_PTR sort_block)
{
  char *s1 = GC_strdup(get_symbol_name(receiver));

  sort_with_block(s1, sort_block);

  object_t *sorted_coll_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  sorted_coll_obj->class_object = OrderedCollection;
  sorted_coll_obj->nof_instance_vars = 1;
  sorted_coll_obj->instance_vars = (binding_t *)GC_MALLOC(sorted_coll_obj->nof_instance_vars *
                                                          sizeof(binding_t));

  //not needed, as we are not expected to refer to the instance object by name,
  //but object equivalence will fail otherwise
  sorted_coll_obj->instance_vars[0].key = NIL;

  sorteded_coll_obj->instance_vars[0].val = as_array(get_symbol(s1));
  
  return sorted_coll_obj;  
}

OBJECT_PTR collect(OBJECT_PTR receiver, OBJECT_PTR transformer)
{
  char *s = GC_strdup(get_symbol_name(receiver));

  size_t len = strlen(s);
  size_t i;

  object_t *coll_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  coll_obj->class_object = OrderedCollection;
  coll_obj->nof_instance_vars = 1;
  coll_obj->instance_vars = (binding_t *)GC_MALLOC(coll_obj->nof_instance_vars *
                                                   sizeof(binding_t));

  //not needed, as we are not expected to refer to the instance object by name,
  //but object equivalence will fail otherwise
  coll_obj->instance_vars[0].key = NIL;

  array_object_t *arr = (array_object_ *)GC_MALLOC(sizeof(array_object_t));
  arr->nof_elements = len;
  arr->elements = (OBJECT_PTR *)GC_MALLOC(arr_nof_elements * sizeof(OBJECT_PTR));

  for(i=0; i<len; i++)
  {
    //TODO: handle exception from message_send
    arr->elements[i] = = message_send(transformer, selector("value"), convert_char_to_object(s[i]));
  }
  
  coll_obj->instance_vars[0].val = arr;
  
  return coll_obj;  
}

OBJECT_PTR detect(OBJECT_PTR receiver, OBJECT_PTR discriminator)
{
  char *s = GC_strdup(get_symbol_name(receiver));

  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
  {
    OBJECT_PTR obj = convert_char_to_object(s[i]);
    //TODO: handle exception from message_send
    if(message_send(discriminator, selector("value:"), obj) == TRUE)
      return obj;
  }
  
  return NIL;
}

OBJECT_PTR detect_if_none(OBJECT_PTR receiver, OBJECT_PTR discriminator, OBJECT_PTR exception_handler)
{
  char *s = get_symbol_name(receiver);

  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
  {
    OBJECT_PTR obj = convert_char_to_object(s[i]);
    //TODO: handle exception from message_send
    if(message_send(discriminator, selector("value:"), obj) == TRUE)
      return obj;
  }

  //TODO: handle exception from message_send
  return message_send(exception_handler, "value");
}

OBJECT_PTR do_operation(OBJECT_PTR receiver, OBJECT_PTR operation)
{
  char *s = get_symbol_name(receiver);

  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
    //TODO: handle exception from message_send
    message_send(operation, "value:", convert_char_to_object(s[i]));

  return NIL;
}

OBJECT_PTR do_operation_separated_by(OBJECT_PTR receiver, OBJECT_PTR operation, OBJECT_PTR separator)
{
  char *s = get_symbol_name(receiver);

  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
  {
    if(i != 0 && i != (len-1))
      //TODO: handle exception from message_send
      message_send(separator, selector("value"));
    
    //TODO: handle exception from message_send
    message_send(operation, "value:", convert_char_to_object(s[i]));
  }
  
  return NIL;
}

OBJECT_PTR includes(OBJECT_PTR receiver, OBJECT_PTR target)
{
  char *s = get_symbol_name(receiver);

  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
    if(single_equals(convert_char_to_object(s[i]), target))
      return TRUE;

  return FALSE;
}

OBJECT_PTR inject_into(OBJECT_PTR receiver, OBJECT_PTR initial_value, OBJECT_PTR operation)
{
  char *s = get_symbol_name(receiver);

  size_t len = strlen(s);
  size_t i;

  OBJECT_PTR val = initial_value;
  
  for(i=0; i<len; i++)
    val = message_send(operation, selector("value:value:"), val, convert_char_to_object(s[i]));

  return val;
}

OBJECT_PTR is_empty(OBJECT_PTR receiver)
{
  if(strlen(get_symbol_name(receiver)) == 0)
    return TRUE;
  else
    return FALSE;
}

OBJECT_PTR not_empty(OBJECT_PTR receiver)
{
  if(strlen(get_symbol_name(receiver)) == 0)
    return FALSE;
  else
    return TRUE;
}

OBJECT_PTR occurrences_of(OBJECT_PTR receiver, OBJECT_PTR target)
{
  char *s = get_symbol_name(receiver);

  size_t len = strlen(s);
  size_t i;

  int val = 0
  
  for(i=0; i<len; i++)
    if(single_equals(target, convert_char_to_object(s[i])))
      val++;

  return convert_int_to_object(val);
}

OBJECT_PTR rehash(OBJECT_PTR receiver)
{
  //TODO
  return NIL;
}

OBJECT_PTR reject(OBJECT_PTR receiver, OBJECT_PTR discriminator)
{
  OBJECT_PTR ret = NIL;

  char *s = get_symbol_name(receiver);
  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
  {
    OBJECT_PTR obj = convert_char_to_object(s[i]);

    if(message_send(discriminator, selector("value:"), obj) == FALSE)
      ret = cons(obj,ret);
  }

  ret = reverse(ret);

  size_t len1 = cons_length(ret);

  object_t *coll_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  coll_obj->class_object = Collection;
  coll_obj->nof_instance_vars = 1;
  coll_obj->instance_vars = (binding_t *)GC_MALLOC(coll_obj->nof_instance_vars *
                                                   sizeof(binding_t));

  //not needed, as we are not expected to refer to the instance object by name,
  //but object equivalence will fail otherwise
  coll_obj->instance_vars[0].key = NIL;

  array_object_t *arr = (array_object_ *)GC_MALLOC(sizeof(array_object_t));
  arr->nof_elements = len1;
  arr->elements = (OBJECT_PTR *)GC_MALLOC(arr->nof_elements * sizeof(OBJECT_PTR));  

  for(i=0; i<len1; i++)
  {
    arr->elements[i] = car(ret);
    ret = cdr(ret);
  }

  coll_obj->instance_vars[0].val = arr;

  return coll_obj;
  
}

//TODO: refactor to avoid code duplication with reject(),
//after we ensure that dispatching from message_send
//will not pose any problems
OBJECT_PTR select(OBJECT_PTR receiver, OBJECT_PTR discriminator)
{
  OBJECT_PTR ret = NIL;

  char *s = get_symbol_name(receiver);
  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
  {
    OBJECT_PTR obj = convert_char_to_object(s[i]);

    if(message_send(discriminator, selector("value:"), obj) == TRUE)
      ret = cons(obj,ret);
  }

  ret = reverse(ret);

  size_t len1 = cons_length(ret);

  object_t *coll_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  coll_obj->class_object = Collection;
  coll_obj->nof_instance_vars = 1;
  coll_obj->instance_vars = (binding_t *)GC_MALLOC(coll_obj->nof_instance_vars *
                                                   sizeof(binding_t));

  //not needed, as we are not expected to refer to the instance object by name,
  //but object equivalence will fail otherwise
  coll_obj->instance_vars[0].key = NIL;

  array_object_t *arr = (array_object_ *)GC_MALLOC(sizeof(array_object_t));
  arr->nof_elements = len1;
  arr->elements = (OBJECT_PTR *)GC_MALLOC(arr->nof_elements * sizeof(OBJECT_PTR));  

  for(i=0; i<len1; i++)
  {
    arr->elements[i] = car(ret);
    ret = cdr(ret);
  }

  coll_obj->instance_vars[0].val = arr;

  return coll_obj;  
}

OBJECT_PTR size(OBJECT_PTR receiver)
{
  return convert_int_to_object(strlen(get_symbol_name(receiver)));
}

//end of methods from <collection> protocol

//methods from <sequencedReadableCollection> protocol

OBJECT_PTR symbol_comma(OBJECT_PTR receiver, OBJECT_PTR operand)
{
  char *s1 = get_symbol_name(receiver);
  char *s2 = get_symbol_name(operand);

  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);

  char *res = (char *)GC_MALLOC((len1+len2+1) * sizeof(char));
  memset(res, '\0', len1 + len2 + 1);
  sprintf(res,"%s%s", s1, s2);

  return get_symbol(res);
}

OBJECT_PTR symbol_single_equals(OBJECT_PTR receiver, OBJECT_PTR comparand)
{
  return single_equals(receiver, comparand);
}

OBJECT_PTR symbol_after(OBJECT_PTR receiver, OBJECT_PTR target)
{
  char *s = get_symbol_name(receiver);
  size_t len = strlen(s);
  size_t i;

  for(i=0; i<len; i++)
    if(single_equals(convert_char_to_object(s[i]), target) == TRUE &&
       i != (len -1))
      return convert_char_to_object(s[i+1]);

  //TODO: raise error
  return NIL;
}
