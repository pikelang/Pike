// Extraction of C files.
//
// NOTE that a C file always produces a full module tree with
// a root node. Use the functions in ProcessXML.pmod to merge
// the module trees extracted from the different C files.

#pragma strict_types

static inherit .PikeObjects;
static inherit .DocParser;

#define DEB werror("CExtractor.pmod: %d\n", __LINE__);

constant CdocMarker = "*!";
constant EOF = .PikeParser.EOF;

/*static private*/ array(string) getDocComments(string s) {
  array(string) a = filter(Parser.C.split(s), isDocComment);
  array(string) res = ({ });
  foreach(a, string doc) {
    string quux = doc[3 .. strlen(doc) - 3];
    //    werror("quux == %O\n", quux);

    array(string) lines = doc[1 .. strlen(doc) - 3]/"\n";
    //    werror("%O\n", lines);
    lines = map(lines, stripDocMarker);
    if (lines[-1] == "")
      lines = lines[0 .. sizeof(lines) - 2];
    res += ({ lines * "\n" });
  }
  return res;
}

static int isDocComment(string s) {
  return has_prefix(s, "/"+CdocMarker);
}

static constant scanString = "%*[ \t]" + CdocMarker + "%s";
static string stripDocMarker(string s) {
  string res;
  if (sscanf(s, scanString, res) == 2)
    return res;
  return "";
}

static void extractorErrorAt(SourcePosition sp, string message, mixed ... args)
{
  message = sprintf(message, @args);
  werror("CExtractor error! %O\n", message);
  message = "CExtractor error: " + message;
  throw ( ({ message, sp }) );
}

static private class Extractor {
  static constant EOF = .PikeParser.EOF;
  static string filename;
  static array(.DocParser.Parse) tokens = ({});

  static void create(string s, string filename) {
    local::filename = filename;

    array(string) ctokens = Parser.C.split(s);
    array(array(SourcePosition|string)) a = ({});
    int line = 1;
    foreach(ctokens, string ctoken) {
      if (isDocComment(ctoken)) {
        int firstLine = line;
        array(string) lines = ctoken[1 .. strlen(ctoken) - 3]/"\n";
        //    werror("%O\n", lines);
        int lastLine = firstLine + sizeof(lines) - 1;
        lines = map(lines, stripDocMarker);
        if (lines[-1] == "")
          lines = lines[0 .. sizeof(lines) - 2];
        a += ({
          ({ SourcePosition(filename, firstLine, lastLine), lines * "\n" })
        });
      }
      line += sizeof(ctoken/"\n") - 1;
    }

    foreach(a, [SourcePosition sp, string doc]) {
      array(array(string|array(string))) s = .DocParser.splitDocBlock(doc);
      foreach(s, array(string|array(string)) a)
        tokens += ({ .DocParser.Parse(a, sp) });
    }
    tokens += ({ 0 });
  }

  // returns ({"class", Class })
  // or ({"module", Module })
  // or ({"docgroup", DocGroup })
  // or 0 if no objects to parse.
  static array(string|Class|Module|DocGroup) parseObject() {
    Parse token = tokens[0];
    if (!token)
      return 0;
    MetaData meta = token->metadata();
    array(PikeObject) decls = meta->decls;
    switch (meta->type) {
      case "class":
      case "module":
        {
        object(Class)|object(Module) c;
        c = (meta->type == "module" ? Module() : Class());
        decls = ({ c });
        c->name = meta->name;
        tokens = tokens[1..];
        parseClassBody(c);
        .DocParser.Parse p = tokens[0];
        MetaData endmeta = p->metadata();
        if (endmeta->type != "end" + meta->type)
          extractorErrorAt(token->sourcePos,
                           "@%s without matching @end%s",
                           meta->type, meta->type);
        if (endmeta->name && endmeta->name != meta->name)
          extractorErrorAt(token->sourcePos,
                           "'@end%s %s' does not match '@%s %s'",
                           meta->type, endmeta->name, meta->type, meta->name);
        string endXML = p->doc("_general");
        if (endXML && endXML != "")
          extractorErrorAt(p->sourcePos,
                           "doc for @%s not allowed", endmeta->type);

        Documentation doc = Documentation();
        doc->xml = token->doc("_" + meta->type);
        c->documentation = doc;
        c->appears = meta->appears;
        c->belongs = meta->belongs;
        c->global = meta->global;
        tokens = tokens[1..];
        return ({ meta->type, c });
        }
        break;
      case "decl":
        {
        mapping(string:int) contexts = ([]);
        foreach (decls, PikeObject obj)
          contexts[obj->objtype] = 1;
        if (contexts["inherit"] && sizeof(decls) > 1)
          extractorErrorAt(token->sourcePos,
                           "inherit can not be grouped"
                           " with other declarations");
        string context;
        if (sizeof(indices(contexts)) == 1)
          context = "_" + indices(contexts)[0];
        else
          context = "_general";
        Documentation doc = Documentation();
        doc->xml = token->doc(context);
        DocGroup d = DocGroup(decls, doc);
        d->appears = meta->appears;
        d->belongs = meta->belongs;
        d->global = meta->global;
        d->documentation = doc;
        tokens = tokens[1..];
        return ({ "docgroup", d });
        }
        break;
      case 0:
        extractorErrorAt(token->sourcePos, "doc comment without destination");
      default:
        return 0;
    }

  }

  void parseClassBody(Class|Module c) {
    for(;;) {
      array(string|Class|Module|DocGroup) a = parseObject();
      if (!a)
        return;
      switch ([string]a[0]) {
        case "class":
        case "module":
          //werror("in parent %O: found child %O\n", c->name, a[1]->name);
          c->AddChild([object(Class)|object(Module)]a[1]);
          break;
        case "docgroup":
          array(PikeObject) objects = ([object(DocGroup)]a[1])->objects;
          foreach (objects, PikeObject obj)
            if (obj->objtype == "inherit")
              c->AddInherit(obj);
          c->AddGroup([object(DocGroup)]a[1]);
          break;
      }
    }
  }
}

Module extract(string s, string|void filename) {
  Extractor e = Extractor(s, filename);
  Module m = Module();
  m->name = "";  // the top-level module
  e->parseClassBody(m);
  return m;
}
