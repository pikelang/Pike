string html_encode_string(string str)
{
  return replace(str, ({"&", "<", ">", "\"", "\'", "\000" }),
		 ({"&amp;", "&lt;", "&gt;", "&#34;", "&#39;", "&#0;"}));
}

string make_tag_attributes(mapping(string:string) in)
{
  if(!in || !sizeof(in)) return "";
  int sl=0;
  string res=" ";
#ifdef MODULE_DEBUG
  array(string) s=sort(indices(in));
  foreach(s, string a) {
#else
  foreach(indices(in), string a) {
#endif
    if(a=="/" && in[a]=="/")
      sl=1;
    else
      res+=a+"=\""+html_encode_string((string)in[a])+"\" ";
  }
  if(sl) return res+"/";
  return res[..sizeof(res)-2];
}

string make_tag(string s, mapping(string:string) in)
{
  return "<"+s+make_tag_attributes(in)+">";
}

string make_container(string s, mapping(string:string) in, string contents)
{
  if(in["/"]) m_delete(in, "/");
  return make_tag(s,in)+contents+"</"+s+">";
}


string class_file_name( string c )
{
  return lower_case( classname(c) )+".xml";
}

string link_class( string c )
{
  return make_container( "a",(["href":class_file_name( c )]),classname( c ) );
}

string handle_type( string q, int link )
{
  if( search( q, "(" ) != -1 )
    return link?"":q;
  switch( q )
  {
   case "array":
   case "string":
   case "mapping":
   case "object":
   case "multiset":
   case "int":
   case "float":
     return link?"":q;
   default:
     return link?class_file_name( q ):classname( q );
  }
}

string make_args( string from )
{
  string res="";
  foreach( (from  / ",")-({""}), string f )
  {
    string a;
    f = String.trim_whites( f );
    if(  sscanf( f, "%s %s", a, f ) != 2 )
    {
      a = f;
      f = "";
    }
    mapping q =([ "/":"/", 
                  "name":f,
                  "href":handle_type( a,1 ),
                  "type":handle_type( a,0 ) ]);
    if( q->href="" )
      m_delete( q, "href" );
    res += make_tag( "arg", q )+"\n";
  }
  return res;
}

void write_method( object fd, string fun, string ret, 
                   string arg, string doc, string linkto )
{
  mapping q = ([ ]);
  if( linkto )
    q->href = class_file_name( linkto )+"#"+fun;
  q->name = fun;
  q->returns = ret;
  fd->write( make_container( "method",
                             q,
                             "\n"+make_args( arg )+
                             make_container( "doc", ([]), doc||"" ) )+"\n" );
}

void print_inherited_functions( string cl, object fd )
{
  foreach(sort(indices(struct[cl]))-({"inherit"}), string fun)
  {
    string a,b;
    string a = true_types[cl+fun][1],b=true_types[cl+fun][0];
    if( (a-" ") == "void"  )
      a = "";
    write_method( fd, fun, b, a, "", cl );
  }

  if(struct[cl]["inherit"])
    print_inherited_functions( struct[cl]["inherit"], fd );
}

void print_signals( mapping sigs, object on, string|void p)
{
  if(!sigs) 
    return;
  foreach(sort(indices(sigs)), string s)
    on->write(make_container( "signal",
                              ([ "name":s, ]),
                              make_container( "doc",([]),sigs[s] ) ) );
}

void print_inherited_signals( string cl, object fd )
{
  print_signals( signals[cl], fd, cl );
  if(struct[cl]["inherit"])
    print_inherited_signals( struct[cl]["inherit"], fd );
}

void print_function_defs( string f, int|void global )
{
  rm("xmldocs/"+f+".xml");
  if(f == "global")
    global = 1;

  object fd = Stdio.File("xmldocs/"+class_file_name( f ), "wct");

  if( struct[f]->create )
  {
    if( warn->noexample )
      if( !examples[ lower_case(f) ]  )
        werror(lines[f]+": Warning: No example image\n");
    fd->write( make_tag( "class", (["name":classname( f ), ]) ) );
  } else
    fd->write( make_tag( "class", (["name":classname( f ), "nocreate":""]) ) );

  fd->write( make_container( "doc", ([]), docs[f]||"" ) );

  if(!global)
  {
    if(string i = struct[f]["inherit"])
      fd->write( make_tag( "inherit", ([ "/":"/",
                                         "href":class_file_name( i ),
                                         "class":classname( i ) ])) );

    foreach(sort(indices(struct)), string w)
      if(struct[w]["inherit"] == f)
      fd->write( make_tag( "inherited", ([ "/":"/",
                                          "href":class_file_name( f ),
                                          "class":classname( f ) ])) );
  }
  if(sizeof(indices(struct[f])) > 1)
  {
    if(struct[f]->create)
    {
      fd->write("<constructor>\n");
      string a = true_types[f+"create"][1];
      if( (a-" ") == "void"  )
        a = "";
      if( warn->noargs )
        if(strlen(a) && !named[ f + "create" ] )
          werror(lines[ f + "create" ]+": Warning: Arguments not named\n");
      write_method(fd, classname( f ), classname( f ), a, docs[f+"create"],0);
      if( warn->nodocs )
        if(!docs[f+"create"])
          werror(lines[ f + "create" ]+
                 ": Warning: No documentation for constructor\n");
      fd->write("</constructor>\n");
    }
    fd->write("<methods>");
    foreach(sort(indices(struct[f])-({"inherit","create","destroy"})),
            string fun)
    {
      string a = true_types[f+fun][1],b=true_types[f+fun][0];
      if( (a-" ") == "void"  )
        a = "";
      if( warn->noargs )
        if(strlen(a) && !named[ f + fun ] )
          werror(lines[f+fun]+": Warning: Arguments not named\n");

      write_method( fd, fun, b, a, docs[f+fun],0);

      if(!docs[f+fun] || !strlen(docs[f+fun]))
      {
        if( warn->nodocs )
          werror(lines[ f + fun ]+
                 ": Warning: No documentation for "+fun+"\n");
      }
      else
      {
        if( warn->cstyle )
        {
          if( search(docs[f+fun], "GTK_") != -1)
            werror(lines[ f + fun ]+
                   ": Warning: Possible C-style GTK constant name\n");
          if( search(docs[f+fun], "gtk_") != -1)
            if(search( docs[f+fun], "C-") == -1)
              werror(lines[ f + fun ]+
                     ": Warning: Possible C-style GTK function name\n");
        }
      }
    }
    if(struct[f]["inherit"])
    {
      fd->write("<inherited>");
      print_inherited_functions( struct[f]["inherit"], fd );
      fd->write("</inherited>");
    }
    fd->write("</methods>");
  }
  if(signals[f] && sizeof(signals[f]))
  {
    fd->write("<signals>");
    print_signals( signals[f], fd, f );
    fd->write("<inherited>");
    if(struct[f]["inherit"])
      print_inherited_signals( struct[f]["inherit"], fd );
    fd->write("</inherited>");
    fd->write("</signals>");
  }
  fd->write("</class>\n");
  fd->close();
}

void print_rec_tree(array plane, mapping t, int ind, object to)
{
  foreach(sort(plane), string n)
  {
    print_function_defs( n );
//  werror(" "*ind + n+"\n");
    to->write("<li><a href="+class_file_name(n)+">"+classname(n)+"</a>");
    if(t[n])
    {
      to->write("<ul>\n");
      print_rec_tree(sort(t[n]), t, ind+1, to);
      to->write("</ul>\n");
    }
  }
}

int fnamesfun( string a, string b )
{
  return (classname(a) > classname(b));
}


int do_docs;
array (string) sort_dependencies( array bunch, mapping extra )
{
  mapping inheriting = ([]);
  foreach(bunch, string s)
  {
    if(inheriting[extra[s]["inherit"]])
      inheriting[extra[s]["inherit"]] += ({ s });
    else
      inheriting[extra[s]["inherit"]] = ({ s });
  }
  array (string) result = sort(inheriting[0]);

  if(do_docs )
  {
    multiset roots=
      mkmultiset(indices(inheriting))-mkmultiset(`+(@values(inheriting)));;

    mkdir("docs");
    rm("xmldocs/index.html");
    object fd = Stdio.File("xmldocs/index.html", "wc");
    fd->write("<a href=gtk.global.xml><h1>Global functions</h1></a>\n");
    fd->write("<h1>Pike GTK Inheritance tree</h1>\n");
    fd->write("<ul>");
//     print_function_defs( "global", 1 );
    foreach(sort(indices(roots)), string r)
      if(r != "global")
        print_rec_tree( inheriting[r], inheriting, 1, fd);
    fd->write("</ul>");
    foreach( sort(indices(requires)), string req )
    {
      fd->write("<h1>All classes requiring "+req+"</h1>\n" );
      fd->write("<ul>");
      foreach(Array.sort_array(requires[req], fnamesfun), string s)
        fd->write("<li> <a href="+class_file_name(s)+
                  ">"+classname(s)+"</a>\n");
      fd->write("</ul>\n");
    }
    fd->write("<h1>All classes in alphabetical order</h1>\n");
    fd->write("<ul>");
    foreach(Array.sort_array(indices(struct), fnamesfun), string s)
      if(s != "global")
        fd->write("<li> <a href="+class_file_name(s)+">"+
                  classname(s)+"</a>\n");
    fd->write("</ul>\n");
//     fd->write("<h1>All constants in alphabetical order</h1>\n");
//     fd->write("<ul>");
//     array consts =constants_name;
//     foreach(Array.sort_array(consts,fnamesfun), string s)
//       fd->write("<li> "+classname(String.capitalize(lower_case(s)))+"\n");
//     fd->write("</ul>");
  }
  m_delete(inheriting, 0);
  while(sizeof(inheriting))
  {
    int mod;
    foreach(result, string s)
    {
      if(inheriting[s])
      {
	result += sort(inheriting[s]);
	m_delete(inheriting, s);
	mod++;
      }
    }
    if(!mod)
    {
      werror("Inconsistent inheritance tree!\n");
      multiset roots=mkmultiset(indices(inheriting))-mkmultiset(`+(@values(inheriting)));;
      werror("orphans:\n");
      foreach(indices(roots), string r)
      {
	if(extra[r])
	  werror("\n+ "+r+"   [ "+extra[r]["inherit"]+"]\n");
	else
	  werror("\n+ "+r+" UNDEFINED\n");
	print_rec_tree( inheriting[r], inheriting, 1, Stdio.stdout);
      }
      exit(1);
    }
  }
 return result;
}

