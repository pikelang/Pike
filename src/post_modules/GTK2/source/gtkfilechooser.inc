//! Properties:
//! int action
//! GTK2.Widget extra-widget
//! string file-system-backend
//! GTK2.FileFilter filter
//! int local-only
//! GTk.Widget preview-widget
//! int preview-widget-active
//! int select-multiple
//! int show-hidden
//! int use-preview-label

signal current_folder_changed;
signal file_activated;
signal selection_changed;
signal update_preview;

void set_action(int action)
//! Sets type of action, from opening or saving a file, or 
//! opening or saving a folder.
//! Options are:
//!  CONST(GTK_FILE_CHOOSER_ACTION_OPEN)
//!  CONST(GTK_FILE_CHOOSER_ACTION_SAVE)
//!  CONST(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
//!  CONST(GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
{
  pgtk_verify_inited();
  {
    int action;
    get_all_args("set_action",args,"%i",&action);
    gtk_file_chooser_set_action(GTK_FILE_CHOOSER(THIS->obj),action);
  }
  RETURN_THIS();
}

int get_action()
//! Get the type of action the file chooser is performing.
{
  pgtk_verify_inited();
  {
    int res=gtk_file_chooser_get_action(GTK_FILE_CHOOSER(THIS->obj));
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

void set_local_only(int local_only)
//! Sets whether on local files can be selected.
{
  pgtk_verify_inited();
  {
    int loc;
    get_all_args("set_local_only",args,"%i",&loc);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(THIS->obj),loc);
  }
  RETURN_THIS();
}

int get_local_only()
//! Gets whether only local files can be selected in the file selector.
{
  pgtk_verify_inited();
  {
    int res=gtk_file_chooser_get_local_only(GTK_FILE_CHOOSER(THIS->obj));
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

void set_select_multiple(int select_multiple)
//! Sets whether multiple files can be selected in the file selector.
//! This is only relevant if the action is set to be
//! CONST(GTK_FILE_CHOOSER_ACTION_OPEN) or
//! CONST(GTK_FILE_CHOOSER_ACTION_SAVE)
{
  pgtk_verify_inited();
  {
    int sel;
    get_all_args("set_select_multiple",args,"%i",&sel);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(THIS->obj),sel);
  }
  RETURN_THIS();
}

int get_select_multiple()
//! Gets whether multiple files can be selected.
{
  pgtk_verify_inited();
  {
    int res=gtk_file_chooser_get_select_multiple(GTK_FILE_CHOOSER(THIS->obj));
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

require gtk26;
void set_show_hidden(int setting)
//! Sets whether hidden files and folders are displayed.
{
  pgtk_verify_inited();
  {
    int set;
    get_all_args("set_show_hidden",args,"%i",&set);
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(THIS->obj),set);
  }
  RETURN_THIS();
}

int get_show_hidden()
//! Gets whether hidden files and folders are displayed.
{
  pgtk_verify_inited();
  {
    int res=gtk_file_chooser_get_show_hidden(GTK_FILE_CHOOSER(THIS->obj));
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}
endrequire;

void set_current_name(string filename)
//! Sets current name.
{
  pgtk_verify_inited();
  {
    char *s;
    get_all_args("set_current_name",args,"%s",&s);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(THIS->obj),s);
  }
  RETURN_THIS();
}

string get_filename()
//! Gets the filename for the currently select file.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    gchar *file=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(THIS->obj));
    PGTK_PUSH_GCHAR(file);
    g_free(file);
  }
}

int set_filename(string filename)
//! Sets filename as the current filename.
//! Same as unselect_all() followed by select_filename();
{
  pgtk_verify_inited();
  {
    int res;
    char *file;
    get_all_args("set_filename",args,"%s",&file);
    res=gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(THIS->obj),file);
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

int select_filename(string filename)
//! Selects a filename.
{
  pgtk_verify_inited();
  {
    int res;
    char *file;
    get_all_args("select_filename",args,"%s",&file);
    res=gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(THIS->obj),file);
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

void unselect_filename(string filename)
//! Unselects a currently selected filename.
{
  pgtk_verify_inited();
  {
    char *file;
    get_all_args("unselect_filename",args,"%s",&file);
    gtk_file_chooser_unselect_filename(GTK_FILE_CHOOSER(THIS->obj),file);
  }
  RETURN_THIS();
}

void select_all()
//! Select all files.
{
  pgtk_verify_inited();
  gtk_file_chooser_select_all(GTK_FILE_CHOOSER(THIS->obj));
  RETURN_THIS();
}

void unselect_all()
//! Unselect all files.
{
  pgtk_verify_inited();
  gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(THIS->obj));
  RETURN_THIS();
}

array(string) get_filenames()
//! Get a list of all selected file and subfolders.
//! The returned names are full absolute paths.
//! If files in the current folder cannot be represented as local
//! filenames, they will be ignored.  If you want those files,
//! You may want to use get_uris() instead.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    GSList *gs1=gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(THIS->obj));
    GSList *gs2=gs1;
    int i=0;
    while (gs2) {
      i++;
      PGTK_PUSH_GCHAR(gs2->data);
      g_free(gs2->data);
      gs2=g_slist_next(gs2);
    }
    f_aggregate(i);
    g_slist_free(gs1);
  }
}

