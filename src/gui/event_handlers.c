#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksource.h>

#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

int call_repl(char *);

void show_info_dialog(char *msg);

void create_workspace_window(int, int, int, int, char *);

void print_object_to_string(OBJECT_PTR, char *);

void print_to_workspace(char *);

void remove_all_from_list(GtkTreeView *);

void show_error_dialog(char *);

BOOLEAN g_debug_in_progress;

enum DebugAction g_debug_action;

extern GtkWindow *action_triggering_window;

extern GtkWindow *transcript_window;
extern GtkWindow *workspace_window;

extern GtkTextBuffer *transcript_buffer;
extern GtkTextBuffer *workspace_buffer;

extern OBJECT_PTR g_last_eval_result;

extern GtkTextBuffer *debugger_source_buffer;

extern stack_type *g_call_chain;

extern OBJECT_PTR NIL;
extern char **g_string_literals;

extern GtkTreeView *temp_vars_list;

extern GtkWindow *debugger_window;

extern exception_handler_t *g_active_handler;

extern stack_type *g_exception_contexts;

extern OBJECT_PTR g_idclo;

extern OBJECT_PTR g_debug_cont;

extern OBJECT_PTR g_msg_snd_closure;
extern OBJECT_PTR Smalltalk;

extern BOOLEAN g_debugger_invoked_for_exception;

void evaluate()
{
  GtkTextBuffer *buf;
  GtkTextIter start_sel, end_sel;
  gboolean selected;

  if(action_triggering_window == workspace_window)
    buf = workspace_buffer;
  else
    assert(0); //TODO

  selected = gtk_text_buffer_get_selection_bounds(buf, &start_sel, &end_sel);

  char *expression = NULL;

  if(selected)
    expression = (char *) gtk_text_buffer_get_text(buf, &start_sel, &end_sel, FALSE);

  if(expression)
    call_repl(expression);
}

gboolean handle_key_press_events(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  if(widget == (GtkWidget *)workspace_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_d)
  {
    if(event->state & GDK_CONTROL_MASK)
    {
      action_triggering_window = workspace_window;
      evaluate();
      return TRUE;
    }
  }
  else if(widget == (GtkWidget *)workspace_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_p)
  {
    if(event->state & GDK_CONTROL_MASK)
    {
      action_triggering_window = workspace_window;
      evaluate();

      char buf[500];
      memset(buf, '\0', 500);
      print_object_to_string(g_last_eval_result, buf);
      print_to_workspace(buf);

      return TRUE;
    }
  }

  return FALSE;
}

void quit_application()
{
  GtkWidget *dialog = gtk_message_dialog_new ((GtkWindow *)transcript_window,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "Do you really want to quit?");

  gtk_widget_grab_focus(gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO));

  if(gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
  {
    gtk_widget_destroy((GtkWidget *)dialog);

    /*
    if(!check_for_sys_browser_changes())
      return;
    
    //uncomment this if we want to
    //auto save image on quit
    //if(loaded_image_file_name != NULL)
    //  save_image();
    if(system_changed && loaded_image_file_name)
    {
      GtkWidget *dialog1 = gtk_message_dialog_new ((GtkWindow *)transcript_window,
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_QUESTION,
                                                   GTK_BUTTONS_YES_NO,
                                                   "System has changed; save image?");

      gtk_widget_grab_focus(gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog1), GTK_RESPONSE_YES));

      if(gtk_dialog_run(GTK_DIALOG (dialog1)) == GTK_RESPONSE_YES)
      {
        gtk_widget_destroy((GtkWidget *)dialog1);
        save_image();
      }
    }

    cleanup();
    */
    
    gtk_main_quit();
    exit(0);
  }
  else
    gtk_widget_destroy((GtkWidget *)dialog);
}

void quit(GtkWidget *widget,
          gpointer   data )
{
  quit_application();
}

void load_image_file(GtkWidget *widget,
                     gpointer data)
{
  show_info_dialog("To be implemented");
}

void save_image_file(GtkWidget *widget,
                     gpointer data)
{
  show_info_dialog("To be implemented");
}

void show_system_browser_win(GtkWidget *widget,
			     gpointer data)
{
  show_info_dialog("To be implemented");
}

void show_workspace_window()
{
  if(workspace_window == NULL)
    create_workspace_window(DEFAULT_WORKSPACE_POSX,
                            DEFAULT_WORKSPACE_POSY,
                            DEFAULT_WORKSPACE_WIDTH,
                            DEFAULT_WORKSPACE_HEIGHT,
                            "");
  else
  {
    gtk_window_present(workspace_window);
  }
}

void show_workspace_win(GtkWidget *widget,
                           gpointer  data)
{
  show_workspace_window();
}

