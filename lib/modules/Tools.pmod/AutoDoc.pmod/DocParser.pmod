#pike __REAL_VERSION__

//========================================================================
// PARSING OF DOCUMENTATION COMMENTS
//========================================================================

inherit .PikeObjects;

#include "./debug.h"

//! Metadata about a @[Documentation] object.
class MetaData {
  //! Type of documented entity.
  string type;

  //! If @[type] is one of @expr{"class"@}, @expr{"module"@},
  //! @expr{"endmodule"@}, or @expr{"endclass"@}.
  string name;

  //! Set of declarations.
  array(PikeObject) decls = ({});

  //! Relocation information.
  string belongs = 0;
  string appears = 0;

  //! Set of inherits.
  array(PikeObject) inherits = ({});
}

//! End of file marker.
constant EOF = .PikeParser.EOF;

//! Enum of documentation token types.
enum DocTokenType {
  METAKEYWORD = 1,	//! eg @expr{@@decl@}
  BRACEKEYWORD = 2,	//! eg @expr{@@i{@@}@}
  DELIMITERKEYWORD = 3,	//! eg @expr{@@param@}
  CONTAINERKEYWORD = 4,	//! eg @expr{@@mapping@}
  SINGLEKEYWORD = 5,	//! None existant.
  ENDKEYWORD = 6,	//! eg @expr{@@endmapping@}
  ERRORKEYWORD = 7,	//! eg @expr{@@invalid@}

  TEXTTOKEN = 8,	//! Documentation text.
  ENDTOKEN = 9,		//! End of documentation marker.
};

// The following is the "DTD" of
// the documentation markup language.

