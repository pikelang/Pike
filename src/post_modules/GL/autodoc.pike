// Generates autodoc.c, with AutoDoc documentation of
// Pikes OpenGL support, from OpenGL man pages.

#include "constants.pike"
#include "features.pike"
// funcEV, funcV, func_misc, func_cat

string newline(string in) {
  if (in[-1]=='\n') return in+"\n";
  return in+"\n\n";
}

string fix_row(string in) {
  if(in==".BP") return "";
  if(in==".P" || in==".br" || in==".IP") return "\n";
  in = replace(in, ([ "@":"@@",
		      "\\-":"-",
		      "\\&":"", ]) );
  string a, b, c;
  while( sscanf(in, "%s\\f3%s\\fP%s", a, b, c)==3 ) {
    if(has_value(b, "]")) error("] in reference.\n");
    in = a + "@[" + b + "]" + c;
  }

  while( sscanf(in, "%s\\f2%s\\fP%s", a, b, c)==3 ||
	 sscanf(in, "%s\\f2%s\\f1%s", a, b, c)==3 )
    in = a + "@i{" + b + "@}" + c;

  // We should read delim
  while( sscanf(in, "%s$%s$%s", a, b, c)==3 ) {
    if( sscanf(b, "%s sup %s^-^%s", string d, string e, string f)==3 )
      b = d+e+f;
    in = a+b+c;
  }

  return in+"\n";
}

string fix_xml_row(string in) {
  in = fix_row(in);
  in = replace(in, ([ "&":"&amp;", "<":"&lt;", ">":"&gt;" ]) );
  while( sscanf(in, "%s@[%s]%s", string a, string b, string c) )
    in = a + "<ref>" + b + "</ref>" + c;
  while( sscanf(in, "%s@i{%s@}%s", string a, string b, string c) )
    in = a + "<i>" + b + "</i>" + c;
  return in;
}

mapping(string:array(array(string)|string)) docs = ([]);
mapping(string:string) ref_alias = ([]);

