#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksource.h>

#include <assert.h>

#include "../smalltalk.h"

int call_repl(char *);

void show_info_dialog(char *msg);

void create_workspace_window(int, int, int, int, char *);

void print_object_to_string(OBJECT_PTR, char *);

void print_to_workspace(char *);

extern GtkWindow *action_triggering_window;

extern GtkWindow *transcript_window;
extern GtkWindow *workspace_window;

extern GtkTextBuffer *transcript_buffer;
extern GtkTextBuffer *workspace_buffer;

extern OBJECT_PTR g_last_eval_result;

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
