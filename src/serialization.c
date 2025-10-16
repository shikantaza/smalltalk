#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "gc.h"

#include "queue.h"
#include "hashtable.h"

#include "global_decls.h"

enum PointerType
{
  OBJECT_PTR1,
  NONE, //this is to reset the value of g_sub_type
  ARRAY_OBJ_PTR,
  METHOD_PTR,
  OBJ_PTR,
  CLASS_OBJ_PTR,
  BINDING_ENV_PTR,
  BINDING_PTR,
  EXPRESSION_PTR,
  RETURN_STATEMENT_PTR,
  PRIMARY_PTR,
  MESSAGE_PTR,
  CASCADED_MESSAGES_PTR,
  LITERAL_PTR,
  BLOCK_CONSTRUCTOR_PTR,
  BLOCK_ARGUMENT_PTR,
  BINARY_MESSAGES_PTR,
  BINARY_MESSAGE_PTR,
  KEYWORD_MESSAGE_PTR,
  BINARY_ARGUMENT_PTR,
  KEYWORD_ARGUMENT_PTR,
  KEYWORD_ARGUMENT_PAIR_PTR,
  TEMPORARIES_PTR,
  STATEMENTS_PTR,
  IDENTIFIERS_PTR,
  EXEC_CODE_PTR,
  PACKAGE_PTR,
  STACK_TYPE_PTR,
  EXCEPTION_HANDLER_PTR,
  CALL_CHAIN_ENTRY_PTR,
  DEBUG_EXPRESSION_PTR,
  ASSIGNMENT_PTR,
  BASIC_EXPRESSION_PTR,
  UNARY_MESSAGES_PTR,
  ARRAY_ELEMENTS_PTR,
  ARRAY_ELEMENT_PTR
};

typedef struct
{
  enum PointerType type, sub_type;

  //will be a pointer like method_t, class_object_t, etc.
  void *ref;

  //will be the pointer for a member of the 'ref' pointer
  //(e.g., if 'ref' is an executable_code_t pointer,
  //ptr could be a temporaries_t pointer or a statement_t pointer)
  //void * ptr; //don't think this is required

  //the index position of ptr within the ref pointer's definition
  //(e.g., if 'ref' is an executable pointer, index will be 1
  //if ptr is a temporaries_t pointer and 2 if it is a
  //statement_t pointer)
  //unsigned int index; //don't think this is required
} struct_slot_t;

void print_object_ptr_reference(FILE *, OBJECT_PTR, BOOLEAN);
void print_heap_representation(FILE *, OBJECT_PTR, BOOLEAN);
BOOLEAN is_dynamic_memory_object(OBJECT_PTR);
void hashtable_delete(hashtable_t *);

//TODO: need to reimplement these or
//create the necessary global variable
//(native_fn_src_mapping_t)

nativefn get_nativefn_value(OBJECT_PTR);
char *get_native_fn_source(nativefn);

queue_t *print_queue;
hashtable_t *hashtable, *printed_objects;
unsigned int obj_count = 0;

queue_t *print_queue_struct;
hashtable_t *hashtable_struct, *printed_struct_objects;
unsigned int struct_obj_count = 0;

//global variable that indicates what is the type
//of the pointer that is stored in a stack_type object
enum PointerType g_sub_type;

extern OBJECT_PTR g_message_selector;
extern unsigned int g_nof_string_literals;
extern OBJECT_PTR g_msg_snd_closure;
extern OBJECT_PTR g_msg_snd_super_closure;
extern OBJECT_PTR g_compile_time_method_selector;
extern OBJECT_PTR g_run_till_cont;
extern enum DebugAction g_debug_action;
extern package_t *g_smalltalk_symbols;
extern package_t *g_compiler_package;
extern char **g_string_literals;
extern stack_type *g_exception_environment;
extern stack_type *g_call_chain;
extern stack_type *g_exception_contexts;
extern stack_type *g_breakpointed_methods;
extern BOOLEAN g_loading_core_library;
extern BOOLEAN g_running_tests;
extern OBJECT_PTR g_method_call_stack;
extern OBJECT_PTR g_last_eval_result;
extern BOOLEAN g_system_initialized;
extern enum UIMode g_ui_mode;
extern BOOLEAN g_eval_aborted;
extern executable_code_t *g_exp;
extern binding_env_t *g_top_level;
extern BOOLEAN g_debugger_invoked_for_exception;
extern exception_handler_t *g_active_handler;
extern BOOLEAN g_debug_in_progress;
extern OBJECT_PTR g_debug_cont;
extern stack_type *g_handler_environment;
extern stack_type *g_signalling_environment;

extern OBJECT_PTR NIL;

void add_obj_to_print_list(OBJECT_PTR obj)
{
  //this search is O(n), but this is OK because
  //the queue keeps growing and shrinking, so its
  //size at any point in time is quite small (<10)
  if(obj != NIL && (queue_item_exists(print_queue, (void *)obj) || hashtable_get(printed_objects, (void *)obj)))
    return;

  //assert(is_dynamic_memory_object(obj));
  queue_enqueue(print_queue, (void *)obj);
}

int struct_queue_item_exists(queue_t *q, void *value)
{
  struct_slot_t *s = (struct_slot_t *)value;
  
  queue_item_t *np = q->first;

  while(np != NULL)
  {
    struct_slot_t *s1 = (struct_slot_t *)np->data;
    if(s1->type == s->type &&
       s1->sub_type == s->sub_type
       && s1->ref == s->ref)
      return 1;

    np = np->next;
  }

  return 0;
}

void add_obj_to_print_list_struct(void *struct_ptr, enum PointerType type)
{
  assert(struct_ptr);

  //this search is O(n), but this is OK because
  //the queue keeps growing and shrinking, so its
  //size at any point in time is quite small (<10)
  if(struct_ptr != NULL &&
     (struct_queue_item_exists(print_queue_struct, struct_ptr) ||
      hashtable_get(printed_struct_objects, struct_ptr)))
    return;

  struct_slot_t *s = (struct_slot_t *)GC_MALLOC(sizeof(struct_slot_t));
  s->type = type;
  s->sub_type = g_sub_type;
  s->ref = struct_ptr;
  
  queue_enqueue(print_queue_struct, (void *)s);
}

