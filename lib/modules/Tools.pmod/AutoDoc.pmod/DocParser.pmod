#pike __REAL_VERSION__

//========================================================================
// PARSING OF DOCUMENTATION COMMENTS
//========================================================================

inherit "module.pmod";
inherit .PikeObjects;

#include "./debug.h"

class MetaData {
  string type;
  string name; // if (type == "class", "module", "endmodule", or "endclass")
  array(PikeObject) decls = ({});
  string belongs = 0;
  string appears = 0;
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
  "class" : METAKEYWORD,
  "endclass" : METAKEYWORD,
  "module" : METAKEYWORD,
  "endmodule" : METAKEYWORD,
  "namespace" : METAKEYWORD,
  "endnamespace" : METAKEYWORD,
  "decl" : METAKEYWORD,

  "b" : BRACEKEYWORD,
  "i" : BRACEKEYWORD,
  "u" : BRACEKEYWORD,
  "tt" : BRACEKEYWORD,
  "url" : BRACEKEYWORD,
  "pre" : BRACEKEYWORD,
  "ref" : BRACEKEYWORD,
  "xml" : BRACEKEYWORD,  // well, not really, but....
  "code" : BRACEKEYWORD,
  "expr" : BRACEKEYWORD,
  "image" : BRACEKEYWORD,

  "deprecated" : SINGLEKEYWORD,

  "example" : DELIMITERKEYWORD,
  "note" : DELIMITERKEYWORD,
  "bugs" : DELIMITERKEYWORD,
  "returns" : DELIMITERKEYWORD,
  "throws" : DELIMITERKEYWORD,
  "param" : DELIMITERKEYWORD,
  "seealso" : DELIMITERKEYWORD,
  "fixme" : DELIMITERKEYWORD,

  "section" : CONTAINERKEYWORD,

  "constant" : DELIMITERKEYWORD, // used inside @decl enum Foo

  "mapping" : CONTAINERKEYWORD, "member" : DELIMITERKEYWORD,
  "multiset" : CONTAINERKEYWORD, "index" : DELIMITERKEYWORD,
  "array" : CONTAINERKEYWORD, "elem" : DELIMITERKEYWORD,
  "int" : CONTAINERKEYWORD, "value" : DELIMITERKEYWORD,
  "string" : CONTAINERKEYWORD,
  "mixed" : CONTAINERKEYWORD, "type" : DELIMITERKEYWORD,

  "dl" : CONTAINERKEYWORD, "item" : DELIMITERKEYWORD,
  "ol" : CONTAINERKEYWORD,
  "ul" : CONTAINERKEYWORD,
]);

mapping(string : array(string)) attributenames =
([
  "item" : ({ "name" }),
  "param" : ({ "name" }),
  "mapping" : ({ "name" }),
  "array" : ({ "name" }),
  "multiset" : ({ "name" }),
  "int" : ({ "name" }),
  "string" : ({ "name" }),
  "mixed" : ({ "name" }),
  "constant" : ({ "name" }),
  "typedef" : ({ "name" }),
  "enum" : ({ "name" }),
]);

mapping(string:array(string)) required_attributes =
([
  "param" : ({ "name" }),
]);  

static constant standard = (<
  "note", "bugs", "example", "seealso", "deprecated", "fixme"
>);

mapping(string : multiset(string)) allowedChildren =
(["_general" : standard,
  "_method" : (< "param", "returns", "throws" >) + standard,
  "_variable": standard,
  "_inherit" : standard,
  "_class" : standard,
  "_module" : standard,
  "_constant" : standard,
  "_enum" : (< "constant" >) + standard,
  "_typedef" : standard,
  "mapping" : (< "member" >),
  "multiset": (< "index" >),
  "array"   : (< "elem" >),
  "int"     : (< "value" >),
  "string"  : (< "value" >),
  "mixed"   : (< "type" >),
  "dl"      : (< "item" >),
  "ol"      : (< "item" >),
  "ul"      : (< "item" >),
]);

mapping(string : multiset(string)) allowGrouping =
(["param" : (< "param" >),
  "index" : (< "index" >),
  "member" : (< "member" >),
  "type" : (< "type" >),
  "value" : (< "value" >),
  "constant" : (< "constant" >),
  "item" : (< "item" >),
]);

multiset(string) allowOnlyOne =
(< "seealso",
   "returns",
   "throws",
   "deprecated",
>);

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

static private class Token (
  int type,
  string keyword,
  string arg,   // the rest of the line, after the keyword
  string text,
  SourcePosition position,
) {
  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d, %O, %O, %O, %O)", this_program, type,
			     keyword, arg, text, position);
  }
}

