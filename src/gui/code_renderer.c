#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gtksourceview/gtksource.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

#include "../global_decls.h"
#include "../util.h"

#include "../parser_header.h"

//forward declarations
void render_temporaries(GtkTextBuffer *, temporaries_t *);
void render_statements(GtkTextBuffer *, statement_t *);
void render_return_statement(GtkTextBuffer *, return_statement_t *);
void render_expression(GtkTextBuffer *, expression_t *);
void render_assignment(GtkTextBuffer *, assignment_t *);
void render_basic_expression(GtkTextBuffer *, basic_expression_t *);
void render_primary(GtkTextBuffer *, primary_t *);
void render_message(GtkTextBuffer *, message_t *);
void render_cascaded_messages(GtkTextBuffer *, cascaded_messages_t *);
void render_literal(GtkTextBuffer *, literal_t *);
void render_block_constructor(GtkTextBuffer *, block_constructor_t *);
void render_block_arguments(GtkTextBuffer *, block_arguments_t *);
void render_binary_message(GtkTextBuffer *, binary_message_t *);
void render_keyword_message(GtkTextBuffer *, keyword_message_t *);
void render_binary_argument(GtkTextBuffer *, binary_argument_t *);
void render_keyword_argument_pair(GtkTextBuffer *, keyword_argument_pair_t *);
void render_keyword_argument(GtkTextBuffer *, keyword_argument_t *);
void render_array_element(GtkTextBuffer *, array_element_t *);

void render_string_to_buffer(GtkTextBuffer *buf, char *str)
{
  GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter(buf, &iter );
  gtk_text_buffer_move_mark(buf, mark, &iter );
  gtk_text_buffer_insert_at_cursor(buf, str, -1);
}

void render_executable_code(GtkTextBuffer *code_buf, executable_code_t *ec)
{
  if(!ec)
    return;
  render_temporaries(code_buf, ec->temporaries);
  render_statements(code_buf, ec->statements);
}

void render_temporaries(GtkTextBuffer *code_buf, temporaries_t *t)
{
  if(!t)
    return;
  
  unsigned int i;

  render_string_to_buffer(code_buf, "|");
  
  for(i=0; i < t->nof_temporaries; i++)
  {
    render_string_to_buffer(code_buf, t->temporaries[i]);
    if(i != t->nof_temporaries-1)
      render_string_to_buffer(code_buf, " ");
  }
  render_string_to_buffer(code_buf, "| ");
}

void render_statements(GtkTextBuffer *code_buf, statement_t *st)
{
  if(!st)
    return;
  
  if(st->type == RETURN_STATEMENT)
    render_return_statement(code_buf, st->ret_stmt);
  else if(st->type == EXPRESSION)
    render_expression(code_buf, st->exp);
  else if(st->type == EXP_PLUS_STATEMENTS)
  {
    render_expression(code_buf, st->exp);
    render_string_to_buffer(code_buf, ". ");
    render_statements(code_buf, st->statements);
  }
  else
  {
    printf("Unknown statement type: %d", st->type);
    exit(1);
  }
}

void render_identifiers(GtkTextBuffer *code_buf, identifiers_t *ids)
{
  if(!ids)
    return;
  
  unsigned int i;
  for(i=0; i < ids->nof_identifiers; i++)
  {
    render_string_to_buffer(code_buf, ids->identifiers[i]);
    render_string_to_buffer(code_buf, " ");
  }
}

void render_return_statement(GtkTextBuffer *code_buf, return_statement_t *r)
{
  if(!r)
    return;

  render_string_to_buffer(code_buf, "^(");
  render_expression(code_buf, r->exp);
  render_string_to_buffer(code_buf, ")");
}

void render_expression(GtkTextBuffer *code_buf, expression_t *exp)
{
  if(!exp)
    return;
  
  if(exp->type == ASSIGNMENT)
    render_assignment(code_buf, exp->asgn);
  else if(exp->type == BASIC_EXPRESSION)
    render_basic_expression(code_buf, exp->basic_exp);
  else
  {
    printf("Unknown expression type: %d\n", exp->type);
    exit(1);
  }
}

