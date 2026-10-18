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
#include <string.h>
#include "parser_header.h"

/* Public entry point (add this prototype to parser_header.h, since
   clone_block_constructor() below calls it before its definition). */
executable_code_t *clone_executable_code(const executable_code_t *);

/* ---- forward declarations (the structures are mutually recursive) ---- */
static temporaries_t *clone_temporaries(const temporaries_t *);
static statement_t *clone_statements(const statement_t *);
static return_statement_t *clone_return_statement(const return_statement_t *);
static expression_t *clone_expression(const expression_t *);
static assignment_t *clone_assignment(const assignment_t *);

//needed outside this file, so not static
basic_expression_t *clone_basic_expression(const basic_expression_t *);

static primary_t *clone_primary(const primary_t *);
static block_constructor_t *clone_block_constructor(const block_constructor_t *);
static block_arguments_t *clone_block_arguments(const block_arguments_t *);
static unary_messages_t *clone_unary_messages(const unary_messages_t *);
static binary_messages_t *clone_binary_messages(const binary_messages_t *);
static cascaded_messages_t *clone_cascaded_messages(const cascaded_messages_t *);
static message_t *clone_message(const message_t *);
static keyword_message_t *clone_keyword_message(const keyword_message_t *);

//needed outside this file, so not static
binary_argument_t *clone_binary_argument(const binary_argument_t *);
keyword_argument_t *clone_keyword_argument(const keyword_argument_t *);

static literal_t *clone_literal(const literal_t *);
static array_elements_t *clone_array_elements(const array_elements_t *);
static number_t *clone_number(const number_t *);

/* "into" variants: fill a struct that lives by value inside a heap array
   (the array itself is allocated by the caller). */
static void clone_message_into(message_t *, const message_t *);
static void clone_binary_message_into(binary_message_t *, const binary_message_t *);
static void clone_keyword_argument_pair_into(keyword_argument_pair_t *,
                                             const keyword_argument_pair_t *);
static void clone_array_element_into(array_element_t *, const array_element_t *);

/* ---- helpers ---- */

/* Allocation failure is treated as fatal. calloc() is used everywhere so
   that fields we don't set are NULL/0, which keeps free_executable_code()
   safe on every node. */
static void *xcalloc(size_t n, size_t size)
{
  void *p = calloc(n ? n : 1, size);
  if (!p) {
    fprintf(stderr, "clone: out of memory\n");
    abort();
  }
  return p;
}

static char *xstrdup(const char *s)
{
  if (!s) return NULL;
  size_t len = strlen(s) + 1;
  char *p = xcalloc(len, 1);
  memcpy(p, s, len);
  return p;
}

static char **clone_string_array(char **strs, unsigned int n)
{
  if (!strs) return NULL;
  char **out = xcalloc(n, sizeof(char *));
  for (unsigned int i = 0; i < n; i++)
    out[i] = xstrdup(strs[i]);
  return out;
}

/* ---- leaf-ish structures ---- */

static temporaries_t *clone_temporaries(const temporaries_t *t)
{
  if (!t) return NULL;
  temporaries_t *c = xcalloc(1, sizeof *c);
  c->nof_temporaries = t->nof_temporaries;
  c->temporaries = clone_string_array(t->temporaries, t->nof_temporaries);
  return c;
}

static block_arguments_t *clone_block_arguments(const block_arguments_t *ba)
{
  if (!ba) return NULL;
  block_arguments_t *c = xcalloc(1, sizeof *c);
  c->nof_args = ba->nof_args;
  c->identifiers = clone_string_array(ba->identifiers, ba->nof_args);
  return c;
}

static unary_messages_t *clone_unary_messages(const unary_messages_t *um)
{
  if (!um) return NULL;
  unary_messages_t *c = xcalloc(1, sizeof *c);
  c->nof_messages = um->nof_messages;
  c->identifiers = clone_string_array(um->identifiers, um->nof_messages);
  return c;
}

