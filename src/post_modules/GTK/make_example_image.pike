function write;

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
    werror("Error while compiling or running "+what+"\n: %s\n", err?err[0]:"No return value");
  return res;
}

string file_name(string from)
{
  return ((MIME.encode_base64((Crypto.md5()->update(from)->
			       digest()),1)-"=")-"/")[..40]+".png";
}

object ex;
string source;

string tags(string source)
{
  return ("<box>\n<img src=gtkimg/"+file_name(source)+" />"
          "<br />\n"
          "<example language=pike>"+
          replace(source,({"<",">","&"}),({"&lt;", "&gt;", "&amp;"}))+
          "</example>\n</box>\n\n");
}

int wmml;
void grab(object w)
{
  object i = GTK.GdkImage();
  if(w->xsize() == 40 || w->ysize() == 40 )
  {
    werror("Image dimensions are 0x0!\n");
    exit(1);
  }
  i->grab( w, 20,20, w->xsize()-40, w->ysize()-40);
  object i = Image.PNM.decode( i->get_pnm() );

  mkdir(wmml?"wmml/gtkimg":"xmldocs");
  rm((wmml?"wmml/gtkimg/":"xmldocs/")+file_name(source));
  function fun;
  Stdio.write_file((wmml?"wmml/gtkimg/":"xmldocs/")+
                   file_name(source), Image.PNG.encode(i));
  write(tags( source ));

  exit(0);
}

void got_event( mixed a, mixed b, object e )
{
  call_out(grab, 0.3, w );
}

void show_recursively(object w)
{
  if(w->children)
    Array.map( w->children(), show_recursively );
  w->show();
}

object w;
int main(int argc, array (string) argv)
{
  werror("IMAGE ["+argv[1]+"]\n");
  if(!write) write = Stdio.stdout->write;
  source = argv[1];
  wmml = (argv[3] == "WMML");

  if(file_stat( (wmml?"wmml/gtkimg/":"xmldocs/")+file_name(argv[1])))
  {
    write(tags(argv[1]));
    exit(0);
  }
  if(search( argv[1], "Gnome" ) != -1 )
    Gnome.init( "example", "1.0", ({ "example image" }) );
  else
    GTK.setup_gtk( "make_gtkexample", 1 );

  if(argv[2] != "TOP")
  {
    w = GTK.Window( GTK.WINDOW_TOPLEVEL );
    w->set_title("Example image generation");
    w->add( ex=get_widget_from( argv[1] ) );
    w->set_border_width(20);
  } else {
    ex = w = get_widget_from( argv[1] );
    w->set_border_width( 20 );
  }
  w->signal_connect( "map_event", got_event, w );
  w->signal_connect( "destroy", _exit, 1 );
  show_recursively( w );
  return -1;
}