string preprocess_man(array(string) rows, string fn) {

  string _args;
  string desc;
  string param;
  string throws;
  string seealso;

  mapping(string:array(string)) prots;

  int(0..1) param_spec;
  int(0..3) tp_state;
  string state;

  foreach(rows, string row) {

    if(has_prefix(row, ".SH ")) {

      switch(tp_state) {
      case 2:
	desc += "<c>";
      case 3:
	desc += "</c></r>";
      case 1:
	desc += "</matrix>@}\n\n";
      }

      param_spec = 0;
      tp_state = 0;
      state = row[4..];
      
      // I don't know if these can occur, but now it can handle it.
      if(state=="DESCRIPTION" && desc)
        desc = newline(desc);
      if(state=="PARAMETERS" && param)
        param = newline(param);
      if(state=="ERRORS" && throws)
        throws = newline(throws);
      if(state=="SEE ALSO" && seealso)
        seealso = newline(seealso);
      continue;
    }
    else if(has_prefix(row, ".ds") || has_prefix(row, ".TH")) {
      state = "";
      continue;
    }

    switch(state) {
    case "NAME":
      if(prots) continue;
      string tmp_name;
      if( sscanf(row, ".B \"%s", tmp_name) != 1 )
	error("No function name found.\n");
      array tmp_names = tmp_name / ", ";
      prots = mkmapping( tmp_names, allocate(sizeof(tmp_names)) );
      break;
    case "C SPECIFICATION":
      if(has_prefix(row, ".nf")) continue;
      if(has_prefix(row, ".ta")) continue;
      if(has_prefix(row, ".fi")) continue;
      _args += " " + row;
      if(has_value(_args, "(") && has_value(_args, ")")) {
        sscanf(_args, "%*s\\f3%s\\fP(%s)", string spec_name, _args);
	array args = ({});
        while( sscanf(_args, "%*s\\fI%s\\fP%s", string arg, _args)==3 )
          args += ({ arg });
	prots[spec_name] = args;
	_args = "";
      }
      break;
    case "ASSOCIATED GETS":
      break;

    case "DESCRIPTION":
      if(!desc) desc="";

      // 0 - out of matrix
      // 1 - empty line
      // 2 - done first cell
      // 3 - begun second cell
      if(has_prefix(row, ".TP")) {
	if(!tp_state) {
	  desc += "\n@xml{<matrix>\n";
	}
	else if(tp_state==2) {
	  desc += "<c></c></r>\n";
	}
	else if(tp_state==3) {
	  desc += "</c></r>\n";
	}
	tp_state = 1;
	continue;
      }
      if(tp_state==1) {
	tp_state = 2;
	desc += "<r><c>" + fix_xml_row(row) + "</c>";
	continue;
      }
      if(tp_state==2) {
	tp_state = 3;
	desc += "<c>" + fix_xml_row(row);
	continue;
      }

      if(tp_state==3) {
	if(has_prefix(row, ".P")) {
	  desc += "</c></r>\n</matrix>@}\n\n";
	  tp_state = 0;
	  continue;
	}
	desc += fix_xml_row(row);
	continue;
      }

      desc += fix_row(row);
      break;
    case "PARAMETERS":
      if(has_prefix(row, ".TP")) {
	param_spec = 1;
	continue;
      }
      if(param_spec) {
 	if(sscanf(row, "%*s\\f2%s\\fP", string p)==2) {
	  param_spec = 0;
	  if(param)
	    param = newline(param);
	  else
	    param = "";
	  param += "@param " + p + "\n\n";
	}
	else
	  error("No param variable\n");
      }
      else
        param += fix_row(row);
      break;
    case "ERRORS":
      if(!throws) throws="@throws\n\n";
      throws += fix_row(row);
    case "SEE ALSO":
      if(!seealso) seealso="@seealso\n\n";
      seealso += fix_row(row);
    }
  }

  mapping names = mkmapping(map(indices(prots), lower_case), indices(prots));
  fn = "gl" + fn;
  string name, new_name;
  if(name = names[fn+"4f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[fn+"3f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[fn+"2f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[fn+"f"])  new_name = name[..sizeof(name)-2];
  else if(name = names[fn+"2"])  new_name = name[..sizeof(name)-2];
  else if(name = names[fn+"fv"]) new_name = name[..sizeof(name)-3];

  if(new_name) {
    foreach(indices(prots), string name)
      ref_alias[name] = new_name;
    prots = ([ new_name : prots[name] ]);
  }

  // Assemble result
  string res = "";
  if(desc) res += newline(desc);
  if(param) res += newline(param);
  if(throws) res += newline(throws);

  foreach(prots; string name; array args) {
    if(!args) error("Empty prototype for %O\n", name);
    docs[name] = ({ args, res });
  }
}

string process_man(string name, string prot_ret, array(string) prot_types) {
  if(!docs[name]) werror("%O is not documented\n", name);
  if(!docs[name]) error("%O is not documented\n", name);

  array args;
  string doc;
  [ args, doc ] = m_delete(docs, name);


  if( sizeof(prot_types) != sizeof(args) )
    error("Prototype argument types and names mismatch in size. %O %O\n", prot_types, args);
  prot_types = (prot_types[*] + " ")[*] + args[*];

  return "@decl " + prot_ret + " " + name + "(" + (prot_types*", ") + ")\n\n" + doc;
}

string comment(string in) {
  array rows = in/"\n";
  rows = " *!" + rows[*];
  in = rows * "\n";
  in[0] = '/';
  return in + "\n */\n";
}


array(string|array(string)) special_234(int mi, int mx, string ty, int|void a)
{
  string baset="float|int";
  array(string) typ=({});

  if(sizeof(Array.uniq(values(ty)))!=1)
    error("Unparsable 234 type '%s'.", ty);

  switch(ty[0]) {
  case 'E':
  case 'B':
  case 'I':
  case 'O':
    baset="int";
    break;
  case 'D':
  case 'F':
    baset="float";
    break;
  case 'R':
  case 'Z':
    break;
  case 'Q':
    break;
  default:
    error("Unknown 234 type '%s'.", ty[0..0]);
  }

  if(a)
    typ+=({"array("+baset+")"});
  else for(int i=0; i<mx; i++) {
    string t = baset;
    if(!i)
      t+="|array("+baset+")";
    if(i>=mi || i>0)
      t+="|void";
    typ+=({t});
  }

  return typ;
}

string document(string name, string features) {

  string ret;
  switch(features[0]) {
  case 'V':
    ret="void";
    break;
  case 'I':
  case 'O':
  case 'E':
    ret="int";
    break;
  case 'S':
    ret="string";
    break;
  default:
    error("%s: Unknown return type '%c'.", name, features[0]);
  }

  array(string) args = ({});
  for(int i=1; i<sizeof(features); i++) {
    switch(features[i]) {
    case 'B':
    case 'E':
    case 'O':
    case 'I':
      args += ({"int"});
      break;
    case 'D':
      args += ({"float"});
      break;
    case 'F':
      args += ({"float"});
      break;
    case 'Z':
      args += ({"float|int"});
      break;
    case '+':
      int mi, mx;
      switch(sizeof(features[i+1..])) {
      case 1:
	mi = 1; mx = 4; break;
      case 2:
	mi = 1; mx = 2; break;
      case 3:
	mi = 2; mx = 4; break;
      case 4:
	mi = 3; mx = 4; break;
      default:
	error("Can't understand + followed by %d chars.\n", sizeof(features[i+1..]));
      }
      args += special_234(mi, mx, features[i+1..]);
      i=sizeof(features);
      break;
    case '#':
    case '!':
    case '=':
      args += special_234(sizeof(features[i+1..]), sizeof(features[i+1..]),
			  features[i+1..]);
      i=sizeof(features);
      break;
    case '@':
      args += special_234(1, 1, features[i+1..]);
      i=sizeof(features);
      break;
    case '[':
      sscanf(features[i+1..], "%d%s", int nn, string rst);
      args += special_234(nn, nn, rst, 1);
      i=sizeof(features);
      break;
    case 'w':
    case 'h':
    case 'f':
    case 't':
    case 'i':
      args += ({"object|mapping(string:object)"});
      break;

    default:
      error("%s: Unknown parameter type '%c'.", name, features[i]);
    }
  }

  string x = comment(process_man(name, ret, args));
  //  write(x+"\n");
  return x;
}

void prefetch() {
  foreach(glob("*.3gl", get_dir("release/xc/doc/man/GL/gl/")), string fn) {
    array(string) rows = Stdio.read_file("release/xc/doc/man/GL/gl/"+fn)/"\n";
    preprocess_man(rows, fn[..sizeof(fn)-5]);
  }
}

string first_page() {

  string ret = "@module GL\n\nNot implemented OpenGL methods:\n\n";

  ret += "@xml{<matrix>\n";
  foreach(sort(indices(docs)), string name)
    ret += "<r><c>" + name + "</c></r>\n";
  ret += "</matrix>@}\n";
  return comment(ret);
}

mapping(string:array(string)) refs = ([]);

void fix_refs() {
  werror("Resolving references to renamed methods.\n");
  foreach(docs; string func; array jox) {
    string out = "";
    string in = jox[1];
    while( sscanf(in, "%s@[%s]%s", string a, string b, string c)==3 ) {
      if(ref_alias[b]) {
	werror("Remapped %O (%O)\n", b, func);
	b = ref_alias[b];
      }
      out += a + "@[" + b + "]";
      in = c;
    }
    jox[1] = out + in;
  }

  werror("Finding constant references.\n");
  foreach(indices(constants), string name) {
    array r = ({});
    foreach(docs; string func; array jox)
      if(has_value(jox[1], name)) r += ({ func });
    if(sizeof(r))
      refs[name] = r;
  }
}

void main() {
  werror("Reading manpages.\n");
  prefetch();
  fix_refs();

  werror("Building documentation.\n");
  string doc = "";

  foreach( func_misc, array func) {
    if(catch {
      doc += document(func[0], func[1]) + "\n";
    }) werror("Failed with %O\n", func[0]);
  }

  foreach( funcV, string func)
    doc += document(func, "V") + "\n";

  foreach( funcEV, string func)
    doc += document(func, "VE") + "\n";

  foreach( sort(indices(constants)), string name ) {
    doc += "/*!@decl constant "+name+" " + constants[name] + "\n";
    if(refs[name])
      doc += " *! Used in " +
	String.implode_nicely(map(refs[name],
				  lambda(string in) {
				    return "@[" + in + "]";
				  })) + "\n";
    doc += " */\n\n";
  }

  doc = "\n/* AutoDoc generated from OpenGL man pages\n$Id"
    "$ */\n\n" + first_page() + "\n\n" + doc +
    "\n/*! @endmodule\n */\n\n";

  werror("Writing result file.\n");
  Stdio.write_file("autodoc.c", doc);
  werror("Done.\n");
}
