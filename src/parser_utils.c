#include <stdlib.h>
#include <stdio.h>

#include "parser_header.h"

//forward declarations
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
void print_array_element(array_element_t *);

void print_executable_code(executable_code_t *ec)
{
  if(!ec)
    return;
  print_temporaries(ec->temporaries);
  print_statements(ec->statements);
}

void print_temporaries(temporaries_t *t)
{
  if(!t)
    return;
  
  unsigned int i;
  printf("temporaries:\n");
  for(i=0; i < t->nof_temporaries; i++)
    printf("%s ", t->temporaries[i]);
  printf("\b\n");
}

void print_statements(statement_t *st)
{
  if(!st)
    return;
  
  if(st->type == RETURN_STATEMENT)
    print_return_statement(st->ret_stmt);
  else if(st->type == EXPRESSION)
    print_expression(st->exp);
  else if(st->type == EXP_PLUS_STATEMENTS)
  {
    print_expression(st->exp);
    print_statements(st->statements);
  }
  else
  {
    printf("Unknown statement type: %d", st->type);
    exit(1);
  }
}

void print_identifiers(identifiers_t *ids)
{
  if(!ids)
    return;
  
  unsigned int i;
  for(i=0; i < ids->nof_identifiers; i++)
    printf("%s\n", ids->identifiers[i]);
}

void print_return_statement(return_statement_t *r)
{
  if(!r)
    return;
  
  print_expression(r->exp);
}

void print_expression(expression_t *exp)
{
  if(!exp)
    return;
  
  if(exp->type == ASSIGNMENT)
    print_assignment(exp->asgn);
  else if(exp->type == BASIC_EXPRESSION)
    print_basic_expression(exp->basic_exp);
  else
  {
    printf("Unknown expression type: %d\n", exp->type);
    exit(1);
  }
}

void print_assignment(assignment_t *as)
{
  if(!as)
    return;
  
  printf("Identifier: %s\n", as->identifier);
  printf("R-value:\n");
  print_expression(as->rvalue);
  printf("\n");
}

void print_basic_expression(basic_expression_t *b)
{
  if(!b)
    return;
  
  if(b->type == PRIMARY)
    print_primary(b->prim);
  else if(b->type == PRIMARY_PLUS_MESSAGES)
  {
    print_primary(b->prim);
    print_message(b->msg);
    print_cascaded_messages(b->cascaded_msgs);
  }
  else
  {
    printf("Unknown basic expression type: %d\n", b->type);
    exit(1);
  }
}

void print_cascaded_messages(cascaded_messages_t *c)
{
  if(!c)
    return;
  
  unsigned int i;
  for(i=0; i < c->nof_cascaded_msgs; i++)
  {
    print_message(c->cascaded_msgs + i);
    printf("\n");
  }
}

void print_primary(primary_t *p)
{
  if(!p)
    return;
  
  if(p->type == IDENTIFIER)
    printf("%s\n", p->identifier);
  else if(p->type == LITERAL)
    print_literal(p->lit);
  else if(p->type == BLOCK_CONSTRUCTOR)
    print_block_constructor(p->blk_cons);
  else if(p->type == EXPRESSION1)
    print_expression(p->exp);
  else
  {
    printf("Unknown primary type: %d\n", p->type);
    exit(1);
  }
}

void print_block_constructor(block_constructor_t *b)
{
  if(!b)
    return;
  
  if(b->type == BLOCK_ARGS)
  {
    print_block_arguments(b->block_args);
    print_executable_code(b->exec_code);
  }
  else if(b->type == NO_BLOCK_ARGS)
    print_executable_code(b->exec_code);
  else
  {
    printf("Unknown block_constructor type: %d\n", b->type);
    exit(1);
  }
}

void print_block_arguments(block_arguments_t *args)
{
  if(!args)
    return;
  
  unsigned int i = args->nof_args;
  for(i=0; i < args->nof_args; i++)
    printf("%s\n", args->identifiers[i]);
}