static number_t *clone_number(const number_t *n)
{
  if (!n) return NULL;
  number_t *c = xcalloc(1, sizeof *c);
  c->type = n->type;
  c->val = xstrdup(n->val);
  return c;
}

/* ---- literals ---- */

static void clone_array_element_into(array_element_t *dst, const array_element_t *src)
{
  dst->type = src->type;
  dst->lit = clone_literal(src->lit);
  dst->identifier = xstrdup(src->identifier);
}

static array_elements_t *clone_array_elements(const array_elements_t *aes)
{
  if (!aes) return NULL;
  array_elements_t *c = xcalloc(1, sizeof *c);
  c->nof_elements = aes->nof_elements;
  if (aes->elements) {
    c->elements = xcalloc(aes->nof_elements, sizeof(array_element_t));
    for (unsigned int i = 0; i < aes->nof_elements; i++)
      clone_array_element_into(&c->elements[i], &aes->elements[i]);
  }
  return c;
}

static literal_t *clone_literal(const literal_t *lit)
{
  if (!lit) return NULL;
  literal_t *c = xcalloc(1, sizeof *c);
  c->type = lit->type;
  c->num = clone_number(lit->num);
  c->val = xstrdup(lit->val);
  c->array_elements = clone_array_elements(lit->array_elements); /* nests */
  return c;
}

/* ---- messages ---- */

binary_argument_t *clone_binary_argument(const binary_argument_t *ba)
{
  if (!ba) return NULL;
  binary_argument_t *c = xcalloc(1, sizeof *c);
  c->prim = clone_primary(ba->prim);
  c->unary_messages = clone_unary_messages(ba->unary_messages);
  c->id = ba->id;
  return c;
}

static void clone_binary_message_into(binary_message_t *dst, const binary_message_t *src)
{
  dst->binary_selector = xstrdup(src->binary_selector);
  dst->bin_arg = clone_binary_argument(src->bin_arg);
}

static binary_messages_t *clone_binary_messages(const binary_messages_t *bms)
{
  if (!bms) return NULL;
  binary_messages_t *c = xcalloc(1, sizeof *c);
  c->nof_messages = bms->nof_messages;
  if (bms->bin_msgs) {
    c->bin_msgs = xcalloc(bms->nof_messages, sizeof(binary_message_t));
    for (unsigned int i = 0; i < bms->nof_messages; i++)
      clone_binary_message_into(&c->bin_msgs[i], &bms->bin_msgs[i]);
  }
  return c;
}

keyword_argument_t *clone_keyword_argument(const keyword_argument_t *ka)
{
  if (!ka) return NULL;
  keyword_argument_t *c = xcalloc(1, sizeof *c);
  c->prim = clone_primary(ka->prim);
  c->unary_messages = clone_unary_messages(ka->unary_messages);
  c->binary_messages = clone_binary_messages(ka->binary_messages);
  c->id = ka->id;
  return c;
}

static void clone_keyword_argument_pair_into(keyword_argument_pair_t *dst,
                                             const keyword_argument_pair_t *src)
{
  dst->keyword = xstrdup(src->keyword);
  dst->kw_arg = clone_keyword_argument(src->kw_arg);
}

static keyword_message_t *clone_keyword_message(const keyword_message_t *km)
{
  if (!km) return NULL;
  keyword_message_t *c = xcalloc(1, sizeof *c);
  c->nof_args = km->nof_args;
  if (km->kw_arg_pairs) {
    c->kw_arg_pairs = xcalloc(km->nof_args, sizeof(keyword_argument_pair_t));
    for (unsigned int i = 0; i < km->nof_args; i++)
      clone_keyword_argument_pair_into(&c->kw_arg_pairs[i], &km->kw_arg_pairs[i]);
  }
  return c;
}

static void clone_message_into(message_t *dst, const message_t *src)
{
  dst->type = src->type;
  dst->unary_messages = clone_unary_messages(src->unary_messages);
  dst->binary_messages = clone_binary_messages(src->binary_messages);
  dst->kw_msg = clone_keyword_message(src->kw_msg);
}

