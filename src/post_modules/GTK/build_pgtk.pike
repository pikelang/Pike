int ln, eml=-100;
string buffer="";

mapping warn = 
([
  "nodocs":1,
  "noargs":1,
  "noexample":1,
  "cstyle":1,
]);

mapping struct = ([ "global":([]) ]);
mapping signals = ([  ]);
string glob_prototypes="";
string line_id = "";

int last_ended_with_nl, debug_mode=1, no_hashline;
void emit_nl(string what)
{
  if(!no_hashline && debug_mode)
  {
    buffer += (last_ended_with_nl?line_id:"")
      + (what/"\n") * ("\n"+line_id) + "\n";
    last_ended_with_nl = 1;
  } else
    buffer += what;
}

string classname( string what )
{
  if(what[0] == 'p' && upper_case(what[1..1])==what[1..1])
    return "GTK."+what;
  string base = "GTK.";
  sscanf(what, "%s.", what);
  if(sscanf(what, "Gdk_%s", what) ||
     sscanf(what, "Gdk%s", what))
    base = "GDK.";
  if(what == "Atom")
    return "GDK._Atom";
  return base+sillycaps( what, 1 );
}

string sillycaps( string what, int|void nolower )
{
  string a;
  if(what[0]=='_') return what;
  if(!nolower) 
    what = lower_case(what);
  what = String.capitalize(what);
  while(sscanf(what, "%s_%s", a, what) == 2)
    what = a+String.capitalize(what);
  if(nolower) return what;
  return "Gtk"+what;
}

void emit_proto(string what)
{
  emit(what);
  glob_prototypes += replace(what,"\n", ";\n");
}

string files="", head;
string internal_progname = "globals";
void end_last_program()
{ 
  eml=-1;
  files += "pgtk_"+internal_progname+".c ";
  if(do_docs) return;
//   werror(internal_progname+"...\n");
  string data;  
//   if(Stdio.file_size("pgtk_"+internal_progname+".c") !=
//      (strlen(buffer)+strlen(head)))
//   {
  if(Stdio.read_bytes( "pgtk_"+internal_progname+".c" ) !=
     head+buffer )
  {
    werror("Creating pgtk_"+internal_progname+".c\n");
    object outf = Stdio.File("pgtk_"+internal_progname+".c", "rwct"); 
    outf->write(head);
    outf->write(buffer);
  }
//     werror("Not modified: pgtk_"+internal_progname+".c\n");
//   }
  buffer="";
}

void begin_new_program(string which)
{
  internal_progname = which;
}

int current_expected_line, current_line;
string emit_line_id;
void emit(string what)
{
  if(!no_hashline && debug_mode)
  {
    if( what[0..4] == "#line" )
    {
      line_id = what;
      if((int)what[5..] != current_line)
        emit_line_id = what;
      else
        emit_line_id = "";
      current_line = current_expected_line = (int)what[5..];
    }
    else if(last_ended_with_nl)
    {
      if(current_line++ == current_expected_line)
      {
        if(sizeof(replace(what, ({" ","\t", "\r", "\n" }),
                          ({"","","",""}))))
          buffer += emit_line_id+what;
        else
          buffer += what;
      }
      else
        buffer += line_id+what;
    }
    else
      buffer += what;
    if(strlen(what))
      last_ended_with_nl = what[-1] == '\n';
  } else
    buffer += what;
}

