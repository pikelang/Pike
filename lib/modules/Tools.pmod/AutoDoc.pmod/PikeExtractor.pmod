#pike __REAL_VERSION__

//========================================================================
// EXTRACTION OF DOCUMENTATION FROM PIKE SOURCE FILES
//========================================================================

// #pragma strict_types

protected inherit .DocParser;

#include "./debug.h"

//========================================================================
// DOC EXTRACTION
//========================================================================

private class Extractor {
  protected constant WITH_NL = .PikeParser.WITH_NL;

  // private string filename;
  private .PikeParser parser;

  protected .Flags flags;
  protected int verbosity;

  protected string report(SourcePosition pos, int level,
			  sprintf_format fmt, sprintf_args ... args)
  {
    string msg = fmt;
    if (sizeof(args)) {
      msg = sprintf("PikeExtractor: " + fmt, @args);
    }

    if (level <= verbosity) {
      if (pos->lastline) {
	werror("%s:%d..%d: %s",
	       pos->filename||"-", pos->firstline, pos->lastline, msg);
      } else {
	werror("%s:%d: %s",
	       pos->filename||"-", pos->firstline, msg);
      }
    }
    msg = (pos->filename||"-")+":"+(pos->firstline)+": "+msg;
    return msg;
  }

  protected void report_error(SourcePosition pos, int level,
			      sprintf_format fmt, sprintf_args ... args)
  {
    string msg = report(pos, level, fmt, @args);
    if (!(flags & .FLAG_KEEP_GOING)) {
      error("%s", msg);
    }
  }

  protected void create(string s, string filename, .Flags flags) {
    this::flags = flags;
    verbosity = flags & .FLAG_VERB_MASK;

    parser = .PikeParser(0, filename, flags);

    array(string) tokens;
    array(int) positions;
    [tokens, positions] = parser->tokenize(s, 1);

    array(string) new_tokens = ({});
    array(int) new_positions = ({});

    ADT.Stack ignores = ADT.Stack();
    ignores->push(0);	// End sentinel.
    for(int i; i<sizeof(tokens); i++) {
      string s = tokens[i];
      int pos = positions[i];
      if (has_prefix(s, DOC_COMMENT)) {
        s = String.trim(s[sizeof(DOC_COMMENT) .. ]);
        if (s == "@ignore") {
	  ignores->push(pos);
	  continue;
        } else if (s == "@endignore") {
          if (ignores->top())
            ignores->pop();
          else
            report_error(SourcePosition(filename, pos), .FLAG_NORMAL,
			 "@endignore without matching @ignore.\n");
	  continue;
	}
      }
      if (!ignores->top()) {
        new_tokens += ({ tokens[i] });
	new_positions += ({ pos });
      }
    }

    while (ignores->top())
      report_error(SourcePosition(filename, ignores->pop()), .FLAG_NORMAL,
		   "@ignore without matching @endignore.\n");

    parser->setTokens(new_tokens, new_positions);
  }

  protected void extractorError(sprintf_format message, sprintf_args ... args)
  {
    report_error(parser->currentPosition, .FLAG_NORMAL, message + "\n", @args);
  }

  protected void extractorWarning(sprintf_format message, sprintf_args ... args)
  {
    report(parser->currentPosition, .FLAG_VERBOSE,
	   "Warning: " + message + "\n", @args);
  }

  protected int isDocComment(string s) {
    return (sizeof(s) >= 3 && s[0..2] == DOC_COMMENT);
  }

  protected string stripDocMarker(string s) {
    if (isDocComment(s))
      return s[3..];
    extractorError("Not a doc comment!");
    return s;
  }

  // readAdjacentDocLines consumes the doc lines AND the "\n" after the last one
  protected Documentation readAdjacentDocLines() {
    Documentation res = Documentation();
    string s = parser->peekToken();
    string text = "";
    SourcePosition pos = parser->currentPosition->copy();
    while (isDocComment(s)) {
      text += stripDocMarker(s) + "\n";
      parser->readToken();              // Grok the doc comment string
      s = parser->peekToken(WITH_NL);
      if (s == "\n")
        parser->readToken(WITH_NL);     // Skip the corresp. NL
      else
        break;             // Ought to be EOF
      s = parser->peekToken(WITH_NL);   // NL will break the loop
    }
    res->text = text;
    res->position = pos;
    return res;
  }