static message_t *clone_message(const message_t *m)
{
  if (!m) return NULL;
  message_t *c = xcalloc(1, sizeof *c);
  clone_message_into(c, m);
  return c;
}

static cascaded_messages_t *clone_cascaded_messages(const cascaded_messages_t *cm)
{
  if (!cm) return NULL;
  cascaded_messages_t *c = xcalloc(1, sizeof *c);
  c->nof_cascaded_msgs = cm->nof_cascaded_msgs;
  if (cm->cascaded_msgs) {
    c->cascaded_msgs = xcalloc(cm->nof_cascaded_msgs, sizeof(message_t));
    for (unsigned int i = 0; i < cm->nof_cascaded_msgs; i++)
      clone_message_into(&c->cascaded_msgs[i], &cm->cascaded_msgs[i]);
  }
  return c;
}

/* ---- expressions / primaries / blocks (the recursive core) ---- */

static primary_t *clone_primary(const primary_t *p)
{
  if (!p) return NULL;
  primary_t *c = xcalloc(1, sizeof *c);
  c->type = p->type;
  c->identifier = xstrdup(p->identifier);
  c->lit = clone_literal(p->lit);
  c->blk_cons = clone_block_constructor(p->blk_cons);
  c->exp = clone_expression(p->exp);            /* parenthesised expression */
  return c;
}

basic_expression_t *clone_basic_expression(const basic_expression_t *be)
{
  if (!be) return NULL;
  basic_expression_t *c = xcalloc(1, sizeof *c);
  c->type = be->type;
  c->prim = clone_primary(be->prim);
  c->msg = clone_message(be->msg);
  c->cascaded_msgs = clone_cascaded_messages(be->cascaded_msgs);
  c->id = be->id;
  return c;
}

static assignment_t *clone_assignment(const assignment_t *a)
{
  if (!a) return NULL;
  assignment_t *c = xcalloc(1, sizeof *c);
  c->identifier = xstrdup(a->identifier);
  c->rvalue = clone_expression(a->rvalue);      /* a := b := c ... */
  return c;
}

static expression_t *clone_expression(const expression_t *e)
{
  if (!e) return NULL;
  expression_t *c = xcalloc(1, sizeof *c);
  c->type = e->type;
  c->asgn = clone_assignment(e->asgn);
  c->basic_exp = clone_basic_expression(e->basic_exp);
  return c;
}

static return_statement_t *clone_return_statement(const return_statement_t *rs)
{
  if (!rs) return NULL;
  return_statement_t *c = xcalloc(1, sizeof *c);
  c->exp = clone_expression(rs->exp);
  return c;
}

/* statements form a singly linked list via ->statements; clone it
   iteratively so long method bodies can't overflow the stack. */
static statement_t *clone_statements(const statement_t *s)
{
  statement_t *head = NULL;
  statement_t **tail = &head;
  for (; s; s = s->statements) {
    statement_t *c = xcalloc(1, sizeof *c);
    c->type = s->type;
    c->ret_stmt = clone_return_statement(s->ret_stmt);
    c->exp = clone_expression(s->exp);
    /* c->statements stays NULL until the next node is linked in */
    *tail = c;
    tail = &c->statements;
  }
  return head;
}

static block_constructor_t *clone_block_constructor(const block_constructor_t *bc)
{
  if (!bc) return NULL;
  block_constructor_t *c = xcalloc(1, sizeof *c);
  c->type = bc->type;
  c->docstring = xstrdup(bc->docstring);
  c->block_args = clone_block_arguments(bc->block_args);
  c->exec_code = clone_executable_code(bc->exec_code);  /* blocks nest code */
  return c;
}

/* ---- public entry point ---- */

executable_code_t *clone_executable_code(const executable_code_t *ec)
{
  if (!ec) return NULL;
  executable_code_t *c = xcalloc(1, sizeof *c);
  c->temporaries = clone_temporaries(ec->temporaries);
  c->statements = clone_statements(ec->statements);
  return c;
}
