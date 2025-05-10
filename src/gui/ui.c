#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gtksourceview/gtksource.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

#include "../global_decls.h"
#include "../util.h"

gboolean handle_key_press_events(GtkWidget *, 
				 GdkEventKey *,
				 gpointer);

void quit(GtkWidget *, gpointer);
void load_image_file(GtkWidget *, gpointer);
void save_image_file(GtkWidget *, gpointer);
void show_system_browser_win(GtkWidget *, gpointer);
void show_workspace_win(GtkWidget *, gpointer);
void clear_transcript(GtkWidget *, gpointer);
void clear_workspace(GtkWidget *, gpointer);

void close_window(GtkWidget *, gpointer);
gboolean delete_event(GtkWidget *widget, GdkEvent *, gpointer);

void set_triggering_window(GtkWidget *, gpointer);

void load_source_file(GtkWidget *, gpointer);
void show_file_browser_win(GtkWidget *, gpointer);
void eval_expression(GtkWidget *, gpointer);

void setup_language_manager_path(GtkSourceLanguageManager *);

void fetch_details_for_call_chain_entry(GtkWidget *, gpointer);

void debug_abort(GtkWidget *, gpointer);
void debug_retry(GtkWidget *, gpointer);
void debug_resume(GtkWidget *, gpointer);
void debug_resume_with_val(GtkWidget *, gpointer);
void debug_continue(GtkWidget *, gpointer);
void debug_step_into(GtkWidget *, gpointer);

GtkTextBuffer *transcript_buffer;
GtkTextBuffer *workspace_buffer;

GtkSourceView *workspace_source_view;
GtkSourceBuffer *workspace_source_buffer;

GtkStatusbar *workspace_statusbar;

GtkSourceLanguage *source_language;
GtkSourceLanguageManager *lm;

GtkSourceCompletionProvider *provider = NULL;

GtkWindow *action_triggering_window;

GtkWindow *transcript_window;
GtkWindow *workspace_window;

GtkTextView *transcript_textview;

GtkTextTag *workspace_tag;

GtkSourceView *debugger_source_view;
GtkSourceBuffer *debugger_source_buffer;

GtkWindow *debugger_window;

GtkTreeView *temp_vars_list;

OBJECT_PTR g_debug_cont;

BOOLEAN g_debugger_invoked_for_exception;

extern stack_type *g_call_chain;

GtkToolbar *create_transcript_toolbar()
{
  GtkWidget *toolbar;

  GtkWidget *load_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/load_image.png");
  GtkWidget *save_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/save_image.png");
  GtkWidget *workspace_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/workspace.png");
  GtkWidget *browser_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/browser.png");
  GtkWidget *clear_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/clear.png");
  GtkWidget *exit_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/exit.png");

  toolbar = gtk_toolbar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);

  GtkToolItem *load_button = gtk_tool_button_new(load_icon, NULL);
  gtk_tool_item_set_tooltip_text(load_button, "Load image (Ctrl-L)");
  g_signal_connect (load_button, "clicked", G_CALLBACK (load_image_file), transcript_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, load_button, 0);

  GtkToolItem *save_button = gtk_tool_button_new(save_icon, NULL);
  gtk_tool_item_set_tooltip_text(save_button, "Save image (Ctrl-S)");
  g_signal_connect (save_button, "clicked", G_CALLBACK (save_image_file), transcript_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, save_button, 1);

  GtkToolItem *workspace_button = gtk_tool_button_new(workspace_icon, NULL);
  gtk_tool_item_set_tooltip_text(workspace_button, "Show workspace window (F7)");
  g_signal_connect (workspace_button, "clicked", G_CALLBACK (show_workspace_win), transcript_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, workspace_button, 2);

  GtkToolItem *browser_button = gtk_tool_button_new(browser_icon, NULL);
  gtk_tool_item_set_tooltip_text(browser_button, "System Browser (F9)");
  g_signal_connect (browser_button, "clicked", G_CALLBACK (show_system_browser_win), transcript_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, browser_button, 3);

  GtkToolItem *clear_button = gtk_tool_button_new(clear_icon, NULL);
  gtk_tool_item_set_tooltip_text(clear_button, "Clear Transcript");
  g_signal_connect (clear_button, "clicked", G_CALLBACK (clear_transcript), transcript_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, clear_button, 4);

  GtkToolItem *exit_button = gtk_tool_button_new(exit_icon, NULL);
  gtk_tool_item_set_tooltip_text(exit_button, "Exit (Ctrl-W)");
  g_signal_connect (exit_button, "clicked", G_CALLBACK (quit), transcript_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, exit_button, 5);

  return (GtkToolbar *)toolbar;
}

