#pike __REAL_VERSION__

//! A very special purpose Pike parser that can parse some selected
//! elements of the Pike language...

#include "./debug.h"
#define TOKEN_DEBUG 0

protected inherit .PikeObjects;
protected inherit "module.pmod";

//! The end of file marker.
constant EOF = "";

protected mapping(string : string) quote =
  (["\n" : "\\n", "\\" : "\\\\" ]);
protected string quoteString(string s) {
  return "\"" + (quote[s] || s) + "\"";
}

protected mapping(string : string) matchTokens =
([ "(":")", "{":"}", "[":"]",
   "({" : "})", "([" : "])", "(<":">)" ]);

protected mapping(string : string) reverseMatchTokens =
  mkmapping(values(matchTokens), indices(matchTokens));

protected multiset(string) modifiers =
(< "nomask", "final", "static", "extern",
   "private", "local", "public", "protected",
   "inline", "optional", "variant"  >);

protected multiset(string) scopeModules =
(< "predef", "top", "lfun", "efun" >);

void skip(multiset(string)|string tokens) {
  if (multisetp(tokens))
    while (tokens[peekToken(WITH_NL)]) readToken(WITH_NL);
  else
    while (peekToken(WITH_NL) == tokens) readToken(WITH_NL);
}

void skipBlock() {
  string left = peekToken();
  string right = matchTokens[left];
  if (!right)        // there was no block to skip !!
    return;
  int level = 1;
  readToken();
  string s = peekToken();
  while (s != EOF) {
    if (s == left)
      ++level;
    else if (s == right && !--level) {
      readToken();
      return;
    }
    readToken();
    s = peekToken();
  }
  parseError("expected " + quoteString(right));
}

void skipUntil(multiset(string)|string tokens) {
  string s = peekToken(WITH_NL);
  if (stringp(tokens)) {
    while (s != tokens) {
      if (s == EOF)
        parseError("expected " + quoteString(tokens));
      if (matchTokens[s])
        skipBlock();
      else
        readToken(WITH_NL);
      s = peekToken(WITH_NL);
    }
  }
  else {
    while (!tokens[s]) {
      if (s == EOF)
        parseError("expected one of " +
                   Array.map(Array.uniq(indices(tokens)), quoteString) * ", ");
      if (matchTokens[s])
        skipBlock();
      else
        readToken(WITH_NL);
      s = peekToken(WITH_NL);
    }
  }
}

void skipNewlines() {
  while (peekToken(WITH_NL) == "\n")
    readToken(WITH_NL);
}

//========================================================================
// PARSING OF PIKE SOURCE FILES
//========================================================================

//! The current position in the source.
SourcePosition currentPosition = 0;

protected int parseError(string message, mixed ... args) {
  message = sprintf(message, @args);
  // werror("parseError! \n");
  // werror("%s\n", describe_backtrace(backtrace()));
  error("PikeParser: %s (%s:%d)\n", message, filename, positions[tokenPtr]);
}

private int tokenPtr = 0;
private array(string) tokens;
private array(int) positions;
private string filename;
constant WITH_NL = 1;

string lookAhead(int offset, int | void with_newlines) {
  if (with_newlines)
    return tokens[min(tokenPtr + offset, sizeof(tokens) - 1)];
  int i = tokenPtr;
  for (;;) {
    while (tokens[i] == "\n")
      ++i;
    if (offset <= 0)
      return tokens[i];
    --offset;
    ++i;
    if (i >= sizeof(tokens))
      return EOF;
  }
}

//! Peek at the next token in the stream without advancing.
//!
//! @param with_newlines
//!   If set will return @expr{"\n"@} tokens, these will
//!   otherwise silently be skipped.
//!
//! @returns
//!   Returns the next token.
//!
//! @seealso
//!   @[readToken()]
string peekToken(int | void with_newlines) {
  int at = tokenPtr;

  if (tokenPtr >= sizeof(tokens))
    return EOF;

  if (!with_newlines)
    while (tokens[at] == "\n")
      ++at;

  currentPosition = SourcePosition( filename, positions[at] );
  return tokens[at];
}

protected int nReadDocComments = 0;
int getReadDocComments() { return nReadDocComments; }