int set_current_folder(string folder)
//! Sets the current folder from a local filename.  The user
//! will be shown the full contents of the current folder, plus
//! user interface elements for navigating to other folders.
{
  pgtk_verify_inited();
  {
    int res;
    char *fold;
    get_all_args("set_current_folder",args,"%s",&fold);
    res=gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(THIS->obj),fold);
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

string get_current_folder()
//! Gets the current folder as a local filename.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    gchar *s;
    s=gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(THIS->obj));
    PGTK_PUSH_GCHAR(s);
    g_free(s);
  }
}

string get_uri()
//! Gets the URI for the currently selected file.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    gchar *s;
    s=gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(THIS->obj));
    PGTK_PUSH_GCHAR(s);
    g_free(s);
  }
}

int set_uri(string uri)
//! Sets the uri as the current file.
{
  pgtk_verify_inited();
  {
    int res;
    char *uri;
    get_all_args("set_uri",args,"%s",&uri);
    res=gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(THIS->obj),uri);
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

int select_uri(string uri)
//! Selects the file by uri.
{
  pgtk_verify_inited();
  {
    int res;
    char *uri;
    get_all_args("select_uri",args,"%s",&uri);
    res=gtk_file_chooser_select_uri(GTK_FILE_CHOOSER(THIS->obj),uri);
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

void unselect_uri(string uri)
//! Unselect the uri.
{
  pgtk_verify_inited();
  {
    char *uri;
    get_all_args("unselect_uri",args,"%s",&uri);
    gtk_file_chooser_unselect_uri(GTK_FILE_CHOOSER(THIS->obj),uri);
  }
  RETURN_THIS();
}

array(string) get_uris()
//! List all the selected files and subfolders in the current folder.
//! The returned names are full absolute URIs.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    GSList *gs1=gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(THIS->obj));
    GSList *gs2=gs1;
    int i=0;
    while (gs2) {
      i++;
      PGTK_PUSH_GCHAR(gs2->data);
      g_free(gs2->data);
      gs2=g_slist_next(gs2);
    }
    f_aggregate(i);
    g_slist_free(gs1);
  }
}

int set_current_folder_uri(string uri)
//! Sets the current folder from a URI.
{
  pgtk_verify_inited();
  {
    int res;
    char *uri;
    get_all_args("set_current_folder_uri",args,"%s",&uri);
    res=gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(THIS->obj),uri);
    my_pop_n_elems(args);
    PGTK_PUSH_INT(res);
  }
}

string get_current_folder_uri()
//! Gets the current folder as a URI.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    gchar *s=gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(THIS->obj));
    PGTK_PUSH_GCHAR(s);
    g_free(s);
  }
}

