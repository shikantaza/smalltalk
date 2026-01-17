#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "gc.h"

#include "queue.h"
#include "hashtable.h"

#include "global_decls.h"

#include "json.h"

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
  METHOD_BINDING_ENV_PTR,
  METHOD_BINDING_PTR,
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

//this stores both OBJECT_PTRs and native pointers.
//for OBJECT_PTRs index1 is the position of interest,
//for native pointers it's both index1 and index2
//(index1 is the ordinal position of the data member
//in the native pointer's struct definition, index2
//is the ordinal position if that data member itself
//is an array (e.g., 'instances' array in class_object_t)).
//index2 is to be ignored if not applicable; this will
//be evident from the context.
struct slot
{
  OBJECT_PTR ref;
  OBJECT_PTR ptr;
  enum PointerType type;
  enum PointerType sub_type;
  unsigned int index1;
  unsigned int index2;
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
} native_ptr_slot_t;

void print_object_ptr_reference(FILE *, OBJECT_PTR, BOOLEAN);
void print_heap_representation(FILE *, OBJECT_PTR, BOOLEAN);
BOOLEAN is_dynamic_memory_object(OBJECT_PTR);
void hashtable_delete(hashtable_t *);

//TODO: need to reimplement these or
//create the necessary global variable
//(native_fn_src_mapping_t)

nativefn get_nativefn_value(OBJECT_PTR);
char *get_native_fn_source(nativefn);

void print_native_functions(FILE *);

queue_t *obj_print_queue;
hashtable_t *obj_hashtable, *printed_objects;
unsigned int obj_count = 0;

queue_t *native_ptr_print_queue;
hashtable_t *native_ptr_hashtable, *printed_native_objects;
unsigned int native_ptr_count = 0;

//global variable that indicates what is the type
//of the pointer that is stored in a stack_type object
enum PointerType g_sub_type;

extern OBJECT_PTR           g_message_selector;
extern unsigned int         g_nof_string_literals;
extern OBJECT_PTR           g_msg_snd_closure;
extern OBJECT_PTR           g_msg_snd_super_closure;
extern OBJECT_PTR           g_compile_time_method_selector;
extern OBJECT_PTR           g_run_till_cont;
extern enum DebugAction     g_debug_action;
extern package_t           *g_smalltalk_symbols;
extern package_t           *g_compiler_package;
extern char               **g_string_literals;
extern stack_type          *g_exception_environment;
extern stack_type          *g_call_chain;
extern stack_type          *g_exception_contexts;
extern stack_type          *g_breakpointed_methods;
extern BOOLEAN              g_loading_core_library;
extern BOOLEAN              g_running_tests;
extern OBJECT_PTR           g_method_call_stack;
extern OBJECT_PTR           g_last_eval_result;
extern BOOLEAN              g_system_initialized;
extern enum UIMode          g_ui_mode;
extern BOOLEAN              g_eval_aborted;
extern executable_code_t   *g_exp;
extern binding_env_t       *g_top_level;
extern BOOLEAN              g_debugger_invoked_for_exception;
extern exception_handler_t *g_active_handler;
extern BOOLEAN              g_debug_in_progress;
extern OBJECT_PTR           g_debug_cont;
extern stack_type          *g_handler_environment;
extern stack_type          *g_signalling_environment;
extern unsigned int         g_nof_compiler_states;

extern OBJECT_PTR NIL;

extern unsigned int g_nof_native_fns;
extern native_fn_src_mapping_t *g_native_fn_objects;

//forward declarations
OBJECT_PTR deserialize_object_reference(struct JSONObject *,
					struct JSONObject *,
					OBJECT_PTR,
					hashtable_t *,
					hashtable_t *,
					queue_t *);
void *deserialize_native_ptr_reference(struct JSONObject *,
				       struct JSONObject *,
				       enum PointerType ptr_type,
				       OBJECT_PTR ,
				       hashtable_t *,
				       hashtable_t *,
				       queue_t *);
//end forward declarations

void add_obj_to_print_list(OBJECT_PTR obj)
{
  //this search is O(n), but this is OK because
  //the queue keeps growing and shrinking, so its
  //size at any point in time is quite small (<10)
  if(obj != NIL && (queue_item_exists(obj_print_queue, (void *)obj) || hashtable_get(printed_objects, (void *)obj)))
    return;

  //assert(is_dynamic_memory_object(obj));
  queue_enqueue(obj_print_queue, (void *)obj);
}

int native_ptr_queue_item_exists(queue_t *q, void *value)
{
  native_ptr_slot_t *s = (native_ptr_slot_t *)value;
  
  queue_item_t *np = q->first;

  while(np != NULL)
  {
    native_ptr_slot_t *s1 = (native_ptr_slot_t *)np->data;
    if(s1->type == s->type &&
       s1->sub_type == s->sub_type
       && s1->ref == s->ref)
      return 1;

    np = np->next;
  }

  return 0;
}

void add_obj_to_native_ptr_print_list(void *native_ptr, enum PointerType type)
{
  assert(native_ptr);

  //this search is O(n), but this is OK because
  //the queue keeps growing and shrinking, so its
  //size at any point in time is quite small (<10)
  if(native_ptr != NULL &&
     (native_ptr_queue_item_exists(native_ptr_print_queue, native_ptr) ||
      hashtable_get(printed_native_objects, native_ptr)))
    return;

  native_ptr_slot_t *s = (native_ptr_slot_t *)GC_MALLOC(sizeof(native_ptr_slot_t));
  s->type = type;
  s->sub_type = g_sub_type;
  s->ref = native_ptr;
  
  queue_enqueue(native_ptr_print_queue, (void *)s);
}

void print_native_ptr_reference(FILE *fp,
				enum PointerType type,
				void *native_ptr)
{
  hashtable_entry_t *e = hashtable_get(native_ptr_hashtable, native_ptr);

  if(e)
    fprintf(fp, "%d", (int)e->value);
  else
  {
    fprintf(fp, "%lu", native_ptr_count);
    hashtable_put(native_ptr_hashtable, native_ptr, (void *)native_ptr_count);
    native_ptr_count++;
  }

  add_obj_to_native_ptr_print_list(native_ptr, type);
}

void print_native_ptr_heap_representation(FILE *fp,
					  void *native_ptr,
					  enum PointerType type)
{
  unsigned int i;

  if(type == ARRAY_OBJ_PTR)
  {
    array_object_t *arr_obj = (array_object_t *)native_ptr;

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
    method_t *m = (method_t *)native_ptr;

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
    print_native_ptr_reference(fp, CLASS_OBJ_PTR, (void *)m->cls_obj);

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

    print_native_ptr_reference(fp, EXEC_CODE_PTR, (void *)m->exec_code);
    fprintf(fp, ", ");
    
    fprintf(fp, "\"%s\"", (m->breakpointed == true) ? "true" : "false");
    fprintf(fp, "] ");
  }
  else if(type == OBJ_PTR)
  {
    object_t *obj = (object_t *)native_ptr;

    /*
    OBJECT_PTR class_object;
    binding_env_t *instance_vars; //instance var name, value
    */
    fprintf(fp, "[ ");
    print_object_ptr_reference(fp, obj->class_object, false);
    fprintf(fp, ", ");

    g_sub_type = OBJECT_PTR1;
    print_native_ptr_reference(fp, BINDING_ENV_PTR, (void *)obj->instance_vars);
    g_sub_type = NONE;
    
    fprintf(fp, "] ");
  }
  else if(type == CLASS_OBJ_PTR)
  {
    class_object_t *cls_obj = (class_object_t *)native_ptr;

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

    //fprintf(fp, "[ ");
    fprintf(fp, "%d, ", nof_instances);

    fprintf(fp, "[ ");
    for(i=0; i<nof_instances; i++)
    {
      print_object_ptr_reference(fp, cls_obj->instances[i], false);
      if(i != nof_instances - 1)
	fprintf(fp, ", ");
    }
    //fprintf(fp, "]], ");
    fprintf(fp, "], ");

    unsigned int nof_inst_vars = cls_obj->nof_instance_vars;
    
    //fprintf(fp, "[ ");
    fprintf(fp, "%d, ", nof_inst_vars);

    fprintf(fp, "[ ");
    for(i=0; i<nof_inst_vars; i++)
    {
      print_object_ptr_reference(fp, cls_obj->inst_vars[i], false);
      if(i != nof_inst_vars - 1)
	fprintf(fp, ", ");
    }
    //fprintf(fp, "]], ");
    fprintf(fp, "], ");

    //g_sub_type = OBJECT_PTR1;
    print_native_ptr_reference(fp, BINDING_ENV_PTR, (void *)cls_obj->shared_vars);
    //g_sub_type = NONE;

    //g_sub_type = METHOD_PTR;
    print_native_ptr_reference(fp, METHOD_BINDING_ENV_PTR, (void *)cls_obj->instance_methods);
    //g_sub_type = NONE;

    //g_sub_type = METHOD_PTR;
    print_native_ptr_reference(fp, METHOD_BINDING_ENV_PTR, (void *)cls_obj->class_methods);
    //g_sub_type = NONE;

    fprintf(fp, "] ");
  }
  else if(type == BINDING_ENV_PTR)
  {
    binding_env_t *env = (binding_env_t *)native_ptr;

    /*
    unsigned int count;
    binding_t *bindings;
    */

    fprintf(fp, "[ ");

    unsigned count = env->count;

    for(i=0; i<count; i++)
    {
      //fprintf(fp, "[ ");
      print_native_ptr_reference(fp, BINDING_PTR, (void *)(env->bindings+i));
      //fprintf(fp, "]");
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");
  }
  else if(type == BINDING_PTR)
  {
    binding_t *binding = (binding_t *)native_ptr;

    /*
    OBJECT_PTR key;
    OBJECT_PTR val;
    */

    fprintf(fp, "[ ");
    print_object_ptr_reference(fp, binding->key, false);
    fprintf(fp, ", ");
    print_object_ptr_reference(fp, binding->val, false);
    /*
    if(g_sub_type == METHOD_PTR)
    {
      method_t *m = (method_t *)extract_ptr(binding->val);
      print_native_ptr_reference(fp, METHOD_PTR, (void *)m);
    }
    else if(g_sub_type == OBJECT_PTR1 || g_sub_type == NONE)
      print_object_ptr_reference(fp, binding->val, false);
    else
      assert(false);
    */
    fprintf(fp, "] ");
  }
  else if(type == METHOD_BINDING_ENV_PTR)
  {
    method_binding_env_t *env = (method_binding_env_t *)native_ptr;

    /*
    unsigned int count;
    binding_t *bindings;
    */

    fprintf(fp, "[ ");

    unsigned count = env->count;

    for(i=0; i<count; i++)
    {
      //fprintf(fp, "[ ");
      print_native_ptr_reference(fp, METHOD_BINDING_PTR, (void *)(env->bindings+i));
      //fprintf(fp, "]");
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");
  }
  else if(type == METHOD_BINDING_PTR)
  {
    method_binding_t *binding = (method_binding_t *)native_ptr;

    /*
    OBJECT_PTR key;
    method_t *val;
    */

    fprintf(fp, "[ ");
    print_object_ptr_reference(fp, binding->key, false);
    fprintf(fp, ", ");
    print_native_ptr_reference(fp, METHOD_PTR, (void *)binding->val);
    /*
    if(g_sub_type == METHOD_PTR)
    {
      method_t *m = (method_t *)extract_ptr(binding->val);
      print_native_ptr_reference(fp, METHOD_PTR, (void *)m);
    }
    else if(g_sub_type == OBJECT_PTR1 || g_sub_type == NONE)
      print_object_ptr_reference(fp, binding->val, false);
    else
      assert(false);
    */
    fprintf(fp, "] ");
  }
  else if(type == EXPRESSION_PTR)
  {
    expression_t *exp = (expression_t *)native_ptr;

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
      print_native_ptr_reference(fp, ASSIGNMENT_PTR, (void *)exp->asgn);
    }
    else if(exp->type == BASIC_EXPRESSION)
    {
      fprintf(fp, "BASIC_EXPRESSION, ");
      print_native_ptr_reference(fp, BASIC_EXPRESSION_PTR, (void *)exp->basic_exp);
    }      
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == RETURN_STATEMENT_PTR)
  {
    return_statement_t *ret_stmt = (return_statement_t *)native_ptr;

    fprintf(fp, "[ ");
    print_native_ptr_reference(fp, EXPRESSION_PTR, (void *)ret_stmt->exp);
    fprintf(fp, "] ");
  }
  else if(type == PRIMARY_PTR)
  {
    primary_t *prim = (primary_t *)native_ptr;

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
      print_native_ptr_reference(fp, LITERAL_PTR, (void *)prim->lit);
    }
    else if(prim->type == BLOCK_CONSTRUCTOR)
    {
      fprintf(fp, "BLOCK_CONSTRUCTOR, ");
      print_native_ptr_reference(fp, BLOCK_CONSTRUCTOR_PTR, (void *)prim->blk_cons);
    }
    else if(prim->type == EXPRESSION1)
    {
      fprintf(fp, "EXPRESSION, ");
      print_native_ptr_reference(fp, EXPRESSION_PTR, (void *)prim->exp);
    }
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == MESSAGE_PTR)
  {
    message_t *msg = (message_t *)native_ptr;

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
      print_native_ptr_reference(fp, UNARY_MESSAGES_PTR, (void *)msg->unary_messages);
    }
    else if(msg->type == BINARY_MESSAGE)
    {
      fprintf(fp, "BINARY_MESSAGE, ");
      print_native_ptr_reference(fp, BINARY_MESSAGES_PTR, (void *)msg->binary_messages);
    }
    else if(msg->type == KEYWORD_MESSAGE)
    {
      fprintf(fp, "KEYWORD_MESSAGE, ");
      print_native_ptr_reference(fp, KEYWORD_MESSAGE_PTR, (void *)msg->kw_msg);
    }
    else
      assert(false);
    
