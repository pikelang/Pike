signal changed;
signal delete_text;
signal insert_text;

void select_region(int start, int end)
//! Selects a region of text.
{
  pgtk_verify_inited();
  {
    int start,end;
    get_all_args("select_region",args,"%i%i",&start,&end);
    gtk_editable_select_region(GTK_EDITABLE(THIS->obj),start,end);
  }
  RETURN_THIS();
}

array get_selection_bounds()
//! Returns the selection bounds.
{
  pgtk_verify_inited();
  {
    int start,end;
    gtk_editable_get_selection_bounds(GTK_EDITABLE(THIS->obj),&start,&end);
    my_pop_n_elems(args);
    PGTK_PUSH_INT(start);
    PGTK_PUSH_INT(end);
  }
}

int insert_text(string text, int length, int pos)
//! Inserts text at a given position.  Returns the position after the new text.
{
  pgtk_verify_inited();
  {
    char *text;
    int len,pos;
    get_all_args("insert_text",args,"%s%i%i",&text,&len,&pos);
    gtk_editable_insert_text(GTK_EDITABLE(THIS->obj),text,len,&pos);
    my_pop_n_elems(args);
    PGTK_PUSH_INT(pos);
  }
}

void delete_text(int start, int end)
//! Deletes a sequence of characters.
{
  pgtk_verify_inited();
  {
    int start,end;
    get_all_args("delete_text",args,"%i%i",&start,&end);
    my_pop_n_elems(args);
    gtk_editable_delete_text(GTK_EDITABLE(THIS->obj),start,end);
  }
  RETURN_THIS();
}

string get_chars(int start, int end)
//! Retrieves a sequence of characters.
{
  pgtk_verify_inited();
  {
    gchar *s;
    int start,end;
    get_all_args("get_chars",args,"%i%i",&start,&end);
    my_pop_n_elems(args);
    s=gtk_editable_get_chars(GTK_EDITABLE(THIS->obj),start,end);
    PGTK_PUSH_GCHAR(s);
    g_free(s);
  }
}

void cut_clipboard()
//! Causes the characters in the current selection to be copied to the
//! clipboard and then deleted.
{
  pgtk_verify_inited();
  gtk_editable_cut_clipboard(GTK_EDITABLE(THIS->obj));
  RETURN_THIS();
}

void copy_clipboard()
//! Causes the characters in the current selection to be copied to the
//! clipboard.
{
  pgtk_verify_inited();
  gtk_editable_copy_clipboard(GTK_EDITABLE(THIS->obj));
  RETURN_THIS();
}

void paste_clipboard()
//! Causes the contents of the clipboard to be pasted into the given widget at
//! the current cursor position.
{
  pgtk_verify_inited();
  gtk_editable_paste_clipboard(GTK_EDITABLE(THIS->obj));
  RETURN_THIS();
}

void delete_selection()
//! Deletes the current contents of the selection.
{
  pgtk_verify_inited();
  gtk_editable_delete_selection(GTK_EDITABLE(THIS->obj));
  RETURN_THIS();
}

void set_position(int pos)
//! Sets the cursor position.
{
  pgtk_verify_inited();
  {
    int pos;
    get_all_args("set_position",args,"%i",&pos);
    gtk_editable_set_position(GTK_EDITABLE(THIS->obj),pos);
  }
  RETURN_THIS();
}

int get_position()
//! Retrieves the current cursor position.
{
  pgtk_verify_inited();
  {
    int pos=gtk_editable_get_position(GTK_EDITABLE(THIS->obj));
    my_pop_n_elems(args);
    PGTK_PUSH_INT(pos);
  }
}

void set_editable(int setting)
//! Determines if the user can edit the text or not.
{
  pgtk_verify_inited();
  {
    int ed;
    get_all_args("set_editable",args,"%i",&ed);
    gtk_editable_set_editable(GTK_EDITABLE(THIS->obj),ed);
  }
  RETURN_THIS();
}

int get_editable()
//! Retrieves whether this widget is editable.
{
  pgtk_verify_inited();
  {
    int ed=gtk_editable_get_editable(GTK_EDITABLE(THIS->obj));
    my_pop_n_elems(args);
    PGTK_PUSH_INT(ed);
  }
}