void create_transcript_window(int posx, int posy, int width, int height, char *text)
{

  //TODO: find a better place for this (maybe an init_gui() function?)
  lm = gtk_source_language_manager_get_default();
  setup_language_manager_path(lm);
  source_language = gtk_source_language_manager_get_language(lm, "smalltalk");

  GtkWidget *scrolled_win, *vbox;

  transcript_window = (GtkWindow *)gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_icon_from_file(transcript_window, SMALLTALKDATADIR "/icons/evaluate.png", NULL);

  gtk_window_set_title((GtkWindow *)transcript_window, "Transcript");

  gtk_window_set_default_size(transcript_window, width, height);

  gtk_window_move(transcript_window, posx, posy); 
      
  //g_signal_connect (transcript_window, "delete-event",
  //                  G_CALLBACK (delete_event), NULL);

  //g_signal_connect (transcript_window, "focus",
  //                  G_CALLBACK (set_triggering_window), NULL);
    
  //g_signal_connect(transcript_window, 
  //                 "key_press_event", 
  //                 G_CALLBACK (handle_key_press_events), 
  //                 NULL);
    
  gtk_container_set_border_width (GTK_CONTAINER (transcript_window), 10);
  
  GtkWidget *textview = gtk_text_view_new ();

  transcript_textview = (GtkTextView *)textview;

  gtk_text_view_set_editable((GtkTextView *)textview, FALSE);
  gtk_text_view_set_cursor_visible((GtkTextView *)textview, FALSE);
  //gtk_widget_set_sensitive(textview, FALSE);

  //gtk_widget_override_font(GTK_WIDGET(textview), pango_font_description_from_string(FONT));

  transcript_buffer = gtk_text_view_get_buffer((GtkTextView *)textview);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), textview);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  gtk_box_pack_start (GTK_BOX (vbox), (GtkWidget *)create_transcript_toolbar(), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
  
  gtk_container_add (GTK_CONTAINER (transcript_window), vbox);
  
  gtk_widget_show_all((GtkWidget *)transcript_window);
}

void set_up_workspace_source_buffer()
{
  workspace_source_buffer = gtk_source_buffer_new_with_language(source_language);
  workspace_source_view = (GtkSourceView *)gtk_source_view_new_with_buffer(workspace_source_buffer);

  GtkSourceCompletion *sc1 = gtk_source_view_get_completion(workspace_source_view);
  GValue gv = G_VALUE_INIT;
  g_value_init(&gv, G_TYPE_BOOLEAN);
  g_value_set_boolean(&gv, FALSE);
  g_object_set_property((GObject *)sc1, "show-headers", &gv);

  if(!provider)
    provider = (GtkSourceCompletionProvider *)gtk_source_completion_words_new("Symbols", NULL);

  gtk_source_completion_add_provider(sc1, provider, NULL);

}

