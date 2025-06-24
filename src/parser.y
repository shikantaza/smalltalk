%{

#include <stdlib.h>  
#include <stdio.h>
#include <assert.h>

#include "gc.h"

#include "global_decls.h"
#include "stack.h"

// declarations specific to parser,
// so they have not been moved
// to global_decls.h
void yyerror(const char *);
int yylex(void);
int set_up_new_yyin(FILE *);
void pop_yyin();
int yy_scan_string(char *);

void initialize();
void initialize_pass2();

BOOLEAN is_create_class_exp(OBJECT_PTR);
BOOLEAN is_add_var_exp(OBJECT_PTR, char *);
BOOLEAN is_add_instance_var_exp(OBJECT_PTR);
BOOLEAN is_add_class_var_exp(OBJECT_PTR);
BOOLEAN is_add_method_exp(OBJECT_PTR, char *);
BOOLEAN is_add_instance_method_exp(OBJECT_PTR);
BOOLEAN is_add_class_method_exp(OBJECT_PTR);

void gtk_main();
void gtk_init(int *, char ***);
void create_transcript_window(int, int, int, int, char *);
void create_workspace_window(int, int, int, int, char *);
void create_debug_window(int, int, int, int, char *);
void print_to_transcript(char *);

void replace_block_constructor(executable_code_t *);

executable_code_t *g_exp;
int g_open_square_brackets;
BOOLEAN g_loading_core_library;
BOOLEAN g_running_tests;
OBJECT_PTR g_method_call_stack;

OBJECT_PTR g_last_eval_result;

BOOLEAN g_system_initialized;

extern OBJECT_PTR NIL;
extern OBJECT_PTR MESSAGE_SEND;
extern OBJECT_PTR SMALLTALK;
extern OBJECT_PTR LET;
extern binding_env_t *g_top_level;
extern OBJECT_PTR g_test;
extern OBJECT_PTR g_idclo;
extern stack_type *g_exception_contexts;
extern OBJECT_PTR THIS_CONTEXT;
extern OBJECT_PTR g_compile_time_method_selector;
extern stack_type *g_call_chain;
extern stack_type *g_exception_environment;

extern OBJECT_PTR CompileError;

extern enum DebugAction g_debug_action;

%}

%union{
  executable_code_t      *exec_code_value;
  temporaries_t          *temporaries_value;
  identifiers_t          *identifiers_value;
  statement_t            *statement_value;
  return_statement_t     *ret_statement_value;
  expression_t           *expression_value;
  assignment_t           *assignment_value;
  basic_expression_t     *basic_expr_value;
  primary_t              *primary_value;
  block_constructor_t    *blk_cons_value;
  block_arguments_t      *block_arg_value;
  message_t              *message_value;
  unary_messages_t       *unary_messages_value;    
  binary_message_t       *binary_message_value;
  binary_messages_t      *binary_messages_value;  
  binary_argument_t      *binary_arg_value;
  keyword_message_t      *keyword_message_value;
  keyword_argument_t     *keyword_arg_value;
  keyword_argument_pair_t *kw_arg_pair_value;
  literal_t               *literal_value;
  array_elements_t       *array_elements_value;
  array_element_t        *array_element_value;
  char                   *default_value;
  cascaded_messages_t    *cascaded_messages_value;
  number_t               *number_value;
}

%start executable_code

%token                           T_COMMENT
%token <default_value>           T_IDENTIFIER
%token <default_value>           T_KEYWORD
%token <default_value>           T_BINARY_SELECTOR 
%token                           T_RETURN_OPERATOR 
%token                           T_ASSIGNMENT_OPERATOR
%token <default_value>           T_INTEGER
%token <default_value>           T_FLOAT
%token <default_value>           T_SCALED_DECIMAL
%token <default_value>           T_QUOTED_CHARACTER
%token <default_value>           T_QUOTED_STRING
%token <default_value>           T_HASHED_STRING
%token <default_value>           T_QUOTED_SELECTOR
%token                           T_VERTICAL_BAR
%token                           T_PERIOD
%token                           T_LEFT_PAREN
%token                           T_RIGHT_PAREN
%token                           T_LEFT_SQUARE_BRACKET
%token                           T_RIGHT_SQUARE_BRACKET
%token                           T_COLON
%token                           T_SEMI_COLON
%token                           T_HASH
%token                           T_MINUS

%token                           END_OF_FILE

%type <exec_code_value>          executable_code
%type <temporaries_value>        temporaries
%type <statement_value>          statements
%type <identifiers_value>        identifiers
%type <ret_statement_value>      return_statement
%type <expression_value>         expression
%type <assignment_value>         assignment
%type <basic_expr_value>         basic_expression
%type <primary_value>            primary
%type <message_value>            message
%type <cascaded_messages_value>  cascaded_messages
%type <message_value>            cascaded_message
%type <literal_value>            literal
%type <blk_cons_value>           block_constructor
%type <block_arg_value>          block_arguments
%type <default_value>            block_argument
%type <default_value>            unary_message
%type <unary_messages_value>     unary_messages
%type <binary_message_value>     binary_message
%type <binary_messages_value>    binary_messages
%type <binary_arg_value>         binary_argument
%type <keyword_message_value>    keyword_message
%type <keyword_arg_value>        keyword_argument
%type <kw_arg_pair_value>        keyword_arg_pair
%type <keyword_message_value>    keyword_arg_pairs
%type <number_value>             number
%type <default_value>            string_literal
%type <default_value>            character_literal
%type <default_value>            symbol_literal
%type <default_value>            selector_literal
%type <array_elements_value>     array_literal
%type <array_elements_value>     array_elements
%type <array_element_value>      array_element

