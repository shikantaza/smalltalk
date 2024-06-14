#include "../smalltalk.h"

extern OBJECT_PTR NIL;
extern OBJECT_PTR TRUE;
extern OBJECT_PTR FALSE;

//functions for implementing the <Object> protocol

OBJECT_PTR get_parent_class(OBJECT_PTR cls)
{
  return ((class_object_t *)extract_ptr(cls))->parent_class_object;
}

//=
OBJECT_PTR single_equals(OBJECT_PTR receiver, OBJECT_PTR comparand)
{
  //we define two objects to be equivalent if they
  //are structurally equal

  if(receiver == comparand)
    return TRUE;
  else if(IS_CONS_OBJECT(receiver) && IS_CONS_OBJECT(comparand))
  {
    return single_equals(car(receiver), car(comparand)) &&
      single_equals(cdr(receiver), cdr(comparand));
  }
  //symbols will be handled in the if clause itself  
  //TO DO: array objects, floats; 
  else
    return FALSE;

  object_t *recv = (object_t *)extract_ptr(receiver);
  object_t *comp = (object_t *)extract_ptr(comparand);

  if(recv->nof_instance_vars != comp->nof_instance_vars)
    return FALSE;

  unsigned int i;

  unsigned int n = recv->nof_instance_vars;

  //note: we assume the instance variables bindings
  //are in the same order in both the objects (i.e.,
  //if the bindings are the same, but in a different
  //key-order, the equivalence check will fail)
  for(i=0; i<n; i++)
    if(!single_equals(recv->instance_vars[i].key,
                      comp->instance_vars[i].key) ||
       !single_equals(recv->instance_vars[i].val,
                      comp->instance_vars[i].val))
      return FALSE;

  return TRUE;
}

//==
OBJECT_PTR double_equals(OBJECT_PTR receiver, OBJECT_PTR comparand)
{
  return receiver == comparand ? TRUE : FALSE;
}

//~=
OBJECT_PTR not_single_equals(OBJECT_PTR receiver, OBJECT_PTR comparand)
{
  return single_equals(receiver, comparand) == TRUE ? FALSE : TRUE;
}

//~~
OBJECT_PTR not_double_equals(OBJECT_PTR receiver, OBJECT_PTR comparand)
{
  return receiver == comparand ? FALSE : TRUE;
}

OBJECT_PTR class(OBJECT_PTR receiver)
{
  OBJECT_PTR recv_obj = extract_ptr(receiver);
  OBJECT_PTR recv_class_obj = ((object_t *)recv_obj)->class_object;

  return recv_class_obj;
}

OBJECT_PTR copy(OBJECT_PTR receiver)
{
  return clone_object(receiver);
}

OBJECT_PTR does_not_understand(OBJECT_PTR receiver, OBJECT_PTR failed_message)
{
  signal_exception(MessageNotUnderstood, message_send(failed_message, selector("getSelector")));
  return NIL; //TODO: should return the resumption value of the exception if it resumes
}

OBJECT_PTR error1(OBJECT_PTR receiver, OBJECT_PTR signalerText)
{
  signal_exception(Error, message_send(signalerText, selector("asString")));
  return NIL;
}

OBJECT_PTR hash(OBJECT_PTR receiver)
{
  //TODO
  return get_int_value(receiver);
}

OBJECT_PTR identity_hash(OBJECT_PTR receiver)
{
  //TODO:
  return get_int_value(receiver);
}

OBJECT_PTR is_kind_of(OBJECT_PTR receiver, OBJECT_PTR candidate_class)
{
  if(candidate_class == Object)
    return TRUE;

  OBJECT_PTR class_obj = get_class_object(receiver);

  OBJECT_PTR cls = candidate_class;
  
  while(cls != Object)
  {
    if(class_obj == cls)
      return TRUE;

    cls = get_parent_class(cls);
  }

  return FALSE;
}

OBJECT_PTR is_member_of(OBJECT_PTR receiver, OBJECT_PTR candidate_class)
{
  return get_class_object(receiver) == candidate_class ? TRUE : FALSE;
}

OBJECT_PTR is_nil(OBJECT_PTR receiver)
{
  return receiver == NIL ? TRUE : FALSE;
}

OBJECT_PTR is_not_nil(OBJECT_PTR receiver)
{
  return receiver == NIL ? FALSE : TRUE;
}

OBJECT_PTR perform(OBJECT_PTR receiver, OBJECT_PTR selector)
{
  return message_send(receiver, selector);
}

OBJECT_PTR perform_with1(OBJECT_PTR receiver,
                         OBJECT_PTR selector,
                         OBJECT_PTR arg1)
{
  return message_send(receiver, selector, arg1);
}

OBJECT_PTR perform_with2(OBJECT_PTR receiver,
                         OBJECT_PTR selector,
                         OBJECT_PTR arg1,
                         OBJECT_PTR arg2)
{
  return message_send(receiver, selector, arg1, arg2);
}

OBJECT_PTR perform_with3(OBJECT_PTR receiver,
                         OBJECT_PTR selector,
                         OBJECT_PTR arg1,
                         OBJECT_PTR arg2,
                         OBJECT_PTR arg3)
{
  return message_send(receiver, selector, arg1, arg2, arg3);
}

OBJECT_PTR perform_with_args(OBJECT_PTR receiver, OBJECT_PTR selector, OBJECT_PTR args)
{
  //TODO: use pLisp's FFI infrastructure to implement this
  return NIL;
}

