#pike __REAL_VERSION__

// Extraction of C files.
//
// NOTE that a C file always produces a full module tree with
// a root node. Use the functions in ProcessXML.pmod to merge
// the module trees extracted from the different C files.

#pragma strict_types

static inherit .PikeObjects;
static inherit .DocParser;

#include "./debug.h"

constant CdocMarker = "*!";
constant EOF = .PikeParser.EOF;

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
  werror("CExtractor error! %O  %O\n", message, sp);
  throw (AutoDocError(sp, "CExtractor", message));
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
      array(array) s = .DocParser.splitDocBlock(doc, sp);
      foreach(s, array a)
        tokens += ({ .DocParser.Parse(a, sp) });
    }
    tokens += ({ 0 });
  }

  // returns ({"class", Class })
  // or ({"module", Module })
  // or ({"docgroup", DocGroup })
  // or 0 if no objects to parse.
  static array(string|Class|Module|DocGroup) parseObject(Class|Module|void parent)
  {
    Parse token = tokens[0];
    if (!token)
      return 0;
    MetaData meta = token->metadata();
    array(PikeObject) decls = meta->decls;
    switch (meta->type) {
      case "class":
      case "module":
        {
        object(Class)|object(Module) alreadyChild =
          parent->findChild(meta->name);
        object(Class)|object(Module) c;

        // see if we are re-enteri ng a previously created class/module...
        if (alreadyChild) {
          c = alreadyChild;
          if (meta->type != c->objtype)
            extractorErrorAt(token->sourcePos,
                             "'%s %s' doesnt match '%s %s'"
                             " in previous declaration",
                             meta->type, meta->name, c->objtype, c->name);
          if (meta->appears || meta->belongs)
            extractorErrorAt(token->sourcePos,
                             "cannot specify @appears or @belongs on"
                             " reentrance into '%s %s'", c->objtype, c->name);
        }
        else {
          c = (meta->type == "module" ? Module() : Class());
          c->name = meta->name;
        }
        decls = ({ c });
        tokens = tokens[1..];
        parseClassBody(c);
        .DocParser.Parse p = tokens[0];
        MetaData endmeta = p ? p->metadata() : 0;
        if (!endmeta || endmeta->type != "end" + meta->type)
          extractorErrorAt(token->sourcePos,
                           "'@%s %s' without matching '@end%s'",
                           meta->type, meta->name, meta->type);
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
        if (alreadyChild && doc->xml && doc->xml != "")
          extractorErrorAt(p->sourcePos,
                           "doc not allowed on reentrance into '%s %s'",
                           c->objtype, c->name);
        c->documentation = doc;
        c->appears = meta->appears;
        c->belongs = meta->belongs;
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
      array(string|Class|Module|DocGroup) a = parseObject(c);
      if (!a)
        return;
      switch ([string]a[0]) {
        case "class":
        case "module":
          //werror("in parent %O: found child %O\n", c->name, a[1]->name);
          // Check if it was a @class or @module that was reentered:
          if (search(c->children, a[1]) < 0)
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