%%

executable_code:
    temporaries statements
    {
      executable_code_t *exec_code = (executable_code_t *)GC_MALLOC(sizeof(executable_code_t));
      exec_code->temporaries = $1;
      exec_code->statements = $2;
      $$ = exec_code;
      if(g_open_square_brackets == 0)
      {
	g_exp = $$;
	YYACCEPT;
      }
    }
    |
    temporaries
    {
      executable_code_t *exec_code = (executable_code_t *)GC_MALLOC(sizeof(executable_code_t));
      exec_code->temporaries = $1;
      exec_code->statements = NULL;
      $$ = exec_code;
      if(g_open_square_brackets == 0)
      {
	g_exp = $$;
	YYACCEPT;
      }
    }
    |
    statements
    {
      executable_code_t *exec_code = (executable_code_t *)GC_MALLOC(sizeof(executable_code_t));
      exec_code->temporaries = NULL;
      exec_code->statements = $1;
      $$ = exec_code;
      if(g_open_square_brackets == 0)
      {
	g_exp = $$;
	YYACCEPT;
      }
    }
    ;

temporaries:
    T_VERTICAL_BAR identifiers T_VERTICAL_BAR
    {
      temporaries_t *temps = (temporaries_t *)GC_MALLOC(sizeof(temporaries_t));
      temps->nof_temporaries = $2->nof_identifiers;

      temps->temporaries = (char **)GC_MALLOC(temps->nof_temporaries * sizeof(char *));

      unsigned int i;
      for(i=0; i < temps->nof_temporaries; i++)
        temps->temporaries[i] = GC_strdup($2->identifiers[i]);
      
      $$ = temps;
    }
    ;

identifiers:
    /* empty */
    {
      identifiers_t *idents = (identifiers_t *)GC_MALLOC(sizeof(identifiers_t));
      idents->nof_identifiers = 0;
      idents->identifiers = NULL;
      $$ = idents;
    }
    |
    identifiers T_IDENTIFIER
    {
      $1->nof_identifiers++;
      char **temp = (char **)GC_REALLOC($1->identifiers,
					$1->nof_identifiers * sizeof(char *));
      assert(temp);
      $1->identifiers = temp;

      $1->identifiers[$1->nof_identifiers - 1] = GC_strdup($2);
      $$ = $1;
    }
    ;

statements:
    return_statement
    {
      statement_t *stmt = (statement_t *)GC_MALLOC(sizeof(statement_t));
      stmt->type = RETURN_STATEMENT;
      stmt->ret_stmt = $1;
      stmt->exp = NULL;
      stmt->statements = NULL;
      $$ = stmt;
    }
    |
    return_statement T_PERIOD
    {
      statement_t *stmt = (statement_t *)GC_MALLOC(sizeof(statement_t));
      stmt->type = RETURN_STATEMENT;
      stmt->ret_stmt = $1;
      stmt->exp = NULL;
      stmt->statements = NULL;
      $$ = stmt;
    }
    |
    expression
    {
      statement_t *stmt = (statement_t *)GC_MALLOC(sizeof(statement_t));
      stmt->type = EXPRESSION;
      stmt->ret_stmt = NULL;
      stmt->exp = $1;
      stmt->statements = NULL;
      $$ = stmt;
    }
    |
    expression T_PERIOD statements
    {
      statement_t *stmt = (statement_t *)GC_MALLOC(sizeof(statement_t));
      stmt->type = EXP_PLUS_STATEMENTS;
      stmt->ret_stmt = NULL;
      stmt->exp = $1;
      stmt->statements = $3;
      $$ = stmt;
    }
    ;

return_statement:
    T_RETURN_OPERATOR expression
    {
      return_statement_t *ret_stmt = (return_statement_t *)GC_MALLOC(sizeof(return_statement_t));
      ret_stmt->exp = $2;
      $$ = ret_stmt;
    }
    ;

expression:
    assignment
    {
      expression_t *exp = (expression_t *)GC_MALLOC(sizeof(expression_t));
      exp->type = ASSIGNMENT;
      exp->asgn = $1;
      exp->basic_exp = NULL;
      $$ = exp;
    }
    |
    basic_expression
    {
      expression_t *exp = (expression_t *)GC_MALLOC(sizeof(expression_t));
      exp->type = BASIC_EXPRESSION;
      exp->asgn = NULL;
      exp->basic_exp = $1;
      $$ = exp;
    }    
    ; 

assignment:
    T_IDENTIFIER T_ASSIGNMENT_OPERATOR expression
    {
      assignment_t *asgnmt = (assignment_t *)GC_MALLOC(sizeof(assignment_t));
      asgnmt->identifier = $1;
      asgnmt->rvalue = $3;
      $$ = asgnmt;
    }
    ;