  // check if the token is one that can not start a declaration
  // adjacent to the last one
  protected int isDelimiter(string token) {
    return (< "\n", "}", "EOF" >) [token];
  }

  // consumes the "\n" after the last constant too...
  protected array(array(EnumConstant)|int(0..1)) parseAdjacentEnumConstants() {
    array(EnumConstant) result = ({ });
    parser->skipNewlines();
    int terminating_nl;
    while (isIdent(parser->peekToken(WITH_NL))) {
      EnumConstant c = EnumConstant();
      c->position = parser->currentPosition->copy();
      c->name = parser->readToken(); // read the identifier
      result += ({ c });
      parser->skipUntil((<",", "}">)); // don't care about any init expr
      if (parser->peekToken() == ",")
        parser->eat(",");
      else
        break;
      // allow at most ONE intervening newline character
      if (terminating_nl = (parser->peekToken(WITH_NL) == "\n"))
        parser->readToken(WITH_NL);
    }
    return ({ result, terminating_nl });
  }

  // Returns nothing, instead adds the enumconstants as children to
  // the Enum parent
  protected void parseEnumBody(Enum parent) {
    for (;;) {
      parser->skipNewlines();
      object(Documentation)|zero doc = 0;
      array(EnumConstant) consts = ({});
      int got_nl;
      if (isIdent(parser->peekToken())) {
        // The case with one or more constants, optionally followed by
        // a doc comment.
        [consts, got_nl] = parseAdjacentEnumConstants();
        if (!sizeof(consts)) // well, could never happen, but ...
          extractorError("expected enum constant");

        // read the optional doc comment
        if (isDocComment(parser->peekToken(WITH_NL))) {
          doc = readAdjacentDocLines();
	  if (got_nl && isIdent(parser->peekToken()))
	    extractorError("constant + doc + constant not allowed");
	}
      }
      else if (isDocComment(parser->peekToken())) {
        // The case with a doc comment, that must be followed by
        // one or more constants.
        doc = readAdjacentDocLines(); // consumes the \n after //... too!
        if (!isIdent(parser->peekToken(WITH_NL)))
          extractorError("expected enum constant");
        [consts, got_nl] = parseAdjacentEnumConstants();
        // consumes the "\n" after them too...
        if (isDocComment(parser->peekToken(WITH_NL)))
          extractorError("cant have doc both before and after enum constant");
      }
      else if (parser->peekToken() == "}")
        // reached the end of the enum { ... }.
        return;
      else {
        extractorError("expected doc comment or enum constant, got %O",
                       parser->peekToken());
	// NB: Only reached in FLAG_KEEP_GOING mode.

	if (parser->peekToken() == "") return;	// EOF.

	// Get rid of the erroneous token.
	parser->readToken();
	continue;
      }

      if (doc) {
        .DocParser.Parse parse =
	  .DocParser.Parse(doc->text, doc->position, flags);
        .DocParser.MetaData metadata = parse->metadata();
        if (metadata->appears || metadata->belongs)
          extractorError("@appears or @belongs not allowed in "
                         "doc for enum constant");
        if (metadata->type)
          extractorError("@%s not allowed in doc for enum constant",
                         metadata->type);
        doc->xml = parse->doc("_enumconstant");
      }
      parent->addChild(DocGroup(consts, doc));
    }
  }