string PIKE;

void find_pike()
{
  PIKE="../../pike -DNOT_INSTALLED -m../../master.pike";
}

string make_example_image(string from, int top)
{
  object mei;
  if(!mei) mei = (object)("make_example_image.pike");

  if(file_stat( "xmldocs/"+mei->file_name(from)))
    return mei->tags( from );

  if(!PIKE)
    find_pike();

  string res=Process.popen(PIKE+" "+dirname(__FILE__)+
                           "/make_example_image.pike '"+from+"'"+
                           (top?" TOP":" POT") + " XML");
  if(!strlen(res))
    werror("Failed to make example image from '"+from+"'\n");
  return res;
}

int in_img, img_toplevel;
string delay_slot="", normal_slot="";

string handle_img(string line)
{
  if(line=="toplevel")
    img_toplevel=1;
  else if(sscanf(line, "delay: %s", line))
    delay_slot += line+"\n";
  else if(line == "img_end" || sscanf(line, "img_end%*s"))
  {
    string code = ("lambda() {\n"+"GDK.GC g;\nGTK.Widget w;\n"+ normal_slot+"\n");
    if(strlen(delay_slot))
      code += "call_out(lambda(object w,object g) {\n"+delay_slot+"}, 0.05, w,g);\n";
    code+="return w;\n}()";
    delay_slot=normal_slot="";
    in_img = 0;
    return make_example_image(code,img_toplevel);
  } else
    normal_slot += line+"\n";
  return "";
}