basic_expression:
    primary
    {
      basic_expression_t *basic_exp = (basic_expression_t *)GC_MALLOC(sizeof(basic_expression_t));
      basic_exp->type = PRIMARY;
      basic_exp->prim = $1;
      basic_exp->msg = NULL;
      basic_exp->cascaded_msgs = NULL;
      $$ = basic_exp;
    }
    |
    primary message cascaded_messages
    {
      basic_expression_t *basic_exp = (basic_expression_t *)GC_MALLOC(sizeof(basic_expression_t));
      basic_exp->type = PRIMARY_PLUS_MESSAGES;
      basic_exp->prim = $1;
      basic_exp->msg = $2;
      basic_exp->cascaded_msgs = $3;
      $$ = basic_exp;
    }
    ;

primary:
    T_IDENTIFIER
    {
      primary_t *prim = (primary_t *)GC_MALLOC(sizeof(primary_t));
      prim->type = IDENTIFIER;
      prim->identifier = GC_strdup($1);
      prim->lit = NULL;
      prim->blk_cons = NULL;
      prim->exp = NULL;
      $$ = prim;
    }
    |
    literal
    {
      primary_t *prim = (primary_t *)GC_MALLOC(sizeof(primary_t));
      prim->type = LITERAL;
      prim->identifier = NULL;
      prim->lit = $1;
      prim->blk_cons = NULL;
      prim->exp = NULL;
      $$ = prim;
    }
    |
    block_constructor
    {
      primary_t *prim = (primary_t *)GC_MALLOC(sizeof(primary_t));
      prim->type = BLOCK_CONSTRUCTOR;
      prim->identifier = NULL;
      prim->lit = NULL;
      prim->blk_cons = $1;
      prim->exp = NULL;
      $$ = prim;
    }
    |
    T_LEFT_PAREN expression T_RIGHT_PAREN
    {
      primary_t *prim = (primary_t *)GC_MALLOC(sizeof(primary_t));
      prim->type = EXPRESSION1;
      prim->identifier = NULL;
      prim->lit = NULL;
      prim->blk_cons = NULL;
      prim->exp = $2;
      $$ = prim;
    }
    ;

block_constructor:
    T_LEFT_SQUARE_BRACKET
    { g_open_square_brackets++; }
    block_arguments block_argument
    T_VERTICAL_BAR
    executable_code
    T_RIGHT_SQUARE_BRACKET
    { g_open_square_brackets--; }
    {
      block_constructor_t *blk_cons = (block_constructor_t *)GC_MALLOC(sizeof(block_constructor_t));
      blk_cons->type = BLOCK_ARGS;

      $3->nof_args++;
      char **temp = (char **)GC_REALLOC($3->identifiers, $3->nof_args * sizeof(char *));
      assert(temp);
      $3->identifiers = temp;
      $3->identifiers[$3->nof_args - 1] = GC_strdup($4);

      blk_cons->block_args = $3;

      blk_cons->exec_code = $6;
      $$ = blk_cons;
    }
    |
    T_LEFT_SQUARE_BRACKET {g_open_square_brackets++; }executable_code T_RIGHT_SQUARE_BRACKET { g_open_square_brackets--; }
    {
      block_constructor_t *blk_cons = (block_constructor_t *)GC_MALLOC(sizeof(block_constructor_t));
      blk_cons->type = NO_BLOCK_ARGS;
      blk_cons->block_args = NULL;
      blk_cons->exec_code = $3;
      $$ = blk_cons;
    }
    ;

block_arguments:
    /* empty */
    {
      block_arguments_t *blk_args = (block_arguments_t *)GC_MALLOC(sizeof(block_arguments_t));
      blk_args->nof_args = 0;
      $$ = blk_args;
    }
    |
    block_arguments block_argument
    {
      $1->nof_args++;
      char **temp = (char **)GC_REALLOC($1->identifiers,
					$1->nof_args * sizeof(char *));
      assert(temp);
      $1->identifiers = temp;

      $1->identifiers[$1->nof_args - 1] = $2;
      $$ = $1;
    }
    ;

block_argument:
    T_COLON T_IDENTIFIER
    {
      /*
      unsigned int len = strlen($2) + 1;
      char *str = (char *)GC_MALLOC(len * sizeof(char));
      memset(str, '\0', len);
      sprintf(str, ":%s", $2);
      $$ = str;
      */
      $$ = GC_strdup($2); //having the colon in the argument name screws up the renaming transformation
    }
    ;

