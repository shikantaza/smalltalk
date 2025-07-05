#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <assert.h>

#include <gtksourceview/gtksource.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

#include "gc.h"

#include "../global_decls.h"
#include "../util.h"

#define FONT "DejaVu Sans Mono Bold 11"

gboolean delete_event(GtkWidget *widget, GdkEvent *, gpointer);
void set_triggering_window(GtkWidget *, gpointer);
gboolean handle_key_press_events(GtkWidget *, 
				 GdkEventKey *,
				 gpointer);

void close_window(GtkWidget *, gpointer);

void remove_all_from_list(GtkTreeView *list);

void render_executable_code(GtkTextBuffer *, int *, BOOLEAN, gint64, executable_code_t *);

GtkWindow *class_browser_window;

GtkTreeView *packages_list;
GtkTreeView *classes_list;
GtkTreeView *methods_list;

GtkSourceView *class_browser_source_view;
GtkSourceBuffer *class_browser_source_buffer;

GtkWidget *class_radio_button, *instance_radio_button;
GtkWidget *raw_radio_button, *pretty_printed_radio_button;

extern OBJECT_PTR Package;

extern GtkSourceLanguage *source_language;

extern char **g_string_literals;

extern binding_env_t *g_top_level;

extern GtkWindow *action_triggering_window;

extern OBJECT_PTR SELF;
extern OBJECT_PTR THIS_CONTEXT;
extern OBJECT_PTR SUPER;

extern OBJECT_PTR NIL;

extern OBJECT_PTR g_idclo;
extern OBJECT_PTR g_msg_snd_closure;

extern OBJECT_PTR Object;

GtkToolbar *create_class_browser_toolbar()
{
  GtkWidget *toolbar;

  GtkWidget *accept_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/accept.png");
  GtkWidget *refresh_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/refresh.png");
  GtkWidget *exit_icon = gtk_image_new_from_file (SMALLTALKDATADIR "/icons/exit32x32.png");

  toolbar = gtk_toolbar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);

  GtkToolItem *accept_button = gtk_tool_button_new(accept_icon, NULL);
  gtk_tool_item_set_tooltip_text(accept_button, "Accept (Ctrl-S)");
  //g_signal_connect (accept_button, "clicked", G_CALLBACK (accept), class_browser_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, accept_button, 0);

  GtkToolItem *refresh_button = gtk_tool_button_new(refresh_icon, NULL);
  gtk_tool_item_set_tooltip_text(refresh_button, "Refresh (F5)");
  //g_signal_connect (refresh_button, "clicked", G_CALLBACK (refresh_sys_browser), class_browser_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, refresh_button, 1);

  GtkToolItem *close_button = gtk_tool_button_new(exit_icon, NULL);
  gtk_tool_item_set_tooltip_text(close_button, "Close (Ctrl-W)");
  g_signal_connect (close_button, "clicked", G_CALLBACK (close_window), class_browser_window);
  gtk_toolbar_insert((GtkToolbar *)toolbar, close_button, 2);

  return (GtkToolbar *)toolbar;
}

void initialize_packages_list(GtkTreeView *list)
{
  GtkCellRenderer    *renderer;
  GtkTreeViewColumn  *column1;
  GtkListStore       *store;

  renderer = gtk_cell_renderer_text_new();

  column1 = gtk_tree_view_column_new_with_attributes("Packages",
                                                     renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW (list), column1);

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT64);

  gtk_tree_view_set_model(GTK_TREE_VIEW (list), 
                          GTK_TREE_MODEL(store));

  g_object_unref(store);  
}

void initialize_classes_list(GtkTreeView *list)
{
  GtkCellRenderer    *renderer;
  GtkTreeViewColumn  *column1;
  GtkListStore       *store;

  renderer = gtk_cell_renderer_text_new();

  column1 = gtk_tree_view_column_new_with_attributes("Classes",
                                                     renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW (list), column1);

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT64);
  
  gtk_tree_view_set_model(GTK_TREE_VIEW (list), 
                          GTK_TREE_MODEL(store));

  g_object_unref(store);  
}

