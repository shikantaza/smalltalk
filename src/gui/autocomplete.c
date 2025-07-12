#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gtksourceview/gtksource.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

#include <assert.h>

#include "gc.h"

#include "../global_decls.h"
#include "../util.h"

#define FONT "DejaVu Sans Mono Bold 11"

void setup_language_manager_path(GtkSourceLanguageManager *);

char **autocomplete_words = NULL;
unsigned int nof_autocomplete_words = 0;

GtkSourceBuffer *keywords_buffer = NULL;

extern GtkSourceLanguageManager *lm;
extern GtkSourceLanguage *source_language;
extern GtkSourceCompletionProvider *provider;

extern GtkSourceView *workspace_source_view;
extern GtkSourceBuffer *workspace_source_buffer;

extern GtkSourceView *class_browser_source_view;
extern GtkSourceBuffer *class_browser_source_buffer;

extern GtkTextView *curr_file_browser_text_view;
extern GtkTextBuffer *curr_file_browser_buffer;

BOOLEAN word_already_exists(char *word)
{
  int i;

  for(i=0; i<nof_autocomplete_words; i++)
  {
    if(!strcmp(word, autocomplete_words[i]))
      return true; 
  }
  return false;
}

void add_to_autocomplete_list_single(char *word)
{
  if(word_already_exists(word) || strlen(word) <= 2)
    return;
  
  nof_autocomplete_words++;

  char **temp = GC_REALLOC(autocomplete_words, nof_autocomplete_words * sizeof(char *));
  assert(temp);
  autocomplete_words = temp;

  autocomplete_words[nof_autocomplete_words-1] = word;

  //also add the word to keywords_buffer
  GtkTextMark *mark = gtk_text_buffer_get_insert((GtkTextBuffer *)keywords_buffer);
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter((GtkTextBuffer *)keywords_buffer, &iter );
  gtk_text_buffer_move_mark((GtkTextBuffer *)keywords_buffer, mark, &iter );

  gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)keywords_buffer, word, -1);
  gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)keywords_buffer, "\n", -1);
  //
}

void add_to_autocomplete_list(char *word)
{
  int i, len = strlen(word);
  int colon_idx = len;
  
  for(i=0; i<len; i++)
  {
    if(word[i] == ':')
    {
      colon_idx = i;
      break;
    }
  }

  if(colon_idx == len) //i.e., no colon in the word
    add_to_autocomplete_list_single(word);
  else
  {
    //add the part before the colon
    add_to_autocomplete_list_single(substring(word, 0, colon_idx + 1));

    //recurse for the part after the colon
    add_to_autocomplete_list(substring(word, colon_idx + 1, len - colon_idx - 1));
  }
}

char *get_common_prefix(unsigned int count, char **word_list)
{
  if(count == 0)
    return NULL;

  int idx = 0;

  int i;

  BOOLEAN done = false;

  while(!done)
  {
    for(i=0; i<count; i++)
    {
      if(!word_list[i][idx])
      {
        done = true;
        break;
      }
    }

    if(!done)
    {
      for(i=0; i<count-1; i++)
      {
        if(word_list[i][idx] != word_list[i+1][idx])
        {
          done = true;
          break;
        }
      }
      if(!done)
        idx++;
    }
  }

  if(idx)
    return GC_strndup(word_list[0], idx);
  else
    return NULL;

}

enum {FIRST, LAST};

BOOLEAN is_non_identifier_char(char c)
{
  return c != '-' && c != '+' && c != '*' && c != '/' &&
         c != '<' && c != '>' && c != '=' && c != '\'' &&
         c != ',' && c != '`' && c != '@' &&
         !(c >= 65 && c <= 90) &&
         !(c >= 97 && c <= 122) &&
         !(c >= 48 && c <= 57);
}

//returns the index of the first or last non-identifier
//character (non-alphanumeric, non-hyphen) in the string
//returns -1 if such a character doesn't exist in the string
unsigned int get_loc_of_non_identifier_character(char *s, int pos)
{
  int i;
  int len = strlen(s);

  if(pos == FIRST)
  {
    for(i=0; i<len; i++)
      if(is_non_identifier_char(s[i]))
        return i;
  }
  else if(pos == LAST)
  {
    for(i=len-1; i>=0; i--)
      if(is_non_identifier_char(s[i]))
        return i;
  }
  return -1;
}