    fprintf(fp, "] ");
  }
  else if(type == CASCADED_MESSAGES_PTR)
  {
    cascaded_messages_t *casc_msgs = (cascaded_messages_t *)native_ptr;

    /*
    unsigned int nof_cascaded_msgs;
    struct message *cascaded_msgs;
    */

    unsigned int count = casc_msgs->nof_cascaded_msgs;

    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      print_native_ptr_reference(fp, MESSAGE_PTR, (void *)(casc_msgs->cascaded_msgs+i));
      if(i != count - 1)
	fprintf(fp, "] ");
    }

    fprintf(fp, "] ");
  }
  else if(type == LITERAL_PTR)
  {
    literal_t *lit = (literal_t *)native_ptr;

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
      print_native_ptr_reference(fp, LITERAL_PTR, (void *)lit->num);
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
      print_native_ptr_reference(fp, ARRAY_ELEMENTS_PTR, (void *)lit->array_elements);
    }
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == BLOCK_CONSTRUCTOR_PTR)
  {
    block_constructor_t *constructor = (block_constructor_t *)native_ptr;

    /*
    unsigned int type;
    struct block_arguments *block_args;
    struct executable_code *exec_code;
    */
    
    fprintf(fp, "[ ");

    if(constructor->type == BLOCK_ARGS)
    {
      fprintf(fp, "\"BLOCK_ARGS\", ");
      print_native_ptr_reference(fp, BLOCK_ARGUMENT_PTR, (void *)constructor->block_args);
    }
    else if(constructor->type == NO_BLOCK_ARGS)
    {
      fprintf(fp, "\"NO_BLOCK_ARGS\", ");
      print_native_ptr_reference(fp, EXEC_CODE_PTR, (void *)constructor->exec_code);
    }
    else
      assert(false);
    
    fprintf(fp, "] ");
  }
  else if(type == BLOCK_ARGUMENT_PTR)
  {
    block_arguments_t *args = (block_arguments_t *)native_ptr;

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
    binary_messages_t *bin_msgs = (binary_messages_t *)native_ptr;

    /*
    unsigned int nof_messages;
    struct binary_message *bin_msgs;
    */
    unsigned int count = bin_msgs->nof_messages;
   
    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      print_native_ptr_reference(fp, MESSAGE_PTR, (void *)(bin_msgs->bin_msgs+i));
      if(i != count - 1)
	fprintf(fp, ", ");
    }

    fprintf(fp, "] ");
  }
  else if(type == BINARY_MESSAGE_PTR)
  {
    binary_message_t *bin_msg = (binary_message_t *)native_ptr;

    /*
    char *binary_selector;
    struct binary_argument *bin_arg;
    */
   
    fprintf(fp, "[ ");
    fprintf(fp, "\"%s\", ", bin_msg->binary_selector);
    print_native_ptr_reference(fp, BINARY_ARGUMENT_PTR, (void *)bin_msg->bin_arg);
    fprintf(fp, "] ");
  }
  else if(type == KEYWORD_MESSAGE_PTR)
  {
    keyword_message_t *kw_msg = (keyword_message_t *)native_ptr;

    /*
    unsigned int nof_args;
    struct keyword_argument_pair *kw_arg_pairs;
    */

    unsigned int count = kw_msg->nof_args;
   
    fprintf(fp, "[ ");

    for(i=0; i<count; i++)
    {
      print_native_ptr_reference(fp, KEYWORD_ARGUMENT_PAIR_PTR, (void *)(kw_msg->kw_arg_pairs+i));
      if(i != count - 1)
	fprintf(fp, ", ");
    }

    fprintf(fp, "] ");
  }
  else if(type == BINARY_ARGUMENT_PTR)
  {
    binary_argument_t *bin_arg = (binary_argument_t *)native_ptr;

    /*
    struct primary *prim;
    struct unary_messages *unary_messages;
    */

    fprintf(fp, "[ ");
    print_native_ptr_reference(fp, PRIMARY_PTR, (void *)bin_arg->prim);
    fprintf(fp, ", ");
    print_native_ptr_reference(fp, UNARY_MESSAGES_PTR, (void *)bin_arg->unary_messages);
    fprintf(fp, "] ");
  }
  else if(type == KEYWORD_ARGUMENT_PAIR_PTR)
  {
    keyword_argument_pair_t *kw_arg_pair = (keyword_argument_pair_t *)native_ptr;

    /*
    char *keyword;
    struct keyword_argument *kw_arg;
    */
    
    fprintf(fp, "[ ");
    fprintf(fp, "\"%s\", ", kw_arg_pair->keyword);
    print_native_ptr_reference(fp, KEYWORD_ARGUMENT_PTR, (void *)kw_arg_pair->kw_arg);
    fprintf(fp, "] ");
  }
  else if(type == TEMPORARIES_PTR)
  {
    temporaries_t *temps = (temporaries_t *)native_ptr;

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
    statement_t *stmt = (statement_t *)native_ptr;

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
      print_native_ptr_reference(fp, RETURN_STATEMENT_PTR, (void *)stmt->ret_stmt);
    }
    if(stmt->type == EXPRESSION)
    { 
      fprintf(fp, "\"EXPRESSION\", ");
      print_native_ptr_reference(fp, EXPRESSION_PTR, (void *)stmt->exp);
    }
    if(stmt->type == EXP_PLUS_STATEMENTS)
    {
      fprintf(fp, "\"EXP_PLUS_STATEMENTS\", ");
      print_native_ptr_reference(fp, STATEMENTS_PTR, (void *)stmt->statements);
    }
    else
      assert(false);

    fprintf(fp, "] ");
  }
  else if(type == IDENTIFIERS_PTR)
  {
    identifiers_t *ids = (identifiers_t *)native_ptr;

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
    executable_code_t *exec_code = (executable_code_t *)native_ptr;

    /*
    temporaries_t *temporaries;
    struct statement *statements;
    */
    
    fprintf(fp, "[ ");
    print_native_ptr_reference(fp, TEMPORARIES_PTR, (void *)exec_code->temporaries);
    fprintf(fp, ", ");
    print_native_ptr_reference(fp, STATEMENTS_PTR, (void *)exec_code->statements);
    fprintf(fp, "] ");
  }
  else if(type == PACKAGE_PTR)
  {
    package_t *pkg = (package_t *)native_ptr;

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
    stack_type *stack = (stack_type *)native_ptr;

    /*
    unsigned int count;
    void **data;
    */

    unsigned int count = stack->count;

    fprintf(fp, "[ %d, ", g_sub_type);
    
    fprintf(fp, "[ ");
    for(i=0; i<count; i++)
    {
      if(g_sub_type == OBJECT_PTR1)
	print_object_ptr_reference(fp, (OBJECT_PTR)stack->data[i], false);
      else
	print_native_ptr_reference(fp, g_sub_type, stack->data[i]);
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "]] ");
  }
  else if(type == EXCEPTION_HANDLER_PTR)
  {
    exception_handler_t *handler = (exception_handler_t *)native_ptr;

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
    print_native_ptr_reference(fp, STACK_TYPE_PTR, (void *)handler->exception_environment);
    g_sub_type = NONE;

    fprintf(fp, ", ");
    print_object_ptr_reference(fp, handler->cont, false);
    fprintf(fp, "] ");
  }
  else if(type == CALL_CHAIN_ENTRY_PTR)
  {
    call_chain_entry_t *entry = (call_chain_entry_t *)native_ptr;

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
    print_native_ptr_reference(fp, DEBUG_EXPRESSION_PTR, (void *)exp);
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
    
    print_native_ptr_reference(fp, METHOD_PTR, (void *)entry->method);
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
    debug_expression_t *debug_exp = (debug_expression_t *)native_ptr;

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
      print_native_ptr_reference(fp, BASIC_EXPRESSION_PTR, (void *)debug_exp->be);
    }
    else if(debug_exp->type == DEBUG_BINARY_ARGUMENT)
    {
      fprintf(fp, "\"DEBUG_BINARY_ARGUMENT\", ");
      print_native_ptr_reference(fp, BINARY_ARGUMENT_PTR, (void *)debug_exp->bin_arg);
    }
    else if(debug_exp->type == DEBUG_KEYWORD_ARGUMENT)
    {
      fprintf(fp, "\"DEBUG_KEYWORD_ARGUMENT\", ");
      print_native_ptr_reference(fp, KEYWORD_ARGUMENT_PTR, (void *)debug_exp->kw_arg);
    }
    else
      assert((false));

    fprintf(fp, "] ");
  }
  else if(type == ASSIGNMENT_PTR)
  {
    assignment_t *asgn = (assignment_t *)native_ptr;

    /*
    char *identifier;
    struct expression *rvalue;
    */

    fprintf(fp, "[ ");
    fprintf(fp, "\"%s\", ", asgn->identifier);
    fprintf(fp, ", ");
    print_native_ptr_reference(fp, EXPRESSION_PTR, (void *)asgn->rvalue);
    fprintf(fp, "[ ");
  }
  else if(type == BASIC_EXPRESSION_PTR)
  {
    basic_expression_t *basic_exp = (basic_expression_t *)native_ptr;

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
      print_native_ptr_reference(fp, PRIMARY_PTR, basic_exp->prim);
    }
    else if(basic_exp->type == PRIMARY_PLUS_MESSAGES)
    {
      fprintf(fp, "\"PRIMARY_PLUS_MESSAGES\", ");
      print_native_ptr_reference(fp, MESSAGE_PTR, basic_exp->msg);
      fprintf(fp, ", ");
      print_native_ptr_reference(fp, CASCADED_MESSAGES_PTR, basic_exp->cascaded_msgs);
    }
    else
      assert(false);
    fprintf(fp, "] ");
  }
  else if(type == UNARY_MESSAGES_PTR)
  {
    unary_messages_t *unary_msgs = (unary_messages_t *)native_ptr;

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
    array_elements_t *elems = (array_elements_t *)native_ptr;

    /*
    unsigned int nof_elements;
    struct array_element *elements;
    */

    fprintf(fp, "[ ");

    unsigned int count = elems->nof_elements;
   
    fprintf(fp, "[ ");
    for(i=0; i<count; i++)
    {
      print_native_ptr_reference(fp, ARRAY_ELEMENT_PTR, (void *)(elems->elements+i));
      if(i != count - 1)
	fprintf(fp, ", ");
    }
    fprintf(fp, "] ");
    
    fprintf(fp, "] ");
  }
  else if(type == ARRAY_ELEMENT_PTR)
  {
    array_element_t *elem = (array_element_t *)native_ptr;

    /*
    enum ArrayElementType type;
    struct literal *lit;
    char *identifier;
    */

    fprintf(fp, "[ ");

    if(elem->type == LITERAL1)
    {
      fprintf(fp, "\"LITERAL\", ");
      print_native_ptr_reference(fp, LITERAL_PTR, (void *)elem->lit);
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

  hashtable_put(printed_native_objects, (void *)native_ptr, (void *)1);
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

  fprintf(fp, "{ ");

  fprintf(fp, " \"g_message_selector\" : ");
  print_object_ptr_reference(fp, g_message_selector, false);
  fprintf(fp, " , ");

  fprintf(fp, " \"g_nof_string_literals\" : ");
  fprintf(fp, "%d", g_nof_string_literals);
  fprintf(fp, " , ");

  fprintf(fp, " \"g_msg_snd_closure\" : ");
  print_object_ptr_reference(fp, g_msg_snd_closure, false);
  fprintf(fp, " , ");
  
  fprintf(fp, " \"g_msg_snd_super_closure\" : ");
  print_object_ptr_reference(fp, g_msg_snd_super_closure, false);
  fprintf(fp, " , ");

  fprintf(fp, " \"g_compile_time_method_selector\" : ");
  print_object_ptr_reference(fp, g_compile_time_method_selector, false);
  fprintf(fp, " , ");

  fprintf(fp, " \"g_run_till_cont\" : ");
  print_object_ptr_reference(fp, g_run_till_cont, false);
  fprintf(fp, " , ");

  fprintf(fp, " \"g_debug_action\" : ");
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
  fprintf(fp, " , ");

  fprintf(fp, " \"g_smalltalk_symbols\" : ");
  print_native_ptr_reference(fp, PACKAGE_PTR, (void *)g_smalltalk_symbols);
  fprintf(fp, " , ");
  
  fprintf(fp, " \"g_compiler_package\" : ");
  print_native_ptr_reference(fp, PACKAGE_PTR, (void *)g_compiler_package);
  fprintf(fp, " , ");

  fprintf(fp, " \"g_string_literals\" : ");
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
  fprintf(fp, "] ,  ");

  fprintf(fp, " \"g_exception_environment\" : ");
  print_native_ptr_reference(fp, STACK_TYPE_PTR, (void *)g_exception_environment);
  fprintf(fp, " , ");
  
  fprintf(fp, " \"g_call_chain\" : ");
  g_sub_type = CALL_CHAIN_ENTRY_PTR;
  print_native_ptr_reference(fp, STACK_TYPE_PTR, (void *)g_call_chain);
  g_sub_type = NONE;
  fprintf(fp, " , ");

  fprintf(fp, " \"g_exception_contexts\" : ");
  g_sub_type = OBJECT_PTR1;
  print_native_ptr_reference(fp, STACK_TYPE_PTR, (void *)g_exception_contexts);
  g_sub_type = NONE;
  fprintf(fp, " , ");

  fprintf(fp, " \"g_breakpointed_methods\" : ");
  g_sub_type = METHOD_PTR;
  print_native_ptr_reference(fp, STACK_TYPE_PTR, (void *)g_breakpointed_methods);
  g_sub_type = NONE;
  fprintf(fp, " , ");

  fprintf(fp, " \"g_top_level\" : ");
  print_native_ptr_reference(fp, BINDING_ENV_PTR, (void *)g_top_level);
  fprintf(fp, " , ");

  fprintf(fp, " \"g_debugger_invoked_for_exception\" : ");
  if(g_debugger_invoked_for_exception)
    fprintf(fp, "\"true\"");
  else
    fprintf(fp, "\"false\"");
  fprintf(fp, " , ");

  //TODO: add this null check for other
  //variables as required
  if(g_active_handler)
  {
    fprintf(fp, " \"g_active_handler\" : ");
    print_native_ptr_reference(fp, EXCEPTION_HANDLER_PTR, (void *)g_active_handler);
    fprintf(fp, " , ");
  }

  fprintf(fp, " \"g_debug_in_progress\" : ");
  if(g_debug_in_progress)
    fprintf(fp, "\"true\"");
  else
    fprintf(fp, "\"false\"");
  fprintf(fp, " , ");

  fprintf(fp, " \"g_debug_cont\" : ");
  print_object_ptr_reference(fp, g_debug_cont, false);
  fprintf(fp, "  ");

  if(g_handler_environment)
  {
    fprintf(fp, ",  \"g_handler_environment\" : ");
    g_sub_type = EXCEPTION_HANDLER_PTR;
    print_native_ptr_reference(fp, STACK_TYPE_PTR, (void *)g_handler_environment);
    g_sub_type = NONE;
    fprintf(fp, "  ");
  }

  if(g_signalling_environment)
  {
    fprintf(fp, ",  \"g_signalling_environment\" : ");
    g_sub_type = EXCEPTION_HANDLER_PTR;
    print_native_ptr_reference(fp, STACK_TYPE_PTR, (void *)g_signalling_environment);
    g_sub_type = NONE;
    fprintf(fp, "  ");
  }

  fprintf(fp, ",  \"g_nof_compiler_states\" : %d", g_nof_compiler_states);
  fprintf(fp, "  ");

  fprintf(fp, " } ");
}

//need a struct version of convert_heap() too

void create_image(char *file_name)
{
  FILE *fp = fopen(file_name, "w");  

  obj_print_queue = queue_create();
  obj_hashtable = hashtable_create(1000001);
  printed_objects = hashtable_create(1000001);

  native_ptr_print_queue = queue_create();
  native_ptr_hashtable = hashtable_create(1000001);
  printed_native_objects = hashtable_create(1000001);
  
  fprintf(fp, "{ ");

  print_global_variables(fp);

  fprintf(fp, ",");

  print_native_functions(fp);

  //heap for structs
  fprintf(fp, ", \"native_heap\" : [");

  while(!queue_is_empty(native_ptr_print_queue))
  {
    queue_item_t *queue_item = queue_dequeue(native_ptr_print_queue);
    native_ptr_slot_t *s = (native_ptr_slot_t *)queue_item->data;
    print_native_ptr_heap_representation(fp, s->ref, s->type);
    if(!queue_is_empty(native_ptr_print_queue))fprintf(fp, ", ");
  }

  queue_delete(native_ptr_print_queue);
  hashtable_delete(native_ptr_hashtable);
  hashtable_delete(printed_native_objects);

  fprintf(fp, "] ");
  //end of heap for structs

  //OBJECT_PTR heap
  fprintf(fp, ", \"object_heap\" : [");

  BOOLEAN beginning = true;

  BOOLEAN message_send_native_fn_obj;

  while(!queue_is_empty(obj_print_queue))
  {
    queue_item_t *queue_item = queue_dequeue(obj_print_queue);
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

  queue_delete(obj_print_queue);
  hashtable_delete(obj_hashtable);
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
          IS_CLASS_OBJECT(obj);     /*  ||
          IS_NATIVE_FN_OBJECT(obj);*/
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
    hashtable_entry_t *e = hashtable_get(obj_hashtable, (void *)obj);

    if(e)
      fprintf(fp, "%d", (int)e->value);
    else
    {
      fprintf(fp, "%lu",  ((obj_count) << OBJECT_SHIFT) + (obj & BIT_MASK));
      hashtable_put(obj_hashtable, (void *)obj, (void *)  ((obj_count) << OBJECT_SHIFT) + (obj & BIT_MASK) );
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
    {
      if(IS_NATIVE_FN_OBJECT(obj))
      {
	nativefn nf = get_nativefn_value(obj);
	if(nf != (nativefn)message_send &&
	   nf != (nativefn)message_send_super)
	{
	  BOOLEAN found = false;
	  unsigned int i;
	  for(i=0; i<g_nof_native_fns; i++)
	  {
	    if(g_native_fn_objects[i].nf == nf)
	    {
	      fprintf(fp, "%d", i);
	      found = true;
	      break;
	    }
	  }
	  assert(found);
	}
	else
	  fprintf(fp, "-1"); //for message_send and message_send_super
      }
      else
	fprintf(fp, "%lu", obj);
    }
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
    print_native_ptr_reference(fp, ARRAY_OBJ_PTR, (void *)extract_ptr(obj));
  }
  else if(IS_OBJECT_OBJECT(obj))
  {
    //TODO: this does not handle method_t pointers which
    //are tagged with OBJECT_TAG
    print_native_ptr_reference(fp, OBJ_PTR, (void *)extract_ptr(obj));
  }
  else if(IS_CLASS_OBJECT(obj))
  {
    print_native_ptr_reference(fp, CLASS_OBJ_PTR, (void *)extract_ptr(obj));
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
    //print_object_ptr_reference(fp, arity, single_object);
    fprintf(fp, "%d", get_int_value(arity));
    fprintf(fp, "] ");
  }
  /*
  else if(IS_NATIVE_FN_OBJECT(obj))
  {
    nativefn nf = get_nativefn_value(obj);
    if(nf != (nativefn)message_send &&
       nf != (nativefn)message_send_super)
    {
      BOOLEAN found = false;
      unsigned int i;
      for(i=0; i<nof_native_fns; i++)
      {
	if(native_fn_objects[i].nf == nf)
	{
	  fprintf(fp, "%d", i);
	  found = true;
	  break;
	}
      }
      assert(found);
    }
    //else
    //  fprintf(fp, "-1"); //for message_send and message_send_super
  }
  */
  else if(IS_INTEGER_OBJECT(obj))
    fprintf(fp, "%d", get_int_value(obj));
  else if(IS_FLOAT_OBJECT(obj))
    fprintf(fp, "%lf", get_float_value(obj));
  else
    if(!single_object)assert(false);

  hashtable_put(printed_objects, (void *)obj, (void *)1);
}

void print_native_functions(FILE *fp)
{
  fprintf(fp, "\"native_functions\": ");
  unsigned int i;

  fprintf(fp, "[ ");

  for(i=0; i<g_nof_native_fns; i++)
  {
    fprintf(fp, "[ ");
    fprintf(fp, "%d, ", g_native_fn_objects[i].state_index);
    fprintf(fp, "\"%s\", ", g_native_fn_objects[i].fname);

    char *src = g_native_fn_objects[i].source;

    if(src)
    {
      unsigned int j, len = strlen(src);

      fprintf(fp, "\"");

      for(j=0; j<len; j++)
      {
	if(src[j] == '\n')
	  fprintf(fp, "\\n");
	else
	  fprintf(fp, "%c", src[j]);
      }
      fprintf(fp, "\"");
    }
    else
      fprintf(fp, "NULL");

    fprintf(fp, "]");

    if(i != g_nof_native_fns - 1)
      fprintf(fp, ", ");
  }

  fprintf(fp, "] ");
}

char *get_json_core_symbol(struct JSONObject *native_heap, int index)
{
  struct JSONObject *core_symbols = JSON_get_array_item(native_heap, 1);
  struct JSONObject *core_symbols_array = JSON_get_array_item(core_symbols, 1);
  assert(index >= 0 && index < core_symbols_array->array->count);
  return core_symbols_array->array->elements[index]->strvalue;
}

char *get_json_smalltalk_symbol(struct JSONObject *native_heap, int index)
{
  struct JSONObject *smalltalk_symbols = JSON_get_array_item(native_heap, 0);
  struct JSONObject *smalltalk_symbols_array = JSON_get_array_item(smalltalk_symbols, 1);
  assert(index >= 0 && index < smalltalk_symbols_array->array->count);
  return smalltalk_symbols_array->array->elements[index]->strvalue;
}

/* void add_to_deserialization_queue_old(struct JSONObject *heap, queue_t *q, OBJECT_PTR ref, uintptr_t ptr, unsigned int index) */
/* { */
/*   struct slot *s = (struct slot *)GC_MALLOC(sizeof(struct slot)); */
/*   s->ref = ref; */
/*   s->ptr = ptr; */
/*   s->index = index; */
/*   queue_enqueue(q, s); */
/* } */

//enqueues a member pointer (ref) for a native struct object (ptr).
//the type indicates the struct object's type (e.g., object_t *),
//the index the ordinal position of the member pointer in the struct
//object's definition. this is quite brittle as we are counting
//on the fact the the struct definition will not change; ideally
//the index should be replaced by an enum
void add_to_deserialization_queue(queue_t *q,
				  OBJECT_PTR ref,
				  uintptr_t ptr,
				  enum PointerType type,
				  unsigned int index1,
				  unsigned int index2)
{
  struct slot *s = (struct slot *)GC_MALLOC(sizeof(struct slot));
  s->ref = ref;
  s->ptr = ptr;
  s->type = type;
  s->index1 = index1;
  s->index2 = index2;
  queue_enqueue(q, s);
}

//sets all the object references in the passed queue
//to OBJECT_PTR or native pointer values (either from the
//hashtable cache or by invoking deserialize_object_reference()
//or deserialize_native_ptr_reference())
void convert_heap(struct JSONObject *object_heap,
		  struct JSONObject *native_heap,
		  hashtable_t *obj_ht,
		  hashtable_t *native_ptr_ht,
		  queue_t *q)
{
  //TODO
  while(!queue_is_empty(q))
  {
    queue_item_t *queue_item = queue_dequeue(q);
    struct slot *slot_obj = (struct slot *)(queue_item->data);

    OBJECT_PTR ref = slot_obj->ref;

    enum PointerType type = slot_obj->type;

    unsigned int index1 = slot_obj->index1;
    unsigned int index2 = slot_obj->index2;

    if(type == NONE) //it's an OBJECT_PTR
    {
      hashtable_entry_t *e = hashtable_get(obj_ht, (void *)ref);

      if(e)
	set_heap(slot_obj->ptr, slot_obj->index1, (OBJECT_PTR)e->value);
      else
      {
	OBJECT_PTR obj = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	set_heap(slot_obj->ptr, slot_obj->index1, obj);
      }
    }
    else
    {
      if(type == OBJ_PTR)
      {
	object_t *obj = (object_t *)slot_obj->ptr;

	if(index1 == 0) //object_t.class_object
	{
	  hashtable_entry_t *e = hashtable_get(obj_ht, (void *)ref);

	  if(e)
	    obj->class_object = (OBJECT_PTR)e->value;
	  else
	  {
	    OBJECT_PTR cls_obj = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	    obj->class_object = cls_obj;
	  }
	}
	else if(index1 == 1) //object_t.instance_vars
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    obj->instance_vars = (binding_env_t *)e->value;
	  else
	  {
	    binding_env_t *env = (binding_env_t *)deserialize_native_ptr_reference(object_heap,
										   native_heap,
										   BINDING_ENV_PTR,
										   ref,
										   obj_ht,
										   native_ptr_ht,
										   q);
	    obj->instance_vars = env;
	  }
	}
	else
	  assert(false);
      }
      else if(type == CLASS_OBJ_PTR)
      {
	class_object_t *cls_obj = (class_object_t *)slot_obj->ptr;

	if(index1 == 0) //cls_obj->parent_class_object
	{
	  hashtable_entry_t *e = hashtable_get(obj_ht, (void *)ref);

	  if(e)
	    cls_obj->parent_class_object = (OBJECT_PTR)e->value;
	  else
	  {
	    OBJECT_PTR parent_cls_obj = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	    cls_obj->parent_class_object = parent_cls_obj;
	  }
	}
	//index1 = 1 is the class name, which doesn't need a call to deserialize
	else if(index1 == 2) //cls_obj->package
	{
	  hashtable_entry_t *e = hashtable_get(obj_ht, (void *)ref);

	  if(e)
	    cls_obj->package = (OBJECT_PTR)e->value;
	  else
	  {
	    OBJECT_PTR pkg = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	    cls_obj->package = pkg;
	  }
	}
	//index1 = 3 is the number of instances, which doesn't need a call to deserialize
	else if(index1 == 4) //cls_obj->instances
	{
	  hashtable_entry_t *e = hashtable_get(obj_ht, (void *)ref);

	  if(e)
	    cls_obj->instances[index2] = (OBJECT_PTR)e->value;
	  else
	  {
	    OBJECT_PTR instance = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	    cls_obj->instances[index2] = instance;
	  }
	}
	//index1 = 5 is the number of instance variables, which doesn't need a call to deserialize
	//index1 = 6 is the instance variable names, which don't need a call to deserialize
	else if(index1 == 7) //cls_obj->shared_vars
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    cls_obj->shared_vars = (binding_env_t *)e->value;
	  else
	  {
	    binding_env_t *env = (binding_env_t *)deserialize_native_ptr_reference(object_heap,
										   native_heap,
										   BINDING_ENV_PTR,
										   ref,
										   obj_ht,
										   native_ptr_ht,
										   q);
	    cls_obj->shared_vars = env;
	  }
	}
	else if(index1 == 8) //cls_obj->instance_methods
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    cls_obj->instance_methods = (method_binding_env_t *)e->value;
	  else
	  {
	    method_binding_env_t *env = (method_binding_env_t *)deserialize_native_ptr_reference(object_heap,
												 native_heap,
												 METHOD_BINDING_ENV_PTR,
												 ref,
												 obj_ht,
												 native_ptr_ht,
												 q);
	    cls_obj->instance_methods = env;
	  }
	}
	else if(index1 == 9) //cls_obj->class_methods
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    cls_obj->class_methods = (method_binding_env_t *)e->value;
	  else
	  {
	    method_binding_env_t *env = (method_binding_env_t *)deserialize_native_ptr_reference(object_heap,
												 native_heap,
												 METHOD_BINDING_ENV_PTR,
												 ref,
												 obj_ht,
												 native_ptr_ht,
												 q);
	    cls_obj->class_methods = env;
	  }
	}
	else
	  assert(false);
      }
      else if(type == ARRAY_OBJ_PTR)
      {
	array_object_t *arr_obj = (array_object_t *)slot_obj->ptr;

	if(index1 == 1) //arr_obj->elements
	{
	  hashtable_entry_t *e = hashtable_get(obj_ht, (void *)ref);

	  if(e)
	    arr_obj->elements[index2] = (OBJECT_PTR)e->value;
	  else
	  {
	    OBJECT_PTR element = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	    arr_obj->elements[index2] = element;
	  }
	}
	else
	  assert(false);
      }
      else if(type == BINDING_ENV_PTR)
      {
	binding_env_t *env = (binding_env_t *)slot_obj->ptr;

	if(index1 == 1) //env->bindings
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    env->bindings[index2] = *((binding_t *)e->value);
	  else
	  {
	    binding_t *binding = (binding_t *)deserialize_native_ptr_reference(object_heap,
									       native_heap,
									       BINDING_PTR,
									       (uintptr_t)ref,
									       obj_ht,
									       native_ptr_ht,
									       q);
	    env->bindings[index2] = *binding;
	  }
	}
	else
	  assert(false);
      }
      else if(type == BINDING_PTR)
      {
	binding_t *binding = (binding_t *)slot_obj->ptr;

	if(index1 == 1) //binding->key
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    binding->key = (OBJECT_PTR)e->value;
	  else
	  {
	    OBJECT_PTR key = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	    binding->key = key;
	  }
	}
	else if(index1 == 2) //binding->val
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    binding->val = (OBJECT_PTR)e->value;
	  else
	  {
	    OBJECT_PTR val = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	    binding->val = val;
	  }
	}
	else
	  assert(false);
      }
      else if(type == METHOD_BINDING_ENV_PTR)
      {
	method_binding_env_t *env = (method_binding_env_t *)slot_obj->ptr;

	if(index1 == 1) //env->bindings
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    env->bindings[index2] = *((method_binding_t *)e->value);
	  else
	  {
	    method_binding_t *binding = (method_binding_t *)deserialize_native_ptr_reference(object_heap,
											     native_heap,
											     METHOD_BINDING_PTR,
											     (uintptr_t)ref,
											     obj_ht,
											     native_ptr_ht,
											     q);
	    env->bindings[index2] = *binding;
	  }
	}
	else
	  assert(false);
      }
      else if(type == METHOD_BINDING_PTR)
      {
	method_binding_t *binding = (method_binding_t *)slot_obj->ptr;

	if(index1 == 1) //binding->key
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    binding->key = (OBJECT_PTR)e->value;
	  else
	  {
	    OBJECT_PTR key = deserialize_object_reference(object_heap, native_heap, ref, obj_ht, native_ptr_ht, q);
	    binding->key = key;
	  }
	}
	else if(index1 == 2) //binding->val
	{
	  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

	  if(e)
	    binding->val = (method_t *)e->value;
	  else
	  {
	    method_t *m = deserialize_native_ptr_reference(object_heap,
							   native_heap,
							   METHOD_PTR,
							   ref,
							   obj_ht,
							   native_ptr_ht,
							   q);
	    binding->val = m;
	  }
	}
	else
	  assert(false);
      }
      else if(type == EXPRESSION_PTR)
      {
	expression_t *exp = (expression_t *)slot_obj->ptr;

      }
      //TODO: other types
    }
  }
}

