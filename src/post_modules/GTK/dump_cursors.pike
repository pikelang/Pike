object da, fn, gc, ia, w;
string prefix;

#define GRAB(i) Image.PNM.decode(ia->grab(w,4,4+(i*80),60,60)->get_pnm())

void grab_2( int x )
{
  call_out( grab, 0.001, x+2 );

  object l = Image.Layer( GRAB(0), GRAB(1)->invert() );
  l = l->autocrop();
  
  mkdir("cursors");
  Stdio.File( prefix+x+".gif", "wct")->
    write(Image.GIF.encode_trans( l->image(),l->alpha() ) );


  Stdio.File( prefix+x+"_red.gif", "wct")->
    write(Image.GIF.encode_trans( l->image()->change_color(Image.Color.black,
                                                           Image.Color.red),
                                  l->alpha() ) );

  Stdio.File( prefix+x+"_green.gif", "wct")->
    write(Image.GIF.encode_trans( l->image()->change_color(Image.Color.black,
                                                           Image.Color.green),
                                  l->alpha() ) );

  Stdio.File( prefix+x+"_blue.gif", "wct")->
    write(Image.GIF.encode_trans( l->image()->change_color(Image.Color.black,
                                                           Image.Color.blue),
                                  l->alpha() ) );


  Stdio.File( prefix+x+"_red_inv.gif", "wct")->
    write(Image.GIF.encode_trans( l->image()->change_color(Image.Color.black,
                                                           Image.Color.red)
                                  ->change_color(Image.Color.white,
                                                 Image.Color.black),
                                  l->alpha() ) );

  Stdio.File( prefix+x+"_green_inv.gif", "wct")->
    write(Image.GIF.encode_trans( l->image()->change_color(Image.Color.black,
                                                           Image.Color.green)
                                  ->change_color(Image.Color.white,
                                                 Image.Color.black),
                                  l->alpha() ) );

  Stdio.File( prefix+x+"_blue_inv.gif", "wct")->
    write(Image.GIF.encode_trans( l->image()->change_color(Image.Color.black,
                                                           Image.Color.blue)
                                  ->change_color(Image.Color.white,
                                                 Image.Color.black),
                                  l->alpha() ) );

  Stdio.File( prefix+x+"_inv.gif", "wct")->
    write(Image.GIF.encode_trans( l->image()->invert(),l->alpha() ) );

}

void grab( int x )
{
  if( x > 152 ) _exit(0);
  if(!file_stat( prefix+x+".gif" ) )
  {
    if(!da)
    {
      GTK.setup_gtk();
      w = GTK.Window( GTK.WindowToplevel );
      w->add( da = GTK.DrawingArea( ) );
      da->set_usize( 100,400 );
      w->show_all();
      GTK.flush();
      fn = GDK.Font( "cursor" );
      gc = GDK.GC( da );
      ia = GDK.Image( );
      da->set_app_paintable( 1 );
      da->set_background( GDK.Color( @(array)Image.Color.white ) );
    }
    da->clear();
    da->draw_text( gc, fn, 30,30, sprintf("%c", x ) );
    da->draw_text( gc, fn, 30,80+30, sprintf("%c", x+1 ) );
    call_out( grab_2, 0.01, x );
  } else
     grab( x+2 );
}

int main(int c, array argv)
{
  prefix = argv[1];
  grab(0);
  return -1;
}
