#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksource.h>

#include <glib.h>
#include <pthread.h>

#include <assert.h>

#include "gc.h"

#include "../global_decls.h"

int call_repl(char *);

void show_info_dialog(char *msg);

void create_workspace_window(int, int, int, int, char *);
void create_class_browser_window(int, int, int, int);
void create_file_browser_window(int, int, int, int);

void print_object_to_string(OBJECT_PTR, char *);

void print_to_workspace(char *, GtkTextTag *);

void remove_all_from_list(GtkTreeView *);

void show_error_dialog(char *);

void hide_debug_window();

void render_executable_code(GtkTextBuffer *, int *, BOOLEAN, gint64, executable_code_t *);

void show_system_browser_window();
void show_workspace_window();
void show_file_browser_window();

void close_application_window(GtkWidget **window);

void quit_application();

void refresh_system_browser();

void do_auto_complete(GtkTextBuffer *);

void load_source();

void reload_file();
void save_file();
void close_file();
void fb_load_source_file();
void new_source_file();
BOOLEAN quit_file_browser();
void find_text();

BOOLEAN g_debug_in_progress;

enum DebugAction g_debug_action;

extern GtkWindow *action_triggering_window;

extern GtkWindow *transcript_window;
extern GtkWindow *workspace_window;
extern GtkWindow *class_browser_window;
extern GtkWindow *file_browser_window;

extern GtkTextBuffer *transcript_buffer;
extern GtkTextBuffer *workspace_buffer;
extern GtkTextBuffer *class_browser_source_buffer;
extern GtkTextBuffer *curr_file_browser_buffer;

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

extern executable_code_t *g_exp;

extern GtkTextTag *workspace_tag;

extern GtkTreeView *call_chain_list;

extern stack_type *g_breakpointed_methods;

void evaluate()
{
  char *expression = NULL;

  if(action_triggering_window == workspace_window || action_triggering_window == file_browser_window)
  {
    GtkTextBuffer *buf = action_triggering_window == workspace_window ? workspace_buffer : curr_file_browser_buffer;

    gboolean selected;
    GtkTextIter start_sel, end_sel;

    selected = gtk_text_buffer_get_selection_bounds(buf, &start_sel, &end_sel);

    if(selected)
      expression = (char *) gtk_text_buffer_get_text(buf, &start_sel, &end_sel, FALSE);
  }
  else if(action_triggering_window == class_browser_window)
  {
    GtkTextIter start_buffer, end_buffer;

    gtk_text_buffer_get_start_iter(class_browser_source_buffer, &start_buffer);
    gtk_text_buffer_get_end_iter(class_browser_source_buffer, &end_buffer);

    expression = (char *)gtk_text_buffer_get_text(class_browser_source_buffer, &start_buffer, &end_buffer, FALSE);
  }
  else
    assert(0); //TODO

  if(expression)
    call_repl(expression);
}

