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
		      "\\&":"",
		      "\\(<=":"<=",
		      "\\(>=":">=",
		      "\\(!=":"!=", ]) );
  string a, b, c;
  while( sscanf(in, "%s\\f3%s\\fP%s", a, b, c)==3 ) {
    if(has_value(b, "]")) error("] in reference.\n");
    in = a + "@[" + b + "]" + c;
  }

  while( sscanf(in, "%s\\f2%s\\fP%s", a, b, c)==3 ||
	 sscanf(in, "%s\\fI%s\\fP%s", a, b, c)==3 ||
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

array(string) not_documented = ({});
array(string) not_implemented = ({});
mapping(string:array(array(string)|string)) docs = ([]);
mapping(string:string) ref_alias = ([
  "glColor3": "glColor",
  "glColor4": "glColor",
  "glEvalCoord1": "glEvalCoord",
  "glEvalCoord2": "glEvalCoord",
  "glEvalPoint1": "glEvalPoint",
  "glEvalPoint2": "glEvalPoint",
  "glRasterPos2": "glRasterPos",
  "glRasterPos3": "glRasterPos",
  "glRasterPos4": "glRasterPos",
  "glMapGrid1": "glMapGrid",
  "glMapGrid2": "glMapGrid",
  "glTexCoord1": "glTexCoord",
  "glTexCoord2": "glTexCoord",
  "glTexCoord3": "glTexCoord",
  "glTexCoord4": "glTexCoord",
]);

string preprocess_man(array(string) rows, string fn)
{
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
        while( sscanf(_args, "%*s\\fI%s\\fP%s", string arg, _args)==3 ) {
          if (has_prefix(arg, "*")) arg = arg[1..];
          args += ({ arg });
        }
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

  if (fn == "evalmesh") {
    werror("Raw prots: %O\n", prots);
  }

  mapping names = mkmapping(map(indices(prots), lower_case), indices(prots));
  fn = "gl" + fn;
  string name, new_name;
  if (name = ([
        "glget": "glGet",
        // "glevalmesh": "glEvalMesh",	// Documents EvalMesh1 and EvalMesh2.
        "gledgeflag": "glEdgeFlag",
      ])[fn]) {
    // glGetFloatv et al ==> glGet (cf top.c).
    new_name = name;
    name = sort(indices(prots))[0];
  } else if ((< "glevalmesh" >)[fn]) {
    // Keep as glEvalMesh1() and glEvalMesh2().
    werror("Skip canonicalization for %O.\n", fn);
  } else if(name = names[fn+"4f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[fn+"3f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[fn+"2f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[fn+"f"])  new_name = name[..sizeof(name)-2];
  else if(name = names[fn+"2"])  new_name = name[..sizeof(name)-2];
  else if(name = names[fn+"fv"]) new_name = name[..sizeof(name)-3];

  if(new_name) {
    foreach(indices(prots), string name) {
      if (name != new_name) {
        ref_alias[name] = new_name;
      }
    }
    prots = ([ new_name : prots[name] ]);
  } else if (sizeof(prots) > 1) {
    werror("Warning: New canonical name for %O not found.\n"
           "Known names: %O\n", fn, names);
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


  if( sizeof(prot_types) != sizeof(args) ) {
    if (name == "glClipPlane") {
      doc = "@decl " + prot_ret + " glClipPlane(" +
        prot_types[0] + " " + args[0] + ", "
        "float equation_0, float equation_1, float equation_2, "
        "float equation_3)\n\n" + doc;
      prot_types = ({ prot_types[0], "array(4:float)" });
    } else {
      error("Prototype argument types and names mismatch in size. %O %O\n",
            prot_types, args);
    }
  }

  // Zap arguments not used by Pike.
  foreach(prot_types; int i; string prot_type) {
    if (prot_type == "void") {
      prot_types[i] = "";
      args[i] = "";
    }
  }
  prot_types -= ({ "" });
  args -= ({ "" });

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

string document(string name, string features)
{
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
  case ']':
    if (features[1] == 'I') {
      ret = "array(int)";
    } else {
      ret = "array";
    }
    features = features[1..];
    break;
  case '+':
    if (has_prefix(features, "+Z") || has_prefix(features, "+Q")) {
      ret = "int|float|array(int)|array(float)";
      features = features[1..];	// Make later code happy.
      break;
    }
    // FALL_THRU
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
    case 'Q': case 'Z':
      args += ({"float|int"});
      break;
    case 'V':
      args += ({ "void" });	// Argument ignored in Pike.
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
    case '-':
      args[-1] += " ...";
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
    case ']':
      if (features[i+1] == 'I') {
        args += ({ "array(int)" });
      } else {
        args += ({ "array" });
      }
      i++;
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

    case '&':
      args += ({ "System.Memory" });
      break;

    default:
      error("%s: Unknown parameter type '%c'.\n", name, features[i]);
    }
  }

  string x = comment(process_man(name, ret, args));
  //  write(x+"\n");
  return x;
}

void prefetch()
{
  if( !file_stat( "release/xc/doc/man/GL/gl/" ) )
  {
    werror( "Need OpenGL man pages unpacked in present working directory.\n"
            "Download ftp://ftp.sgi.com/sgi/opengl/doc/mangl.tar.Z first.\n"
            "sgi.com is defunct there is still (2026-01-10) a mirror at\n"
            "https://ftp.jurassic.nl/mirrors/ftp.sgi.com/opengl/doc/\n"
            );
    exit( 1 );
  }
  foreach(glob("*.3gl", get_dir("release/xc/doc/man/GL/gl/")), string fn) {
    array(string) rows = Stdio.read_file("release/xc/doc/man/GL/gl/"+fn)/"\n";
    preprocess_man(rows, fn[..sizeof(fn)-5]);
  }
}

string first_page()
{
  string ret = #"@module GL

OpenGL glue. All method and constant names have been kept close to their low
level counterparts for easy adoption of OpenGL code from other languages and
examples off the web. Superfluous suffixes specifying the number and types of
arguments have been dropped, though.
";
  if (sizeof(not_implemented)) {
    ret += #"
OpenGL methods still missing in the Pike API:

@xml{<matrix>
";
    foreach( sort( (array)not_implemented ), string name )
      ret += "<r><c>" + name + "</c></r>\n";
    ret += "</matrix>@}\n";
  }
  if (sizeof(not_documented)) {
    ret += #"
@fixme
Methods available, but lacking documentation:

@xml{<matrix>
";
    foreach( sort( not_documented ), string name )
      ret += "<r><c>" + name + "</c></r>\n";
    ret += "</matrix>@}";
  }
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
	werror("Remapped %s (to %s)\n", b, func);
	b = ref_alias[b];
      }
      // else if( !GL[b] ) werror( "Maybe not %s?\n", b );
      out += a + "@[" + b + "]";
      in = c;
    }
    jox[1] = out + in;
  }

  foreach(docs; string func; array jox) {
    string out = "";
    string in = jox[1];
    while( sscanf(in, "%s<ref>%s</ref>%s", string a, string b, string c)==3 ) {
      if(ref_alias[b]) {
        werror("Remapped %s (to %s)\n", b, func);
        b = ref_alias[b];
      }
      // else if( !GL[b] ) werror( "Maybe not %s?\n", b );
      out += a + "<ref>" + b + "</ref>";
      in = c;
    }
    jox[1] = out + in;
  }

  werror("Finding constant references.\n");
  foreach(indices(constants), string name) {
    array r = ({});
    foreach(docs; string func; array jox) {
      if(has_value(jox[1], name)) r += ({ func });
      foreach(({({ "GL_", "_BIAS", "GL_c_BIAS" }),
                ({ "GL_", "_SCALE", "GL_c_SCALE" }),
                ({ "GL_AUX", "", "GL_AUX" }),
                ({ "GL_CLIP_PLANE", "", "GL_CLIP_PLANE" }),
                ({ "GL_LIGHT", "", "GL_LIGHT" }),
                ({ "GL_MAP1_", "", "GL_MAP1_" }),
                ({ "GL_MAP2_", "", "GL_MAP2_" }),
                ({ "GL_PIXEL_MAP_", "", "GL_PIXEL_MAP_c_TO_c" }),
                ({ "GL_POLYGON_OFFSET", "", "GL_POLYGON_OFFSET" }),
                ({ "GL_TEXTURE_GEN_", "", "GL_TEXTURE_GEN_" }),
                ({ "glEvalMesh", "", "glEvalMesh" }),
              }), [string prefix, string suffix, string alt_name]) {
        if (has_prefix(name, prefix) && has_suffix(name, suffix) &&
            has_value(jox[1], alt_name)) {
          r += ({ func });
        }
      }
    }
    if(sizeof(r))
      refs[name] = r;
  }
}

void main()
{
  werror("Reading manpages.\n");
  prefetch();
  fix_refs();

  werror("Building documentation.\n");
  string doc = "";

  foreach( func_misc + ({
             ({"glCallLists", "VVVI-"}),
             ({"glDeleteTextures", "VVI-"}),
             ({"glFrustum", "VDDDDDD"}),
             ({"glGenTextures", "]IIV"}),
             ({"glDrawElements", "VEVV]I"}),
             ({"glMapGrid", "VIFFIFF"}),
             ({"glGet", "+QIV" }),
           }), array func)
  {
    if(catch { doc += document(func[0], func[1]) + "\n"; })
    {
      werror("Failed with %s [%s]\n", @func);
      not_documented += ({ func[0] });
    }
  }

  foreach( funcV, string func)
    doc += document(func, "V") + "\n";

  foreach( funcEV, string func)
    doc += document(func, "VE") + "\n";

  not_implemented = indices(docs) - not_documented;
  foreach( sort(indices(constants)), string name )
  {
    array relevant = refs[name] || ({});
    relevant -= not_implemented;
    // if( sizeof(relevant) )
    {
      array r = sort(map(relevant,
                         lambda(string in) { return "@[" + in + "]"; }));
      doc += sprintf("/*!@decl constant %s = %d\n", name, constants[name]);
      if (sizeof(r)) {
        doc += sprintf(" *! Used in %s\n", String.implode_nicely(r));
      }
      doc += " */\n\n";
    }
  }

  doc = #"/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/\n\n/* AutoDoc generated from OpenGL man pages */"
    "\n\n" + first_page() + "\n\n" + doc +
    "\n/*! @endmodule\n */\n\n";

  // Get rid of white space at end of line.
  doc = replace(doc, "        \n", "\n");
  doc = replace(doc, "    \n", "\n");
  doc = replace(doc, "  \n", "\n");
  doc = replace(doc, " \n", "\n");
  doc = replace(doc, "\t\n", "\n");
  while (has_suffix(doc, "\n\n")) {
    doc = doc[..<1];
  }

  // Fixup various references.
  doc = replace(doc, ([
                  "@[glEvalMesh]": "@[glEvalMesh1] and @[glEvalMesh2]",
                  "@[GL_AUX]": "@[GL_AUX0] through @[GL_AUX3]",
                  "<ref>GL_AUX</ref>":
                  "<ref>GL_AUX0</ref> through <ref>GL_AUX3</ref>",
                  "@[GL_MAP1_]@i{x@}": "@tt{GL_MAP1_@i{x@}@}",
                  "@[GL_MAP2_]@i{x@}": "@tt{GL_MAP2_@i{x@}@}",
                  "@[GL_CLIP_PLANE]@i{i@}": "@tt{GL_CLIP_PLANE@i{i@}@}",
                  "<ref>GL_CLIP_PLANE</ref><i>i</i>":
                  "<tt>GL_CLIP_PLANE<i>i</i></tt>",
                  "@[GL_LIGHT]@i{i@}": "@tt{GL_LIGHT@i{i@}@}",
                  "<ref>GL_LIGHT</ref><i>i</i>": "<tt>GL_LIGHT<i>i</i></tt>",
                  "@[GL_LIGHT]i": "@tt{GL_LIGHT@i{i@}@}",
                  "@[GL_POLYGON_OFFSET]": "@tt{GL_POLYGON_OFFSET@}",
                  "@[GL_TEXTURE_GEN_]@i{x@}": "@tt{GL_TEXTURE_GEN_@i{x@}@}",
                  "<ref>GL_PIXEL_MAP_c_TO_c</ref>":
                  "<tt>GL_PIXEL_MAP_<i>c</i>_TO_<i>c</i></tt>",
                  "<ref>GL_c_BIAS</ref>": "<tt>GL_<i>c</i>_BIAS</tt>",
                  "<ref>GL_c_SCALE</ref>": "<tt>GL_<i>c</i>_SCALE</tt>",
                ]));

  werror("Writing result file.\n");
  Stdio.write_file("autodoc.c", doc);
  werror("Done.\n");
}