//given a reference (ivalue) to a native_heap JSON entry,
//this function converts it to a native pointer.
//It enqueues the internal references
//that this reference may have, for processing by convert_heap()
void *deserialize_native_ptr_reference(struct JSONObject *object_heap,
				       struct JSONObject *native_heap,
				       enum PointerType ptr_type,
				       OBJECT_PTR ref, //this should match the type of JSONObject->ivalue
				       hashtable_t *obj_ht,
				       hashtable_t *native_ptr_ht,
				       queue_t *queue)
{
  int i;

  hashtable_entry_t *e = hashtable_get(native_ptr_ht, (void *)ref);

  if(e)
    return e->value;

  struct JSONObject *ptr_entry = JSON_get_array_item(native_heap, ref);

  if(ptr_type == BINDING_ENV_PTR)
  {
    binding_env_t *env = (binding_env_t *)GC_MALLOC(sizeof(binding_env_t));

    env->count = JSON_get_array_size(ptr_entry);
    env->bindings = (binding_t *)GC_MALLOC(env->count * sizeof(binding_t));

    for(i=0; i<env->count; i++)
    {
      long long ref1 = JSON_get_array_item(ptr_entry,i)->ivalue;
      hashtable_entry_t *e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
	env->bindings[i] = *((binding_t *)e1->value);
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)env->bindings+i, BINDING_PTR, 1, i);
    }
    return (void *)env;
  }
  else if(ptr_type == BINDING_PTR)
  {
    binding_t *binding = (binding_t *)GC_MALLOC(sizeof(binding_env_t));

    hashtable_entry_t *e1;

    //key
    long long ref1 = JSON_get_array_item(ptr_entry,0)->ivalue;
    e1 = hashtable_get(obj_ht, (void *)ref1);

    if(e1)
      binding->key = (OBJECT_PTR)e1->value;
    else
      add_to_deserialization_queue(queue, ref1, (uintptr_t)binding->key, NONE, 0, 0);
    //end key

    //val
    long long ref2 = JSON_get_array_item(ptr_entry,1)->ivalue;
    e1 = hashtable_get(obj_ht, (void *)ref2);

    if(e1)
      binding->val = (OBJECT_PTR)e1->value;
    else
      add_to_deserialization_queue(queue, ref2, (uintptr_t)binding->val, NONE, 0, 0);
    //end val

    return (void *)binding;
  }
  else if(ptr_type == METHOD_BINDING_ENV_PTR)
  {
    method_binding_env_t *env = (method_binding_env_t *)GC_MALLOC(sizeof(method_binding_env_t));

    env->count = JSON_get_array_size(ptr_entry);
    env->bindings = (method_binding_t *)GC_MALLOC(env->count * sizeof(method_binding_t));

    for(i=0; i<env->count; i++)
    {
      long long ref1 = JSON_get_array_item(ptr_entry,i)->ivalue;
      hashtable_entry_t *e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
	env->bindings[i] = *((method_binding_t *)e1->value);
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)env->bindings+i, METHOD_BINDING_PTR, 1, i);
    }
    return (void *)env;
  }
  else if(ptr_type == METHOD_BINDING_PTR)
  {
    method_binding_t *binding = (method_binding_t *)GC_MALLOC(sizeof(method_binding_t));

    hashtable_entry_t *e1;

    //key
    long long ref1 = JSON_get_array_item(ptr_entry,0)->ivalue;
    e1 = hashtable_get(obj_ht, (void *)ref1);

    if(e1)
      binding->key = (OBJECT_PTR)e1->value;
    else
      add_to_deserialization_queue(queue, ref1, (uintptr_t)binding->key, NONE, 0, 0);
    //end key

    //val
    long long ref2 = JSON_get_array_item(ptr_entry,1)->ivalue;
    e1 = hashtable_get(native_ptr_ht, (void *)ref2);

    if(e1)
      binding->val = (method_t *)e1->value;
    else
      add_to_deserialization_queue(queue, ref2, (uintptr_t)binding->val, METHOD_PTR, 0, 0);
    //end val

    return (void *)binding;
  }
  else if(ptr_type == EXPRESSION_PTR)
  {
    expression_t *exp = (expression_t *)GC_MALLOC(sizeof(expression_t));

    hashtable_entry_t *e1;

    if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "ASSIGNMENT"))
    {
      exp->type = ASSIGNMENT;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        exp->asgn = (assignment_t *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)exp->asgn, ASSIGNMENT_PTR, 0, 0);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "BASIC_EXPRESSION"))
    {
      exp->type = BASIC_EXPRESSION;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        exp->basic_exp = (basic_expression_t *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)exp->basic_exp, BASIC_EXPRESSION_PTR, 0, 0);
    }
    else
      assert(false);

    return (void *)exp;
  }
  else if(ptr_type == RETURN_STATEMENT_PTR)
  {
    return_statement_t *ret_stmt = (return_statement_t *)GC_MALLOC(sizeof(return_statement_t));

    hashtable_entry_t *e1;

    long long ref1 = JSON_get_array_item(ptr_entry, 0)->ivalue;

    e1 = hashtable_get(native_ptr_ht, (void *)ref1);

    if(e1)
      ret_stmt->exp = (expression_t *)e1->value;
    else
      add_to_deserialization_queue(queue, ref1, (uintptr_t)ret_stmt->exp, EXPRESSION_PTR, 0, 0);

    return (void *)ret_stmt;
  }
  else if(ptr_type == PRIMARY_PTR)
  {
    primary_t *prim = (primary_t *)GC_MALLOC(sizeof(primary_t));

    hashtable_entry_t *e1;

    if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "IDENTIFIER"))
    {
      prim->type = IDENTIFIER;

      prim->identifier = GC_strdup(JSON_get_array_item(ptr_entry, 1)->strvalue);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "LITERAL"))
    {
      prim->type = LITERAL;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        prim->lit = (struct literal *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)prim->lit, LITERAL_PTR, 0, 0);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "BLOCK_CONSTRUCTOR"))
    {
      prim->type = BLOCK_CONSTRUCTOR;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(obj_ht, (void *)ref1);

      if(e1)
        prim->blk_cons = (struct block_constructor *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)prim->blk_cons, BLOCK_CONSTRUCTOR_PTR, 0, 0);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "EXPRESSION"))
    {
      prim->type = EXPRESSION1;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        prim->exp = (struct expression *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)prim->exp, EXPRESSION_PTR, 0, 0);
    }
    else
      assert(false);

    return (void *)prim;
  }
  else if(ptr_type == MESSAGE_PTR)
  {
    message_t *msg = (message_t *)GC_MALLOC(sizeof(message_t));

    hashtable_entry_t *e1;

    if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "UNARY_MESSAGE"))
    {
      msg->type = UNARY_MESSAGE;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        msg->unary_messages = (struct unary_messages *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)msg->unary_messages, UNARY_MESSAGES_PTR, 0, 0);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "BINARY_MESSAGE"))
    {
      msg->type = BINARY_MESSAGE;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        msg->binary_messages = (struct binary_messages *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)msg->binary_messages, BINARY_MESSAGES_PTR, 0, 0);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "KEYWORD_MESSAGE"))
    {
      msg->type = KEYWORD_MESSAGE;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        msg->kw_msg = (struct keyword_message *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)msg->kw_msg, KEYWORD_MESSAGE_PTR, 0, 0);
    }
    else
      assert(false);

    return (void *)msg;
  }
  else if(ptr_type == CASCADED_MESSAGES_PTR)
  {
    cascaded_messages_t *casc_msgs = (cascaded_messages_t *)GC_MALLOC(sizeof(cascaded_messages_t));

    hashtable_entry_t *e1;

    casc_msgs->nof_cascaded_msgs = JSON_get_array_size(ptr_entry);

    casc_msgs->cascaded_msgs = (struct message *)GC_MALLOC(casc_msgs->nof_cascaded_msgs * sizeof(struct message));

    for(i=0; i<casc_msgs->nof_cascaded_msgs; i++)
    {
      long long ref1 = JSON_get_array_item(ptr_entry, i)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        casc_msgs->cascaded_msgs[i] = *((struct message *)e1->value);
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)casc_msgs->cascaded_msgs+i, MESSAGE_PTR, 0, 0);
    }

    return (void *)casc_msgs;
  }
  else if(ptr_type == LITERAL_PTR)
  {
    literal_t *lit = (literal_t *)GC_MALLOC(sizeof(literal_t));

    hashtable_entry_t *e1;

    if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "NUMBER_LITERAL"))
    {
      lit->type = NUMBER_LITERAL;
      lit->val = GC_strdup(JSON_get_array_item(ptr_entry, 1)->strvalue);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "STRING_LITERAL"))
    {
      lit->type = STRING_LITERAL;
      lit->val = GC_strdup(JSON_get_array_item(ptr_entry, 1)->strvalue);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "CHAR_LITERAL"))
    {
      lit->type = CHAR_LITERAL;
      lit->val = GC_strdup(JSON_get_array_item(ptr_entry, 1)->strvalue);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "SYMBOL_LITERAL"))
    {
      lit->type = SYMBOL_LITERAL;
      lit->val = GC_strdup(JSON_get_array_item(ptr_entry, 1)->strvalue);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "SELECTOR_LITERAL"))
    {
      lit->type = SELECTOR_LITERAL;
      lit->val = GC_strdup(JSON_get_array_item(ptr_entry, 1)->strvalue);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "ARRAY_LITERAL"))
    {
      lit->type = ARRAY_LITERAL;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        lit->array_elements = (struct array_elements *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)lit->array_elements, ARRAY_ELEMENTS_PTR, 0, 0);
    }
    else
      assert(false);

    return (void *)lit;
  }
  else if(ptr_type == BLOCK_CONSTRUCTOR_PTR)
  {
    block_constructor_t *cons = (block_constructor_t *)GC_MALLOC(sizeof(block_constructor_t));

    hashtable_entry_t *e1;

    if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "BLOCK_ARGS"))
    {
      cons->type = BLOCK_ARGS;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        cons->block_args = (struct block_arguments *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)cons->block_args, BLOCK_ARGUMENT_PTR, 0, 0);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "NO_BLOCK_ARGS"))
    {
      cons->type = NO_BLOCK_ARGS;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        cons->exec_code = (struct executable_code *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)cons->exec_code, EXEC_CODE_PTR, 0, 0);
    }

    return (void *)cons;
  }
  else if(ptr_type == BLOCK_ARGUMENT_PTR)
  {
    block_arguments_t *args = (block_arguments_t *)GC_MALLOC(sizeof(block_arguments_t));

    args->nof_args = JSON_get_array_size(ptr_entry);
    args->identifiers = (char **)GC_MALLOC(args->nof_args * sizeof(char *));

    for(i=0; i<args->nof_args; i++)
      args->identifiers[i] = GC_strdup(JSON_get_array_item(ptr_entry, i)->strvalue);

    return (void *)args;
  }
  else if(ptr_type == BINARY_MESSAGES_PTR)
  {
    binary_messages_t *bin_msgs = (binary_messages_t *)GC_MALLOC(sizeof(binary_messages_t));

    hashtable_entry_t *e1;

    bin_msgs->nof_messages = JSON_get_array_size(ptr_entry);
    bin_msgs->bin_msgs = (struct binary_message *)GC_MALLOC(bin_msgs->nof_messages * sizeof(struct binary_message));

    for(i=0; i<bin_msgs->nof_messages; i++)
    {
      long long ref1 = JSON_get_array_item(ptr_entry, i)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        bin_msgs->bin_msgs[i] = *((struct binary_message *)e1->value);
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)bin_msgs->bin_msgs+i, BINARY_MESSAGE_PTR, 0, 0);
    }

    return (void *)bin_msgs;
  }
  else if(ptr_type == BINARY_MESSAGE_PTR)
  {
    binary_message_t *bin_msg = (binary_message_t *)GC_MALLOC(sizeof(binary_message_t));

    hashtable_entry_t *e1;

    bin_msg->binary_selector = GC_strdup(JSON_get_array_item(ptr_entry, 0)->strvalue);

    long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
    e1 = hashtable_get(native_ptr_ht, (void *)ref1);

    if(e1)
      bin_msg->bin_arg = (struct binary_argument *)e1->value;
    else
      add_to_deserialization_queue(queue, ref1, (uintptr_t)bin_msg->bin_arg, BINARY_ARGUMENT_PTR, 0, 0);

    return (void *)bin_msg;
  }
  else if(ptr_type == KEYWORD_MESSAGE_PTR)
  {
    keyword_message_t *kw_msg = (keyword_message_t *)GC_MALLOC(sizeof(keyword_message_t));

    hashtable_entry_t *e1;

    kw_msg->nof_args = JSON_get_array_size(ptr_entry);
    kw_msg->kw_arg_pairs = (struct keyword_argument_pair *)GC_MALLOC(kw_msg->nof_args * sizeof(struct keyword_argument_pair));

    for(i=0; i<kw_msg->nof_args; i++)
    {
      long long ref1 = JSON_get_array_item(ptr_entry, i)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        kw_msg->kw_arg_pairs[i] = *((struct keyword_argument_pair *)e1->value);
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)kw_msg->kw_arg_pairs+i, KEYWORD_ARGUMENT_PAIR_PTR, 0, 0);
    }

    return (void *)kw_msg;
  }
  else if(ptr_type == BINARY_ARGUMENT_PTR)
  {
    binary_argument_t *bin_arg = (binary_argument_t *)GC_MALLOC(sizeof(binary_argument_t));

    hashtable_entry_t *e1;

    long long ref1 = JSON_get_array_item(ptr_entry, 0)->ivalue;
    e1 = hashtable_get(native_ptr_ht, (void *)ref1);

    if(e1)
      bin_arg->prim = (struct primary *)e1->value;
    else
      add_to_deserialization_queue(queue, ref1, (uintptr_t)bin_arg->prim, PRIMARY_PTR, 0, 0);

    long long ref2 = JSON_get_array_item(ptr_entry, 1)->ivalue;
    e1 = hashtable_get(native_ptr_ht, (void *)ref2);

    if(e1)
      bin_arg->unary_messages = (struct unary_messages *)e1->value;
    else
      add_to_deserialization_queue(queue, ref2, (uintptr_t)bin_arg->unary_messages, UNARY_MESSAGES_PTR, 0, 0);

    return (void *)bin_arg;
  }
  else if(ptr_type == KEYWORD_ARGUMENT_PAIR_PTR)
  {
    keyword_argument_pair_t *kw_arg_pair = (keyword_argument_pair_t *)GC_MALLOC(sizeof(keyword_argument_pair_t));

    hashtable_entry_t *e1;

    kw_arg_pair->keyword = GC_strdup(JSON_get_array_item(ptr_entry, 0)->strvalue);

    long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
    e1 = hashtable_get(native_ptr_ht, (void *)ref1);

    if(e1)
      kw_arg_pair->kw_arg = (struct keyword_argument *)e1->value;
    else
      add_to_deserialization_queue(queue, ref1, (uintptr_t)kw_arg_pair->kw_arg, KEYWORD_ARGUMENT_PTR, 0, 0);

    return (void *)kw_arg_pair;
  }
  else if(ptr_type == TEMPORARIES_PTR)
  {
    temporaries_t *temps = (temporaries_t *)GC_MALLOC(sizeof(temporaries_t));

    temps->nof_temporaries = JSON_get_array_size(ptr_entry);
    temps->temporaries = (char **)GC_MALLOC(temps->nof_temporaries * sizeof(char *));

    for(i=0; i<temps->nof_temporaries; i++)
      temps->temporaries[i] = GC_strdup(JSON_get_array_item(ptr_entry, i)->strvalue);

    return (void *)temps;
  }
  else if(ptr_type == STATEMENTS_PTR)
  {
    statement_t *stmt = (statement_t *)GC_MALLOC(sizeof(statement_t));

    hashtable_entry_t *e1;

    if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "RETURN_STATEMENT"))
    {
      stmt->type = RETURN_STATEMENT;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        stmt->ret_stmt = (struct return_statement *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)stmt->ret_stmt, RETURN_STATEMENT_PTR, 0, 0);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "EXPRESSION"))
    {
      stmt->type = EXPRESSION;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        stmt->exp = (struct expression *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)stmt->exp, EXPRESSION_PTR, 0, 0);
    }
    else if(!strcmp(JSON_get_array_item(ptr_entry, 0)->strvalue, "EXP_PLUS_STATEMENTS"))
    {
      stmt->type = EXP_PLUS_STATEMENTS;

      long long ref1 = JSON_get_array_item(ptr_entry, 1)->ivalue;
      e1 = hashtable_get(native_ptr_ht, (void *)ref1);

      if(e1)
        stmt->statements = (struct statement *)e1->value;
      else
	add_to_deserialization_queue(queue, ref1, (uintptr_t)stmt->statements, STATEMENTS_PTR, 0, 0);
    }
    else
      assert(false);

    return (void *)stmt;
  }
  else if(ptr_type == IDENTIFIERS_PTR)
  {
    identifiers_t *ids = (identifiers_t *)GC_MALLOC(sizeof(identifiers_t));

    ids->nof_identifiers = JSON_get_array_size(ptr_entry);
    ids->identifiers = (char **)GC_MALLOC(ids->nof_identifiers * sizeof(char *));

    for(i=0; i<ids->nof_identifiers; i++)
      ids->identifiers[i] = GC_strdup(JSON_get_array_item(ptr_entry, i)->strvalue);

    return (void *)ids;
  }
  else if(ptr_type == EXEC_CODE_PTR)
  {
    executable_code_t *exec_code = (executable_code_t *)GC_MALLOC(sizeof(executable_code_t));

    hashtable_entry_t *e1;

    long long ref1 = JSON_get_array_item(ptr_entry, 0)->ivalue;
    e1 = hashtable_get(native_ptr_ht, (void *)ref1);

    if(e1)
      exec_code->temporaries = (temporaries_t *)e1->value;
    else
      add_to_deserialization_queue(queue, ref1, (uintptr_t)exec_code->temporaries, TEMPORARIES_PTR, 0, 0);

    long long ref2 = JSON_get_array_item(ptr_entry, 1)->ivalue;
    e1 = hashtable_get(native_ptr_ht, (void *)ref2);

    if(e1)
      exec_code->statements = (struct statement *)e1->value;
    else
      add_to_deserialization_queue(queue, ref2, (uintptr_t)exec_code->statements, STATEMENTS_PTR, 0, 0);

    return (void *)exec_code;
  }
  else if(ptr_type == PACKAGE_PTR)
  {
    package_t *pkg = (package_t *)GC_MALLOC(sizeof(package_t));

    pkg->name = GC_strdup(JSON_get_array_item(ptr_entry, 0)->strvalue);

    struct JSONObject *symbols = JSON_get_array_item(ptr_entry,1);

    pkg->nof_symbols = JSON_get_array_size(symbols);
    pkg->symbols = (char **)GC_MALLOC(pkg->nof_symbols * sizeof(char *));

    for(i=0; i<pkg->nof_symbols; i++)
      pkg->symbols[i] = GC_strdup(JSON_get_array_item(symbols, i)->strvalue);

    return (void *)pkg;
  }
  else if(ptr_type == STACK_TYPE_PTR)
  {
    stack_type *stack = (stack_type *)GC_MALLOC(sizeof(stack_type));

    enum PointerType stack_content_type = JSON_get_array_item(ptr_entry, 0)->ivalue;

    struct JSONObject *stack_data = JSON_get_array_item(ptr_entry, 1);

    stack->count = JSON_get_array_size(stack_data);
    stack->data = (void **)GC_MALLOC(stack->count * sizeof(void *));

    if(stack_content_type == OBJECT_PTR1)
    {
      for(i=0; i<stack->count; i++)
	stack->data[i] = (void *)deserialize_object_reference(object_heap,
							      native_heap,
							      JSON_get_array_item(stack_data, i)->ivalue,
							      obj_ht,
							      native_ptr_ht,
							      queue);
    }
    else
    {
      for(i=0; i<stack->count; i++)
	stack->data[i] = deserialize_native_ptr_reference(object_heap,
							  native_heap,
							  stack_content_type,
							  JSON_get_array_item(stack_data, i)->ivalue,
							  obj_ht,
							  native_ptr_ht,
							  queue);
    }

    return (void *)stack;
  }

  return NULL; //TODO
}

