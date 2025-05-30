#ifndef PARSER_HEADER_H
#define PARSER_HEADER_H

#include <string.h>

/* forward declarations */
struct expression;
struct return_statement;
struct primary;
struct message;
struct cascaded_messages;
struct literal;
struct block_constructor;
struct block_argument;
struct binary_message;
struct keyword_message;
struct binary_argument;
struct keyword_argument;
struct keyword_argument_pairs;
struct keyword_argument_pair;
struct literal;

typedef struct temporaries
{
  unsigned int nof_temporaries;
  char **temporaries;
  
} temporaries_t;

typedef struct identifiers
{
  unsigned int nof_identifiers;
  char **identifiers;
  
} identifiers_t;

typedef struct executable_code
{
  temporaries_t *temporaries;
  struct statement *statements;
} executable_code_t;

enum StatementType
{
  RETURN_STATEMENT,
  EXPRESSION,
  EXP_PLUS_STATEMENTS
};

typedef struct statement
{
  /* RETURN_STATEMENT, 
     EXPRESSION, or
     EXP_PLUS_STATEMENTS */ 
  enum StatementType type; 
  struct return_statement *ret_stmt;
  struct expression *exp;
  struct statement *statements;
} statement_t;

typedef struct return_statement
{
  struct expression *exp;
} return_statement_t;


enum ExpressionType
{
  ASSIGNMENT,
  BASIC_EXPRESSION
};
  
typedef struct expression
{
  /* ASSIGNMENT or BASIC_EXPRESSION */
  enum ExpressionType type;
  struct assignment *asgn;
  struct basic_expression *basic_exp;
} expression_t;

typedef struct assignment
{
  char *identifier;
  struct expression *rvalue;
} assignment_t;

enum BasicExpressionType
{
  PRIMARY,
  PRIMARY_PLUS_MESSAGES
};
  
typedef struct basic_expression
{
  /* PRIMARY, OR
     PRIMARY_PLUS_MESSAGES */
  enum BasicExpressionType type;
  struct primary *prim;
  struct message *msg;
  struct cascaded_messages *cascaded_msgs;
} basic_expression_t;

typedef struct cascaded_messages
{
  unsigned int nof_cascaded_msgs;
  struct message *cascaded_msgs;
} cascaded_messages_t;

enum PrimaryType
{
  IDENTIFIER,
  LITERAL,
  BLOCK_CONSTRUCTOR,
  EXPRESSION1
};
  
typedef struct primary
{
  /* IDENTIFIER,
     LITERAL,
     BLOCK_CONSTRUCTOR, or
     EXPRESSION1 */
  enum PrimaryType type;
  char *identifier;
  struct literal *lit;
  struct block_constructor *blk_cons;
  struct expression *exp;
} primary_t;

enum BlockConstructorType
{
  BLOCK_ARGS,
  NO_BLOCK_ARGS
};
  
typedef struct block_constructor
{
  /* BLOCK_ARGS or
     NO_BLOCK_ARGS */
  unsigned int type;
  struct block_arguments *block_args;
  struct executable_code *exec_code;
} block_constructor_t;

typedef struct block_arguments
{
  unsigned int nof_args;
  char **identifiers;
} block_arguments_t;

typedef struct unary_messages
{
  unsigned int nof_messages;
  char **identifiers;
} unary_messages_t;

typedef struct binary_messages
{
  unsigned int nof_messages;
  struct binary_message *bin_msgs;
} binary_messages_t;

enum MessageType
{
  UNARY_MESSAGE,
  BINARY_MESSAGE,
  KEYWORD_MESSAGE
};

typedef struct message
{
  /* UNARY_MESSAGE,
     BINARY_MESSAGE, or
     KEYWORD_MESSAGE */
  enum MessageType type;
  struct unary_messages *unary_messages;
  struct binary_messages *binary_messages;
  struct keyword_message *kw_msg;
} message_t;

typedef struct binary_message
{
  char *binary_selector;
  struct binary_argument *bin_arg;
} binary_message_t;

