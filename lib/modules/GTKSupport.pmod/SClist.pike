#pike __REAL_VERSION__

//! A @[Clist] with scrollbars. Defines the same interface as
//! @[Clist].

//! @ignore

inherit GTK.ScrolledWindow;

GTK.Clist cl;
void create( mixed ... args )
{
  ::create( 0, 0 );
  cl = GTK.Clist( @args );
  add( cl->show() );
}

#define W(X) mixed X(mixed ... args) { return cl->X(@args); }

/* inherited: object */
W(signal_connect); W(signal_emit_stop); W(signal_disconnect);
W(signal_block);   W(signal_unblock); 

/* inherited: widget */
W(text_width); W(set_background); W(add_accelerator); W(remove_accelerator);
W(set_state); W(set_sensitive); 

W(get_style); W(ensure_style);

/* inherited: container */
W(children); W(drag_dest_unset); W(drag_highlight); W(drag_unhighlight);
W(drag_dest_set); W(drag_source_set); W(drag_source_unset); 
W(drag_source_set_icon);

/* inherited: clist */
W(sort);
W(set_hadjustment); W(set_vadjustment); W(get_hadjustment); 
W(get_vadjustment); W(set_shadow_type); W(set_selection_mode); 
W(set_reorderable); W(set_use_drag_icons); W(set_button_actions); 
W(freeze); W(thaw); W(column_titles_show); W(column_titles_hide); 
W(column_titles_active); W(column_titles_passive); W(column_title_active); 
W(column_title_passive); W(set_column_title); W(get_column_title); 
W(set_column_widget); W(get_column_widget); W(set_column_visibility); 
W(set_column_resizeable); W(set_column_auto_resize); W(columns_autosize);
W(optimal_column_width); W(set_column_justification); W(set_column_width); 
W(set_column_min_width); W(set_column_max_width); W(set_row_height); 
W(moveto); W(get_cell_type); W(set_text); W(get_text); W(set_pixmap); 
W(set_pixtext); W(set_foreground); W(set_cell_style); 
W(get_cell_style); W(set_row_style); W(get_row_style); W(set_shift); 
W(set_selectable); W(get_selectable); W(append); W(prepend); W(insert); 
W(remove); W(set_row_data); W(get_row_data); W(find_row_from_data);
W(select_row); W(unselect_row); W(undo_selection); W(clear); 
W(get_selection_info); W(select_all); W(unselect_all); W(row_move); 
W(get_foreground); W(get_background); W(set_compare_func); W(get_selection); 
W(set_sort_type); W(set_sort_column); W(set_auto_sort); W(get_shadow_type); 
W(get_selection_mode); W(get_drag_button); W(get_focus_row); W(get_sort_type); 
W(get_sort_column); W(get_flags); W(get_rows); W(get_row_height); 
W(get_row_center_offset); W(get_columns);

//! @endignore