gboolean handle_key_press_events(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  if(widget == (GtkWidget *)workspace_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_d)
  {
    action_triggering_window = workspace_window;
    evaluate();
    return TRUE;
  }
  /* else if(widget == (GtkWidget *)class_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_s) */
  /* { */
  /*   if(event->state & GDK_CONTROL_MASK) */
  /*   { */
  /*     action_triggering_window = class_browser_window; */
  /*     evaluate(); */
  /*     return TRUE; */
  /*   } */
  /* } */
  else if(widget == (GtkWidget *)workspace_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_p)
  {
    action_triggering_window = workspace_window;
    evaluate();

    char buf[500];
    memset(buf, '\0', 500);
    print_object_to_string(g_last_eval_result, buf);
    print_to_workspace(buf, workspace_tag);

    return TRUE;
  }
  else if(event->keyval == GDK_KEY_F9)
    show_system_browser_window();
  else if(event->keyval == GDK_KEY_F7)
    show_workspace_window();
  else if(widget == (GtkWidget *)workspace_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)
    close_application_window((GtkWidget **)&workspace_window);
  else if(widget == (GtkWidget *)class_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)
    close_application_window((GtkWidget **)&class_browser_window);
  else if(widget == (GtkWidget *)transcript_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_Q)
    quit_application();
  else if(widget == (GtkWidget *)class_browser_window && event->keyval == GDK_KEY_F5)
  {
    action_triggering_window = class_browser_window;

    //if(!check_for_sys_browser_changes())
    //  return TRUE;

    refresh_system_browser();
  }
  else if((widget == (GtkWidget *)workspace_window || widget == (GtkWidget *)class_browser_window) &&
	  event->keyval == GDK_KEY_Tab)
  {
    do_auto_complete(widget == (GtkWidget *)workspace_window ? workspace_buffer : class_browser_source_buffer);
    return TRUE;
  }
  else if(widget == (GtkWidget *)workspace_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_l)
    load_source();
  else if(widget == (GtkWidget *)workspace_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_b)
  {
    show_file_browser_window();
    return TRUE;
  }
  else if(widget == (GtkWidget *)file_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_d)
  {
    action_triggering_window = file_browser_window;
    evaluate();
    return TRUE;
  }
  else if(widget == (GtkWidget *)file_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_n)
  {
    new_source_file();
    return TRUE;
  }
  else if(widget == (GtkWidget *)file_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_o)
  {
    fb_load_source_file();
    return TRUE;
  }
  else if(widget == (GtkWidget *)file_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_s)
    save_file();
  else if(widget == (GtkWidget *)file_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)
    close_file();
  else if(widget == (GtkWidget *)file_browser_window && event->keyval == GDK_KEY_F5)
    reload_file();
  else if(widget == (GtkWidget *)file_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_f)
    find_text();
  else if(widget == (GtkWidget *)file_browser_window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_q)
  {
    quit_file_browser();
    return TRUE;
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

void show_system_browser_window()
{
  if(class_browser_window == NULL)
    create_class_browser_window(DEFAULT_BROWSER_WINDOW_POSX,
				DEFAULT_BROWSER_WINDOW_POSY,
				DEFAULT_BROWSER_WINDOW_WIDTH,
				DEFAULT_BROWSER_WINDOW_HEIGHT);
  else
  {
    gtk_window_present(class_browser_window);
  }
}

void show_system_browser_win(GtkWidget *widget,
			     gpointer data)
{
  show_system_browser_window();
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
  else if((GtkWidget *)data == (GtkWidget *)class_browser_window)
    close_application_window((GtkWidget **)&class_browser_window);
  else if((GtkWidget *)data == (GtkWidget *)file_browser_window)
    close_application_window((GtkWidget **)&file_browser_window);
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
  else if(widget == (GtkWidget *)class_browser_window)
    close_application_window((GtkWidget **)&class_browser_window);

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

    if(call_chain_entry_index < 0 || call_chain_entry_index >= stack_count(g_call_chain))
      return;

    char buf[MAX_STRING_LENGTH];
    memset(buf, '\0', MAX_STRING_LENGTH);

    gtk_text_buffer_set_text((GtkTextBuffer *)debugger_source_buffer, buf, -1);

    call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);

    call_chain_entry_t *entry = entries[call_chain_entry_index];
    OBJECT_PTR method = entry->method;

    method_t *m = (method_t *)extract_ptr(method);

    remove_all_from_list(temp_vars_list);

    action_triggering_window = debugger_window;

    if(m->code_str != NIL)
    {
      //gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)debugger_source_buffer,
      //			       g_string_literals[m->code_str >> OBJECT_SHIFT], -1);
      /*
      executable_code_t *prev_exp = g_exp;
      char *buf = GC_strdup(g_string_literals[m->code_str >> OBJECT_SHIFT]);
      yy_scan_string(buf);
      assert(!yyparse()); //m->code_str should be valid
      render_executable_code(debugger_source_buffer, false, call_chain_entry_index, g_exp);
      g_exp = prev_exp;
      */

      char header[200];
      memset(header, '\0', 200);

      char *sym = get_symbol_name(entry->selector);

      sprintf(header,
	      "Smalltalk %s #%s toClass: %s withBody:\n",
	      m->class_method ? "addClassMethod:" : "addInstanceMethod:",
	      substring(sym, 1, strlen(sym) - 1),
	      m->cls_obj->name);
      gtk_text_buffer_insert_at_cursor(debugger_source_buffer, header, -1);

      int indents = 0;
      render_executable_code(debugger_source_buffer, &indents, false, call_chain_entry_index, m->exec_code);

      //fetch temp vars for the call chain entry

      GtkListStore *store1;
      GtkTreeIter  iter1;

      store1 = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(temp_vars_list)));

      //this assert will not hold now, as the local vars list will be populated
      //only when the method is executed
      //assert(cons_length(m->temporaries) == cons_length(entry->local_vars_list));

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
    else
      gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)debugger_source_buffer,
				       "<primitive method>", -1);
  }
}

