//========================================================================
// PARSING OF DOCUMENTATION COMMENTS
//========================================================================

inherit "module.pmod";

#define DEB werror("###DocParser.Pike: %d\n", __LINE__);

static void parseError(string s, mixed ... args) {
  s = sprintf(s, @args);
  werror(s+"\n");
  s = "DocParser error: " + s;
  throw(({ s, 0 }));
}

inherit .PikeObjects;

class MetaData {
  string type;
  string name; // if (type == "class", "module", "endmodule", or "endclass")
  array(PikeObject) decls = ({});
  string belongs = 0;
  string appears = 0;
  int global = 0;
}

constant EOF = .PikeParser.EOF;

constant METAKEYWORD = 1;          // @decl
constant BRACEKEYWORD = 2;         // @i{ @}
constant DELIMITERKEYWORD = 3;     // @param
constant CONTAINERKEYWORD = 4;     // @mapping
constant SINGLEKEYWORD = 5;        // @deprecated
constant ENDKEYWORD = 6;
constant ERRORKEYWORD = 7;

constant TEXTTOKEN = 8;
constant ENDTOKEN = 9;

// The following is the "DTD" of
mapping(string : int) keywordtype =
([
  "appears" : METAKEYWORD,
  "belongs" : METAKEYWORD,
  "global" : METAKEYWORD,
  "class" : METAKEYWORD,
  "endclass" : METAKEYWORD,
  "module" : METAKEYWORD,
  "endmodule" : METAKEYWORD,
  "decl" : METAKEYWORD,

  "i" : BRACEKEYWORD,
  "tt" : BRACEKEYWORD,
  "ref" : BRACEKEYWORD,

  "deprecated" : SINGLEKEYWORD,

  "example" : DELIMITERKEYWORD,
  "note" : DELIMITERKEYWORD,
  "param" : DELIMITERKEYWORD,
  "seealso" : DELIMITERKEYWORD,

  "mapping" : CONTAINERKEYWORD, "member" : DELIMITERKEYWORD,
  "multiset" : CONTAINERKEYWORD, "index" : DELIMITERKEYWORD,
  "array" : CONTAINERKEYWORD, "elem" : DELIMITERKEYWORD,

  "dl" : CONTAINERKEYWORD, "item" : DELIMITERKEYWORD,
]);

mapping(string : array(string)) attributenames =
([
  "item" : ({ "name" }),
  "param" : ({ "name" }),
  "mapping" : ({ "name" }),
  "array" : ({ "name" }),
  "multiset" : ({ "name" }),
]);

static constant standard = (< "note", "example", "seealso", "deprecated" >);

mapping(string : multiset(string)) allowedChildren =
(["_general" : standard,
  "_method" : (< "param", "returns" >) + standard,
  "mapping" : (< "member" >),
  "multiset": (< "index" >),
  "array"   : (< "elem" >),
  "dl"      : (< "item" >),
]);

mapping(string : multiset(string)) allowGrouping =
(["param" : (< "param" >),
]);

// argHandlers:
//
//   Contains functions that handle keywords with non-standard arg list
//   syntax. The function for each keyword can return a mapping or a string:
//
//   If a mapping(string:string) is returned, it is interpreted as the
//   attributes to put in the tag.
//   If a string is returned, it is an XML fragment that gets inserted
//   inside the tag.
mapping(string : function(string, string : string)
        | function(string, string : mapping(string : string))) argHandlers =
([
  "member" : memberArgHandler,
  "elem" : elemArgHandler,
  "index" : indexArgHandler,
  "deprecated" : deprArgHandler,
]);

static string memberArgHandler(string keyword, string arg) {
  //  werror("This is the @member arg handler ");
  .PikeParser parser = .PikeParser(arg);
  //  werror("&&& %O\n", arg);
  Type t = parser->parseOrType();
  if (!t)
    parseError("@member: expected type, got %O", arg);
  //  werror("%%%%%% got type == %O\n", t->xml());
  string s = parser->parseLiteral() || parser->parseIdents();
  if (!s)
    parseError("@member: expected indentifier or literal constant, got %O", arg);
  parser->eat(EOF);
  return xmltag("type", t->xml())
    + xmltag("index", xmlquote(s));
}