void close_application_window(GtkWidget **window)
{
  gtk_widget_destroy(*window);
  *window = NULL;
}

void close_window(GtkWidget *widget,
                  gpointer data)
{
  if((GtkWidget *)data == (GtkWidget *)workspace_window)
    close_application_window((GtkWidget **)&workspace_window);
}

gboolean delete_event(GtkWidget *widget,
		      GdkEvent *event,
		      gpointer data)
{
  if(widget == (GtkWidget *)transcript_window)
  {
    quit_application();

    //if control comes here, it means
    //the user cancelled the quit operation
    return TRUE;
  }
  else if(widget == (GtkWidget *)workspace_window)
    close_application_window((GtkWidget **)&workspace_window);

  return FALSE;
}

void set_triggering_window(GtkWidget *widget,
                           gpointer   data)
{
  action_triggering_window = (GtkWindow *)widget;
}

void clear_transcript(GtkWidget *widget,
		      gpointer data)
{
  gtk_text_buffer_set_text(transcript_buffer, "", -1);
}

void clear_workspace(GtkWidget *widget, gpointer data)
{
  gtk_text_buffer_set_text(workspace_buffer, "", -1);
}

void load_source_file(GtkWidget *widget, gpointer data)
{
  show_info_dialog("To be implemented");
}

void show_file_browser_win(GtkWidget *widget, gpointer data)
{
  show_info_dialog("To be implemented");
}

void eval_expression(GtkWidget *widget, gpointer data)
{
  action_triggering_window = (GtkWindow *)data;
  evaluate();
}

void fetch_details_for_call_chain_entry(GtkWidget *lst, gpointer data)
{
  GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lst)));
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (lst));
  GtkTreeIter  iter;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(lst));

  if(!selection)
    return;

  if(gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gint64 call_chain_entry_index;

    gtk_tree_model_get(model, &iter,
                       1, &call_chain_entry_index,
                       -1);

    char buf[MAX_STRING_LENGTH];
    memset(buf, '\0', MAX_STRING_LENGTH);

    gtk_text_buffer_set_text((GtkTextBuffer *)debugger_source_buffer, buf, -1);

    call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);

    call_chain_entry_t *entry = entries[call_chain_entry_index];
    OBJECT_PTR method = entry->method;

    method_t *m = (method_t *)extract_ptr(method);

    if(m->code_str != NIL)
      gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)debugger_source_buffer,
				       g_string_literals[m->code_str >> OBJECT_SHIFT], -1);
    else
      gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)debugger_source_buffer,
				       "<primitive method>", -1);

    //fetch temp vars for the call chain entry
    remove_all_from_list(temp_vars_list);

    GtkListStore *store1;
    GtkTreeIter  iter1;

    store1 = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(temp_vars_list)));

    assert(cons_length(m->temporaries) == cons_length(entry->local_vars_list));

    OBJECT_PTR rest = reverse(entry->local_vars_list);
    OBJECT_PTR rest1 = m->temporaries;

    char var_name[100], var_val[100];

    while(rest != NIL)
    {
      memset(var_name, '\0', 100);
      memset(var_val, '\0', 100);

      print_object_to_string(car(rest1), var_name);
      print_object_to_string(car(car(rest)), var_val);

      gtk_list_store_append(store1, &iter1);
      gtk_list_store_set(store1, &iter1, 0, var_name, 1, var_val, -1);

      rest = cdr(rest);
      rest1 = cdr(rest1);
    }
    //
  }
}

void debug_abort(GtkWidget *widget, gpointer data)
{
  g_debug_action = ABORT;

  gtk_main_quit();
  g_last_eval_result = NIL;
  close_application_window((GtkWidget **)&debugger_window);
  g_debug_in_progress = false;
}

void debug_retry(GtkWidget *widget, gpointer data)
{
  if(!g_debugger_invoked_for_exception)
    return;

  gtk_main_quit();
  close_application_window((GtkWidget **)&debugger_window);

  stack_pop(g_call_chain); //Exception>>signal

  call_chain_entry_t *entry = stack_pop(g_call_chain); //the method that signalled the exception

  int i;
  int n = entry->nof_args;

  OBJECT_PTR *args = (OBJECT_PTR *)GC_MALLOC((n+1) * sizeof(OBJECT_PTR));

  for(i=0; i<n; i++)
    args[i] = entry->args[i];

  args[n] = entry->cont;

  g_last_eval_result = message_send_internal(entry->super,
					     entry->receiver,
					     entry->selector,
					     convert_int_to_object(n),
					     args);

  g_debug_in_progress = false;
}