void initialize_methods_list(GtkTreeView *list)
{
  GtkCellRenderer    *renderer;
  GtkTreeViewColumn  *column1;
  GtkListStore       *store;

  renderer = gtk_cell_renderer_text_new();

  column1 = gtk_tree_view_column_new_with_attributes("Methods",
                                                     renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW (list), column1);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT64);
  
  gtk_tree_view_set_model(GTK_TREE_VIEW (list), 
                          GTK_TREE_MODEL(store));

  g_object_unref(store);  
}

void populate_packages_list()
{
  remove_all_from_list(packages_list);

  GtkListStore *store;
  GtkTreeIter  iter;

  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(packages_list)));

  class_object_t *pkg_cls_object = (class_object_t *)extract_ptr(Package);

  OBJECT_PTR packages = car(get_binding(pkg_cls_object->shared_vars, get_symbol("packages")));

  object_t *pkgs_obj_int = (object_t *)extract_ptr(packages);

  OBJECT_PTR arr = car(get_binding(pkgs_obj_int->instance_vars, get_symbol("arr")));
  OBJECT_PTR size = car(get_binding(pkgs_obj_int->instance_vars, get_symbol("size")));

  array_object_t *arr_obj = (array_object_t *)extract_ptr(arr);
  
  int i, nof_packages;

  nof_packages = get_int_value(size);

  for(i=0; i<nof_packages; i++)
  {
    gtk_list_store_append(store, &iter);

    //TODO: display the package hierarchy

    OBJECT_PTR pkg = arr_obj->elements[i];
    object_t *pkg_obj_int = (object_t *)extract_ptr(pkg);
    
    OBJECT_PTR pkg_name = car(get_binding(pkg_obj_int->instance_vars, get_symbol("name")));
    
    gtk_list_store_set(store, &iter, 0, GC_strdup(g_string_literals[pkg_name >> OBJECT_SHIFT]), -1);  
    gtk_list_store_set(store, &iter, 1, pkg, -1);
  }
}

void set_up_class_browser_source_buffer()
{
  class_browser_source_buffer = gtk_source_buffer_new_with_language(source_language);
  class_browser_source_view = (GtkSourceView *)gtk_source_view_new_with_buffer(class_browser_source_buffer);

  //TODO: is the code in set_up_workspace_source_buffer() to be replicated here?
}

void fetch_classes_for_package(GtkWidget *list, gpointer selection1)
{
  GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(packages_list)));
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (packages_list));
  GtkTreeIter  iter;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packages_list));

  if(!selection)
    return;

  remove_all_from_list(methods_list);
  
  action_triggering_window = class_browser_window;
  
  /* if(!check_for_sys_browser_changes()) */
  /* { */
  /*   revert_to_prev_pkg_sym(); */
  /*   return; */
  /* } */
  
  if(gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gint64 id;

    gtk_tree_model_get(model, &iter,
                       1, &id,
                       -1);

    //TODO: check if this is relevant
    //print_context_pkg_index = id;

    remove_all_from_list(classes_list);

    GtkListStore *store2;
    GtkTreeIter  iter2;

    store2 = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(classes_list)));

    int i, n;

    n = g_top_level->count;

    for(i=0; i<n; i++)
    {
      if(g_top_level->bindings[i].key == SELF ||
	 g_top_level->bindings[i].key == SUPER ||
	 g_top_level->bindings[i].key == THIS_CONTEXT)
	continue;

      OBJECT_PTR binding_val = g_top_level->bindings[i].val;

      if(IS_CLASS_OBJECT(car(binding_val)))
      {
	class_object_t *cls_obj_int = (class_object_t *)extract_ptr(car(binding_val));

	if(cls_obj_int->package == id)
	{
	  gtk_list_store_append(store2, &iter2);
	  gtk_list_store_set(store2, &iter2, 0, cls_obj_int->name, -1);  
	  gtk_list_store_set(store2, &iter2, 1, car(binding_val), -1);
	}
      }
    }

    OBJECT_PTR ret = message_send(g_msg_snd_closure,
				  id,
				  NIL,
				  get_symbol("_getQualifiedName"),
				  convert_int_to_object(0),
				  g_idclo);
        
    char code[200];
    memset(code, '\0', 200);
    unsigned int len = 0;
    len += sprintf(code+len, "Smalltalk createClass: #SomeClass\n");
    len += sprintf(code+len, "  parentClass: SomeParentClass\n");
    len += sprintf(code+len, "  instanceVars: #(#var1 #var2)\n");
    len += sprintf(code+len, "  classVars: #(#var3 #var4)\n");
    len += sprintf(code+len, "  inPackage: '%s'", g_string_literals[ret >> OBJECT_SHIFT]);

    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(class_browser_source_buffer), code, -1);
    //gtk_statusbar_remove_all(class_browser_statusbar, 0);
  }

  gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(class_browser_source_buffer), FALSE);
}

