function write;

object get_widget_from( string what )
{
  array err;
  mixed res;
  err = catch {
    res= compile_string( "object foo(){ return "+what+"; }")()->foo();
  };
  if(!res)
    werror("Error while compiling or running "+what+"\n: %s\n", err?err[0]:"No return value");
  return res;
}

string file_name(string from)
{
  return ((MIME.encode_base64((Crypto.md5()->update(from)->
			       digest()),1)-"=")-"/")[..40]
    +".gif";
}

object ex;
string source;
int start_indent;
array(int) istack = ({});

void push_indent()
{
  istack += ({ start_indent });
}

void pop_indent()
{
  if(sizeof(istack)) start_indent = istack[-1];
  istack = istack[..sizeof(istack)-2];
}

string indent(string what, int|void limit)
{
  if(!limit) limit=40;
  string res="";
  string line="";
  int nobr;
  for(int i=0;i<strlen(what); i++)
  {
    line += what[i..i];
    switch(what[i])
    {
     case '(': push_indent(); start_indent=strlen(line); break;
     case '"': nobr = !nobr; break;
     case ')': pop_indent(); if(!(strlen(what)>i+1 && what[i+1]=='-')) break;
     case '}': pop_indent(); res += line+"\n"; line=" "*start_indent; break;
     case '{': push_indent(); start_indent += 2;
     case ';': res += line+"\n"; line=" "*start_indent; break;
     case ' ':
     case ',':
     case '>':
       if(!nobr)
       {
	 if(strlen(line) >= limit)
	 {
	   if(strlen(line) > limit)
	   {
	     if(limit < 50) limit=strlen(line);
	   }
	   res += line+"\n";
	   line = " "*start_indent;
	 }
       }
    }
  }
  return res+line;
}

string tags(string source)
{
  if( !wmml )
  {
    return ("<table cellpadding=1 bgcolor=black><tr><td><table bgcolor=#e0e0e0 cellpadding=8 cellspacing=0 border=0><tr><td><img src="+file_name(source)+">"
            "</td></td><tr><td><font size=-1><pre>"+
            replace(indent(source), ({"<",">","&"}), ({"&lt;", "&gt;", "&amp;"}))+
            "</pre></font></td></tr></table></td></tr></table>");
  } else {
    return ("<table framed=1><tr><td><img src=gtkimg/"+file_name(source)+">"
            "</td></tr><tr><td><font size=-1>"
            "<example language=pike>"+
            replace(indent(source), ({"<",">","&"}), ({"&lt;", "&gt;", "&amp;"}))+
            "</example></font></td></tr></table>");
  }
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

  mkdir(wmml?"wmml/gtkimg":"docs");
  rm((wmml?"wmml/gtkimg/":"docs/")+file_name(source));
  function fun;
#if constant(Image.GIF.encode)
  Stdio.write_file((wmml?"wmml/gtkimg/":"docs/")+
                   file_name(source), Image.GIF.encode(i));
#else
  Stdio.write_file((wmml?"wmml/gtkimg/":"docs/")+
                   file_name(source), i->togif_fs());
#endif
  write(tags( source ));

  exit(0);
}

void got_event( int q, object w, mapping  e )
{
  if(e->type == "expose" )
    call_out(grab, 0.01, w );
}

void show_recursively(object w)
{
  if(w->children)
    Array.map( w->children(), show_recursively );
  w->show();
}

int main(int argc, array (string) argv)
{
  object w;
  werror("IMAGE ["+argv[1]+"]\n");
  if(!write) write = Stdio.stdout->write;
  source = argv[1];
  wmml = (argv[3] == "WMML");

  if(file_stat( (wmml?"wmml/gtkimg/":"docs/")+file_name(argv[1])))
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
    w = get_widget_from( argv[1] );
    w->set_border_width( 20 );
  }
  show_recursively( w );
  w->signal_connect( "event", got_event );
  return -1;
}