void debug_abort(GtkWidget *widget, gpointer data)
{
  g_debug_action = ABORT;

  gtk_main_quit();
  g_last_eval_result = NIL;
  hide_debug_window();
  g_debug_in_progress = false;
}

void debug_retry(GtkWidget *widget, gpointer data)
{
  if(!g_debugger_invoked_for_exception)
    return;

  gtk_main_quit();
  hide_debug_window();

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
					     entry->exp_ptr,
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
  hide_debug_window();

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
  g_debug_action = CONTINUE;

  gtk_main_quit();
  hide_debug_window();

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
    hide_debug_window();

    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  Smalltalk,
				  NIL,
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
    //return;
  }

  g_debug_action = STEP_INTO;

  gtk_main_quit();
  hide_debug_window();

  g_debug_in_progress = false;

  //control passed pack to message_send_internal()
}

void debug_step_over(GtkWidget *widget, gpointer data)
{
  if(g_debugger_invoked_for_exception)
    return;

  g_debug_action = STEP_OVER;

  gtk_main_quit();
  hide_debug_window();

  g_debug_in_progress = false;

  //control passed pack to message_send_internal()
}

void debug_step_out(GtkWidget *widget, gpointer data)
{
  if(g_debugger_invoked_for_exception)
    return;

  g_debug_action = STEP_OUT;

  gtk_main_quit();
  hide_debug_window();

  g_debug_in_progress = false;

  //control passed pack to message_send_internal()
}

void debug_delete_breakpoint(GtkWidget *widget, gpointer data)
{
  if(g_debugger_invoked_for_exception)
    return;

  GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(call_chain_list)));
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (call_chain_list));
  GtkTreeIter  iter;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(call_chain_list));

  if(!selection)
    return;

  if(gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gint64 call_chain_entry_index;

    gtk_tree_model_get(model, &iter,
                       1, &call_chain_entry_index,
                       -1);

    if(call_chain_entry_index < 0 || call_chain_entry_index >= stack_count(g_call_chain))
      return;

    call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);

    call_chain_entry_t *entry = entries[call_chain_entry_index];
    OBJECT_PTR method = entry->method;

    method_t *m = (method_t *)extract_ptr(method);

    m->breakpointed = false;
  }
}

void debug_delete_all_breakpoints(GtkWidget *widget, gpointer data)
{
  if(g_debugger_invoked_for_exception)
    return;

  while(!stack_is_empty(g_breakpointed_methods))
  {
    method_t *m = (method_t *)stack_pop(g_breakpointed_methods);
    m->breakpointed = false;
  }
}

static gpointer invoke_load_file_message(gpointer data)
{
  char *file_name = (char *)data;

  OBJECT_PTR ret = message_send(g_msg_snd_closure,
				Smalltalk,
				NIL,
				get_symbol("_loadFile:"),
				convert_int_to_object(1),
				get_string_obj(file_name),
				g_idclo);

  if(ret == NIL)
    show_error_dialog("Error loading file");
  else
    show_info_dialog("File loaded successfully");

  return NULL;
}

void load_source()
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new ("Load Smalltalk source file",
                                        (GtkWindow *)workspace_window,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        "Cancel", GTK_RESPONSE_CANCEL,
                                        "Open", GTK_RESPONSE_ACCEPT,
                                        NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {

    char *loaded_source_file_name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

    gtk_widget_destroy (dialog);

    //invoking the load file in a separate thread (to
    //close the dialog immediately) results in segfault
    //as there could be GUI updates triggered by the code
    //in the loaded file (e.g. Transcript>>show:) - such GUI
    //updates from worker threads are not recommended without
    //proper synchronization.
    //g_thread_new("Load file", invoke_load_file_message, loaded_source_file_name);
    invoke_load_file_message(loaded_source_file_name);
  }
  else
    gtk_widget_destroy (dialog);
}

void load_source_file(GtkWidget *widget,
                      gpointer data)
{
  load_source();
}

void show_file_browser_window()
{
  if(file_browser_window == NULL)
    create_file_browser_window(DEFAULT_WORKSPACE_POSX,
                               DEFAULT_WORKSPACE_POSY,
                               DEFAULT_WORKSPACE_WIDTH,
                               DEFAULT_WORKSPACE_HEIGHT);
  else
  {
    gtk_window_present(file_browser_window);
  }
}

void show_file_browser_win(GtkWidget *widget,
                           gpointer  data)
{
  show_file_browser_window();
}
