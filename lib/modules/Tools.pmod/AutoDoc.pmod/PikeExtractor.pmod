//========================================================================
// EXTRACTION OF DOCUMENTATION FROM PIKE SOURCE FILES
//========================================================================

#pragma strict_types

static inherit .PikeObjects;
static inherit .DocParser;

#define DEB werror("###%s:%d\n", __FILE__,  __LINE__);

//========================================================================
// DOC EXTRACTION
//========================================================================

static private class Extractor {
  static constant WITH_NL = .PikeParser.WITH_NL;

  static private string filename;
  static private .PikeParser parser;

  static void create(string s, string filename) {
    parser = .PikeParser(s, filename);
    local::filename = filename;
  }
  static void extractorError(string message, mixed ... args) {
    message = sprintf(message, @args);
    werror("Pike extractor error!\n");
    message = "Pike extractor error: " + message;
    throw ( ({ message, parser->currentline }) );
  }

  static int isDocComment(string s) {
    return (strlen(s) >= 3 && s[0..2] == DOC_COMMENT);
  }

  static string stripDocMarker(string s) {
    if (isDocComment(s))
      return s[3..];
    throw("OOPS");
  }

  // readAdjacentDocLines consumes the doc lines AND the "\n" after the last one
  static Documentation readAdjacentDocLines() {
    Documentation res = Documentation();
    string s = parser->peekToken();
    string text = "";
    int firstline = 0;
    int lastline;
    while (isDocComment(s)) {
      text += stripDocMarker(s) + "\n";
      lastline = parser->currentline;
      parser->readToken();              // Grok the doc comment string
      if (!firstline)
        firstline = parser->currentline;
      s = parser->peekToken(WITH_NL);
      if (s == "\n")
        parser->readToken(WITH_NL);     // Skip the corresp. NL
      else
        break;             // Ought to be EOF
      s = parser->peekToken(WITH_NL);   // NL will break the loop
    }
    res->text = text;
    res->position = SourcePosition(filename, firstline, lastline);
    return res;
  }

  // check if the token is one that can not start a declaration
  // adjacent to the last one
  static int isDelimiter(string token) {
    return (< "\n", "}", "EOF" >) [token];
  }

