// Generates autodoc.c, with AutoDoc documentation of
// Pikes OpenGL support, from OpenGL docbook pages.

#include "constants.pike"
#include "features.pike"
// funcEV, funcV, func_misc, func_cat


constant Tree = Parser.XML.Tree;
constant Node = Parser.XML.Tree.SimpleNode;
constant RootNode = Parser.XML.Tree.SimpleRootNode;
constant HeaderNode = Parser.XML.Tree.SimpleHeaderNode;
constant DoctypeNode = Parser.XML.Tree.SimpleDoctypeNode;
constant CommentNode = Parser.XML.Tree.SimpleCommentNode;
constant TextNode = Parser.XML.Tree.SimpleTextNode;
constant ElementNode = Parser.XML.Tree.SimpleElementNode;
constant NodeType = Parser.XML.Tree.NodeType;

constant docbook_entities = ([
  "Hat" : "\u005e",               // cicumflex accent
  "nbsp" : "\u00a0",              // no-break space
  "CenterDot" : "\u00b7",         // middle dot
  "times" : "\u00d7",             // multiplication sign
  "Delta" : "\u0394",             // Greek capital letter Delta
  "Sigma" : "\u03a3",             // Greek capital letter Sigma
  "prime" : "\u2032",             // prime
  "Prime" : "\u2033",             // double prime
  "af" : "\u2061",                // function application
  "it" : "\u2062",                // invisible times
  "PartialD" : "\u2202",          // partial differential
  "VerticalBar" : "\u2223",       // divides
  "DoubleVerticalBar" : "\u2225", // parallel to
  "ne" : "\u2260",                // not equal to
  "le" : "\u2264",                // less-than or equal to
  "LeftCeiling" : "\u2308",       // left ceiling
  "RightCeiling" : "\u2309",      // right ceiling
  "LeftFloor" : "\u230a",         // left floor
  "RightFloor" : "\u230b"         // right floor
]);

constant subtree_elements =
  (<"refentry", "funcprototype", "variablelist", "varlistentry", "term",
    "math", "programlisting", "itemizedlist">);


