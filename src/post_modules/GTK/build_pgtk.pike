int ln, eml=-100;
string buffer="";
#include "docs.pike"

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

int last_ended_with_nl, debug_mode=1, no_hashline=0;
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

  if( sscanf(what, "%s.%s", base, what) == 2)
    base = base+".";

  if(sscanf(what, "Gdk_%s", what) ||
     sscanf(what, "Gdk%s", what))
    base = "GDK.";
  else if(!search(lower_case(what),"gnome"))
  {
    what = what[5..];
    sscanf( what, "_%s", what );
    base = "Gnome.";
  }
  else if(what == "Atom" || what == "GDK.Atom")
    return "GDK._Atom";
  return base+sillycaps( what, 1 );
}

string funname( string f )
{
  if( sscanf( f, "gtk_gnome_applet_%s", f ) )
    return "applet_"+f;
  if( sscanf( f, "gtk_gnome_%s", f ) )
    return "gnome_"+f;
  return f;
}

string castname( string f )
{
  if( sscanf( f, "GTK_GNOME_APPLET_%s", f ) )
    return "APPLET_"+f;
  if( sscanf( f, "GTK_GNOME_%s", f ) )
    return "GNOME_"+f;
  return f;
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
  if( !search( what, "Gnome" ) )return what;
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
  if(do_docs) 
    return;
//   werror(internal_progname+"...\n");
  string data;
  if((Stdio.file_size("pgtk_"+internal_progname+".c") !=
      (strlen(buffer)+strlen(head))) ||
     (Stdio.read_bytes( "pgtk_"+internal_progname+".c" ) !=
      head+buffer ))
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

string make_c_string( string from )
{
  string line = "\"";
  string res = "";
  int c;
  for( int i=0; i<strlen( from ); i++ )
  {
    switch( (c=from[i]) )
    {
     case 'a'..'z':
     case 'A'..'Z':
     case '0'..'9':
     case 0300..0376:
     case '_': case ' ':
       line += from[i..i];
       break;
     default:
       line += sprintf("\\%o", c );
       break;
    }
    if( strlen( line ) > 75 )
    {
      res += line+"\"\n";
      line="\"";
    }
  }
  return res+line+"\"";
}

string function_type( string what )
{
  what = reverse( what ); sscanf( what, "%*[ \t\n\r\"]%s", what );
  what = reverse( what ); sscanf( what, "%*[ \t\n\r\"]%s", what );
  return __parse_pike_type( what );
}

string data = "";
int enclen;
mapping ocache = ([]);
int data_offset( string what )
{
  int off;
  enclen += strlen(what)+1;
  if( ocache[what] ) return ocache[what];
  if( (off = search( data, what )) != -1 )
    return ocache[what] = off;
  data += what;
  return ocache[what]=strlen(data)-strlen(what);
}

string emit_function_def( string fun, string cfun, string type, int opt )
{
  type = function_type( type );
  emit_nl( "    quick_add_function((char*)_data+"+data_offset(fun)+","+
           strlen(fun)+","+cfun+",(char*)_data+"+data_offset( type )+
           ","+strlen(type)+","+(fun=="create"?"ID_STATIC":"0")+",0);\n");
}

int _num_functions;
multiset do_default_sprintf = (<>);
void emit_program_block(mapping block, string cl)
{
  line_id = "";
  if( cl != "global" && !block->_sprintf )
  {
    do_default_sprintf[cl] = 1;
    emit( " static void _pgtk_"+cl+"_default_sprintf(INT32 args);\n" );
    emit_function_def( "_sprintf", "_pgtk_"+cl+"_default_sprintf",
                       "function(int:string)", 1 );
  }

  foreach(sort(indices(block)), string f)
  {
    _num_functions++;
    string cfun = "pgtk_"+cl+"_"+f;
    switch(f)
    {
     case "union":
       emit_function_def("`|",cfun,block[f],1);
       break;
     case "intersect":
       emit_function_def("`&",cfun,block[f],1);
       break;
     case "subtract":
       emit_function_def("`-",cfun,block[f],1);
       break;
     case "equal":
       emit_function_def("`==",cfun,block[f],1);
       break;
     case "lt":
       emit_function_def("`<",cfun,block[f],1);
       break;
     case "xor":
       emit_function_def("`^",cfun,block[f],1);
       break;
     case "not":
       emit_function_def("`~",cfun,block[f],1);
       break;
     case "gt":
       emit_function_def("`>",cfun,block[f],1);
       break;
     case "_add":
       emit_function_def("`+",cfun,block[f],1);
       continue;
     case "_index":
       emit_function_def("`->",cfun,block[f],1);
       emit_function_def("`[]",cfun,block[f],1);
       continue;
    }
    emit_function_def(f,cfun,block[f],0);
  }
}

mapping docs = ([]), true_types = ([]), named = ([]);
mapping lines = ([]), examples = ([]);

string constants="";

string gödsla_med_line(string s, string f)
{
  if( no_hashline ) return s;
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
      data += gödsla_med_line(Stdio.read_bytes( dir+"/source/"+f )+
                              "\nendrequire", dir+"/source/"+f );
  return data;
}

multiset options;
int has_cond_widget( string what )
{
  if(!options)
    options = mkmultiset( (Stdio.read_bytes( "options" )-" ")/"\n" );
  return options[what];
}

string dir;
mapping requires = ([]);

int main(int argc, array argv)
{
  string progname = "global", extra_cpp="";
  string types;
  string last_function;
  int skip_mode, verbatim_mode;
  string type_switch="";
  string default_sprintf=
#"
void do_default_sprintf( int args, int offset, int len )
{
  my_pop_n_elems( args );
  push_string( make_shared_binary_string( _data+offset, len ) );
}
";
  string signal_doc, current_require;

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

    if((String.trim_whites(line)-";") == "endrequire")
    {
      skip_mode=0;
      current_require = 0;
      continue;
    }

    if(skip_mode)
      continue;

    if( line == "%{" )
    {
      verbatim_mode = 1;
      continue;
    }
    else if( line == "%}" )
    {
      verbatim_mode = 0;
      continue;
    }
    if( verbatim_mode )
    {
      emit( line+"\n");
      continue;
    }

    if(sscanf(line, "require %s;", line))
    {
      line = String.trim_whites( line );
      current_require = line;
      if(!requires[current_require])
        requires[current_require] = ({});
      if(!has_cond_widget(line))
        skip_mode=1;
    }
    else if(sscanf(line, "ADD_INCLUDE(%s);", line))
    {
      extra_cpp += line+"\n";
    }
    else if(sscanf(line, "class %s;", line))
    {
      signal_doc = 0;
      line = String.trim_whites( line );
      end_last_program();
      if( current_require )
        requires[current_require] += ({ line });
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
    else if(sscanf(line, "signal %s;", string name))
    {
      signal_doc = name;
      if(signals[progname])
	signals[progname][signal_doc] = "";
      else
	signals[progname] = ([ name:"" ]);
    }
    else if(sscanf(line, "inherit %s;", line))
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
	    line = a+find_constants(b,lines[progname+last_function]||progname)
                 +line;
	  while(sscanf(line, "%sCURS(%s)%s", a, b, line)==3)
	    line = a+build_cursor(b)+line;
	  while(sscanf(line, "%sW(%s)%s", a, b, line)==3)
	    line = a+link_class( b )+line;
	  while(sscanf(line, "%sGDK(%s)%s", a, b, line)==3)
	    line = a+link_class( "GDK."+b )+line;
        }
        if( signal_doc )
          signals[progname][signal_doc] += line;
        else if(!docs[progname+last_function])
	  docs[progname+last_function] = line;
	else
	  docs[progname+last_function] += "\n"+line;
      }
    }
    else if(sscanf(line, "array %s %s[%s];", string type, line, string size ) )
    {
      string ptype;
      int and, star;
      type = String.trim_whites( type );
      and = sscanf( type, "&%s", type ) || sscanf(line,"&%s",line);
      star = sscanf( type, "*%s", type ) || sscanf(line,"*%s",line);;
      line = String.trim_whites( line );
      size = String.trim_whites( size );
      size = replace( size, "this", 
                      castname("GTK_"+upper_case( progname ))+"( THIS->obj )");
      switch( type )
      {
       case "int":    ptype = "int";   break;
       case "float":  ptype = "float";   break;
       case "double": ptype = "float";  break;
       case "string": ptype = "string"; break;
       default:       ptype = "object"; break;
      }
      signal_doc = 0;
      last_function="get_"+line;
      NUMBER_FUNCTION();
      struct[progname]["get_"+line] = "\"function(void:array("+ptype+"))\"";
      true_types[progname+last_function] = ({ "array("+type+")", "" });
      
      emit("/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_get_"+line+"(int args)\n");
      emit("{\n");
      emit("  int n;\n");
      emit("  my_pop_n_elems(args);\n");
      emit(" for( n = 0; n<"+size+"; n++ )\n");
      if( ptype != "object" )
      {
        emit("   push_"+(ptype=="string"?"text":ptype)+"( "+
             (and?"&":"")+(star?"*":"")+
             "("+castname("GTK_"+upper_case( progname ))+
             "( THIS->obj )->"+line+"[n]));\n");
      } else {
        if( sscanf( type, "GDK.%s", type ) )
          emit("    push_gdkobject( "+
               (and?"&":"")+(star?"*":"")+
               "("+castname("GTK_"+upper_case( progname ))+
               "( THIS->obj )->"+line+"[n]), "+type+");\n" );
        else
        {
          sscanf( type, "GTK.%s", type );
          type = lower_case( type );
          emit("    push_gtkobjectclass( "+
               (and?"&":"")+(star?"*":"")+
               "("+castname("GTK_"+upper_case( progname ))+
               "( THIS->obj )->"+line+"[n]), pgtk_"+type+"_program);\n" );
        }
      }
      emit("  f_aggregate( "+size+" );\n");
      emit("}\n");
    }
    else if(sscanf(line, "member %s %s;", string type, line ) )
    {
      string ptype;
      int and, star;
      signal_doc=0;
      line = String.trim_whites( line );
      type = String.trim_whites( type );
      and = sscanf( type, "&%s", type ) || sscanf(line,"&%s",line);
      star = sscanf( type, "*%s", type ) || sscanf(line,"*%s",line);;
      last_function="get_"+line;
      switch( type )
      {
       case "int":    ptype = "int";   break;
       case "float":  ptype = "float";   break;
       case "double": ptype = "float";  break;
       case "string": ptype = "string"; break;
       default:       ptype = "object"; break;
      }
      NUMBER_FUNCTION();
      struct[progname][last_function] = "\"function(void:"+ptype+")\"";
      true_types[progname+last_function] = ({ type, "" });

      emit("/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_get_"+line+"(int args)\n");
      emit("{\n");
      emit("  my_pop_n_elems(args);\n");
      if( ptype != "object" )
      {
        emit(" push_"+(ptype=="string"?"text":ptype)+"( "+
             (and?"&":"")+(star?"*":"")+
             "("+castname("GTK_"+upper_case( progname ))+
             "( THIS->obj )->"+line+"));\n");
      } else {
        if( sscanf( type, "GDK.%s", type ) )
          emit("  push_gdkobject( "+
               (and?"&":"")+(star?"*":"")+
               "("+castname("GTK_"+upper_case( progname ))+
               "( THIS->obj )->"+line+"), "+type+");\n" );
        else
        {
          sscanf( type, "GTK.%s", type );
          type = lower_case( type );
          emit("  push_gtkobjectclass( "+
               (and?"&":"")+(star?"*":"")+
               "("+castname("GTK_"+upper_case( progname ))+
               "( THIS->obj )->"+line+"), pgtk_"+type+"_program);\n" );
        }
      }
      emit("}\n");
    }
    else if(sscanf(line, "setmember %s %s;", string type, line))
    {
      signal_doc=0;
      line = String.trim_whites( line );
      type = String.trim_whites( type );
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
      emit("  old = ( "+castname("GTK_"+upper_case( progname ))+
           "( THIS->obj )->"+line+");\n");
      emit("  ( "+castname("GTK_"+upper_case( progname ))+
           "( THIS->obj )->"+line+") = to;\n");

      emit("  push_"+(type=="string"?"text":type)+"( old );\n");
      emit("}\n");
    }
    else if(sscanf(line, "subwidget %s %s;", string type, line))
    {
      signal_doc=0;
      line = String.trim_whites( line );
      type = String.trim_whites( type );
      last_function=line;
      NUMBER_FUNCTION();
      struct[progname][line] = "\"function(void:object)\"";
      true_types[progname+line] = ({ link_class( type ), "" });

      emit("/* "+oline+" */\n");
      emit_proto("void pgtk_"+progname+"_"+line+"(int args)\n");
      emit("{\n");
      emit("  my_pop_n_elems(args);\n");
      emit("  push_gtkobjectclass( "+castname("GTK_"+upper_case( progname ))
           +"( THIS->obj )->"+line+", pgtk_"+type+"_program );\n");
      emit("}\n");
    }
    else if(sscanf(line, "%[^ ] %s(%s);", rest, fn, types ) == 3)
    {
      signal_doc=0;
      string return_type;
      string argument_list = "";
      string fundef = "";
      string format_string="";
      string args="";
      string sargs="", pre_call="";
      string post = "", fin="", zap="";
      int na, i_added, free_res;


      types = String.trim_whites( types );
      rest = String.trim_whites( rest );
      fn = String.trim_whites( fn );

      if(!strlen(rest) || rest[0] == '#' || rest == "extern")
      {
        emit( line+"\n" );
        continue;
      }

//       werror(rest+" "+fn+" ( "+types+" )\n");

      if( rest == "void" )
        rest = "";

      if(!strlen(rest))
      {
        rest = 0;
        return_type = classname(progname);
      } else {
        if( rest == "fstring" )
        {
          free_res = 1;
          rest = rest[1..];
        }
        return_type = rest;
      }

      if(rest == "/*" || rest == "*")
        continue;
      last_function=fn;
      NUMBER_FUNCTION();
      if(fn == "create")
	fin = "  pgtk__init_object( fp->current_object );\n";
      int last_was_optional;
      array tmp = map( (types/","-({""})),
                       lambda( string q ) {
                         return (String.trim_whites(q)/" "-({""}))+({"",""});
                       } );
      array types = column(tmp,0);
      tmp = column(tmp,1);
//       werror("%O\n", types );

      foreach(types-({""}), string t)
      {
	na++;
        last_was_optional=0;
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

         case "nullstringarray":
           int null = 1;
         case "stringarray":
           if(!i_added++)
             args += "  int _i;\n";
	   fundef += ",array(string)";
	   argument_list += ", array(string)";
	   format_string += "%a";
	   args += "  struct array *_arg"+na+";\n";
	   args += "  gchar **arg"+na+";\n";
	   sargs += ", &_arg"+na;
	   fin += " free(arg"+na+");\n";
	   post += ("  arg"+na+"=malloc(sizeof(char *)* (_arg"+na+"->size+"+null+"));\n"
		    "  for(_i=0; _i<_arg"+na+"->size; _i++)\n"
		    "  {\n"
		    "    if(_arg"+na+"->item[_i].type != T_STRING)\n"
		    "    {\n"
		    "      free(arg"+na+");\n"
		    "      error(\"Wrong type array argument.\\n\");\n"
		    "    }\n"
		    "    arg"+na+"[_i] = _arg"+na+"->item[_i].u.string->str;\n"
		    "  }\n");
           if( null )
             post += "  arg"+na+"[_i] = NULL;\n";
	   break;
	 case "!floatarray":
           free_res=1; // Somewhat misleading reuse of variable...
	 case "floatarray":
           if(!i_added++)
             args += "  int _i;\n";
	   fundef += ",array(float)";
	   argument_list += ", array(float)";
	   format_string += "%a";
	   args += "  struct array *_arg"+na+";\n";
	   args += "  gfloat *arg"+na+";\n";
           if(!free_res)
             fin += " g_free(arg"+na+");\n";
	   sargs += (",&_arg"+na);
	   post += ("  arg"+na+"=g_malloc(sizeof(gfloat)* (_arg"+na+"->size));\n"
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
	 case "!doublearray":
           free_res=1; // Somewhat misleading reuse of variable...
	 case "doublearray":
           if(!i_added++)
             args += "  int _i;\n";
	   fundef += ",array(float)";
	   argument_list += ", array(float)";
	   format_string += "%a";
	   args += "  struct array *_arg"+na+";\n";
	   args += "  gdouble *arg"+na+";\n";
           if(!free_res)
             fin += " g_free(arg"+na+");\n";
	   sargs += (",&_arg"+na);
	   post += ("  arg"+na+"=g_malloc(sizeof(gfloat)* (_arg"+na+"->size));\n"
		    "  for(_i=0; _i<_arg"+na+"->size; _i++)\n"
		    "  {\n"
		    "    if(_arg"+na+"->item[_i].type != T_FLOAT)\n"
		    "    {\n"
		    "      free(arg"+na+");\n"
		    "      error(\"Wrong type array argument. Expected float\\n\");\n"
		    "    }\n"
		    "    arg"+na+"[_i] = (gdouble)_arg"+na+"->item[_i].u.float_number;\n"
		    "  }\n");
	   break;
	 case "!intarray":
           free_res=1; // Somewhat misleading reuse of variable...
	 case "intarray":
           if(!i_added++) args += "  int _i;\n";
	   fundef += ",array(int)";
	   argument_list += ", array(int)";
	   format_string += "%a";
	   args += "  struct array *_arg"+na+";\n";
	   args += "  gint *arg"+na+";\n";
           if(!free_res)
             fin += " g_free(arg"+na+");\n";
	   sargs += (",&_arg"+na);
	   post += ("  arg"+na+"=g_malloc(sizeof(gint)* (1+_arg"+na+"->size));\n"
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
	 case "?int":
           last_was_optional = 1;
	 case "int":
	   argument_list += ", int"+(last_was_optional?"|void":"");
	   fundef += ",mixed"+(last_was_optional?"|void":"");
	   format_string += "%D";
	   args += "  int arg"+na+(last_was_optional?" = 0":"")+";\n";
	   sargs += ", &arg"+na;
	   break;
	 case "intp":
	   argument_list += ", int";
	   fundef += ",mixed"+(last_was_optional?"|void":"");
	   format_string += "%D";
	   args += "  int _arg"+na+", *arg"+na+"=&_arg"+na+";\n";
	   sargs += ", _arg"+na;
	   break;
	 case "?float":
           last_was_optional = 1;
	 case "float":
	   argument_list += ", float"+(last_was_optional?"|void":"");
	   fundef += ",mixed"+(last_was_optional?"|void":"");
	   format_string += "%F";
	   args += "  float arg"+na+(last_was_optional?" = 0.0;":"")+";\n";
	   sargs += ", &arg"+na;
	   break;
	 case "?string":
           last_was_optional = 1;
	 case "string":
	   fundef += ",string"+(last_was_optional?"|void":"");
	   argument_list += ", string"+(last_was_optional?"|void":"");
	   format_string += "%s";
	   args += "  char *arg"+na+(last_was_optional?" = NULL":"")+";\n";
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
		       "    error(\"argument "+na+" (array(string)) has wrong size (columns=%d, elements=%d)\\n\",\n"
		       "          "+t+",_arg"+na+"->size);\n"
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
           int opt = sscanf(t, "?%s", t);
           if( opt )
             fundef += ",object|int|void";
           else
             fundef += ",object";
	   argument_list+=", "+classname(String.capitalize(lower_case(t)));
           if(opt)
             argument_list += "|void";

           if(!opt)
             format_string += "%o";
           else
           {
             last_was_optional = 1;
             format_string += "%O";
           }
           if(sscanf( t, "Gdk%s", t ) || sscanf(t,"GDK.%s",t))
           {
             t = String.capitalize(lower_case(t));
             sargs += ", &"+(star?"*":"")+"_arg"+na;
             post+=(" arg"+na+" = get_gdkobject( _arg"+na+", "+t+");\n");
             if(!opt)
             {
               post +=("  if(!arg"+na+") error(\"Argument "+
                       na+": Wanted GDK object of type "+t+".\\n\");\n");
              args += "  struct object *_arg"+na+";\n";
              args += "  Gdk"+t+" *arg"+na+";\n";
             } else {
              args += "  struct object *_arg"+na+" = NULL;\n";
              args += "  Gdk"+t+" *arg"+na+";\n";
             }
           } else {
             sargs += ", &"+(star?"*":"")+"_arg"+na;
             if(opt)
             {
               args += "  struct object *_arg"+na+" = NULL;\n";
               args += "  "+sillycaps(t)+" *arg"+na+" = NULL;\n";
               post += "  if(_arg"+na+")\n  ";
             } else {
               args += "  struct object *_arg"+na+";\n";
               args += "  "+sillycaps(t)+" *arg"+na+";\n";
             }
             post+=(" arg"+na+" = "+castname("GTK_"+upper_case(t))+
                    "(get_pgtkobject(_arg"+
                    na+", pgtk_"+lower_case(t)+"_program ) );\n");
             if(opt)
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

      int ti;
      true_types[progname+fn] = ({ return_type,
                                   map((argument_list[2..]/","-({""})),
                                       lambda( string q ) {
                                         return q+(sizeof(tmp)>ti?" "+tmp[ti++]:"");
                                       })*", " });
      named[progname+fn] = 1;
//       werror("arguments: %O\n",true_types[progname+fn]);
      struct[progname][fn] = "\"function("+
                           (strlen(fundef[1..])?fundef[1..]:"void")
                           +":"+srt+")\"";

      if(na)
      {
	emit( args );
        if( fn == "create" )
          if( search( progname, "gnome" ) != -1 )
            emit("  pgtk_verify_gnome_setup();\n");
          else
            emit("  pgtk_verify_setup();\n");

        if( last_was_optional )
        {
          string sfs = format_string[..sizeof(format_string)-3];
          emit( "  if( args == "+(sizeof(sfs)/2)+" )\n"
                "    get_all_args(\""+fn+"\", args, \""+sfs
                +"\""+((sargs-"*")-zap)+");\n  else\n  ");
        }
	emit( "  get_all_args(\""+fn+"\", args, \""+
	      format_string+"\""+((sargs-"*")-zap)+");\n");
	sargs = replace(sargs,"&tmp1,&tmp2","(void *)pgtk_button_func_wrapper, b");
	emit(post);
      }
      if(fn == "create")
      {
        emit("  pgtk_verify_not_inited();\n");
	emit("  THIS->obj = GTK_OBJECT( "+funname("gtk_"+progname+"_new")+
             "("+replace((sargs[1..]-"&")-"_","^^","_")+" ) );\n");
      }
      else
      {
        emit("  pgtk_verify_inited();\n");
        if(rest)
          emit("  result = ");
        else
          emit( "  " );
	emit(funname("gtk_"+progname+"_"+fn)+"( "+
             castname("GTK_"+upper_case(progname))+
             "( THIS->obj )"+replace((sargs-"&")-"_","^^","_")+" );\n");
      }
      if(strlen(fin))
        emit(fin+"\n");
      if(!rest)
      {
        emit("  RETURN_THIS();\n");
      } else {
        emit("  my_pop_n_elems(args);\n");
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
           if( free_res )
             emit( "  if( result ) g_free( result );\n");
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
    }
    else if(sscanf(line, "constant string %s;", line)==1)
    {
      signal_doc=0;
      if(!sscanf(line, "GTK_%s", fn))
	fn = line;
      constants_name += ({ fn });
      constants += ("  add_string_constant((char*)_data+"+
                    data_offset(fn+"\0")+", "+line+", 0);\n");
    }
    else if(sscanf(line, "constant int %s;", line)==1 ||
            sscanf(line, "constant %[^ ];", line)==1)
    {
      signal_doc=0;
      if(!sscanf(line, "GTK_%s", fn))
	fn = line;
      constants_name += ({ fn });
      constants += ("  add_integer_constant((char*)_data+"+
                    data_offset(fn+"\0")+", "+line+", 0);\n");
    }
    else if(sscanf(line, "ARGS(%s);", line)==1)
    {
      signal_doc=0;
      true_types[progname+last_function][1] = line;
    }
    else if(sscanf(line, "RETURNS(%s);", line)==1)
    {
      signal_doc=0;
      true_types[progname+last_function][0] = line;
    }
    else if(sscanf(line, "NAME_ARGS(%s);", line)==1)
    {
      signal_doc=0;
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
  emit_nl("static void _1()\n{\n");
  array _inits = ({ "_1" });
  emit_nl(constants);
  emit_nl("}\n");
  emit_nl("static void _2()\n{\n");
  _inits += ({ "_2" });
  foreach(sort(indices(`+(@values(signals)))), string s)
    emit_nl("  add_string_constant( (char*)_data+"+data_offset("s_"+
                                                               replace(s,"-","_")
                                                               +"\0")+
            ", (char*)_data+"+data_offset(s+"\0")+", 0 );\n");

  emit_program_block( struct->global, "global" );
  emit_nl("}\n");

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
          else if(sscanf(line, "%sW(%s)%s", a,b,line))
          {
            line = a+link_class( b )+line;
            examples[progname]++;
          }
          else
            while(sscanf(line, "%sCONST(%s)%s", a, b, line)==3)
              line = a+find_constants(b,lines[progname+last_function]||progname)+line;
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
          true_types[progname+line] = ({ link_class( type ), "" });
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

  string to_free="";

  if(!do_docs)
  {
    array functions = ({});
    foreach( indices( struct ), mapping q )
      foreach( indices( struct[q] ) , string f )
        if( f != "inherit"  && f)
          functions |= ({ f, function_type(struct[q][f]) });
    mapping q = ([]);
    // Do a stable sort, to avoid rebuilding pgtk.c all the time.

    foreach( functions, function f )
      q[strlen(f)] = (q[strlen(f)]||({})) | ({ f });

    functions = ({});
    foreach(sort(indices( q )), int i)
      functions += sort(q[i]);

    foreach(reverse(functions), string q ) // Evil me. Sort for best compression
      data_offset( q );
  }

  int init_num = 2;
  foreach(sort_dependencies(indices(struct),struct), string w)
  {
    mapping q = struct[w];
    init_num++;
    _inits += ({ "_"+init_num });
    emit_nl( "\nstatic void _"+init_num+"()\n{\n");
    emit_nl("  start_new_program(); /* "+String.capitalize(w)+" */\n");
    to_free += "  free_program( pgtk_"+w+"_program );\n";
    if(q["inherit"])
    {
      emit_nl("   low_inherit( pgtk_"+q["inherit"]+"_program, 0,0,0,0,0);\n");
      m_delete(q, "inherit");
    }
    else
      emit_nl("  ADD_STORAGE(struct object_wrapper);\n");

    emit_nl("   set_init_callback(clear_obj_struct);\n");
    emit( "  {\n" );
    emit_program_block( q, w );
    emit( "  }\n" );
    emit_nl("  add_program_constant((char*)_data+"+
            data_offset(String.capitalize(w)+"\0")+",\n"
	    "                       (pgtk_"+w+"_program = end_program()), 0);"
	    "\n");
    pre += "/*ext*/ struct program *pgtk_"+w+"_program;\n";

    if( do_default_sprintf[w] )
      default_sprintf +=
                     "\n"
                     "static void _pgtk_"+w+"_default_sprintf(INT32 args)\n{\n"
                     " do_default_sprintf(args, "+
                      data_offset(classname(w)+"()")+", "+
                      strlen(classname(w)+"()")+");\n"
                     "}\n";
    string flop = replace(upper_case(w),
			  ({ "GDK_", "GDK" }),
			  ({ "GDK", "GDK_" }));
    type_switch += "#ifdef GTK_TYPE_"+flop+"\n"
                "  if(PGTK_CHECK_TYPE(widget, GTK_TYPE_"+flop+")) "
                "return pgtk_"+w+"_program;\n#endif\n";
    emit_nl("}\n");
  }
  emit_nl( default_sprintf );
  emit_nl( "void pike_module_init()\n{\n");
  foreach( _inits, string i )
    emit_nl( "  "+i+"();\n" );
  emit_nl("}\n\n");
  pre +=
    "#define PGTK_CHECK_TYPE(type_object, otype)       ( "
    "((GtkTypeObject*) (type_object)) != NULL && "
    "PGTK_CHECK_CLASS_TYPE (((GtkTypeObject*) (type_object))->klass, (otype)))\n"
    "#define PGTK_CHECK_CLASS_TYPE(type_class, otype)  ("
    "((GtkTypeClass*) (type_class)) != NULL && "
    "(((GtkTypeClass*) (type_class))->type == (otype)))\n";

  emit_nl("\nstruct program *pgtk_type_to_program(GtkWidget *widget)\n{\n");
  emit_nl(type_switch);
  emit_nl("  return pgtk_object_program;\n}\n\n");

  emit_nl("\nvoid pike_module_exit()\n{\n"+to_free+"}\n\n");
  files = "pgtk.c "+files;
  if(!do_docs)
  {
    werror("Types and function names compressed from %d to %d bytes\n",
           enclen, strlen(data) );
    string q =replace(Stdio.read_bytes(dir+"/pgtk.c.head"),
                      "PROTOTYPES",
                      replace(pre, "/*ext*/ ", "")+
                      "\nstatic const char _data[] =\n"+
                      make_c_string( data )+";");
    pre = replace(pre, "/*ext*/", "extern");
    if(Stdio.read_bytes("prototypes.h") != pre )
    {
      rm("prototypes.h");
      werror("prototypes.h was modified\n");
      Stdio.write_file("prototypes.h", pre);
    }
    q+=(sort(glob_prototypes/"\n")*"\n") + "\n\n" + buffer;
    if( Stdio.read_file("pgtk.c") != q )
    {
      werror("pgtk.c modified\n");
      object outf = Stdio.File("pgtk.c", "rwct");
      outf->write(q);
    }
  }
  rm("files_to_compile");
  Stdio.write_file("files_to_compile", replace(files, ".c", ".o"));

#if constant( get_profiling_info )
  mapping pi = get_profiling_info( object_program(this_object()) )[1];
  array q = ({});
  foreach( indices( pi ), string f )
    q += ({({ pi[f][2], pi[f][0], f })});

  foreach(reverse(sort(q)), array f )
    write( "%-20s %6d  %4.2f\n",
           f[2], f[1], f[0]/1000000.0 );
#endif
}
