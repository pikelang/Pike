#! /usr/bin/env pike
// -*- Pike -*-
// $Id: cgrep.pike,v 1.9 2003/01/20 14:59:27 jhs Exp $

constant description = "Context aware grep.";

// Future expansions:
// - We already stream files, so we should be able to stream from stdin.
// - Support for more file formats. Python should be easy, since there
//   is alrady a Python parser in Pike. It just needs stream support.
// - Report in which function the match was made.
// - Support grep contexts (-A -B -C). Rewrite lines to an object.
// - Mimic -d behaviour.
// - Implement -Z and -b.
// - Make something smart with macro definitions. Right now they are
//   ignored.
// - Implement support for grepping for the definition of a symbol,
//   be it a function definition, variable declaration, struct/enum/typedef
//   definition or a macro definition.

// Search parameters
int(0..2) in_token; // 1=is_token, 2=in_token
int(0..2) in_string; // 1=is_string, 2=in_string
int(0..1) in_comment;
int(0..1) case_insensitive;
string target;

// Presentation
int(0..1) show_lineno;
int(0..1) show_fn;
int(0..1) verbose;
int(0..1) quiet;
int(0..1) supress_match;
int(0..3) fn_only; // 1=with, 2=without, 3=count

class File {

  void create(string _fn) {
    fn = _fn;
  }

  string fn;
  array(string) lines = ({});
  int line;

  mapping parse_state = ([]);
  string queue = "";
  int matches;

  void found() {
    if(quiet) exit(0);
    matches++;
    if(supress_match) return;
    if(show_fn) write(fn+":");
    if(show_lineno) write("%d:", line+1);
    write("%s\n", lines[line]);
  }

  array(string) split(string, void|mapping);
  void grep(array(string));

  void feed(string data) {
    // Taking care of the line state
    data = queue+data;
    array(string) newlines = data/"\n";
    queue = newlines[-1];
    newlines = newlines[..sizeof(newlines)-2];
    data = newlines*"\n"+"\n";
    lines += newlines;

    if(case_insensitive)
      data = lower_case(data);

    grep(split(data, parse_state));
  }

  int close() {
    if(sizeof(queue)) feed("\n");
    if(fn_only) {
      if(fn_only==1 && matches)
	write("%s\n", fn);
      else if(fn_only==2 && !matches)
	write("%s\n", fn);
      else if(fn_only==3 && matches)
	write("%s:%d\n", fn, matches);
    }
    return matches;
  }
}

class PikeFile {
  inherit File;

  array(string) split(string data, mapping state) {
    // For some reason the Pike parser leaves parts of its
    // end sentinel in the output data.
    array(string) ret = Parser.Pike.split(data, state);
    if(state->in_token) return ret;
    if(ret[-1]=="\n") return ret[..sizeof(ret)-2];
    ret[-1] = ret[-1][..sizeof(ret[-1])-2];
    return ret;
  }

  void grep(array(string) tokens) {
    int old;
    foreach(tokens; int i; string token) {
#if 0
      if(line!=old) {
        catch(write("\n%4d:%s\n", line+1, lines[line]));
	old=line;
      }
      write("%O ", token);
#endif
      if(in_token==1 && token==target) {
	found();
	line += String.count(token, "\n");
	continue;
      }
      if(token[0]=='"') {
	if(in_string==1 && token[1..sizeof(token)-2]==target)
	  found();
	else if(in_string==2 && has_value(token[1..sizeof(token)-2], target))
	  found();
	line += String.count(token, "\n");
	continue;
      }
      if(token[..1]=="//") {
	if(in_comment && has_value(token[2..], target))
	  found();
	continue;
      }
      if(token[..1]=="/*") {
	foreach(token[2..sizeof(token)-3]/"\n", string l) {
	  if(in_comment && has_value(l, target))
	    found();
	  line++;
	}
	line--;
	continue;
      }
      if(token[0]=='#') {
	line += String.count(token, "\n");
	continue;
      }
      if(in_token==2 && has_value(token, target)) {
	found();
	continue;
	line += String.count(token, "\n");
      }
      line += String.count(token, "\n");
    }
  }
}

class CFile {
  inherit PikeFile;

  array(string) split(string data, mapping state) {
    // For some reason the C parser leaves parts of its
    // end sentinel in the output data.
    array(string) ret = Parser.C.split(data, state);
    if(state->in_token) return ret;
    if(ret[-1]=="\n") return ret[..sizeof(ret)-2];
    ret[-1] = ret[-1][..sizeof(ret[-1])-2];
    return ret;
  }
}

void msg(string x, mixed ... y) {
  if(verbose)
    werror(x, @y);
}