message:
    unary_messages unary_message binary_messages
    {
      message_t *msg = (message_t *)GC_MALLOC(sizeof(message_t));
      msg->type = UNARY_MESSAGE;

      $1->nof_messages++;
      char **temp  = (char **)GC_REALLOC($1->identifiers, $1->nof_messages * sizeof(char *));
      assert(temp);
      $1->identifiers = temp;
      $1->identifiers[$1->nof_messages - 1] = GC_strdup($2);

      msg->unary_messages = $1;

      if($3->nof_messages > 0)
        msg->binary_messages = $3;
      else
        msg->binary_messages = NULL;

      msg->kw_msg = NULL;
      
      $$ = msg;
    }
    |
    unary_messages unary_message binary_messages keyword_message
    {
      message_t *msg = (message_t *)GC_MALLOC(sizeof(message_t));
      msg->type = UNARY_MESSAGE;

      $1->nof_messages++;
      char **temp = (char **)GC_REALLOC($1->identifiers, $1->nof_messages * sizeof(char *));
      assert(temp);
      $1->identifiers = temp;
      $1->identifiers[$1->nof_messages - 1] = GC_strdup($2);

      msg->unary_messages = $1;

      if($3->nof_messages > 0)
        msg->binary_messages = $3;
      else
        msg->binary_messages = NULL;

      msg->kw_msg = $4;

      $$ = msg;     
    }
    |
    binary_messages binary_message
    {
      message_t *msg = (message_t *)GC_MALLOC(sizeof(message_t));
      msg->type = BINARY_MESSAGE;

      msg->unary_messages = NULL;

      $1->nof_messages++;
      binary_message_t *temp = (binary_message_t *)GC_REALLOC($1->bin_msgs, $1->nof_messages * sizeof(binary_messages_t));
      assert(temp);
      $1->bin_msgs = temp;
      $1->bin_msgs[$1->nof_messages - 1] = *($2);

      msg->binary_messages = $1;

      msg->kw_msg = NULL;
        
      $$ = msg;
    }
    |
    binary_messages binary_message keyword_message
    {
      message_t *msg = (message_t *)GC_MALLOC(sizeof(message_t));
      msg->type = BINARY_MESSAGE;

      msg->unary_messages = NULL;

      $1->nof_messages++;
      binary_message_t *temp = (binary_message_t *)GC_REALLOC($1->bin_msgs, $1->nof_messages * sizeof(binary_messages_t));
      assert(temp);
      $1->bin_msgs = temp;
      $1->bin_msgs[$1->nof_messages - 1] = *($2);

      msg->binary_messages = $1;

      msg->kw_msg = $3;

      $$ = msg;
    }
    |
    keyword_message
    {
      message_t *msg = (message_t *)GC_MALLOC(sizeof(message_t));
      msg->type = KEYWORD_MESSAGE;
      msg->unary_messages = NULL;
      msg->binary_messages = NULL;
      msg->kw_msg = $1;
      $$ = msg;
    }
    ;

unary_message:
    T_IDENTIFIER
    {
      $$ = GC_strdup($1);
    }
    ;

unary_messages:
    /* empty */
    {
      unary_messages_t *unary_msgs = (unary_messages_t *)GC_MALLOC(sizeof(unary_messages_t));
      unary_msgs->nof_messages = 0;
      $$ = unary_msgs;
    }
    |
    unary_messages unary_message
    {
      $1->nof_messages++;
      char **temp = (char **)GC_REALLOC($1->identifiers,
					$1->nof_messages * sizeof(char *));
      assert(temp);
      $1->identifiers = temp;

      /* no GC_strdup() as this has already been done 
         in the rule for unary_message */
      $1->identifiers[$1->nof_messages - 1] = $2;
      
      $$ = $1;            
    }
    ;

binary_messages:
    /* empty */
    {
      binary_messages_t *binary_msgs = (binary_messages_t *)GC_MALLOC(sizeof(binary_messages_t));
      binary_msgs->nof_messages = 0;
      $$ = binary_msgs;
    }
    |
    binary_messages binary_message
    {
      $1->nof_messages++;
      binary_message_t *temp = (binary_message_t *)GC_REALLOC($1->bin_msgs,
							      $1->nof_messages * sizeof(binary_message_t));
      assert(temp);
      $1->bin_msgs = temp;

      $1->bin_msgs[$1->nof_messages - 1] = *($2);
      $$ = $1;      
    }
    ;

binary_message:
    T_BINARY_SELECTOR binary_argument
    {
      binary_message_t *bin_msg = (binary_message_t *)GC_MALLOC(sizeof(binary_message_t));
      bin_msg->binary_selector = GC_strdup($1);
      bin_msg->bin_arg = $2;
      $$ = bin_msg;
    }
    ;

binary_argument:
    primary unary_messages
    {
      binary_argument_t *bin_arg = (binary_argument_t *)GC_MALLOC(sizeof(binary_argument_t));
      bin_arg->prim = $1;
      bin_arg->unary_messages = $2;
      $$ = bin_arg;
    }
    ;

keyword_message:
    keyword_arg_pairs keyword_arg_pair
    {
      $1->nof_args++;
      keyword_argument_pair_t *temp = (keyword_argument_pair_t *)GC_REALLOC($1->kw_arg_pairs,
									    $1->nof_args * sizeof(keyword_argument_pair_t));
      assert(temp);
      $1->kw_arg_pairs = temp;
      $1->kw_arg_pairs[$1->nof_args - 1] = *($2);

      $$ = $1;
    }
    ;

keyword_argument:
    primary unary_messages binary_messages
    {
      keyword_argument_t *kw_arg = (keyword_argument_t *)GC_MALLOC(sizeof(keyword_argument_t));
      kw_arg->prim = $1;
      kw_arg->unary_messages = $2;
      kw_arg->binary_messages = $3;
      $$ = kw_arg;
    }
    ;

