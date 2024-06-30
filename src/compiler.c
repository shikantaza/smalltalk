#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "gc.h"

#include "parser_header.h"
#include "smalltalk.h"
#include "util.h"

package_t *compiler_package;
package_t *smalltalk_symbols;

//forward declarations
OBJECT_PTR convert_temporaries_to_lisp(temporaries_t *);
OBJECT_PTR convert_identifier_to_atom(char *);
OBJECT_PTR convert_statements_to_lisp(statement_t *);
OBJECT_PTR convert_ret_stmt_to_lisp(return_statement_t *);
OBJECT_PTR convert_exp_to_lisp(expression_t *);
OBJECT_PTR convert_assignment_to_lisp(assignment_t *);
OBJECT_PTR convert_basic_exp_to_lisp(basic_expression_t *);
OBJECT_PTR convert_literal_to_lisp(literal_t *);
OBJECT_PTR convert_block_constructor_to_lisp(block_constructor_t *);
OBJECT_PTR convert_number_literal_to_atom(number_t *);
OBJECT_PTR convert_string_literal_to_atom(char *);
OBJECT_PTR convert_char_literal_to_atom(char *);
OBJECT_PTR convert_symbol_literal_to_atom(char *);
OBJECT_PTR convert_selector_literal_to_atom(char *);
OBJECT_PTR convert_array_literal_to_atom(array_elements_t *);
OBJECT_PTR convert_block_constructor_to_lisp(block_constructor_t *);
OBJECT_PTR convert_msg(primary_t *,
                       unary_messages_t *,
                       binary_messages_t *,
                       keyword_message_t *);
OBJECT_PTR convert_primary_to_lisp(primary_t *);
OBJECT_PTR convert_keyword_argument_to_lisp(keyword_argument_t *);
OBJECT_PTR convert_binary_selector_to_atom(char *);
OBJECT_PTR convert_binary_argument_to_lisp(binary_argument_t *);

OBJECT_PTR expand_body(OBJECT_PTR);
OBJECT_PTR assignment_conversion(OBJECT_PTR);
OBJECT_PTR translate_to_il(OBJECT_PTR);
OBJECT_PTR desugar_il(OBJECT_PTR);
OBJECT_PTR convert_int_to_object(int);
binding_env_t *create_binding_env();
OBJECT_PTR ren_transform(OBJECT_PTR, binding_env_t *);
OBJECT_PTR simplify_il(OBJECT_PTR);
OBJECT_PTR mcps_transform(OBJECT_PTR);
OBJECT_PTR closure_conv_transform(OBJECT_PTR);
OBJECT_PTR lift_transform(OBJECT_PTR);

