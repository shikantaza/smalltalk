/**
  Copyright 2026 Rajesh Jayaprakash <rajesh.jayaprakash@protonmail.com>

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  It is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this file.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <stdio.h>
#include <stdlib.h>
#include "parser_header.h"

/* ---- forward declarations (the structures are mutually recursive) ---- */
static void free_temporaries(temporaries_t *);
static void free_statements(statement_t *);
static void free_return_statement(return_statement_t *);
static void free_expression(expression_t *);
static void free_assignment(assignment_t *);
static void free_basic_expression(basic_expression_t *);
static void free_primary(primary_t *);
static void free_block_constructor(block_constructor_t *);
static void free_block_arguments(block_arguments_t *);
static void free_unary_messages(unary_messages_t *);
static void free_binary_messages(binary_messages_t *);
static void free_cascaded_messages(cascaded_messages_t *);
static void free_message(message_t *);
static void free_keyword_message(keyword_message_t *);
static void free_binary_argument(binary_argument_t *);
static void free_keyword_argument(keyword_argument_t *);
static void free_literal(literal_t *);
static void free_array_elements(array_elements_t *);
static void free_number(number_t *);

/* "contents" variants: free what a struct points to, but not the struct
   itself. Needed for structs stored by value inside heap arrays. */
static void free_message_contents(message_t *);
static void free_binary_message_contents(binary_message_t *);
static void free_keyword_argument_pair_contents(keyword_argument_pair_t *);
static void free_array_element_contents(array_element_t *);

void free_executable_code(executable_code_t *);

/* ---- helpers ---- */

static void free_string_array(char **strs, unsigned int n)
{
  if (!strs) return;
  for (unsigned int i = 0; i < n; i++)
    free(strs[i]);
  free(strs);
}

/* ---- leaf-ish structures ---- */

static void free_temporaries(temporaries_t *t)
{
  if (!t) return;
  free_string_array(t->temporaries, t->nof_temporaries);
  free(t);
}

static void free_block_arguments(block_arguments_t *ba)
{
  if (!ba) return;
  free_string_array(ba->identifiers, ba->nof_args);
  free(ba);
}

static void free_unary_messages(unary_messages_t *um)
{
  if (!um) return;
  free_string_array(um->identifiers, um->nof_messages);
  free(um);
}

static void free_number(number_t *n)
{
  if (!n) return;
  free(n->val);
  free(n);
}

/* ---- literals ---- */

static void free_array_element_contents(array_element_t *ae)
{
  free_literal(ae->lit);
  free(ae->identifier);
}

static void free_array_elements(array_elements_t *aes)
{
  if (!aes) return;
  if (aes->elements) {
    for (unsigned int i = 0; i < aes->nof_elements; i++)
      free_array_element_contents(&aes->elements[i]);
    free(aes->elements);
  }
  free(aes);
}

static void free_literal(literal_t *lit)
{
  if (!lit) return;
  free_number(lit->num);
  free(lit->val);
  free_array_elements(lit->array_elements);  /* literal arrays can nest */
  free(lit);
}

/* ---- messages ---- */

static void free_binary_argument(binary_argument_t *ba)
{
  if (!ba) return;
  free_primary(ba->prim);
  free_unary_messages(ba->unary_messages);
  free(ba);
}

static void free_binary_message_contents(binary_message_t *bm)
{
  free(bm->binary_selector);
  free_binary_argument(bm->bin_arg);
}

static void free_binary_messages(binary_messages_t *bms)
{
  if (!bms) return;
  if (bms->bin_msgs) {
    for (unsigned int i = 0; i < bms->nof_messages; i++)
      free_binary_message_contents(&bms->bin_msgs[i]);
    free(bms->bin_msgs);
  }
  free(bms);
}

static void free_keyword_argument(keyword_argument_t *ka)
{
  if (!ka) return;
  free_primary(ka->prim);
  free_unary_messages(ka->unary_messages);
  free_binary_messages(ka->binary_messages);
  free(ka);
}

static void free_keyword_argument_pair_contents(keyword_argument_pair_t *kp)
{
  free(kp->keyword);
  free_keyword_argument(kp->kw_arg);
}

static void free_keyword_message(keyword_message_t *km)
{
  if (!km) return;
  if (km->kw_arg_pairs) {
    for (unsigned int i = 0; i < km->nof_args; i++)
      free_keyword_argument_pair_contents(&km->kw_arg_pairs[i]);
    free(km->kw_arg_pairs);
  }
  free(km);
}

static void free_message_contents(message_t *m)
{
  free_unary_messages(m->unary_messages);
  free_binary_messages(m->binary_messages);
  free_keyword_message(m->kw_msg);
}

static void free_message(message_t *m)
{
  if (!m) return;
  free_message_contents(m);
  free(m);
}

static void free_cascaded_messages(cascaded_messages_t *cm)
{
  if (!cm) return;
  if (cm->cascaded_msgs) {
    for (unsigned int i = 0; i < cm->nof_cascaded_msgs; i++)
      free_message_contents(&cm->cascaded_msgs[i]);
    free(cm->cascaded_msgs);
  }
  free(cm);
}

/* ---- expressions / primaries / blocks (the recursive core) ---- */

static void free_primary(primary_t *p)
{
  if (!p) return;
  free(p->identifier);
  free_literal(p->lit);
  free_block_constructor(p->blk_cons);
  free_expression(p->exp);                   /* parenthesised expression */
  free(p);
}

static void free_basic_expression(basic_expression_t *be)
{
  if (!be) return;
  free_primary(be->prim);
  free_message(be->msg);
  free_cascaded_messages(be->cascaded_msgs);
  free(be);
}

static void free_assignment(assignment_t *a)
{
  if (!a) return;
  free(a->identifier);
  free_expression(a->rvalue);                /* a := b := c ... */
  free(a);
}

static void free_expression(expression_t *e)
{
  if (!e) return;
  free_assignment(e->asgn);
  free_basic_expression(e->basic_exp);
  free(e);
}

static void free_return_statement(return_statement_t *rs)
{
  if (!rs) return;
  free_expression(rs->exp);
  free(rs);
}

/* statements form a singly linked list via ->statements; walk it
   iteratively so long method bodies can't overflow the stack. */
static void free_statements(statement_t *s)
{
  while (s) {
    statement_t *next = s->statements;
    free_return_statement(s->ret_stmt);
    free_expression(s->exp);
    free(s);
    s = next;
  }
}

static void free_block_constructor(block_constructor_t *bc)
{
  if (!bc) return;
  free(bc->docstring);
  free_block_arguments(bc->block_args);
  free_executable_code(bc->exec_code);       /* blocks nest executable code */
  free(bc);
}

/* ---- public entry point ---- */

void free_executable_code(executable_code_t *ec)
{
  if (!ec) return;
  free_temporaries(ec->temporaries);
  free_statements(ec->statements);
  free(ec);
}