array(string) not_documented = ({});
array(string) not_implemented = ({});
multiset(string) deferred_docs = (<>);
multiset(string) completed_docs = (<>);
mapping(string:RefEntrySegment) docs = ([]);
mapping(string:string) ref_alias = ([
  "glColor3": "glColor",
  "glColor4": "glColor",
  "glDrawElement": "glDrawElements",	// Typo in enableclientstate.3gl.
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

class Formatter
{
  constant limit = 70;
  constant first_prefix = "/*! ";
  constant mid_prefix = " *! ";
  constant last_prefix = " */";
  
  protected String.Buffer result = String.Buffer();
  protected string accumulator = "";
  protected int nb = 0;
  protected int nl_esc = 0;
  protected int breakpoint = -1; // Line can be broken after this position
  protected int blank_lines = 0;
  
  int `nobreak() { return nb; }
  void `nobreak=(int n) {
    if (!(nb = n) && sizeof(accumulator) > limit)
      break_line();
  }
  int `escape_newlines() { return nl_esc; }
  void `escape_newlines=(int n) {
    if (((n && !nl_esc) || (!n && nl_esc)) && sizeof(accumulator) > limit)
      break_line();
    nl_esc = n;
  }

  void newline(int(0..1)|void soft)
  {
    int n = sizeof(accumulator);
    while (n > 0 && accumulator[n-1] == ' ')
      --n;
    if (n > 0 || !soft) {
      result->add((sizeof(result)? mid_prefix : first_prefix),
                  (n? accumulator[..n-1] : ""),
                  (nl_esc? " @" : ""), "\n");
      if (n > 0)
        blank_lines = 0;
      else
        blank_lines ++;
    }
    accumulator = "";
    breakpoint = -1;
  }

  void paragraph()
  {
    if (sizeof(accumulator) > limit)
      break_line();
    newline(1);
    if (sizeof(result))
      while (blank_lines < 1)
        newline();
  }
  
  void break_line()
  {
    if (breakpoint < 0)
      return;
    string spill = accumulator[breakpoint+1..];
    accumulator = accumulator[..breakpoint];
    newline(1);
    accumulator = spill;
  }

  void outnb(string s)
  {
    int pos;
    while ((pos = search(s, "\n")) >= 0) {
      accumulator += s[..pos-1];
      if (sizeof(accumulator) > limit && breakpoint >= 0) {
        break_line();
        newline(1);
      } else
        newline();
      s = s[pos+1..];
    }
    accumulator += s;
    if (sizeof(accumulator) > limit && breakpoint >= 0)
      break_line();
  }

  void allow_break()
  {
    if (sizeof(accumulator) > limit)
      break_line();
    if(sizeof(accumulator))
      breakpoint = sizeof(accumulator)-1;
  }
  
  void out(string s)
  {
    if (nb) {
      outnb(s);
      return;
    }
    int pos;
    while ((pos = search(s, "\n")) >= 0) {
      out(s[..pos-1]);
      newline(1);
      s = s[pos+1..];
    }
    pos = 0;
    int offs = sizeof(accumulator);
    while (pos < sizeof(s))
      if (s[pos] == ' ') {
        if (breakpoint == offs+pos-1 || offs+pos <= limit )
          breakpoint = offs + pos++;
        else {
          accumulator += s[..pos-1];
          s = s[pos..];
          break_line();
          offs = sizeof(accumulator);
          pos = 1;
          breakpoint = offs;
        }
      } else
        pos++;
    accumulator += s;
    if (sizeof(accumulator) > limit)
      break_line();
  }

  string finalize()
  {
    if (sizeof(accumulator) > limit && breakpoint >= 0)
      break_line();
    newline(1);
    if (sizeof(result))
      result->add(last_prefix, "\n");
    return result->get();
  }
}

class ChildCollector(program target)
{
  protected array(Segment) segs = ({});
  
  void add(Segment s)
  {
    if (object_program(s) == target)
      segs += ({ s });
  }

  array(Segment) get() { return segs; }
}

class Segment
{
  void output_autodoc(Formatter f, mapping context);
  void output_xml(Formatter f, mapping context);
  Segment|void visit(string|function func, mixed ... args) {
    if (stringp(func) && this[func])
      return this[func](@args);
    else if (callablep(func))
      return func(this, @args);
  }
  void get_last(mapping(program:Segment) segs) {
    segs[this_program] = this;
  }
}

class NullSegment
{
  inherit Segment;
  void output_autodoc(Formatter f, mapping context) { }
  void output_xml(Formatter f, mapping context) { }
}

/* Continuation:
 * 0 = start of paragraph
 * 1 = preceding text ends in non-whitespace
 * 2 = preceding text ends in whitespace
 */

string trim_text(string|multiset(string) s, mapping context)
{
  string s2 = String.normalize_space(s);
  if (strlen(s2)) {
    if(s2[-1] != s[-1])
      s2 += " ";
    if(s2[0] != s[0] && context->continuation == 1)
      s2 = " " + s2;
  } else if (sizeof(s) && context->continuation == 1)
    return " ";
  return s2;
}

Charset.Encoder autodoc_to_ascii =
  Charset.encoder("us-ascii", 0, lambda(string s) {
    return "@xml{"+
      map(values(s), lambda(int n) { return sprintf("&#%d;", n); })*"" +
      "@}";
  } );

Charset.Encoder xml_to_ascii =
  Charset.encoder("us-ascii", 0, lambda(string s) {
    return map(values(s), lambda(int n) { return sprintf("&#%d;", n); })*"";
  } );

class TextSegment(string text)
{
  inherit Segment;

  protected string output(mapping context, int(0..1)|void escape_at)
  {
    string t = context->preformatted && !context->math ?
      replace(text, "\t", "    ") : trim_text(text, context);
    if (context->math)
      t = t - "\u2061" - "\u2062";
    if (sizeof(t))
      context->continuation = (t[-1] == ' '? 2 : 1);
    return escape_at||context->escape_at? replace(t, "@", "@@") : t;
  }

  void output_autodoc(Formatter f, mapping context)
  {
    f->out(replace(autodoc_to_ascii->feed(output(context, 1))->drain(),
                   "*/", "@xml{*@}/"));
  }

  void output_xml(Formatter f, mapping context)
  {
    f->out(replace(xml_to_ascii->feed(Parser.XML.text_quote(output(context)))
                   ->drain(), "*/", "&#42;/"));
  }

  void cleanup_doc()
  {
    text = replace(text, "a pointer to an array", "an array");
  }
}

class ContainerSegment(array contents)
{
  inherit Segment;

  void output_autodoc(Formatter f, mapping context)
  {
    foreach(contents;; Segment c)
      c->output_autodoc(f, context);
  }
  
  void output_xml(Formatter f, mapping context)
  {
    foreach(contents;; Segment c)
      c->output_xml(f, context);
  }

  Segment|void visit(string|function func, mixed ... args) {
    foreach(contents; int i; Segment c)
      if (c = c->visit(func, @args))
        contents[i] = c;
    return ::visit(func, @args);
  }

  protected array(int) index_of_keys(string ... keys)
  {
    int pos = 0;
    array(int) res = allocate(sizeof(keys));
    string s = "";
    foreach(contents;; Segment c)
      if (c->text)
        s += c->text;
    foreach(keys; int i; string key)
      if (pos < 0)
        res[i] = -1;
      else {
        int n = search(s, key, pos);
        res[i] = n;
        pos = (n<0? -1 : n+sizeof(key));
      }
    return res;
  }
  
  void replace_sequence(string prefix, string suffix, mixed ... replacement)
  {
    [int start, int end] = index_of_keys(prefix, suffix);
    if (end < 0)
      return;
    end += sizeof(suffix);
    int starti = 0, endi = sizeof(contents);
    foreach(contents; int i; Segment c)
      if (c->text) {
        int cut1 = -1, cut2 = -1;
        if (start < 0)
          ;
        else if (start == 0) {
          starti = i;
          --start;
        } else if (start < sizeof(c->text)) {
          cut1 = start-1;
          starti = i+1;
          start = -1;
        } else
          start -= sizeof(c->text);
        if (end < 0)
          ;
        else if (end == 0) {
          endi = i;
          --end;
        } else if (end < sizeof(c->text)) {
          cut2 = end;
          endi = i;
          end = -1;
        } else
          end -= sizeof(c->text);
        if (cut1 >= 0 || cut2 >= 0) {
          if (cut2 < 0)
            c->text = c->text[..cut1];
          else if (cut1 < 0)
            c->text = c->text[cut2..];
          else {
            /* Split current segment into two */
            contents = contents[..i]+({ mksegment(replacement, c->text[cut2..]) })+contents[i+1..];
            c->text = c->text[..cut1];
            return;
          }
        }
      } else {
        if (!start) { starti = i; --start; }
        if (!end) { endi = i; --end; }
      }
    contents = (starti > 0? contents[..starti-1] : ({})) +
      ({ mksegment(replacement) }) + contents[endi..];
  }
}

class MathSegment
{
  inherit ContainerSegment;
  protected int mrow;
  
  protected void create(array contents, int mr)
  {
    mrow = mr;
    ::create(contents);
  }

  void output_autodoc(Formatter f, mapping context)
  {
    if (mrow && !context->preformatted && context->continuation) {
      f->newline(1);
      context->continuation = 0;
    }
    context->math++;
    ::output_autodoc(f, context);
    --context->math;
    if (mrow && !context->preformatted && context->continuation) {
      f->newline(1);
      context->continuation = 0;
    }
  }

  void output_xml(Formatter f, mapping context)
  {
    if (mrow && !context->preformatted && context->continuation == 1) {
      f->out(" ");
      context->continuation = 2;
    }
    context->math++;
    ::output_xml(f, context);
    --context->math;
    if (mrow && !context->preformatted && context->contunation == 1) {
      f->out(" ");
      context->continuation = 2;
    }
  }
  
}

class ParagraphSegment
{
  inherit ContainerSegment;

  void output_autodoc(Formatter f, mapping context)
  {
    ::output_autodoc(f, context);
    f->paragraph();
    context->continuation = 0;
  }

  void output_xml(Formatter f, mapping context)
  {
    f->outnb("<p");
    f->allow_break();
    f->outnb(">");
    ::output_xml(f, context);
    f->outnb("</p");
    f->allow_break();
    f->outnb(">");
    context->continuation = 0;
  }

  Segment remove_paragraph(string key)
  {
    if (index_of_keys(key)[0] >= 0)
      return NullSegment();
    else
      return UNDEFINED;
  }
}

class RefSegment
{
  inherit TextSegment;

  void output_autodoc(Formatter f, mapping context)
  {
    f->nobreak++;
    f->outnb("@[");
    ::output_autodoc(f, context);
    f->outnb("]");
    --f->nobreak;
  }
  
  void output_xml(Formatter f, mapping context) 
  {
    f->outnb("<ref");
    f->allow_break();
    f->outnb(">");
    ::output_xml(f, context);
    f->outnb("</ref");
    f->allow_break();
    f->outnb(">");
  }

  void fix_ref(mapping(string:string) aliases, string func)
  {
    if(aliases[text]) {
      werror("Remapped %s (to %s)\n", text, func);
      text = aliases[text];
    }
  }

  Segment fixup_various_references()
  {
    switch (text) {
    case "glEvalMesh":
      return mksegment(RefSegment("glEvalMesh1"), " and ", RefSegment("glEvalMesh2"));
    case "GL_AUX":
      return mksegment(RefSegment("GL_AUX0"), " through ", RefSegment("GL_AUX3"));
    case "GL_CLIP_PLANE": return tag("GL_CLIP_PLANE", "tt");
    case "GL_LIGHT": return tag("GL_LIGHT", "tt");
    case "GL_POLYGON_OFFSET": return tag("GL_POLYGON_OFFSET", "tt");
    case "GL_TEXTURE_GEN_": return tag("GL_TEXTURE_GEN_", "tt");
    case "GL_PIXEL_MAP_c_TO_c":
      return tag(mksegment("GL_PIXEL_MAP_", tag("c", "i"),
                           "_TO_", tag("c", "i")), "tt");
    case "GL_c_BIAS":
      return tag(mksegment("GL_", tag("c", "i"), "_BIAS"), "tt");
    case "GL_c_SCALE":
      return tag(mksegment("GL_", tag("c", "i"), "_SCALE"), "tt");
    }
  }
}

class TagSegment(string tag, Segment child)
{
  inherit Segment;

  void output_autodoc(Formatter f, mapping context)
  {
    if (context->notags)
      child->output_autodoc(f, context);
    else {
      f->outnb("@"+tag+"{");
      child->output_autodoc(f, context);
      f->outnb("@}");
    }
  }
  
  void output_xml(Formatter f, mapping context)
  {
    f->outnb("<"+tag);
    f->allow_break();
    f->outnb(">");
    child->output_xml(f, context);
    f->outnb("</"+tag);
    f->allow_break();
    f->outnb(">");
  }

  Segment|void visit(string|function func, mixed ... args) {
    Segment c = child->visit(func, @args);
    if (c) child = c;
    return ::visit(func, @args);
  }
}

class ParamSegment
{
  inherit TagSegment; // Special case of an @i tag

  protected void create(array children)
  {
    ::create("i", mksegment(children));
  }
}

class CodeSegment
{
  inherit ContainerSegment;

  void output_autodoc(Formatter f, mapping context)
  {
    f->newline(1);
    f->outnb("@code\n");
    context->preformatted++;
    f->nobreak++;
    ::output_autodoc(f, context);
    --f->nobreak;
    --context->preformatted;
    f->newline(1);
    f->outnb("@endcode\n");
    context->continuation = 0;
  }

  void output_xml(Formatter f, mapping context)
  {
    f->outnb("\n<code>");
    context->preformatted++;
    f->nobreak++;
    ::output_xml(f, context);
    --f->nobreak;
    --context->preformatted;
    f->outnb("</code>\n");
    context->continuation = 0;
  }
}

class TableSegment
{
  inherit ContainerSegment;

  void output_autodoc(Formatter f, mapping context)
  {
    context->escape_at++;
    f->outnb("@xml{");
    output_xml(f, context);
    f->outnb("@}");
    --context->escape_at;
  }

  void output_xml(Formatter f, mapping context)
  {
    f->outnb("<matrix");
    f->allow_break();
    f->outnb(">");
    ::output_xml(f, context);
    f->outnb("</matrix");
    f->allow_break();
    f->outnb(">");
  }
}

class ListItemSegment
{
  inherit TagSegment; // XML output is just normal tag

  protected void create(array children)
  {
    ::create("item", mksegment(children));
  }

  void output_autodoc(Formatter f, mapping context)
  {
    f->newline(1);
    f->outnb("@item\n");
    child->output_autodoc(f, context);
    f->newline(1);
  }
}

class UnorderedListSegment
{
  inherit TagSegment; // XML output is just normal tag

  protected void create(array children)
  {
    ::create("ul", mksegment(children));
  }

  void output_autodoc(Formatter f, mapping context)
  {
    f->newline(1);
    f->outnb("@ul\n");
    child->output_autodoc(f, context);
    f->newline(1);
    f->outnb("@endul\n");
  }
}

class VariableListSegment
{
  inherit ContainerSegment;

  void output_autodoc(Formatter f, mapping context)
  {
    if (1 && !context->isparam) {
      context->escape_at++;
      f->outnb("@xml{");
      output_xml(f, context);
      f->outnb("@}");
      --context->escape_at;
      return;
    }

    if (context->isparam)
      f->paragraph();
    else {
      f->newline(1);
      f->outnb("@int\n");
    }
    ::output_autodoc(f, context);
    if (context->isparam)
      f->paragraph();
    else
      f->outnb("\n@endint\n");
    context->continuation = 0;
  }

  void output_xml(Formatter f, mapping context)
  {
    f->outnb("<matrix");
    f->allow_break();
    f->outnb(">");
    ::output_xml(f, context);
    f->outnb("</matrix");
    f->allow_break();
    f->outnb(">");
  }

  Segment remove_variablelist(string key)
  {
    int found = 0;
    visit(lambda(Segment seg) {
      if (seg->text && search(seg->text, key) >= 0)
        found = 1;
    });
    if (found)
      return NullSegment();
    else
      return UNDEFINED;
  }
}

class VarListEntrySegment
{
  inherit ContainerSegment;
  protected array terms;

  protected void create(array ts, array contents)
  {
    terms = ts;
    ::create(contents);
  }
  
  void output_autodoc(Formatter f, mapping context)
  {
    if (context->isparam) {
      foreach(terms;; Segment term) {
        if (object_program(term) != ParamSegment)
          error("%O: term = %O\n", context->entry, term);
      }
    }

    foreach(terms;; Segment term) {
      f->out(context->isparam? "@param " : "@value ");
      context->notags++;
      term->output_autodoc(f, context);
      --context->notags;
      f->newline();
      context->continuation = 0;
    }
    if (context->isparam)
      f->paragraph();
    ::output_autodoc(f, context);
    f->newline(1);
    context->continuation = 0;
  }

  void output_xml(Formatter f, mapping context)
  {
    if (context->isparam)
      error("FIXME\n");
    f->newline(1);
    f->out("<r><c>");
    context->continuation = 0;
    foreach(terms;; Segment term)
      term->output_xml(f, context);
    f->out("</c>\n<c>");
    context->continuation = 0;
    ::output_xml(f, context);
    f->out("</c></r>\n");
    context->continuation = 0;
  }

  Segment|void visit(string|function func, mixed ... args) {
    foreach(terms; int i; Segment term)
      if (term = term->visit(func, @args))
        terms[i] = term;
    return ::visit(func, @args);
  }

  void fixup_terms()
  {
    ChildCollector c = ChildCollector(ParamSegment);
    foreach(terms;; Segment term)
      term->visit(c->add);
    terms = c->get();
  }

  Segment remove_varlistentry(string key)
  {
    int found = 0;
    foreach(terms;; Segment term) {
      term->visit(lambda(Segment seg) {
        if (seg->text && search(seg->text, key) >= 0)
          found = 1;
      });
      if (found)
        return NullSegment();
    }
  }
}

class FuncPrototypeSegment
{
  inherit ContainerSegment;
  string name;
  string retval;
  int(0..1) array_version = 0;

  protected void create(string n, string rv,
                        array(ParamDefSegment) contents)
  {
    name = n;
    retval = rv;
    ::create(contents);
  }

  void output_autodoc(Formatter f, mapping context)
  {
    context->funcprototype++;
    f->escape_newlines++;
    f->out("@decl ");
    f->out(retval);
    f->out(name);
    f->out("(");
    foreach(contents; int i; Segment c) {
      if (i)
        f->out(", ");
      f->nobreak++;
      c->output_autodoc(f, context);
      --f->nobreak;
    }
    f->out(")");
    --f->escape_newlines;
    f->newline(1);
    --context->funcprototype;
  }

  void output_xml(Formatter f, mapping context)
  {
    error("FIXME\n");
  }
}

class ParamDefSegment(string ty, string var)
{
  inherit Segment;

  void output_autodoc(Formatter f, mapping context)
  {
    TextSegment(ty)->output_autodoc(f, context);
    TextSegment(var)->output_autodoc(f, context);
  }

  void output_xml(Formatter f, mapping context)
  {
    error("FIXME\n");
  }
}

class RefEntrySegment(string id, array(FuncPrototypeSegment) synopsis,
                      mapping sections)
{
  inherit Segment;

  void output_autodoc(Formatter f, mapping context)
  {
    if (synopsis)
      foreach(synopsis;; FuncPrototypeSegment proto)
        proto->output_autodoc(f, context|([]));
    foreach(({"description", "parameters", "parameters2", "parameters3",
              "example", "examples", "errors",
              "notes", /* "associatedgets", */ "seealso"});; string section)
      if (sections[section]) {
        string heading = ([
          "example": "@example",
          "examples": "@example",
          "errors": "@throws",
          "notes": "@note",
          "associatedgets": "@section Associated Gets",
          "seealso": "@seealso"
        ])[section];
        f->paragraph();
        if (heading)
          f->outnb(heading + "\n\n");
        sections[section]->output_autodoc(f, context|
                                          (["entry":id,
                                            "isparam":
                                            has_prefix(section, "parameters")
                                          ]));
        if (heading && has_prefix(heading, "@section"))
          f->outnb("\n@endsection");
      }
  }

  void output_xml(Formatter f, mapping context)
  {
    error("FIXME\n");
  }

  Segment|void visit(string|function func, mixed ... args) {
    if (synopsis)
      foreach(synopsis; int i; Segment s)
        if (s = s->visit(func, @args))
          synopsis[i] = s;
    foreach(sections; string i; Segment s)
      if (s = s->visit(func, @args))
        sections[i] = s;
    return ::visit(func, @args);
  }

  void cleanup_doc()
  {
    switch(id) {
    case "glColor":
      visit("replace_sequence", "glColor has two major variants:",
            "glColor4 variants specify all four color components explicitly.",
            "If no alpha value has been given, 1.0 (full intensity) is implied.");
      visit("remove_paragraph", "three or four signed byte, short, or long");
      visit("replace_sequence", "Included only in the four-argument glColor4 ",
            "commands.", "");
      break;
    case "glRasterPos":
      visit("replace_sequence", "glRasterPos4 specifies object coordinates",
            "to 0 and 1.", "The variable ", tag("z", "i"),
            " defaults to 0 and ", tag("w", "i"), " defaults to 1.");
      break;
    case "glCallLists":
      visit("replace_sequence", " more than one display list.",
            "follows:", " more than one display list.");
      visit("remove_variablelist", "unsigned bytes");
      visit("remove_paragraph", "null-terminated");
      visit("remove_varlistentry", "n");
      visit("remove_varlistentry", "type");
      visit("replace_sequence", "The pointer type is void",
            ".", "");
      m_delete(sections, "errors");
      break;
    case "glDeleteTextures":
      visit("remove_varlistentry", "n");
      visit("remove_paragraph", "is negative");
      visit("replace_sequence", "an array of textures to be deleted.", "",
            "the textures to be deleted.");
      break;
    case "glGenTextures":
      visit("replace_sequence", "texture names in ", ".",
            "texture names in an array.");
      visit("remove_varlistentry", "textures");
      break;
    case "glDrawElements":
      visit("replace_sequence", "it uses ", " sequential elements",
            "it uses sequential elements");
      visit("replace_sequence", "enabled array, starting at", "",
            "enabled array");
      visit("remove_varlistentry", "count");
      visit("remove_varlistentry", "type");
      visit("replace_sequence", "a pointer to the location where ",
            " are stored", "an array of indices");
      visit("remove_paragraph", "is negative");
      break;
    case "glMapGrid":
      visit("replace_sequence", "glMapGrid1 and glMapGrid2 specify the linear",
            "", RefSegment("glMapGrid"), " specifies the linear");
      visit("replace_sequence", "glMapGrid1 specifies ",
            "", RefSegment("glMapGrid"), " with three arguments specifies ");
      visit("replace_sequence", "glMapGrid2 specifies ",
            "", RefSegment("glMapGrid"), " with six arguments specifies ");
      visit("replace_sequence", "(glMapGrid2 only).", "",
            "(With six arguments only).");
      break;
    case "glGet":
      visit("replace_sequence", "These four commands return ", "",
            "This command returns ");
      visit("replace_sequence", "returned,", "returned data.",
            "returned.");
      break;
    }
  }
}

Segment get_last(Segment root, program ty)
{
  mapping(program:Segment) segs = ([]);
  root->visit("get_last", segs);
  return segs[ty];
}

Segment mksegment(Segment|string|array ... contents)
{
  contents = map(Array.flatten(contents), lambda(string|Segment c) {
    return stringp(c)? TextSegment(c) : c;
  });
  return sizeof(contents) == 1? contents[0] : ContainerSegment(contents);
}

TagSegment tag(Segment|string|array contents, string t)
{
  return TagSegment(t, mksegment(contents));
}

Segment math_identifier(array children, string|zero style)
{
  int singlechar =
    sizeof(children) == 1 && children[0]->text && strlen(children[0]->text)==1;
  if (/*!style*/ style != "normal") // styles other than normal deprecated
    style = (singlechar == 1? "italic" : "normal");
  switch (style) {
  case "normal": return mksegment(children);
  case "italic": return tag(children, "i");
  case "bold": return tag(children, "b");
  case "bold-italic": return tag(tag(children, "b"), "i");
  case "monospace": return tag(children, "tt");
  }
}

Segment math_fenced(array elts, string|zero open, string|zero close,
		   string|zero delim)
{
  array r = ({ TextSegment(open || "(") });
  if (!delim) delim = ",";
  foreach(elts; int idx; Segment elt) {
    if (idx) {
      if (idx > sizeof(delim))
	idx = sizeof(delim);
      string d = delim[idx-1..idx];
      if (search(",;", d[0]) >= 0)
	d += " ";
      r += ({ TextSegment(d) });
    }
    r += ({ elt });
  }
  return mksegment(r, TextSegment(close || ")"));
}

mixed visit_header(HeaderNode h, array(mixed) children, mapping context)
{
  mapping(string:string) attrs = h->get_attributes();
  if (attrs->version != "1.0")
    error("Unsupported XML version %s\n", attrs->version);
  return UNDEFINED;
}

mixed visit_doctype(DoctypeNode d, array(mixed) children, mapping context)
{
  mapping(string:string) attrs = d->get_attributes();
  string root_tag_name = d->get_tag_name();
  if (root_tag_name != "book" ||
      attrs->PUBLIC != "-//OASIS//DTD DocBook MathML Module V1.1b1//EN")
    error("Invalid DOCTYPE %s %s\n", root_tag_name, attrs->PUBLIC);
  return UNDEFINED;
}

mixed visit_comment(CommentNode c, array(mixed) children, mapping context)
{
  return UNDEFINED;
}

mixed visit_text(TextNode t, array(mixed) children, mapping context)
{
  string s = t->get_text();
  return String.trim(s) == "" ? UNDEFINED : TextSegment(s);
}

void visit_refentry(string id, mapping context)
{
  if (!context->funcsynopsis) {
    werror("Warning: %s has no synopsis, skipped\n", id);
    return;
  }

  array(FuncPrototypeSegment) prots = context->funcsynopsis;
  mapping names = mkmapping(prots->name, prots->name);
  string name, new_name;
  if (id == "glGet") {
    new_name = id;
    name = prots[0]->name;
  } else if ((< "glEvalMesh" >)[id]) {
    // Keep as glEvalMesh1() and glEvalMesh2().
    werror("Skip canonicalization for %O.\n", id);
  } else if(name = names[id+"4f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[id+"3f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[id+"2f"]) new_name = name[..sizeof(name)-3];
  else if(name = names[id+"f"])  new_name = name[..sizeof(name)-2];
  else if(name = names[id+"2"])  new_name = name[..sizeof(name)-2];
  else if(name = names[id+"fv"]) new_name = name[..sizeof(name)-3];
  else if(name = names[id+"uiv"]) new_name = name[..sizeof(name)-4];
  else if(name = names[id+"v"]) new_name = name[..sizeof(name)-2];

  if(new_name) {
    FuncPrototypeSegment delegate, delegate2;
    if (new_name != id)
      error("Unexpected rename %O -> %O\n", id, new_name);
    foreach(prots;; FuncPrototypeSegment prot) {
      if (prot->name != new_name) {
        ref_alias[prot->name] = new_name;
      }
      if (prot->name == name)
        delegate = prot;
      else if (prot->name == name+"v") {
        prot->array_version = 1;
        delegate2 = prot;
      } else if (id == "glMapGrid" && prot->name == id+"1f")
        delegate2 = prot;
    }
    delegate->name = new_name;
    prots = ({ delegate });
    if (delegate2) {
      delegate2->name = new_name;
      if (delegate2->array_version)
        prots += ({ delegate2 });
      else
        prots = ({ delegate2 }) + prots;
    }
  } else {
    /* Use first declared name when mapping to pike prototypes, except
       in the case of glEvalMesh where the second declared name is used
       in order to get all the parameter types */
    if (id == "glEvalMesh")
      new_name = "glEvalMesh2";
    else
      new_name = prots[0]->name;
  }
  
  (context->parent[new_name] =
   RefEntrySegment(id, prots, context->refsect1))->visit("cleanup_doc");
}

mixed visit_element(ElementNode e, array(mixed) children, mapping context)
{
  mapping(string:string) attrs = e->get_attributes();
  string name = e->get_tag_name();

  switch(name) {
  case "refentry":
    visit_refentry(attrs->id, context);
    break;
  case "mfenced":
    return math_fenced(children, attrs->open, attrs->close, attrs->separators);
  case "mfrac":
    return mksegment(children[0], TextSegment("/"), children[1]);
  case "mi":
    return math_identifier(children, attrs->mathvariant);
  case "math":
    return MathSegment(children, context->mrow);
  case "mrow":
    context->mrow++;
  case "mn":
  case "mo":
    return mksegment(children);
  case "msub": return mksegment(children[0], tag(children[1], "sub"));
  case "msup": return mksegment(children[0], tag(children[1], "sup"));
  case "mover": {
    Segment a = get_last(children[0], TextSegment);
    string b = object_program(children[1]) == TextSegment && children[1]->text;
    if (a && b == "^") {
      a->text = Unicode.normalize(a->text+"\u0302", "NFC");
      return children[0];
    }
    error("Unsupported mover %O\n", children[1]);
  }
  case "munderover": return mksegment(children[0], tag(children[1], "sub"), tag(children[2], "sup")); // FIXME!?
  case "infinity": return mksegment("\u221e");
  case "msqrt": return mksegment("\u221a", children);
  case "mspace":
    if (attrs->width == "1ex")
      return mksegment("\u2002"); // one ex is approximately 1/2 em = 1 en
    else
      error("Unhandled mspace width %O\n", attrs->width);

  case "term":
    if (!context->parent->term)
      context->parent->term = ({});
    context->parent->term += ({ mksegment(children) });
    break;
  case "funcdef":
    if (sizeof(children) == 1 && object_program(children[0]) == TextSegment &&
        context->funcname)
      context->funcret = children[0]->text;
    else
      error("Invalid funcdef\n");
    break;

  case "refsect1":
    if (!context->refsect1)
      context->refsect1 = ([]);
    context->refsect1[attrs->id] = mksegment(children);
    if (attrs->id == "parameters")
      context->refsect1[attrs->id]->visit("fixup_terms");
    break;
  case "funcsynopsis":
    if (Array.any(children, lambda(Segment s) {
      return object_program(s) != FuncPrototypeSegment;
    }))
      error("Invalid synopsis %O\n", children);
    if (!context->funcsynopsis)
      context->funcsynopsis = ({});
    context->funcsynopsis += children;
  case "paramdef":
    if (context->subtree != "funcprototype")
      return mksegment(children);
    if (sizeof(children) == 1 &&
        object_program(children[0]) == ParamSegment &&
        object_program(children[0]->child) == TextSegment &&
        children[0]->child->text == "void")
      // Discard typeless parameter named "void"
      return UNDEFINED;
    if (sizeof(children) != 2 ||
        object_program(children[0]) != TextSegment ||
        object_program(children[1]) != ParamSegment ||
        object_program(children[1]->child) != TextSegment)
      error("Invalid paramdef children %O\n", children);
    return ParamDefSegment(children[0]->text,
                           children[1]->child->text);

  case "variablelist":
    return VariableListSegment(children);
  case "varlistentry":
    return VarListEntrySegment(context->term, children);
  case "funcprototype":
    if (!context->funcname)
      error("Missing function name\n");
    if (!context->funcret)
      error("Missing function return type\n");
    if (Array.any(children, lambda(Segment s) {
      return object_program(s) != ParamDefSegment;
    }))
      error("Invalid prototype contents %O\n", children);
    return FuncPrototypeSegment(context->funcname, context->funcret, children);

  case "para":
    return ParagraphSegment(children);

  case "inlineequation":
  case "informalequation":
  case "refentrytitle":
  case "thead":
  case "tbody":
  case "tgroup":
    return mksegment(children);

  case "trademark":
    switch(attrs->class) {
    case 0:
      return mksegment("\u2122");
    case "registered":
      return mksegment("\u00ae");
    case "copyright":
      return mksegment("\u00a9");
    default:
      error("Unknown trademark class %s\n", attrs->class);
    }
  case "function":
    if (context->subtree == "funcprototype") {
      if (sizeof(children)==1 && object_program(children[0]) == TextSegment) {
        context->funcname = children[0]->text;
        return UNDEFINED;
      } else
        error("Unexpected function contents %O\n", children);
      return mksegment(children);
    }
  case "constant":
  case "citerefentry":
    if (sizeof(children) != 1 || object_program(children[0]) != TextSegment)
      error("Unexpected reference %O\n", children);
    return RefSegment(String.normalize_space(children[0]->text));
  case "parameter":
    return ParamSegment(children);
  case "ulink":
    if (attrs->url != children[0]->text)
      error("ulink mismatch %O != %O\n", attrs->url, children[0]->text);
    return tag(attrs->url, "url");
  case "emphasis":
    return tag(children, attrs->role=="bold"? "b":"i");
  case "programlisting":
    return CodeSegment(children);
  case "informaltable":
  case "mtable":
    return TableSegment(children);
  case "row":
  case "mtr":
    return tag(children, "r");
  case "entry":
  case "mtd":
    return tag(children, "c"); 
  case "code":
    return tag(children, "tt");
  case "itemizedlist":
    return UnorderedListSegment(children);
  case "listitem":
    if (context->subtree == "itemizedlist")
      return ListItemSegment(children);
    else
      return mksegment(children);

  case "year":
  case "holder":
  case "copyright":
  case "refmeta":
  case "refmetainfo":
  case "manvolnum":
  case "refdescriptor":
  case "refname":
  case "refnamediv":
  case "refpurpose":
  case "title":
  case "refsynopsisdiv":
  case "colspec":
    break;
  default:
    error("Unknown element %O\n", name);
  }
  return UNDEFINED;
}

mixed visit_root(RootNode r, array(mixed) children, mapping context)
{
  return UNDEFINED;
}

mixed visit(Node n, array(mixed) children, mapping context)
{
  NodeType nt = n->get_node_type();
  switch(nt) {
  case Tree.XML_ELEMENT: return visit_element(n, children, context);
  case Tree.XML_TEXT: return visit_text(n, children, context);
  case Tree.XML_HEADER: return visit_header(n, children, context);
  case Tree.XML_DOCTYPE: return visit_doctype(n, children, context);
  case Tree.XML_COMMENT: return visit_comment(n, children, context);
  case Tree.XML_ROOT: return visit_root(n, children, context);
  default:
    error("Unhandled node type %O\n", nt);
  }
}

mixed walk_postorder(Node n, mapping|void context)
{
  if (!context)
    context = ([]);
  if (n->get_node_type() == Tree.XML_ELEMENT &&
      subtree_elements[n->get_tag_name()])
    context = ([ "subtree": n->get_tag_name(), "parent": context ]);
  array(mixed) c = map(n->get_children(), walk_postorder, context);
  return visit(n, c - ({ UNDEFINED }), context);
}

void preprocess_xml(string(8bit) s)
{
  Node root;
  mixed err = catch {
      root = Parser.XML.Tree.simple_parse_input(s, docbook_entities);
    };
  if (err) {
    int n;
    if (1 == sscanf(err[0], "Parser.XML.Tree: No such entity. [Offset: %d", n)){
      /* Improve error message... */
      int n2 = n-1;
      while (n2 > 0 && n2 > n-64 && s[n2] != '&') --n2;
      while (n < sizeof(s) && n < n2+64 && s[n-1] != ';') n++;
      string ent = s[n2..n-1];
      err[0] = sprintf("Parser.XML.Tree: No such entity %O.\n", ent);
    }
    throw(err);
  }
  walk_postorder(root, docs);
}

void visit_docs(string func, mixed ... args)
{
  foreach(docs; string ident; Segment entry)
    entry->visit(func, @args);
}

string process_docbook(string name, string prot_ret, array(string) prot_types) {
  array(string) prot_types2;

  if(!docs[name]) {
    deferred_docs[name] = 1;
    return "";
  }

  RefEntrySegment doc = m_delete(docs, name);

  if (doc->synopsis[-1]->array_version) {
    int n = search(prot_types[0], "|array(");
    if (n > 0) {
      prot_types2 = ({}) + prot_types;
      prot_types[0] = prot_types[0][..n-1];
      prot_types2[0] = prot_types2[0][n+1..];
      prot_types = map(prot_types, replace, "|VOID", "");
    } else
      doc->synopsis = doc->synopsis[..<1];
  }
  prot_types = map(prot_types, replace, "VOID", "void");

  foreach(doc->synopsis;; FuncPrototypeSegment proto) {
    completed_docs[proto->name] = 1;
    if (proto->array_version && prot_types2)
      prot_types = prot_types2;
    if (proto->name == "glClipPlane") {
      proto->contents[-1]->var += "_0";
      proto->contents += ({ ParamDefSegment("", "equation_1"),
                            ParamDefSegment("", "equation_2"),
                            ParamDefSegment("", "equation_3") });
    }
    proto->retval = prot_ret+" ";
    foreach(proto->contents; int i; ParamDefSegment param)
      if (i < sizeof(prot_types)) {
        param->ty = prot_types[i]+" ";
        // Zap arguments not used by Pike.
        if (prot_types[i] == "void")
          proto->contents[i] = UNDEFINED;
      }
    proto->contents -= ({ UNDEFINED });
  }

  Formatter f = Formatter();
  doc->visit("fixup_various_references");
  doc->output_autodoc(f, ([]));
  return f->finalize();
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
    if(i>=mi)
      t+="|void";
    else if(i>0)
      t+="|VOID";
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

  string x = process_docbook(name, ret, args);
  //  write(x+"\n");
  return x;
}

void prefetch()
{
  if( !file_stat( "OpenGL-Refpages-main/gl2.1/" ) ) {
    werror( "Need OpenGL docbook pages unpacked in present working directory.\n"
            "Download\n"
            "https://github.com/KhronosGroup/OpenGL-Refpages/archive/refs/heads/main.zip\n"
            "first.\n"
            );
    exit( 1 );
  }
  foreach(glob("*.xml", get_dir("OpenGL-Refpages-main/gl2.1/")), string fn) {
    preprocess_xml(Stdio.read_file("OpenGL-Refpages-main/gl2.1/"+fn));
  }
}

string first_page()
{
  Formatter f = Formatter();
  f->out("@module GL");
  f->paragraph();
  f->out(#"
OpenGL glue. All method and constant names have been kept close to their low \
level counterparts for easy adoption of OpenGL code from other languages and \
examples off the web. Superfluous suffixes specifying the number and types of \
arguments have been dropped, though.");
  if (sizeof(not_documented)) {
    f->paragraph();
    f->out(#"@fixme
Methods available, but lacking documentation:

@xml{<matrix>
");
    foreach( sort( not_documented ), string name ) {
      f->out("<r><c>" + name + "</c></r>");
      f->newline(1);
    }
    f->out("</matrix>@}");
  }

  if (sizeof(not_implemented)) {
    f->paragraph();
    f->out(#"@note
All OpenGL methods have not been implemented. For a list of unimplemented \
methods, see @[glUnimplemented].");
  }

  string ret = f->finalize();

  if (sizeof(not_implemented)) {
    Formatter trailer = Formatter();
    trailer->out("@decl private void glUnimplemented()");
    foreach( sort( (array)not_implemented ), string name ) {
      trailer->newline(1);
      trailer->out("@decl private void " + name + "()");
    }
    trailer->paragraph();
    trailer->out("These OpenGL methods are still missing in the Pike @[GL] API.");
    trailer->paragraph();
    trailer->out("@seealso\n");
    trailer->out("@[GL]\n");
    ret += trailer->finalize();
  }

  return ret;
}

mapping(string:array(string)) refs = ([]);

void fix_refs() {
  werror("Resolving references to renamed methods.\n");
  foreach(docs; string func; Segment entry)
    entry->visit("fix_ref", ref_alias, func);

  werror("Finding constant references.\n");
  mapping(string:multiset(string)) allrefs = ([]);
  foreach(docs; string func; Segment entry) {
    allrefs[func] = (<>);
    entry->visit(lambda(Segment seg) {
      if (object_program(seg) == RefSegment)
        allrefs[func][seg->text] = 1;
    });
  }
  foreach(indices(constants), string name) {
    array r = ({});
    string name2;
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
            }), [string prefix, string suffix, string alt_name]) {
      if (has_prefix(name, prefix) && has_suffix(name, suffix))
        name2 = alt_name;
    }
    foreach(docs; string func; Segment entry) {
      if (allrefs[func][name] || (name2 && allrefs[func][name2]))
        r += ({ func });
    }
    if(sizeof(r))
      refs[name] = r;
  }
}

void main()
{
  werror("Reading docbook pages.\n");
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
    doc += document(func[0], func[1]) + "\n";
    if(catch { doc += document(func[0], func[1]) + "\n"; })
    {
      error("Failed with %s [%s]\n", @func);
      not_documented += ({ func[0] });
    }
  }

  foreach( funcV, string func)
    doc += document(func, "V") + "\n";

  foreach( funcEV, string func)
    doc += document(func, "VE") + "\n";

  foreach(deferred_docs; string name; )
    if(!completed_docs[name])
      werror("%O is not documented\n", name);

  not_implemented = indices(docs) - not_documented;
  foreach( sort(indices(constants)), string name )
  {
    array relevant = refs[name] || ({});
    relevant -= not_implemented;
    // if( sizeof(relevant) )
    {
      Formatter f = Formatter();
      array r = map(sort(relevant), RefSegment);
      f->outnb(sprintf("@decl constant %s = %d", name, constants[name]));
      if (sizeof(r)) {
        foreach(r; int i; Segment s) {
          s = s->visit("fixup_various_references");
          if (s)
            r[i] = s;
        }
        r = r/1*({", "});
        if (sizeof(r) > 1)
          r[-2] = " and ";
        f->newline();
        f->out("Used in ");
        mksegment(r)->output_autodoc(f, ([]));
      }
      doc += f->finalize();
    }
  }

  doc = #"/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/\n\n/* AutoDoc generated from OpenGL docbook pages */"
    "\n\n" + first_page() + "\n\n" + doc +
    "\n/*! @endmodule\n */\n\n";

  werror("Writing result file.\n");
  Stdio.write_file("autodoc.c", doc);
  werror("Done.\n");
}