int handle_file(string path, string fn) {
  if(Stdio.is_dir(path)) {
    msg("%O is a directory.\n", path);
    return 0;
  }
  Stdio.File in;
  catch { in = Stdio.File(path, "r"); };
  if(!in) {
    msg("File %O could not be read.\n", path);
    return 0;
  }

  File f;
  array(string) parts = fn/".";
  string suffix = parts[-1];
  if(suffix=="in" && sizeof(parts)>1)
    suffix = parts[-2];
  if( (< "pike", "pmod" >)[suffix] )
    f = PikeFile(fn);
  else if( (< "h", "c", "cmod" >)[suffix] )
    f = CFile(fn);

  // We could add some heuristics here looking for #! rows ending
  // in pike and emacs -*- things with c or pike in them.

  if(!f) {
    msg("No tokenizer found for %O\n", path);
    return 0;
  }

  string data;
  while(sizeof(data=in->read(4096)))
    f->feed(data);
  return f->close();
}

void usage() {
  werror("Usage: cgrep [OPTION]... TEXT FILE...\n"
	 "Try `grep --help' for more information.\n");
  exit(2);
}

int main(int num, array(string) args) {

  int(0..1) recurse;
  int(0..1) summarize;

  foreach(Getopt.find_all_options(args, ({
    ({ "is_token",   Getopt.NO_ARG, "-T,--is-token"/","   }),
    ({ "in_token",   Getopt.NO_ARG, "-t,--in-token"/","   }),
    ({ "in_string",  Getopt.NO_ARG, "-s,--in-string"/","  }),
    ({ "is_string",  Getopt.NO_ARG, "-S,--is-string"/","  }),
    ({ "in_comment", Getopt.NO_ARG, "-c,--in-comment"/"," }),
    ({ "summarize",  Getopt.NO_ARG, "--summarize"         }),
    ({ "help",       Getopt.NO_ARG, "--help"              }),
    ({ "version",    Getopt.NO_ARG, "--version"           }),
    ({ "verbose",    Getopt.NO_ARG, "--verbose"           }),
    ({ "quiet",      Getopt.NO_ARG, "-q,--quiet"/","      }),

    ({ "-n", Getopt.NO_ARG, "-n,--line-number"/","    }),
    ({ "-i", Getopt.NO_ARG, "-i,--ignore-case"/","    }),
    ({ "-H", Getopt.NO_ARG, "-H,--with-filename"/","  }),
    ({ "-r", Getopt.NO_ARG, "-r,--recursive"/","      }),
    ({ "-l", Getopt.NO_ARG, "-l,--files-without-match"/"," }),
    ({ "-L", Getopt.NO_ARG, "-L,--files-with-matches"/","  }),
    ({ "-c", Getopt.NO_ARG, "--count"                 }),
  })), array opt) {
    switch(opt[0]) {
    case "is_token": in_token=1; break;
    case "in_token": in_token=2; break;
    case "is_string": in_string=1; break;
    case "in_string": in_string=2; break;
    case "in_comment": in_comment=1; break;
    case "summarize": summarize=1; break;
    case "verbose": verbose=1; break;
    case "quiet": quiet=1; break;
    case "-n": show_lineno=1; break;
    case "-i": case_insensitive=1; break;
    case "-H": show_fn=1; break;
    case "-r": recurse=1; break;
    case "-l": supress_match=1; fn_only=1; break;
    case "-L": supress_match=1; fn_only=2; break;
    case "-c": supress_match=1; fn_only=3; break;
    case "help":
      write(doc);
      exit(0);
    case "version":
      write( replace(version, ([ "$":"", "Revision: ":"" ])) );
      exit(0);
    }
  }
  args = Getopt.get_args(args)[1..];

  if(!sizeof(args)) usage();

  target = args[0];
  if(case_insensitive) target = lower_case(target);

  array(string) files = args[1..];
  if(recurse && !sizeof(files)) files = ({ "." });
  if(sizeof(files)>1 || recurse) show_fn = 1;
  if(!sizeof(files)) usage();

  int matches;
  foreach(files, string fn)
    if(recurse && Stdio.is_dir(fn))
      foreach(Filesystem.Traversion(fn); string path; string fn)
	matches += handle_file(path+fn, fn);
    else
      matches += handle_file(fn, basename(fn));

  if(summarize)
    write("Total matches: %d\n", matches);

  exit(!matches);
}

constant doc = #"Usage: cgrep [OPTION]... TEXT FILE...
Search for TEXT in each FILE.
Example: grep -i -s hello menu.h main.c prog.pike

Text matching:
  -T, --is-token            TEXT is a complete token
  -t, --in-token            TEXT is part of a token
  -S, --is-string           TEXT is a complete string literal
  -s, --in-string           TEXT is part of a string literal
  -c, --in-comment          TEXT is part of a comment
  -i, --ignore-case         ignore case distinctions
			    
Miscellaneous:		    
      --version             print version information and exit
      --help                display this help and exit
			    
Output control:		    
  -n, --line-number         print line number with output lines
  -H, --with-filename       print the filename for each match
      --verbose             output error messages
  -q, --quiet               suppress all normal output
  -r, --recursive           recurse into directories
  -L, --files-without-match only print FILE names containing no match
  -l, --files-with-matches  only print FILE names containing matches
      --count               only print a count of matching lines per FILE
      --summarize           print a summary of the number of matches
";

constant version = #"cgrep $Revision: 1.9 $
A token based grep with UI stolen from GNU grep.
By Martin Nilsson 2003.
";