//returns the word under the cursor (for autocomplete)
char *get_current_word(GtkTextBuffer *buffer)
{
  GtkTextIter curr_iter, line_start_iter, line_end_iter;
  gint line_number, line_count;

  //get the current iter
  gtk_text_buffer_get_iter_at_mark(buffer, &curr_iter, gtk_text_buffer_get_insert(buffer));  

  //get the current line
  line_number = gtk_text_iter_get_line(&curr_iter);

  //get the total number of lines in the buffer
  line_count = gtk_text_buffer_get_line_count(buffer);

  //get the iter at the beginning of the current line
  gtk_text_buffer_get_iter_at_line(buffer, &line_start_iter, line_number);

  //get the iter at the end of the current line
  if(line_number == (line_count-1))
    gtk_text_buffer_get_end_iter(buffer, &line_end_iter);
  else
    gtk_text_buffer_get_iter_at_line(buffer, &line_end_iter, line_number+1);

  gchar *str1, *str2;

  //get the strings before and after the cursor
  str1 = gtk_text_buffer_get_text(buffer, &line_start_iter, &curr_iter, FALSE);
  str2 = gtk_text_buffer_get_text(buffer, &curr_iter, &line_end_iter, FALSE);

  unsigned int idx1 = get_loc_of_non_identifier_character(str1, LAST);
  unsigned int idx2 = get_loc_of_non_identifier_character(str2, FIRST);

  char *left, *right;

  if(idx1 == -1)
    left = GC_strdup(str1);
  else
    left = GC_strndup(str1+idx1+1, strlen(str1)-idx1-1);

  if(idx2 == -1)
    right = GC_strdup(str2);
  else
    right = GC_strndup(str2, idx2);

  char *ret = (char *)GC_MALLOC((strlen(left) + strlen(right) + 1) * sizeof(char));
  assert(ret);

  memset(ret, '\0', strlen(left) + strlen(right) + 1);

  sprintf(ret,"%s%s", left, right);

  //free(left);
  //free(right);

  return ret;
}

void do_auto_complete(GtkTextBuffer *buffer)
{
  char *s = get_current_word(buffer);

  if(strlen(s) == 0)
    return;

  //don't do autocomplete if cursor is in the middle of a word
  //begin
  GtkTextIter curr_iter1;

  gtk_text_buffer_get_iter_at_mark(buffer, &curr_iter1, gtk_text_buffer_get_insert(buffer));
  
  gunichar ret = gtk_text_iter_get_char(&curr_iter1);

  //next character not space, newline, or end of buffer
  if(!g_unichar_isspace(ret) && ret)
    return;
  //end
  
  int i;
  unsigned int nof_matches = 0;
  unsigned int len = strlen(s);

  char **matches = NULL;

  for(i=0; i<nof_autocomplete_words; i++)
  {
    if(!strncmp(s, autocomplete_words[i], len))
    {
      nof_matches++;

      if(nof_matches == 1)
        matches = (char **)GC_MALLOC(nof_matches * sizeof(char *));
      else
      {
        char **temp = (char **)GC_REALLOC(matches, nof_matches * sizeof(char *));
        assert(temp);
        matches = temp;
      }

      matches[nof_matches-1] = GC_strdup(autocomplete_words[i]);
    }
  }

  unsigned int n;
  char *buf;

  char *s1 = get_common_prefix(nof_matches, matches);

  if(s1)
  {
    n = strlen(s1) - len;

    if(n==0)
      return;

    buf = (char *)GC_MALLOC(n * sizeof(char) + 2);
    memset(buf, '\0', n+2);

    for(i=len; i<len+n; i++)
      buf[i-len] = s1[i];

    /* free(s1); */

    /* for(i=0; i<nof_matches; i++) */
    /*   free(matches[i]); */

    /* if(matches) */
    /*   free(matches); */
  }
  else
    return;

  //GtkTextView *view = (buffer == (GtkTextBuffer *)workspace_source_buffer) ? (GtkTextView *)workspace_source_view : system_browser_textview;

  GtkTextView *view;

  if(buffer == (GtkTextBuffer *)workspace_source_buffer)
    view = (GtkTextView *)workspace_source_view;
  else if(buffer == (GtkTextBuffer *)class_browser_source_buffer)
    view = (GtkTextView *)class_browser_source_view;
  else if(buffer == curr_file_browser_buffer)
    view = curr_file_browser_text_view;
  else
    assert(false);


  //going into overwrite mode
  //to handle the case when autocomplete
  //is attempted when cursor is in the
  //middle of an already fully complete symbol
  gtk_text_view_set_overwrite(view, TRUE);
  gtk_text_buffer_insert_at_cursor(buffer, (char *)buf, -1);
  gtk_text_view_set_overwrite(view, FALSE);

  //to take the cursor to the end of the word
  //(overwrite doesn't move the cursor)
  GtkTextIter curr_iter;
  gtk_text_buffer_get_iter_at_mark(buffer, &curr_iter, gtk_text_buffer_get_insert(buffer));

  for(i=0; i<n; i++)
    gtk_text_iter_forward_char(&curr_iter);

  //free(buf);

  //free(s);
}