void set_extra_widget(GTK.Widget extra)
//! Sets an application-supplied widget to provide extra
//! options to the user.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    get_all_args("set_extra_widget",args,"%o",&o1);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(THIS->obj),
			GTK_WIDGET(get_gobject(o1)));
  }
  RETURN_THIS();
}

void set_preview_widget(GTK2.Widget widget)
//! Sets an application-supplied widget to use to display a custom preview of
//! the currently selected file.  To implement a preview, after setting the
//! preview widget, you connect to the "update-preview" signal, and call
//! get_preview_filename() or get_preview_uri() on each change.  If you can
//! display a preview of the new file, update your widget and set the preview
//! active using set_preview_widget_active().  Otherwise, set the preview
//! inactive.
//! <p>
//! When there is no application-supplied preview widget, or the application-
//! supplied preview widget is not active, the file chooser may display an
//! internally generated preview of the current file or it may display no
//! preview at all.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    get_all_args("set_preview_widget",args,"%o",&o1);
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(THIS->obj),
			GTK_WIDGET(get_gobject(o1)));
  }
  RETURN_THIS();
}

GTK2.Widget get_preview_widget()
//! Gets the current preview widget.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    GtkWidget *widget;
    widget=gtk_file_chooser_get_preview_widget(GTK_FILE_CHOOSER(THIS->obj));
    push_gobject(widget);
  }
}

void set_preview_widget_active(int setting)
//! Sets whether the preview widget set by set_preview_widget() should be shown
//! for the current filename.  When setting is set to false, the file chooser
//! may display an internally generated preview of the current file or it may
//! display no preview at all.
{
  pgtk_verify_inited();
  {
    int set;
    get_all_args("set_preview_widget_active",args,"%i",&set);
    gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(THIS->obj),set);
  }
  RETURN_THIS();
}

int get_preview_widget_active()
//! Gets whether the preview widget set by set_preview_widget_active() should
//! be shown for the current filename.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    int res=gtk_file_chooser_get_preview_widget_active(
			GTK_FILE_CHOOSER(THIS->obj));
    PGTK_PUSH_INT(res);
  }
}

void set_use_preview_label(int setting)
//! Sets whether the file chooser should display a stock label with the name
//! of the file that is being previewed; the default is true.  Applications
//! that want to draw the whole preview area themselves should set this to
//! false and display the name themselves in their preview widget.
{
  pgtk_verify_inited();
  {
    int set;
    get_all_args("set_use_preview_label",args,"%i",&set);
    gtk_file_chooser_set_use_preview_label(GTK_FILE_CHOOSER(THIS->obj),set);
  }
  RETURN_THIS();
}

int get_use_preview_label()
//! Gets whether a stock label should be drawn with the name of the previewed
//! file.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    int res=gtk_file_chooser_get_use_preview_label(GTK_FILE_CHOOSER(THIS->obj));
    PGTK_PUSH_INT(res);
  }
}

string get_preview_filename()
//! Gets the filename that should be previewed in a custom preview widget.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    char *file=gtk_file_chooser_get_preview_filename(GTK_FILE_CHOOSER(THIS->obj));
    PGTK_PUSH_GCHAR(file);
    g_free(file);
  }
}

string get_preview_uri()
//! Gets the URI that should be previewed in a custom preview widget.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    char *uri=gtk_file_chooser_get_preview_uri(GTK_FILE_CHOOSER(THIS->obj));
    PGTK_PUSH_GCHAR(uri);
    g_free(uri);
  }
}

void set_extra_widget(GTK2.Widget widget)
//! Sets an application-supplied widget to provide extra options to the user.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    get_all_args("set_extra_widget",args,"%o",&o1);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(THIS->obj),
			GTK_WIDGET(get_gobject(o1)));
  }
  RETURN_THIS();
}

GTK2.Widget get_extra_widget()
//! Gets the extra widget.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    GtkWidget *widget=gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(THIS->obj));
    push_gobject(widget);
  }
}

