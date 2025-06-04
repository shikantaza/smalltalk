#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gtksourceview/gtksource.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

#include "../global_decls.h"
#include "../util.h"

#include "../parser_header.h"

//forward declarations
void render_temporaries(GtkTextBuffer *, int *, BOOLEAN, gint64, temporaries_t *);
void render_statements(GtkTextBuffer *, int *, BOOLEAN, gint64, statement_t *);
void render_return_statement(GtkTextBuffer *, int *, BOOLEAN, gint64, return_statement_t *);
void render_expression(GtkTextBuffer *, int *, BOOLEAN, gint64, expression_t *);
void render_assignment(GtkTextBuffer *, int *, BOOLEAN, gint64, assignment_t *);
void render_basic_expression(GtkTextBuffer *, int *, BOOLEAN, gint64, basic_expression_t *);
void render_primary(GtkTextBuffer *, int *, BOOLEAN, gint64, primary_t *);
void render_message(GtkTextBuffer *, int *, BOOLEAN, gint64, message_t *);
void render_cascaded_messages(GtkTextBuffer *, int *, BOOLEAN, gint64, cascaded_messages_t *);
void render_literal(GtkTextBuffer *, int *, BOOLEAN, gint64, literal_t *);
void render_block_constructor(GtkTextBuffer *, int *, BOOLEAN, gint64, block_constructor_t *);
void render_block_arguments(GtkTextBuffer *, int *, BOOLEAN, gint64, block_arguments_t *);
void render_binary_message(GtkTextBuffer *, int *, BOOLEAN, gint64, binary_message_t *);
void render_keyword_message(GtkTextBuffer *, int *, BOOLEAN, gint64, keyword_message_t *);
void render_binary_argument(GtkTextBuffer *, int *, BOOLEAN, gint64, binary_argument_t *);
void render_keyword_argument_pair(GtkTextBuffer *, int *, BOOLEAN, gint64, keyword_argument_pair_t *);
void render_keyword_argument(GtkTextBuffer *, int *, BOOLEAN, gint64, keyword_argument_t *);
void render_array_element(GtkTextBuffer *, int *, BOOLEAN, gint64, array_element_t *);

void print_debug_expression(debug_expression_t *);

extern GtkTextTag *debugger_tag;

extern stack_type *g_call_chain;

extern OBJECT_PTR NIL;

void render_indents_to_buffer(GtkTextBuffer *buf, int *indents, BOOLEAN highlight)
{
  GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter(buf, &iter );
  gtk_text_buffer_move_mark(buf, mark, &iter );

  int i;

  if(highlight)
  {
    for(i=0; i < *indents; i++)
      gtk_text_buffer_insert_with_tags(buf, &iter, " ", -1, debugger_tag, NULL);
  }
  else
  {
    for(i=0; i < *indents; i++)
      gtk_text_buffer_insert_at_cursor(buf, " ", -1);
  }
}

void render_string_to_buffer(GtkTextBuffer *buf, int *indents, BOOLEAN highlight, gint64 index, char *str)
{
  GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter(buf, &iter );
  gtk_text_buffer_move_mark(buf, mark, &iter );

  int i;

  if(highlight)
    gtk_text_buffer_insert_with_tags(buf, &iter, str, -1, debugger_tag, NULL);
  else
    gtk_text_buffer_insert_at_cursor(buf, str, -1);
}

void render_executable_code(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, executable_code_t *ec)
{
  if(!ec)
    return;
  render_temporaries(code_buf, indents, highlight, index, ec->temporaries);
  render_statements(code_buf, indents, highlight, index, ec->statements);
}

