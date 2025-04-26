#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gtksourceview/gtksource.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

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