void debug_resume(GtkWidget *widget, gpointer data)
{
  if(!g_debugger_invoked_for_exception)
    return;

  gtk_main_quit();
  close_application_window((GtkWidget **)&debugger_window);

  if(!stack_is_empty(g_call_chain) && g_active_handler != NULL)
    invoke_curtailed_blocks(g_active_handler->cont);

  OBJECT_PTR exception_context;

  if(!stack_is_empty(g_exception_contexts))
    exception_context = (OBJECT_PTR)stack_pop(g_exception_contexts);
  else
    exception_context = g_idclo;

  assert(IS_CLOSURE_OBJECT(exception_context));

  //TODO: check if the exception object is resumable,
  //if it is, return the default resumption value

  //two pops, the frame corresponding to Exception>>signal
  //and the frame corresponding to the method that signalled
  //the exception
  if(!stack_is_empty(g_call_chain))
    stack_pop(g_call_chain);

  if(!stack_is_empty(g_call_chain))
    stack_pop(g_call_chain);

  g_last_eval_result = invoke_cont_on_val(exception_context, NIL);

  g_debug_in_progress = false;
}

void debug_continue(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
  close_application_window((GtkWidget **)&debugger_window);

  if(g_debugger_invoked_for_exception)
    g_last_eval_result = invoke_cont_on_val(g_debug_cont, NIL);

  g_debug_in_progress = false;
}

int get_expression(char *buf)
{
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *content_area;

  dialog = gtk_dialog_new_with_buttons("Resume with value",
                                       action_triggering_window,
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       //GTK_STOCK_OK,
                                       "OK",
                                       GTK_RESPONSE_ACCEPT,
                                       //GTK_STOCK_CANCEL,
                                       "Cancel",
                                       GTK_RESPONSE_REJECT,
                                       NULL);

  gtk_window_set_resizable((GtkWindow *)dialog, FALSE);

  gtk_window_set_transient_for(GTK_WINDOW(dialog), action_triggering_window);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  entry = gtk_entry_new();
  gtk_container_add(GTK_CONTAINER(content_area), entry);

  gtk_widget_show_all(dialog);
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));

  if(result == GTK_RESPONSE_ACCEPT)
    strcpy(buf, gtk_entry_get_text(GTK_ENTRY(entry)));

  gtk_widget_destroy(dialog);

  return result;
}

void debug_resume_with_val(GtkWidget *widget, gpointer data)
{
  if(!g_debugger_invoked_for_exception)
    return;

  char buf[MAX_STRING_LENGTH];
  memset(buf, '\0', MAX_STRING_LENGTH);

  int result = GTK_RESPONSE_ACCEPT;

  char trimmed_buf[100];

  while(result == GTK_RESPONSE_ACCEPT && strlen(buf) == 0 )
  {
    result = get_expression(buf);

    memset(trimmed_buf, '\0', 100);

    //trim_whitespace(trimmed_buf, 100, buf);

    if(strlen(buf) == 0 && result == GTK_RESPONSE_ACCEPT)
      show_error_dialog("Please enter a valid expression\n");
  }

  if(result == GTK_RESPONSE_ACCEPT)
  {
    gtk_main_quit();
    close_application_window((GtkWidget **)&debugger_window);

    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  Smalltalk,
				  get_symbol("_eval:"),
				  convert_int_to_object(1),
				  get_string_obj(buf),
				  g_idclo);

    if(ret == NIL)
      show_error_dialog("Error evaluating the given resumption value, resuming with nil\n");

    if(!stack_is_empty(g_call_chain) && g_active_handler != NULL)
      invoke_curtailed_blocks(g_active_handler->cont);

    OBJECT_PTR exception_context;

    if(!stack_is_empty(g_exception_contexts))
      exception_context = (OBJECT_PTR)stack_pop(g_exception_contexts);
    else
      exception_context = g_idclo;

    assert(IS_CLOSURE_OBJECT(exception_context));

    //two pops, the frame corresponding to Exception>>signal
    //and the frame corresponding to the method that signalled
    //the exception
    if(!stack_is_empty(g_call_chain))
      stack_pop(g_call_chain);

    if(!stack_is_empty(g_call_chain))
      stack_pop(g_call_chain);

    g_last_eval_result = invoke_cont_on_val(exception_context, ret);

    g_debug_in_progress = false;
  }
}

void debug_step_into(GtkWidget *widget, gpointer data)
{
  if(g_debugger_invoked_for_exception)
    return;

  //can't step into a primitive method
  assert(!stack_is_empty(g_call_chain));

  call_chain_entry_t *entry = stack_top(g_call_chain);
  method_t *m = (method_t *)extract_ptr(entry->method);

  if(m->code_str == NIL)
  {
    show_error_dialog("Cannot step into primitive method");
    return;
  }

  g_debug_action = STEP_INTO;

  gtk_main_quit();
  close_application_window((GtkWidget **)&debugger_window);

  g_debug_in_progress = false;

  //control passed pack to message_send_internal()
}