void fetch_methods_for_class(GtkWidget *list, gpointer selection1)
{
  GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(classes_list)));
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (classes_list));
  GtkTreeIter  iter;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(classes_list));

  if(!selection)
    return;

  action_triggering_window = class_browser_window;
  
  /* if(!check_for_sys_browser_changes()) */
  /* { */
  /*   revert_to_prev_pkg_sym(); */
  /*   return; */
  /* } */
  
  if(gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gint64 id;

    gtk_tree_model_get(model, &iter,
                       1, &id,
                       -1);

    //TODO: check if this is relevant
    //print_context_pkg_index = id;

    remove_all_from_list(methods_list);

    GtkListStore *store2;
    GtkTreeIter  iter2;

    store2 = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(methods_list)));

    assert(IS_CLASS_OBJECT(id));

    class_object_t *cls_obj = (class_object_t *)extract_ptr(id);

    int i, n;

    // populate class creation code
    char str[500];
    memset(str, '\0', 500);
    unsigned int len = 0;

    len += sprintf(str+len, "Smalltalk createClass: #%s\n", cls_obj->name);

    OBJECT_PTR parent_cls_obj = get_class_object(cls_obj->parent_class_object);
    if(id != Object)
    {
      class_object_t *parent_cls_obj_int = (class_object_t *)extract_ptr(cls_obj->parent_class_object);
      len += sprintf(str+len, "  parentClass: %s\n", parent_cls_obj_int->name);
    }
    else
      len += sprintf(str+len, "  parentClass: nil\n");

    len += sprintf(str+len, "  instVars : #(");

    n = cls_obj->nof_instance_vars;

    for(i=0; i<n; i++)
    {
      len += sprintf(str+len, "#%s", get_symbol_name(cls_obj->inst_vars[i]));
      if(i != (n-1))
	len += sprintf(str+len, " ");
    }
    len += sprintf(str+len, ")\n");

    len += sprintf(str+len, "  classVars : #(");

    binding_env_t *shared_vars = cls_obj->shared_vars;

    if(shared_vars)
    {
      n = shared_vars->count;

      for(i=0; i<n; i++)
      {
	len += sprintf(str+len, "#%s", get_symbol_name(shared_vars->bindings[i].key));
	if(i != (n-1))
	  len += sprintf(str+len, " ");
      }
    }

    len += sprintf(str+len, ")\n");

    OBJECT_PTR pkg_obj = cls_obj->package;
    object_t *pkg_obj_int = (object_t *)extract_ptr(pkg_obj);
    char *pkg_name = g_string_literals[car(get_binding(pkg_obj_int->instance_vars, get_symbol("name"))) >> OBJECT_SHIFT];

    len += sprintf(str+len, "  inPackage: \'%s'\n", pkg_name);

    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(class_browser_source_buffer), "", -1);
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(class_browser_source_buffer), str, -1);
    //

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(class_radio_button)))
    {
      n = cls_obj->class_methods->count;

      for(i=0; i<n; i++)
      {
	OBJECT_PTR key = cls_obj->class_methods->bindings[i].key;
	OBJECT_PTR val = cls_obj->class_methods->bindings[i].val;

	gtk_list_store_append(store2, &iter2);
	char *method_name = get_symbol_name(key);
	gtk_list_store_set(store2, &iter2, 0, substring(method_name, 1, strlen(method_name)-1), 1, val, -1);  
      }
    }
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(instance_radio_button)))
    {
      n = cls_obj->instance_methods->count;

      for(i=0; i<n; i++)
      {
	OBJECT_PTR key = cls_obj->instance_methods->bindings[i].key;
	OBJECT_PTR val = cls_obj->instance_methods->bindings[i].val;

	gtk_list_store_append(store2, &iter2);
	char *method_name = get_symbol_name(key);
	gtk_list_store_set(store2, &iter2, 0, substring(method_name, 1, strlen(method_name)-1), 1, val, -1);  
      }
    }
    else
      assert(false);
  }

 //gtk_statusbar_remove_all(class_browser_statusbar, 0);

  gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(class_browser_source_buffer), FALSE);
}