array constants_name = ({});
string find_constants(string prefix, string where)
{
  array res = ({});
  sscanf(prefix, "GTK_%s", prefix );
  foreach(constants_name, string c)
    if(search(c,prefix) != -1)
      res += ({ classname(String.capitalize(lower_case(c))) });

  if(!sizeof(res))
  {
    werror("Fatal error: "+where+": CONST("+prefix+") in doc string: No consts found\n");
    exit(1);
  }
  return String.implode_nicely( res );
}


int odd;
string build_cursor( string a )
{
  int num=0;
  sscanf( a, "GDK.%s", a );
  num = master()->resolv("GDK")[a];

  return "<tr bgcolor="+(odd++%2?"#f6e0d0":"#faebd7")+
         "><td><font color=black>GDK."+a+"</font></td>"
         "<td><img src=cursor_"+num+".gif></td>"
         "<td><img src=cursor_"+num+"_inv.gif></td>"
         "<td><img src=cursor_"+num+"_red.gif></td>"
         "<td><img src=cursor_"+num+"_red_inv.gif></td>"
         "<td><img src=cursor_"+num+"_green.gif></td>"
         "<td><img src=cursor_"+num+"_green_inv.gif></td>"
         "<td><img src=cursor_"+num+"_blue.gif></td>"
         "<td><img src=cursor_"+num+"_blue_inv.gif></td>"
         "</tr>";
}

string unsillycaps(string what)
{
  sscanf( what, "Gtk%s", what );
  string res=upper_case(what[0..0]);
  foreach(what[1..]/"", string q)
    if(lower_case(q)==q)
      res += q;
    else
      res += "_"+lower_case(q);
  return replace(res,"__","_");
}
