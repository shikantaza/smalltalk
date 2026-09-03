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

%option noyywrap

%{

#include <stdio.h>

#include "gc.h"

#include "smalltalk.h"

#include "parser_header.h"
#include "parser.h"

#define MAX_INCLUDE_DEPTH 10

YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];

int g_include_stack_ptr = 0;

BOOLEAN expecting_docstring = 0;

int repl2();

extern OBJECT_PTR NIL;
extern OBJECT_PTR g_last_eval_result;

extern int g_open_square_brackets;

extern char *g_method_code;

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

int call_repl(char *expression)
{
  g_last_eval_result = NIL;
  g_open_square_brackets = 0;
  YY_BUFFER_STATE buf = yy_scan_string(expression);

  if(!yyparse())
  {
    g_method_code = GC_strdup(expression);
    repl2();
    yy_delete_buffer(buf);
    return 0;
  }
  else
  {
    yy_delete_buffer(buf);
    return 1;
  }
}

int call_repl_orig(char *expression)
{
  YY_BUFFER_STATE buf = yy_scan_string(expression);

  int ret;

  while((ret = yyparse()) == 0)
  {
    if(repl2())
    {
      yy_delete_buffer(buf);
      return -1;
    }
  }

  yy_delete_buffer(buf);

  if(ret == 1)
    return -1;

  return 0;
}

char *remove_backslash(char *s)
{
  unsigned int i, j, n;

  n = strlen(s);

  char *ret = (char *)GC_MALLOC((n+1) * sizeof(char));

  i=0, j=0;

  while(i<n-1)
  {
    if(s[i] == '\\' && s[i+1] == '\'')
    {
      ret[j] = s[i+1];
      i += 2;
    }
    else
    {
      ret[j] = s[i];
      i++;
    }
    j++;
  }

  ret[j] = s[i];
  ret[j+1] = '\0';

  return ret;
}
%}

/* to avoid warning related to 'yyunput' being defined but not used */
%option nounput

/* to avoid warning related to 'input' being defined but not used */
%option noinput


%%

[\n\t ]+ { }   /* ignore whitespace */

\"[^\"]*\" {
  if (expecting_docstring)
  {
    expecting_docstring = 0; // Reset
    yylval.default_value = GC_strdup(yytext);
    return T_DOCSTRING;
  }
  /* Default: Ignore normal block comments */
}

[a-zA-Z][a-zA-Z_0-9]* {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext);
  return T_IDENTIFIER;
}

[a-zA-Z][a-zA-Z_0-9]*: {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext);
  return T_KEYWORD;
}

("-"|"!"|"%"|"&"|"*"|"+"|","|"/"|"<"|"="|">"|"?"|"@"|"\\"|"~")+ {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext); 
  return T_BINARY_SELECTOR; 
}

\^ {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext); 
  return T_RETURN_OPERATOR; 
}

:= {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext); 
  return T_ASSIGNMENT_OPERATOR; 
}

[-]?[0-9]+|[0-9]+r[0-9A-Z]+ {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext);
  return T_INTEGER;
}

[-]?[0-9]+\.[0-9]+((e|d|q)[-]?[0-9]+)? {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext);
  return T_FLOAT;
}

([-]?[0-9]+\.[0-9]+|[-]?[0-9]+)s[0-9]+ {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext); 
  return T_SCALED_DECIMAL; 
}

$. {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext); 
  return T_QUOTED_CHARACTER; 
}

\'([^\'\\]|\\.)*\' {
  expecting_docstring = 0;
  yylval.default_value = remove_backslash(yytext);
  return T_QUOTED_STRING;
}

\#\'([^\'\\]|\\.)*\' {
  expecting_docstring = 0;
  yylval.default_value = remove_backslash(yytext);
  return T_HASHED_STRING; 
}

\#([a-zA-Z][a-zA-Z_0-9]*|(!|%|&|\*|\+|,|\/|<|=|>|\?|@|\\|~|\||-)+|([a-zA-Z_][a-zA-Z_0-9]*:)+) {
  expecting_docstring = 0;
  yylval.default_value = GC_strdup(yytext);
  return T_QUOTED_SELECTOR;
}

\| {
  expecting_docstring = 0;
  return T_VERTICAL_BAR;
}

\. {
  expecting_docstring = 0;
  return T_PERIOD;
}

\( {
  expecting_docstring = 0;
  return T_LEFT_PAREN;
}

\) {
  expecting_docstring = 0;
  return T_RIGHT_PAREN;
}

\[ {
  expecting_docstring = 1;
  return T_LEFT_SQUARE_BRACKET;
}

\] {
  expecting_docstring = 0;
  return T_RIGHT_SQUARE_BRACKET;
}

\: {
  expecting_docstring = 0;
  return T_COLON;
}

\; {
  expecting_docstring = 0;
  return T_SEMI_COLON;
}

\# {
  expecting_docstring = 0;
  return T_HASH;
}

\- {
  expecting_docstring = 0;
  return T_MINUS;
}

. {
  expecting_docstring = 0;
  return YYUNDEF; /* report an error for anything else */
}

%%

#ifdef LEX
int main()
{
  yylex();
  return 0;
}
#endif