//! Read the next token from the stream and advance.
//!
//! @param with_newlines
//!   If set will return @expr{"\n"@} tokens, these will
//!   otherwise silently be skipped.
//!
//! @returns
//!   Returns the next token.
//!
//! @seealso
//!   @[peekToken()]
string readToken(int | void with_newlines) {
  if (tokenPtr >= sizeof(tokens))
    return EOF;

  int pos;
  if (with_newlines)
    pos = tokenPtr++;
  else {
    while (tokens[tokenPtr] == "\n")
      ++tokenPtr;
    pos = tokenPtr++;
  }
  if (isDocComment(tokens[pos]))
    ++nReadDocComments;

  currentPosition = SourcePosition( filename, positions[pos] );
  return tokens[pos];
}

//! Consume one token, error if not (one of) the expected
string eat(multiset(string) | string token) {
  string s = peekToken();
  if (multisetp(token)) {
    if (!token[s]) {
      parseError("expected one of " +
                 Array.map(Array.uniq(indices(token)), quoteString) * ", " +
                 " , got " + quoteString(s));
    }
  }
  else {
    if (s != token)
      parseError("expected " + quoteString(token) +
                 ", got " + quoteString(s));
  }
  return readToken();
}

//! Expect an identifier.
//!
//! @note
//!   Also @expr{::ident@}, @expr{scope::ident@}.
//!
//! @note
//!   This function also converts old-style getters and setters
//!   into new-style.
string eatIdentifier(void|int allowScopePrefix) {
  //  SHOW(({lookAhead(0),lookAhead(1),lookAhead(2)}));
  string scope = scopeModules[lookAhead(0)] && lookAhead(1) == "::"
    ? readToken()
    : "";
  string colons = peekToken() == "::" ? readToken() : "";
  //  werror("scope == %O ,colons == %O\n", scope, colons);

  if (sizeof(scope + colons) && !allowScopePrefix)
    parseError("scope prefix not allowed");
  string s = peekToken();
  if (!isIdent(s))
    parseError("expected identifier, got %O", s);
  readToken();

  // Special hax for `->symbol and `->symbol=, and `symbol and `symbol=
  if( (s=="`->" || s=="`") && isIdent(peekToken()) )
  {
    if (s == "`->") s = "`"; // Convert old to new syntax.
    s += readToken();
    if( peekToken()=="=" )
      s += readToken();
  }

  return scope + colons + s;
}

Type parseOrType() {
  array(Type) a = ({ parseType() });
  while (peekToken() == "|") {
    readToken();
    a += ({ parseType() });
  }
  Type base;
  if (sizeof(a) == 1) {
    base = a[0];
  } else {
    base = OrType();
    base->types = a;
  }
  int level = 0;
  while (peekToken() == "*") {
    readToken();
    level++;
  }
  if (!level) {
    return base;
  }
  while(level--) {
    Type a = ArrayType();
    a->valuetype = base;
    base = a;
  }
  return base;
}

MappingType parseMapping() {
  eat("mapping");
  MappingType m = MappingType();
  if (peekToken() == "(") {
    readToken();
    m->indextype = parseOrType();
    eat(":");
    m->valuetype = parseOrType();
    eat(")");
  }
  return m;
}

ArrayType parseArray() {
  eat("array");
  ArrayType a = ArrayType();
  if (peekToken() == "(") {
    readToken();
    a->valuetype = parseOrType();
    eat(")");
  }
  return a;
}

MultisetType parseMultiset() {
  eat("multiset");
  MultisetType m = MultisetType();
  if (peekToken() == "(") {
    readToken();
    m->indextype = parseOrType();
    eat(")");
  }
  return m;
}

FunctionType parseFunction() {
  eat("function");
  FunctionType f = FunctionType();
  if (peekToken() == "(") {
    f->argtypes = ({ });
    do {
      readToken();
      Type t = parseOrType();
      if (!t)
        if (peekToken() == ")" || peekToken() == ":")
          break;
        else
          parseError("expected type, got %O", peekToken());
      f->argtypes += ({ t });
    } while(peekToken() == ",");
    if (peekToken() == "...") {
      if (!sizeof(f->argtypes))
        parseError("expected type, got ...");
      readToken();
      f->argtypes[-1] = VarargsType(f->argtypes[-1]);
    }
    if (peekToken() == ":") {
      readToken();
      f->returntype = parseOrType() || parseError("expected return type");
    }
    eat(")");
  }
  return f;
}

