#include <stding.h>

#include "gc.h"

#include "../smalltalk.h"

OBJECT_PTR create_string_object(char *s)
{
  size_t i;
  size_t len = strlen(s);
  
  object_t *str_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  str_obj->nof_instance_vars = 1;
  str_obj->instance_vars = (binding_t *)GC_MALLOC(str_obj->nof_instance_vars * sizeof(binding_t));
  str_obj->instance_vars[0].key = NIL;

  array_object_t *arr = (array_object_ *)GC_MALLOC(sizeof(array_object_t));
  arr->nof_elements = len;
  arr->elements = (OBJECT_PTR *)GC_MALLOC(arr_nof_elements * sizeof(OBJECT_PTR));

  for(i=0; i<n; i++)
    arr->elements[i] = convert_char_to_object(s[i]);

  str_obj->instance_vars[0].val = convert_array_object_to_object_ptr(arr);

  return (OBJECT_PTR)str_obj + STRING_TAG;
}