static array(Token) split(string s, SourcePosition pos) {
  s = s - "\r";
  s = replace(s, "@\n", "@\r");
  array(string) lines = s / "\n";

  // array holding the number of src lines per line
  array(int) counts = allocate(sizeof(lines));
  for(int i = 0; i < sizeof(lines); ++i) {
    array(string) a = lines[i] / "@\r";
    counts[i] = sizeof(a);
    lines[i] = a * "";
  }
  array(Token) res = ({ });

  string filename = pos->filename;
  string text = 0;
  int textFirstLine;
  int textLastLine;

  int curLine = pos->firstline;

  for(int i = 0; i < sizeof(lines); ++i) {
    string line = lines[i];
    array(string) extr = extractKeyword(line);
    if (extr) {
      if (text)
        res += ({
          Token(TEXTTOKEN, 0, 0, text,
                SourcePosition(filename, textFirstLine, textLastLine))
        });
      text = 0;
      res += ({
        Token(getKeywordType(extr[0]), extr[0], extr[1], 0,
              SourcePosition(filename, curLine, curLine + counts[i] - 1))
      });
    }
    else {
      if (allSpaces(line))
        line = "";
      if (!text) {
        text = "";
        textFirstLine = curLine;
      }
      text += line + "\n";
      textLastLine = curLine;
    }
    curLine += counts[i];
  }
  if (text)
    res += ({ Token(TEXTTOKEN, 0, 0, text,
                    SourcePosition(filename, textFirstLine, textLastLine))
           });
  return res;
}

static class DocParserClass {

  SourcePosition currentPosition = 0;

  static void parseError(string s, mixed ... args) {
    s = sprintf(s, @args);
    werror("DocParser error: %O\n", s);
    throw (AutoDocError(currentPosition, "DocParser", s));
  }