//given a reference (ivalue) to an object_heap JSON entry,
//this function converts it to an OBJECT_PTR.
//It enqueues the internal references
//that this reference may have, for processing by convert_heap()
OBJECT_PTR deserialize_object_reference(struct JSONObject *object_heap,
					struct JSONObject *native_heap,
					OBJECT_PTR ref, //this should match the type of JSONObject->ivalue
					hashtable_t *obj_ht,
					hashtable_t *native_ptr_ht,
					queue_t *queue)
{
  int i;

  unsigned int object_type = ref & BIT_MASK;

  //it is sufficient to just return ref
  //if it's not a dynamic memory object.
  //the assumption is that the same symbol/string indexes
  //hold good. the other types are constant.
  if(object_type == SYMBOL_TAG         ||
     object_type == CHARACTER_TAG      ||
     object_type == STRING_TAG         ||
     object_type == INTEGER_TAG        ||
     object_type == STRING_LITERAL_TAG ||
     object_type == TRUE_TAG           ||
     object_type == FALSE_TAG          ||
     object_type == SMALLTALK_SYMBOL_TAG)
    return ref;

  //TODO: FLOAT_TAG

  hashtable_entry_t *e = hashtable_get(obj_ht, (void *)ref);

  if(e)
    return (OBJECT_PTR)e->value;

  struct JSONObject *heap_obj = JSON_get_array_item(object_heap, ref >> OBJECT_SHIFT);

  uintptr_t ptr;

  if(object_type == CONS_TAG)
  {
    ptr = object_alloc(2, CONS_TAG);

    OBJECT_PTR car_ref = JSON_get_array_item(heap_obj, 0)->ivalue;
    OBJECT_PTR cdr_ref = JSON_get_array_item(heap_obj, 1)->ivalue;

    if(is_dynamic_memory_object(car_ref))
    {
      hashtable_entry_t *e = hashtable_get(obj_ht, (void *)car_ref);
      if(e)
        set_heap(ptr, 0, (OBJECT_PTR)(e->value));
      else
        add_to_deserialization_queue(queue, car_ref, ptr, NONE, 0, 0);
    }
    else
      set_heap(ptr, 0, car_ref);

    if(is_dynamic_memory_object(cdr_ref))
    {
      hashtable_entry_t *e = hashtable_get(obj_ht, (void *)cdr_ref);
      if(e)
        set_heap(ptr, 1, (OBJECT_PTR)(e->value));
      else
        add_to_deserialization_queue(queue, cdr_ref, ptr, NONE, 1, 0);
    }
    else
      set_heap(ptr, 1, cdr_ref);

    return ptr + object_type;
  }
  else if(object_type == OBJECT_TAG)
  {
    hashtable_entry_t *e1;

    object_t *obj = (object_t *)GC_MALLOC(sizeof(object_t));

    /* class_object */
    //we index into the native heap to get the object_t 'pointer',
    //and then take the first element of the array there (which maps
    //to the object_t struct)
    OBJECT_PTR cls_obj = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
					     0)->ivalue;

    e1 = hashtable_get(native_ptr_ht, (void *)cls_obj);

    if(e1)
      obj->class_object = (OBJECT_PTR)e1->value;
    else
      add_to_deserialization_queue(queue, cls_obj, (uintptr_t)obj, OBJ_PTR, 0, 0);
    /* end of class_object */

    /* instance_vars */
    OBJECT_PTR binding_env = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						 1)->ivalue;

    e1 = hashtable_get(native_ptr_ht, (void *)binding_env);

    if(e1)
      obj->class_object = (OBJECT_PTR)e1->value;
    else
      add_to_deserialization_queue(queue, binding_env, (uintptr_t)obj, OBJ_PTR, 1, 0);
    /* end of instance_vars */

    return (uintptr_t)obj + object_type;
  }
  else if(object_type == CLASS_OBJECT_TAG)
  {
    hashtable_entry_t *e1;

    class_object_t *cls_obj = (class_object_t *)GC_MALLOC(sizeof(class_object_t));

    /* parent_class_object */
    OBJECT_PTR parent_cls_obj = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						    0)->ivalue;

    e1 = hashtable_get(native_ptr_ht, (void *)parent_cls_obj);

    if(e1)
      cls_obj->parent_class_object = (OBJECT_PTR)e1->value;
    else
      add_to_deserialization_queue(queue, parent_cls_obj, (uintptr_t)cls_obj, CLASS_OBJ_PTR, 0, 0);
    /* end of parent_class_object */

    /* name */
    cls_obj->name = GC_strdup(JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						  1)->strvalue);
    /* package */
    OBJECT_PTR pkg = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
					 2)->ivalue;

    e1 = hashtable_get(native_ptr_ht, (void *)pkg);

    if(e1)
      cls_obj->package = (OBJECT_PTR)e->value;
    else
      add_to_deserialization_queue(queue, pkg, (uintptr_t)cls_obj, CLASS_OBJ_PTR, 2, 0);
    /* end of parent_class_object */

    /* nof_instances */
    cls_obj->nof_instances = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						 3)->ivalue;

    /* instances */
    cls_obj->instances = (OBJECT_PTR *)GC_MALLOC(cls_obj->nof_instances * sizeof(OBJECT_PTR));

    struct JSONObject *instances = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT), 4);
    assert(JSON_get_array_size(instances) == cls_obj->nof_instances);

    for(i=0; i< cls_obj->nof_instances; i++)
    {
      OBJECT_PTR instance = JSON_get_array_item(instances, i)->ivalue;

      e1 = hashtable_get(obj_ht, (void *)instance);

      if(e1)
	cls_obj->instances[i] = (OBJECT_PTR)e1->value;
      else
	add_to_deserialization_queue(queue, cls_obj->instances[i], (uintptr_t)cls_obj, CLASS_OBJ_PTR, 4, i);
    }
    /* end of instances */

    /* nof_inst_vars */
    cls_obj->nof_instance_vars = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						     5)->ivalue;

    /* instance vars */
    cls_obj->inst_vars = (OBJECT_PTR *)GC_MALLOC(cls_obj->nof_instance_vars * sizeof(OBJECT_PTR));

    struct JSONObject *instance_vars = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT), 6);
    assert(JSON_get_array_size(instance_vars) == cls_obj->nof_instance_vars);

    for(i=0; i< cls_obj->nof_instance_vars; i++)
      cls_obj->inst_vars[i] = JSON_get_array_item(instance_vars, i)->ivalue;
    /* end of instance vars */

    /* shared vars */
    OBJECT_PTR shared_vars = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						 7)->ivalue;

    e1 = hashtable_get(native_ptr_ht, (void *)shared_vars);

    if(e1)
      cls_obj->shared_vars = (binding_env_t *)e1->value;
    else
      add_to_deserialization_queue(queue, shared_vars, (uintptr_t)cls_obj, CLASS_OBJ_PTR, 7, 0);
    /* end of shared vars */

    /* instance methods */
    OBJECT_PTR instance_methods = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						      8)->ivalue;

    e1 = hashtable_get(native_ptr_ht, (void *)instance_methods);

    if(e1)
      cls_obj->instance_methods = (method_binding_env_t *)e1->value;
    else
      add_to_deserialization_queue(queue, instance_methods, (uintptr_t)cls_obj, CLASS_OBJ_PTR, 8, 0);
    /* end of instance methods */

    /* class methods */
    OBJECT_PTR class_methods = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						   9)->ivalue;

    e1 = hashtable_get(native_ptr_ht, (void *)class_methods);

    if(e1)
      cls_obj->class_methods = (method_binding_env_t *)e1->value;
    else
      add_to_deserialization_queue(queue, class_methods, (uintptr_t)cls_obj, CLASS_OBJ_PTR, 9, 0);
    /* end of class methods */

    return (uintptr_t)cls_obj + object_type;
  }
  else if(object_type == ARRAY_TAG)
  {
    hashtable_entry_t *e1;

    array_object_t *arr_obj = (array_object_t *)GC_MALLOC(sizeof(array_object_t));

    /* nof_elements */
    arr_obj->nof_elements = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT),
						0)->ivalue;

    /* elements */
    arr_obj->elements = (OBJECT_PTR *)GC_MALLOC(arr_obj->nof_elements * sizeof(OBJECT_PTR));

    struct JSONObject *elements = JSON_get_array_item(JSON_get_array_item(native_heap, ref >> OBJECT_SHIFT), 1);
    assert(JSON_get_array_size(elements) == arr_obj->nof_elements);

    for(i=0; i<arr_obj->nof_elements; i++)
    {
      OBJECT_PTR element = JSON_get_array_item(elements, i)->ivalue;

      e1 = hashtable_get(obj_ht, (void *)element);

      if(e1)
	arr_obj->elements[i] = (OBJECT_PTR)e1->value;
      else
	add_to_deserialization_queue(queue, arr_obj->elements[i], (uintptr_t)arr_obj, ARRAY_OBJ_PTR, 1, i);
    }
    /* end of elements */

    return (uintptr_t)arr_obj + object_type;
  }
  else if(object_type == CLOSURE_TAG)
  {
    hashtable_entry_t *e1;

    ptr = object_alloc(2, CONS_TAG);

    /* native fn object */
    int native_fn_index = JSON_get_array_item(JSON_get_array_item(object_heap, ref >> OBJECT_SHIFT),
					      0)->ivalue;

    set_heap(ptr, 0, convert_native_fn_to_object(g_native_fn_objects[native_fn_index].nf));
    /* end of native fn object */

    /* closed vals */
    OBJECT_PTR closed_vals = JSON_get_array_item(JSON_get_array_item(object_heap, ref >> OBJECT_SHIFT),
						 1)->ivalue;

    uintptr_t ptr1 = object_alloc(2, CONS_TAG);

    e1 = hashtable_get(obj_ht, (void *)closed_vals);

    if(e1)
      set_heap(ptr1, 0, (OBJECT_PTR)e1->value);
    else
      add_to_deserialization_queue(queue, closed_vals, ptr1, NONE, 0, 0);

    set_heap(ptr, 1, ptr1);
    /* end of closed vals */

    /* arity */
    uintptr_t ptr2 = object_alloc(2, CONS_TAG);
    set_heap(ptr2,
	     0,
	     convert_int_to_object(JSON_get_array_item(JSON_get_array_item(object_heap, ref >> OBJECT_SHIFT),
						       2)->ivalue));
    set_heap(ptr2, 1, NIL);
    /* end of arity */

    set_heap(ptr1, 1, ptr2);

    return (uintptr_t)ptr + object_type;
  }

  //TODO: need to confirm if NATIVE_FN_TAG needs to be handled separately
  assert(false);

}