keyword_arg_pair:
    T_KEYWORD keyword_argument
    {
      keyword_argument_pair_t *pair =
        (keyword_argument_pair_t *)GC_MALLOC(sizeof(keyword_argument_pair_t));
      pair->keyword = GC_strdup($1);
      pair->kw_arg = $2;
      $$ = pair;
    }
    ;

keyword_arg_pairs:
    /* empty */
    {
      keyword_message_t *kw_arg_pairs =
        (keyword_message_t *)GC_MALLOC(sizeof(keyword_message_t));
      kw_arg_pairs->nof_args = 0;
      $$ = kw_arg_pairs;
    }
    |
    keyword_arg_pairs keyword_arg_pair
    {
      $1->nof_args++;
      keyword_argument_pair_t *temp =
        (keyword_argument_pair_t *)GC_REALLOC($1->kw_arg_pairs,
                                              $1->nof_args * sizeof(keyword_argument_pair_t));
      assert(temp);
      $1->kw_arg_pairs = temp;

      $1->kw_arg_pairs[$1->nof_args - 1] = *($2);
      $$ = $1;
    }
    ;

cascaded_messages:
    /* empty */
    {
      cascaded_messages_t *casc_msgs =
        (cascaded_messages_t *)GC_MALLOC(sizeof(cascaded_messages_t));
      casc_msgs->nof_cascaded_msgs = 0;
      $$ = casc_msgs;
    }
    |
    cascaded_messages cascaded_message
    {
      $1->nof_cascaded_msgs++;
      message_t *temp =
        (message_t *)GC_REALLOC($1->cascaded_msgs,
                                $1->nof_cascaded_msgs * sizeof(message_t));
      assert(temp);
      $1->cascaded_msgs = temp;

      $1->cascaded_msgs[$1->nof_cascaded_msgs - 1] = *($2);
      $$ = $1;
    }
    ;

cascaded_message:
    T_SEMI_COLON message
    {
      $$ = $2; /* don't need the semicolon */
    }
    ;

literal:
    number
    {
      literal_t *lit = (literal_t *)GC_MALLOC(sizeof(literal_t));
      lit->type = NUMBER_LITERAL;
      lit->num = $1;
      lit->val = NULL;
      lit->array_elements = NULL;
      $$ = lit;
    }
    | string_literal
    {
      literal_t *lit = (literal_t *)GC_MALLOC(sizeof(literal_t));
      lit->type = STRING_LITERAL;
      lit->num = NULL;      
      lit->val = $1;
      lit->array_elements = NULL;
      $$ = lit;      
     }
    | character_literal
    {
      literal_t *lit = (literal_t *)GC_MALLOC(sizeof(literal_t));
      lit->type = CHAR_LITERAL;
      lit->num = NULL;      
      lit->val = $1;
      lit->array_elements = NULL;
      $$ = lit;
    }
    | symbol_literal
    {
      literal_t *lit = (literal_t *)GC_MALLOC(sizeof(literal_t));
      lit->type = SYMBOL_LITERAL;
      lit->num = NULL;      
      lit->val = $1;
      lit->array_elements = NULL;
      $$ = lit;
    }
    | selector_literal
    {
      literal_t *lit = (literal_t *)GC_MALLOC(sizeof(literal_t));
      lit->type = SELECTOR_LITERAL;
      lit->num = NULL;      
      lit->val = $1;
      lit->array_elements = NULL;
      $$ = lit;
    }
    | array_literal
    {
      literal_t *lit = (literal_t *)GC_MALLOC(sizeof(literal_t));
      lit->type = ARRAY_LITERAL;
      lit->num = NULL;      
      lit->val = NULL;
      lit->array_elements = $1;
      $$ = lit;
    }
    ;

number:
    T_INTEGER
    {
      number_t *n = (number_t *)GC_MALLOC(sizeof(number_t));
      n->type = INTEGER;
      n->val = GC_strdup($1);
      $$ = n;
    }
    |
    T_FLOAT
    {
      number_t *n = (number_t *)GC_MALLOC(sizeof(number_t));
      n->type = FLOAT;
      n->val = GC_strdup($1);
      $$ = n;
    }
    |
    T_SCALED_DECIMAL
    {
      number_t *n = (number_t *)GC_MALLOC(sizeof(number_t));
      n->type = SCALED_DECIMAL;
      n->val = GC_strdup($1);
      $$ = n;
    }
    ;

character_literal:
    T_QUOTED_CHARACTER
    {
      $$ = GC_strdup($1);
    }
    ;

string_literal:
    T_QUOTED_STRING
    {
      $$ = GC_strdup($1);
    }
    ;

symbol_literal:
    T_HASHED_STRING
    {
      $$ = GC_strdup($1);
    }
    ;

selector_literal:
    T_QUOTED_SELECTOR
    {
      $$ = GC_strdup($1);
    }
    ;

array_literal:
    T_HASH T_LEFT_PAREN array_elements T_RIGHT_PAREN
    {
      $$ = $3;
    }
    ;