GtkToolbar *create_workspace_toolbar()
{
  GtkWidget *toolbar;

  GtkWidget *load_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/load_file.png");
  GtkWidget *fb_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/file_browser.png");
  GtkWidget *eval_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/evaluate.png");
  GtkWidget *clear_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/clear32x32.png");
  GtkWidget *exit_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/exit32x32.png");

  toolbar = gtk_toolbar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);

  GtkToolItem *load_button = gtk_tool_button_new(load_icon, NULL);
  gtk_tool_item_set_tooltip_text(load_button, "Load file (Ctrl-L)");
  g_signal_connect (load_button, "clicked", G_CALLBACK (load_source_file), workspace_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, load_button, 0);

  GtkToolItem *fb_button = gtk_tool_button_new(fb_icon, NULL);
  gtk_tool_item_set_tooltip_text(fb_button, "File Browser (Ctrl-B)");
  g_signal_connect (fb_button, "clicked", G_CALLBACK (show_file_browser_win), workspace_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, fb_button, 1);

  GtkToolItem *eval_button = gtk_tool_button_new(eval_icon, NULL);
  gtk_tool_item_set_tooltip_text(eval_button, "Evaluate (Ctrl+D)");
  g_signal_connect (eval_button, "clicked", G_CALLBACK (eval_expression), workspace_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, eval_button, 2);

  GtkToolItem *clear_button = gtk_tool_button_new(clear_icon, NULL);
  gtk_tool_item_set_tooltip_text(clear_button, "Clear Workspace");
  g_signal_connect (clear_button, "clicked", G_CALLBACK (clear_workspace), workspace_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, clear_button, 3);

  GtkToolItem *close_button = gtk_tool_button_new(exit_icon, NULL);
  gtk_tool_item_set_tooltip_text(close_button, "Close (Ctrl-W)");
  g_signal_connect (close_button, "clicked", G_CALLBACK (close_window), workspace_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, close_button, 4);

  return (GtkToolbar *)toolbar;
}

void create_workspace_window(int posx, int posy, int width, int height, char *text)
{
  GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  workspace_window = (GtkWindow *)win;

  gtk_window_set_icon_from_file(workspace_window, SMALLTALKDATADIR "/icons/evaluate.png", NULL);

  gtk_window_set_title((GtkWindow *)workspace_window, "Workspace");

  gtk_window_set_default_size((GtkWindow *)win, width, height);
  gtk_window_move((GtkWindow *)win, posx, posy); 

  g_signal_connect (win, "delete-event",
                    G_CALLBACK (delete_event), NULL);

  g_signal_connect (win, "focus",
                    G_CALLBACK (set_triggering_window), NULL);

  gtk_container_set_border_width (GTK_CONTAINER (win), 10);

  //Widgets *w = g_slice_new (Widgets);
  GtkWidget *scrolled_win, *vbox;

  set_up_workspace_source_buffer();

  //workspace_textview = gtk_text_view_new ();
  GtkWidget *textview = (GtkWidget *)workspace_source_view;

  //gtk_widget_override_font(GTK_WIDGET(textview), pango_font_description_from_string(FONT));

  workspace_buffer = gtk_text_view_get_buffer((GtkTextView *)workspace_source_view);
  //workspace_buffer = (GtkTextBuffer *)workspace_source_buffer;

  workspace_tag = gtk_text_buffer_create_tag((GtkTextBuffer *)workspace_source_buffer, "orange_bg",
					     "background", "orange", NULL);

  //g_signal_connect(G_OBJECT(workspace_buffer), 
  //                 "notify::cursor-position", 
  //                 G_CALLBACK (handle_cursor_move), 
  //                 NULL);

  //print_to_workspace(text);

  g_signal_connect(G_OBJECT(win), 
                   "key_press_event", 
                   G_CALLBACK (handle_key_press_events), 
                   NULL);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), textview);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start (GTK_BOX (vbox), (GtkWidget *)create_workspace_toolbar(), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);

  workspace_statusbar = (GtkStatusbar *)gtk_statusbar_new();
  gtk_box_pack_start (GTK_BOX (vbox), (GtkWidget *)workspace_statusbar, FALSE, FALSE, 0);  
  
  gtk_container_add (GTK_CONTAINER (win), vbox);

  gtk_widget_show_all(win);

  //prompt();

  gtk_widget_grab_focus(textview);
}