void load_native_functions(struct JSONObject *native_functions)
{
  int i, j;

  g_nof_native_fns = JSON_get_array_size(native_functions);

  g_native_fn_objects = (native_fn_src_mapping_t *)GC_MALLOC(g_nof_native_fns * sizeof(native_fn_src_mapping_t));

  for(i=0; i<g_nof_native_fns; i++)
  {
    g_native_fn_objects[i].state_index = JSON_get_array_item(JSON_get_array_item(native_functions, i), 0)->ivalue;
    g_native_fn_objects[i].fname       = GC_strdup(JSON_get_array_item(JSON_get_array_item(native_functions, i), 1)->strvalue);
    g_native_fn_objects[i].nf          = NULL; //to be populated by compiling
    g_native_fn_objects[i].source      = GC_strdup(JSON_get_array_item(JSON_get_array_item(native_functions, i), 2)->strvalue);
  }

  int curr_state_index = g_native_fn_objects[0].state_index;

  char *source = NULL;

  unsigned int state_start_index = 0;
  unsigned int prev_state_start_index = 0;

  // 800 is a conservative estimate of the size of the code generated by
  // build_fn_prototypes()
  unsigned int FN_DECL_BUFF_SIZE = 800;

  char *source_with_fn_decls;
  void *state;

  char fn_decls[FN_DECL_BUFF_SIZE];
  memset(fn_decls, '\0', FN_DECL_BUFF_SIZE);
  build_fn_prototypes(fn_decls, 0);

  for(i=0; i<g_nof_native_fns; i++)
  {
    if(g_native_fn_objects[i].state_index == curr_state_index)
    {
      if(!source) //for the first time
	source = GC_strdup(g_native_fn_objects[i].source);
      else
	source = append_string(source, g_native_fn_objects[i].source);
    }
    else //'source' now contains all the sources that belong to one state and can now be compiled
    {
      curr_state_index = g_native_fn_objects[i].state_index;
      prev_state_start_index = state_start_index;
      state_start_index = i;

      source_with_fn_decls = GC_strdup(fn_decls);
      source_with_fn_decls = append_string(source_with_fn_decls, source);
      state = compile_functions_from_string(source_with_fn_decls);

      for(j=prev_state_start_index; j<i; j++)
	g_native_fn_objects[j].nf = get_function(state, g_native_fn_objects[j].fname);

      source = GC_strdup(g_native_fn_objects[i].source);
    }
  }

  //to handle the last state index
  source_with_fn_decls = GC_strdup(fn_decls);
  append_string(source_with_fn_decls, source);
  state = compile_functions_from_string(source_with_fn_decls);

  for(j=state_start_index; j<g_nof_native_fns; j++)
    g_native_fn_objects[j].nf = get_function(state, g_native_fn_objects[j].fname);
}