array_elements:
    /* empty */
    {
      array_elements_t *array = (array_elements_t *)GC_MALLOC(sizeof(array_elements_t));
      array->nof_elements = 0;
      $$ = array;
    }
    |
    array_elements array_element
    {
      $1->nof_elements++;
      array_element_t *temp = (array_element_t *)GC_REALLOC($1->elements,
							    $1->nof_elements * sizeof(array_element_t));
      assert(temp);
      $1->elements = temp;

      $1->elements[$1->nof_elements - 1] = *($2);
      $$ = $1;      
    }
    ;

array_element:
    literal
    {
      array_element_t *elem = (array_element_t *)GC_MALLOC(sizeof(array_element_t));
      elem->type = LITERAL1;
      elem->lit = $1;
      elem->identifier = NULL;
      $$ = elem;
    }
    |
    T_IDENTIFIER
    {
      array_element_t *elem = (array_element_t *)GC_MALLOC(sizeof(array_element_t));
      elem->type = IDENTIFIER1;
      elem->lit = NULL;
      elem->identifier = GC_strdup($1);
      $$ = elem;
    }    
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
}

int repl2()
{
  g_method_call_stack = NIL;

  stack_empty(g_call_chain);

  stack_empty(g_exception_environment);

  stack_empty(g_exception_contexts);

  g_compile_time_method_selector = NIL;

  g_debug_action = CONTINUE;

  //OBJECT_PTR exp = convert_exec_code_to_lisp(g_exp);

  //exp = decorate_message_selectors(exp);
    
#ifdef DEBUG
  print_object(exp); printf("\n");
#endif

  /*
  //awkward conditionals because of
  //absence of shortcirtuing for && (confirm)
  if(!IS_CONS_OBJECT(exp))
    repl();
  else if(cons_length(exp) != 3)
    repl();
  else if(!IS_CONS_OBJECT(third(exp)))
    repl();
  else if(first(third(exp)) == MESSAGE_SEND &&
	  second(third(exp)) == SMALLTALK &&
	  fourth(third(exp)) == get_symbol("_addInstanceMethod:toClass:withBody:"))
  {
    if(!IS_SMALLTALK_SYMBOL_OBJECT(sixth(third(exp))))
    {
      printf("Invalid method selector passed to Smalltalk>>addInstanceMethod\n");
      return 0;
    }

    OBJECT_PTR class_object_val, class_object;

    //TODO: figure out how to convert these
    //asserts into exceptions

    if(!IS_SYMBOL_OBJECT(seventh(third(exp))))
    {
      printf("Smalltalk>>addInstanceMethod: Invalid class name\n");
      return 0;
    }

    if(!get_top_level_val(seventh(third(exp)), &class_object_val))
    {
      printf("Smalltalk>>addInstanceMethod: Class does not exist\n");
      return 0;
    }

    class_object = car(class_object_val);

    if(!IS_CLASS_OBJECT(class_object))
    {
      printf("Invalid class passed to Smalltalk>>addInstanceMethod\n");
      return 0;
    }

    //if(!IS_CONS_OBJECT(seventh(third(exp))))
    if(!IS_CONS_OBJECT(nth(convert_int_to_object(7), third(exp))))
    {
      printf("Invalid block passed to Smalltalk>>addInstanceMethod\n");
      return 0;
    }

    OBJECT_PTR ret = add_instance_method(class_object,
					 sixth(third(exp)),
					 list(3, LET, NIL, nth(convert_int_to_object(7), third(exp))),
					 NIL, //we do not have a code string to pass; will be resolved when we
					 g_exp);  //switch to addInstanceMethod:toClass:withBodyStr
    if(g_loading_core_library == false)
    {
      //printf("\n");
      //print_object(ret);
      //printf("\n");

      //not printing anything by default
      //char buf[500];
      //memset(buf, '\0', 500);
      //print_object_to_string(ret, buf);
      //print_to_transcript(buf);
    }
    g_last_eval_result = ret;
  }
  else if(first(third(exp)) == MESSAGE_SEND &&
	  second(third(exp)) == SMALLTALK &&
	  fourth(third(exp)) == get_symbol("_addClassMethod:toClass:withBody:"))
  {
    if(!IS_SMALLTALK_SYMBOL_OBJECT(sixth(third(exp))))
    {
      printf("Invalid method selector passed to Smalltalk>>addClassMethod\n");
      return 0;
    }

    OBJECT_PTR class_object_val, class_object;

    //TODO: figure out how to convert these
    //asserts into exceptions

    if(!IS_SYMBOL_OBJECT(seventh(third(exp))))
    {
      printf("Smalltalk>>addClassMethod: Invalid class name\n");
      return 0;
    }

    if(!get_top_level_val(seventh(third(exp)), &class_object_val))
    {
      printf("Smalltalk>>addClassMethod: Class does not exist\n");
      return 0;
    }

    class_object = car(class_object_val);

    if(!IS_CLASS_OBJECT(class_object))
    {
      printf("Invalid class passed to Smalltalk>>addInstanceMethod\n");
      return 0;
    }

    //if(!IS_CONS_OBJECT(seventh(third(exp))))
    if(!IS_CONS_OBJECT(nth(convert_int_to_object(7), third(exp))))
    {
      printf("Invalid block passed to Smalltalk>>addInstanceMethod\n");
      return 0;
    }

    OBJECT_PTR ret = add_class_method(class_object,
				      sixth(third(exp)),
				      list(3, LET, NIL, nth(convert_int_to_object(7), third(exp))),
				      NIL, //we do not have a code string to pass; will be resolved
				      g_exp); //switch to addInstanceMethod:toClass:withBodyStr
    if(g_loading_core_library == false)
    {
      //printf("\n");
      //print_object(ret);
      //printf("\n");

      //not printing anything by default
      //char buf[500];
      //memset(buf, '\0', 500);
      //print_object_to_string(ret, buf);
      //print_to_transcript(buf);
    }
    g_last_eval_result = ret;
  }
  else
    repl();
  */
  repl();

  return 0;
}

