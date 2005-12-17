signal sort_column_changed;

void sort_column_changed()
//! Emits a "sort-column-changed" signal.
{
  pgtk_verify_inited();
  gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(THIS->obj));
  RETURN_THIS();
}

mapping(string:int) get_sort_column_id()
//! Returns ([ "column": int, "order": int ])
{
  pgtk_verify_inited();
  {
    int col;
    GtkSortType order;
    int res=gtk_tree_sortable_get_sort_column_id(
			GTK_TREE_SORTABLE(THIS->obj),&col,&order);
    my_pop_n_elems(args);
    push_text("column"); PGTK_PUSH_INT(col);
    push_text("order"); PGTK_PUSH_INT(order);
    f_aggregate_mapping(4);
  }
}

void set_sort_column_id(int column, int order)
//! Sets the current sort column to be column.  The widget will resort itself
//! to reflect this change, after emitting a "sort-column-changed" signal.
//! If column is GTK2.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, then the default
//! sort function will be used, if it is set.
{
  pgtk_verify_inited();
  {
    INT_TYPE col,order;
    get_all_args("set_sort_column_id",args,"%i%i",&col,&order);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(THIS->obj),
			col,order);
  }
  RETURN_THIS();
}

void set_sort_func(int column, function f, mixed user_data)
//! Sets the comparison function used when sorting to be f.  If the current
//! sort column id of this sortable is the same as column, then the model will
//! sort using this function.
{
  pgtk_verify_inited();
  {
    struct svalue *sv1,*sv2;
    INT_TYPE col;
    struct signal_data *sd;
    get_all_args("set_sort_func",args,"%i%*%*",&col,&sv1,&sv2);
    sd=(struct signal_data *)g_malloc(sizeof(struct signal_data));
    if (sd==NULL)
      SIMPLE_OUT_OF_MEMORY_ERROR("set_sort_func",sizeof(struct signal_data));
    assign_svalue_no_free(&sd->cb,sv1);
    assign_svalue_no_free(&sd->args,sv2);
    gtk_tree_sortable_set_sort_func(
			GTK_TREE_SORTABLE(THIS->obj),col,
			(GtkTreeIterCompareFunc)pgtk_tree_sortable_callback,sd,
			(GtkDestroyNotify)pgtk_free_signal_data);
  }
  RETURN_THIS();
}

void set_default_sort_func(function f, mixed user_data)
//! Sets the default comparison function used when sorting to be f.  If the
//! current sort column id of this sortable is 
//! GTK2.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, then the model will sort using
//! this function.
//! <p>
//! if f is 0, then there will be no default comparison function.  This means
//! once the model has been sorted, it can't go back to the default state.  In
//! this case, when the current sort column id of this sortable is
//! GTK2.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, the model will be unsorted.
{
  pgtk_verify_inited();
  {
    struct svalue *sv1,*sv2;
    struct signal_data *sd;
    get_all_args("set_default_sort_func",args,"%*%*",&sv1,&sv2);
    sd=(struct signal_data *)g_malloc(sizeof(struct signal_data));
    if (sd==NULL)
      SIMPLE_OUT_OF_MEMORY_ERROR("set_default_sort_func",sizeof(struct signal_data));
    assign_svalue_no_free(&sd->cb,sv1);
    assign_svalue_no_free(&sd->args,sv2);
    gtk_tree_sortable_set_default_sort_func(
			GTK_TREE_SORTABLE(THIS->obj),
			(GtkTreeIterCompareFunc)pgtk_tree_sortable_callback,sd,
			(GtkDestroyNotify)pgtk_free_signal_data);
  }
  RETURN_THIS();
}

int has_default_sort_func()
//! Returns true if the model has a default sort function.
{
  pgtk_verify_inited();
  my_pop_n_elems(args);
  PGTK_PUSH_INT(gtk_tree_sortable_has_default_sort_func(
			GTK_TREE_SORTABLE(THIS->obj)));
}