IntType parseInt() {
  eat("int");
  IntType i = IntType();
  if (peekToken() == "(") {
    readToken();
    if (peekToken() != "..")
      i->min = eatLiteral();
    eat("..");
    if (peekToken() != ")")
      i->max = eatLiteral();
    eat(")");
  }
  return i;
}

StringType parseString() {
  eat("string");
  StringType s = StringType();
  if (peekToken() == "(") {
    readToken();
    if (peekToken() != "..")
      s->min = eatLiteral();
    eat("..");
    if (peekToken() != ")")
      s->max = eatLiteral();
    eat(")");
  }
  return s;
}

//! Also parses stuff preceded by @expr{"scope::"@} or @{"::"@}
string|void parseIdents() {
  string result = "";
  if (peekToken() == "." || peekToken() == "::")
    result = readToken();
  else if (isIdent(peekToken()) || isFloat(peekToken())) {
    result = readToken();
    if (peekToken() != "::" && peekToken() != ".") // might be "top.ident" ...
      return result;
    result += readToken();
  }
  for (;;) {
    string id = peekToken();
    if (!isIdent(id))
      return result == "" ? 0 : result;
    result += id;
    readToken();
    if (peekToken() != ".")
      break;
    readToken();
    result += ".";
  }
  return result == "" ? 0 : result;
}

string parseProgramName() {
  string s = peekToken();
  if (sizeof(s) && s[0] == '"')
    return (readToken(), s);
  s = parseIdents();
  if (!s)
    parseError("expected program name");
  return s;
}

ObjectType parseObject() {
  ObjectType obj = ObjectType();
  if (peekToken() == "object") {
    readToken();
    if (peekToken() == "(") {
      readToken();
      string s = peekToken();
      if (sizeof(s) >= 2 && s[0] == '"') {
        readToken();
        obj->classname = s;
      }
      else
        obj->classname = parseProgramName();
      eat(")");
    }
  }
  else
    obj->classname = parseProgramName();
  return obj;
}

ProgramType parseProgram() {
  ProgramType prg = ProgramType();
  eat("program");
  if( peekToken() == "(" ) {
    readToken();
    prg->classname = "";
    while( peekToken() != ")" )
      prg->classname += readToken();
    eat(")");
  }
  return prg;
}

TypeType parseTypeType()
{
  eat("type");
  TypeType t = TypeType();
  if (peekToken() == "(") {
    readToken();
    t->subtype = parseType();
    eat(")");
  }
  return t;
}

AttributeType parseAttributeType()
{
  eat("__attribute__");
  AttributeType t = AttributeType();
  eat("(");
  string s = peekToken();
  if (sizeof(s) >= 2 && s[0] == '"') {
    t->attribute = s;
  } else parseError("expected attribute name");
  if (peekToken() == ",") {
    readToken();
    if (peekToken() != ")") {
      t->subtype = parseType();
      eat(")");
      return t;
    }
  }
  eat(")");
  t->prefix = 1;
  t->subtype = parseType();
  return t;
}

AttributeType parseDeprecated()
{
  eat("__deprecated__");
  AttributeType t = AttributeType();
  t->attribute = "\"deprecated\"";
  if (peekToken() == "(") {
    readToken();
    if (peekToken() == ")") {
      readToken();
    } else {
      t->subtype = parseType();
      eat(")");
      return t;
    }
  }
  t->prefix = 1;
  t->subtype = parseType();
  return t;
}

Type parseType() {
  string s = peekToken();
  switch(s) {
    case "float":
      eat("float");
      return FloatType();
    case "mixed":
      eat("mixed");
      return MixedType();
    case "string":
      return parseString();
    case "void":
      eat("void");
      return VoidType();
    case "zero":
      eat("zero");
      return ZeroType();

    case "array":
      return parseArray();
    case "function":
      return parseFunction();
    case "int":
      return parseInt();
    case "mapping":
      return parseMapping();
    case "multiset":
      return parseMultiset();
    case "object":
      return parseObject();
    case "program":
      return parseProgram();
    case "type":
      return parseTypeType();
    case "__attribute__":
      return parseAttributeType();
    case "__deprecated__":
      return parseDeprecated();
    case ".":
      return parseObject();
    default:
      if (isIdent(s))
        return parseObject();
      else
        return 0;
  }
}