int _num_functions;
void emit_program_block(mapping block, string cl)
{
  line_id = "";
  foreach(sort(indices(block)), string f)
  {
    _num_functions++;
    if( search( block[f], "()" ) != -1 ||
         search( block[f], "(:" ) != -1 ||
        search( block[f], ":)" ) != -1 ||
        search( block[f], " " ) != -1 ||
        sizeof(block[f]/")") != sizeof( block[f] / "(" ) ||
        ((sizeof(block[f]/")") != sizeof( block[f] / ":" )+
          sizeof(block[f]/"array")-1) &&
         (sizeof(block[f]/")") != sizeof( block[f] / ":" )) ) ||
        search( block[f], "function" ) == -1)
    {
        werror(lines[cl+f]+": Warning: suspicious type "+block[f]+"\n");
    }      
    switch(f)
    {
     case "union":
       emit_nl("   add_function(\"`|\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       emit_nl("   add_function(\"`+\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       break;
     case "intersection":
       emit_nl("   add_function(\"`&\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       break;
     case "subtract":
       emit_nl("   add_function(\"`-\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       break;
     case "equal":
       emit_nl("   add_function(\"`==\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       break;
     case "lt":
       emit_nl("   add_function(\"`<\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       break;
     case "xor":
       emit_nl("   add_function(\"`^\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       break;
     case "not":
       emit_nl("   add_function(\"`~\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       break;
     case "gt":
       emit_nl("   add_function(\"`>\", pgtk_"+cl+"_"+f+",\n"
               "               "+block[f]+", 0);\n");
    _num_functions++;
       break;
    }
    emit_nl("   add_function(\""+f+"\", pgtk_"+cl+"_"+f+",\n"
	    "               "+block[f]+", 0);\n");
  }
}

mapping docs = ([]), true_types = ([]), named = ([]);
mapping lines = ([]), examples = ([]);

void print_inherited_functions( string cl, object fd )
{
  foreach(sort(indices(struct[cl]))-({"inherit"}), string fun)
  {
    string a,b;
    string a = true_types[cl+fun][1],b=true_types[cl+fun][0];
    if( (a-" ") == "void"  )
      a = "";
//     sscanf(struct[cl][fun], "\"function(%s:%s)\"", a, b);
    fd->write("<dt><b>"+b+" <a href="+cl+".html#"+fun+">"+
	      classname(cl)+"."+fun+"</a>"
	      "( "+replace(a, ",", ", ")+" );</b>\n");
  }

  if(struct[cl]["inherit"])
    print_inherited_functions( struct[cl]["inherit"], fd );

}

void print_signals( mapping sigs, object on, string|void p)
{
  if(!sigs) return;
  if(p) 
    p = sillycaps(p,1)+".";
  else 
    p="";
  foreach(sort(indices(sigs)), string s)
    on->write("<dt><b>GTK.s_"+s+"</b><dd>"+sigs[s]+"\n");
}

void print_inherited_signals( string cl, object fd )
{
  print_signals( signals[cl], fd, cl );
  if(struct[cl]["inherit"])
    print_inherited_signals( struct[cl]["inherit"], fd );
}

void print_function_defs( string f, int|void global )
{
  rm("docs/"+f+".html");
  if(f == "global")
    global = 1;
//   if(global)
//     werror("global functions\n");
  object fd = Stdio.File("docs/"+f+".html", "wct");

//   fd->write("<body bgcolor=white text=black><a href=\"\">Index</a><p>\n");



  if(global)
    fd->write("<h1>Global Pike GTK functions</h1><p>\n");
  else if( struct[f]->create )
  {
    if( warn->noexample )
      if( !examples[ lower_case(f) ]  )
        werror(lines[f]+": Warning: No example image\n");
    fd->write("<h1>"+classname(f)+"</h1>\n");
  } else {
    fd->write("<h1>"+classname(f)+" (abstract class)</h1>\n");
  }
  fd->write("<blockquote>"+(docs[f]?docs[f]:"")+"</blockquote>\n");

  if(!global)
  {
    if(struct[f]["inherit"])
      fd->write("Inherits <a href="+struct[f]["inherit"]+".html>"+
                classname( struct[f]["inherit"] )+"</a><p>");

    foreach(sort(indices(struct)), string w)
      if(struct[w]["inherit"] == f)
        fd->write("Inherited by <a href="+w+".html>"+classname(w)+"</a><br>");
  }
  fd->write("<p>");

  if(sizeof(indices(struct[f])) > 1)
  {
    if(struct[f]->create)
    {
      string a = true_types[f+"create"][1];
      if( (a-" ") == "void"  )
        a = "";
      if( warn->noargs )
        if(strlen(a) && !named[ f + "create" ] )
          werror(lines[ f + "create" ]+": Warning: Arguments not named\n");

//       sscanf(struct[f]->create, "\"function(%s:%s)\"", a, b);
      fd->write("<h2>Constructor</h2>\n");
      fd->write("<dl>");
      fd->write("<dt><b>"+
                (global?"GTK":(classname(f)+" "+classname(f)))
                +"( "+replace(a, ",", ", ")+" );</b>\n");
      if( warn->nodocs )
        if(!docs[f+"create"])
          werror(lines[ f + "create" ]+
                 ": Warning: No documentation for constructor\n");

      fd->write("<dd>"+(docs[f+"create"]?docs[f+"create"]:"")+"\n");
      fd->write("</dl>");
    }
    if(!global)
      fd->write("<h2>Methods</h2>\n");
    fd->write("<dl>");
    foreach(sort(indices(struct[f])), string fun)
    {
      if(fun != "inherit")
      {
	string a = true_types[f+fun][1],b=true_types[f+fun][0];
        if( (a-" ") == "void"  )
          a = "";
// 	sscanf(struct[f][fun], "\"function(%s:%s)\"", a, b);
	if(fun == "create" || fun == "destroy")
	  ;
	else
	{
          if( warn->noargs )
            if(strlen(a) && !named[ f + fun ] )
              werror(lines[f+fun]+": Warning: Arguments not named\n");
	  fd->write("<a name=\""+fun+"\"><dt><b>"+b+" "+(global?"GTK.":"")
                    +fun+"( "+replace(a, ",", ", ")+" );</b>\n");
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
	  fd->write("<dd>"+(docs[f+fun]?docs[f+fun]:"")+"</a>\n");
	}
      }
    }
  }

  if(signals[f] && sizeof(signals[f]))
  {
    fd->write("</dl><h2>Signals</h2>\n");
    fd->write("<dl>");
    print_signals( signals[f], fd, f );
  }
  
  if(struct[f]["inherit"])
  {
    fd->write("</dl><h2>Inherited methods</h2><dl>\n");
    print_inherited_functions( struct[f]["inherit"], fd );
    fd->write("</dl><h2>Inherited signals</h2><dl>\n");
    print_inherited_signals( struct[f]["inherit"], fd );
  }
  fd->write("</dl>\n");
  fd->close();
}

void print_rec_tree(array plane, mapping t, int ind, object to)
{
  foreach(sort(plane), string n)
  {
    print_function_defs( n );
//     werror(" "*ind + n+"\n");

    to->write("<li><a href="+n+".html>"+classname(n)+"</a>");
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
  return (classname(String.capitalize(lower_case(a))) >
          classname(String.capitalize(lower_case(b))));
}

int do_docs;
array (string) sort_dependencies( array bunch, mapping extra )
{
  mapping inheriting = ([]);
  foreach(bunch, string s)
  {
    if(inheriting[extra[s]["inherit"]])
      inheriting[extra[s]["inherit"]]+=({ s });
    else
      inheriting[extra[s]["inherit"]]=({ s });
  }
  array (string) result = inheriting[0];

  if(do_docs)
  {
    multiset roots=
      mkmultiset(indices(inheriting))-mkmultiset(`+(@values(inheriting)));;

    mkdir("docs");
    rm("docs/index.html");
    object fd = Stdio.File("docs/index.html", "wc");
    fd->write("<a href=global.html><h1>Global functions</h1></a>\n");
    fd->write("<h1>Pike GTK Inheritance tree</h1>\n");
    fd->write("<ul>");
//     print_function_defs( "global", 1 );
    foreach(sort(indices(roots)), string r)
      if(r != "global")
        print_rec_tree( inheriting[r], inheriting, 1, fd);
    fd->write("</ul>");
    fd->write("<h1>All classes in alphabetical order</h1>\n");
    fd->write("<ul>");
    mapping ltos=mkmapping(Array.map(indices(struct),String.capitalize),
                           indices(struct));
    foreach(Array.sort_array(Array.map(indices(struct), String.capitalize), 
                             fnamesfun), string s)
      if(s != "global")
        fd->write("<li> <a href="+ltos[s]+".html>"+classname(s)+"</a>\n");
    fd->write("</ul>\n");
    fd->write("<h1>All constants in alphabetical order</h1>\n");
    fd->write("<ul>");
    array consts = 
      Array.map(constants/"\n", 
                lambda(string s) {
                  if((sscanf(s, "%*[^\"]\"%s\"", s)==2) && strlen(s))
                    return s;
                }) - ({ 0 });
    
    foreach(Array.sort_array(consts,fnamesfun), string s)
      fd->write("<li> "+classname(String.capitalize(lower_case(s)))+"\n");
    fd->write("</ul>");
  }
  m_delete(inheriting, 0);
  while(sizeof(inheriting))
  {
    int mod;
    foreach(result, string s)
    {
      if(inheriting[s])
      {
	result += inheriting[s];
	m_delete(inheriting, s);
	mod++;
      }
    }
    if(!mod)
    {
      werror("Inconsistend inheritance tree!\n");
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
string constants="";

string PIKE;

void find_pike()
{
  string m = Stdio.read_bytes("config.cache");
  if(!m) 
  {
    werror("Cannot find config.cache!\n");
    exit(0);
  }
  foreach(m/"\n", string l) {
    if(!sscanf(l, "ac_cv_path_PIKE=${ac_cv_path_PIKE='%s'}", PIKE))
      sscanf(l, "ac_cv_path_PIKE=${ac_cv_path_PIKE=%s}", PIKE);
    if(PIKE)
      break;
  }
  if(!PIKE)
  {
    werror("Cannot find pike program cache variable in config.cache file.\n");
    exit(0);
  }
}



string make_example_image(string from, int top)
{
  object mei;
  if(!mei)
    mei = (object)("make_example_image.pike");
  if(file_stat( "docs/"+mei->file_name(from)))
    return mei->tags(from);

  if(!PIKE) 
    find_pike();

  string res=Process.popen(PIKE+" "+dirname(__FILE__)+
                           "/make_example_image.pike '"+from+"'"+
                           (top?" TOP":""));
  if(!strlen(res))
  {
    werror("Failed to make example image from '"+from+"'\n");
  }
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

string find_constants(string prefix)
{
  array res = ({});
  foreach(constants/"\n", string c)
    if(search(c,prefix) != -1)
    {
      sscanf(c, "%*[ \t]%*s\"%s,", c);
      sscanf(c, "GTK_%s", c);
      sscanf(c, "%[^\"]", c);
      res += ({ classname(String.capitalize(lower_case(c))) });
    }

  if(!sizeof(res)) 
  {
    werror("Fatal error: CONST("+prefix+") in doc string: No consts found\n");
    exit(1);
  }
  return String.implode_nicely( res );
}


string gödsla_med_line(string s, string f)
{
  string line = "#line %d \""+f+"\"\n%s\n";
  int l;
  string res="";
  foreach(s/"\n", string d)
    res += sprintf(line, ++l, d );
  return res;
}

string read_indata()
{
  string data = gödsla_med_line(Stdio.read_bytes( dir+"/source/global.pre" ),
                                dir+"/source/global.pre");
  foreach(get_dir( dir+"/source"), string f)
    if( f[-1] == 'c' )
      emit_nl("#line 1 \""+dir+"/source/"+f+"\"\n"+
              Stdio.read_bytes( dir+"/source/"+f ));
    else if(f[-1] == 'e'  && f != "global.pre")
      data += gödsla_med_line(Stdio.read_bytes( dir+"/source/"+f ),
                              dir+"/source/"+f );
  return data;
}

multiset options;
int has_cond_widget( string what )
{
  if(!options)
    options = mkmultiset( (Stdio.read_bytes( "options" )-" ")/"\n" );
//   werror( "has_cond_widget("+what+") -> "+options[what]+"\n");
  return options[what];
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


string dir;
int main(int argc, array argv)
{
  string progname = "global", extra_cpp="";
  string types;
  string last_function;
  int skip_mode;
  do_docs = argc > 2;
  
  foreach( argv[2..], string w )
  {
    if( sscanf( w, "--warn-%s", w ) )
      warn[w] = 1;
    if( sscanf( w, "--no-warn-%s", w ) )
      warn[w] = 0;
  }

  dir = argv[1];
  head=replace(Stdio.read_bytes(dir+"/pgtk.c.head"),"PROTOTYPES","");
  int rl;
  string rf;

#define ERR(X) werror(rf+":"+rl+":  "+X+"\n");
#define NUMBER_FUNCTION() lines[progname+last_function] = rf+":"+rl

  if(array err = catch {

  foreach(read_indata()/"\n", string line)
  {
    string oline = (line-"*");
    string fn, rest;
    ln++;


    if( line[0..4] == "#line" )
      sscanf( line, "#line %d \"%s\"", rl, rf );


    if(sscanf(line, "END_COND_WIDGET%s", line))
    {
      skip_mode=0;
      continue;
    }
    if(skip_mode)
    {
      continue;
    }
    if(sscanf(line, "COND_WIDGET(%s);", line))
    {
      if(!has_cond_widget(line))
        skip_mode=1;
    }
    else if(sscanf(line, "ADD_INCLUDE(%s);", line))
    {
      extra_cpp += line+"\n";
    }
    else if(sscanf(line, "PROGRAM(%[^)]s)", line))
    {
      end_last_program();
      begin_new_program( line );
      last_function="";
      NUMBER_FUNCTION();
      //       werror("found program "+line+"\n");
      progname = line;
      struct[line]= ([]);
      emit("/* "+oline+" */\n");
      emit(" /* Class "+line+" */\n");
    }
    else if(sscanf(line, "FUNCTION(%s", line)) 
    {
      line = reverse(line);
      sscanf(line, "%*s)%s", line);
      line = reverse(line);
      if(sscanf(line, "%s, %s", fn, line) != 2)
      {
        ERR("Bad 'FUNCTION' line");
	exit(1);
      }
      last_function=fn;
      NUMBER_FUNCTION();
      emit("/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_"+fn+"(int args)\n");
      struct[progname][fn] = line;
      string a,b;
      sscanf(line, "\"function(%s:%s)\"", a, b);
      true_types[progname+fn] = ({ b, a });
    } 
    else if(sscanf(line, "SIGNAL(%s\")", line)) 
    {
      string name;
      string doc;
      sscanf(line, "\"%s\",%s", name, doc);
      sscanf(doc, "%*[ \t]\"%s", doc);
      if(signals[progname])
	signals[progname][name] = doc;
      else
	signals[progname] = ([ name:doc ]);
    } 
    else if(sscanf(line, "INHERIT(%s)", line)) 
    {
      emit("/* "+oline+" */\n");
      struct[progname]["inherit"] = line;
    } 
    else if(sscanf(line, "//%s", line)) 
    {
      if(do_docs) 
      {

	sscanf(line, "%*[ \t]%s", line);
	string a, b;
	if(in_img)
	  line = handle_img( line );
	if(sscanf(line, "img_begin%*s"))
	{
	  in_img=1;
          examples[lower_case(progname)]++;
	  line="";
	} 
	else if(sscanf(line, "IMG: %s", line))
        {
	  line = make_example_image(line,0);
          examples[lower_case(progname)]++;
        }
	else if(line[0..4] == "NOIMG")
        {
	  line = "";
          examples[lower_case(progname)]++;
        }
	else if(sscanf(line, "TIMG: %s", line))
        {
	  line = make_example_image(line,1);
          examples[lower_case(progname)]++;
        }
	else
        {
	  while(sscanf(line, "%sCONST(%s)%s", a, b, line)==3)
	    line = a+find_constants(b)+line;
	  while(sscanf(line, "%sCURS(%s)%s", a, b, line)==3)
	    line = a+build_cursor(b)+line;
	  while(sscanf(line, "%sW(%s)%s", a, b, line)==3)
	    line = a+"<a href="+lower_case( b )+".html>GTK."+b+"</a>"+line;
	  while(sscanf(line, "%sGDK(%s)%s", a, b, line)==3)
	    line = a+"<a href=Gdk"+b+".html>GDK."+b+"</a>"+line;
        }
	if(!docs[progname+last_function])
	  docs[progname+last_function] = line;
	else
	  docs[progname+last_function] += "\n"+line;
      }
    } 
    else if(sscanf(line, "CLASSMEMBER(%[^,],%s)", line, string type)) 
    {
      type -= " ";
      line -= " ";
      last_function="get_"+line;
      NUMBER_FUNCTION();
      struct[progname]["get_"+line] = "\"function(void:"+type+")\"";
      true_types[progname+"get_"+line] = ({ type, "" });

      emit("/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_get_"+line+"(int args)\n");
      emit("{\n");
      emit("  pop_n_elems(args);\n");
      emit("  push_"+(type=="string"?"text":type)+"( GTK_"+upper_case( progname )+"( THIS->obj )->"+line+");\n");
      emit("}\n");
    }
    else if(sscanf(line, "SETCLASSMEMBER(%[^,],%s)", line, string type)) 
    {
      type -= " ";
      line -= " ";
      last_function="set_"+line;
      NUMBER_FUNCTION();
      struct[progname]["set_"+line] = "\"function("+type+":"+type+")\"";
      true_types[progname+"set_"+line] = ({ type, type+" to" });

      emit("/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_set_"+line+"(int args)\n");
      emit("{\n");
      emit("  "+type+" to, old;\n");
      switch( type )
      {
       case "int":
         emit("  get_all_args( \"set_"+line+"\", args, \"%d\", &to );\n");
         break;
       case "float":
         emit("  get_all_args( \"set_"+line+"\", args, \"%f\", &to );\n");
         break;
       default:
         ERR("Cannot setclassmember of type "+type+"!");
         exit(1);
      }
      emit("  old = ( GTK_"+upper_case( progname )+
           "( THIS->obj )->"+line+");\n");
      emit("  ( GTK_"+upper_case( progname )+
           "( THIS->obj )->"+line+") = to;\n");
      
      emit("  push_"+(type=="string"?"text":type)+"( old );\n");
      emit("}\n");
    }
    else if(sscanf(line, "SUBWIDGET(%[^,],%s)", line, string type)) 
    {
      type -= " ";
      line -= " ";
      last_function=line;
      NUMBER_FUNCTION();
      struct[progname][line] = "\"function(void:object)\"";
      true_types[progname+line] = ({ "<a href=\""+lower_case(type)+".html\">"+
				     classname(lower_case(type))+"</a>", "" });

      emit("/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_"+line+"(int args)\n");
      emit("{\n");
      emit("  pop_n_elems(args);\n");
      emit("  push_gtkobjectclass( GTK_"+upper_case( progname )+"( THIS->obj )->"+line+", pgtk_"+type+"_program );\n");
      emit("}\n");
    } 
    else if((sscanf(line, "%sCOMPLEX_FUNCTION(%[^,],%s)",
                    rest,fn,types)==3)  ||
            ((sscanf(line, "%sCOMPLEX_FUNCTION(%[^,)])",
                     rest,fn)==2)&&(types="")))
    {
      string return_type;
      string argument_list = "";
      string fundef = "";
      string format_string="";
      string args="";
      string sargs="", pre_call="";
      string post = "", fin="", zap="";
      int na, i_added;
      
      sscanf(rest, "%*[\t ]%[^ \t]", rest);
      if(!strlen(rest))
      {
        rest = 0;
        return_type = classname(progname);
      } else
        return_type = rest;

      if(rest == "/*" || rest == "*")
        continue;
      last_function=fn;
      NUMBER_FUNCTION();
      if(fn == "create")
	fin = " pgtk__init_object( fp->current_object );\n";

      foreach(((types-" ")/"," - ({""})), string t)
      {
	na++;
	switch(lower_case(t))
	{
	 case "null":
	   sargs += ", NULL";
	   break;
	 case "callback":
	   fundef += ",function,mixed";
	   argument_list += ", function(mixed:mixed), mixed";
	   format_string += "%*%*";
	   args += ("  struct signal_data *b;\n"
	            "struct svalue *tmp1, *tmp2;\n");
	   post += ("  b = malloc(sizeof(struct signal_data));\n"
		    "  assign_svalue_no_free( &b->cb, tmp1 );\n"
		    " assign_svalue_no_free( &b->args, tmp2 );\n");
	   sargs += ",&tmp1,&tmp2";
	   break;

	 case "stringarray":
           if(!i_added++)
             args += "  int _i;\n";
	   fundef += ",array(string)";
	   argument_list += ", array(string)";
	   format_string += "%a";
	   args += "  struct array *_arg"+na+";\n";
	   args += "  char **arg"+na+";\n";
	   sargs += ", &_arg"+na;
	   fin += " free(arg"+na+");\n";
	   post += ("  arg"+na+"=malloc(sizeof(char *)* (_arg"+na+"->size));\n"
		    "  for(_i=0; _i<_arg"+na+"->size; _i++)\n"
		    "  {\n"
		    "    if(_arg"+na+"->item[_i].type != T_STRING)\n"
		    "    {\n"
		    "      free(arg"+na+");\n"
		    "      error(\"Wrong type array argument.\\n\");\n"
		    "    }\n"
		    "    arg"+na+"[_i] = _arg"+na+"->item[_i].u.string->str;\n"
		    "  }\n");
	   break;
	 case "floatarray":
	   string size = ", ^^arg"+na+"->size";
           if(!i_added++)
             args += "  int _i;\n";
	   fundef += ",array(float)";
	   argument_list += ", array(float)";
	   format_string += "%a";
	   args += "  struct array *_arg"+na+";\n";
	   args += "  gfloat *arg"+na+";\n";
	   zap += size;
	   fin += " free(arg"+na+");\n";
	   sargs += (size+", &_arg"+na);
	   post += ("  arg"+na+"=malloc(sizeof(gfloat)* (_arg"+na+"->size));\n"
		    "  for(_i=0; _i<_arg"+na+"->size; _i++)\n"
		    "  {\n"
		    "    if(_arg"+na+"->item[_i].type != T_FLOAT)\n"
		    "    {\n"
		    "      free(arg"+na+");\n"
		    "      error(\"Wrong type array argument. Expected float\\n\");\n"
		    "    }\n"
		    "    arg"+na+"[_i] = _arg"+na+"->item[_i].u.float_number;\n"
		    "  }\n");
	   break;
	 case "intarray":
	   string size = ", ^^arg"+na+"->size";
           if(!i_added++)
             args += "  int _i;\n";
	   fundef += ",array(int)";
	   argument_list += ", array(int)";
	   format_string += "%a";
	   args += "  struct array *_arg"+na+";\n";
	   args += "  gint *arg"+na+";\n";
	   zap += size;
	   fin += " free(arg"+na+");\n";
	   sargs += (",&_arg"+na);
	   post += ("  arg"+na+"=malloc(sizeof(gint)* (1+_arg"+na+"->size));\n"
		    "  for(_i=0; _i<_arg"+na+"->size; _i++)\n"
		    "  {\n"
		    "    if(_arg"+na+"->item[_i].type != T_INT)\n"
		    "    {\n"
		    "      free(arg"+na+");\n"
		    "      error(\"Wrong type array argument. Expected int\\n\");\n"
		    "    }\n"
		    "    arg"+na+"[_i] = _arg"+na+"->item[_i].u.integer;\n"
		    "  }\n"
                    "  arg"+na+"[_i] = 0;\n");
	   break;
	 case "int":
	   argument_list += ", int";
#if __VERSION__ >= 0.7
	   fundef += ",mixed";
	   format_string += "%D";
#else
	   fundef += ","+t;
	   format_string += "%d";
#endif
	   args += "  int arg"+na+";\n";
	   sargs += ", &arg"+na;
	   break;
	 case "intp":
	   argument_list += ", int";
#if __VERSION__ >= 0.7
	   fundef += ",mixed";
	   format_string += "%D";
#else
	   fundef += ",int";
	   format_string += "%d";
#endif
	   args += "  int _arg"+na+", *arg"+na+"=&_arg"+na+";\n";
	   sargs += ", _arg"+na;
	   break;
	 case "float":
	   argument_list += ", float";
#if __VERSION__ >= 0.7
	   fundef += ",mixed";
	   format_string += "%F";
#else
	   fundef += ",float";
	   format_string += "%f";
#endif
	   args += "  float arg"+na+";\n";
	   sargs += ", &arg"+na;
	   break;
	 case "string":
	   fundef += ","+t;
	   argument_list += ", string";
	   format_string += "%s";
	   args += "  char *arg"+na+";\n";
	   sargs += ", &arg"+na;
	   break;
	 default:
           if (sscanf(t,"stringarray/%s",t))
	   {
	      if(!i_added++)
		 args += "  int _i;\n";
	      fundef += ",array(string)";
	      argument_list += ", array(string)";
	      format_string += "%a";
	      args += "  struct array *_arg"+na+";\n";
	      args += "  char **arg"+na+";\n";
	      sargs += ", &_arg"+na;
	      post += ("  if (_arg"+na+"->size != "+
		       (t=replace(t,({"{","}"}),({"(",")"})))+
		       ") \n"
		       "    error(\"argument "+na+" (array(string)) has wrong size (columns=%d, elements=%d)\n\","+t+",_arg"+na+"->size);"
		       "  arg"+na+"=alloca(sizeof(char *)* (_arg"+na+"->size));\n"
		       "  for(_i=0; _i<_arg"+na+"->size; _i++)\n"
		       "  {\n"
		       "    if(_arg"+na+"->item[_i].type != T_STRING)\n"
		       "      error(\"Wrong type array argument.\\n\");\n"
		       "    arg"+na+"[_i] = _arg"+na+"->item[_i].u.string->str;\n"
		       "  }\n");
	      break;
	   }

           int star = sscanf(t, "*%s", t);
           int optional = sscanf(t, "?%s", t) && __VERSION__ >= 0.7;
	   fundef += ",object|int";
	   argument_list+=", "+classname(String.capitalize(lower_case(t)));
           if(optional) 
             argument_list += "|null";
           if(!optional)
             format_string += "%o";
           else
             format_string += "%O";
           if(sscanf( t, "Gdk%s", t ))
           {
             t = String.capitalize(lower_case(t));
             args += "  struct object *_arg"+na+";\n";
             args += "  Gdk"+t+" *arg"+na+";\n";
             sargs += ", &"+(star?"*":"")+"_arg"+na;
             post+=(" arg"+na+" = get_gdkobject( _arg"+na+", "+t+");\n");
             if(!optional)
               post +=("  if(!arg"+na+") error(\"Argument "+
                       na+": Wanted GDK object of type "+t+".\\n\");\n");
           } else {
             args += "  struct object *_arg"+na+";\n";
             args += "  "+sillycaps(t)+" *arg"+na+";\n";
             sargs += ", &"+(star?"*":"")+"_arg"+na;
             if(optional)
               post += "  if(_arg"+na+")\n  ";
             post+=(" arg"+na+" = GTK_"+upper_case(t)+"(get_pgtkobject(_arg"+
                    na+", pgtk_"+lower_case(t)+"_program ) );\n");
             if(optional)
               post += " else\n    arg"+na+" = NULL;\n";
             else
               post += ("  if(!arg"+na+") error(\"Argument "+
                        na+": Wanted GTK object of type "+t+".\\n\");\n");
           }
           break;
	}
      }
      
      emit( "/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_"+fn+"(int args)\n");
      emit("{\n");

      string srt;
      switch(rest)
      {
       case 0: srt = "object"; break;
       case "float": srt="float"; emit("  float result;\n"); break;
       case "int": srt="int";emit("  int result;\n"); break;
       case "string": srt="string"; emit("  gchar *result;\n"); break;
       default: 
         emit("  void *result;  /* "+rest+" */\n"); 
         srt = "object";
         break;
      }

      true_types[progname+fn] = ({ return_type, argument_list[2..] });
      struct[progname][fn] = "\"function("+
                           (strlen(fundef[1..])?fundef[1..]:"void")
                           +":"+srt+")\"";

      if(na)
      {
	emit( args );
        if( fn == "create" )
          emit( "  if( !pigtk_is_setup )\n"
                "      error(\"You must call GTK.setup_gtk( argv ) first"
                "\\n\");\n");
	emit( "  get_all_args(\""+progname+"->"+fn+"\", args, \""+
	      format_string+"\""+((sargs-"*")-zap)+");\n");
	sargs = replace(sargs,"&tmp1,&tmp2","(void *)pgtk_button_func_wrapper, b");
	emit(post);
      }
      if(fn == "create") 
      {
	emit("  if(THIS->obj) error(\"create called more than once\\n\");\n");
	emit("  THIS->obj = GTK_OBJECT( gtk_"+progname+"_new("+
	     replace((sargs[1..]-"&")-"_","^^","_")+" ) );\n");
	emit("  if(!THIS->obj) error(\"Failed to initiate "+progname+"\\n\");\n");
      }
      else
      {
	emit("  if(!THIS->obj)\n"
	     "    error(\"Calling function in unitiated object\\n\");\n");
        if(rest)
          emit("  result = ");
        else
          emit( "  " );
	emit("gtk_"+progname+"_"+fn+"( GTK_"+upper_case(progname)+
	     "( THIS->obj )"+replace((sargs-"&")-"_","^^","_")+" );\n");
      }
      if(strlen(fin)) 
        emit(fin+"\n");
      emit("  pop_n_elems(args);\n");
      if(!rest)
      {
        emit("  ref_push_object( fp->current_object );\n");
      } else {
        switch(rest)
        {
         case "float":
           emit( "  push_float( result );\n" );
           break;
         case "int":
           emit( "  push_int( result );\n" );
           break;
         case "void":
           emit( "  push_int( 0 );\n" );
           break;
         case "string":
           emit( "  if(result) push_text( result ); else push_int( 0 );\n" );
           break;
         default:
           if(sscanf( rest, "GDK.%s", rest ))
           {
             emit("  push_pgdkobject( result, pgtk_Gdk"+
                  String.capitalize(lower_case(rest))+"_program);\n");
           } else {
             sscanf(rest, "GTK.%s", rest );
             emit("  push_gtkobjectclass( result, "+
                  "pgtk_"+lower_case(rest)+"_program );\n");
           }
        }
      }
      emit("}\n\n");
    } else if(sscanf(line, "CONSTANT(%s)", line)==1) {
      if(!sscanf(line, "GTK_%s", fn))
	fn = line;
      constants += ("  add_integer_constant(\""+fn+"\", "+line+", 0);\n");
    } else if(sscanf(line, "SIMPLE%sFUNCTION(%s)", line, fn)==2) {
      last_function=fn;
      NUMBER_FUNCTION();
      emit("/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_"+fn+"(int args)\n");
      emit("{\n");
      switch(line)
      {
       case "_":
	 /* void function.... */
	 true_types[progname+fn] = ({ classname(progname), "" });
	 struct[progname][fn] = "\"function(void:object)\"";
	 emit("  if(!THIS->obj)\n"
	      "    error(\"Calling function in unitiated object\\n\");\n");
	 emit("  gtk_"+progname+"_"+fn+"(GTK_"+upper_case(progname)+"(THIS->obj));\n");
	 break;
       case "_INT_":
	 emit("  int i;\n");
	 struct[progname][fn] = "\"function(int:object)\"";
	 true_types[progname+fn] = ({ classname(progname), "int" });
	 emit("  if(!THIS->obj)\n"
	      "    error(\"Calling function in unitiated object\\n\");\n");
#if __VERSION__ >= 0.7
	 emit("  get_all_args(\""+progname+"->"+fn+"\",args, \"%D\", &i);\n");
#else
	 emit("  get_all_args(\""+progname+"->"+fn+"\",args, \"%d\", &i);\n");
#endif
	 emit("  gtk_"+progname+"_"+fn+"( GTK_"+upper_case(progname)+
	      "( THIS->obj ), i );\n");
	 break;
       default:
	 line-="_";
	 emit("  struct object *o;\n");
	 struct[progname][fn] = "\"function(object:object)\"";
	 true_types[progname+fn]=({ classname(progname), 
				    "<a href=\""+lower_case(line)+".html\">"+
				    classname(lower_case(line))+"</a>" });
	 emit("  GtkObject *f;\n");
	 emit("  if(!THIS->obj)\n"
	      "    error(\"Calling function in unitiated object\\n\");\n");
	 emit("  get_all_args(\""+progname+"->"+fn+"\", args, \"%o\", &o);\n");
	 emit("  f = get_gtkobject( o );\n");
	 emit("  if(!f)\n"
	      "    error(\"Expected "+classname(line)+", got NULL\\n\");\n");
	 emit("  gtk_"+progname+"_"+fn+"( GTK_"+upper_case(progname)+"( THIS->obj ), GTK_"+upper_case(line)+"( f ) );\n");
      }

      emit("  pop_n_elems(args);\n");
      emit("  ref_push_object( fp->current_object );\n");
      emit("}\n");
    } else if(sscanf(line, "ARGS(%s);", line)==1) {
      true_types[progname+last_function][1] = line;
    } else if(sscanf(line, "RETURNS(%s);", line)==1) {
      true_types[progname+last_function][0] = line;
    } else if(sscanf(line, "NAME_ARGS(%s);", line)==1) {
      array a = line / ",";
      array b = true_types[progname+last_function][1] / ",";
      int i;
      string res="";
      for(i=0; i<min(sizeof(a),sizeof(b)); i++)
	res += ","+b[i]+" "+a[i];
      true_types[progname+last_function][1] = res[1..];      
      named[progname+last_function] = 1;
    } else {
      emit(line+"\n");
    }
  }
  })
  {
    ERR("Fatal error on: "+describe_backtrace(err));
    exit(1);
  }
  string pre="";

  end_last_program();
  /* global stuff.. */
  no_hashline=1;

  emit_nl(extra_cpp);
  emit_nl("void clear_obj_struct(struct object *o)\n{\n");
  emit_nl("  MEMSET(fp->current_storage, 0, sizeof(struct object_wrapper));\n");
  emit_nl("}\n");
  emit_nl("void pike_module_init()\n{\n");
  emit_nl(constants);

  foreach(sort(indices(`+(@values(signals)))), string s)
    emit_nl("  add_string_constant( \"s_"+s+"\", \""+s+"\", 0 );\n");
  
  emit_program_block( struct->global, "global" );
  if(!do_docs)
    m_delete(struct, "global");

  if(do_docs)
  {
    foreach(get_dir(dir+"/pcdocs"), string d)
    {
      string last_function, progname, a, b;
      int ln;
      if(Stdio.file_size(dir+"/pcdocs/"+d) < 1)
        continue;

#undef  NUMBER_FUNCTION
#define NUMBER_FUNCTION() lines[progname+last_function] = "pcdocs/"+d+":"+ln
      foreach(Stdio.read_bytes(dir+"/pcdocs/"+d)/"\n", string line)
      {
        ln++;
        if(sscanf(line, "PROGRAM(%s);", line))
        {
          last_function ="";
          progname = line;
          NUMBER_FUNCTION();
          struct[line] = ([]);
        } else if(sscanf(line, "INHERIT(%s)", line)) {
          struct[progname]["inherit"] = line;
        } else if(sscanf(line, "FUNCTION(%s", line)) {
          string fn;
          line = reverse(line);
          sscanf(line, "%*s)%s", line);
          line = reverse(line);
          if(sscanf(line, "%s, %s", fn, line) != 2)
          {
            werror("Bad 'FUNCTION' line '"+line+"' in file "+d+"; line "+ln+"\n");
            exit(1);
          }
          last_function=fn;
          NUMBER_FUNCTION();
          struct[progname][fn] = line;
          sscanf(line, "\"function(%s:%s)\"", a, b);
          true_types[progname+fn] = ({ b, a });
        } else if(sscanf(line, "SIGNAL(%s\")", line)) {
          string name;
          string doc;
          sscanf(line, "\"%s\",%s", name, doc);
          sscanf(doc, "%*[ \t]\"%s", doc);
          if(signals[progname])
            signals[progname][name] = doc;
          else
            signals[progname] = ([ name:doc ]);
        } else if(sscanf(line, "//%s", line)) {
          sscanf(line, "%*[ \t]%s", line);
          string a, b;
          if(in_img)
            line = handle_img( line );
          if(sscanf(line, "img_begin%*s"))
          {
            examples[progname]++;
            in_img=1;
            line="";
          } 
          else if(sscanf(line, "IMG: %s", line))
          {
            line = make_example_image(line,0);
            examples[progname]++;
          }
          else if(sscanf(line, "TIMG: %s", line))
          {
            line = make_example_image(line,1);
            examples[progname]++;
          }
          else
            while(sscanf(line, "%sCONST(%s)%s", a, b, line)==3)
              line = a+find_constants(b)+line;
          if(!docs[progname+last_function])
            docs[progname+last_function] = line;
          else
            docs[progname+last_function] += "\n"+line;
        } else if(sscanf(line, "SUBWIDGET(%[^,],%s)", line, string type)) {
          type -= " ";
          line -= " ";
          last_function=line;
          NUMBER_FUNCTION();
          struct[progname][line] = "\"function(void:object)\"";
          true_types[progname+line] = ({ "<a href=\""+lower_case(type)+".html\">"+
                                         classname(lower_case(type))+"</a>", "" });
        } else if(sscanf(line, "ARGS(%s);", line)==1) {
          true_types[progname+last_function][1] = line;
        } else if(sscanf(line, "RETURNS(%s);", line)==1) {
          true_types[progname+last_function][0] = line;
        } else if(sscanf(line, "NAME_ARGS(%s);", line)==1) {
          array a = line / ",";
          array b = true_types[progname+last_function][1] / ",";
          int i;
          string res="";
          for(i=0; i<min(sizeof(a),sizeof(b)); i++)
            res += ","+b[i]+" "+a[i];
          true_types[progname+last_function][1] = res[1..];      
          named[progname+last_function] = 1;
        }
      }
    }
  }

//   werror(sizeof(struct)+" classes\n");
//   werror(sizeof(constants/"\n")+" constants\n");
//   werror(sizeof(signals)+" signal constants (strings)\n");

  string to_free="";
  foreach(sort_dependencies(indices(struct),struct), string w)
  {
    mapping q = struct[w];
    emit_nl("\n\n  start_new_program(); /* "+String.capitalize(w)+" */\n");
    to_free += "  free_program( pgtk_"+w+"_program );\n";
    if(q["inherit"])
    {
      emit_nl("   low_inherit( pgtk_"+q["inherit"]+"_program, 0,0,0,0,0);\n");
      m_delete(q, "inherit");
    }
    else
    {
#if __VERSION__ > 0.6
      emit_nl("  ADD_STORAGE(struct object_wrapper);\n");
#else
      emit_nl("  add_storage(sizeof(struct object_wrapper));\n");
#endif
    }
    emit_nl("   set_init_callback(clear_obj_struct);\n");
    emit_program_block( q, w );
    emit_nl("  add_program_constant(\""+String.capitalize(w)+"\",\n"
	    "                       (pgtk_"+w+"_program = end_program()), 0);"
	    "\n");
    pre += "/*ext*/ struct program *pgtk_"+w+"_program;\n";
  }
//   werror(_num_functions+" functions\n");
  emit_nl("}\n\n");
  emit_nl("\nvoid pike_module_exit()\n{\n"+to_free+"}\n\n");
  files += "pgtk.c ";
  if(!do_docs)
  {
    string q =replace(Stdio.read_bytes(dir+"/pgtk.c.head"), "PROTOTYPES", 
                      replace(pre, "/*ext*/ ", ""));
    pre = replace(pre, "/*ext*/", "extern");
    if(!equal(sort((Stdio.read_bytes("prototypes.h") || "")/"\n"), sort(pre/"\n")))
    {
      rm("prototypes.h");
      werror("prototypes.h was modified\n");
      Stdio.write_file("prototypes.h", pre);
    }
    q+=(sort(glob_prototypes/"\n")*"\n") + "\n\n" + buffer;
    if(!equal(sort((Stdio.read_bytes("pgtk.c") || "")/"\n"),
	      sort(q/"\n")))
    {
      werror("pgtk.c modified\n");
      object outf = Stdio.File("pgtk.c", "rwct");
      outf->write(q);
    }
  }
  rm("files_to_compile");
  Stdio.write_file("files_to_compile", replace(files, ".c", ".o"));
}