  // parseAdjacentDecls consumes the "\n" that may follow the last decl
  protected array(array(PikeObject)|int(0..1)) parseAdjacentDecls(AutoDoc root,
								  Class|Module c)
  {
    array(PikeObject) res = ({ });
    for (;;) {
      // To get the correct line# :
      parser->skipNewlines();
      SourcePosition pos = parser->currentPosition->copy();

      object(PikeObject)|array(PikeObject)|Annotation p = parser->parseDecl();

      if (objectp(p) && p->is_annotation) {
	return ({ ({ p }), 0 });
      }

      multiset(string) allowSqueeze = (<
        "method",
        "class",
        "enum"
      >);

      if (isDocComment(parser->peekToken())) {
        if (!objectp(p) || !allowSqueeze[p->objtype])
          extractorError("sqeezed in doc only allowed for %s",
                         String.implode_nicely(indices(allowSqueeze)));
        Documentation doc = readAdjacentDocLines();
        p->squeezedInDoc = doc;
      }
      if (objectp(p) && p->objtype == "class") {
        ([object(Class)]p)->generics = parser->parseGenericsDecl();
	if (parser->peekToken() == "(") {
	  parser->eat("(");
	  parseCreateArgList([object(Class)] p);
	  parser->eat(")");
	}
        if (isDocComment(parser->peekToken())) {
          Documentation doc = readAdjacentDocLines();
          p->squeezedInDoc = doc;
        }
	if (parser->peekToken() == "{") {
	  parser->eat("{");
	  parseClassBody(root, [object(Class)] p, 0);
	  parser->eat("}");
	}
      }
      else if (objectp(p) && p->objtype == "modifier" &&
	       parser->peekToken() == "{") {
	parser->eat("{");
        parseClassBody(root, c, p->modifiers);
	parser->eat("}");
      }
      else if (objectp(p) && p->objtype == "enum") {
        parser->eat("{"); // after ("enum" opt_id) must come "{"
        parseEnumBody([object(Enum)] p);
        parser->eat("}");
      }
      else if (parser->peekToken() == "{") {
        int mark1 = parser->getReadDocComments();
        parser->skipBlock();
        int mark2 = parser->getReadDocComments();
        if (mark2 != mark1)
          extractorError("%d illegal doc comment lines inside block",
                         mark2 - mark1);
      }
      else
        parser->eat(";");
      while (parser->peekToken(WITH_NL) == ";")
        parser->readToken();
      foreach(arrayp(p) ? [array(object(PikeObject))]p :
	      ({ [object(PikeObject)]p }), PikeObject obj)
        obj->position = obj->position || pos;

      res += arrayp(p) ? p : ({ p });   // int x,y;  =>  array of PikeObject
      int(0..1) terminating_nl;
      if (parser->peekToken(WITH_NL) == "\n") { // we allow ONE "\n" inbetween
	terminating_nl = 1;
        parser->readToken(WITH_NL);
      }
      string s = parser->peekToken(WITH_NL);
      if (isDelimiter(s) || isDocComment(s))
        return ({ res, terminating_nl });
    }
  }

