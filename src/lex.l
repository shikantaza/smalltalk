%option noyywrap

%{

#include <stdio.h>

#include "gc.h"

#include "parser_header.h"
#include "parser.h"

#define MAX_INCLUDE_DEPTH 10

YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];

int g_include_stack_ptr = 0;

int set_up_new_yyin(FILE *fp)
{
  if (g_include_stack_ptr >= MAX_INCLUDE_DEPTH)
  {
    fprintf(stderr, "LOAD-FILEs nested too deeply");
    return 1;
  }

  include_stack[g_include_stack_ptr] = YY_CURRENT_BUFFER;

  g_include_stack_ptr++;

  yy_switch_to_buffer(yy_create_buffer(fp, YY_BUF_SIZE));

  return 0;
}

void pop_yyin()
{
  g_include_stack_ptr--;

  if(g_include_stack_ptr == 0)
  {
    yyin = stdin;
    yyrestart(yyin);
  }
  else
  {
    yy_delete_buffer(YY_CURRENT_BUFFER);
    yy_switch_to_buffer(include_stack[g_include_stack_ptr]);
  }
}

%}

/* to avoid warning related to 'yyunput' being defined but not used */
%option nounput

/* to avoid warning related to 'input' being defined but not used */
%option noinput

%%

\"[^\"]*\"   /* ignore comments */

[a-zA-Z][a-zA-Z_0-9]* {
  yylval.default_value = GC_strdup(yytext);
  return T_IDENTIFIER;
}

[a-zA-Z][a-zA-Z_0-9]*: {
  yylval.default_value = GC_strdup(yytext);
  return T_KEYWORD;
}

(!|%|&|\*|\+|,|\/|<|=|>|\?|@|\\|~|-)+ { 
  yylval.default_value = GC_strdup(yytext); 
  return T_BINARY_SELECTOR; 
}

\^ { 
  yylval.default_value = GC_strdup(yytext); 
  return T_RETURN_OPERATOR; 
}

:= { 
  yylval.default_value = GC_strdup(yytext); 
  return T_ASSIGNMENT_OPERATOR; 
}

[-]?[0-9]+|[0-9]+r[0-9A-Z]+ {
  yylval.default_value = GC_strdup(yytext);
  return T_INTEGER;
}

[-]?[0-9]+\.[0-9]+((e|d|q)[-]?[0-9]+)? {
  yylval.default_value = GC_strdup(yytext);
  return T_FLOAT;
}

([-]?[0-9]+\.[0-9]+|[-]?[0-9]+)s[0-9]+ { 
  yylval.default_value = GC_strdup(yytext); 
  return T_SCALED_DECIMAL; 
}

$. { 
  yylval.default_value = GC_strdup(yytext); 
  return T_QUOTED_CHARACTER; 
}

'[^']*\' { 
  yylval.default_value = GC_strdup(yytext); 
  return T_QUOTED_STRING; 
}

\#'[^']*\' { 
  yylval.default_value = GC_strdup(yytext); 
  return T_HASHED_STRING; 
}

\#([a-zA-Z][a-zA-Z_0-9]*|(!|%|&|\*|\+|,|\/|<|=|>|\?|@|\\|~|\||-)+|([a-zA-Z_][a-zA-Z_0-9]*:)+) {
  yylval.default_value = GC_strdup(yytext);
  return T_QUOTED_SELECTOR;
}

\| {
  return T_VERTICAL_BAR;
}

\. {
  return T_PERIOD;
}

\( {
  return T_LEFT_PAREN;
}

\) {
  return T_RIGHT_PAREN;
}

\[ {
  return T_LEFT_SQUARE_BRACKET;
}

\] {
  return T_RIGHT_SQUARE_BRACKET;
}

\: {
  return T_COLON;
}

\; {
  return T_SEMI_COLON;
}

\# {
  return T_HASH;
}

\- {
  return T_MINUS;
}

[\n\t ]+   /* ignore whitespace */

%%


/*
TODO: '|' has been temporarily removed from the rule for binary selectors
as the rule hides the rule for vertical bar. Need to figure out a way to
avoid this hiding.
The minus sign too has a similar problem.
*/

#ifdef LEX
int main()
{
  yylex();
  return 0;
}
#endif


