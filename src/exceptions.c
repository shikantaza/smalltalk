/* these protocols need to be implemented for the
   first cut implementation:

   <exceptionDescription>
     defaultAction
     description
     isResumable
     messageText
     tag

   <signaledException>
     isNested
     outer
     pass
     resignalAs:
     resume
     resume:
     retry
     retryUsing:
     return
     return:
   
   <exceptionSignaler>
     signal
     signal:

   <exceptionBuilder>
     messageText:

   <Exception>
     <no messages, is an abstract class (applications will define subclasses of Exception)

   <Error>
     defaultAction
     isResumable

   <MessageNotUnderstood>
     message
     isResumable
     receiver

   <ZeroDivide>
     dividend
     isResumable

   <failedMessage>
     arguments
     selector

*/

OBJECT_PTR zero_divide_dividend(OBJECT_PTR receiver)
{
  return get_inst_var(receiver, get_symbol("dividend"));  
}

OBJECT_PTR zero_divide_is_resumable(OBJECT_PTR receiver)
{
  return TRUE;  
}

OBJECT_PTR Zero_Divide_dividend(OBJECT_PTR receiver, OBJECT_PTR argument)
{
  object_t *zd_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  zd_obj->nof_instance_vars = 1;
  zd_obj->instance_vars = (binding_t *)GC_MALLOC(sizeof(binding_t));
  zd_obj->instance_vars[0].key = get_symbol("dividend");
  zd_obj->instance_vars[0].val = argument;

  return (OBJECT_PTR)(zd_obj + OBJECT_TAG);
}

OBJECT_PTR failed_msg_arguments(OBJECT_PTR receiver)
{
  return get_inst_var(receiver, get_symbol("arguments"));
}

OBJECT_PTR failed_msg_selector(OBJECT_PTR receiver)
{
  return get_inst_var(receiver, get_symbol("selector"));
}

OBJECT_PTR create_FailedMessage()
{
  class_object_t *cls_obj = (class_object_t *)GC_MALLOC(sizeof(class_object_t));
  cls_obj->name = GC_strdup("FailedMessage");
  cls_obj->nof_shared_vars = 0;
  cls_obj->shared_vars = NULL;
  cls_obj->nof_instance_methods = 2;  
  
  cls_obj->instance_methods = (binding_t *)GC_MALLOC(cls_obj->nof_instance_methods *
                                                     sizeof(binding_t));

  cls_obj->instance_methods[0].key = get_symbol("arguments");
  cls_obj->instance_methods[1].key = get_symbol("selector");

  cls_obj->instance_methods[0].val = convert_native_fn_to_object(failed_msg_arguments);
  cls_obj->instance_methods[1].val = convert_native_fn_to_object(failed_msg_selector);

  return cls_obj;
}

/** <exceptionDescription protocol **/
//defaultAction is implemented in the MessageNotUnderstood class

OBJECT_PTR mnu_description(OBJECT_PTR receiver)
{
  char desc[200];
  memset(desc, '\0', 200);

  char *class_name = (((object_t *)extract_ptr(receiver))->class_object)->name;
  
  sprintf(desc, "MessageNotUnderstood: Objects of class %s do not understand the message %s",
          class_name, get_symbol_name(failed_msg_selector(mnu_message(receiver))));

  return create_string_object(desc);
  
}

//isResumable is implemented in the MessageNotunderstood class

OBJECT_PTR mnu_message_text(OBJECT_PTR receiver)
{
  //no need to incorporate any message provided by the exception signaler,
  //as this is an exception that is signalled internally
  return mnu_description(receiver);
}

OBJECT_PTR mnu_tag(OBJECT_PTR receiver)
{
  //no need to incorporate any tag provided by the exception signaler,
  //as this is an exception that is signalled internally
  return mnu_description(receiver);
}
/** end of <exceptionDescription protocol **/

/** <signaledException> protocol **/

/** end of <signaledException> protocol **/

/** <exceptionBuilder> protocol **/
OBJECT_PTR mnu_message_text(OBJECT_PTR receiver, OBJECT_PTR signaler_text)
{
  set_inst_var(receiver, get_symbol("messageText"), signaler_text);
  return receiver;
}
/** end of <exceptionBuilder protocol **/

/** <exceptionSignaler> protocol **/
OBJECT_PTR mnu_signal(OBJECT_PTR receiver)
{
  
}
/** end of <exceptionSignaler> protocol **/

/** <Error> protocol **/
OBJECT_PTR mnu_default_action(OBJECT_PTR receiver)
{
  printf("Error: MessageNotUnderstood: %s\n", get_symbol_name(failed_msg_selector(mnu_message(receiver))));
  return NIL;
}

// isResumable is defined in the MessageNotunderstood subclass
/** end of <Error> protocol **/

/** <MessageNotUnderstood> protocol **/
OBJECT_PTR mnu_message(OBJECT_PTR receiver)
{
  //TODO: ensure that the receiver, i.e., instance of MessageNotUnderstood,
  //has an instance variable called "failedMessage" when the instance  is constructed
  return get_inst_var(receiver, get_symbol("failedMessage"));
}

OBJECT_PTR mnu_is_resumable(OBJECT_PTR receiver)
{
  return TRUE;
}

OBJECT_PTR mnu_receiver(OBJECT_PTR receiver)
{
  //TODO: ensure that the receiver, i.e., instance of MessageNotUnderstood,
  //has an instance variable called "receiver" when the instance  is constructed  
  return get_inst_var(receiver, get_symbol("receiver"));
}

OBJECT_PTR create_mnu_object()
{
  object_t *mnu_obj = (object_t *)GC_MALLOC(sizeof(object_t));
  mnu_obj->nof_instance_vars = 1;
  mnu_obj->instance_vars = (binding_t *)GC_MALLOC(sizeof(binding_t));
  mnu_obj->instance_vars[0].key = get_symbol("messageText");
  zd_obj->instance_vars[0].val = NIL;

  return (OBJECT_PTR)(mnu_obj + OBJECT_TAG);  
}

OBJECT_PTR create_MessageNotUnderstood()
{
  class_object_t *cls_obj = (class_object_t *)GC_MALLOC(sizeof(class_object_t));
  cls_obj->name = GC_strdup("MessageNotUnderstood");
  cls_obj->nof_shared_vars = 0;
  cls_obj->shared_vars = NULL;
  cls_obj->nof_instance_methods = 3;  
  
  cls_obj->instance_methods = (binding_t *)GC_MALLOC(cls_obj->nof_instance_methods *
                                                     sizeof(binding_t));

  cls_obj->instance_methods[0].key = get_symbol("message");
  cls_obj->instance_methods[1].key = get_symbol("isResumable");
  cls_obj->instance_methods[2].key = get_symbol("receiver");

  cls_obj->instance_methods[0].val = convert_native_fn_to_object(mnu_message);
  cls_obj->instance_methods[1].val = convert_native_fn_to_object(mnu_is_resumable);
  cls_obj->instance_methods[2].val = convert_native_fn_to_object(mnu_receiver);

  return cls_obj;
}
/** end of <MessageNotunderstood> protocol **/