int load_from_image(char *file_name)
{
  int i, j;

  struct JSONObject *root = JSON_parse(file_name);

  if(!root)
    return 1;

  struct JSONObject *global_variables = JSON_get_object_item(root, "global_variables");
  struct JSONObject *native_functions = JSON_get_object_item(root, "native_functions");
  struct JSONObject *native_heap = JSON_get_object_item(root, "native_heap");
  struct JSONObject *object_heap = JSON_get_object_item(root, "object_heap");

  /*
  struct JSONObject *top_level = JSON_get_object_item(global_variables, "g_top_level");

  JSON_print_object(top_level);
  printf("printed top level\n");
  printf("%d\n", top_level->ivalue);
  struct JSONObject *top_level_bindings =  JSON_get_array_item(native_heap, top_level->ivalue);

  JSON_print_object(top_level_bindings);

  int bindings_count = JSON_get_array_size(top_level_bindings);
  printf("bindings count = %d\n", bindings_count);

  for(i=0; i<bindings_count; i++)
  {
    struct JSONObject *binding = JSON_get_array_item(top_level_bindings, i);
    struct JSONObject *key_val_pair = JSON_get_array_item(native_heap, binding->ivalue);
    printf("%s : %d\n", get_json_core_symbol(native_heap,
					     extract_symbol_index(JSON_get_array_item(key_val_pair, 0)->ivalue)),
	   JSON_get_array_item(key_val_pair, 1)->ivalue);
  }
  */

  /////
  struct JSONObject *smalltalk_symbols = JSON_get_array_item(JSON_get_array_item(native_heap, 0), 1);
  int nof_smalltalk_symbols = JSON_get_array_size(smalltalk_symbols);

  g_smalltalk_symbols = (package_t *)GC_MALLOC(sizeof(package_t));
  g_smalltalk_symbols->nof_symbols = 0;
  g_smalltalk_symbols->name = GC_strdup("SMALLTALK");
  g_smalltalk_symbols->symbols = NULL;

  for(i=0; i<nof_smalltalk_symbols; i++)
    add_smalltalk_symbol(JSON_get_array_item(smalltalk_symbols, i)->strvalue);

  printf("No of Smalltalk symbols = %d\n", g_smalltalk_symbols->nof_symbols);
  /////

  /////
  struct JSONObject *core_symbols = JSON_get_array_item(JSON_get_array_item(native_heap, 1), 1);
  int nof_core_symbols = JSON_get_array_size(core_symbols);

  g_compiler_package = (package_t *)GC_MALLOC(sizeof(package_t));
  g_compiler_package->nof_symbols = 0;
  g_compiler_package->name = GC_strdup("CORE");
  g_compiler_package->symbols = NULL;

  for(i=0; i<nof_core_symbols; i++)
    add_symbol(JSON_get_array_item(core_symbols, i)->strvalue);

  printf("No of core symbols = %d\n", g_compiler_package->nof_symbols);
  /////

  /////
  struct JSONObject *msg_selector = JSON_get_object_item(global_variables, "g_message_selector");
  g_message_selector = (extract_symbol_index(msg_selector->ivalue) << OBJECT_SHIFT) + SMALLTALK_SYMBOL_TAG;
  print_object(g_message_selector); printf("\n");
  /////

  /////
  g_nof_string_literals = JSON_get_object_item(global_variables, "g_nof_string_literals")->ivalue;

  struct JSONObject *string_literals = JSON_get_object_item(global_variables, "g_string_literals");

  assert(JSON_get_array_size(string_literals) == g_nof_string_literals);

  g_string_literals = (char **)GC_MALLOC(g_nof_string_literals * sizeof(char *));

  for(i=0; i< g_nof_string_literals; i++) {
    g_string_literals[i] = GC_strdup(JSON_get_array_item(string_literals, i)->strvalue);
    printf("%s\n", g_string_literals[i]);
  }
  /////

  /////
  //this is not really needed as g_msg_snd_closure can be constructed
  //without any information from the JSON file
  struct JSONObject *msg_snd_closure = JSON_get_object_item(global_variables, "g_msg_snd_closure");
  int object_heap_index = msg_snd_closure->ivalue >> OBJECT_SHIFT;

  int arity = JSON_get_array_item(JSON_get_array_item(object_heap, object_heap_index),
				  2)->ivalue;
  assert(arity == 0);

  OBJECT_PTR closed_vals = JSON_get_array_item(JSON_get_array_item(object_heap, object_heap_index),
					       1)->ivalue;
  assert(closed_vals == NIL);

  g_msg_snd_closure = create_closure(convert_int_to_object(arity),
				     convert_int_to_object(cons_length(closed_vals)),
				     (nativefn)message_send);
  /////

  /////
  //this is not really needed as g_msg_snd_super_closure can be constructed
  //without any information from the JSON file
  struct JSONObject *msg_snd_super_closure = JSON_get_object_item(global_variables, "g_msg_snd_super_closure");
  object_heap_index = msg_snd_super_closure->ivalue >> OBJECT_SHIFT;

  arity = JSON_get_array_item(JSON_get_array_item(object_heap, object_heap_index),
			      2)->ivalue;
  assert(arity == 0);

  closed_vals = JSON_get_array_item(JSON_get_array_item(object_heap, object_heap_index),
				    1)->ivalue;
  assert(closed_vals == NIL);

  g_msg_snd_super_closure = create_closure(convert_int_to_object(arity),
					   convert_int_to_object(cons_length(closed_vals)),
					   (nativefn)message_send_super);
  /////

  /////
  struct JSONObject *compile_time_method_selector = JSON_get_object_item(global_variables, "g_compile_time_method_selector");
  g_compile_time_method_selector = compile_time_method_selector->ivalue;

  print_object(g_compile_time_method_selector);
  printf("\n");
  /////

  g_nof_compiler_states = JSON_get_object_item(global_variables, "g_compile_time_method_selector")->ivalue;

  load_native_functions(native_functions);

  hashtable_t *object_hashtable = hashtable_create(1001);
  hashtable_t *native_ptr_hashtable = hashtable_create(1001);
  queue_t *queue = queue_create();


  //TODO:
  //1. create global variables (including their components), either directly (as above)
  //   or by using deserialize_object_reference()
  //2. call convert_heap() to set all the other object references
  //3. also need to handle native_heap (similar strategy as above may be needed)
  return 0;
}