void render_temporaries(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, temporaries_t *t)
{
  if(!t)
    return;
  
  unsigned int i;

  render_string_to_buffer(code_buf, indents, highlight, index, "|");
  
  for(i=0; i < t->nof_temporaries; i++)
  {
    render_string_to_buffer(code_buf, indents, highlight, index, t->temporaries[i]);
    if(i != t->nof_temporaries-1)
      render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
  render_string_to_buffer(code_buf, indents, highlight, index, "|\n");
  render_indents_to_buffer(code_buf, indents, highlight);
}

void render_statements(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, statement_t *st)
{
  if(!st)
    return;
  
  if(st->type == RETURN_STATEMENT)
    render_return_statement(code_buf, indents, highlight, index, st->ret_stmt);
  else if(st->type == EXPRESSION)
    render_expression(code_buf, indents, highlight, index, st->exp);
  else if(st->type == EXP_PLUS_STATEMENTS)
  {
    render_expression(code_buf, indents, highlight, index, st->exp);
    render_string_to_buffer(code_buf, indents, highlight, index, ".\n");
    render_indents_to_buffer(code_buf, indents, highlight);
    render_statements(code_buf, indents, highlight, index, st->statements);
  }
  else
  {
    printf("Unknown statement type: %d", st->type);
    exit(1);
  }
}

void render_identifiers(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, identifiers_t *ids)
{
  if(!ids)
    return;
  
  unsigned int i;
  for(i=0; i < ids->nof_identifiers; i++)
  {
    render_string_to_buffer(code_buf, indents, highlight, index, ids->identifiers[i]);
    render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
}

void render_return_statement(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, return_statement_t *r)
{
  if(!r)
    return;

  //render_string_to_buffer(code_buf, indents, highlight, index, "^(");
  render_string_to_buffer(code_buf, indents, highlight, index, "^");
  render_expression(code_buf, indents, highlight, index, r->exp);
  //render_string_to_buffer(code_buf, indents, highlight, index, ")");
}

void render_expression(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, expression_t *exp)
{
  if(!exp)
    return;
  
  if(exp->type == ASSIGNMENT)
    render_assignment(code_buf, indents, highlight, index, exp->asgn);
  else if(exp->type == BASIC_EXPRESSION)
    render_basic_expression(code_buf, indents, highlight, index, exp->basic_exp);
  else
  {
    printf("Unknown expression type: %d\n", exp->type);
    exit(1);
  }
}

void render_assignment(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, assignment_t *as)
{
  if(!as)
    return;

  render_string_to_buffer(code_buf, indents, highlight, index, as->identifier);
  render_string_to_buffer(code_buf, indents, highlight, index, " := ");
  render_expression(code_buf, indents, highlight, index, as->rvalue);
}

void render_basic_expression(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, basic_expression_t *b)
{
  if(!b)
    return;

  BOOLEAN my_highlight = highlight;
  
  if(index < stack_count(g_call_chain) - 1) //there is at least one more call chain entry above this entry
  {
    call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
    call_chain_entry_t *entry = entries[index + 1];

    /* if(entry->exp_ptr == NIL) */
    /*   printf("exp ptr is NIL\n"); */
    /* printf("%p %p %p\n", entry->exp_ptr, (void*)extract_ptr(entry->exp_ptr), b); */

    if(entry->exp_ptr != NIL)
    {
      debug_expression_t *debug_exp = (debug_expression_t *)extract_ptr(entry->exp_ptr);

      /* printf("debug expression is: "); */
      /* print_debug_expression(debug_exp); */
      /* printf("basic_expression is: "); */
      /* print_basic_expression(b); */
      /* printf("---&&&---\n"); */

      if(debug_exp->type == DEBUG_BASIC_EXPRESSION && debug_exp->be == b)
	my_highlight = true;
    }
  }

  if(b->type == PRIMARY)
    render_primary(code_buf, indents, my_highlight, index, b->prim);
  else if(b->type == PRIMARY_PLUS_MESSAGES)
  {
    render_primary(code_buf, indents, my_highlight, index, b->prim);
    render_string_to_buffer(code_buf, indents, my_highlight, index, " ");
    render_message(code_buf, indents, my_highlight, index, b->msg);
    render_cascaded_messages(code_buf, indents, my_highlight, index, b->cascaded_msgs);
  }
  else
  {
    printf("Unknown basic expression type: %d\n", b->type);
    exit(1);
  }
}

void render_cascaded_messages(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, cascaded_messages_t *c)
{
  if(!c)
    return;
  
  unsigned int i;
  for(i=0; i < c->nof_cascaded_msgs; i++)
  {
    render_string_to_buffer(code_buf, indents, highlight, index, "; ");
    render_message(code_buf, indents, highlight, index, c->cascaded_msgs + i);
  }
}

void render_primary(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, primary_t *p)
{
  if(!p)
    return;
  
  if(p->type == IDENTIFIER)
  {
    render_string_to_buffer(code_buf, indents, highlight, index, p->identifier);
    render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
  else if(p->type == LITERAL)
  {
    render_literal(code_buf, indents, highlight, index, p->lit);
    render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
  else if(p->type == BLOCK_CONSTRUCTOR)
    render_block_constructor(code_buf, indents, highlight, index, p->blk_cons);
  else if(p->type == EXPRESSION1)
    render_expression(code_buf, indents, highlight, index, p->exp);
  else
  {
    printf("Unknown primary type: %d\n", p->type);
    exit(1);
  }
}

void render_block_constructor(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, block_constructor_t *b)
{
  if(!b)
    return;
  
  render_string_to_buffer(code_buf, indents, highlight, index, "[ ");

  if(b->type == BLOCK_ARGS)
  {
    render_block_arguments(code_buf, indents, highlight, index, b->block_args);
    int temp = 0;
    render_string_to_buffer(code_buf, &temp, highlight, index, "\n");
    *indents += 2;
    render_indents_to_buffer(code_buf, indents, highlight);
    render_executable_code(code_buf, indents, highlight, index, b->exec_code);
  }
  else if(b->type == NO_BLOCK_ARGS)
  {
    int temp = 0;
    render_string_to_buffer(code_buf, &temp, highlight, index, "\n");
    *indents += 2;
    render_indents_to_buffer(code_buf, indents, highlight);
    render_executable_code(code_buf, indents, highlight, index, b->exec_code);
  }
  else
  {
    printf("Unknown block_constructor type: %d\n", b->type);
    exit(1);
  }

  render_string_to_buffer(code_buf, indents, highlight, index, "\n");
  *indents -= 2;
  render_indents_to_buffer(code_buf, indents, highlight);
  render_string_to_buffer(code_buf, indents, highlight, index, "]");
}

void render_block_arguments(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, block_arguments_t *args)
{
  if(!args)
    return;
  
  unsigned int i = args->nof_args;
  for(i=0; i < args->nof_args; i++)
  {
    render_string_to_buffer(code_buf, indents, highlight, index, ":");
    render_string_to_buffer(code_buf, indents, highlight, index, args->identifiers[i]);
    if(i != args->nof_args-1)
      render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
  render_string_to_buffer(code_buf, indents, highlight, index, " | ");
}

void render_unary_messages(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, unary_messages_t *msgs)
{
  if(!msgs)
    return;
  
  if(msgs->nof_messages == 0)
    return;
  
  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
  {
    render_string_to_buffer(code_buf, indents, highlight, index, msgs->identifiers[i]);
    if(i != msgs->nof_messages -1)
      render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }    
}

void render_binary_messages(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, binary_messages_t *msgs)
{
  if(!msgs)
    return;
  
  if(msgs->nof_messages == 0)
    return;
  
  unsigned int i = msgs->nof_messages;
  for(i=0; i < msgs->nof_messages; i++)
    render_binary_message(code_buf, indents, highlight, index, msgs->bin_msgs + i);
}

void render_message(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, message_t *msg)
{
  if(!msg)
    return;
  
  if(msg->type == UNARY_MESSAGE)
  {
    render_unary_messages(code_buf, indents, highlight, index, msg->unary_messages);
    if(msg->binary_messages)
      render_binary_messages(code_buf, indents, highlight, index, msg->binary_messages);
    if(msg->kw_msg)
      render_keyword_message(code_buf, indents, highlight, index, msg->kw_msg);
  }
  else if(msg->type == BINARY_MESSAGE)
  {
    render_binary_messages(code_buf, indents, highlight, index, msg->binary_messages);
    if(msg->kw_msg)
      render_keyword_message(code_buf, indents, highlight, index, msg->kw_msg);
  }
  else if(msg->type == KEYWORD_MESSAGE)
    render_keyword_message(code_buf, indents, highlight, index, msg->kw_msg);
  else
  {
    printf("Unknown message_type: %d\n", msg->type);
    exit(0);
  }
}

void render_binary_message(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, binary_message_t *msg)
{
  if(!msg)
    return;
  
  render_string_to_buffer(code_buf, indents, highlight, index, msg->binary_selector);
  render_string_to_buffer(code_buf, indents, highlight, index, " ");
  render_binary_argument(code_buf, indents, highlight, index, msg->bin_arg);
}

void render_binary_argument(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, binary_argument_t *arg)
{
  if(!arg)
    return;

  BOOLEAN my_highlight = highlight;
  
  if(index < stack_count(g_call_chain) - 1) //there is at least one more call chain entry above this entry
  {
    call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
    call_chain_entry_t *entry = entries[index + 1];

    debug_expression_t *debug_exp = (debug_expression_t *)extract_ptr(entry->exp_ptr);

    if(debug_exp->type == DEBUG_BINARY_ARGUMENT && debug_exp->bin_arg == arg)
      my_highlight = true;
  }

  render_primary(code_buf, indents, my_highlight, index, arg->prim);
  render_unary_messages(code_buf, indents, my_highlight, index, arg->unary_messages);
}

void render_keyword_message(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, keyword_message_t *msg)
{
  if(!msg)
    return;
  
  unsigned int i;
  for(i=0; i < msg->nof_args; i++)
  {
    render_keyword_argument_pair(code_buf, indents, highlight, index, msg->kw_arg_pairs + i);
    if(i != msg->nof_args - 1)
      render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
}

void render_keyword_argument_pair(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, keyword_argument_pair_t *arg_pair)
{
  if(!arg_pair)
    return;
  
  render_string_to_buffer(code_buf, indents, highlight, index, arg_pair->keyword);
  render_string_to_buffer(code_buf, indents, highlight, index, " ");
  render_keyword_argument(code_buf, indents, highlight, index, arg_pair->kw_arg);
}

void render_keyword_argument(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, keyword_argument_t *arg)
{
  if(!arg)
    return;

  BOOLEAN my_highlight = highlight;
  
  if(index < stack_count(g_call_chain) - 1) //there is at least one more call chain entry above this entry
  {
    call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
    call_chain_entry_t *entry = entries[index + 1];

    debug_expression_t *debug_exp = (debug_expression_t *)extract_ptr(entry->exp_ptr);

    if(debug_exp->type == DEBUG_KEYWORD_ARGUMENT && debug_exp->kw_arg == arg)
      my_highlight = true;
  }

  render_primary(code_buf, indents, my_highlight, index, arg->prim);
  render_unary_messages(code_buf, indents, my_highlight, index, arg->unary_messages);
  render_binary_messages(code_buf, indents, my_highlight, index, arg->binary_messages);
}

void render_array_elements(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, array_elements_t *e)
{
  if(!e)
    return;
  
  render_string_to_buffer(code_buf, indents, highlight, index, "#(");
  unsigned int i;
  for(i=0; i < e->nof_elements; i++)
  {
    render_array_element(code_buf, indents, highlight, index, e->elements + i);
    render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
  render_string_to_buffer(code_buf, indents, highlight, index, ")");
}

void render_array_element(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, array_element_t *e)
{
  if(!e)
    return;
  
  if(e->type == LITERAL1)
    render_literal(code_buf, indents, highlight, index, e->lit);
  else if(e->type == IDENTIFIER1)
  {
    render_string_to_buffer(code_buf, indents, highlight, index, e->identifier);
    render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
  else
  {
    printf("Unknown array element type: %d", e->type);
    exit(1);
  }
}

void render_number(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, number_t *n)
{
  if(!n)
    return;

  render_string_to_buffer(code_buf, indents, highlight, index, n->val);
}

void render_literal(GtkTextBuffer *code_buf, int *indents, BOOLEAN highlight, gint64 index, literal_t *lit)
{
  if(!lit)
    return;
  
  if(lit->type == NUMBER_LITERAL)
    render_number(code_buf, indents, highlight, index, lit->num);
  else if(lit->type == STRING_LITERAL   ||
          lit->type == CHAR_LITERAL     ||
          lit->type == SYMBOL_LITERAL   ||
          lit->type == SELECTOR_LITERAL)
  {
    render_string_to_buffer(code_buf, indents, highlight, index, lit->val);
    render_string_to_buffer(code_buf, indents, highlight, index, " ");
  }
  else if(lit->type == ARRAY_LITERAL)
    render_array_elements(code_buf, indents, highlight, index, lit->array_elements);
  else
  {
    printf("Unknown literal type: %d\n", lit->type);
    exit(1);
  }     
}
