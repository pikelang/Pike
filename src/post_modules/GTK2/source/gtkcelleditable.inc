signal editing_done;
signal remove_widget;

void start_editing(GDK2.Event event)
//! Begins editing.  event is the GDK2.Event that began the editing process.
//! It may be empty, in the instance that editing was initiated through
//! programmatic means.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    get_all_args("start_editing",args,"%o",&o1);
    gtk_cell_editable_start_editing(GTK_CELL_EDITABLE(THIS->obj),
		(GdkEvent *)get_gdkobject(o1,event));
  }
  RETURN_THIS();
}

void editing_done()
//! Emits the "editing-done" signal.  This signal is a sign for the cell
//! renderer to update its value from the cell.
{
  pgtk_verify_inited();
  gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(THIS->obj));
  RETURN_THIS();
}

void remove_widget()
//! Emits the "remove-widget" signal.  This signal is meant to indicate that
//! the cell is finished editing, and the widget may now be destroyed.
{
  pgtk_verify_inited();
  gtk_cell_editable_remove_widget(GTK_CELL_EDITABLE(THIS->obj));
  RETURN_THIS();
}