void print_to_transcript(char *str)
{
  GtkTextMark *mark = gtk_text_buffer_get_insert(transcript_buffer);
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter(transcript_buffer, &iter );
  gtk_text_buffer_move_mark(transcript_buffer, mark, &iter );
  gtk_text_buffer_insert_at_cursor(transcript_buffer, str, -1 );
  gtk_text_view_scroll_to_mark(transcript_textview, mark, 0.0, TRUE, 0.5, 1 );
}

void print_to_workspace(char *str)
{
  GtkTextMark *mark = gtk_text_buffer_get_insert(workspace_buffer);
  GtkTextIter iter;

  gtk_text_buffer_get_end_iter(workspace_buffer, &iter );
  gtk_text_buffer_move_mark(workspace_buffer, mark, &iter );
  gtk_text_buffer_insert_with_tags(workspace_buffer, &iter, str, -1, workspace_tag, NULL);
  gtk_text_view_scroll_to_mark((GtkTextView *)workspace_source_view, mark, 0.0, TRUE, 0.5, 1 );
}

void show_info_dialog(char *msg)
{
  GtkWidget *dialog = gtk_message_dialog_new (action_triggering_window,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_CLOSE,
                                              "%s",
                                              msg);
  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy((GtkWidget *)dialog);
}

void setup_language_manager_path(GtkSourceLanguageManager *lm)
{
  gchar **lang_files;
  int i, lang_files_count;
  char **new_langs;

  lang_files = g_strdupv ((gchar **)gtk_source_language_manager_get_search_path (lm));

  lang_files_count = g_strv_length (lang_files);
  new_langs = g_new (char*, lang_files_count + 2);

  for (i = 0; lang_files[i]; i++)
    new_langs[i] = lang_files[i];

  new_langs[lang_files_count] = g_strdup (SMALLTALKDATADIR);

  new_langs[lang_files_count+1] = NULL;

  g_free (lang_files);

  gtk_source_language_manager_set_search_path (lm, new_langs);

  g_free(new_langs);
}

void initialize_call_chain_list(GtkTreeView *list)
{
  GtkCellRenderer    *renderer;
  GtkTreeViewColumn  *column1;
  GtkListStore       *store;

  renderer = gtk_cell_renderer_text_new();

  column1 = gtk_tree_view_column_new_with_attributes("Call Chain",
                                                     renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW (list), column1);

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT64);

  gtk_tree_view_set_model(GTK_TREE_VIEW (list),
                          GTK_TREE_MODEL(store));

  g_object_unref(store);
}

void remove_all_from_list(GtkTreeView *list)
{
  GtkListStore *store;
  GtkTreeModel *model;
  GtkTreeIter  iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
  store = GTK_LIST_STORE(model);

  if(gtk_tree_model_get_iter_first(model, &iter) == FALSE)
      return;
  gtk_list_store_clear(store);
}

void populate_call_chain_list(GtkTreeView *call_chain_list)
{
  remove_all_from_list(call_chain_list);

  GtkListStore *store;
  GtkTreeIter  iter;

  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(call_chain_list)));

  call_chain_entry_t **entries = (call_chain_entry_t **)stack_data(g_call_chain);
  int count = stack_count(g_call_chain);

  int i = count - 1;

  int j;

  char buf[500];

  while(i >= 0)
  {
    call_chain_entry_t *entry = entries[i];

    unsigned int len = 0;
    memset(buf, '\0', 500);

    char recv[100];
    memset(recv, '\0', 100);
    print_object_to_string(entry->receiver, recv);

    len += sprintf(buf+len, "(%s)>>", recv);

    char *str = get_symbol_name(entry->selector);

    len += sprintf(buf+len, "%s; ", substring(str, 1, strlen(str)-1));

    if(entry->nof_args > 0)
      len += sprintf(buf+len, "args: [");

    for(j=0; j<entry->nof_args; j++)
    {
      char arg[100];
      memset(arg, '\0', 100);
      print_object_to_string(entry->args[j], arg);
      len += sprintf(buf+len, "%s ", arg);
    }

    if(entry->nof_args > 0)
      len += sprintf(buf+len, "] ");

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, buf, -1);
    gtk_list_store_set(store, &iter, 1, i, -1);

    i--;
  }
}