/* TODO: *shortcut_folder* *filter* */
/*
void add_filter(GTK2.FileFilter filter);
//! Adds filter to the list of filters that the user can select between.  When
//! a filter is selected, only files that are passed by that filter are
//! displayed.

void remove_filter(GTK2.FileFilter filter);
//! Removes filter from the list of filters that the user can select between.

array(GTK2.FileFilter) list_filters()
//! Lists the current set of user-selectable filters.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  {
    GSList *gsl,*gs2;
    int i=0;
    gsl=gs2=gtk_file_chooser_list_filters(GTK_FILE_CHOOSER(THIS->obj));
    while (gs2) {
      push_gobject(gs2->data);
      i++;
      gs2=g_slist_next(gs2);
    }
    f_aggregate(i);
    g_slist_free(gsl);
  }
}

void set_filter(GTK2.FileFilter filter);
//! Sets the current filter; only the files that pass the filter will be
//! displayed.  If the user-selectable list of filters is non-empty, then the
//! filter should be one of the filters in that list.  Setting the current
//! filter when the list of filters is empty is useful if you want to restrict
//! the displayed set of files without letting the user change it.

GTK2.FileFilter get_filter();
//! Gets the current filter.
*/

int add_shortcut_folder(string folder)
//! Adds a folder to be displayed with the shortcut folders in a file chooser.
//! Note that shortcut folders do not get saved, as they are provided by the
//! application.  For example, you can use this to add a 
//! "/usr/share/mydrawprogram/Clipart" folder to the volume list.
{
  pgtk_verify_inited();
  {
    const char *folder;
    int res;
    get_all_args("add_shortcut_folder",args,"%s",&folder);
    res=gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(THIS->obj),
			folder,NULL);
    my_pop_n_elems(args);
    push_int(res);
  }
}

void remove_shortcut_folder(string folder)
//! Removes a folder from the list of shortcut folders.
{
  pgtk_verify_inited();
  {
    const char *folder;
    get_all_args("remove_shortcut_folder",args,"%s",&folder);
    gtk_file_chooser_remove_shortcut_folder(GTK_FILE_CHOOSER(THIS->obj),
			folder,NULL);
  }
  RETURN_THIS();
}

array(string) list_shortcut_folders()
//! Queries the list of shortcut folders.
{
  pgtk_verify_inited();
  {
    GSList *gsl,*gs2;
    int i=0;
    gsl=gs2=gtk_file_chooser_list_shortcut_folders(GTK_FILE_CHOOSER(THIS->obj));
    while (gs2) {
      PGTK_PUSH_GCHAR(gs2->data);
      i++;
      g_free(gs2->data);
      gs2=g_slist_next(gs2);
    }
    f_aggregate(i);
    g_slist_free(gsl);
  }
}

int add_shortcut_folder_uri(string uri)
//! Adds a folder URI to be displayed with the shortcut folders.
{
  pgtk_verify_inited();
  {
    const char *uri;
    int res;
    get_all_args("add_shortcut_folder_uri",args,"%s",&uri);
    res=gtk_file_chooser_add_shortcut_folder_uri(GTK_FILE_CHOOSER(THIS->obj),
			uri,NULL);
    my_pop_n_elems(args);
    push_int(res);
  }
}

void remove_shortcut_folder_uri(string uri)
//! Removes a folder URI.
{
  pgtk_verify_inited();
  {
    const char *uri;
    get_all_args("remove_shortcut_folder_uri",args,"%s",&uri);
    gtk_file_chooser_remove_shortcut_folder_uri(GTK_FILE_CHOOSER(THIS->obj),
			uri,NULL);
  }
  RETURN_THIS();
}

array(string) list_shortcut_folder_uris()
//! Queries the list of shortcut folders.
{
  pgtk_verify_inited();
  {
    GSList *gsl,*gs2;
    int i=0;
    gsl=gs2=gtk_file_chooser_list_shortcut_folder_uris(GTK_FILE_CHOOSER(THIS->obj));
    while (gs2) {
      PGTK_PUSH_GCHAR(gs2->data);
      i++;
      g_free(gs2->data);
      gs2=g_slist_next(gs2);
    }
    f_aggregate(i);
    g_slist_free(gsl);
  }
}

