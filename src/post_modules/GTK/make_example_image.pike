function write;
string source;

object get_widget_from( string what )
{
  array err;
  mixed res;
  err = catch
  {
    if( search(what, "return") == -1 || (search(what,"lambda")!=-1) )
      res= compile_string( "object foo(){ return "+what+"; }")()->foo();
    else
      res= compile_string( "object foo(){ "+what+" }")()->foo();
  };
  if(!res)
    werror("Error while compiling or running "+what+"\n: %s\n",
           err?err[0]:"No return value");
  return res;
}

string tags( string imgdata)
{
  return sprintf( "@code{%s@}\n"
                  "@xml{<image-data>%{&#%03d;%}</image-data>@}\n",
                  source,(array(int))imgdata );
}

void grab(object w, string s)
{
  object i = GTK.GdkImage();
  if(w->xsize() == 40 || w->ysize() == 40 )
  {
    werror("Image dimensions are 0x0!\n");
    exit(1);
  }
  i->grab( w, 20,20, w->xsize()-40, w->ysize()-40);
  write(tags( Image.PNG.encode(Image.PNM.decode( i->get_pnm() )) ));
  exit(0);
}

void got_event( mixed a, mixed b, object e )
{
  call_out(grab, 0.1, w );
}

void show_recursively(object w)
{
  if(w->children)
    Array.map( w->children(), show_recursively );
  w->show();
}

object w, ex;
int main(int argc, array (string) argv)
{
  werror("IMAGE ["+argv[1]+"]\n");
  if(!write)
    write = Stdio.stdout.write;
  source = argv[1];

  if(search( argv[1], "Gnome" ) != -1 )
    Gnome.init( "example", "1.0", ({ "example image" }),0 );
  else
    GTK.setup_gtk( "make_gtkexample", 1 );

  if(argv[2] != "TOP")
  {
    w = GTK.Window( GTK.WindowToplevel );
    w->set_title("Example image generation");
    w->add( ex=get_widget_from( argv[1] ) );
    w->set_border_width(20);
  }
  else
  {
    ex = w = get_widget_from( argv[1] );
    w->set_border_width( 20 );
  }
  w->signal_connect( "map_event", got_event, w );
  w->signal_connect( "destroy", _exit, 1 );
  show_recursively( w );
  return -1;
}