//! The @[DocTokenType]s for the documentation @@-keywords.
mapping(string : DocTokenType) keywordtype =
([
  "appears" : METAKEYWORD,
  "belongs" : METAKEYWORD,
  "class" : METAKEYWORD,
  "global" : METAKEYWORD,
  "endclass" : METAKEYWORD,
  "module" : METAKEYWORD,
  "endmodule" : METAKEYWORD,
  "namespace" : METAKEYWORD,
  "endnamespace" : METAKEYWORD,
  "decl" : METAKEYWORD,
  "directive" : METAKEYWORD,
  "inherit" : METAKEYWORD,
  "enum" : METAKEYWORD,
  "endenum" : METAKEYWORD,

  "b" : BRACEKEYWORD,
  "i" : BRACEKEYWORD,
  "u" : BRACEKEYWORD,
  "tt" : BRACEKEYWORD,
  "url" : BRACEKEYWORD,
  "pre" : BRACEKEYWORD,
  "sub" : BRACEKEYWORD,
  "sup" : BRACEKEYWORD,
  "ref" : BRACEKEYWORD,
  "rfc" : BRACEKEYWORD,
  "xml" : BRACEKEYWORD,  // well, not really, but....
  "expr" : BRACEKEYWORD,
  "image" : BRACEKEYWORD,

  "deprecated" : DELIMITERKEYWORD,
  "obsolete" : DELIMITERKEYWORD,

  "bugs" : DELIMITERKEYWORD,
  "copyright" : DELIMITERKEYWORD,
  "example" : DELIMITERKEYWORD,
  "fixme" : DELIMITERKEYWORD,
  "note" : DELIMITERKEYWORD,
  "param" : DELIMITERKEYWORD,
  "returns" : DELIMITERKEYWORD,
  "seealso" : DELIMITERKEYWORD,
  "thanks" : DELIMITERKEYWORD,
  "throws" : DELIMITERKEYWORD,

  "code" : CONTAINERKEYWORD,
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

protected constant standard = (<
  "note", "bugs", "example", "seealso", "deprecated", "fixme", "code",
  "copyright", "thanks", "obsolete",
>);

mapping(string : multiset(string)) allowedChildren =
(["_general" : standard,
  "_method" : (< "param", "returns", "throws" >) + standard,
  "_variable": standard,
  "_inherit" : standard,
  "_class" : (< "param" >) + standard,
  "_namespace" : standard,
  "_module" : standard,
  "_constant" : standard,
  "_enum" : (< "constant" >) + standard,
  "_typedef" : standard,
  "_directive" : standard,
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

protected int getKeywordType(string keyword) {
  if (keywordtype[keyword])
    return keywordtype[keyword];
  if (sizeof(keyword) > 3 && keyword[0..2] == "end")
    return ENDKEYWORD;
  return ERRORKEYWORD;
}

protected int getTokenType(array(string) | string token) {
  if (arrayp(token))
    return getKeywordType(token[0]);
  if (!token)
    return ENDTOKEN;
  return TEXTTOKEN;
}

protected int isSpaceChar(int char) {
  return (< '\t', '\n', ' ' >) [char];
}

protected int isKeywordChar(int char) {
  return char >= 'a' && char <= 'z';
}

protected array(string)|zero extractKeyword(string line) {
  line += "\0";
  int i = 0;
  while (i < sizeof(line) && isSpaceChar(line[i]))
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
  return ({ keyword, line[i .. sizeof(line) - 2] });  // skippa "\0" ...
}

protected int allSpaces(string s) {
  for (int i = sizeof(s) - 1; i >= 0; --i)
    if (s[i] != ' ' && s[i] != '\t')
      return 0;
  return 1;
}

protected private class Token (
  int type,
  string|zero keyword,
  string|zero arg,   // the rest of the line, after the keyword
  string|zero text,
  object(SourcePosition)|zero position,
) {
  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d, %O, %O, %O, %O)", this_program, type,
			     keyword, arg, text, position);
  }
}

protected array(Token) split(string s, SourcePosition pos) {
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
  string|zero text = 0;
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

//! Internal class for parsing documentation markup.
protected class DocParserClass {

  //!
  SourcePosition currentPosition = 0;

  .Flags flags = .FLAG_NORMAL;

  protected void parseError(string s, mixed ... args) {
    s = sprintf(s, @args);
    if (currentPosition->lastline) {
      werror("%s:%d..%d: DocParser error: %s\n",
	     currentPosition->filename, currentPosition->firstline,
	     currentPosition->lastline, s);
    } else {
      werror("%s:%d: DocParser error: %s\n",
	     currentPosition->filename, currentPosition->firstline, s);
    }
    if (flags & .FLAG_KEEP_GOING) return;
    throw (AutoDocError(currentPosition, "DocParser", s));
  }

  protected array(Token) tokenArr = 0;
  protected Token peekToken() {
    Token t = tokenArr[0];
    currentPosition = t->position || currentPosition;
    return t;
  }
  protected Token readToken() {
    Token t = peekToken();
    tokenArr = tokenArr[1..];
    return t;
  }

  //! Contains functions that handle keywords with non-standard arg list
  //! syntax. The function for each keyword can return a mapping or a string:
  //!
  //! @mixed
  //!   @type mapping(string:string)
  //!     If a @expr{mapping(string:string)@} is returned, it is interpreted
  //!     as the attributes to put in the tag.
  //!   @type string
  //!     If a string is returned, it is an XML fragment that gets inserted
  //!     inside the tag.
  //! @endmixed
  mapping(string : function(string, string : string)
          | function(string, string : mapping(string : string))) argHandlers =
  ([
    "member" : memberArgHandler,
    "elem" : elemArgHandler,
    "index" : indexArgHandler,
    "deprecated" : deprArgHandler,
    "obsolete" : deprArgHandler,
    "section" : sectionArgHandler,
    "type" : typeArgHandler,
    "value" : valueArgHandler,
    "item": itemArgHandler,
  ]);

  protected string memberArgHandler(string keyword, string arg) {
    //  werror("This is the @member arg handler ");
    .PikeParser parser = .PikeParser(arg, currentPosition);
    //  werror("&&& %O\n", arg);
    Type t = parser->parseOrType();
    if (!t) {
      parseError("@member: expected type, got %O", arg);
      t = MixedType();
    }
    //  werror("%%%%%% got type == %O\n", t->xml());
    string s = parser->parseLiteral() || parser->parseIdents();
    if (!s) {
      parseError("@member: expected type followed by identifier or literal constant, got %O", arg);
      s = "";
    }
    parser->eat(EOF);
    return xmltag("type", t->xml())
      + xmltag("index", xmlquote(s));
  }

  protected string elemArgHandler(string keyword, string arg) {
    //  werror("This is the @elem arg handler\n");
    .PikeParser parser = .PikeParser(arg, currentPosition);
    Type t = parser->parseOrType();
    if (!t) {
      parseError("@elem: expected type, got %O", arg);
      t = MixedType();
    }
    if (parser->peekToken() == "...") {
      t = VarargsType(t);
      parser->eat("...");
    }
    string s = parser->parseLiteral() || parser->parseIdents();
    string|zero s2 = 0;
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
    return type;
  }

  protected string indexArgHandler(string keyword, string arg) {
    //  werror("indexArgHandler\n");
    .PikeParser parser = .PikeParser(arg, currentPosition);
    string s = parser->parseLiteral();
    if (!s) {
      parseError("@index: expected identifier, got %O", arg);
      s = "";
    }
    parser->eat(EOF);
    return xmltag("value", xmlquote(s));
  }

  protected string deprArgHandler(string keyword, string arg)
  {
    if (keyword != "deprecated") {
      parseError("Illegal keyword: @%s, did you mean @deprecated?");
    }
    .PikeParser parser = .PikeParser(arg, currentPosition);
    if (parser->peekToken() == EOF)
      return "";
    string res = "";
    for (;;) {
      string s = parser->parseIdents();
      if (!s) {
        parseError("@deprecated: expected list identifier, got %O", arg);
	if (parser->peekToken() != ",") return res;
      } else
	res += xmltag("name", xmltag("ref", s));
      if (parser->peekToken() == EOF)
        return res;
      parser->eat(",");
    }
  }

  protected mapping(string : string) sectionArgHandler(string keyword, string arg) {
    return ([ "title" : String.trim (arg) ]);
  }

  protected string typeArgHandler(string keyword, string arg) {
    //  werror("This is the @type arg handler ");
    .PikeParser parser = .PikeParser(arg, currentPosition);
    //  werror("&&& %O\n", arg);
    Type t = parser->parseOrType();
    if (!t) {
      parseError("@member: expected type, got %O", arg);
      t = MixedType();
    }
    //  werror("%%%%%% got type == %O\n", t->xml());
    parser->eat(EOF);
    return t->xml();
  }

  protected string valueArgHandler(string keyword, string arg)
  {
    //  werror("This is the @value arg handler ");
    catch {
      // NB: Throws errors on some syntax errors.
      .PikeParser parser = .PikeParser(arg, currentPosition);

      //  werror("&&& %O\n", arg);
      string s = parser->parseLiteral() || parser->parseIdents();
      string|zero s2 = 0;
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
    };

    parseError("@value: expected identifier or literal constant, got %O", arg);
    return "";
  }

  protected string|mapping(string:string) itemArgHandler(string keyword, string arg)
  {
    arg = String.trim(arg);
    if (arg == "") return "";
    if (!has_value(arg, "@")) return ([ "name":arg ]);
    return xmlNode(arg);
  }

  protected mapping(string : string) standardArgHandler(string keyword, string arg)
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
	String.Buffer out = String.Buffer();
	int quote = 0;
        while (arg[i] && (!isSpaceChar(arg[i]) || quote) ) {
	  if(arg[i]=='@') {
	    switch(arg[i+1]) {
	    case '@':
	      out->putchar( '@' );
	      break;
	    case '{':
	      quote++;
	      break;
	    case '}':
	      quote--;
	      if(quote<0)
		parseError(sprintf("@%s with too many @}.\n", keyword));
	      break;
	    case 0:
	    default:
	      parseError("Illegal @ statement.\n");
	    }
	  }
	  else
	    out->putchar(arg[i]);
          ++i;
	}
        args += ({ (string)out });
      }
    }

    mapping(string:string) res = ([]);

    array(string) attrnames = attributenames[keyword];
    int attrcount = sizeof(attrnames || ({}) );
    if (attrcount < sizeof(args)) {
      parseError(sprintf("@%s with too many parameters", keyword));
      args = args[..attrcount-1];
    }
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

  protected string|mapping(string:string) getArgs(string keyword, string arg) {
    return (argHandlers[keyword] || standardArgHandler)(keyword, arg);
  }

  protected string xmlNode(string s) {  /* now, @xml works like @i & @tt */
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
            } else {
	      parseError("@[ without matching ]");
	      j++;
	    }
          }
          res += xmltag("ref", xmlquote(s[i .. j - 2]));
          i = j;
        }
        else if (s[i] == '}') {
          if (!sizeof(tagstack)) {
            werror("///\n%O\n///\n", s);
            parseError("closing @} without corresponding @keyword{");
          } else {
	    if (tagstack[0] == "xml")
	      --inXML;
	    else
	      res += closetag(tagstack[0]);
	    tagstack = tagstack[1..];
	  }
          ++i;
        }
        else if (isKeywordChar(s[i])) {
          int start = i;
          while (isKeywordChar(s[++i]))
            ;
          string keyword = s[start .. i - 1];
          if (s[i] != '{') {
            parseError("expected @keyword{, got %O", s[start .. i]);
	    i--;
	  }
          if (getKeywordType(keyword) != BRACEKEYWORD) {
            parseError("@%s cannot be used like this: @%s{ ... @}",
                      keyword, keyword);
	    if (keyword == "code") {
	      // Common mistake.
	      parseError("Converted @code{ to @expr{.");
	      keyword = "expr";
	    }
	  }
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
    while (sizeof(tagstack)) {
      parseError("@" + tagstack[0] + "{ without matching @}");
      if (tagstack[0] == "xml")
	--inXML;
      else
	res += closetag(tagstack[0]);
      tagstack = tagstack[1..];
    }
    res = String.trim(res-"<p></p>");
    if(!sizeof(res)) return "\n";
    return "<p>" + res + "</p>\n";
  }

  // Read until the next delimiter token on the same level, or to
  // the end.
  protected string xmlText() {
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
              res += " " + s +
		"=\"" + Parser.encode_html_entities(args[s]) + "\"";
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

  protected string xmlContainerContents(string container) {
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
	readToken();
	return xmlContainerContents(container);
    }
    for (;;) {
      token = peekToken();
      if (! (<SINGLEKEYWORD, DELIMITERKEYWORD>) [token->type] )
        return res;

      string|zero single = 0;
      array(string) keywords = ({});
      res += opentag("group");
    group:
      while ( (<SINGLEKEYWORD, DELIMITERKEYWORD>) [token->type] ) {
        string keyword = token->keyword;
        single = single || (token->type == SINGLEKEYWORD && keyword);
        multiset(string) allow = allowedChildren[container];
        if (!allow || !allow[keyword]) {
          string e = sprintf("@%s is not allowed inside @%s",
                             keyword, container);
          if (allow) {
            e += sprintf(" (allowed children are:%{ @%s%})", indices(allow));
          } else
            e += " (no children are allowed)";
          parseError(e);
	  if (allow && sizeof(allow) == 1) {
	    parseError("Rewriting @%s to @%s.", keyword, indices(allow)[0]);
	    token->keyword = indices(allow)[0];
	    continue;
	  }
	  readToken();
	  token = peekToken();
	  break;
        }

        multiset(string) allowGroup = allowGrouping[keyword] || ([]);
        foreach (keywords, string k)
          if (!allowGroup[k]) {
            parseError("@" + keyword + " may not be grouped together with @" + k);
	    break group;
	  }
        keywords += ({ keyword });

        string arg = token->arg;
        res += "<" + keyword;
        string|mapping(string:string) args = getArgs(keyword, arg);
        if (mappingp(args)) {
          foreach(indices(args), string s)
            res += " " + s +
	      "=\"" + Parser.encode_html_entities(args[s]) + "\"";
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
	  else
	    res += opentag("text") + xmlText() + closetag("text");
      }
      res += closetag("group");
    }
  }

  protected void create(string | array(Token) s,
			SourcePosition|void position,
			.Flags|void flags)
  {
    if (undefinedp(flags)) flags = .FLAG_NORMAL;

    this::flags = flags;

    if (arrayp(s)) {
      tokenArr = s;
    }
    else {
      if (!position) error("position == 0");
      tokenArr = split(s, position);
    }
    tokenArr += ({ Token(ENDTOKEN, 0, 0, 0, 0) });
  }

  //! @returns
  //!   Returns the @[MetaData] for the documentation string.
  MetaData getMetaData() {
    MetaData meta = MetaData();
    string|zero scopeModule = 0;
    while(peekToken()->type == METAKEYWORD) {
      Token token = readToken();
      string keyword = token->keyword;
      string arg = token->arg;
      string|zero endkeyword = 0;
      switch (keyword) {
      case "namespace":
      case "enum":
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
	    if (keyword == "namespace") {
	      if (s == "::") {
		s = "";
	      } else if (!isFloat(s)) {
		parseError("@%s: expected %s name, got %O",
			   keyword, keyword, s);
	      }
	    } else {
	      parseError("@%s: expected %s name, got %O", keyword, keyword, s);
	    }
	  }
	  if (nameparser->peekToken() == "::") {
	    nameparser->readToken();
            if (keyword != "namespace")
              parseError("@%s: '%s::' only allowed as @namespace name",
			 keyword, s);
	  }
          if (keyword == "class") {
            foreach(nameparser->parseGenericsDecl() || ({});
                    int i; array(string|Type) generic) {
              meta->decls += ({ DocGroup(({ Generic(i, @generic) })) });
            }
          }
          if (nameparser->peekToken() != EOF) {
            // werror("Tokens: %O\n", nameparser->tokens);
            parseError("@%s: expected %s name, got %O (next: %O)",
                       keyword, keyword, arg, nameparser->peekToken());
          }
          meta->name = s;
	}
	break;

      case "endnamespace":
      case "endenum":
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
          array(PikeObject)|PikeObject pp = declparser->parseDecl(
            ([ "allowArgListLiterals" : 1,
               "allowScopePrefix" : 1 ]) ); // constants/literals + scope::
          string s = declparser->peekToken();
          if (s != ";" && s != EOF)
            parseError("@decl: expected end of line, got %O", s);
          foreach(arrayp(pp)?pp:({pp}), PikeObject p) {
            int i = search(p->name||"", "::");
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
	}
	break;

      case "inherit":
	{
          if (endkeyword)
            parseError("@%s must stand alone", endkeyword);
	  string s = .PikeParser(arg, currentPosition)->parseIdents();
	  if (!s)
	    parseError("@inherits: expected identifier, got %O", arg);
	  Inherit i = .PikeObjects.Inherit();
	  i->name = s;
	  i->classname = s;
	  meta->inherits += ({ i });
	}
	break;

      case "directive":
	{
          if (endkeyword)
            parseError("@%s must stand alone", endkeyword);
          int first = !meta->type;
          if (!meta->type)
            meta->type = "decl";
          else if (meta->type != "decl")
            parseError("@directive can not be combined with @%s", meta->type);
          if (meta->appears)
            parseError("@appears before @directive");
          if (meta->belongs)
            parseError("@belongs before @directive");
          meta->type = "decl";
          string s = String.trim(arg);
          meta->decls += ({ .PikeObjects.CppDirective(s) });
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
	    else
	      meta->appears = s;
          }
          else
            parseError("@appears not allowed here");
	}
	break;

      case "global":
	{
	  parseError("The @global keyword is obsolete. "
		     "Use @belongs predef:: instead.");
	  if (meta->type == "class" || meta->type == "decl"
	      || meta->type == "module" || !meta->type)
	  {
	    if (meta->belongs)
	      parseError("duplicate @belongs/@global");
	    if (meta->appears)
	      parseError("both @appears and @belongs/@global");
	    if (scopeModule)
	      parseError("both 'scope::' and @belongs/@global");
	    meta->belongs = "predef::";
	  }
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

  //! @returns
  //!   Returns the documentation corresponding to the @[context]
  //!   as an XML string.
  //!
  //! @note
  //!   @[getMetaData()] must be called before this function.
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

//! Split a @[block] of documentation text into segments of @[Token]s
//! split on @[METAKEYWORD]s.
//!
//! @returns
//!   Each of the arrays in the returned array can be fed to
//!   @[Parse::create()]
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

//! Documentation markup parser.
//!
//! This is a class, because you need to examine the meta lines
//! @b{before@} you can determine which context to parse the
//! actual doc lines in.
class Parse {
  //!
  inherit DocParserClass;

  protected int state;
  protected MetaData mMetaData = 0;
  protected string mDoc = 0;
  protected string mContext = 0;

  //! Parse a documentation string @[s].
  protected void create(string | array(Token) s, SourcePosition|void sp,
			.Flags|void flags) {
    ::create(s, sp, flags);
    state = 0;
  }

  //! @returns
  //!   Returns the @[MetaData] for the documentation string.
  MetaData metadata() {
    if (state == 0) {
      ++state;
      mMetaData = ::getMetaData();
    }
    return mMetaData;
  }

  //! @returns
  //!   Returns the documentation corresponding to the @[context]
  //!   as an XML string.
  //!
  //! @note
  //!   @[metadata()] must be called before this function.
  string|zero doc(string context) {
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