void parse_from_fp(FILE *fp)
{
  char buf[1024];
  char *line = NULL;

  unsigned int len;
  int nbytes = 200;

  BOOLEAN eof = 0;
  
  while(eof != -1)
  {
    memset(buf, '\0', 1024);
    len = 0;
    
    while(1)
    {
      line = (char *)GC_MALLOC((nbytes + 1) * sizeof(char));
      eof = getline(&line, (size_t *)&nbytes, fp);
    
      if(!strcmp(line, "\n") || eof == -1)
	 break;

      len += sprintf(buf+len, "%s", line);
    }

    if(strlen(buf) == 0)
      break;

    yy_scan_string(buf);

    if(!yyparse())
    {
      repl2();
    }

    if(eof == -1)
      break;
  }
}

void load_file(char *file_name)
{
  FILE *fp = fopen(file_name, "r");
  assert(fp);
  parse_from_fp(fp);
  fclose(fp);
}

void load_core_library()
{
  g_loading_core_library = true;
  printf("Loading core library...");
  load_file(SMALLTALKDATADIR "/smalltalk.st");
  printf("done.\n");
  g_loading_core_library = false;
}

void load_core_library2()
{
  g_loading_core_library = true;
  printf("Loading core library (Phase 2)...");
  load_file(SMALLTALKDATADIR "/smalltalk2.st");
  printf("done.\n");
  g_loading_core_library = false;
}

void load_tests()
{
  g_running_tests = true;
  printf("Running tests...");
  load_file("tests.st");
  printf("done.\n");
  g_running_tests = false;
}

#ifndef LEX
int main(int argc, char **argv)
{
  g_system_initialized = false;

  initialize();  

  load_core_library();

  initialize_pass2();

  load_core_library2();

  g_system_initialized = true;

  gtk_init(&argc, &argv);
  create_transcript_window(DEFAULT_TRANSCRIPT_POSX,
			   DEFAULT_TRANSCRIPT_POSY,
			   DEFAULT_TRANSCRIPT_WIDTH,
			   DEFAULT_TRANSCRIPT_HEIGHT,
			   "Smalltalk");
  create_workspace_window(DEFAULT_WORKSPACE_POSX,
			  DEFAULT_WORKSPACE_POSY,
			  DEFAULT_WORKSPACE_WIDTH,
			  DEFAULT_WORKSPACE_HEIGHT,
			  "Smalltalk");
  create_debug_window(DEFAULT_DEBUG_WINDOW_POSX,
		      DEFAULT_DEBUG_WINDOW_POSY,
		      DEFAULT_DEBUG_WINDOW_WIDTH,
		      DEFAULT_DEBUG_WINDOW_HEIGHT,
		      "Smalltalk");
  gtk_main();

  /*
  //load_tests();
  //TODO: get this parsing too done
  //by parser_from_fp()
  
  char buf[1024];
  char *line = NULL;

  unsigned int len;
  int nbytes = 100;

  memset(buf, '\0', 1024);

  fprintf(stdout, "Type the Smalltalk expression at the prompt. Press Enter on an empty line\n");
  fprintf(stdout, "(after completing the expression) to evaluate the expression, Control-C to quit.\n");
  
  while(1)
  {
    memset(buf, '\0', 1024);
    len = 0;
    
    fprintf(stdout, "> ");

    while(1)
    {
      line = (char *)GC_MALLOC((nbytes + 1) * sizeof(char));
      getline(&line, (size_t *)&nbytes, stdin);
    
      if(!strcmp(line, "\n"))
	 break;

      len += sprintf(buf+len, "%s", line);
    }

    if(!strcmp(buf, "quit\n"))
      break;

    yy_scan_string(buf);
    if(!yyparse())
    {
      repl2();
    }
  }
  */

  exit(0);    
}