void initialize_temp_vars_list(GtkTreeView *list)
{
  GtkCellRenderer    *renderer;
  GtkTreeViewColumn  *column1, *column2;
  GtkListStore       *store;

  renderer = gtk_cell_renderer_text_new();

  column1 = gtk_tree_view_column_new_with_attributes("Variable",
                                                     renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW (list), column1);

  column2 = gtk_tree_view_column_new_with_attributes("Value",
                                                     renderer, "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW (list), column2);

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  gtk_tree_view_set_model(GTK_TREE_VIEW (list),
                          GTK_TREE_MODEL(store));

  g_object_unref(store);
}

GtkToolbar *create_debug_toolbar()
{
  GtkWidget *toolbar;

  GtkWidget *abort_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/abort.png");
  GtkWidget *retry_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/retry32x32.png");
  GtkWidget *resume_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/resume32x32.png");
  GtkWidget *resume_with_val_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/resume_with_val32x32.png");
  GtkWidget *continue_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/continue.png");
  GtkWidget *step_into_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/step.png");

  toolbar = gtk_toolbar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);

  GtkToolItem *abort_button = gtk_tool_button_new(abort_icon, NULL);
  gtk_tool_item_set_tooltip_text(abort_button, "Abort");
  g_signal_connect (abort_button, "clicked", G_CALLBACK (debug_abort), debugger_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, abort_button, 0);

  GtkToolItem *retry_button = gtk_tool_button_new(retry_icon, NULL);
  gtk_tool_item_set_tooltip_text(retry_button, "Retry");
  g_signal_connect (retry_button, "clicked", G_CALLBACK (debug_retry), debugger_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, retry_button, 1);

  GtkToolItem *resume_button = gtk_tool_button_new(resume_icon, NULL);
  gtk_tool_item_set_tooltip_text(resume_button, "Resume with nil");
  g_signal_connect (resume_button, "clicked", G_CALLBACK (debug_resume), debugger_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, resume_button, 2);

  GtkToolItem *resume_with_val_button = gtk_tool_button_new(resume_with_val_icon, NULL);
  gtk_tool_item_set_tooltip_text(resume_with_val_button, "Resume with value");
  g_signal_connect (resume_with_val_button, "clicked", G_CALLBACK (debug_resume_with_val), debugger_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, resume_with_val_button, 3);

  GtkToolItem *continue_button = gtk_tool_button_new(continue_icon, NULL);
  gtk_tool_item_set_tooltip_text(continue_button, "Continue");
  g_signal_connect (continue_button, "clicked", G_CALLBACK (debug_continue), debugger_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, continue_button, 4);

  GtkToolItem *step_into_button = gtk_tool_button_new(step_into_icon, NULL);
  gtk_tool_item_set_tooltip_text(step_into_button, "Step into");
  g_signal_connect (step_into_button, "clicked", G_CALLBACK (debug_step_into), debugger_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, step_into_button, 5);

  return (GtkToolbar *)toolbar;
}