OBJECT_PTR print_on(OBJECT_PTR receiver, OBJECT_PTR target)
{
  return message_send(target, selector("nextPutAll"), receiver);
}

OBJECT_PTR printString(OBJECT_PTR receiver)
{
  return get_string_object(get_class_object(receiver)->name);
}

OBJECT_PTR responds_to(OBJECT_PTR receiver, OBJECT_PTR selector)
{
  class_object_t *cls_obj = (class_object_t *)extract_ptr(get_class_object(receiver));

  unsigned int n = cls_obj->nof_instance_methods;
  int i;

  for(i=0; i<n; i++)
  {
    if(cls_obj->instance_methods[i].key == selector)
      return TRUE;
  }

  return FALSE;
}

OBJECT_PTR yourself(OBJECT_PTR receiver)
{
  return receiver;
}

OBJECT_PTR create_Object()
{
  class_object_t *cls_obj = (class_object_t *)GC_MALLOC(sizeof(class_object_t));
  cls_obj->name = GC_strdup("Object");
  cls_obj->nof_shared_vars = 0;
  cls_obj->shared_vars = NULL;
  cls_obj->nof_instance_methods = 23;  
  
  cls_obj->instance_methods = (binding_t *)GC_MALLOC(cls_obj->nof_instance_methods *
                                                     sizeof(binding_t));

  cls_obj->instance_methods[0].key = get_symbol("=");
  cls_obj->instance_methods[1].key = get_symbol("==");
  cls_obj->instance_methods[2].key = get_symbol("~=");
  cls_obj->instance_methods[3].key = get_symbol("~~");
  cls_obj->instance_methods[4].key = get_symbol("class");
  cls_obj->instance_methods[5].key = get_symbol("copy");
  cls_obj->instance_methods[6].key = get_symbol("doesNotUnderstand:");
  cls_obj->instance_methods[7].key = get_symbol("error:");
  cls_obj->instance_methods[8].key = get_symbol("hash");
  cls_obj->instance_methods[9].key = get_symbol("identityHash");
  cls_obj->instance_methods[10].key = get_symbol("isKindOf:");
  cls_obj->instance_methods[11].key = get_symbol("isMemberOf:");
  cls_obj->instance_methods[12].key = get_symbol("isNil");
  cls_obj->instance_methods[13].key = get_symbol("notNil");
  cls_obj->instance_methods[14].key = get_symbol("perform:");
  cls_obj->instance_methods[15].key = get_symbol("perform:with");
  cls_obj->instance_methods[16].key = get_symbol("perform:with:with:");
  cls_obj->instance_methods[17].key = get_symbol("perform:with:with:with:");
  cls_obj->instance_methods[18].key = get_symbol("perform:withArguments:");
  cls_obj->instance_methods[19].key = get_symbol("printOn:");
  cls_obj->instance_methods[20].key = get_symbol("printString");
  cls_obj->instance_methods[21].key = get_symbol("respondsTo:");
  cls_obj->instance_methods[22].key = get_symbol("yourself");

  cls_obj->instance_methods[0].val = convert_native_fn_to_object(single_equals);
  cls_obj->instance_methods[1].val = convert_native_fn_to_object(double_equals);
  cls_obj->instance_methods[2].val = convert_native_fn_to_object(not_single_equals);
  cls_obj->instance_methods[3].val = convert_native_fn_to_object(not_double_equals);
  cls_obj->instance_methods[4].val = convert_native_fn_to_object(class);
  cls_obj->instance_methods[5].val = convert_native_fn_to_object(copy);
  cls_obj->instance_methods[6].val = convert_native_fn_to_object(does_not_understand);
  cls_obj->instance_methods[7].val = convert_native_fn_to_object(error1);
  cls_obj->instance_methods[8].val = convert_native_fn_to_object(hash);
  cls_obj->instance_methods[9].val = convert_native_fn_to_object(identity_hash);
  cls_obj->instance_methods[10].val = convert_native_fn_to_object(is_kind_of);
  cls_obj->instance_methods[11].val = convert_native_fn_to_object(is_member_of);
  cls_obj->instance_methods[12].val = convert_native_fn_to_object(is_nil);
  cls_obj->instance_methods[13].val = convert_native_fn_to_object(is_not_nil);
  cls_obj->instance_methods[14].val = convert_native_fn_to_object(perform);
  cls_obj->instance_methods[15].val = convert_native_fn_to_object(perform_with1);
  cls_obj->instance_methods[16].val = convert_native_fn_to_object(perform_with2);
  cls_obj->instance_methods[17].val = convert_native_fn_to_object(perform_with3);
  cls_obj->instance_methods[18].val = convert_native_fn_to_object(perform_with_args);
  cls_obj->instance_methods[19].val = convert_native_fn_to_object(print_on);
  cls_obj->instance_methods[20].val = convert_native_fn_to_object(print_string);
  cls_obj->instance_methods[21].val = convert_native_fn_to_object(responds_to);
  cls_obj->instance_methods[22].val = convert_native_fn_to_object(yourself);
  
  cls_obj->nof_class_methods = 0;
  cls_obj->class_methods = NULL;

  return convert_class_object_to_object_ptr(cls_obj);
}

OBJECT_PTR get_inst_var(OBJECT_PTR obj, OBJECT_PTR var_name)
{
  object_t obj1 = (object_t *)extract_ptr(obj);

  return get_binding_val(obj1->instance_vars, var_name);
}
