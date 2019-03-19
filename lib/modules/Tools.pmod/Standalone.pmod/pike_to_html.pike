#pike __REAL_VERSION__

//! Convert Pike code to HTML with syntax highlighting
//!
//! @code
//! pike -x pike_to_html /path/to/file.pike > file.html
//! @endcode

constant description = "Pike code to syntax highlighted HTML converter";

constant HELP = #"
Converts either a string of code or a Pike file into syntax highlighted HTML.
The generated HTML will be written to stdout.

Usage:
  pike -x pike_to_html \"path to file or string of code\"
";

string delimiters = ",;:.>-+=()|[]&%#!<>^~*/`{}\\";
string res_words = "if|else|while|for|foreach|do|return|continue|break|"
                   "switch|case|default|goto|catch|lambda|gauge|inherit|import";
string type = "enum|float|int|typedef|void|object|class|mapping|string|"
              "array|multiset|mixed|function|program|bool";
string modifiers = "public|protected|private|static|constant|local|final|"
                   "optional|variant";
string constants  = "UNDEFINED|__LINE__|__FILE__|__DIR__|__func__|"
                    "__VERSION__|__MAJOR__|__MINOR__|__BUILD__|"
                    "__REAL_VERSION__|__REAL_MAJOR__|__REAL_MINOR__|"
                    "__REAL_BUILD__|__DATE__|__TIME__|__AUTO_BIGNUM__|__NT__|"
                    "__PIKE__|__amigaos__|true|false|this|this_program";

string namespaces = #"ADT
Arg
Array
Audio
Builtin
Bz2
Cache
Calendar
Charset
Colors
CommonLog
Crypto
DVB
Debug
DefaultCompilerEnvironment
Error
Filesystem
Float
Function
Fuse
GDK
GDK2
GL
GLU
GLUE
GLUT
GSSAPI
GTK
GTK2
Gdbm
Geography
Getopt
Git
Gmp
Gnome2
Graphics
Gz
HPack
HTTPLoop
Image
Int
Java
Languages
Local
Locale
MIME
Mapping
Math
Msql
Multiset
Mysql
NetUtils
Nettle
Object
Odbc
PDF
Pango
Parser
Pike
Pipe
Postgres
Process
Program
Protocols
Regexp
Remote
SANE
SDL
SSL
Search
Serializer
Shuffler
Sql
Standards
Stdio
String
System
Thread
Tools
Unicode
VCDiff
Val
Web
Yabu
Yp
ZXID
_Ffmpeg
__builtin";

protected multiset(string) delims, reserved, types, consts, ns, mods;

final int(0..1) main(int argc, array(string) argv)
{
  if (argc == 1) {
    werror("Missing argument.%s\n", HELP);
    return 1;
  }

  string h = #"<!doctype html>
<html>
  <head>
    <meta charset='utf-8'>
    <title>%s</title>
    <style>
      html, body { font-family: sans-serif; font-size: 100%%; }
      pre { font-family: menlo, monospace; font-size: .8rem; }
      .comment { color: #999; }
      .type { color: #00007B; font-weight: bold; }
      .string { color: #037F00; }
      .macro { color: #B04700; font-weight: bold; }
      .lang, .const, .mod { color: #863069; font-weight: bold; }
      .const { color: #0000ff; }
      .delim { color: #00008e; }
      .ns { font-weight: bold; }
      .nested { opacity: .7; transition: opacity .2s ease-in; }
      .nested:hover { opacity: 1; }
    </style>
  </head>
  <body>
    <pre lang='pike'>%s</pre>
  </body>
</html>";

  //string code = Stdio.exist(argv[1]) ? Stdio.read_file(argv[1]) : argv[1];

  string code, title;

  if (Stdio.is_file(argv[1])) {
    code = Stdio.read_file(argv[1]);
    title = basename(argv[1]);
  }
  else {
    code = argv[1];
    title = "Pike code";
  }

  write(sprintf(h, title, convert(code)));
  return 0;
}

//! Turn @[code] into HTML.
//!
//! The following css classes will be used:
//!
//! @ul
//!  @item
//!   Delimiters:        delim
//!  @item
//!   Reserved words:    lang
//!  @item
//!   Data types:        types
//!  @item
//!   Constants:         const
//!  @item
//!   Modifiers:         mods
//!  @item
//!   Root namespaces:   ns
//!  @item
//!   Strings:           string
//!  @item
//!   Comments:          comment
//!  @item
//!   Macros:            macro
//! @endul
//!
//! @param code
string convert(string code)
{
  delims   = (multiset)(delimiters/1);
  reserved = (multiset)(res_words/"|");
  types    = (multiset)(type/"|");
  consts   = (multiset)(constants/"|");
  ns       = (multiset)(namespaces/"\n");
  mods     = (multiset)(modifiers/"|");
  return make_html(Parser.Pike.split(code));
}

#define ADDTAG(TYP) add("<span class='" + TYP + "'>" +  \
                        replace(tok, ([ "&" : "&amp;",  \
                                        "<" : "&lt;",   \
                                        ">" : "&gt;" ])) + "</span>")
#define ADD() add(replace(tok, ([ "&" : "&amp;", "<" : "&lt;", ">" : "&gt;" ])))


protected string make_html(array(string) tokens)
{
  String.Buffer buf = String.Buffer();
  function add = buf->add;

  tokens = ({ 0 }) + tokens + ({ "  ", "  " });

  int i;
  int len = sizeof(tokens) - 2;

  for (i = 1; i < len; i++) {
    string tok = tokens[i];

    if (delims[tok]) {
      ADDTAG("delim");
    }
    else if (mods[tok]) {
      ADDTAG("mod");
    }
    else if (reserved[tok]) {
      ADDTAG("lang");
    }
    else if (types[tok]) {
      ADDTAG("type");
    }
    else if (consts[tok]) {
      ADDTAG("const");
    }
    else if (ns[tok]) {
      ADDTAG("ns");
    }
    else {
      if (sizeof(tok) > 1) {
        switch (tok[0])
        {
          case '\'':
            /* fallthrough */
          case '"':
            ADDTAG("string");
            break;

          case '/':
            ADDTAG("comment");
            break;

          case '#':
            if (tok[1] == '"') {
              ADDTAG("string");
            }
            else {
              array(string) ntoks = Parser.Pike.split(tok[1..]);
              tok = "#" + ntoks[0];
              ADDTAG("macro");

              if (ntoks[-1] == "\n")
                ntoks = ntoks[..<1];

              if (sizeof(ntoks) > 2 && (< "<", "\"" >)[ntoks[2]]) {
                tok = ntoks[2..]*"";
                add(ntoks[1]);
                ADDTAG("string");
              }
              else if (ntoks[0] == "!") {
                tok = ntoks[1..]*"";
                ADDTAG("macro");
              }
              else if (sizeof(ntoks) > 1) {
                add("<span class='nested'>", make_html(ntoks[1..]), "</span>");
              }
            }

            break;

          default:
            ADD();
            break;
        }
      }
      else ADD();
    }
  }

  return buf->get();
}