static string elemArgHandler(string keyword, string arg) {
  //  werror("This is the @elem arg handler\n");
  .PikeParser parser = .PikeParser(arg);
  Type t = parser->parseOrType();
  if (!t)
    parseError("@elem: expected type, got %O", arg);
  if (parser->peekToken() == "...") {
    t = VarargsType(t);
    parser->eat("...");
  }
  string s = parser->parseLiteral() || parser->parseIdents();
  string s2 = 0;
  int dots = 0;
  if (parser->peekToken() == "..") {
    dots = 1;
    parser->readToken();
    s2 = parser->parseLiteral() || parser->parseIdents();
  }
  parser->eat(EOF);
  if (s)
    if (s2)
      return xmltag("minindex", xmlquote(s))
        + xmltag("maxindex", xmlquote(s2));
    else
      return xmltag(dots ? "minindex" : "index", xmlquote(s));
  else
    if (s2)
      return xmltag("maxindex", xmlquote(s2));
    else
      parseError("@elem: expected identifier or literal");
}

static string indexArgHandler(string keyword, string arg) {
  //  werror("indexArgHandler\n");
  .PikeParser parser = .PikeParser(arg);
  string s = parser->parseLiteral();
  if (!s)
    parseError("@index: expected identifier, got %O", arg);
  parser->eat(EOF);
  return xmltag("value", xmlquote(s));
}

static string deprArgHandler(string keyword, string arg) {
  .PikeParser parser = .PikeParser(arg);
  if (parser->peekToken() == EOF)
    return "";
  string res = "";
  for (;;) {
    string s = parser->parseIdents();
    if (!s)
      parseError("@deprecated: expected list identifier, got %O", arg);
    res += xmltag("name", s);
    if (parser->peekToken() == EOF)
      return res;
    parser->eat(",");
  }
}

static mapping(string : string) standardArgHandler(string keyword, string arg)
{
  array(string) args = ({});
  arg += "\0";

  int i = 0;
  for (;;) {
    while (isSpaceChar(arg[i]))
      ++i;
    if (!arg[i])
      break;
    if (arg[i] == '"' || arg[i] == '\'') {
      int quotechar = arg[i];
      ++i;
      int start = i;
      while(arg[i]) {
        if (arg[i] == quotechar)
          if (arg[i + 1] == quotechar)
            ++i;
          else
            break;
        // Or should we have \n style quoting??
        //else if (arg[i] == '\\' && arg[i + 1])
        //  ++i;
        ++i;
      }
      if (!arg[i])
        parseError("keyword parameter is unterminated string constant");
      else
        ++i;
      string s = arg[start .. i - 2];
      array(string) replacefrom = ({ quotechar == '"' ? "\"\"" : "''" });
      array(string) replaceto = ({ quotechar == '"' ?  "\""  : "'" });
      // Or should we have \n style quoting??
      //array(string) replacefrom = ({ "\\n", "\\t", "\\r", "\\\"", "\\\\",
      //                               quotechar == '"' ? "\"\"" : "''" });
      //array(string) replaceto = ({ "\n", "\t", "\r", "\"", "\\",
      //                             quotechar == '"' ?  "\""  : "'" });
      s = replace(s,replacefrom, replaceto);
      args += ({ s });
    }
    else {
      int start = i;
      while (arg[i] && !isSpaceChar(arg[i]))
        ++i;
      args += ({ arg[start .. i - 1] });
    }
  }

  mapping(string:string) res = ([]);

  array(string) attrnames = attributenames[keyword];
  int attrcount = sizeof(attrnames || ({}) );
  if (attrcount < sizeof(args))
    parseError("@keyword with too many parameters");
  for (int i = 0; i < sizeof(args); ++i)
    res[attrnames[i]] =  attributequote(args[i]);
  return res;
}

static string|mapping(string:string) getArgs(string keyword, string arg) {
  return (argHandlers[keyword] || standardArgHandler)(keyword, arg);
}

static int getKeywordType(string keyword) {
  if (keywordtype[keyword])
    return keywordtype[keyword];
  if (strlen(keyword) > 3 && keyword[0..2] == "end")
    return ENDKEYWORD;
  return ERRORKEYWORD;
}

static int getTokenType(array(string) | string token) {
  if (arrayp(token))
    return getKeywordType(token[0]);
  if (!token)
    return ENDTOKEN;
  return TEXTTOKEN;
}

static int isSpaceChar(int char) {
  return (< '\t', '\n', ' ' >) [char];
}

static int isKeywordChar(int char) {
  return char >= 'a' && char <= 'z';
}

