%option noyywrap

%{

#include <stdio.h>

#include "gc.h"

#include "parser_header.h"
#include "parser.h"

%}

/* to avoid warning related to 'yyunput' being defined but not used */
%option nounput

/* to avoid warning related to 'input' being defined but not used */
%option noinput

%%

\"[^\"]*\" {
  yylval.default_value = GC_strdup(yytext);
  return T_COMMENT;
}

[a-zA-Z_][a-zA-Z_0-9]* {
  yylval.default_value = GC_strdup(yytext);
  return T_IDENTIFIER;
}

[a-zA-Z_][a-zA-Z_0-9]*: {
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

\#([a-zA-Z_][a-zA-Z_0-9]*|(!|%|&|\*|\+|,|\/|<|=|>|\?|@|\\|~|\||-)+|([a-zA-Z_][a-zA-Z_0-9]*:)+) {
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


