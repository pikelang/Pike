/*
 * $Id: mklibpike.pike,v 1.2 2005/01/03 17:59:05 grubba Exp $
 *
 * Create strapping code for a list of symbols in pike.so,
 * and the pike headerfiles.
 *
 * Henrik Grubbström 2005-01-03
 */

constant default_headers = ({
  "global.h",
  "pike_embed.h",
});

class C_Include_Handler(array(string) search_path)
{
  string handle_include(string f, string current_file, int local_include)
  {
    if (!local_include) {
      // Ignore system header files.
      return "";
    } else {
      foreach(search_path, string path) {
	path = combine_path(path, f);
	if (file_stat(path)) {
	  return path;
	}
      }
    }
    return 0;	// Header not found.
  }

  string read_include(string f)
  {
    if (!sizeof(f)) return "";	// System header.
    return Stdio.read_file(f);
  }
}

//! Returns a mapping from symbol to a tuple of return type and parameters.
mapping(string:array(array(Parser.C.Token))) parse(array(Parser.C.Token) tokens)
{
  mapping(string:array(array(Parser.C.Token))) res = ([]);
  int i;
  int start;
  for (start=i=0; i < sizeof(tokens); i++) {
    array(Parser.C.Token)|object(Parser.C.Token) token = tokens[i];
    if ((objectp(token)) && (token->text == ";")) {
      if (i > start + 2) {
	if (objectp(tokens[start]) && (tokens[start]->text == "typedef")) {
	  // Ignore.
	} else {
	  array(Parser.C.Token)|Parser.C.Token last;
	  if (arrayp(last = tokens[i-1]) && sizeof(last)) {
	    if (last[0]->text == "(") {
	      // Function declaration.
	      res[(string)tokens[i-2]] = ({
		tokens[start..i-3],
		last,
	      });
	    } else {
	      // { : possible inline function, but probably an struct or enum.
	      // [ : array declaration.
	      // Ignored for now.
	    }
	  }
	}
      } else {
	// Too few tokens to be a function.
      }
      start = i+1;
    }
  }
  return res;
}

int main(int argc, array(string) argv)
{
  array(string) headers = default_headers;
  array(string) srcdirs = ({ ".", "/home/grubba/src/Pike/7.7/src", });
  array(string) symbols = ({
    "init_pike",
    "init_pike_runtime",
    "pike_set_default_master",
    "set_pike_debug_options",
  });

  Stdio.File out = Stdio.stdout;

  // FIXME: Argument parsing here.

  // Suggested arguments:
  //
  //   -h	Help.
  //   -i	Add include file.
  //   -I	Add include directory.
  //   -o	Specify output filename.
  //   -s	Add single symbol.
  //   -S	Specify symbol file.

  string raw = cpp(sprintf("%{#include \"%s\"\n%}", headers), "include_all.c",
		   0,
		   C_Include_Handler(srcdirs));

  array(Parser.C.Token|array) tokens =
    Parser.C.strip_line_statements(
      Parser.C.hide_whitespaces(
        Parser.C.group(Parser.C.tokenize(Parser.C.split(raw)))
      )
    );

  mapping(string:array(array(Parser.C.Token))) symbol_info = parse(tokens);

  int fail;
  foreach(symbols, string sym) {
    array(array(Parser.C.Token)) info = symbol_info[sym];
    if (!info) {
      werror("Symbol %O not found!\n", sym);
      fail = 1;
      continue;
    }
    string rettype = String.trim_whites(Parser.C.simple_reconstitute(info[0]));
    string params = Parser.C.simple_reconstitute(info[1]);
    out->write("static %s (*vec_%s)%s;\n"
	       "%s %s%s\n"
	       "{\n"
	       "  %svec_%s(",
	       rettype, sym, params,
	       rettype, sym, params,
	       (rettype=="void")?"":"return ", sym);
    
    array(array(Parser.C.Token)) args = info[1][1..sizeof(info[1])-2]/({","});
    foreach(args; int n; array(Parser.C.Token) arg) {
      array(Parser.C.Token)|Parser.C.Token name = arg[-1];
      if (arrayp(name)) {
	if (name[0]->text == "(") {
	  // Function pointer arg.
	  name = arg[-2];
	  if (arrayp(name)) {
	    // Expect something like ( * foo ).
	    // FIXME: Support addresses to function pointers?
	    name = name[2];
	  } else {
	    // Syntax error.
	    werror("Failed to find variable name in %s\n",
		   Parser.C.simple_reconstitute(arg));
	    fail = 1;
	  }
	} else {
	  // FIXME: Support array notation?
	  werror("Failed to find variable name in %s\n",
		 Parser.C.simple_reconstitute(arg));
	  fail = 1;
	}
      }
      out->write("%s%s", n?", ":"", name->text);
    }
    out->write(");\n"
	       "}\n\n");
  }
  return fail;
}