  // parseClassBody does the main work and scans the stream looking for:
  // 1.   doclines + decls, no blank line in between
  // 2.   decls + doclines,    -----  "  " -----
  // 3.   doclines                        (stand-alone doc)
  // 4.   decls                           (undocumented decls, discarded)
  //
  // If 'filename' is supplied, it will look for standalone doc comments
  // at the beginning of the file, and then the return value is that
  // Documentation for the file.
  object(Documentation)|zero
    parseClassBody(AutoDoc root, Class|Module c,
                   array(string)|zero defModifiers,
                   void|string filename,
                   void|string inAt)
  {
    object(Documentation)|zero filedoc = 0;

    object(Method)|zero implicit_create;
    foreach(c->docGroups, DocGroup dg) {
      foreach(dg->objects, PikeObject po) {
        if ((po->objtype == "method") && (po->name == "__create__")) {
          // Found.
          implicit_create = po;
          break;
        }
      }
    }
    int(0..1) explicit_create;

  mainloop:
    for (;;) {
      object(Documentation)|zero doc = 0;
      array(PikeObject) decls = ({ });
      int(0..1) got_nl;

      int docsMark = parser->getReadDocComments();

      string s = parser->peekToken();

      if (s == ";") {      // allow a semi-colon at the top level
        parser->eat(";");
        continue;
      }

      if (s == EOF || s == "}")         // end of class body reached
        break mainloop;
      if (isDocComment(s)) {
        doc = readAdjacentDocLines();    // read the doc comment lines
        s = parser->peekToken(WITH_NL);
        if (!isDelimiter(s)) {           // and decls that may follow
          [decls, got_nl] = parseAdjacentDecls(root, c);
          s = parser->peekToken(WITH_NL);
          if (isDocComment(s))
            extractorError("doc + decl + doc is forbidden!");
        }
      }
      else {
        [decls, got_nl] = parseAdjacentDecls(root, c);
        s = parser->peekToken(WITH_NL);
        if (isDocComment(s))
          doc = readAdjacentDocLines();
	if (got_nl) {
	  s = parser->peekToken(WITH_NL);
	  if ( !isDelimiter(s) )
	    extractorError("decl + doc + decl is forbidden!");
	}
      }

      foreach (decls, object(PikeObject)|Annotation obj)
        if (obj->squeezedInDoc) {
          if (sizeof(decls) > 1)
            extractorError(
              "declaration with sqeezed in documentation must stand alone"
            );
          if (doc)
            extractorError("duplicate documentation");
          doc = obj->squeezedInDoc;
        }
      array(PikeObject|Annotation) docDecls = ({ });

      if (!doc) {
        foreach (decls, object(PikeObject)|Annotation obj)
          if ((< "class", "enum" >)[obj->objtype] && obj->containsDoc()) {
	    extractorWarning("undocumented %s %O contains doc comments",
			     obj->objtype, obj->name);
	    doc = EmptyDoc;
	  }
      }


      object(.DocParser.Parse)|zero parse = 0;
      string|zero appears = 0;
      string|zero belongs = 0;
      if (doc) {
        parse = .DocParser.Parse(doc->text, doc->position, flags);
        MetaData meta = parse->metadata();
	object(Class)|Module parent = c;
        if (meta->type && meta->type != "decl") {
          string what = meta->type;
          switch(what) {
	    case "namespace":
	      parent = root;
	      // FALL_THROUGH
            case "module":
              // if (c->objtype == "class" && what == "module")
              //   extractorError("@module not allowed in class files");
              // fall through
            case "class":
              if (sizeof(decls))
                extractorError("@%s doc comment must stand alone", what);
	      object(Class)|object(Module) alreadyChild =
		parent->findChild(meta->name);
              object(Class)|object(Module) m;
              if (alreadyChild) {
                m = alreadyChild;
                if (m->objtype != what)
                  extractorError("found @%s %s, but %s has "
                                 "previously been defined as %s",
                                 what, m->name, m->name, m->objtype);
              }
              else {
                m = what == "class" ? Class() : Module();
                m->appears = meta->appears;
                m->belongs = meta->belongs;
                m->name = meta->name;
              }
              doc->xml = parse->doc("_" + what);
	      if (doc->xml && doc->xml != "") {
		if (alreadyChild && alreadyChild->documentation &&
		    alreadyChild->documentation->xml &&
		    alreadyChild->documentation->xml != "") {
                  extractorError("doc not allowed on reentrance into '%s %s'",
                                 m->objtype, m->name);
		}
		m->documentation = doc;
	      }
              if (!alreadyChild)
                parent->addChild(m);
              parseClassBody(root, m, 0, 0, what);
              continue mainloop;

            case "endclass":
            case "endmodule":
            case "endnamespace":
              if (sizeof(decls))
                extractorError("@%s doc comment must stand alone", meta->type);
              if (inAt != what - "end")
                extractorError("@%s has no matching %s",
                               meta->type, meta->type - "end");
              if (meta->name && meta->name != c->name)
                extractorError("'@%s %s' doesn't match '@%s %s'",
                               meta->type, meta->name, c->objtype, c->name || "");
              filedoc = 0;  // no filedoc possible
              break mainloop;

            default:
              extractorError("@%s is not allowed in Pike files", meta->type);
          }
        }
        docDecls = meta->decls;
        appears = meta->appears;
        belongs = meta->belongs;
      }
      // Objects added by @decl replace the true objects:
      if (sizeof(docDecls)) {
        if (sizeof(decls)) {
          if (sizeof(decls) != 1)
            extractorError("only one pike declaration can be combined with @decl %O", decls->position);
          foreach(docDecls, object(PikeObject)|Annotation d)
            if (decls[0]->objtype != d->objtype)
              extractorError("@decl of %s mismatches %s in pike code",
                            d->objtype, decls[0]->objtype);
          foreach(docDecls, object(PikeObject)|Annotation d)
            if ((d->objtype != "annotation") && (decls[0]->name != d->name))
              extractorError("@decl'd %s %s mismatches %s %s in pike code",
                             d->objtype, d->name, d->objtype, decls[0]->name);
        }
        decls = docDecls;
      }

      int wasNonGroupable = 0;
      multiset(string) nonGroupable = (<"class","module","enum">);
      foreach (decls, PikeObject obj)
        if (nonGroupable[obj->objtype]) {
          wasNonGroupable = 1;
          if (sizeof(decls) > 1 && doc && (doc != EmptyDoc)) {
            extractorError("%s are not groupable",
                           String.implode_nicely(indices(nonGroupable)));
          }
        }

      if (doc && !sizeof(decls)) {
	// the first stand-alone comment is allowed and is interpreted
	// as documentation for the class or module (foo.pike or bar.pmod)
	// _itself_.
	doc->xml = parse->doc((["class" : "_class",
				"module" : "_module",
				"namespace" : "_namespace"])[c->objtype]);

        if (filedoc) {
	  extractorError("documentation comment without destination");
	}

	filedoc = doc;
	// The @appears and @belongs directives regarded _this_file_
	c->appears = appears;
	c->belongs = belongs;
	doc = 0;
      }

      if (defModifiers && sizeof(defModifiers))
        foreach(decls, PikeObject obj)
          obj->modifiers |= defModifiers;

      mapping(string:int) contexts = ([]);

      foreach(decls, object(PikeObject)|Annotation obj) {
        if (implicit_create && (obj->objtype == "method") &&
            (obj->name == "create")) {
          // Prepend the arguments for __create__() to create().
          obj->argnames = implicit_create->argnames + obj->argnames;
          obj->argtypes = implicit_create->argtypes + obj->argtypes;
          explicit_create = 1;
        }
	if (obj->objtype == "annotation") {
	  c->annotations += ({ obj });
	}
	contexts[obj->objtype] = 1;
      }

      if (doc) {
        if (wasNonGroupable) {
          object(PikeObject) d;
	  foreach(decls, object(PikeObject) po) {
	    if (object_variablep(po, "documentation")) {
	      d = po;
	      break;
	    }
	    werror("Warning: Skipping declaration %O.\n", po);
	  }
	  if (!d) {
	    werror("No documentation variable in documented declarations.\n");
	    werror("decls: %O\n", decls);
	    exit(1);
	  }
          d->documentation = doc;
          d->appears = appears;
          d->belongs = belongs;
          c->addChild(d);
        }
        else {
          DocGroup d = DocGroup(decls, doc);
          d->appears = appears;
          d->belongs = belongs;
          c->addGroup(d);
        }

        string context;
        if (sizeof(indices(contexts)) == 1)
          context = "_" + indices(contexts)[0];
        else
          context = "_general";
        doc->xml = parse->doc(context);
      } else {
        // Make sure that all generics, inherits and imports are added:
	foreach(decls, PikeObject obj) {
	  if ((obj->objtype == "inherit") ||
	      (obj->objtype == "import")) {
	    c->addInherit(obj);
          } else if (obj->objtype == "generic") {
            DocGroup d = DocGroup(({ obj }), EmptyDoc);
            d->appears = appears;
            d->belongs = belongs;
            c->addGroup(d);
	  }
	}
      }
    } // for (;;)

    if (implicit_create && !explicit_create) {
      // Add create() that is a copy of __create__().
      Method create_method = Method();
      create_method->name = "create";
      foreach(({ "modifiers", "returntype", "argnames", "argtypes" }),
              string field) {
        create_method[field] = implicit_create[field];
      }
      c->docGroups += ({
        DocGroup(({ create_method }), EmptyDoc),
      });
    }

    return filedoc;
  }

