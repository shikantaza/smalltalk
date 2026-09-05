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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <zip.h>

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
  size_t n;

  n = strlen(s);

  char *ret = (char *)GC_MALLOC((n+2) * sizeof(char));

  strncpy(ret,s,n);

  ret[n] = a;
  ret[n+1] = '\0';

  return ret;
}

char *prepend_char(char *s, char a)
{
  size_t n;

  n = strlen(s);

  char *ret = (char *)GC_MALLOC((n+2) * sizeof(char));

  strncpy(ret+1,s,n);

  ret[0] = a;
  ret[n+1] = '\0';

  return ret;
}

//http://stackoverflow.com/questions/3747086/reading-the-whole-text-file-into-a-char-array-in-c
char *get_file_contents(char *file_name)
{
  FILE *fp;
  long lSize;
  char *buffer;

  fp = fopen(file_name, "r" );
  if(!fp)
    return NULL;

  fseek(fp, 0L, SEEK_END);
  lSize = ftell(fp);

  //to handle zero-byte files
  if(!lSize)
  {
    fclose(fp);
    return (char *)-1;
  }

  rewind(fp);

  /* allocate memory for entire content */
  //buffer = calloc(1, lSize+1);
  buffer = GC_MALLOC(lSize+1);

  if(!buffer)
  {
    fclose(fp);
    return NULL;
  }

  /* copy the file into the buffer */
  size_t bytes_read = fread(buffer, lSize, 1, fp);
  if(bytes_read != 1)
  {
    fclose(fp);
    //free(buffer);
    return NULL;
  }

  fclose(fp);

  return(buffer);
}

unsigned int file_exists(char *fname)
{
  return access(fname, F_OK) != -1;
}

char *append_string(char *str1, char *str2)
{
  if(!str1)
    return GC_strdup(str2);

  if(!str2)
    return str1;

  unsigned int len1 = strlen(str1);
  unsigned int len2 = strlen(str2);

  int i;

  char *temp = (char *)GC_REALLOC(str1, (len1+len2+1)*sizeof(char));

  str1 = temp;

  for(i=0; i<len2; i++)
    str1[len1+i] = str2[i];

  str1[len1+len2] = '\0';

  return str1;
}

char *reverse_string(char *str)
{
  int i, len;

  len = strlen(str);

  char *ret = GC_MALLOC((len+1) * sizeof(char));
  memset(ret, '\0', len+1);

  for(i=0; i<len; i++)
    ret[i] = str[len-i-1];

  return ret;
}

//replaces occurrences of "\n" in str
//with actual newlines. used to process
//JSON strings from serialization
char *replace_newlines(char *str)
{
  int i=0, len;
  int nof_newlines = 0, nof_double_quotes = 0;
  char *ret = NULL;

  len = strlen(str);

  //note the '<=', as we want to scan
  //only till the penultimate character
  for(i=0; i<=len; i++)
  {
    if(str[i] == '\\' && str[i+1] == 'n')
      nof_newlines++;
    if(str[i] == '\\' && str[i+1] == '"')
      nof_double_quotes++;
  }

  int len_new_string = len - nof_newlines - nof_double_quotes;

  ret = (char *)GC_MALLOC((len_new_string + 1) * sizeof(char));

  int j=0;

  i=0;

  while(i<=(len-1))
  {
    if(str[i] == '\\' && str[i+1] == 'n')
    {
      ret[j] = '\n';
      i += 2;
    }
    else if(str[i] == '\\' && str[i+1] == '"')
    {
      ret[j] = '"';
      i += 2;
    }
    else
    {
      ret[j] = str[i];
      i++;
    }

    j++;
  }

  assert(j==len_new_string);

  ret[j+1] = '\0';

  return ret;
}

int create_image_zip_file(const char *archive_name, const char *json_file_name, char *err_msg)
{
    int errorp = 0;

    zip_t *archive = zip_open(archive_name, ZIP_CREATE | ZIP_TRUNCATE, &errorp);
    if (!archive)
    {
        zip_error_t error;
        zip_error_init_with_code(&error, errorp);
        sprintf(err_msg, "Failed to open archive: %s\n", zip_error_strerror(&error));
        zip_error_fini(&error);
        return EXIT_FAILURE;
    }

    zip_source_t *source_file = zip_source_file(archive, json_file_name, 0, -1);
    if (!source_file)
    {
        sprintf(err_msg, "Failed to create file source: %s\n", zip_strerror(archive));
    }
    else
    {
        if (zip_file_add(archive, json_file_name, source_file, ZIP_FL_ENC_UTF_8) < 0)
	{
            sprintf(err_msg, "Failed to add disk file: %s\n", zip_strerror(archive));
            zip_source_free(source_file);
        }
    }

    if (zip_close(archive) < 0)
    {
      sprintf(err_msg, "Failed to write and close archive: %s\n", zip_strerror(archive));
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#define BUFFER_SIZE 8192 // 8KB chunks for efficient copying

int extract_json_from_image_zip_file(const char *archive_name, char *json_file_name, char *err_msg)
{
    int errorp = 0;

    zip_t *archive = zip_open(archive_name, 0, &errorp);
    if (!archive) {
        sprintf(err_msg, "Error opening archive (code %d)\n", errorp);
        return EXIT_FAILURE;
    }

    zip_int64_t num_entries = zip_get_num_entries(archive, 0);

    assert(num_entries == 1);

    struct zip_stat st;
    zip_stat_init(&st);

    assert(zip_stat_index(archive, 0, 0, &st) == 0);

    zip_file_t *zip_file = zip_fopen_index(archive, 0, 0);
    if(!zip_file)
    {
      sprintf(err_msg, "Could not open %s inside ZIP\n", st.name);
      return EXIT_FAILURE;
    }

    sprintf(json_file_name, "%s", st.name);

    FILE *dest_file = fopen(st.name, "wb");
    if(!dest_file)
    {
      sprintf(err_msg, "Could not create destination file %s\n", st.name);
      zip_fclose(zip_file);
      return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    zip_int64_t bytes_read;
    int write_error = 0;

    while ((bytes_read = zip_fread(zip_file, buffer, sizeof(buffer))) > 0)
    {
      size_t bytes_written = fwrite(buffer, 1, bytes_read, dest_file);
      if (bytes_written < (size_t)bytes_read) {
	sprintf(err_msg, "Write error occurred while writing %s\n", st.name);
	write_error = 1;
	break;
      }
    }

    if (bytes_read < 0) {
      sprintf(err_msg, "Read error occurred while extracting %s\n", st.name);
    }

    fclose(dest_file);
    zip_fclose(zip_file);

    zip_close(archive);
    return EXIT_SUCCESS;
}

//replaces newlines with a literal \ and n
//so that the JSON string is valid
char *replace_newlines_for_serialization(char *str)
{
  unsigned int i, j, len;
  int nof_newlines = 0;
  len = strlen(str);

  char *ret = NULL;

  for(i=0; i<=len; i++)
  {
    if(str[i] == '\n')
      nof_newlines++;
  }

  int len_new_string = len + (2 * nof_newlines);

  ret = (char *)GC_MALLOC((len_new_string + 1) * sizeof(char));

  j=0;
  i=0;

  while(i<len)
  {
    if(str[i] == '"')
    {
      ret[j] = '\\';
      ret[j+1] = '"';
      j += 2;
    }
    else if(str[i] == '\n')
    {
      ret[j] = '\\';
      ret[j+1] = 'n';
      j += 2;
    }
    else
    {
      ret[j] = str[i];
      j++;
    }

    i++;
  }

  ret[j] = '\0';
  printf("%s\n", ret);
  return ret;
}
