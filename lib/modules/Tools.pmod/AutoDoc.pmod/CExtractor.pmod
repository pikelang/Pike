#pike __REAL_VERSION__

// Extraction of C files.
//
// NOTE that a C file always produces a full module tree with
// a root node. Use the functions in ProcessXML.pmod to merge
// the module trees extracted from the different C files.

// #pragma strict_types

static inherit .PikeObjects;
static inherit .DocParser;

#include "./debug.h"

constant EOF = .PikeParser.EOF;

static string stripDocMarker(string s) {
  string res;
  if (sscanf(s, "%*[ \t]*!%s", res) == 2)
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
    this_program::filename = filename;

    array(string) ctokens = Parser.C.split(s);
    array(array(SourcePosition|string)) a = ({});
    int line = 1;
    foreach(ctokens, string ctoken) {
      if (has_prefix(ctoken, "/*!")) {
        int firstLine = line;
        array(string) lines = ctoken[1 .. sizeof(ctoken) - 3]/"\n";
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
  // or ({"namespace", NameSpace })
  // or 0 if no objects to parse.
  static array(string|Class|Module|NameSpace|DocGroup)
    parseObject(Class|Module|NameSpace|AutoDoc parent, AutoDoc root)
  {
    Parse token = tokens[0];
    if (!token)
      return 0;
    MetaData meta = token->metadata();
    array(PikeObject) decls = meta->decls;
    switch (meta->type) {
    case "namespace":
      // Namespaces are always located in the root.
      parent = root;
      // FALL_THROUGH
    case "class":
    case "module":
      {
        object(Class)|object(Module) alreadyChild =
          parent->findChild(meta->name);
        object(Class)|object(Module) c;

        // see if we are re-entering a previously created class/module...
        if (alreadyChild) {
          c = alreadyChild;
          if (meta->type != c->objtype)
            extractorErrorAt(token->currentPosition,
                             "'%s %s' doesnt match '%s %s'"
                             " in previous declaration",
                             meta->type, meta->name, c->objtype, c->name);
          if (meta->appears || meta->belongs)
            extractorErrorAt(token->currentPosition,
                             "cannot specify @appears or @belongs on"
                             " reentrance into '%s %s'", c->objtype, c->name);
        }
        else {
          c = (["module":Module,
		"class":Class,
		"namespace":NameSpace])[meta->type]();
          c->name = meta->name;
	  foreach(meta->inherits, PikeObject p)
	    c->AddInherit(p);
        }
        decls = ({ c });
        tokens = tokens[1..];
        parseClassBody(c, root);
        .DocParser.Parse p = tokens[0];
        MetaData endmeta = p ? p->metadata() : 0;
        if (!endmeta || endmeta->type != "end" + meta->type)
          extractorErrorAt(token->currentPosition,
                           "'@%s %s' without matching '@end%s'",
                           meta->type, meta->name, meta->type);
        if (endmeta->name && endmeta->name != meta->name)
          extractorErrorAt(token->currentPosition,
                           "'@end%s %s' does not match '@%s %s'",
                           meta->type, endmeta->name, meta->type, meta->name);
        string endXML = p->doc("_general");
        if (endXML && endXML != "")
          extractorErrorAt(p->currentPosition,
                           "doc for @%s not allowed", endmeta->type);

        Documentation doc = Documentation();
        doc->xml = token->doc("_" + meta->type);
        if (alreadyChild && doc->xml && doc->xml != "")
          extractorErrorAt(p->currentPosition,
                           "doc not allowed on reentrance into '%s %s'",
                           c->objtype, c->name);
	if (!alreadyChild)
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

	if (sizeof(decls) > 1) {
	  if (contexts["enum"])
	    extractorErrorAt(token->currentPosition,
			     "enum can not be grouped"
			     " with other declarations");
	
	  if (contexts["inherit"])
	    extractorErrorAt(token->currentPosition,
			     "inherit can not be grouped"
			     " with other declarations");
	}
        string context;
        if (sizeof(indices(contexts)) == 1)
          context = "_" + indices(contexts)[0];
        else
          context = "_general";
        Documentation doc = Documentation();
        doc->xml = token->doc(context);
	object(DocGroup)|Enum d;
	if (contexts["enum"]) {
	  // Enums are their own docgroup...
	  d = decls[0];
	} else {
	  d = DocGroup(decls, doc);
	}
	d->appears = meta->appears;
	d->belongs = meta->belongs;
        d->documentation = doc;
        tokens = tokens[1..];
        return ({ d->objtype || "docgroup", d });
      }
      break;
    case 0:
      extractorErrorAt(token->currentPosition,
		       "doc comment without destination");
    default:
      return 0;
    }

  }

  void parseClassBody(Class|Module|NameSpace c, AutoDoc root) {
    for(;;) {
      array(string|Class|Module|DocGroup) a = parseObject(c, root);
      if (!a)
        return;
      switch ([string]a[0]) {
        case "namespace":
          //werror("in parent %O: found child %O\n", c->name, a[1]->name);
          // Check if it was a namespace we already know of:
          if (search(root->children, a[1]) < 0)
            root->AddChild([object(NameSpace)]a[1]);
          break;
	  
        case "class":
        case "module":
        case "enum":
          //werror("in parent %O: found child %O\n", c->name, a[1]->name);
          // Check if it was a @class or @module that was reentered:
	  if (search(c->children, a[1]) < 0)
	    c->AddChild([object(Class)|object(Module)]a[1]);
          break;
        case "docgroup":
          c->AddGroup([object(DocGroup)]a[1]);
          break;
        default:
	  werror("parseClassBody(): Unhandled object: %O\n", a);
	  break;
      }
    }
  }
}

AutoDoc extract(string s, string|void filename, string|void namespace)
{
  Extractor e = Extractor(s, filename);

  // Create the top-level module.
  AutoDoc m = AutoDoc();

  // Create the default namespace.
  NameSpace ns = NameSpace();
  ns->name = namespace || "predef";
  ns->documentation = Documentation();
  ns->documentation->xml = "";
  m->AddChild(ns);

  // Perform the actual parsing.
  e->parseClassBody(ns, m);
  return m;
}
