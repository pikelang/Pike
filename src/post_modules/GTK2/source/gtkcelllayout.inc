require gtk24;

void pack_start(GTK2.CellRenderer cell, int expand)
//! Packs the cell into the beginning.  If expand is false, then the cell is
//! allocated no more space than it needs.  Any unused space is divied evenly
//! between the cells for which expand is true.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    int expand;
    get_all_args("pack_start",args,"%o%i",&o1,&expand);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(THIS->obj),
                GTK_CELL_RENDERER(get_gobject(o1)),expand);
  }
  RETURN_THIS();
}

void pack_end(GTK2.CellRenderer cell, int expand)
//! See pack_start().
{
  pgtk_verify_inited();
  {
    struct object *o1;
    int expand;
    get_all_args("pack_end",args,"%o%i",&o1,&expand);
    gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(THIS->obj),
                GTK_CELL_RENDERER(get_gobject(o1)),expand);
  }
  RETURN_THIS();
}

void reorder(GTK2.CellRenderer cell, int position)
//! Re-inserts cell at position.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    int pos;
    get_all_args("reorder",args,"%o%i",&o1,&pos);
    gtk_cell_layout_reorder(GTK_CELL_LAYOUT(THIS->obj),
                GTK_CELL_RENDERER(get_gobject(o1)),pos);
  }
  RETURN_THIS();
}

void clear()
//! Unsets all the mappings on all renderers and removes all renderers.
{
  pgtk_verify_inited();
  gtk_cell_layout_clear(GTK_CELL_LAYOUT(THIS->obj));
  RETURN_THIS();
}

void add_attribute(GTK2.CellRenderer cell, string attribute, int column)
//! Adds an attribute mapping.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    char *att;
    int col;
    get_all_args("add_attribute",args,"%o%s%i",&o1,&att,&col);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(THIS->obj),
                GTK_CELL_RENDERER(get_gobject(o1)),att,col);
  }
  RETURN_THIS();
}

void clear_attributes(GTK2.CellRenderer cell)
//! Clears all existing attributes.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    get_all_args("clear_attributes",args,"%o",&o1);
    gtk_cell_layout_clear_attributes(GTK_CELL_LAYOUT(THIS->obj),
                GTK_CELL_RENDERER(get_gobject(o1)));
  }
  RETURN_THIS();
}


endrequire;
