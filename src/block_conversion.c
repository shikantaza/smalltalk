#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "global_decls.h"
#include "util.h"

#include "parser_header.h"

//forward declarations
unsigned int convert_temporaries(char *, temporaries_t *);
unsigned int convert_statements(char *, statement_t *);
unsigned int convert_return_statement(char *, return_statement_t *);
unsigned int convert_expression(char *, expression_t *);
unsigned int convert_assignment(char *, assignment_t *);
unsigned int convert_basic_expression(char *, basic_expression_t *);
unsigned int convert_primary(char *, primary_t *);
unsigned int convert_message(char *, message_t *);
unsigned int convert_cascaded_messages(char *, cascaded_messages_t *);
unsigned int convert_literal(char *, literal_t *);
unsigned int convert_block_constructor(char *, block_constructor_t *);
unsigned int convert_block_arguments(char *, block_arguments_t *);
unsigned int convert_binary_message(char *, binary_message_t *);
unsigned int convert_keyword_message(char *, keyword_message_t *);
unsigned int convert_binary_argument(char *, binary_argument_t *);
unsigned int convert_keyword_argument_pair(char *, keyword_argument_pair_t *);
unsigned int convert_keyword_argument(char *, keyword_argument_t *);
unsigned int convert_array_element(char *, array_element_t *);

//replaces the 'withBody:' argument with the string equivalent for
//Smalltalk>>createClassMethod and Smalltalk>>createInstanceMethod
void replace_block_constructor(executable_code_t *e)
{
  if(e->statements->type != EXPRESSION)
    return;

  expression_t *exp = e->statements->exp;

  if(exp->type != BASIC_EXPRESSION)
    return;

  basic_expression_t *basic_exp = exp->basic_exp;

  if(basic_exp->type != PRIMARY_PLUS_MESSAGES)
    return;

  primary_t *prim = basic_exp->prim;

  if(prim->type != IDENTIFIER)
    return;

  if(strcmp(prim->identifier, "Smalltalk"))
    return;

  message_t *msg = basic_exp->msg;

  if(msg->type != KEYWORD_MESSAGE)
    return;

  keyword_message_t *kw_msg = msg->kw_msg;

  if(kw_msg->nof_args != 3)
    return;

  if(strcmp(kw_msg->kw_arg_pairs[0].keyword, "addClassMethod:") &&
     strcmp(kw_msg->kw_arg_pairs[0].keyword, "addInstanceMethod:"))
    return;

  if(strcmp(kw_msg->kw_arg_pairs[1].keyword, "toClass:"))
    return;

  if(strcmp(kw_msg->kw_arg_pairs[2].keyword, "withBody:"))
    return;

  keyword_argument_t *kw_arg = kw_msg->kw_arg_pairs[2].kw_arg;
  primary_t *prim1 = kw_arg->prim;

  if(prim1->type = BLOCK_CONSTRUCTOR)
  {
    char str[2000];
    memset(str, '\0', 2000);

    str[0] = '\'';

    convert_block_constructor(str+1, prim1->blk_cons);

    str[strlen(str)] = '\'';
    str[strlen(str)+1] = '\0';

    literal_t *lit = (literal_t *)GC_MALLOC(sizeof(literal_t));
    lit->type = STRING_LITERAL;
    lit->val = GC_strdup(str);

    prim1->type = LITERAL;
    prim1->lit = lit;

    return;
  }
  else
    return;
}

unsigned int convert_executable_code(char *buf, executable_code_t *ec)
{
  if(!ec)
    return 0;

  //TODO: ideally we need to call replace_block_constructor()
  //on ec to handle embedded Smalltalk>>addInstanceMethod
  //and Smalltalk>>addClassMethod expressions

  unsigned int len = 0;

  len += convert_temporaries(buf+len, ec->temporaries);
  len += convert_statements(buf+len, ec->statements);

  return len;
}

unsigned int convert_temporaries(char *buf, temporaries_t *t)
{
  if(!t)
    return 0;

  unsigned int len = 0;

  unsigned int i;

  len += sprintf(buf+len, "|");

  for(i=0; i < t->nof_temporaries; i++)
  {
    len += sprintf(buf+len, "%s", t->temporaries[i]);
    if(i != t->nof_temporaries-1)
      len += sprintf(buf+len, " ");
  }
  len += sprintf(buf+len, "|\n");

  return len;
}

unsigned int convert_statements(char *buf, statement_t *st)
{
  if(!st)
    return 0;

  unsigned int len = 0;

  if(st->type == RETURN_STATEMENT)
    len += convert_return_statement(buf+len, st->ret_stmt);
  else if(st->type == EXPRESSION)
    len += convert_expression(buf+len, st->exp);
  else if(st->type == EXP_PLUS_STATEMENTS)
  {
    len += convert_expression(buf+len, st->exp);
    len += sprintf(buf+len, ".\n");
    len += convert_statements(buf+len, st->statements);
  }
  else
  {
    printf("Unknown statement type: %d", st->type);
    exit(1);
  }

  return len;
}

