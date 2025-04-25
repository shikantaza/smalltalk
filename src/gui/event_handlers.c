#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksource.h>

#include <assert.h>

int call_repl(char *);

extern GtkWindow *action_triggering_window;
extern GtkWindow *transcript_window;
extern GtkWindow *workspace_window;
extern GtkTextBuffer *workspace_buffer;

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
