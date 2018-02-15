void drag_data_get( int q, object w, mixed context, 
                    object selection, int a, int b)
{
  selection->set("Some data for the drop to receive\n");
}

void drag_data_received( int q, object w, mixed context,
                         int x, int y, object selection, 
                         int type, int time )
{
  werror("got DND type %s\nData is %s\n",
	 selection->type()->get_name(),
	 selection->data());
}

int main(int argc, array argv)
{
  GTK.setup_gtk(argv);

  object drag_icon = GDK.Pixmap( Image.Image( 64,64 )->test() );

  object window = GTK.Window( GTK.WindowToplevel );
  window->signal_connect( "destroy", exit, 0 );
  object table = GTK.Hbox( 0,0 );
  window->add( table );

  object to = GTK.Button("Drop here!");
  object from = GTK.Button("Drag here!");
  
  to->drag_dest_set( GTK.DestDefaultAll, ({
                      ({ "STRING", 0, 0 }),
                      ({ "text/plain", 0, 0 }),
                     }),
		     GDK.ActionCopy|GDK.ActionMove|GDK.ActionDefault|
		     GDK.ActionLink|GDK.ActionPrivate|GDK.ActionAsk);

  to->signal_connect( "drag_data_received", drag_data_received );

  from->drag_source_set( GDK.Button1Mask|GDK.Button2Mask,
                         ({ ({ "STRING", 0, 0 }),
                            ({ "text/plain", 0, 0 }),
                            ({ "application/x-rootwin-drop", 0, 1 }),
                         }),
                         GDK.ActionCopy|GDK.ActionMove );
  from->drag_source_set_icon( drag_icon, 0 );
  from->signal_connect( "drag_data_get", drag_data_get );

  table->add( from );
  table->add( to );
  window->show_all();
  return -1;
}