void render_assignment(GtkTextBuffer *code_buf, assignment_t *as)
{
  if(!as)
    return;
  
  render_string_to_buffer(code_buf, as->identifier);
  render_string_to_buffer(code_buf, " := ");
  render_expression(code_buf, as->rvalue);
}

void render_basic_expression(GtkTextBuffer *code_buf, basic_expression_t *b)
{
  if(!b)
    return;
  
  if(b->type == PRIMARY)
    render_primary(code_buf, b->prim);
  else if(b->type == PRIMARY_PLUS_MESSAGES)
  {
    render_primary(code_buf, b->prim);
    render_string_to_buffer(code_buf, " ");
    render_message(code_buf, b->msg);
    render_cascaded_messages(code_buf, b->cascaded_msgs);
  }
  else
  {
    printf("Unknown basic expression type: %d\n", b->type);
    exit(1);
  }
}

void render_cascaded_messages(GtkTextBuffer *code_buf, cascaded_messages_t *c)
{
  if(!c)
    return;
  
  unsigned int i;
  for(i=0; i < c->nof_cascaded_msgs; i++)
  {
    render_message(code_buf, c->cascaded_msgs + i);
  }
}

void render_primary(GtkTextBuffer *code_buf, primary_t *p)
{
  if(!p)
    return;
  
  if(p->type == IDENTIFIER)
    render_string_to_buffer(code_buf, p->identifier);
  else if(p->type == LITERAL)
    render_literal(code_buf, p->lit);
  else if(p->type == BLOCK_CONSTRUCTOR)
    render_block_constructor(code_buf, p->blk_cons);
  else if(p->type == EXPRESSION1)
    render_expression(code_buf, p->exp);
  else
  {
    printf("Unknown primary type: %d\n", p->type);
    exit(1);
  }
}

void render_block_constructor(GtkTextBuffer *code_buf, block_constructor_t *b)
{
  if(!b)
    return;
  
  render_string_to_buffer(code_buf, "[ ");

  if(b->type == BLOCK_ARGS)
  {
    render_block_arguments(code_buf, b->block_args);
    render_executable_code(code_buf, b->exec_code);
  }
  else if(b->type == NO_BLOCK_ARGS)
    render_executable_code(code_buf, b->exec_code);
  else
  {
    printf("Unknown block_constructor type: %d\n", b->type);
    exit(1);
  }

  render_string_to_buffer(code_buf, " ]");
}

void render_block_arguments(GtkTextBuffer *code_buf, block_arguments_t *args)
{
  if(!args)
    return;
  
  unsigned int i = args->nof_args;
  for(i=0; i < args->nof_args; i++)
  {
    render_string_to_buffer(code_buf, ":");
    render_string_to_buffer(code_buf, args->identifiers[i]);
    if(i != args->nof_args-1)
      render_string_to_buffer(code_buf, " ");
  }
  render_string_to_buffer(code_buf, " | ");
}

void render_unary_messages(GtkTextBuffer *code_buf, unary_messages_t *msgs)
{
  if(!msgs)
    return;
  
  if(msgs->nof_messages == 0)
    return;
  
  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
  {
    render_string_to_buffer(code_buf, msgs->identifiers[i]);
    if(i != msgs->nof_messages -1)
      render_string_to_buffer(code_buf, " ");
  }    
}

void render_binary_messages(GtkTextBuffer *code_buf, binary_messages_t *msgs)
{
  if(!msgs)
    return;
  
  if(msgs->nof_messages == 0)
    return;
  
  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
    render_binary_message(code_buf, msgs->bin_msgs + i);
}