void print_struct_obj_reference(FILE *fp,
				enum PointerType type,
				void *struct_ptr)
{
  hashtable_entry_t *e = hashtable_get(hashtable_struct, struct_ptr);

  if(e)
    fprintf(fp, "%d", (int)e->value);
  else
  {
    fprintf(fp, "%lu", struct_obj_count);
    hashtable_put(hashtable_struct, struct_ptr, (void *)struct_obj_count);
    struct_obj_count++;
  }

  add_obj_to_print_list_struct(struct_ptr, type);
}

void print_heap_representation_struct(FILE *fp, 
				      void *struct_ptr,
				      enum PointerType type)
{
  unsigned int i;

  if(type == ARRAY_OBJ_PTR)
  {
    array_object_t *arr_obj = (array_object_t *)struct_ptr;

    unsigned count = arr_obj->nof_elements;

    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      print_object_ptr_reference(fp, arr_obj->elements[i], false);
      if(i != count - 1)
	fprintf(fp, ", ");
    }

    fprintf(fp, "] ");
  }
  else if(type == METHOD_PTR)
  {
    method_t *m = (method_t *)struct_ptr;

    /*
    class_object_t *cls_obj;
    BOOLEAN class_method;
    OBJECT_PTR nativefn_obj;
    OBJECT_PTR closed_syms;
    OBJECT_PTR temporaries;
    unsigned int arity;
    OBJECT_PTR code_str;
    executable_code_t *exec_code;
    BOOLEAN breakpointed;
    */

    fprintf(fp, "[ ");
    print_struct_obj_reference(fp, CLASS_OBJ_PTR, (void *)m->cls_obj);

    fprintf(fp, "\"%s\"", (m->class_method == true) ? "true" : "false");
    fprintf(fp, ", ");

    print_object_ptr_reference(fp, m->nativefn_obj, false);
    fprintf(fp, ", ");

    print_object_ptr_reference(fp, m->closed_syms, false);
    fprintf(fp, ", ");

    print_object_ptr_reference(fp, m->temporaries, false);
    fprintf(fp, ", ");

    fprintf(fp, "%d", m->arity);
    fprintf(fp, ", ");
    
    print_object_ptr_reference(fp, m->code_str, false);
    fprintf(fp, ", ");

    print_struct_obj_reference(fp, EXEC_CODE_PTR, (void *)m->exec_code);
    fprintf(fp, ", ");
    
    fprintf(fp, "\"%s\"", (m->breakpointed == true) ? "true" : "false");
    fprintf(fp, "] ");
  }
  else if(type == OBJ_PTR)
  {
    object_t *obj = (object_t *)struct_ptr;

    /*
    OBJECT_PTR class_object;
    binding_env_t *instance_vars; //instance var name, value
    */
    fprintf(fp, "[ ");
    print_object_ptr_reference(fp, obj->class_object, false);
    fprintf(fp, ", ");

    g_sub_type = OBJECT_PTR1;
    print_struct_obj_reference(fp, BINDING_ENV_PTR, (void *)obj->instance_vars);
    g_sub_type = NONE;
    
    fprintf(fp, "] ");
  }
  else if(type == CLASS_OBJ_PTR)
  {
    class_object_t *cls_obj = (class_object_t *)struct_ptr;

    /*
    OBJECT_PTR parent_class_object;
    char *name;

    OBJECT_PTR package;
  
    unsigned int nof_instances;
    OBJECT_PTR *instances;
  
    unsigned int nof_instance_vars;
    OBJECT_PTR *inst_vars;
  
    binding_env_t *shared_vars;
    binding_env_t *instance_methods;
    binding_env_t *class_methods;
    */

    fprintf(fp, "[ ");
    print_object_ptr_reference(fp, cls_obj->parent_class_object, false);
    fprintf(fp, ", ");

    fprintf(fp, "\"%s\"", cls_obj->name);
    fprintf(fp, ", ");
    
    print_object_ptr_reference(fp, cls_obj->package, false);
    fprintf(fp, ", ");

    unsigned int nof_instances = cls_obj->nof_instances;
    
    fprintf(fp, "[ ");
    fprintf(fp, "%d, ", nof_instances);

    fprintf(fp, "[ ");
    for(i=0; i<nof_instances; i++)
    {
      print_object_ptr_reference(fp, cls_obj->instances[i], false);
      if(i != nof_instances - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "]], ");

    unsigned int nof_inst_vars = cls_obj->nof_instance_vars;
    
    fprintf(fp, "[ ");
    fprintf(fp, "%d, ", nof_inst_vars);

    fprintf(fp, "[ ");
    for(i=0; i<nof_inst_vars; i++)
    {
      print_object_ptr_reference(fp, cls_obj->inst_vars[i], false);
      if(i != nof_inst_vars - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "]], ");

    g_sub_type = OBJECT_PTR1;
    print_struct_obj_reference(fp, BINDING_ENV_PTR, (void *)cls_obj->shared_vars);
    g_sub_type = NONE;

    g_sub_type = METHOD_PTR;
    print_struct_obj_reference(fp, BINDING_ENV_PTR, (void *)cls_obj->instance_methods);
    g_sub_type = NONE;

    g_sub_type = METHOD_PTR;
    print_struct_obj_reference(fp, BINDING_ENV_PTR, (void *)cls_obj->class_methods);
    g_sub_type = NONE;

    fprintf(fp, "] ");
  }
  else if(type == BINDING_ENV_PTR)
  {
    binding_env_t *env = (binding_env_t *)struct_ptr;

    /*
    unsigned int count;
    binding_t *bindings;
    */

    fprintf(fp, "[ ");

    unsigned count = env->count;

    for(i=0; i<count; i++)
    {
      fprintf(fp, "[ ");
      print_struct_obj_reference(fp, BINDING_PTR, (void *)(env->bindings+i));
      fprintf(fp, "]");
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");
  }
  else if(type == BINDING_PTR)
  {
    binding_t *binding = (binding_t *)struct_ptr;

    /*
    OBJECT_PTR key;
    OBJECT_PTR val;
    */

    fprintf(fp, "[ ");
    print_object_ptr_reference(fp, binding->key, false);
    fprintf(fp, ", ");
    if(g_sub_type == METHOD_PTR)
    {
      method_t *m = (method_t *)extract_ptr(binding->val);
      print_struct_obj_reference(fp, METHOD_PTR, (void *)m);
    }
    else if(g_sub_type == OBJECT_PTR1 || g_sub_type == NONE)
      print_object_ptr_reference(fp, binding->val, false);
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == EXPRESSION_PTR)
  {
    expression_t *exp = (expression_t *)struct_ptr;

    /*
    enum ExpressionType type;
    struct assignment *asgn;
    struct basic_expression *basic_exp;
    */

    fprintf(fp, "[ ");
    //we don't know the numbers corresponding to the enum values,
    //so we store string descriptions of the types
    if(exp->type == ASSIGNMENT)
    {
      fprintf(fp, "ASSIGNMENT, ");
      print_struct_obj_reference(fp, ASSIGNMENT_PTR, (void *)exp->asgn);
    }
    else if(exp->type == BASIC_EXPRESSION)
    {
      fprintf(fp, "BASIC_EXPRESSION, ");
      print_struct_obj_reference(fp, BASIC_EXPRESSION_PTR, (void *)exp->basic_exp);
    }      
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == RETURN_STATEMENT_PTR)
  {
    return_statement_t *ret_stmt = (return_statement_t *)struct_ptr;

    fprintf(fp, "[ ");
    print_struct_obj_reference(fp, EXPRESSION_PTR, (void *)ret_stmt->exp);
    fprintf(fp, "] ");
  }
  else if(type == PRIMARY_PTR)
  {
    primary_t *prim = (primary_t *)struct_ptr;

    /*
    enum PrimaryType type;
    char *identifier;
    struct literal *lit;
    struct block_constructor *blk_cons;
    struct expression *exp;
    */

    fprintf(fp, "[ ");

    if(prim->type == IDENTIFIER)
    {
      fprintf(fp, "IDENTIFIER, \"%s\"", prim->identifier);
    }
    else if(prim->type == LITERAL)
    {
      fprintf(fp, "LITERAL, ");
      print_struct_obj_reference(fp, LITERAL_PTR, (void *)prim->lit);
    }
    else if(prim->type == BLOCK_CONSTRUCTOR)
    {
      fprintf(fp, "BLOCK_CONSTRUCTOR, ");
      print_struct_obj_reference(fp, BLOCK_CONSTRUCTOR_PTR, (void *)prim->blk_cons);
    }
    else if(prim->type == EXPRESSION1)
    {
      fprintf(fp, "EXPRESSION, ");
      print_struct_obj_reference(fp, EXPRESSION_PTR, (void *)prim->exp);
    }
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == MESSAGE_PTR)
  {
    message_t *msg = (message_t *)struct_ptr;

    /*
    enum MessageType type;
    struct unary_messages *unary_messages;
    struct binary_messages *binary_messages;
    struct keyword_message *kw_msg;
    */

    fprintf(fp, "[ ");

    if(msg->type == UNARY_MESSAGE)
    {
      fprintf(fp, "UNARY_MESSAGE, ");
      print_struct_obj_reference(fp, UNARY_MESSAGES_PTR, (void *)msg->unary_messages);
    }
    else if(msg->type == BINARY_MESSAGE)
    {
      fprintf(fp, "BINARY_MESSAGE, ");
      print_struct_obj_reference(fp, BINARY_MESSAGES_PTR, (void *)msg->binary_messages);
    }
    else if(msg->type == KEYWORD_MESSAGE)
    {
      fprintf(fp, "KEYWORD_MESSAGE, ");
      print_struct_obj_reference(fp, KEYWORD_MESSAGE_PTR, (void *)msg->kw_msg);
    }
    else
      assert(false);
    
    fprintf(fp, "] ");
  }
  else if(type == CASCADED_MESSAGES_PTR)
  {
    cascaded_messages_t *casc_msgs = (cascaded_messages_t *)struct_ptr;

    /*
    unsigned int nof_cascaded_msgs;
    struct message *cascaded_msgs;
    */

    unsigned int count = casc_msgs->nof_cascaded_msgs;

    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      print_struct_obj_reference(fp, MESSAGE_PTR, (void *)(casc_msgs->cascaded_msgs+i));
      if(i != count - 1)
	fprintf(fp, "] ");
    }

    fprintf(fp, "] ");
  }
  else if(type == LITERAL_PTR)
  {
    literal_t *lit = (literal_t *)struct_ptr;

    /*
    enum LiteralType type;
    struct number* num;
    char *val;
    struct array_elements *array_elements;
    */

    fprintf(fp, "[ ");

    if(lit->type == NUMBER_LITERAL)
    {
      fprintf(fp, "NUMBER_LITERAL, ");
      print_struct_obj_reference(fp, LITERAL_PTR, (void *)lit->num);
    }
    else if(lit->type == STRING_LITERAL)
    {
      fprintf(fp, "STRING_LITERAL, ");
      fprintf(fp, "\"%s\"", lit->val);
    }
    else if(lit->type == CHAR_LITERAL)
    {
      fprintf(fp, "CHAR_LITERAL, ");
      fprintf(fp, "\"%s\"", lit->val);
    }
    else if(lit->type == SYMBOL_LITERAL)
    {
      fprintf(fp, "SYMBOL_LITERAL, ");
      fprintf(fp, "\"%s\"", lit->val);
    }
    else if(lit->type == SELECTOR_LITERAL)
    {
      fprintf(fp, "SELECTOR_LITERAL, ");
      fprintf(fp, "\"%s\"", lit->val);
    }
    else if(lit->type == ARRAY_LITERAL)
    {
      fprintf(fp, "ARRAY_LITERAL, ");
      print_struct_obj_reference(fp, ARRAY_ELEMENTS_PTR, (void *)lit->array_elements);
    }
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == BLOCK_CONSTRUCTOR_PTR)
  {
    block_constructor_t *constructor = (block_constructor_t *)struct_ptr;

    /*
    unsigned int type;
    struct block_arguments *block_args;
    struct executable_code *exec_code;
    */
    
    fprintf(fp, "[ ");

    if(constructor->type == BLOCK_ARGS)
    {
      fprintf(fp, "\"BLOCK_ARGS\", ");
      print_struct_obj_reference(fp, BLOCK_ARGUMENT_PTR, (void *)constructor->block_args);
    }
    else if(constructor->type == NO_BLOCK_ARGS)
    {
      fprintf(fp, "\"NO_BLOCK_ARGS\", ");
      print_struct_obj_reference(fp, EXEC_CODE_PTR, (void *)constructor->exec_code);
    }
    else
      assert(false);
    
    fprintf(fp, "] ");
  }
  else if(type == BLOCK_ARGUMENT_PTR)
  {
    block_arguments_t *args = (block_arguments_t *)struct_ptr;

    /*
    unsigned int nof_args;
    char **identifiers;
    */
    unsigned int count = args->nof_args;
   
    fprintf(fp, "[ ");
    for(i=0; i<count; i++)
    {
      fprintf(fp, "\"%s\"", args->identifiers[i]);
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    
    fprintf(fp, "] ");
  }
  else if(type == BINARY_MESSAGES_PTR)
  {
    binary_messages_t *bin_msgs = (binary_messages_t *)struct_ptr;

    /*
    unsigned int nof_messages;
    struct binary_message *bin_msgs;
    */
    unsigned int count = bin_msgs->nof_messages;
   
    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      print_struct_obj_reference(fp, MESSAGE_PTR, (void *)(bin_msgs->bin_msgs+i));
      if(i != count - 1)
	fprintf(fp, ", ");
    }

    fprintf(fp, "] ");
  }
  else if(type == BINARY_MESSAGE_PTR)
  {
    binary_message_t *bin_msg = (binary_message_t *)struct_ptr;

    /*
    char *binary_selector;
    struct binary_argument *bin_arg;
    */
   
    fprintf(fp, "[ ");
    fprintf(fp, "\"%s\", ", bin_msg->binary_selector);
    print_struct_obj_reference(fp, BINARY_ARGUMENT_PTR, (void *)bin_msg->bin_arg);
    fprintf(fp, "] ");
  }
  else if(type == KEYWORD_MESSAGE_PTR)
  {
    keyword_message_t *kw_msg = (keyword_message_t *)struct_ptr;

    /*
    unsigned int nof_args;
    struct keyword_argument_pair *kw_arg_pairs;
    */

    unsigned int count = kw_msg->nof_args;
   
    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      print_struct_obj_reference(fp, KEYWORD_ARGUMENT_PAIR_PTR, (void *)(kw_msg->kw_arg_pairs+i));
      if(i != count - 1)
	fprintf(fp, ", ");
    }

    fprintf(fp, "] ");
  }
  else if(type == BINARY_ARGUMENT_PTR)
  {
    binary_argument_t *bin_arg = (binary_argument_t *)struct_ptr;

    /*
    struct primary *prim;
    struct unary_messages *unary_messages;
    */

    fprintf(fp, "[ ");
    print_struct_obj_reference(fp, PRIMARY_PTR, (void *)bin_arg->prim);
    fprintf(fp, ", ");
    print_struct_obj_reference(fp, UNARY_MESSAGES_PTR, (void *)bin_arg->unary_messages);
    fprintf(fp, "] ");
  }
  else if(type == KEYWORD_ARGUMENT_PAIR_PTR)
  {
    keyword_argument_pair_t *kw_arg_pair = (keyword_argument_pair_t *)struct_ptr;

    /*
    char *keyword;
    struct keyword_argument *kw_arg;
    */
    
    fprintf(fp, "[ ");
    fprintf(fp, "\"%s\", ", kw_arg_pair->keyword);
    print_struct_obj_reference(fp, KEYWORD_ARGUMENT_PTR, (void *)kw_arg_pair->kw_arg);
    fprintf(fp, "] ");
  }
  else if(type == TEMPORARIES_PTR)
  {
    temporaries_t *temps = (temporaries_t *)struct_ptr;

    /*
    unsigned int nof_temporaries;
    char **temporaries;
    */
    
    unsigned int count = temps->nof_temporaries;
   
    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      fprintf(fp, "\"%s\"", temps->temporaries[i]);
      if(i != count - 1)
	fprintf(fp, ", ");
    }

    fprintf(fp, "] ");
  }
  else if(type == STATEMENTS_PTR)
  {
    statement_t *stmt = (statement_t *)struct_ptr;

    /*
    enum StatementType type; 
    struct return_statement *ret_stmt;
    struct expression *exp;
    struct statement *statements;
    */
    
    fprintf(fp, "[ ");

    if(stmt->type == RETURN_STATEMENT)
    {
      fprintf(fp, "\"RETURN_STATEMENT\", ");
      print_struct_obj_reference(fp, RETURN_STATEMENT_PTR, (void *)stmt->ret_stmt);
    }
    if(stmt->type == EXPRESSION)
    { 
      fprintf(fp, "\"EXPRESSION\", ");
      print_struct_obj_reference(fp, EXPRESSION_PTR, (void *)stmt->exp);
    }
    if(stmt->type == EXP_PLUS_STATEMENTS)
    {
      fprintf(fp, "\"EXP_PLUS_STATEMENTS\", ");
      print_struct_obj_reference(fp, STATEMENTS_PTR, (void *)stmt->statements);
    }
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == IDENTIFIERS_PTR)
  {
    identifiers_t *ids = (identifiers_t *)struct_ptr;

    /*
    unsigned int nof_identifiers;
    char **identifiers;
    */
    
    unsigned int count = ids->nof_identifiers;
   
    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      fprintf(fp, "\"%s\"", ids->identifiers[i]);
      if(i != count - 1)
	fprintf(fp, ", ");
    }

    fprintf(fp, "] ");
  }
  else if(type == EXEC_CODE_PTR)
  {
    executable_code_t *exec_code = (executable_code_t *)struct_ptr;

    /*
    temporaries_t *temporaries;
    struct statement *statements;
    */
    
    fprintf(fp, "[ ");
    print_struct_obj_reference(fp, TEMPORARIES_PTR, (void *)exec_code->temporaries);
    fprintf(fp, ", ");
    print_struct_obj_reference(fp, STATEMENTS_PTR, (void *)exec_code->statements);
    fprintf(fp, "] ");
  }
  else if(type == PACKAGE_PTR)
  {
    package_t *pkg = (package_t *)struct_ptr;

    /*
    char *name;
    int nof_symbols;
    char ** symbols;
    */

    fprintf(fp, "[ ");
    fprintf(fp, "\"%s\"", pkg->name);
    fprintf(fp, ", ");

    unsigned int count = pkg->nof_symbols;
   
    fprintf(fp, "[ ");
    for(i=0; i<count; i++)
    {
      fprintf(fp, "\"%s\"", pkg->symbols[i]);
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");
    
    fprintf(fp, "] ");
  }
  else if(type == STACK_TYPE_PTR)
  {
    stack_type *stack = (stack_type *)struct_ptr;

    /*
    unsigned int count;
    void **data;
    */

    unsigned int count = stack->count;
    
    fprintf(fp, "[ ");
    for(i=0; i<count; i++)
    {
      if(g_sub_type == OBJECT_PTR1)
	print_object_ptr_reference(fp, (OBJECT_PTR)stack->data[i], false);
      else if(g_sub_type == METHOD_PTR)
      {
	method_t *m = (method_t *)extract_ptr((OBJECT_PTR)stack->data[i]);
	print_struct_obj_reference(fp, METHOD_PTR, (void *)m);
      }
      else
	print_struct_obj_reference(fp, g_sub_type, stack->data[i]);
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");
  }
  else if(type == EXCEPTION_HANDLER_PTR)
  {
    exception_handler_t *handler = (exception_handler_t *)struct_ptr;

    /*
    OBJECT_PTR protected_block;
    OBJECT_PTR selector;
    OBJECT_PTR exception_action;
    stack_type *exception_environment;
    OBJECT_PTR cont;
    */
    
    fprintf(fp, "[ ");
    print_object_ptr_reference(fp, handler->protected_block, false);
    fprintf(fp, ", ");
    print_object_ptr_reference(fp, handler->selector, false);
    fprintf(fp, ", ");
    print_object_ptr_reference(fp, handler->exception_action, false);
    fprintf(fp, ", ");

    g_sub_type = EXCEPTION_HANDLER_PTR;
    print_struct_obj_reference(fp, STACK_TYPE_PTR, (void *)handler->exception_environment);
    g_sub_type = NONE;

    fprintf(fp, ", ");
    print_object_ptr_reference(fp, handler->cont, false);
    fprintf(fp, "] ");
  }
  else if(type == CALL_CHAIN_ENTRY_PTR)
  {
    call_chain_entry_t *entry = (call_chain_entry_t *)struct_ptr;

    /*
    OBJECT_PTR exp_ptr;
    BOOLEAN super;
    OBJECT_PTR receiver;
    OBJECT_PTR selector;
    OBJECT_PTR method;
    OBJECT_PTR closure;
    unsigned int nof_args;
    OBJECT_PTR *args;
    OBJECT_PTR local_vars_list;
    OBJECT_PTR cont;
    OBJECT_PTR termination_blk_closure;
    BOOLEAN termination_blk_invoked;
    */
    
    fprintf(fp, "[ ");
    debug_expression_t *exp = (debug_expression_t *)extract_ptr(entry->exp_ptr);
    print_struct_obj_reference(fp, DEBUG_EXPRESSION_PTR, (void *)exp);
    fprintf(fp, ", ");

    if(entry->super)
      fprintf(fp, "\"true\"");
    else
      fprintf(fp, "\"false\"");
    fprintf(fp, ", ");

    print_object_ptr_reference(fp, entry->receiver, false);
    fprintf(fp, ", ");
    
    print_object_ptr_reference(fp, entry->selector, false);
    fprintf(fp, ", ");
    
    print_object_ptr_reference(fp, entry->method, false);
    fprintf(fp, ", ");

    print_object_ptr_reference(fp, entry->closure, false);
    fprintf(fp, ", ");

    unsigned int count = entry->nof_args;
   
    fprintf(fp, "[ ");
    for(i=0; i<count; i++)
    {
      print_object_ptr_reference(fp, entry->args[i], false);
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");

    print_object_ptr_reference(fp, entry->local_vars_list, false);
    fprintf(fp, ", ");
    
    print_object_ptr_reference(fp, entry->cont, false);
    fprintf(fp, ", ");
    
    print_object_ptr_reference(fp, entry->termination_blk_closure, false);
    fprintf(fp, ", ");

    if(entry->termination_blk_invoked)
      fprintf(fp, "\"true\"");
    else
      fprintf(fp, "\"false\"");

    fprintf(fp, "] ");
  }
  else if(type == DEBUG_EXPRESSION_PTR)
  {
    debug_expression_t *debug_exp = (debug_expression_t *)struct_ptr;

    /*
    enum DebugExpressionType type;
    struct basic_expression *be;
    struct binary_argument *bin_arg;
    struct keyword_argument *kw_arg;
    */

    fprintf(fp, "[ ");
    if(debug_exp->type == DEBUG_BASIC_EXPRESSION)
    {
      fprintf(fp, "\"DEBUG_BASIC_EXPRESSION\", ");
      print_struct_obj_reference(fp, BASIC_EXPRESSION_PTR, (void *)debug_exp->be);
    }
    else if(debug_exp->type == DEBUG_BINARY_ARGUMENT)
    {
      fprintf(fp, "\"DEBUG_BINARY_ARGUMENT\", ");
      print_struct_obj_reference(fp, BINARY_ARGUMENT_PTR, (void *)debug_exp->bin_arg);
    }
    else if(debug_exp->type == DEBUG_KEYWORD_ARGUMENT)
    {
      fprintf(fp, "\"DEBUG_KEYWORD_ARGUMENT\", ");
      print_struct_obj_reference(fp, KEYWORD_ARGUMENT_PTR, (void *)debug_exp->kw_arg);
    }
    else
      assert((false));

    fprintf(fp, "] ");
  }
  else if(type == ASSIGNMENT_PTR)
  {
    assignment_t *asgn = (assignment_t *)struct_ptr;

    /*
    char *identifier;
    struct expression *rvalue;
    */

    fprintf(fp, "[ ");
    fprintf(fp, "\"%s\", ", asgn->identifier);
    fprintf(fp, ", ");
    print_struct_obj_reference(fp, EXPRESSION_PTR, (void *)asgn->rvalue);
    fprintf(fp, "[ ");
  }
  else if(type == BASIC_EXPRESSION_PTR)
  {
    basic_expression_t *basic_exp = (basic_expression_t *)struct_ptr;

    /*
    enum BasicExpressionType type;
    struct primary *prim;
    struct message *msg;
    struct cascaded_messages *cascaded_msgs;
    */

    fprintf(fp, "[ ");
    if(basic_exp->type == PRIMARY)
    {
      fprintf(fp, "\"PRIMARY\", ");
      print_struct_obj_reference(fp, PRIMARY_PTR, basic_exp->prim);
    }
    else if(basic_exp->type == PRIMARY_PLUS_MESSAGES)
    {
      fprintf(fp, "\"PRIMARY_PLUS_MESSAGES\", ");
      print_struct_obj_reference(fp, MESSAGE_PTR, basic_exp->msg);
      fprintf(fp, ", ");
      print_struct_obj_reference(fp, CASCADED_MESSAGES_PTR, basic_exp->cascaded_msgs);
    }
    else
      assert(false);
    fprintf(fp, "] ");
  }
  else if(type == UNARY_MESSAGES_PTR)
  {
    unary_messages_t *unary_msgs = (unary_messages_t *)struct_ptr;

    /*
    unsigned int nof_messages;
    char **identifiers;
    */

    fprintf(fp, "[ ");

    unsigned int count = unary_msgs->nof_messages;
   
    fprintf(fp, "[ ");
    for(i=0; i<count; i++)
    {
      fprintf(fp, "\"%s\"", unary_msgs->identifiers[i]);
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");
    
    fprintf(fp, "] ");
  }
  else if(type == ARRAY_ELEMENTS_PTR)
  {
    array_elements_t *elems = (array_elements_t *)struct_ptr;

    /*
    unsigned int nof_elements;
    struct array_element *elements;
    */

    fprintf(fp, "[ ");

    unsigned int count = elems->nof_elements;
   
    fprintf(fp, "[ ");
    for(i=0; i<count; i++)
    {
      print_struct_obj_reference(fp, ARRAY_ELEMENT_PTR, (void *)(elems->elements+i));
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");
    
    fprintf(fp, "] ");
  }
  else if(type == ARRAY_ELEMENT_PTR)
  {
    array_element_t *elem = (array_element_t *)struct_ptr;

    /*
    enum ArrayElementType type;
    struct literal *lit;
    char *identifier;
    */

    fprintf(fp, "[ ");

    if(elem->type == LITERAL1)
    {
      fprintf(fp, "\"LITERAL\", ");
      print_struct_obj_reference(fp, LITERAL_PTR, (void *)elem->lit);
    }
    else if(elem->type == IDENTIFIER1)
    {
      fprintf(fp, "\"IDENTIFIER\", ");
      fprintf(fp, "\"%s\"", elem->identifier);
    }
    else
      assert(false);
      
    fprintf(fp, "] ");
  }
  else
    assert(false);

  hashtable_put(printed_struct_objects, (void *)struct_ptr, (void *)1);
}

void print_global_variables(FILE *fp)
{
  fprintf(fp, "\"global_variables\": ");

  /*
    OBJECT_PTR g_message_selector
    unsigned int g_nof_string_literals
    OBJECT_PTR g_idclo (this can be created afresh)
    OBJECT_PTR g_msg_snd_closure
    OBJECT_PTR g_msg_snd_super_closure
    OBJECT_PTR g_compile_time_method_selector
    OBJECT_PTR g_run_till_cont
    enum DebugAction g_debug_action
    package_t *g_smalltalk_symbols
    package_t *g_compiler_package
    char **g_string_literals
    stack_type *g_exception_environment
    stack_type *g_call_chain
    stack_type *g_exception_contexts
    stack_type *g_breakpointed_methods
    int g_open_square_brackets (NO NEED TO SERIALIZE THIS)
    BOOLEAN g_loading_core_library
    BOOLEAN g_running_tests
    OBJECT_PTR g_method_call_stack
    OBJECT_PTR g_last_eval_result
    BOOLEAN g_system_initialized
    enum UIMode g_ui_mode
    BOOLEAN g_eval_aborted
    executable_code_t *g_exp
    binding_env_t *g_top_level
    BOOLEAN g_debugger_invoked_for_exception
    exception_handler_t *g_active_handler
    BOOLEAN g_debug_in_progress
    OBJECT_PTR g_debug_cont
    stack_type *g_handler_environment
    stack_type *g_signalling_environment
    int g_include_stack_ptr (NO NEED TO SERIALIZE THIS)
    
   */
  unsigned int i;

  fprintf(fp, "[ ");

  fprintf(fp, "{ \"g_message_selector\" : ");
  print_object_ptr_reference(fp, g_message_selector, false);
  fprintf(fp, " }, ");

  fprintf(fp, "{ \"g_nof_string_literals\" : ");
  fprintf(fp, "%d", g_nof_string_literals);
  fprintf(fp, " }, ");

  fprintf(fp, "{ \"g_msg_snd_closure\" : ");
  print_object_ptr_reference(fp, g_msg_snd_closure, false);
  fprintf(fp, " }, ");
  
  fprintf(fp, "{ \"g_msg_snd_super_closure\" : ");
  print_object_ptr_reference(fp, g_msg_snd_super_closure, false);
  fprintf(fp, " }, ");

  fprintf(fp, "{ \"g_compile_time_method_selector\" : ");
  print_object_ptr_reference(fp, g_compile_time_method_selector, false);
  fprintf(fp, " }, ");

  fprintf(fp, "{ \"g_run_till_cont\" : ");
  print_object_ptr_reference(fp, g_run_till_cont, false);
  fprintf(fp, " }, ");

  fprintf(fp, "{ \"g_debug_action\" : ");
  if(g_debug_action == CONTINUE)
    fprintf(fp, "\"CONTINUE\"");
  else if(g_debug_action == STEP_INTO)
    fprintf(fp, "\"STEP_INTO\"");    
  else if(g_debug_action == STEP_OVER)
    fprintf(fp, "\"STEP_OVER\"");
  else if(g_debug_action == STEP_OUT)
    fprintf(fp, "\"STEP_OUT\"");
  else if(g_debug_action == ABORT)
    fprintf(fp, "\"ABORT\"");
  else
    assert(false);
  fprintf(fp, " }, ");

  fprintf(fp, "{ \"g_smalltalk_symbols\" : ");
  print_struct_obj_reference(fp, PACKAGE_PTR, (void *)g_smalltalk_symbols);
  fprintf(fp, " }, ");
  
  fprintf(fp, "{ \"g_compiler_package\" : ");
  print_struct_obj_reference(fp, PACKAGE_PTR, (void *)g_compiler_package);
  fprintf(fp, "} , ");

  fprintf(fp, "{ \"g_string_literals\" : ");
  unsigned int count = g_nof_string_literals;

  fprintf(fp, "[ ");
  for(i=0; i<count; i++)
  {
    unsigned int j, len = strlen(g_string_literals[i]);

    fprintf(fp, "\"");

    for(j=0; j<len; j++)
    {
      if(g_string_literals[i][j] == '\n')
	fprintf(fp, "\\n");
      else
	fprintf(fp, "%c", g_string_literals[i][j]);
    }
    fprintf(fp, "\"");

    if(i != count - 1)
      fprintf(fp, ", ");
  }
  fprintf(fp, "] },  ");

  fprintf(fp, "{ \"g_exception_environment\" : ");
  print_struct_obj_reference(fp, STACK_TYPE_PTR, (void *)g_exception_environment);
  fprintf(fp, " }, ");
  
  fprintf(fp, "{ \"g_call_chain\" : ");
  g_sub_type = CALL_CHAIN_ENTRY_PTR;
  print_struct_obj_reference(fp, STACK_TYPE_PTR, (void *)g_call_chain);
  g_sub_type = NONE;
  fprintf(fp, "} , ");

  fprintf(fp, "{ \"g_exception_contexts\" : ");
  g_sub_type = OBJECT_PTR1;
  print_struct_obj_reference(fp, STACK_TYPE_PTR, (void *)g_exception_contexts);
  g_sub_type = NONE;
  fprintf(fp, "} , ");

  fprintf(fp, "{ \"g_breakpointed_methods\" : ");
  g_sub_type = METHOD_PTR;
  print_struct_obj_reference(fp, STACK_TYPE_PTR, (void *)g_breakpointed_methods);
  g_sub_type = NONE;
  fprintf(fp, " }, ");

  fprintf(fp, "{ \"g_top_level\" : ");
  print_struct_obj_reference(fp, BINDING_ENV_PTR, (void *)g_top_level);
  fprintf(fp, "} , ");

  fprintf(fp, "{ \"g_debugger_invoked_for_exception\" : ");
  if(g_debugger_invoked_for_exception)
    fprintf(fp, "\"true\"");
  else
    fprintf(fp, "\"false\"");
  fprintf(fp, " }, ");

  //TODO: add this null check for other
  //variables as required
  if(g_active_handler)
  {
    fprintf(fp, "{ \"g_active_handler\" : ");
    print_struct_obj_reference(fp, EXCEPTION_HANDLER_PTR, (void *)g_active_handler);
    fprintf(fp, " }, ");
  }

  fprintf(fp, "{ \"g_debug_in_progress\" : ");
  if(g_debug_in_progress)
    fprintf(fp, "\"true\"");
  else
    fprintf(fp, "\"false\"");
  fprintf(fp, " }, ");

  fprintf(fp, "{ \"g_debug_cont\" : ");
  print_object_ptr_reference(fp, g_debug_cont, false);
  fprintf(fp, " } ");

  if(g_handler_environment)
  {
    fprintf(fp, ", { \"g_handler_environment\" : ");
    g_sub_type = EXCEPTION_HANDLER_PTR;
    print_struct_obj_reference(fp, STACK_TYPE_PTR, (void *)g_handler_environment);
    g_sub_type = NONE;
    fprintf(fp, " } ");
  }

  if(g_signalling_environment)
  {
    fprintf(fp, ", { \"g_signalling_environment\" : ");
    g_sub_type = EXCEPTION_HANDLER_PTR;
    print_struct_obj_reference(fp, STACK_TYPE_PTR, (void *)g_signalling_environment);
    g_sub_type = NONE;
    fprintf(fp, " } ");
  }

  fprintf(fp, " ] ");
}

//need a struct version of convert_heap() too

void create_image(char *file_name)
{
  FILE *fp = fopen(file_name, "w");  

  print_queue = queue_create();
  hashtable = hashtable_create(1000001);
  printed_objects = hashtable_create(1000001);

  print_queue_struct = queue_create();
  hashtable_struct = hashtable_create(1000001);
  printed_struct_objects = hashtable_create(1000001);
  
  fprintf(fp, "{ ");

  print_global_variables(fp);

  //heap for structs
  fprintf(fp, ", \"heap_struct\" : [");

  while(!queue_is_empty(print_queue_struct))
  {
    queue_item_t *queue_item = queue_dequeue(print_queue_struct);
    struct_slot_t *s = (struct_slot_t *)queue_item->data;
    print_heap_representation_struct(fp, s->ref, s->type);
    if(!queue_is_empty(print_queue_struct))fprintf(fp, ", ");
  }

  queue_delete(print_queue_struct);
  hashtable_delete(hashtable_struct);
  hashtable_delete(printed_struct_objects);

  fprintf(fp, "] ");
  //end of heap for structs

  //OBJECT_PTR heap
  fprintf(fp, ", \"heap\" : [");

  BOOLEAN beginning = true;

  BOOLEAN message_send_native_fn_obj;

  while(!queue_is_empty(print_queue))
  {
    queue_item_t *queue_item = queue_dequeue(print_queue);
    OBJECT_PTR obj = (OBJECT_PTR)(queue_item->data);
    if(!is_dynamic_memory_object(obj))
    {
      print_object(obj);printf("\n");
      assert(false);
    }

    if(IS_NATIVE_FN_OBJECT(obj))
    {
      nativefn nf = get_nativefn_value(obj);
      if(nf == (nativefn)message_send ||
	 nf == (nativefn)message_send_super)
	message_send_native_fn_obj = true;
      else
	message_send_native_fn_obj = false;
    }
    else
      message_send_native_fn_obj = false;

    if(!beginning && !message_send_native_fn_obj)
      fprintf(fp, ", ");

    print_heap_representation(fp, obj, false);

    beginning = false;
  }

  queue_delete(print_queue);
  hashtable_delete(hashtable);
  hashtable_delete(printed_objects);

  fprintf(fp, "] ");
  //end of OBJECT_PTR heap
  
 fprintf(fp, "} "); 
}

BOOLEAN is_dynamic_memory_object(OBJECT_PTR obj)
{
   return IS_CONS_OBJECT(obj)         ||
          IS_ARRAY_OBJECT(obj)        ||
          IS_CLOSURE_OBJECT(obj)      ||
          IS_FLOAT_OBJECT(obj)        ||
          IS_OBJECT_OBJECT(obj)       ||
          IS_CLASS_OBJECT(obj)        ||
          IS_NATIVE_FN_OBJECT(obj);
}

void print_object_ptr_reference(FILE *fp,
				OBJECT_PTR obj,
				BOOLEAN single_object)
{
  //TODO: is_valid_object() may not be able to provide correct answers
  //for pointers like debug_expression_t * which are wrapped to become
  //OBJECT_PTRS
  //if(!is_valid_object(obj))
  //  assert(false);

  if(is_dynamic_memory_object(obj))
  {
    hashtable_entry_t *e = hashtable_get(hashtable, (void *)obj);

    if(e)
      fprintf(fp, "%d", (int)e->value);
    else
    {
      fprintf(fp, "%lu",  ((obj_count) << OBJECT_SHIFT) + (obj & BIT_MASK));
      hashtable_put(hashtable, (void *)obj, (void *)  ((obj_count) << OBJECT_SHIFT) + (obj & BIT_MASK) );
      obj_count++;
    }

    add_obj_to_print_list(obj);
  }
  else
  {
    if(single_object)
    {
      fprintf(fp, "%lu",  ((obj_count) << OBJECT_SHIFT) + (obj & BIT_MASK));
      obj_count++;

      add_obj_to_print_list(obj);
    }
    else
      fprintf(fp, "%lu", obj);
  }
}

void print_heap_representation(FILE *fp, 
                               OBJECT_PTR obj, 
                               BOOLEAN single_object)
{
  if(single_object)
  {
    if(IS_SYMBOL_OBJECT(obj))
    {
      fprintf(fp, "\"%s\" ", get_symbol_name(obj));
      return; //TODO: why a return statement only here?
    }
    //TODO: since we are already serializing g_string_literals,
    //this needs to be changed
    else if(IS_STRING_LITERAL_OBJECT(obj))
      fprintf(fp, "%d ", obj >> OBJECT_SHIFT);
    else if(IS_CHARACTER_OBJECT(obj))
      fprintf(fp, "%d ", get_char_value(obj));
    else if(IS_TRUE_OBJECT(obj))
      fprintf(fp, "true ");
    else if(IS_FALSE_OBJECT(obj))
      fprintf(fp, "false ");
  }

  if(!single_object && !is_dynamic_memory_object(obj))
  {
    printf("%d\n", (int)obj);
    assert(false);
  }

  if(IS_CONS_OBJECT(obj))
  {
    OBJECT_PTR car_obj = car(obj);
    OBJECT_PTR cdr_obj = cdr(obj);

    fprintf(fp, "[");
    print_object_ptr_reference(fp, car_obj, single_object);
    fprintf(fp, ", ");
    print_object_ptr_reference(fp, cdr_obj, single_object);
    fprintf(fp, "] ");
  }
  else if(IS_ARRAY_OBJECT(obj))
  {
    print_struct_obj_reference(fp, ARRAY_OBJ_PTR, (void *)extract_ptr(obj));
  }
  else if(IS_OBJECT_OBJECT(obj))
  {
    print_struct_obj_reference(fp, OBJ_PTR, (void *)extract_ptr(obj));
  }
  else if(IS_CLASS_OBJECT(obj))
  {
    print_struct_obj_reference(fp, CLASS_OBJ_PTR, (void *)extract_ptr(obj));
  }
  else if(IS_CLOSURE_OBJECT(obj)) //TODO: confirm we need to serialize closure objects
  {
    OBJECT_PTR cons_form = extract_ptr(obj) + CONS_TAG;

    OBJECT_PTR nativefn_obj = first(cons_form);
    OBJECT_PTR closed_vals = second(cons_form);
    OBJECT_PTR arity = third(cons_form);

    fprintf(fp, "[");
    print_object_ptr_reference(fp, nativefn_obj, single_object);
    fprintf(fp, ", ");
    print_object_ptr_reference(fp, closed_vals, single_object);
    fprintf(fp, ", ");
    print_object_ptr_reference(fp, arity, single_object);
    fprintf(fp, "] ");
  }
  else if(IS_NATIVE_FN_OBJECT(obj))
  {
    nativefn nf = get_nativefn_value(obj);
    if(nf != (nativefn)message_send &&
       nf != (nativefn)message_send_super)
    {
      char *src = get_native_fn_source(nf);

      if(src)
      {
	unsigned int i, len = strlen(src);

	fprintf(fp, "\"");

	for(i=0; i<len; i++)
	{
	  if(src[i] == '\n')
	    fprintf(fp, "\\n");
	  else
	    fprintf(fp, "%c", src[i]);
	}
	fprintf(fp, "\"");
      }
      else
	fprintf(fp, "NULL");
    }
  }
  else if(IS_INTEGER_OBJECT(obj))
    fprintf(fp, "%d", get_int_value(obj));
  else if(IS_FLOAT_OBJECT(obj))
    fprintf(fp, "%lf", get_float_value(obj));
  else
    if(!single_object)assert(false);

  hashtable_put(printed_objects, (void *)obj, (void *)1);
}