void fetch_code_for_method(GtkWidget *list, gpointer selection1)
{
  GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(methods_list)));
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (methods_list));
  GtkTreeIter  iter;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(methods_list));

  if(!selection)
    return;

  action_triggering_window = class_browser_window;
  
  /* if(!check_for_sys_browser_changes()) */
  /* { */
  /*   revert_to_prev_pkg_sym(); */
  /*   return; */
  /* } */
  
  if(gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gint64 val;
    gchar *name = NULL;

    gtk_tree_model_get(model, &iter,
                       0, &name, 1, &val,
                       -1);

    //TODO: check if this is relevant
    //print_context_pkg_index = val;

    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(class_browser_source_buffer), "", -1);
    
    OBJECT_PTR method = val;

    method_t *m = (method_t *)extract_ptr(method);

    if(m->code_str != NIL)
    {
      char header[200];
      memset(header, '\0', 200);

      sprintf(header,
	      "Smalltalk %s #%s toClass: %s withBody:\n",
	      m->class_method ? "addClassMethod:" : "addInstanceMethod:",
	      //substring(sym, 1, strlen(sym) - 1),
	      name,
	      m->cls_obj->name);

      gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(class_browser_source_buffer), header, -1);

      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pretty_printed_radio_button)))
      {
	int indents = 0;
	//the fourth argument (index) is relevant only when render_executable_code()
	//is called for the debugger, so it is set to a dummy value here.
	//This argument is needed for highlighting method calls in the context of
	//the debugger.
	render_executable_code(GTK_TEXT_BUFFER(class_browser_source_buffer), &indents, false, -1, m->exec_code);
      }
      else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(raw_radio_button)))
      {
	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(class_browser_source_buffer),
				       g_string_literals[m->code_str >> OBJECT_SHIFT], -1);
      }
    }
    else
      gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(class_browser_source_buffer),
				       "<primitive method>", -1);
  }

  gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(class_browser_source_buffer), FALSE);
}