//! Parse the list of arguments in a function declaration.
//!
//! @param allowLiterals
//!   If allowLiterals != 0 then you can write a literal or Pike idents
//!   as an argument, like:
//!   @code
//!      void convert("jpeg", Image image, float quality)
//!   @endcode
//!
//!   For a literal arg, the argname[] string contains the literal and
//!   the corresponding argtypes element is 0
//!
//! @note
//!   Expects that the arglist is followed by a ")".
array parseArgList(int|void allowLiterals) {
  array(string) argnames = ({ });
  array(Type) argtypes = ({ });
  if (peekToken() == ")")
    return ({ argnames, argtypes });
  for (;;) {
    Type typ = parseOrType();
    if (typ && typ->name == "void" && peekToken() == ")")
      return ({ argnames, argtypes });
    string literal = 0;
    string identifier = 0;
    if (!typ)
      literal = parseLiteral();
    else {
      if (peekToken() == "...") {
	typ = VarargsType(typ);
	eat("...");
      }
      if (isIdent(peekToken()))
	identifier = readToken();
    }

    if (typ && (identifier || !allowLiterals || (typ->name != "object"))) {
      // Note: identifier may be zero for unnamed arguments in prototypes.
      argnames += ({ identifier });
      argtypes += ({ typ });
    }
    else {
      if (typ) {
	// it's an identifier 'Foo.Bar.gazonk' designating a constant that
	// has been mistaken for an object type ...
	literal = typ->classname;
      }
      argnames += ({ literal });
      argtypes += ({ 0 });
    }
    if (peekToken() == ")")
      return ({ argnames, argtypes });
    eat(",");
  }
}

//! Parse a set of modifiers from the token stream.
array(string) parseModifiers() {
  string s = peekToken();
  array(string) mods = ({ });
  while (modifiers[s]) {
    mods += ({ s });
    readToken();
    s = peekToken();
  }
  return mods;
}

//! Parse the next literal constant (if any) from the token stream.
void|string parseLiteral() {
  string sign = peekToken() == "-" ? readToken() : "";
  string s = peekToken();
  if (s && s != "" &&
      (s[0] == '"' || s[0] == '\'' || isDigit(s[0])))
  {
    readToken();
    return sign + s;
  }
  return 0;
}

//! Expect a literal constant.
//!
//! @seealso
//!   @[parseLiteral()]
string eatLiteral() {
  return parseLiteral() || parseError("expected literal");
}