void create_debug_window(int posx, int posy, int width, int height,
			 BOOLEAN invoked_for_exception, OBJECT_PTR cont, char *title)
{
  GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_modal((GtkWindow *)win, TRUE);

  GtkWidget *scrolled_win1, *scrolled_win2, *scrolled_win3;
  GtkWidget *vbox, *hbox, *hbox2;

  gtk_window_set_icon_from_file((GtkWindow *)win, SMALLTALKDATADIR "/icons/evaluate.png", NULL);

  gtk_window_set_title((GtkWindow *)win, "Debugger");

  gtk_window_set_position((GtkWindow *)win, GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_default_size((GtkWindow *)win, width, height);
  gtk_window_move((GtkWindow *)win, posx, posy);

  g_signal_connect (win, "delete-event",
                    G_CALLBACK (delete_event), NULL);

  g_signal_connect (win, "focus",
                    G_CALLBACK (set_triggering_window), NULL);

  gtk_container_set_border_width (GTK_CONTAINER (win), 10);

  scrolled_win1 = gtk_scrolled_window_new(NULL, NULL);
  scrolled_win2 = gtk_scrolled_window_new(NULL, NULL);
  scrolled_win3 = gtk_scrolled_window_new(NULL, NULL);

  GtkTreeView *call_chain_list = (GtkTreeView *)gtk_tree_view_new();
  gtk_tree_view_set_headers_visible(call_chain_list, TRUE);
  //gtk_widget_override_font(GTK_WIDGET(call_chain_list), pango_font_description_from_string(FONT));

  g_signal_connect(G_OBJECT(call_chain_list), "cursor-changed",
                   G_CALLBACK(fetch_details_for_call_chain_entry), NULL);

  initialize_call_chain_list(call_chain_list);

  //sorting the call chain is not allowed
  //gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(callers_symbols_list, 0), 0);

  populate_call_chain_list(call_chain_list);

  g_signal_connect(G_OBJECT(win),
                   "key_press_event",
                   G_CALLBACK (handle_key_press_events),
                   NULL);

  gtk_container_add(GTK_CONTAINER (scrolled_win1), (GtkWidget *)call_chain_list);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX (hbox), scrolled_win1, TRUE, TRUE, 0);

  GtkWidget *scrolled_win;

  debugger_source_buffer = gtk_source_buffer_new_with_language(source_language);
  debugger_source_view = (GtkSourceView *)gtk_source_view_new_with_buffer(debugger_source_buffer);

  GtkSourceCompletion *sc1 = gtk_source_view_get_completion(debugger_source_view);
  GValue gv = G_VALUE_INIT;
  g_value_init(&gv, G_TYPE_BOOLEAN);
  g_value_set_boolean(&gv, FALSE);
  g_object_set_property((GObject *)sc1, "show-headers", &gv);
  gtk_source_completion_add_provider(sc1, provider, NULL);

  gtk_text_buffer_create_tag((GtkTextBuffer *)debugger_source_buffer, "gray_bg",
                             "background", "lightgray", NULL);

  //gtk_widget_override_font(GTK_WIDGET(debugger_source_view), pango_font_description_from_string(FONT));
  gtk_text_view_set_editable((GtkTextView *)debugger_source_view, FALSE); //TODO: make it editable later

  //TODO: uncomment later
  //g_signal_connect(G_OBJECT(debugger_source_buffer),
  //                 "notify::cursor-position",
  //                 G_CALLBACK (handle_cursor_move),
  //                 NULL);

  hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  //TODO: if we want to allow multiple debug windows
  //having temp_vars_list as global would be a problem
  temp_vars_list = (GtkTreeView *)gtk_tree_view_new();
  gtk_tree_view_set_headers_visible(temp_vars_list, FALSE);

  initialize_temp_vars_list(temp_vars_list);

  gtk_container_add(GTK_CONTAINER (scrolled_win3), (GtkWidget *)temp_vars_list);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), (GtkWidget *)debugger_source_view);

  gtk_box_pack_start(GTK_BOX (hbox2), scrolled_win, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX (hbox2), scrolled_win3, TRUE, TRUE, 0);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start (GTK_BOX (vbox), (GtkWidget *)create_debug_toolbar(), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);

  //callers_statusbar = (GtkStatusbar *)gtk_statusbar_new();
  //gtk_box_pack_start (GTK_BOX (vbox), (GtkWidget *)callers_statusbar, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (win), vbox);

  debugger_window = (GtkWindow *)win;

  //select the top entry of the call chain stack
  gtk_tree_view_set_cursor(call_chain_list, gtk_tree_path_new_from_indices(0, -1), NULL, false);

  g_debugger_invoked_for_exception = invoked_for_exception;
  g_debug_cont = cont;

  gtk_widget_show_all(win);

  gtk_main();
}

void show_error_dialog(char *msg)
{
  GtkWidget *dialog = gtk_message_dialog_new (action_triggering_window,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_CLOSE,
                                              "%s",
                                              msg);
  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy((GtkWidget *)dialog);
}