OBJECT_PTR NIL                          =  (OBJECT_PTR)(                      SYMBOL_TAG);
OBJECT_PTR LET                          =  (OBJECT_PTR)((1 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR SET                          =  (OBJECT_PTR)((2 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR RETURN                       =  (OBJECT_PTR)((3 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR MESSAGE_SEND                 =  (OBJECT_PTR)((4 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR LAMBDA                       =  (OBJECT_PTR)((5 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR SETCAR                       =  (OBJECT_PTR)((6 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR CONS                         =  (OBJECT_PTR)((7 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR CAR                          =  (OBJECT_PTR)((8 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR LET1                         =  (OBJECT_PTR)((9 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR CASCADED                     =  (OBJECT_PTR)((10 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR SAVE_CONTINUATION            =  (OBJECT_PTR)((11 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR GET_CONTINUATION             =  (OBJECT_PTR)((12 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR SAVE_CONTINUATION_TO_RESUME  =  (OBJECT_PTR)((13 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR NTH                          =  (OBJECT_PTR)((14 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR CREATE_CLOSURE               =  (OBJECT_PTR)((15 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR EXTRACT_NATIVE_FN            =  (OBJECT_PTR)((16 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR METHOD_LOOKUP                =  (OBJECT_PTR)((17 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR SMALLTALK                    =  (OBJECT_PTR)((18 << OBJECT_SHIFT) + SYMBOL_TAG);
OBJECT_PTR SELF                         =  (OBJECT_PTR)((19 << OBJECT_SHIFT) + SYMBOL_TAG);

OBJECT_PTR TRUE                         =  (OBJECT_PTR)(                      TRUE_TAG);
OBJECT_PTR FALSE                        =  (OBJECT_PTR)(                      FALSE_TAG);

BOOLEAN IS_SYMBOL_OBJECT(OBJECT_PTR x)                   { return (x & BIT_MASK) == SYMBOL_TAG;                   }
BOOLEAN IS_CONS_OBJECT(OBJECT_PTR x)                     { return (x & BIT_MASK) == CONS_TAG;                     }
BOOLEAN IS_INTEGER_OBJECT(OBJECT_PTR x)                  { return (x & BIT_MASK) == INTEGER_TAG;                  }
BOOLEAN IS_NATIVE_FN_OBJECT(OBJECT_PTR x)                { return (x & BIT_MASK) == NATIVE_FN_TAG;                }
BOOLEAN IS_CLOSURE_OBJECT(OBJECT_PTR x)                  { return (x & BIT_MASK) == CLOSURE_TAG;                  }
BOOLEAN IS_TRUE_OBJECT(OBJECT_PTR x)                     { return (x & BIT_MASK) == TRUE_TAG;                     }
BOOLEAN IS_FALSE_OBJECT(OBJECT_PTR x)                    { return (x & BIT_MASK) == FALSE_TAG;                    }
BOOLEAN IS_CLASS_OBJECT(OBJECT_PTR x)                    { return (x & BIT_MASK) == CLASS_OBJECT_TAG;             }
BOOLEAN IS_OBJECT_OBJECT(OBJECT_PTR x)                   { return (x & BIT_MASK) == OBJECT_TAG;                   }
BOOLEAN IS_STRING_OBJECT(OBJECT_PTR x)                   { return (x & BIT_MASK) == STRING_TAG;                   }
BOOLEAN IS_SMALLTALK_SYMBOL_OBJECT(OBJECT_PTR x)         { return (x & BIT_MASK) == SMALLTALK_SYMBOL_TAG;         }

OBJECT_PTR first(OBJECT_PTR x)    { return car(x); }
OBJECT_PTR second(OBJECT_PTR x)   { return car(cdr(x)); }
OBJECT_PTR third(OBJECT_PTR x)    { return car(cdr(cdr(x))); }
OBJECT_PTR fourth(OBJECT_PTR x)   { return car(cdr(cdr(cdr(x)))); } 
OBJECT_PTR fifth(OBJECT_PTR x)    { return car(cdr(cdr(cdr(cdr(x))))); } 
OBJECT_PTR sixth(OBJECT_PTR x)    { return car(cdr(cdr(cdr(cdr(cdr(x)))))); } 
OBJECT_PTR seventh(OBJECT_PTR x)  { return car(cdr(cdr(cdr(cdr(cdr(cdr(x))))))); } 

OBJECT_PTR CADR(OBJECT_PTR x)    { return car(cdr(x)); }
OBJECT_PTR CDDR(OBJECT_PTR x)    { return cdr(cdr(x)); }
OBJECT_PTR CAAR(OBJECT_PTR x)    { return car(car(x)); }
OBJECT_PTR CDDDR(OBJECT_PTR x)   { return cdr(cdr(cdr(x))); }

int add_symbol(char *);

void initialize_top_level();
void create_Object();
void create_Smalltalk();
void create_Transcript();
void create_Integer();

int extract_symbol_index(OBJECT_PTR);

//this has also been defined in object_utils.c
//use that version (that version returns the
//package index, we ignore it)
/*
void add_symbol(char *symbol)
{
  compiler_package->nof_symbols++; 
  compiler_package->symbols = (char **)GC_MALLOC(compiler_package->nof_symbols * sizeof(char *));

  assert(compiler_package->symbols);

  compiler_package->symbols[compiler_package->nof_symbols - 1] = GC_strdup(symbol);  
}
*/

int add_smalltalk_symbol(char *sym)
{
  smalltalk_symbols->nof_symbols++;
  
  smalltalk_symbols->symbols = (char **)GC_REALLOC(smalltalk_symbols->symbols,
						   smalltalk_symbols->nof_symbols * sizeof(char *));

  smalltalk_symbols->symbols[smalltalk_symbols->nof_symbols - 1] = GC_strdup(sym);

  return smalltalk_symbols->nof_symbols - 1;
}

OBJECT_PTR get_smalltalk_symbol(char *symbol)
{
  unsigned int n = smalltalk_symbols->nof_symbols;
  unsigned int i;

  for(i=0; i<n; i++)
    if(!strcmp(smalltalk_symbols->symbols[i], symbol))
      return (OBJECT_PTR)(((OBJECT_PTR)0 << (SYMBOL_BITS + OBJECT_SHIFT)) + (i << OBJECT_SHIFT) + SMALLTALK_SYMBOL_TAG);

  add_smalltalk_symbol(symbol);
  
  return (OBJECT_PTR)(((OBJECT_PTR)0 << (SYMBOL_BITS + OBJECT_SHIFT)) + (n << OBJECT_SHIFT) + SMALLTALK_SYMBOL_TAG);
  
}

char *get_smalltalk_symbol_name(OBJECT_PTR smalltalk_symbol_object)
{
  int symbol_index;

  if(!IS_SMALLTALK_SYMBOL_OBJECT(smalltalk_symbol_object))
  {
    error("get_smalltalk_symbol_name() passed a non-smalltalk-symbol object\n");
    exit(1);
  }

  symbol_index = extract_symbol_index(smalltalk_symbol_object);
  
  return smalltalk_symbols->symbols[symbol_index];
}

OBJECT_PTR get_symbol(char *symbol)
{
  unsigned int n = compiler_package->nof_symbols;
  unsigned int i;

  for(i=0; i<n; i++)
    if(!strcmp(compiler_package->symbols[i], symbol))
      return (OBJECT_PTR)(((OBJECT_PTR)0 << (SYMBOL_BITS + OBJECT_SHIFT)) + (i << OBJECT_SHIFT) + SYMBOL_TAG);

  add_symbol(symbol);
  
  return (OBJECT_PTR)(((OBJECT_PTR)0 << (SYMBOL_BITS + OBJECT_SHIFT)) + (n << OBJECT_SHIFT) + SYMBOL_TAG);
  
}

void initialize()
{
  compiler_package = (package_t *)GC_MALLOC(sizeof(package_t));
  compiler_package->name = GC_strdup("CORE");
  compiler_package->nof_symbols = 0;

  smalltalk_symbols = (package_t *)GC_MALLOC(sizeof(package_t));
  smalltalk_symbols->name = GC_strdup("SMALLTALK");
  smalltalk_symbols->nof_symbols = 0;
  
  add_symbol("NIL");
  add_symbol("LET");
  add_symbol("SET");
  add_symbol("RETURN");
  add_symbol("MESSAGE-SEND");
  add_symbol("LAMBDA");
  add_symbol("SETCAR");
  add_symbol("CONS");
  add_symbol("CAR");
  add_symbol("LET1");
  add_symbol("CASCADED");
  add_symbol("SAVE-CONTINUATION");
  add_symbol("GET-CONTINUATION");
  add_symbol("SAVE-CONTINUATION-TO-RESUME");
  add_symbol("NTH");
  add_symbol("CREATE-CLOSURE");
  add_symbol("EXTRACT-NATIVE-FN");
  add_symbol("METHOD-LOOKUP");
  add_symbol("Smalltalk");
  add_symbol("self");

  create_Object();
  create_Smalltalk();
  create_Transcript();
  
  //this was initally after the call to
  //initialize_top_level(), but a bus error
  //was thrown in the generated code.
  //moving this here made that error go away.
  create_Integer();

  initialize_top_level();
}

void error(const char *fmt, ...)
{
  //TODO: bring in alignment with
  // error/exception handling framework
  va_list arglist;
  va_start(arglist, fmt);
  vprintf(fmt, arglist);
  va_end(arglist);
}

OBJECT_PTR remove_cascaded_tag(OBJECT_PTR lst)
{
  OBJECT_PTR ret = NIL;
  OBJECT_PTR rest = lst;

  while(rest != NIL)
  {
    if(car(rest) != CASCADED)
       ret = cons(car(rest), ret);
    rest = cdr(rest);
  }

  return reverse(ret);
}

OBJECT_PTR convert_exec_code_to_lisp(executable_code_t *e)
{
  if(!e)
    return NIL;

  return concat(2,
                list(2,
                     LET,
                     convert_temporaries_to_lisp(e->temporaries)),
                remove_cascaded_tag(convert_statements_to_lisp(e->statements)));
}

OBJECT_PTR convert_temporaries_to_lisp(temporaries_t *t)
{
  if(!t)
    return NIL;

  unsigned int i;

  OBJECT_PTR res = NIL;

  for(i=0; i < t->nof_temporaries; i++)
    res = cons(list(1, convert_identifier_to_atom(t->temporaries[i])), res);

  return reverse(res);
}

OBJECT_PTR convert_statements_to_lisp(statement_t *s)
{
  if(!s)
    return NIL;

  if(s->type == RETURN_STATEMENT)
    return list(1, convert_ret_stmt_to_lisp(s->ret_stmt));
  else if(s->type == EXPRESSION)
  {
    OBJECT_PTR exp = convert_exp_to_lisp(s->exp);

    if(IS_CONS_OBJECT(exp))
    {   
      if(car(exp) == CASCADED)
        return exp;
      else
        return list(1, convert_exp_to_lisp(s->exp));
    }
    else
      return list(1, convert_exp_to_lisp(s->exp));
  }
  else if(s->type == EXP_PLUS_STATEMENTS)
  {
    OBJECT_PTR exp = convert_exp_to_lisp(s->exp);
    OBJECT_PTR stmts = convert_statements_to_lisp(s->statements);

    if(IS_CONS_OBJECT(exp))
    {
      if(car(exp) == CASCADED)
        return concat(2, exp, stmts);
      else
        return concat(2, list(1,exp), stmts);
    }
    else
      return concat(2, list(1,exp), stmts);
  }
  else
  {
    error("Unknown statement type: %d\n", s->type);
    return NIL;
  }
}

OBJECT_PTR convert_ret_stmt_to_lisp(return_statement_t *r)
{
  if(!r)
    return cons(RETURN, NIL);

  return list(2, RETURN, convert_exp_to_lisp(r->exp));
}

OBJECT_PTR convert_exp_to_lisp(expression_t *e)
{
  if(!e)
    return NIL;

  if(e->type == ASSIGNMENT)
    return convert_assignment_to_lisp(e->asgn);
  else if(e->type == BASIC_EXPRESSION)
    return convert_basic_exp_to_lisp(e->basic_exp);
  else
  {
    error("Unknown expression type: %d\n", e->type);
    return NIL;
  }
}

OBJECT_PTR convert_assignment_to_lisp(assignment_t *a)
{
  if(!a)
    return NIL;

  return list(3,
              SET,
              convert_identifier_to_atom(a->identifier),
              convert_exp_to_lisp(a->rvalue));
}

OBJECT_PTR convert_basic_exp_to_lisp(basic_expression_t *be)
{
  if(!be)
    return NIL;

  if(be->type == PRIMARY)
    return convert_primary_to_lisp(be->prim);
  else if(be->type == PRIMARY_PLUS_MESSAGES)
  {
    OBJECT_PTR res = NIL;

    if(be->msg)
    {
      if(be->msg->type == UNARY_MESSAGE)
      {
        assert(be->msg->unary_messages);

        res = convert_msg(be->prim,
                          be->msg->unary_messages,
                          be->msg->binary_messages,
                          be->msg->kw_msg);
        
      }
      else if(be->msg->type == BINARY_MESSAGE)
      {
        assert(be->msg->binary_messages);

        res = convert_msg(be->prim,
                          NULL,
                          be->msg->binary_messages,
                          be->msg->kw_msg);
      }
      else if(be->msg->type == KEYWORD_MESSAGE)
      {
        res = convert_msg(be->prim,
                          NULL,
                          NULL,
                          be->msg->kw_msg);
      }
      else
      {
        error("Unknown message type: %d\n", be->msg->type);
        return NIL;
      }
    }
    
    cascaded_messages_t *cm = be->cascaded_msgs;
    
    if(cm)
    {
      if(cm->nof_cascaded_msgs > 0)
      {
        OBJECT_PTR res1 = NIL;
        unsigned int i;
        unsigned int nof_msgs = cm->nof_cascaded_msgs;
        for(i=0; i< nof_msgs; i++)
          res1 = cons(convert_msg(be->prim,
                                  cm->cascaded_msgs[i].unary_messages,
                                  cm->cascaded_msgs[i].binary_messages,
                                  cm->cascaded_msgs[i].kw_msg),
                      res1);

        //res = cons(CASCADED, cons(res, reverse(res1)));
        res = cons(CASCADED, concat(2, list(1, res), reverse(res1)));
      }
    }
    
    return res;
  }
  else
  {
    error("Unknown basic expression type: %d\n", be->type);
    return NIL;
  }
}

OBJECT_PTR convert_primary_to_lisp(primary_t *p)
{
  if(!p)
    return NIL;

  if(p->type == IDENTIFIER)
    return convert_identifier_to_atom(p->identifier);
  else if(p->type == LITERAL)
    return convert_literal_to_lisp(p->lit);
  else if(p->type == BLOCK_CONSTRUCTOR)
    return convert_block_constructor_to_lisp(p->blk_cons);
  else if(p->type == EXPRESSION1)
    return convert_exp_to_lisp(p->exp);
  else
  {
    error("Unknown primary type: %d\n", p->type);
    return NIL;
  }
}

OBJECT_PTR convert_literal_to_lisp(literal_t *l)
{
  if(!l)
    return NIL;

  if(l->type == NUMBER_LITERAL)
    return convert_number_literal_to_atom(l->num);
  else if(l->type == STRING_LITERAL)
    return convert_string_literal_to_atom(l->val);
  else if(l->type == CHAR_LITERAL)
    return convert_char_literal_to_atom(l->val);
  else if(l->type == SYMBOL_LITERAL)
    return convert_symbol_literal_to_atom(l->val);
  else if(l->type == SELECTOR_LITERAL)
    return convert_selector_literal_to_atom(l->val);
  else if(l->type == ARRAY_LITERAL)
    return convert_array_literal_to_atom(l->array_elements);
  else
  {
    error("Unknown literal type: %d\n", l->type);
    return NIL;
  }
}

OBJECT_PTR convert_block_constructor_to_lisp(block_constructor_t *b)
{
  if(!b)
    return NIL;

  OBJECT_PTR res = list(1, LAMBDA);
  
  if(b->type == BLOCK_ARGS)
  {
    block_arguments_t *args = b->block_args;

    OBJECT_PTR res1 = NIL;
    
    if(args)
    {
      unsigned int nof_args = args->nof_args;
      if(nof_args > 0)
      {
        //res1 = NIL;
        unsigned int i;
        for(i=0; i < nof_args; i++)
        {
          res1 = cons(convert_identifier_to_atom(args->identifiers[i]),
                      res1);
        }
      }
    }
    res = concat(2, res, list(1, reverse(res1)));
  }
  else if(b->type == NO_BLOCK_ARGS)
    res = concat(2, res, list(1,NIL));
  else
  {
    error("Unknown block constructor type: %d\n", b->type);
    return NIL;
  }

  return concat(2, res, list(1, convert_exec_code_to_lisp(b->exec_code)));
}

OBJECT_PTR convert_msg(primary_t *prim,
                       unary_messages_t *unary_messages,
                       binary_messages_t *binary_messages,
                       keyword_message_t *kw_msg)
{
  assert(prim);
  
  OBJECT_PTR res;
    
  res = convert_primary_to_lisp(prim);

  unsigned int i;
  unsigned int nof_msgs;

  if(unary_messages)
  {  
    nof_msgs = unary_messages->nof_messages;

    for(i=0; i < nof_msgs; i++)
      res = list(4,
                 MESSAGE_SEND,
                 res,
                 convert_identifier_to_atom(unary_messages->identifiers[i]),
		 convert_int_to_object(0));
  }
  
  if(binary_messages)
  {
    nof_msgs = binary_messages->nof_messages;

    for(i=0; i < nof_msgs; i++)
      res = list(5,
                 MESSAGE_SEND,
                 res,
                 convert_binary_selector_to_atom(binary_messages->bin_msgs[i].binary_selector),
		 convert_int_to_object(1),
                 convert_binary_argument_to_lisp(binary_messages->bin_msgs[i].bin_arg));
  }

  if(kw_msg)
  {
    unsigned int i;
    unsigned nof_arg_pairs = kw_msg->nof_args;

    unsigned int total_len = 0;
    char *combined_keyword;
          
    for(i=0; i<nof_arg_pairs; i++)
      total_len += strlen(kw_msg->kw_arg_pairs[i].keyword);

    combined_keyword = (char *)GC_MALLOC((total_len+1)*sizeof(char));
    memset(combined_keyword, '\0', total_len + 1);

    unsigned int len = 0;

    for(i=0; i<nof_arg_pairs; i++)
      len += sprintf(combined_keyword+len, "%s", kw_msg->kw_arg_pairs[i].keyword);

    res = list(3,
               MESSAGE_SEND,
               res,
               convert_identifier_to_atom(combined_keyword));

    OBJECT_PTR res1 = NIL;
          
    for(i=0; i<nof_arg_pairs; i++)
      res1 = cons(convert_keyword_argument_to_lisp(kw_msg->kw_arg_pairs[i].kw_arg),
                  res1);

    res = concat(2, res, concat(2, list(1, convert_int_to_object(cons_length(res1))), reverse(res1)));
  }

  return res;
}

OBJECT_PTR convert_keyword_argument_to_lisp(keyword_argument_t *k)
{
  if(!k)
    return NIL;

  return convert_msg(k->prim,
                     k->unary_messages,
                     k->binary_messages,
                     NULL); //why no keyword messages within a keyword argument?
}
    
OBJECT_PTR convert_number_literal_to_atom(number_t *n)
{
  if(!n)
    return NIL;

  if(n->type == INTEGER)
  {
    //TODO : check that the value is within the permitted
    //range for Integer?

    /*
    int i = atoi(n->val);
    long l = i; // i may not be necessary

    uintptr_t ptr;

    ptr = (l << 32) + INTEGER_TAG;

    return ptr;
    */
    return convert_int_to_object(atoi(n->val));
  }
  else
  {
    error("Not yet implemented\n");
    return NIL;
  }
}

OBJECT_PTR convert_string_literal_to_atom(char *s)
{
  error("Not implemented yet\n");
  return NIL;
  
}

OBJECT_PTR convert_char_literal_to_atom(char *s)
{
  error("Not implemented yet\n");
  return NIL;
}

OBJECT_PTR convert_symbol_literal_to_atom(char *s)
{
  error("Not implemented yet\n");
  return NIL;
}

OBJECT_PTR convert_selector_literal_to_atom(char *s)
{
  char *stripped_selector = substring(s, 1, strlen(s)-1);

  //TODO: need to unify pLisp symbol objects and
  //Smalltalk symbol objects
  return get_smalltalk_symbol(stripped_selector);
}

OBJECT_PTR convert_array_literal_to_atom(array_elements_t *e)
{
  error("Not implemented yet\n");
  return NIL;
}

OBJECT_PTR convert_identifier_to_atom(char *s)
{
  unsigned int i;

  for(i=0; i < compiler_package->nof_symbols; i++)
  {
    if(!strcmp(s, compiler_package->symbols[i]))
       return (OBJECT_PTR)(((OBJECT_PTR)0 << (SYMBOL_BITS + OBJECT_SHIFT)) + (i << OBJECT_SHIFT) + SYMBOL_TAG);
  }

  compiler_package->nof_symbols++;
  compiler_package->symbols = (char **)GC_REALLOC(compiler_package->symbols,
                                                  compiler_package->nof_symbols * sizeof(char *));
  compiler_package->symbols[compiler_package->nof_symbols - 1] = GC_strdup(s);

  unsigned int n = compiler_package->nof_symbols;

  return (OBJECT_PTR)(((OBJECT_PTR)0 << (SYMBOL_BITS + OBJECT_SHIFT)) + ((n-1) << OBJECT_SHIFT) + SYMBOL_TAG);
}

OBJECT_PTR convert_binary_selector_to_atom(char *s)
{
  return convert_identifier_to_atom(s);
}

OBJECT_PTR convert_binary_argument_to_lisp(binary_argument_t *arg)
{
  if(!arg)
    return NIL;

  return convert_msg(arg->prim,
                     arg->unary_messages,
                     NULL,
                     NULL);
}