//! Parse the next Pike declaration from the token stream.
//!
//! @note
//!   @[parseDecl()] reads ONLY THE HEAD, NOT the @expr{";"@}
//!   or @expr{"{" .. "}"@} !!!
PikeObject|array(PikeObject) parseDecl(mapping|void args) {
  args = args || ([]);
  skip("\n");
  peekToken();
  SourcePosition position = currentPosition;
  array(string) modifiers = parseModifiers();
  string s = peekToken(WITH_NL);
  while (s == "\n") {
    readToken(WITH_NL);
    s = peekToken(WITH_NL);
  }

  if (s == "import") {
    Import i = Import();
    i->position = position;
    readToken();
    i->classname = parseProgramName();
    return i;
  }
  else if (s == "class") {
    Class c = Class();
    c->position = position;
    c->modifiers = modifiers;
    readToken();
    c->name = eatIdentifier();
    return c;
  }
  else if (s == "{") {
    Modifier m = Modifier();
    m->position = position;
    m->modifiers = modifiers;
    return m;
  }
  else if (s == "constant") {
    Constant c = Constant();
    c->position = position;
    c->modifiers = modifiers;
    readToken();
    c->name = eatIdentifier();
    if (peekToken() == "=") {
      eat("=");
      // TODO: parse the expression ???
      //   added parsing only of types...
      //   a constant value will just be ignored.
      //c->typedefType = parseOrType();      // Disabled 2003-11-20 /grubba
    }
    // FIXME: What about multiple constants (constant_list)?
    //        Not that anyone uses the syntax, but...
    skipUntil( (< ";", EOF >) );
    return c;
  }
  else if (s == "inherit") {
    Inherit i = Inherit();
    i->position = position;
    i->modifiers = modifiers;
    readToken();
    i->classname = parseProgramName();
    if (peekToken() == ":") {
      readToken();
      i->name = eatIdentifier();
    } else {
      if (i->classname[0] != '"') {
	// Keep just the last part.
	i->name = (replace(i->classname,
			   ({ "::", "->", "()" }),
			   ({ ".", ".", "" }))/".")[-1];
      }
    }
    return i;
  }
  else if (s == "typedef") {
    Typedef t = Typedef();
    t->position = position;
    t->modifiers = modifiers;
    readToken();
    t->type = parseOrType();
    t->name = eatIdentifier();
    return t;
  }
  else if (s == "enum") {
    Enum e = Enum();
    e->position = position;
    e->modifiers = modifiers;
    readToken();
    if (peekToken() != "{")
      e->name = eatIdentifier();
    return e;
  }
  else {
    Type t = parseOrType();
    // only allow lfun::, predef::, :: in front of methods/variables
    string name = eatIdentifier(args["allowScopePrefix"]);
    if (peekToken() == "(") { // It's a method def
      Method m = Method();
      m->modifiers = modifiers;
      m->name = name;
      m->returntype = t;
      eat("(");
      [m->argnames, m->argtypes] = parseArgList(args["allowArgListLiterals"]);
      eat(")");
      return m;
    } else {
      array vars = ({ });
      for (;;) {
        Variable v = Variable();
        v->modifiers = modifiers;
        v->name = name;
        v->type = t;
        vars += ({ v });
        skipUntil( (< ";", ",", EOF >) );  // Skip possible init expr
        if (peekToken() != ",")
          break;
        eat(",");
        name = eatIdentifier();
      }
      if (sizeof(vars) == 1)
        return vars[0];
      return vars;
    }
  }
}

//! Tokenize a string of Pike code.
array(array(string)|array(int)) tokenize(string s, int line) {
  array(string) a = Parser.Pike.split(s) + ({ EOF });

  array(string) t = ({ });
  array(int) p = ({ });

  for(int i = 0; i < sizeof(a); ++i) {
    string s = a[i];
    int pos = line;

    line += String.count(s, "\n");

    // remove preprocessor directives:
    if (sizeof(s) > 1 && s[0..0] == "#") {
      t += ({ "\n" });
      p += ({ pos });
      continue;
    }
    // remove non-doc comments
    if (sizeof(s) >= 2 &&
        (s[0..1] == "/*" || s[0..1] == "//") &&
	(sizeof(s)==2 || s[2]!='!'))
      continue;
    if(sizeof(s) >= 2 && s[0..1]=="/*") {
      werror("Illegal comment %O in %O.\n", s, filename);
      continue;
    }
    if( sizeof(s) && (< ' ', '\t', '\r', '\n' >)[s[0]] ) {
      string clean = s-" "-"\t"-"\r";
      if(!sizeof(clean)) continue;
      if(clean=="\n"*sizeof(clean)) {
	int i = sizeof(clean);
	while(i--) {
	  t += ({ "\n" });
	  p += ({ pos++ });
	}
	continue;
      }
    }

    t += ({ s });
    p += ({ pos });
  }
  return ({ t, p });
}

//!
void setTokens(array(string) t, array(int) p) {
  tokens = t;
  positions = p;
  tokenPtr = 0;
#if TOKEN_DEBUG
  werror("PikeParser::setTokens(), tokens = \n%O\n", tokens);
#endif
}

//!
protected void create(string|void s,
		      string|SourcePosition|void _filename,
		      int|void line)
{
  if (s) {
    if (objectp(_filename)) {
      line = _filename->firstline;
      filename = _filename->filename;
    }
    if (!line)
      error("PikeParser::create() called without line arg.\n");

    [tokens, positions] = tokenize(s, line);
  }
  else {
    tokens = ({});
    filename = _filename;
  }
#if TOKEN_DEBUG
  werror("PikeParser::create(), tokens = \n%O\n", tokens);
#endif
  tokenPtr = 0;
}