  // parseAdjacentDecls consumes the "\n" that may follow the last decl
  static array(PikeObject) parseAdjacentDecls() {
    array(PikeObject) res = ({ });
    for (;;) {
      // To get the correct line# :
      while (parser->peekToken(WITH_NL) == "\n")
        parser->readToken(WITH_NL);
      int firstline = parser->currentline;
      object(PikeObject)|array(PikeObject) p = parser->parseDecl();

      if (isDocComment(parser->peekToken())) {
        if (!objectp(p) && (p->objtype == "method" || p->objtype == "class"))
          extractorError("sqeezed in doc only allowed for classes and methods");
        Documentation doc = readAdjacentDocLines();
        p->squeezedInDoc = doc;
      }
      // TODO: parse doc squeezed in between the head and the body
      if (!arrayp(p) && p->objtype == "class" && parser->peekToken() == "{") {
        parser->eat("{");
        parseClassBody([object(Class)] p);
        parser->eat("}");
      }
      else if (parser->peekToken() == "{") {
        int mark1 = parser->getReadDocComments();
        parser->skipBlock();
        int mark2 = parser->getReadDocComments();
        if (mark2 != mark1)
          extractorError("%d illegal doc comment lines inside block", mark2-mark1);
      }
      else
        parser->eat(";");
      SourcePosition pos =
        SourcePosition(filename, firstline, parser->currentline);
      foreach(arrayp(p) ? [array(object(PikeObject))]p :
	      ({ [object(PikeObject)]p }), PikeObject obj)
        obj->position = pos;

      res += arrayp(p) ? p : ({ p });   // int x,y;  =>  array of PikeObject
      if (parser->peekToken(WITH_NL) == "\n")   // we allow ONE "\n" inbetween
        parser->readToken(WITH_NL);
      string s = parser->peekToken(WITH_NL);
      if (isDelimiter(s) || isDocComment(s))
        return res;
    }
  }
  // parseClassBody does the main work and scans the stream looking for:
  // 1.   doclines + decls, no blank line in between
  // 2.   decls + doclines,    -----  "  " -----
  // 3.   doclines                        (stand-alone doc)
  // 4.   decls                           (undocumented decls, discarded)
  //
  // If 'filename' is supplied, it will look for standalone doc comments
  // at the beginning of the file.
  Documentation parseClassBody(Class|Module c, void|string filename) {
    Documentation filedoc = 0;
    for (;;) {
      Documentation doc = 0;
      array(PikeObject) decls = ({ });

      int docsMark = parser->getReadDocComments();

      string s = parser->peekToken();
      if (s == EOF || s == "}")         // end of class body reached
        return filedoc;
      if (isDocComment(s)) {
        doc = readAdjacentDocLines();    // read the doc comment lines
        s = parser->peekToken(WITH_NL);
        if (!isDelimiter(s)) {           // and decls that may follow
          decls = parseAdjacentDecls();
          s = parser->peekToken(WITH_NL);
          if (isDocComment(s))
            extractorError("doc + decl + doc  is forbidden!");
        }
      }
      else {
        decls = parseAdjacentDecls();
        s = parser->peekToken(WITH_NL);
        if (isDocComment(s))
          doc = readAdjacentDocLines();
        else if ( !isDelimiter(s) )
          extractorError("decl + doc + decl  is forbidden!");
      }

      foreach (decls, PikeObject obj)
        if (obj->squeezedInDoc) {
          if (sizeof(decls) > 1)
            extractorError(
              "declaration with sqeezed in documentation must stand alone"
            );
          if (doc)
            extractorError("duplicate documentation");
          doc = obj->squeezedInDoc;
        }
      array(PikeObject) docDecls = ({ });

      object(.DocParser.Parse) parse = 0;
      string appears = 0;
      string belongs = 0;
      int global = 0;
      if (doc) {
        parse = .DocParser.Parse(doc->text, doc->position);
        MetaData meta = parse->metadata();
        if (meta->type && meta->type != "decl")
          extractorError("@%s is not allowed in Pike files", meta->type);
        docDecls = meta->decls;
        appears = meta->appears;
        belongs = meta->belongs;
        global = meta->global;
      } else
        foreach (decls, PikeObject obj)
          if (obj->objtype == "class" &&
	      ([object(Class)]obj)->containsDoc())
            extractorError("undocumented class contains doc comments");

      // Objects added by @decl replace the true objects:
      if (sizeof(docDecls)) {
        if (sizeof(decls)) {
          if (sizeof(decls) == 1 && decls[0]->objtype == "method") {
            string name = decls[0]->name;
            foreach (docDecls, PikeObject p) {
              if (p->objtype != "method")
                extractorError("@decl can override only methods");
              if (p->name != name)
                extractorError("@decl and pike methods must have the same name");
            }
          }
          else
            extractorError("inconsistency between @decl and pike code");
        }
        decls = docDecls;
      }

      int wasClassOrModule = 0;
      foreach (decls, PikeObject obj)
        if (obj->objtype == "class" || obj->objtype == "module") {
          wasClassOrModule = 1;
          if (sizeof(decls) > 1)
            extractorError("cannot group classes or modules");
        }

      if (doc && !sizeof(decls))
        if (!filename || filedoc)
          extractorError("documentation comment without destination");
        else {
          // the first stand-alone comment is allowed and is interpreted
          // as documentation for the file(foo.pike or bar.pmod) _itself_.
          doc->xml = parse->doc((["class" : "_class_file",
                                  "module" : "_module_file"])[c->objtype]);
          filedoc = doc;
          doc = 0;
        }

      mapping(string:int) contexts = ([]);

      // Make sure that all inherits are added:
      foreach(decls, PikeObject obj)
        switch (obj->objtype) {
          case "inherit":
            c->AddInherit(obj);
            if (doc && sizeof(decls) > 1)
              extractorError("inherit can not be documented together"
                             " with other declarations");
            // fall through
          default:
            contexts[obj->objtype] = 1;
        }


      if (doc) {
        if (wasClassOrModule) {
          object(Class)|object(Module) d =
	    [object(Class)|object(Module)]decls[0];
          d->documentation = doc;
          d->appears = appears;
          d->belongs = belongs;
          d->global = global;
          c->AddChild(d);
        }
        else {
          DocGroup d = DocGroup(decls, doc);
          d->appears = appears;
          d->belongs = belongs;
          d->global = global;
          c->AddGroup(d);
        }

        string context;
        if (sizeof(indices(contexts)) == 1)
          context = "_" + indices(contexts)[0];
        else
          context = "_general";
        doc->xml = parse->doc(context);
      } // if (doc)
    } // for (;;)
  }

} // static private class Extractor

Module extractModule(string s, void|string filename, void|string moduleName) {
  Extractor e = Extractor(s, filename);
  Module m = Module();
  m->name = moduleName || filename;
  Documentation doc = e->parseClassBody(m, filename);
  m->documentation = doc;
  // if there was no documentation in the file whatsoever
  if (!doc && sizeof(m->docGroups) == 0)
    return 0;
  return m;
}

Class extractClass(string s, void|string filename, void|string className) {
  Extractor e = Extractor(s, filename);
  Class c = Class();
  c->name = className || filename;
  Documentation doc = e->parseClassBody(c, filename);
  c->documentation = doc;
  // if there was no documentation in the file...
  if (!doc && sizeof(c->docGroups) == 0)
    return 0;
  return c;
}