unsigned int convert_identifiers(char *buf, identifiers_t *ids)
{
  if(!ids)
    return 0;

  unsigned int len = 0;

  unsigned int i;
  for(i=0; i < ids->nof_identifiers; i++)
  {
    len += sprintf(buf+len, "%s", ids->identifiers[i]);
    len += sprintf(buf+len, " ");
  }

  return len;
}

unsigned int convert_return_statement(char *buf, return_statement_t *r)
{
  if(!r)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "^(");
  len += convert_expression(buf+len, r->exp);
  len += sprintf(buf+len, ")");

  return len;
}

unsigned int convert_expression(char *buf, expression_t *exp)
{
  if(!exp)
    return 0;

  unsigned int len = 0;

  if(exp->type == ASSIGNMENT)
    len += convert_assignment(buf+len, exp->asgn);
  else if(exp->type == BASIC_EXPRESSION)
    len += convert_basic_expression(buf+len, exp->basic_exp);
  else
  {
    printf("Unknown expression type: %d\n", exp->type);
    exit(1);
  }

  return len;
}

unsigned int convert_assignment(char *buf, assignment_t *as)
{
  if(!as)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "%s", as->identifier);
  len += sprintf(buf+len, " := ");
  len += convert_expression(buf+len, as->rvalue);

  return len;
}

unsigned int convert_basic_expression(char *buf, basic_expression_t *b)
{
  if(!b)
    return 0;

  unsigned int len = 0;

  if(b->type == PRIMARY)
    len += convert_primary(buf+len, b->prim);
  else if(b->type == PRIMARY_PLUS_MESSAGES)
  {
    len += convert_primary(buf+len, b->prim);
    len += sprintf(buf+len, " ");
    len += convert_message(buf+len, b->msg);
    len += convert_cascaded_messages(buf+len, b->cascaded_msgs);
  }
  else
  {
    printf("Unknown basic expression type: %d\n", b->type);
    exit(1);
  }

  return len;
}

unsigned int convert_cascaded_messages(char *buf, cascaded_messages_t *c)
{
  if(!c)
    return 0;

  unsigned int len = 0;

  unsigned int i;
  for(i=0; i < c->nof_cascaded_msgs; i++)
  {
    len += sprintf(buf+len, "; ");
    len += convert_message(buf+len, c->cascaded_msgs + i);
  }

  return len;
}

unsigned int convert_primary(char *buf, primary_t *p)
{
  if(!p)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "(");

  if(p->type == IDENTIFIER)
    len += sprintf(buf+len, "%s", p->identifier);
  else if(p->type == LITERAL)
    len += convert_literal(buf+len, p->lit);
  else if(p->type == BLOCK_CONSTRUCTOR)
    len += convert_block_constructor(buf+len, p->blk_cons);
  else if(p->type == EXPRESSION1)
    len += convert_expression(buf+len, p->exp);
  else
  {
    printf("Unknown primary type: %d\n", p->type);
    exit(1);
  }

  len += sprintf(buf+len, ")");

  return len;
}

unsigned int convert_block_constructor(char *buf, block_constructor_t *b)
{
  if(!b)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "[ ");

  if(b->type == BLOCK_ARGS)
  {
    len += convert_block_arguments(buf+len, b->block_args);
    int temp = 0;
    len += sprintf(buf+len, "\n");
    len += convert_executable_code(buf+len, b->exec_code);
  }
  else if(b->type == NO_BLOCK_ARGS)
  {
    int temp = 0;
    len += sprintf(buf+len, "\n");
    len += convert_executable_code(buf+len, b->exec_code);
  }
  else
  {
    printf("Unknown block_constructor type: %d\n", b->type);
    exit(1);
  }

  len += sprintf(buf+len, "\n");
  len += sprintf(buf+len, "]");

  return len;
}

unsigned int convert_block_arguments(char *buf, block_arguments_t *args)
{
  if(!args)
    return 0;

  unsigned int len = 0;

  unsigned int i = args->nof_args;
  for(i=0; i < args->nof_args; i++)
  {
    len += sprintf(buf+len, ":");
    len += sprintf(buf+len, "%s", args->identifiers[i]);
    if(i != args->nof_args-1)
      len += sprintf(buf+len, " ");
  }
  len += sprintf(buf+len, " | ");

  return len;
}

unsigned int convert_unary_messages(char *buf, unary_messages_t *msgs)
{
  if(!msgs)
    return 0;

  unsigned int len = 0;

  if(msgs->nof_messages == 0)
    return 0;

  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
  {
    len += sprintf(buf+len, "%s", msgs->identifiers[i]);
    if(i != msgs->nof_messages -1)
      len += sprintf(buf+len, " ");
  }

  return len;
}