static array(string) extractKeyword(string line) {
  line += "\0";
  int i = 0;
  while (i < strlen(line) && isSpaceChar(line[i]))
    ++i;
  if (line[i++] != '@')
    return 0;
  int keywordstart = i;
  while (isKeywordChar(line[i]))
    ++i;
  if (i == keywordstart || line[i] && !isSpaceChar(line[i]))
    return 0;
  string keyword = line[keywordstart .. i - 1];
  //  if (getKeywordType(keyword) == METAKEYWORD)
  return ({ keyword, line[i .. strlen(line) - 2] });  // skippa "\0" ...
}

static int allSpaces(string s) {
  for (int i = strlen(s) - 1; i >= 0; --i)
    if (s[i] != ' ' && s[i] != '\t')
      return 0;
  return 1;
}

static array(string|array(string)) split(string s) {
  array(string) lines = (s - "\r" - "@\n") / "\n";
  array(string|array(string)) res = ({ });
  string currentText = "";
  foreach(lines, string line) {
    array(string) keyword = extractKeyword(line);
    if (keyword) {
      if (strlen(currentText))
        res += ({ currentText });
      currentText = "";
      res += ({ keyword });
    }
    else {
      if (allSpaces(line))
        currentText += "\n";
      else
        currentText += line + "\n";
    }
  }
  if (strlen(currentText))
    res += ({ currentText });
  return res;
}

static string xmlNode(string s) {  /* now, @xml works like @i & @tt */
  s += "\0";
  string res = "";
  int i = 0;
  array(string) tagstack = ({ });
  int inXML = 0;

  while (s[i] == '\n')         // strip leading empty lines.
    ++i;
  while (s[i]) {
    if (s[i] == '@') {
      ++i;
      if (s[i] == '@') {
        res += "@";
        ++i;
      }
      else if (s[i] == '[') {  // @ref shortcut
        int j = ++i;
        multiset(int) forbidden = (<'@','\n'>);
	int level = 1;
        while (s[j] && level && !forbidden[s[j]] ) {
	  if (s[j] == ']') {
	    level--;
	  } else if (s[j] == '[') {
	    level++;
	  }
          ++j;
	}
        if (level) {
	  if (forbidden[s[j]]) {
	    parseError("forbidden character inside @[...]: %O", s[i-2..j]);
	  }
          parseError("@[ without matching ]");
	}
        res += xmltag("ref", xmlquote(s[i .. j - 2]));
        i = j;
      }
      else if (s[i] == '}') {
        if (!sizeof(tagstack)) {
          werror("///\n%O\n///\n", s);
          parseError("closing @} without corresponding @keyword{");
        }
        if (tagstack[0] == "xml")
          --inXML;
        else
          res += closetag(tagstack[0]);
        tagstack = tagstack[1..];
        ++i;
      }
      else if (isKeywordChar(s[i])) {
        int start = i;
        while (isKeywordChar(s[++i]))
          ;
        string keyword = s[start .. i - 1];
        if (s[i] != '{' || keyword == "")
          parseError("expected @keyword{");
        ++i;
        tagstack = ({ keyword }) + tagstack;
        if (keyword == "xml")
          ++inXML;
        else
          res += opentag(keyword);
      }
      else
        parseError("expected @keyword{ or @}, got \"" + s[i-1 .. i+4] + "\"");
    }
    else if (s[i] == '\n' && !sizeof(tagstack)) {
      if (s[++i] == '\n') {              // at least two conseq. '\n'
        while (s[i] == '\n')
          ++i;
        if (s[i])                        // no </p><p> if it was trailing stuff
          res += "</p>\n<p>";
      }
      else
        res += "\n";
    }
    else {
      string add = s[i..i];
      if (inXML == 0)
        add = replace(add, ({ "<", ">", "&" }),   // if inside @xml{...@}, escape it
                       ({ "&lt;", "&gt;", "&amp;" }) );
      res += add;
      ++i;
    }
  }
  if (sizeof(tagstack))
    parseError("@" + tagstack[0] + "{ without matching @}");
  return "<p>" + res + "</p>\n";
}

static class DocParserClass {
  static array(array(string)|string) tokens;

  // Read until the next delimiter token on the same level, or to
  // the end.
  static string xmlText() {
    string res = "";
    for (;;) {
      switch (getTokenType(tokens[0])) {
        case TEXTTOKEN:
          res += xmlNode(tokens[0]);
          tokens = tokens[1..];
          break;
        case CONTAINERKEYWORD:
          string keyword = tokens[0][0];
          string arg = tokens[0][1];

          res += "<" + keyword;
          string|mapping(string:string) args = getArgs(keyword, arg);
          if (mappingp(args))
            foreach(indices(args), string s)
              res += " " + s + "=\"" + args[s] + "\"";
          res += ">";
          if (stringp(args))
            res += args;

          tokens = tokens[1..];
          res += xmlContainerContents(keyword);
          if (!(arrayp(tokens[0]) && tokens[0][0] == "end" + keyword))
            parseError("@keyword without matching @endkeyword");
          res += closetag(keyword);

          tokens = tokens[1..];
          break;
        default:
          return res;
      }
    }
  }