typedef struct binary_argument
{
  struct primary *prim;
  struct unary_messages *unary_messages;
} binary_argument_t;

typedef struct keyword_message
{
  unsigned int nof_args;
  struct keyword_argument_pair *kw_arg_pairs;
} keyword_message_t;

typedef struct keyword_argument_pair
{
  char *keyword;
  struct keyword_argument *kw_arg;
} keyword_argument_pair_t;

typedef struct keyword_argument
{
  struct primary *prim;
  struct unary_messages *unary_messages;
  struct binary_messages *binary_messages;
} keyword_argument_t;

enum LiteralType
{
  NUMBER_LITERAL,
  STRING_LITERAL,
  CHAR_LITERAL,
  SYMBOL_LITERAL,
  SELECTOR_LITERAL,
  ARRAY_LITERAL
};

typedef struct array_elements
{
  unsigned int nof_elements;
  struct array_element *elements;
} array_elements_t;

enum ArrayElementType
{
  LITERAL1,
  IDENTIFIER1
};

typedef struct array_element
{
  /* LITERAL1 or
     IDENTIFIER1 */
  enum ArrayElementType type;
  struct literal *lit;
  char *identifier;
} array_element_t;

typedef struct literal
{
  /* NUMBER_LITERAL,
     STRING_LITERAL,
     CHAR_LITERAL,
     SYMBOL_LITERAL,
     SELECTOR_LITERAL, or
     ARRAY_LITERAL */
  enum LiteralType type;
  struct number* num;
  char *val; /* string, char, symbol, or selector */
  struct array_elements *array_elements;
} literal_t;

enum NumberType
{
  INTEGER,
  FLOAT,
  SCALED_DECIMAL
};

typedef struct number
{
  /* INTEGER,
     FLOAT, or
     SCALED_DECIMAL  */
  enum NumberType type;
  char *val;
} number_t;

enum DebugExpressionType
{
  DEBUG_BASIC_EXPRESSION,
  DEBUG_BINARY_ARGUMENT,
  DEBUG_KEYWORD_ARGUMENT
};

typedef struct debug_expression
{
  enum DebugExpressionType type;
  struct basic_expression *be;
  struct binary_argument *bin_arg;
  struct keyword_argument *kw_arg;
} debug_expression_t;

void print_temporaries(temporaries_t *);
void print_statements(statement_t *);
void print_return_statement(return_statement_t *);
void print_expression(expression_t *);
void print_assignment(assignment_t *);
void print_basic_expression(basic_expression_t *);
void print_primary(primary_t *);
void print_message(message_t *);
void print_cascaded_messages(cascaded_messages_t *);
void print_literal(literal_t *);
void print_block_constructor(block_constructor_t *);
void print_block_arguments(block_arguments_t *);
void print_binary_message(binary_message_t *);
void print_keyword_message(keyword_message_t *);
void print_binary_argument(binary_argument_t *);
void print_keyword_argument_pair(keyword_argument_pair_t *);
void print_keyword_argument(keyword_argument_t *);
void print_prim(primary_t *);
void print_array_element(array_element_t *);
void print_executable_code(executable_code_t *);
void print_temporaries(temporaries_t *);
void print_statements(statement_t *);
void print_identifiers(identifiers_t *);
void print_return_statement(return_statement_t *);
void print_expression(expression_t *);
void print_assignment(assignment_t *);
void print_basic_expression(basic_expression_t *);
void print_cascaded_messages(cascaded_messages_t *);
void print_primary(primary_t *);
void print_block_constructor(block_constructor_t *);
void print_block_arguments(block_arguments_t *);
void print_unary_messages(unary_messages_t *);
void print_binary_messages(binary_messages_t *);
void print_message(message_t *);
void print_binary_message(binary_message_t *);
void print_binary_argument(binary_argument_t *);
void print_keyword_message(keyword_message_t *);
void print_keyword_argument_pair(keyword_argument_pair_t *);
void print_keyword_argument(keyword_argument_t *);
void print_array_elements(array_elements_t *);
void print_array_element(array_element_t *);
void print_literal(literal_t *);

#endif