void build_autocomplete_words()
{
  int i;

  char *inbuilt_words[] = {

    //TODO: add to these as we build the class library
    
    //system-created classes
    "Integer", "Float", "Object", "Smalltalk", "Nil", "nil", "Transcript", "NiladicBlock", "MonadicBlock",
    "Boolean", "Exception", "Compiler", "DyadicValuable", "Character",

    //reserved messages (per ANSI standard)
    "ifTrue:", "ifFalse:", "to:", "by:", "do:", "and:", "or:", "==", "timesRepeat:", "basicAt:",
    "put:", "basicSize", "basicNew:",

    "new", "new:", "initialize",
    
    //system-created messages
    "addInstanceVariable:", "toClass:", "addClassVariable:", "createGlobal:", "valued:", "gensym", "addInstanceMethod:",
    "withBody:", "addClassMethod:", "eval:", "loadFile:", "addBreakpointTo:", "removeBreakpointFrom:", "ofClass:",
    "assignClass:", "toPackage:",
    "show:",
    "agrumentCount", "value", "on:", "do:", "ensure:", "ifCurtailed:", "whileTrue:", "whileFalse:",
    "value:",
    "and:", "eqv:", "not", "or:", "xor:", "printString",
    "return", "return:", "retry", "retryUsing:", "resume", "resume:", "pass", "outer", "signal", "resignalAs:", "signal:",
    "compile:", "pass:",
    "at:", "size", "do:", "separatedBy:",
    "add:", "addLast:", "removeLast",
    "isEmpty", "notEmpty", "select", "reject", "occurrencesOf:", "includes:", "detect:", "ifNone:", "collect:", "substringFrom:", "to:"
  };

  nof_autocomplete_words = 93; //TODO: remember to update this when new inbuilt classes and primitive methods are added
  autocomplete_words = (char **)GC_MALLOC(nof_autocomplete_words * sizeof(char *));
  
  assert(autocomplete_words);

  for(i=0; i<nof_autocomplete_words; i++)
    autocomplete_words[i] = inbuilt_words[i];
}

void set_up_autocomplete_words()
{
  lm = gtk_source_language_manager_get_default();
  setup_language_manager_path(lm);
  source_language = gtk_source_language_manager_get_language(lm, "smalltalk");
  
  if(!keywords_buffer)
    keywords_buffer = gtk_source_buffer_new_with_language(source_language);

  //clear the buffer first (to replace symbols from previous package)
  gtk_text_buffer_set_text((GtkTextBuffer *)keywords_buffer, "", -1);
  
  char *inbuilt_words = "Integer\nFloat\nObject\nSmalltalk\nNil\nnil\nTranscript\nNiladicBlock\nMonadicBlock\n \
    Boolean\nException\nCompiler\nDyadicValuable\nCharacter\n \
    ifTrue:\nifFalse:\nto:\nby:\ndo:\nand:\nor:\n==\ntimesRepeat:\nbasicAt:\n \
    put:\nbasicSize\nbasicNew:\n \
    new\nnew:\ninitialize\n \
    addInstanceVariable:\ntoClass:\naddClassVariable:\ncreateGlobal:\nvalued:\ngensym\naddInstanceMethod:\n \
    withBody:\naddClassMethod:\neval:\nloadFile:\naddBreakpointTo:\nremoveBreakpointFrom:\nofClass:\n \
    assignClass:\ntoPackage:\n \
    show:\n \
    agrumentCount\nvalue\non:\ndo:\nensure:\nifCurtailed:\nwhileTrue:\nwhileFalse:\n \
    value:\n \
    and:\neqv:\nnot\nor:\nxor:\nprintString\n \
    return\nreturn:\nretry\nretryUsing:\nresume\nresume:\npass\nouter\nsignal\nresignalAs:\nsignal:\n \
    compile:\npass:\n \
    at:\nsize\ndo:\nseparatedBy:\n \
    add:\naddLast:\nremoveLast\n \
    isEmpty\nnotEmpty\nselect\nreject\noccurrencesOf:\nincludes:\ndetect:\nifNone:\ncollect:\nsubstringFrom:\nto:\n";

  gtk_text_buffer_set_text((GtkTextBuffer *)keywords_buffer, inbuilt_words, -1);

  if(!provider)
    provider = (GtkSourceCompletionProvider *)gtk_source_completion_words_new("Symbols", NULL);
  
  gtk_source_completion_words_register((GtkSourceCompletionWords *)provider, (GtkTextBuffer *)keywords_buffer);
}