  static string xmlContainerContents(string container) {
    string res = "";
    switch( getTokenType(tokens[0]) ) {
      case ENDTOKEN:
        return "";
      case TEXTTOKEN:
        // SHOULD WE KILL EMPTY TEXT LIKE THIS ??
        {
        string text = tokens[0];
        if (text - "\n" - "\t" - " " == "") {
          tokens = tokens[1..];
          break;
        }
        else
          ; // fall through
        }
      case CONTAINERKEYWORD:
        res += opentag("text") + xmlText() + closetag("text");
        break;
      case ERRORKEYWORD:
        //werror("bosse larsson: %O\n", tokens);
        parseError("unknown keyword: @" + tokens[0][0]);
    }
    for (;;) {
      if (! (<SINGLEKEYWORD, DELIMITERKEYWORD>) [getTokenType(tokens[0])] )
        return res;

      string single = 0;
      array(string) keywords = ({});
      res += opentag("group");
      while ( (<SINGLEKEYWORD, DELIMITERKEYWORD>) [getTokenType(tokens[0])] ) {
        string keyword = tokens[0][0];
        single = single || getTokenType(tokens[0]) == SINGLEKEYWORD && keyword;
        multiset(string) allow = allowedChildren[container];
        if (!allow || !allow[keyword])
          parseError("@" + keyword + " is not allowed inside @" + container);

        multiset(string) allowGroup = allowGrouping[keyword] || ([]);
        foreach (keywords, string k)
          if (!allowGroup[k])
            parseError("@" + keyword + " may not be grouped together with @" + k);
        keywords += ({ keyword });

        string arg = tokens[0][1];
        res += "<" + keyword;
        string|mapping(string:string) args = getArgs(keyword, arg);
        if (mappingp(args)) {
          foreach(indices(args), string s)
            res += " " + s + "=\"" + args[s] + "\"";
          res += "/>";
        }
        else if (stringp(args))
          res += ">" + args + "</" + keyword + ">";
        else
          res += "/>";

        tokens = tokens[1..];
      }
      switch(getTokenType(tokens[0])) {
        case TEXTTOKEN:
        // SHOULD WE KILL EMPTY TEXT LIKE THIS ??
          {
          string text = tokens[0];
          if (text - "\n" - "\t" - " " == "") {
            tokens = tokens[1..];
            break;
          }
          else
            ; // fall through
          }
        case CONTAINERKEYWORD:
          if (single)
            parseError("cannot have text after @" + single);
          res += opentag("text") + xmlText() + closetag("text");
      }
      res += closetag("group");
    }
  }

  static void create(string | array(string|array(string)) s) {
    tokens = (arrayp(s) ? s : split(s)) + ({ 0 }); // end sentinel
  }