  void parseCreateArgList(Class c) {
    Method createMethod = Method();
    createMethod->name = "__create__";
    createMethod->modifiers = ({ "protected", "local" });
    createMethod->returntype = VoidType();
    createMethod->argnames = ({});
    createMethod->argtypes = ({});

    array(Variable) createVars = ({});

    for (;;) {
      Documentation doc;
      if (isDocComment(parser->peekToken())) {
	doc = readAdjacentDocLines();
	object(.DocParser.Parse) parse =
	  .DocParser.Parse(doc->text, doc->position, flags);
	.DocParser.MetaData metadata = parse->metadata();
	doc->xml = parse->doc("_variable");
      }
      array(string) m = parser->parseModifiers();
      Type t = parser->parseOrType();
      if (!t) break;
      if (parser->peekToken() == "...") {
	t = VarargsType(t);
	parser->eat("...");
      }
      Variable var = Variable();
      var->name = parser->eatIdentifier();
      createMethod->argnames += ({ var->name });
      createMethod->argtypes += ({ t });
      var->modifiers = m;
      if (object_program(t) == VarargsType) {
	// Convert vararg types to array types.
	ArrayType new_type = ArrayType();
	new_type->valuetype = t->type;
	t = new_type;
      }
      var->type = t;
      if (isDocComment(parser->peekToken())) {
	if (doc)
	  extractorError("doc + decl + doc is forbidden!");
	doc = readAdjacentDocLines();
	object(.DocParser.Parse) parse =
	  .DocParser.Parse(doc->text, doc->position, flags);
	.DocParser.MetaData metadata = parse->metadata();
	doc->xml = parse->doc("_variable");
      }
      if (doc) {
	c->docGroups += ({ DocGroup(({ var }), doc) });
      } else {
	createVars += ({ var });
      }
      if (parser->peekToken() == "=") {
        // Default value.
        Type ot = OrType();
        ot->types = ({ createMethod->argtypes[-1], VoidType() });
        createMethod->argtypes[-1] = ot;
        parser->eat("=");
        parser->skipUntil((< ",", ")", ";", EOF >));
      }
      if (parser->peekToken() != ",") break;
      parser->eat(",");
    }

    c->docGroups += ({
      DocGroup(({ createMethod }), EmptyDoc),
    });
    if (sizeof(createVars)) {
      c->docGroups += ({
	DocGroup(createVars, EmptyDoc),
      });
    }
  }

} // private class Extractor

