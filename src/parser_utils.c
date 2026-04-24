#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "global_decls.h"

#include "parser_header.h"

//forward declarations
void print_temporaries(FILE *, temporaries_t *);
void print_statements(FILE *, statement_t *);
void print_return_statement(FILE *, return_statement_t *);
void print_expression(FILE *, expression_t *);
void print_assignment(FILE *, assignment_t *);
void print_basic_expression(FILE *, basic_expression_t *);
void print_primary(FILE *, primary_t *);
void print_message(FILE *, message_t *);
void print_cascaded_messages(FILE *, cascaded_messages_t *);
void print_literal(FILE *, literal_t *);
void print_block_constructor(FILE *, block_constructor_t *);
void print_block_arguments(FILE *, block_arguments_t *);
void print_binary_message(FILE *, binary_message_t *);
void print_keyword_message(FILE *, keyword_message_t *);
void print_binary_argument(FILE *, binary_argument_t *);
void print_keyword_argument_pair(FILE *, keyword_argument_pair_t *);
void print_keyword_argument(FILE *, keyword_argument_t *);
void print_array_element(FILE *, array_element_t *);

void print_executable_code(FILE *fp, executable_code_t *ec)
{
  if(!ec)
    return;
  print_temporaries(fp, ec->temporaries);
  print_statements(fp, ec->statements);
}

void print_temporaries(FILE *fp, temporaries_t *t)
{
  if(!t)
    return;
  
  unsigned int i;
  fprintf(fp, "temporaries:\n");
  for(i=0; i < t->nof_temporaries; i++)
    fprintf(fp, "%s ", t->temporaries[i]);

  if(fp == stdout)
    fprintf(fp, "\b");
  else
    fseek(fp, -1, SEEK_CUR);

  fprintf(fp, "\n");
}

void print_statements(FILE *fp, statement_t *st)
{
  if(!st)
    return;
  
  if(st->type == RETURN_STATEMENT)
    print_return_statement(fp, st->ret_stmt);
  else if(st->type == EXPRESSION)
    print_expression(fp, st->exp);
  else if(st->type == EXP_PLUS_STATEMENTS)
  {
    print_expression(fp, st->exp);
    print_statements(fp, st->statements);
  }
  else
  {
    fprintf(fp, "Unknown statement type: %d", st->type);
    exit(1);
  }
}

void print_identifiers(FILE *fp, identifiers_t *ids)
{
  if(!ids)
    return;
  
  unsigned int i;
  for(i=0; i < ids->nof_identifiers; i++)
    fprintf(fp, "%s\n", ids->identifiers[i]);
}

void print_return_statement(FILE *fp, return_statement_t *r)
{
  if(!r)
    return;
  
  print_expression(fp, r->exp);
}

void print_expression(FILE *fp, expression_t *exp)
{
  if(!exp)
    return;
  
  if(exp->type == ASSIGNMENT)
    print_assignment(fp, exp->asgn);
  else if(exp->type == BASIC_EXPRESSION)
    print_basic_expression(fp, exp->basic_exp);
  else
  {
    fprintf(fp, "Unknown expression type: %d\n", exp->type);
    exit(1);
  }
}

void print_assignment(FILE *fp, assignment_t *as)
{
  if(!as)
    return;
  
  fprintf(fp, "Identifier: %s\n", as->identifier);
  fprintf(fp, "R-value:\n");
  print_expression(fp, as->rvalue);
  fprintf(fp, "\n");
}

void print_basic_expression(FILE *fp, basic_expression_t *b)
{
  if(!b)
    return;
  
  if(b->type == PRIMARY)
    print_primary(fp, b->prim);
  else if(b->type == PRIMARY_PLUS_MESSAGES)
  {
    print_primary(fp, b->prim);
    print_message(fp, b->msg);
    print_cascaded_messages(fp, b->cascaded_msgs);
  }
  else
  {
    fprintf(fp, "Unknown basic expression type: %d\n", b->type);
    exit(1);
  }
}

void print_cascaded_messages(FILE *fp, cascaded_messages_t *c)
{
  if(!c)
    return;
  
  unsigned int i;
  for(i=0; i < c->nof_cascaded_msgs; i++)
  {
    print_message(fp, c->cascaded_msgs + i);
    fprintf(fp, "\n");
  }
}

void print_primary(FILE *fp, primary_t *p)
{
  if(!p) { fprintf(fp, "<null primary> ");
    return; }
  
  if(p->type == IDENTIFIER)
    fprintf(fp, "%s\n", p->identifier);
  else if(p->type == LITERAL)
    print_literal(fp, p->lit);
  else if(p->type == BLOCK_CONSTRUCTOR)
    print_block_constructor(fp, p->blk_cons);
  else if(p->type == EXPRESSION1)
    print_expression(fp, p->exp);
  else
  {
    fprintf(fp, "Unknown primary type: %d\n", p->type);
    exit(1);
  }
}

void print_block_constructor(FILE *fp, block_constructor_t *b)
{
  if(!b)
    return;
  
  if(b->type == BLOCK_ARGS)
  {
    print_block_arguments(fp, b->block_args);
    print_executable_code(fp, b->exec_code);
  }
  else if(b->type == NO_BLOCK_ARGS)
    print_executable_code(fp, b->exec_code);
  else
  {
    fprintf(fp, "Unknown block_constructor type: %d\n", b->type);
    exit(1);
  }
}