  static array(Token) tokenArr = 0;
  static Token peekToken() {
    Token t = tokenArr[0];
    currentPosition = t->position || currentPosition;
    return t;
  }
  static Token readToken() {
    Token t = peekToken();
    tokenArr = tokenArr[1..];
    return t;
  }

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
    "section" : sectionArgHandler,
    "type" : typeArgHandler,
    "value" : valueArgHandler,
  ]);

  static string memberArgHandler(string keyword, string arg) {
    //  werror("This is the @member arg handler ");
    .PikeParser parser = .PikeParser(arg, currentPosition);
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
    .PikeParser parser = .PikeParser(arg, currentPosition);
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
    string type = xmltag("type", t->xml());
    if (s)
      if (s2)
        return type + xmltag("minindex", xmlquote(s))
          + xmltag("maxindex", xmlquote(s2));
      else
        return type + xmltag(dots ? "minindex" : "index", xmlquote(s));
    else
      if (s2)
        return type + xmltag("maxindex", xmlquote(s2));
      else
        parseError("@elem: expected identifier or literal");
  }

  static string indexArgHandler(string keyword, string arg) {
    //  werror("indexArgHandler\n");
    .PikeParser parser = .PikeParser(arg, currentPosition);
    string s = parser->parseLiteral();
    if (!s)
      parseError("@index: expected identifier, got %O", arg);
    parser->eat(EOF);
    return xmltag("value", xmlquote(s));
  }

  static string deprArgHandler(string keyword, string arg) {
    .PikeParser parser = .PikeParser(arg, currentPosition);
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

  static mapping(string : string) sectionArgHandler(string keyword, string arg) {
    return ([ "name" : arg ]);
  }

  static string typeArgHandler(string keyword, string arg) {
    //  werror("This is the @type arg handler ");
    .PikeParser parser = .PikeParser(arg, currentPosition);
    //  werror("&&& %O\n", arg);
    Type t = parser->parseOrType();
    if (!t)
      parseError("@member: expected type, got %O", arg);
    //  werror("%%%%%% got type == %O\n", t->xml());
    parser->eat(EOF);
    return t->xml();
  }

  static string valueArgHandler(string keyword, string arg) {
    //  werror("This is the @value arg handler ");
    .PikeParser parser = .PikeParser(arg, currentPosition);
    //  werror("&&& %O\n", arg);
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
        return xmltag("minvalue", xmlquote(s))
          + xmltag("maxvalue", xmlquote(s2));
      else
        return dots ? xmltag("minvalue", xmlquote(s)) : xmlquote(s);
    else
      if (s2)
        return xmltag("maxvalue", xmlquote(s2));
      else
        parseError("@value: expected indentifier or literal constant, got %O", arg);
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
      parseError(sprintf("@%s with too many parameters", keyword));
    for (int i = 0; i < sizeof(args); ++i)
      res[attrnames[i]] =  attributequote(args[i]);
    foreach(required_attributes[keyword]||({}), string attrname) {
      if (!res[attrname]) {
	parseError(sprintf("@%s lacking required parameter %s",
			   keyword, attrname));
      }
    }
    return res;
  }

  static string|mapping(string:string) getArgs(string keyword, string arg) {
    return (argHandlers[keyword] || standardArgHandler)(keyword, arg);
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
            parseError("expected @keyword{, got %O", s[start .. i]);
          if (getKeywordType(keyword) != BRACEKEYWORD)
            parseError("@%s cannot be used like this: @%s{ ... @}",
                      keyword, keyword);
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
    res = String.trim_all_whites(res-"<p></p>");
    if(!sizeof(res)) return "\n";
    return "<p>" + res + "</p>\n";
  }

  // Read until the next delimiter token on the same level, or to
  // the end.
  static string xmlText() {
    string res = "";
    for (;;) {
      Token token = peekToken();
      switch (token->type) {
        case TEXTTOKEN:
          res += xmlNode(token->text);
          readToken();
          break;
        case CONTAINERKEYWORD:
          string keyword = token->keyword;
          string arg = token->arg;

          res += "<" + keyword;
          string|mapping(string:string) args = getArgs(keyword, arg);
          if (mappingp(args))
            foreach(indices(args), string s)
              res += " " + s + "=\"" + args[s] + "\"";
          res += ">";
          if (stringp(args))
            res += args;

          readToken();
          res += xmlContainerContents(keyword);
          if (!(peekToken()->type == ENDKEYWORD
                && peekToken()->keyword == "end" + keyword))
            parseError(sprintf("@%s without matching @end%s",
			       keyword, keyword));
          res += closetag(keyword);
          readToken();
          break;
        default:
          return res;
      }
    }
  }

  static string xmlContainerContents(string container) {
    string res = "";
    Token token = peekToken();
    switch(token->type) {
      case ENDTOKEN:
        return "";
      case TEXTTOKEN:
        // SHOULD WE KILL EMPTY TEXT LIKE THIS ??
        {
        string text = token->text;
        if (text - "\n" - "\t" - " " == "") {
          readToken();
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
        parseError("unknown keyword: @" + token->keyword);
    }
    for (;;) {
      token = peekToken();
      if (! (<SINGLEKEYWORD, DELIMITERKEYWORD>) [token->type] )
        return res;

      string single = 0;
      array(string) keywords = ({});
      res += opentag("group");
      while ( (<SINGLEKEYWORD, DELIMITERKEYWORD>) [token->type] ) {
        string keyword = token->keyword;
        single = single || (token->type == SINGLEKEYWORD && keyword);
        multiset(string) allow = allowedChildren[container];
        if (!allow || !allow[keyword]) {
          string e = sprintf("@%s is not allowed inside @%s",
                             keyword, container);
          if (allow)
            e += sprintf(" (allowed children are:%{ @%s%})", indices(allow));
          else
            e += " (no children are allowed)";
          parseError(e);
        }

        multiset(string) allowGroup = allowGrouping[keyword] || ([]);
        foreach (keywords, string k)
          if (!allowGroup[k])
            parseError("@" + keyword + " may not be grouped together with @" + k);
        keywords += ({ keyword });

        string arg = token->arg;
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

        readToken();
        token = peekToken();
      }
      switch(token->type) {
        case TEXTTOKEN:
          // SHOULD WE KILL EMPTY TEXT LIKE THIS ??
          {
          string text = token->text;
          if (text - "\n" - "\t" - " " == "") {
            readToken();
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

  static void create(string | array(Token) s,
                     SourcePosition|void position)
  {
    if (arrayp(s)) {
      tokenArr = s;
    }
    else {
      if (!position) throw(({ __FILE__,__LINE__,"position == 0"}));
      tokenArr = split(s, position);
    }
    tokenArr += ({ Token(ENDTOKEN, 0, 0, 0, 0) });
  }

  MetaData getMetaData() {
    MetaData meta = MetaData();
    string scopeModule = 0;
    while(peekToken()->type == METAKEYWORD) {
      Token token = readToken();
      string keyword = token->keyword;
      string arg = token->arg;
      string endkeyword = 0;
      switch (keyword) {
      case "namespace":
      case "class":
      case "module":
	{
          if (endkeyword)
            parseError("@%s must stand alone", endkeyword);
          if (meta->appears)
            parseError("@appears before @%s", keyword);
          if (meta->belongs)
            parseError("@belongs before @%s", keyword);
          if (meta->type)
            parseError("@%s can not be combined with @%s", keyword, meta->type);
          meta->type = keyword;
          .PikeParser nameparser = .PikeParser(arg, currentPosition);
          string s = nameparser->readToken();
          if (!isIdent(s)) {
	    if ((keyword == "namespace") && (s == "::")) {
	      s = "";
	    } else {
	      parseError("@%s: expected %s name, got %O", keyword, keyword, s);
	    }
	  } else if (nameparser->peekToken() == "::") {
	    nameparser->readToken();
            if (keyword != "namespace")
              parseError("@%s: '%s::' only allowed as @namespace name",
			 keyword, s);
	  }
          if (nameparser->peekToken() != EOF)
            parseError("@%s: expected %s name, got %O", keyword, keyword, arg);
          meta->name = s;
	}
	break;

      case "endnamespace":
      case "endclass":
      case "endmodule":
	{
          if (meta->type || meta->belongs || meta->appears || endkeyword)
            parseError("@%s must stand alone", keyword);
          meta->type = endkeyword = keyword;
          .PikeParser nameparser = .PikeParser(arg, currentPosition);
          while (nameparser->peekToken() != EOF)
            meta->name = (meta->name || "") + nameparser->readToken();
	}
	break;

      case "decl":
	{
          if (endkeyword)
            parseError("@%s must stand alone", endkeyword);
          int first = !meta->type;
          if (!meta->type)
            meta->type = "decl";
          else if (meta->type != "decl")
            parseError("@decl can not be combined with @%s", meta->type);
          if (meta->appears)
            parseError("@appears before @decl");
          if (meta->belongs)
            parseError("@belongs before @decl");
          meta->type = "decl";
          .PikeParser declparser = .PikeParser(arg, currentPosition);
          PikeObject p = declparser->parseDecl(
            ([ "allowArgListLiterals" : 1,
               "allowScopePrefix" : 1 ]) ); // constants/literals + scope::
          string s = declparser->peekToken();
          if (s != ";" && s != EOF)
            parseError("@decl: expected end of line, got %O", s);
          int i = search(p->name, "::");
          if (i >= 0) {
            string scope = p->name[0 .. i + 1];
            p->name = p->name[i + 2 ..];
            if (!first && scopeModule != scope)
              parseError("@decl's must have identical 'scope::' prefix");
            scopeModule = scope;
          }
          else if (!first && scopeModule)
            parseError("@decl's must have identical 'scope::' prefix");
          meta->decls += ({ p });
	}
	break;

      case "appears":
	{
          if (endkeyword)
            parseError("@%s must stand alone", endkeyword);
          if (meta->type == "class" || meta->type == "decl"
              || meta->type == "module" || !meta->type)
          {
            if (meta->appears)
              parseError("duplicate @appears");
            if (meta->belongs)
              parseError("both @appears and @belongs");
            if (scopeModule)
              parseError("both 'scope::' and @appears");
            .PikeParser idparser = .PikeParser(arg, currentPosition);
            string s = idparser->parseIdents();
            if (!s)
              parseError("@appears: expected identifier, got %O", arg);
            meta->appears = s;
          }
          else
            parseError("@appears not allowed here");
	}
	break;

      case "belongs":
	{
          if (endkeyword)
            parseError("@%s must stand alone", endkeyword);
          if (meta->type == "class" || meta->type == "decl"
              || meta->type == "module" || !meta->type)
          {
            if (meta->belongs)
              parseError("duplicate @belongs");
            if (meta->appears)
              parseError("both @appears and @belongs");
            if (scopeModule)
              parseError("both 'scope::' and @belongs");
            .PikeParser idparser = .PikeParser(arg, currentPosition);
            string s = idparser->parseIdents();
            if (!s && idparser->peekToken() != EOF)
              parseError("@belongs: expected identifier or blank, got %O", arg);
            meta->belongs = s || "";  // blank is allowed too, you know ..
          }
	}
	break;

      default:
	parseError("illegal keyword: @%s", keyword);
      }
    }
    if (scopeModule)
      meta->belongs = scopeModule;
    return meta;
  }

  string getDoc(string context) {
    string xml = xmlContainerContents(context);
    switch (peekToken()->type) {
      case ENDTOKEN:
        break;
      case ERRORKEYWORD:
        parseError("illegal keyword: @"+ peekToken()->keyword);
      default:
        parseError("expected end of doc comment");
    }
    return xml;
  }
}

// Each of the arrays in the returned array can be fed to
// Parse::create()
array(array(Token)) splitDocBlock(string block, SourcePosition position) {
  array(Token) tokens = split(block, position);
  array(Token) current = ({ });
  array(array(Token)) result = ({ });
  int prevMeta = 0;
  foreach (tokens, Token token) {
    int meta = token->type == METAKEYWORD;
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
  static MetaData mMetaData = 0;
  static string mDoc = 0;
  static string mContext = 0;
  void create(string | array(Token) s, SourcePosition|void sp) {
    ::create(s, sp);
    state = 0;
  }

  MetaData metadata() {
    if (state == 0) {
      ++state;
      mMetaData = ::getMetaData();
    }
    return mMetaData;
  }

  string doc(string context) {
    if (state == 1) {
      ++state;
      mContext == context;
      mDoc = ::getDoc(context);
    }
    else if (state == 0 || state > 1 && mContext != context)
      return 0;
    return mDoc;
  }
}
