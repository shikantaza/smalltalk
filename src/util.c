#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "gc.h"

#include "global_decls.h"

char *get_symbol_name(OBJECT_PTR);

char *convert_to_lower_case(char *str)
{
  char *ptr = NULL;

  for(ptr=str;*ptr;ptr++) 
  { 
    *ptr=tolower(*ptr); 
  } 

  return str;
}

char convert_special_char(char c)
{
  switch(c)
  {
    case '\\':
      return '0';
    case '^':
      return '1';
    case '$':
      return '2';
    case '.':
      return '3';
    case '|':
      return '4';
    case '?':
      return '5';
    case '*':
      return '6';
    case '+':
      return '7';
    case '{':
      return '8';
    case '!':
      return '9';
    case '_':
      return 'a';
    case '-':
      return 'b';
    case '/':
      return 'c';
    case '<':
      return 'd';
    case '=':
      return 'e';
    case '>':
      return 'f';
    case '#':
      return 'g';
    case '%':
      return 'h';
    case '&':
      return 'i';
    case '}':
      return 'j';
    case '~':
      return 'k';
    case ':':
      return 'l';
    default:
      return c;
  }
}

#define MAX_IDENTIFIER_LENGTH 100

char *convert_identifier(char *id)
{
  int i;

  int len = strlen(id);

  if(len > MAX_IDENTIFIER_LENGTH)
  {
    printf("Max identifier length exceeded\n");
    return NULL;
  }

  char *s = (char *)GC_MALLOC((MAX_IDENTIFIER_LENGTH + 1) * sizeof(char));

  memset(s, '\0', MAX_IDENTIFIER_LENGTH + 1);

  s[0] = '_';

  for(i=0; i<len; i++)
    s[i+1] = convert_special_char(id[i]);

  s[i+1] = '\0';

  return s;
}

char *replace_hyphens(char *s)
{
  int i, len = strlen(s);
  
  for(i=0; i<len; i++)
    if(s[i] == '-')
      s[i] = '_';

  return s;
}

BOOLEAN is_gensym(OBJECT_PTR sym)
{
  assert(IS_SYMBOL_OBJECT(sym));

  char *s = get_symbol_name(sym);

  return s[0] == '#' && s[1] == ':';
}

void sort(char *s)
{
  size_t len = strlen(s);
  size_t i, last;

  char temp;

  for(last=len; last>=1; last--)
  {
    size_t max_index = 0;
    for(i=0; i<last; i++)
      if(s[i] > s[max_index])
        max_index = i;

    temp = s[last-1];
    s[last-1] = s[max_index];
    s[max_index] = temp;
  }
}

// uncomment this later when we get to implementing
// the class library
/*
void sort_with_block(char *s, OBJECT_PTR sort_block)
{
  size_t len = strlen(s);
  size_t i, last;

  char temp;

  for(last=len; last>=1; last--)
  {
    size_t max_index = 0;
    for(i=0; i<last; i++)
      if(message_send(sort_block,
                      selector("value:value"),
                      convert_char_to_object(s[i]),
                      convert_char_to_object(s[max_index])) == TRUE)
        max_index = i;

    temp = s[last-1];
    s[last-1] = s[max_index];
    s[max_index] = temp;
  }
}
*/

char *substring(const char* str, size_t begin, size_t len) 
{ 
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len)) 
    return 0; 

  return GC_strndup(str + begin, len); 
}

//strips the last colon from keyword messages selectors
//that are meant for single arguments, to do a successful
//message lookup
char *strip_last_colon(char *s)
{
  unsigned int i, n;
  n = strlen(s);

  if(s[n-1] != ':')
    return s;
  
  unsigned int count = 0;
  
  for(i=0; i<n; i++)
    if(s[i] == ':')
      count++;

  if(count == 1) //there is only a colon at the end
    return substring(s, 0, n-1);
  else
    return s;
}

//appends a character to a string. used
//for decorating message selectors
//(decorate_message_selectors())
char *append_char(char *s, char a)
{
  unsigned int n;

  n = strlen(s);

  char *ret = (char *)GC_MALLOC((n+2) * sizeof(char));

  strcpy(ret, s);

  ret[n] = a;
  ret[n+1] = '\0';

  return ret;
}