void print_block_arguments(FILE *fp, block_arguments_t *args)
{
  if(!args)
    return;
  
  unsigned int i = args->nof_args;
  for(i=0; i < args->nof_args; i++)
    fprintf(fp, "%s\n", args->identifiers[i]);
}

void print_unary_messages(FILE *fp, unary_messages_t *msgs)
{
  if(!msgs)
    return;
  
  if(msgs->nof_messages == 0)
    return;
  
  fprintf(fp, "unary_messages:\n");
  fprintf(fp, "----------------\n");
  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
    fprintf(fp, "%s\n", msgs->identifiers[i]);
  fprintf(fp, "----------------\n");
}

void print_binary_messages(FILE *fp, binary_messages_t *msgs)
{
  if(!msgs)
    return;
  
  if(msgs->nof_messages == 0)
    return;
  
  fprintf(fp, "binary_messages:\n");
  fprintf(fp, "----------------\n");
  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
    print_binary_message(fp, msgs->bin_msgs + i);
  fprintf(fp, "----------------\n");
}

void print_message(FILE *fp, message_t *msg)
{
  if(!msg)
    return;
  
  if(msg->type == UNARY_MESSAGE)
  {
    print_unary_messages(fp, msg->unary_messages);
    if(msg->binary_messages)
      print_binary_messages(fp, msg->binary_messages);
    if(msg->kw_msg)
      print_keyword_message(fp, msg->kw_msg);
  }
  else if(msg->type == BINARY_MESSAGE)
  {
    print_binary_messages(fp, msg->binary_messages);
    if(msg->kw_msg)
      print_keyword_message(fp, msg->kw_msg);
  }
  else if(msg->type == KEYWORD_MESSAGE)
    print_keyword_message(fp, msg->kw_msg);
  else
  {
    fprintf(fp, "Unknown message_type: %d\n", msg->type);
    exit(0);
  }
}

void print_binary_message(FILE *fp, binary_message_t *msg)
{
  if(!msg)
    return;
  
  fprintf(fp, "%s ", msg->binary_selector);
  print_binary_argument(fp, msg->bin_arg);
}

void print_binary_argument(FILE *fp, binary_argument_t *arg)
{
  if(!arg) { fprintf(fp, "<null binary argument> ");
    return; }
  
  print_primary(fp, arg->prim);
  print_unary_messages(fp, arg->unary_messages);
}

void print_keyword_message(FILE *fp, keyword_message_t *msg)
{
  if(!msg)
    return;
  
  fprintf(fp, "keyword message:\n");
  fprintf(fp, "----------------\n");
  unsigned int i;
  for(i=0; i < msg->nof_args; i++)
    print_keyword_argument_pair(fp, msg->kw_arg_pairs + i);
  fprintf(fp, "\n----------------\n");
}

void print_keyword_argument_pair(FILE *fp, keyword_argument_pair_t *arg_pair)
{
  if(!arg_pair)
    return;
  
  fprintf(fp, "%s ", arg_pair->keyword);
  print_keyword_argument(fp, arg_pair->kw_arg);
}

void print_keyword_argument(FILE *fp, keyword_argument_t *arg)
{
  if(!arg) { fprintf(fp, "NULL! ");
    return; }
  
  print_primary(fp, arg->prim);
  print_unary_messages(fp, arg->unary_messages);
  print_binary_messages(fp, arg->binary_messages);
}

void print_array_elements(FILE *fp, array_elements_t *e)
{
  if(!e)
    return;
  
  fprintf(fp, "array:\n#(");
  unsigned int i;
  for(i=0; i < e->nof_elements; i++)
  {
    print_array_element(fp, e->elements + i);
    fprintf(fp, " ");
  }

  if(fp == stdout)
    fprintf(fp, "\b");
  else
    fseek(fp, -1, SEEK_CUR);

  fprintf(fp, ")\n");
}

void print_array_element(FILE *fp, array_element_t *e)
{
  if(!e)
    return;
  
  if(e->type == LITERAL1)
    print_literal(fp, e->lit);
  else if(e->type == IDENTIFIER1)
    fprintf(fp, "%s ", e->identifier);
  else
  {
    fprintf(fp, "Unknown array element type: %d", e->type);
    exit(1);
  }
}

void print_number(FILE *fp, number_t *n)
{
  if(!n) { fprintf(fp, "<null number> ");
    return; }

  fprintf(fp, "%s", n->val);
}

void print_literal(FILE *fp, literal_t *lit)
{
  if(!lit) { fprintf(fp, "<null literal> ");
    return; }
  
  if(lit->type == NUMBER_LITERAL)
    print_number(fp, lit->num);
  else if(lit->type == STRING_LITERAL   ||
          lit->type == CHAR_LITERAL     ||
          lit->type == SYMBOL_LITERAL   ||
          lit->type == SELECTOR_LITERAL)
    fprintf(fp, "%s ", lit->val);
  else if(lit->type == ARRAY_LITERAL)
    print_array_elements(fp, lit->array_elements);
  else
  {
    fprintf(fp, "Unknown literal type: %d\n", lit->type);
    exit(1);
  }     
}

void print_debug_expression(FILE *fp, debug_expression_t *debug_exp)
{
  if(debug_exp->type == DEBUG_BASIC_EXPRESSION)
    print_basic_expression(fp, debug_exp->be);
  else if(debug_exp->type == DEBUG_BINARY_ARGUMENT)
    print_binary_argument(fp, debug_exp->bin_arg);
  else if(debug_exp->type == DEBUG_KEYWORD_ARGUMENT)
    print_keyword_argument(fp, debug_exp->kw_arg);
  else
    assert(false);
}