//! Extract documentation for a Pike namespace.
//!
//! @seealso
//!   @[extractModule()], @[extractClass()]
void extractNamespace(AutoDoc root, string s, void|string filename,
		      void|string namespaceName, void|.Flags flags)
{
  if (undefinedp(flags)) flags = .FLAG_NORMAL;
  Extractor e = Extractor(s, filename, flags);
  NameSpace ns = NameSpace();
  ns->name = namespaceName || filename;
  root->addChild(ns);
  Documentation doc = e->parseClassBody(root, ns, 0, filename);
  ns->documentation = doc;
}

//! Extract documentation for a Pike module.
//!
//! @seealso
//!   @[extractNamespace()], @[extractClass()]
void extractModule(AutoDoc root, Module parent, string s, void|string filename,
		   void|string moduleName, void|.Flags flags)
{
  if (undefinedp(flags)) flags = .FLAG_NORMAL;
  Extractor e = Extractor(s, filename, flags);
  Module m = Module();
  m->name = moduleName || filename;
  parent->addChild(m);
  Documentation doc = e->parseClassBody(root, m, 0, filename);
  m->documentation = doc;

  // If there's a _module_value it replaces the module itself.
  PikeObject module_value = m->findChild("_module_value");
  if (module_value) {
    module_value->name = m->name;
    parent->children -= ({ m });
    parent->addChild(module_value);
  }
}

//! Extract documentation for a Pike class (aka program).
//!
//! @seealso
//!   @[extractNamespace()], @[extractModule()]
void extractClass(AutoDoc root, Module parent, string s, void|string filename,
		  void|string className, void|.Flags flags)
{
  if (undefinedp(flags)) flags = .FLAG_NORMAL;
  Extractor e = Extractor(s, filename, flags);
  Class c = Class();
  c->name = className || filename;
  parent->addChild(c);
  Documentation doc = e->parseClassBody(root, c, 0, filename);
  c->documentation = doc;
}