  MetaData getMetaData() {
    int i = 0;
    while (arrayp(tokens[i]) && getKeywordType(tokens[i][0]) == METAKEYWORD)
      ++i;
    tokens[0 .. i - 1];
    MetaData meta = MetaData();
    foreach (tokens[0 .. i - 1], [string keyword, string arg])
      switch (keyword) {
        case "class":
        case "module":
          {
          if (meta->appears)
            parseError("@appears before @%s", keyword);
          if (meta->belongs)
            parseError("@belongs before @%s", keyword);
          if (meta->type)
            parseError("@%s can not be combined with @%s", keyword, meta->type);
          meta->type = keyword;
          .PikeParser nameparser = .PikeParser(arg);
          string s = nameparser->readToken();
          if (!isIdent(s) || nameparser->readToken() != EOF)
            parseError("expected %s name, got %O", keyword, s);
          meta->name = s;
          }
          break;

        case "decl":
          {
          if (!meta->type)
            meta->type = "decl";
          else if (meta->type != "decl")
            parseError("@decl can not be combined with @%s", meta->type);
          if (meta->appears)
            parseError("@appears before @decl");
          if (meta->belongs)
            parseError("@belongs before @decl");
          meta->type = "decl";

          .PikeParser declparser = .PikeParser(arg);
          PikeObject p = declparser->parseDecl(
            ([ "allowArgListLiterals" : 1 ])
          ); // with constants/literals
          string s = declparser->peekToken();
          if (s != ";" && s != EOF)
            parseError("expected end of line, got %O", s);
          meta->decls += ({ p });
          }
          break;

        case "global":
          if (meta->type == "class" || meta->type == "decl"
              || meta->type == "module" || !meta->type)
          {
            if (meta->global)
              parseError("duplicate @global");
            .PikeParser ucko = .PikeParser(arg);
            if (ucko->peekToken() != EOF)
              parseError("expected end of line, got %O", arg);
            meta->global = 1;
          }
          else
            parseError("@global not allowed here");
          break;

        case "appears":
          if (meta->type == "class" || meta->type == "decl"
              || meta->type == "module" || !meta->type)
          {
            if (meta->appears)
              parseError("duplicate @appears");
            if (meta->belongs)
              parseError("both @appears and @belongs");
            .PikeParser idparser = .PikeParser(arg);
            string s = idparser->parseIdents();
            if (!s)
              parseError("expected identifier, got %O", arg);
            meta->appears = s;
          }
          else
            parseError("@appears not allowed here");
          break;

        case "belongs":
          if (meta->type == "class" || meta->type == "decl"
               || meta->type == "module" || !meta->type)
          {
            if (meta->belongs)
              parseError("duplicate @belongs");
            if (meta->appears)
              parseError("both @appears and @belongs");
            .PikeParser idparser = .PikeParser(arg);
            string s = idparser->parseIdents();
            if (!s && idparser->peekToken() != EOF)
              parseError("expected identifier or blank, got %O", arg);
            meta->belongs = s || "";  // blank is allowed too, you know ..
          }
          break;

        case "endclass":
        case "endmodule":
          if (i > 1)
            parseError("@%s must stand alone", keyword);
          meta->type = keyword;
          .PikeParser nameparser = .PikeParser(arg);
          string s = nameparser->peekToken();
          if (s != EOF)
            if (isIdent(s))
              meta->name = s;
            else
              parseError("expected %s name, got %O", keyword, s);
          break;

        default:
          parseError("illegal keyword: @%s", keyword);
      }
    tokens = tokens[i ..];
    return meta;
  }


  array(array(string)) getMetaDataOLD() {  // OLD VERSION
    // collect all meta info at the start of the block
    int i = 0;
    //    werror("\n%%%%%% %O\n", tokens);
    while (arrayp(tokens[i]) && getKeywordType(tokens[i][0]) == METAKEYWORD)
      ++i;
    array(array(string)) metadata = tokens[0..i-1];
    tokens = tokens[i..];
    return metadata;
  }

  string getDoc(string context) {
    string xml = xmlContainerContents(context);
    switch (getTokenType(tokens[0])) {
      case ENDTOKEN:
        break;
      case ERRORKEYWORD:
        parseError("illegal keyword: @"+ tokens[0][0]);
      default:
        parseError("expected end of doc comment");
    }
    return xml;
  }
}

// Each of the arrays in the returned array can be fed to
// Parse::create()
array(array(string|array(string))) splitDocBlock(string block) {
  array(string|array(string)) tokens = split(block);
  array(string|array(string)) current = ({ });
  array(array(string|array(string))) result = ({ });
  int prevMeta = 0;
  foreach (tokens, string|array(string) token) {
    int meta = arrayp(token) && getKeywordType(token[0]) == METAKEYWORD;
    if (meta && !prevMeta && sizeof(current)) {
      result += ({ current });
      current = ({ });
    }
    prevMeta = meta;
    current += ({ token });
  }
  result += ({ current });
  return result;
}

// This is a class, because you need to examine the meta lines
// _before_ you can determine which context to parse the
// actual doc lines in.
class Parse {
  inherit DocParserClass;
  static int state;
  static MetaData mMetadata = 0;
  static string mDoc = 0;
  static string mContext = 0;
  SourcePosition sourcePos = 0;
  void create(string | array(string|array(string)) s, SourcePosition|void sp) {
    ::create(s);
    state = 0;
    sourcePos = sp;
  }

  MetaData metadata() {
    mixed err = catch {
      if (state == 0) {
        ++state;
        mMetadata = ::getMetaData();
      }
      return mMetadata;
    };
    if (sourcePos)
      throw(({ err[0], sourcePos }));
  }

  string doc(string context) {
    mixed err = catch {
      if (state == 1) {
        ++state;
        mContext == context;
        mDoc = ::getDoc(context);
      }
      else if (state == 0 || state > 1 && mContext != context)
        return 0;
      return mDoc;
    };
    if (sourcePos)
      throw(({ err[0], sourcePos }));
  }
}