void print_unary_messages(unary_messages_t *msgs)
{
  if(!msgs)
    return;
  
  if(msgs->nof_messages == 0)
    return;
  
  printf("unary_messages:\n");
  printf("----------------\n");
  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
    printf("%s\n", msgs->identifiers[i]);
  printf("----------------\n");
}

void print_binary_messages(binary_messages_t *msgs)
{
  if(!msgs)
    return;
  
  if(msgs->nof_messages == 0)
    return;
  
  printf("binary_messages:\n");
  printf("----------------\n");
  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
    print_binary_message(msgs->bin_msgs + i);
  printf("----------------\n");
}

void print_message(message_t *msg)
{
  if(!msg)
    return;
  
  if(msg->type == UNARY_MESSAGE)
  {
    print_unary_messages(msg->unary_messages);
    if(msg->binary_messages)
      print_binary_messages(msg->binary_messages);
    if(msg->kw_msg)
      print_keyword_message(msg->kw_msg);
  }
  else if(msg->type == BINARY_MESSAGE)
  {
    print_binary_messages(msg->binary_messages);
    if(msg->kw_msg)
      print_keyword_message(msg->kw_msg);
  }
  else if(msg->type == KEYWORD_MESSAGE)
    print_keyword_message(msg->kw_msg);
  else
  {
    printf("Unknown message_type: %d\n", msg->type);
    exit(0);
  }
}

void print_binary_message(binary_message_t *msg)
{
  if(!msg)
    return;
  
  printf("%s ", msg->binary_selector);
  print_binary_argument(msg->bin_arg);
}

void print_binary_argument(binary_argument_t *arg)
{
  if(!arg)
    return;
  
  print_primary(arg->prim);
  print_unary_messages(arg->unary_messages);
}

void print_keyword_message(keyword_message_t *msg)
{
  if(!msg)
    return;
  
  printf("keyword message:\n");
  printf("----------------\n");
  unsigned int i;
  for(i=0; i < msg->nof_args; i++)
    print_keyword_argument_pair(msg->kw_arg_pairs + i);
  printf("\n----------------\n");
}

void print_keyword_argument_pair(keyword_argument_pair_t *arg_pair)
{
  if(!arg_pair)
    return;
  
  printf("%s ", arg_pair->keyword);
  print_keyword_argument(arg_pair->kw_arg);
}

void print_keyword_argument(keyword_argument_t *arg)
{
  if(!arg)
    return;
  
  print_primary(arg->prim);
  print_unary_messages(arg->unary_messages);
  print_binary_messages(arg->binary_messages);
}

void print_array_elements(array_elements_t *e)
{
  if(!e)
    return;
  
  printf("array:\n#(");
  unsigned int i;
  for(i=0; i < e->nof_elements; i++)
  {
    print_array_element(e->elements + i);
    printf(" ");
  }
  printf("\b)\n");
}

void print_array_element(array_element_t *e)
{
  if(!e)
    return;
  
  if(e->type == LITERAL1)
    print_literal(e->lit);
  else if(e->type == IDENTIFIER1)
    printf("%s ", e->identifier);
  else
  {
    printf("Unknown array element type: %d", e->type);
    exit(1);
  }
}

void print_number(number_t *n)
{
  if(!n)
    return;

  printf("%s", n->val);
}

void print_literal(literal_t *lit)
{
  if(!lit)
    return;
  
  if(lit->type == NUMBER_LITERAL)
    print_number(lit->num);
  else if(lit->type == STRING_LITERAL   ||
          lit->type == CHAR_LITERAL     ||
          lit->type == SYMBOL_LITERAL   ||
          lit->type == SELECTOR_LITERAL)
    printf("%s ", lit->val);
  else if(lit->type == ARRAY_LITERAL)
    print_array_elements(lit->array_elements);
  else
  {
    printf("Unknown literal type: %d\n", lit->type);
    exit(1);
  }     
}