unsigned int convert_binary_messages(char *buf, binary_messages_t *msgs)
{
  if(!msgs)
    return 0;

  unsigned int len = 0;

  if(msgs->nof_messages == 0)
    return 0;

  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
    len += convert_binary_message(buf+len, msgs->bin_msgs + i);

  return len;
}

unsigned int convert_message(char *buf, message_t *msg)
{
  if(!msg)
    return 0;

  unsigned int len = 0;

  if(msg->type == UNARY_MESSAGE)
  {
    len += convert_unary_messages(buf+len, msg->unary_messages);
    if(msg->binary_messages)
      len += convert_binary_messages(buf+len, msg->binary_messages);
    if(msg->kw_msg)
      len += convert_keyword_message(buf+len, msg->kw_msg);
  }
  else if(msg->type == BINARY_MESSAGE)
  {
    len += convert_binary_messages(buf+len, msg->binary_messages);
    if(msg->kw_msg)
      len += convert_keyword_message(buf+len, msg->kw_msg);
  }
  else if(msg->type == KEYWORD_MESSAGE)
    len += convert_keyword_message(buf+len, msg->kw_msg);
  else
  {
    printf("Unknown message_type: %d\n", msg->type);
    exit(0);
  }

  return len;
}

unsigned int convert_binary_message(char *buf, binary_message_t *msg)
{
  if(!msg)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "%s", msg->binary_selector);
  len += sprintf(buf+len, " ");
  len += sprintf(buf+len, "(");
  len += convert_binary_argument(buf+len, msg->bin_arg);
  len += sprintf(buf+len, ")");

  return len;
}

unsigned int convert_binary_argument(char *buf, binary_argument_t *arg)
{
  if(!arg)
    return 0;

  unsigned int len = 0;

  len += convert_primary(buf+len, arg->prim);
  len += convert_unary_messages(buf+len, arg->unary_messages);

  return len;
}

unsigned int convert_keyword_message(char *buf, keyword_message_t *msg)
{
  if(!msg)
    return 0;

  unsigned int len = 0;

  unsigned int i;
  for(i=0; i < msg->nof_args; i++)
    len += convert_keyword_argument_pair(buf+len, msg->kw_arg_pairs + i);

  return len;
}

unsigned int convert_keyword_argument_pair(char *buf, keyword_argument_pair_t *arg_pair)
{
  if(!arg_pair)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "%s", arg_pair->keyword);
  len += sprintf(buf+len, " ");
  len += convert_keyword_argument(buf+len, arg_pair->kw_arg);
  len += sprintf(buf+len, " ");

  return len;
}

unsigned int convert_keyword_argument(char *buf, keyword_argument_t *arg)
{
  if(!arg)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "(");

  len += convert_primary(buf+len, arg->prim);
  len += convert_unary_messages(buf+len, arg->unary_messages);
  len += convert_binary_messages(buf+len, arg->binary_messages);

  len += sprintf(buf+len, ")");

  return len;
}

unsigned int convert_array_elements(char *buf, array_elements_t *e)
{
  if(!e)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "#(");
  unsigned int i;
  for(i=0; i < e->nof_elements; i++)
  {
    len += convert_array_element(buf+len, e->elements + i);
    len += sprintf(buf+len, " ");
  }
  len += sprintf(buf+len, ")");

  return len;
}

unsigned int convert_array_element(char *buf, array_element_t *e)
{
  if(!e)
    return 0;

  unsigned int len = 0;

  if(e->type == LITERAL1)
    len += convert_literal(buf+len, e->lit);
  else if(e->type == IDENTIFIER1)
  {
    len += sprintf(buf+len, "%s", e->identifier);
    len += sprintf(buf+len, " ");
  }
  else
  {
    printf("Unknown array element type: %d", e->type);
    exit(1);
  }

  return len;
}

unsigned int convert_number(char *buf, number_t *n)
{
  if(!n)
    return 0;

  unsigned int len = 0;

  len += sprintf(buf+len, "%s", n->val);

  return len;
}

unsigned int convert_literal(char *buf, literal_t *lit)
{
  if(!lit)
    return 0;

  unsigned int len = 0;

  if(lit->type == NUMBER_LITERAL)
    len += convert_number(buf+len, lit->num);
  else if(lit->type == STRING_LITERAL   ||
          lit->type == CHAR_LITERAL     ||
          lit->type == SYMBOL_LITERAL   ||
          lit->type == SELECTOR_LITERAL)
  {
    len += sprintf(buf+len, "%s", lit->val);
    len += sprintf(buf+len, " ");
  }
  else if(lit->type == ARRAY_LITERAL)
    len += convert_array_elements(buf+len, lit->array_elements);
  else
  {
    printf("Unknown literal type: %d\n", lit->type);
    exit(1);
  }

  return len;
}