void render_message(GtkTextBuffer *code_buf, message_t *msg)
{
  if(!msg)
    return;
  
  if(msg->type == UNARY_MESSAGE)
  {
    render_unary_messages(code_buf, msg->unary_messages);
    if(msg->binary_messages)
      render_binary_messages(code_buf, msg->binary_messages);
    if(msg->kw_msg)
      render_keyword_message(code_buf, msg->kw_msg);
  }
  else if(msg->type == BINARY_MESSAGE)
  {
    render_binary_messages(code_buf, msg->binary_messages);
    if(msg->kw_msg)
      render_keyword_message(code_buf, msg->kw_msg);
  }
  else if(msg->type == KEYWORD_MESSAGE)
    render_keyword_message(code_buf, msg->kw_msg);
  else
  {
    printf("Unknown message_type: %d\n", msg->type);
    exit(0);
  }
}

void render_binary_message(GtkTextBuffer *code_buf, binary_message_t *msg)
{
  if(!msg)
    return;
  
  render_string_to_buffer(code_buf, msg->binary_selector);
  render_string_to_buffer(code_buf, " ");
  render_binary_argument(code_buf, msg->bin_arg);
}

void render_binary_argument(GtkTextBuffer *code_buf, binary_argument_t *arg)
{
  if(!arg)
    return;
  
  render_primary(code_buf, arg->prim);
  render_unary_messages(code_buf, arg->unary_messages);
}

void render_keyword_message(GtkTextBuffer *code_buf, keyword_message_t *msg)
{
  if(!msg)
    return;
  
  unsigned int i;
  for(i=0; i < msg->nof_args; i++)
    render_keyword_argument_pair(code_buf, msg->kw_arg_pairs + i);
}

void render_keyword_argument_pair(GtkTextBuffer *code_buf, keyword_argument_pair_t *arg_pair)
{
  if(!arg_pair)
    return;
  
  render_string_to_buffer(code_buf, arg_pair->keyword);
  render_string_to_buffer(code_buf, " ");
  render_keyword_argument(code_buf, arg_pair->kw_arg);
}

void render_keyword_argument(GtkTextBuffer *code_buf, keyword_argument_t *arg)
{
  if(!arg)
    return;
  
  render_primary(code_buf, arg->prim);
  render_unary_messages(code_buf, arg->unary_messages);
  render_binary_messages(code_buf, arg->binary_messages);
}

void render_array_elements(GtkTextBuffer *code_buf, array_elements_t *e)
{
  if(!e)
    return;
  
  render_string_to_buffer(code_buf, "#(");
  unsigned int i;
  for(i=0; i < e->nof_elements; i++)
  {
    render_array_element(code_buf, e->elements + i);
    render_string_to_buffer(code_buf, " ");
  }
  render_string_to_buffer(code_buf, ")");
}

void render_array_element(GtkTextBuffer *code_buf, array_element_t *e)
{
  if(!e)
    return;
  
  if(e->type == LITERAL1)
    render_literal(code_buf, e->lit);
  else if(e->type == IDENTIFIER1)
  {
    render_string_to_buffer(code_buf, e->identifier);
    render_string_to_buffer(code_buf, " ");
  }
  else
  {
    printf("Unknown array element type: %d", e->type);
    exit(1);
  }
}

void render_number(GtkTextBuffer *code_buf, number_t *n)
{
  if(!n)
    return;

  render_string_to_buffer(code_buf, n->val);
}

void render_literal(GtkTextBuffer *code_buf, literal_t *lit)
{
  if(!lit)
    return;
  
  if(lit->type == NUMBER_LITERAL)
    render_number(code_buf, lit->num);
  else if(lit->type == STRING_LITERAL   ||
          lit->type == CHAR_LITERAL     ||
          lit->type == SYMBOL_LITERAL   ||
          lit->type == SELECTOR_LITERAL)
  {
    render_string_to_buffer(code_buf, lit->val);
    render_string_to_buffer(code_buf, " ");
  }
  else if(lit->type == ARRAY_LITERAL)
    render_array_elements(code_buf, lit->array_elements);
  else
  {
    printf("Unknown literal type: %d\n", lit->type);
    exit(1);
  }     
}