void create_class_browser_window(int posx, int posy, int width, int height)
{
  GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  class_browser_window = (GtkWindow *)win;

  gtk_window_set_icon_from_file(class_browser_window, SMALLTALKDATADIR "/icons/evaluate.png", NULL);

  GtkWidget *scrolled_win1, *scrolled_win2, *scrolled_win3;
  GtkWidget *vbox, *hbox;

  gtk_window_set_title((GtkWindow *)win, "Class Browser");

  gtk_window_set_position((GtkWindow *)win, GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_default_size(class_browser_window, width, height);
  gtk_window_move(class_browser_window, posx, posy); 

  g_signal_connect (win, "delete-event",
                    G_CALLBACK (delete_event), NULL);

  g_signal_connect (win, "focus",
                    G_CALLBACK (set_triggering_window), NULL);

  g_signal_connect(G_OBJECT(win), 
                   "key_press_event", 
                   G_CALLBACK (handle_key_press_events), 
                   NULL);

  gtk_container_set_border_width (GTK_CONTAINER (win), 10);

  scrolled_win1 = gtk_scrolled_window_new(NULL, NULL);
  scrolled_win2 = gtk_scrolled_window_new(NULL, NULL);
  scrolled_win3 = gtk_scrolled_window_new(NULL, NULL);

  packages_list = (GtkTreeView *)gtk_tree_view_new();
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(packages_list), TRUE);
  gtk_widget_override_font(GTK_WIDGET(packages_list), pango_font_description_from_string(FONT));

  initialize_packages_list((GtkTreeView *)packages_list);

  g_signal_connect(G_OBJECT(packages_list), "cursor-changed",
                   G_CALLBACK(fetch_classes_for_package), NULL);

  gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(packages_list, 0), 0); 

  populate_packages_list();

  classes_list = (GtkTreeView *)gtk_tree_view_new();
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(classes_list), TRUE);
  gtk_widget_override_font(GTK_WIDGET(classes_list), pango_font_description_from_string(FONT));

  initialize_classes_list((GtkTreeView *)classes_list);
  gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(classes_list, 0), 0);

  g_signal_connect(G_OBJECT(classes_list), "cursor-changed",
                   G_CALLBACK(fetch_methods_for_class), NULL);
  
  methods_list = (GtkTreeView *)gtk_tree_view_new();
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(methods_list), TRUE);
  gtk_widget_override_font(GTK_WIDGET(methods_list), pango_font_description_from_string(FONT));

  initialize_methods_list((GtkTreeView *)methods_list);
  gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(methods_list, 0), 0);

  g_signal_connect(G_OBJECT(methods_list), "cursor-changed",
                   G_CALLBACK(fetch_code_for_method), NULL);
  
  gtk_container_add(GTK_CONTAINER (scrolled_win1), (GtkWidget *)packages_list);
  gtk_container_add(GTK_CONTAINER (scrolled_win2), (GtkWidget *)classes_list);
  gtk_container_add(GTK_CONTAINER (scrolled_win3), (GtkWidget *)methods_list);

  GtkWidget *method_box, *radio1, *radio2, *radio_box;

  method_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  radio_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  radio1 = gtk_radio_button_new_with_label(NULL, "Class Methods");
  radio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1),
						       "Instance Methods");

  g_signal_connect(G_OBJECT(radio1), "toggled",
                   G_CALLBACK(fetch_methods_for_class), NULL);
  
  g_signal_connect(G_OBJECT(radio2), "toggled",
                   G_CALLBACK(fetch_methods_for_class), NULL);
  
  class_radio_button = radio1;
  instance_radio_button = radio2;
  
  gtk_box_pack_start(GTK_BOX(radio_box), radio1, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(radio_box), radio2, FALSE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(method_box), radio_box, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(method_box), scrolled_win3, TRUE, TRUE, 0);
  
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
  gtk_box_pack_start(GTK_BOX (hbox), scrolled_win1, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX (hbox), scrolled_win2, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX (hbox), method_box, TRUE, TRUE, 0);

  GtkWidget *scrolled_win, *radio3, *radio4, *radio_box2, *code_box;

  set_up_class_browser_source_buffer();

  GtkWidget *textview = (GtkWidget *)class_browser_source_view;

  gtk_widget_override_font(GTK_WIDGET(class_browser_source_view), pango_font_description_from_string(FONT));

  /* g_signal_connect(G_OBJECT(class_browser_buffer),  */
  /*                  "notify::cursor-position",  */
  /*                  G_CALLBACK (handle_cursor_move),  */
  /*                  NULL); */

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET(class_browser_source_view));

  code_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  radio_box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  radio3 = gtk_radio_button_new_with_label(NULL, "Pretty Printed");
  radio4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio3),
						       "Raw");
  g_signal_connect(G_OBJECT(radio3), "toggled",
                   G_CALLBACK(fetch_code_for_method), NULL);
  
  g_signal_connect(G_OBJECT(radio4), "toggled",
                   G_CALLBACK(fetch_code_for_method), NULL);

  pretty_printed_radio_button = radio3;
  raw_radio_button = radio4;
  
  gtk_box_pack_start(GTK_BOX(radio_box2), radio3, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(radio_box2), radio4, FALSE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(code_box), radio_box2, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(code_box), scrolled_win, TRUE, TRUE, 0);
  
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start (GTK_BOX (vbox), (GtkWidget *)create_class_browser_toolbar(), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), code_box, TRUE, TRUE, 0);

  /* class_browser_statusbar = (GtkStatusbar *)gtk_statusbar_new(); */
  /* gtk_box_pack_start (GTK_BOX (vbox), (GtkWidget *)class_browser_statusbar, FALSE, FALSE, 0);   */

  gtk_container_add (GTK_CONTAINER (win), vbox);

  gtk_widget_show_all(win);
}