OBJECT_PTR repl_common()
{
  replace_block_constructor(g_exp);
  OBJECT_PTR exp = convert_exec_code_to_lisp(g_exp);

  exp = decorate_message_selectors(exp);
  
#ifdef DEBUG
  print_object(exp); printf(" is returned by convert_exec_code_to_lisp()\n");
#endif
  
  OBJECT_PTR res = apply_lisp_transforms(exp);

#ifdef DEBUG
  print_object(res);
  printf("\n");
#endif
  
  void *state = compile_to_c(res);

  char *fname = extract_variable_string(fourth(first(res)), true);

  nativefn nf = get_function(state, fname);

  assert(nf);
  
  OBJECT_PTR nfo = convert_native_fn_to_object(nf);

  OBJECT_PTR closed_vals = cdr(CDDDR(first(res)));
  
  OBJECT_PTR rest = closed_vals;
  OBJECT_PTR ret = NIL;

  while(rest != NIL)
  {
    OBJECT_PTR closed_val = car(rest);
    OBJECT_PTR closed_val_cons;
    if(get_binding_val_regular(g_top_level, closed_val, &closed_val_cons))
    {
	ret = cons(closed_val_cons, ret);
	rest = cdr(rest);
    }
    else
    {
      //printf("Unbound variable: %s\n", get_symbol_name(closed_val));
      //return NIL;

      //TODO: the exception object's messageText has to be set (after adding it as an instance variable)
      OBJECT_PTR exception_obj = new_object_internal(CompileError,
						     convert_fn_to_closure((nativefn)new_object_internal),
						     g_idclo);
      char str[100];
      sprintf(str, "Unbound variable: %s", get_symbol_name(closed_val));

      return signal_exception_with_text(exception_obj, get_string_obj(str), g_idclo);
    }
  }  

  OBJECT_PTR lst_form = list(3, nfo, reverse(ret), second(first(res)));
  OBJECT_PTR closure_form = extract_ptr(lst_form) + CLOSURE_TAG;

  return closure_form;
}

void repl()
{
  OBJECT_PTR repl_ret_val = repl_common();

  OBJECT_PTR ret;

  if(IS_CLOSURE_OBJECT(repl_ret_val))
  {
    put_binding_val(g_top_level, THIS_CONTEXT, cons(g_idclo, NIL));

    ret = invoke_cont_on_val(repl_ret_val, g_idclo);
  }
  else //unhandled exception would have triggered debugger, so return value from debugger user action
    ret = repl_ret_val;

  if(g_loading_core_library == false)
  {
    //printf("\n");
    //print_object(ret);
    //printf("\n");

    //not printing anything by default
    //char buf[500];
    //memset(buf, '\0', 500);
    //print_object_to_string(ret, buf);
    //print_to_transcript(buf);
  }
  g_last_eval_result = ret;
}

//this too could have been brought under
//the is_add_var_exp code template
BOOLEAN is_create_class_exp(OBJECT_PTR exp)
{
  if(!IS_CONS_OBJECT(exp))
    return false;

  if(cons_length(exp) != 3)
    return false;

  OBJECT_PTR third_obj = third(exp);
  
  if(!IS_CONS_OBJECT(third_obj))
    return false;

  if(cons_length(third_obj) != 6)
    return false;
     
  if(first(third_obj) == MESSAGE_SEND &&
     second(third_obj) == SMALLTALK &&
     fourth(third_obj) == get_symbol("_createClass:parentClass:") &&
     IS_SYMBOL_OBJECT(fifth(third_obj)) &&
     IS_SYMBOL_OBJECT(sixth(third_obj)))
    return true;
  else
    return false;  
}

BOOLEAN is_add_var_exp(OBJECT_PTR exp, char *msg)
{
  if(!IS_CONS_OBJECT(exp))
    return false;

  if(cons_length(exp) != 3)
    return false;

  OBJECT_PTR third_obj = third(exp);
  
  if(!IS_CONS_OBJECT(third_obj))
    return false;

  if(cons_length(third_obj) != 6)
    return false;
     
  if(first(third_obj) == MESSAGE_SEND &&
     second(third_obj) == SMALLTALK &&
     fourth(third_obj) == get_symbol(msg) &&
     IS_SYMBOL_OBJECT(fifth(third_obj)) &&
     IS_SYMBOL_OBJECT(sixth(third_obj)))
    return true;
  else
    return false;  
  
}

BOOLEAN is_add_instance_var_exp(OBJECT_PTR exp)
{
  return is_add_var_exp(exp, "_addInstanceVariable:toClass:");
}

BOOLEAN is_add_class_var_exp(OBJECT_PTR exp)
{
  return is_add_var_exp(exp, "_addClassVariable:toClass:");
}

BOOLEAN is_add_method_exp(OBJECT_PTR exp, char *msg)
{
  if(!IS_CONS_OBJECT(exp))
    return false;

  if(cons_length(exp) != 3)
    return false;
  
  OBJECT_PTR third_obj = third(exp);
  
  if(!IS_CONS_OBJECT(third_obj))
    return false;

  if(cons_length(third_obj) != 7)
    return false;
     
  if(first(third_obj) == MESSAGE_SEND &&
     second(third_obj) == SMALLTALK &&
     fourth(third_obj) == get_symbol(msg) &&
     IS_SMALLTALK_SYMBOL_OBJECT(fifth(third_obj)) &&
     IS_CONS_OBJECT(seventh(third_obj)) && //TODO: maybe some stronger checks?
     IS_SYMBOL_OBJECT(sixth(third_obj)))
    return true;
  else
    return false;  
}

BOOLEAN is_add_instance_method_exp(OBJECT_PTR exp)
{
  return is_add_method_exp(exp, "_addInstanceMethod:toClass:withBody:");
}

BOOLEAN is_add_class_method_exp(OBJECT_PTR exp)
{
  return is_add_method_exp(exp, "_addClassMethod:toClass:withBody:");
}

#endif


