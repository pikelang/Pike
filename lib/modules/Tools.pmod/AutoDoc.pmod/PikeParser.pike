#pike __REAL_VERSION__

// A very special purpose Pike parser that can parse some selected
// elements of the Pike language...

#include "./debug.h"
#define TOKEN_DEBUG 0

static inherit .PikeObjects;
static inherit "module.pmod";

constant EOF = "";

static mapping(string : string) quote =
  (["\n" : "\\n", "\\" : "\\\\" ]);
static string quoteString(string s) {
  return "\"" + (quote[s] || s) + "\"";
}

static mapping(string : string) matchTokens =
([ "(":")", "{":"}", "[":"]",
   "({" : "})", "([" : "])", "(<":">)" ]);

static mapping(string : string) reverseMatchTokens =
  mkmapping(values(matchTokens), indices(matchTokens));

static multiset(string) modifiers =
(< "nomask", "final", "static", "extern",
   "private", "local", "public", "protected",
   "inline", "optional", "variant"  >);

static multiset(string) scopeModules =
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

SourcePosition currentPosition = 0;

static int parseError(string message, mixed ... args) {
  message = sprintf(message, @args);
  // werror("parseError! \n");
  // werror("%s\n", describe_backtrace(backtrace()));
  error("PikeParser: %s (%d)\n", message, positions[tokenPtr]);
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

static int nReadDocComments = 0;
int getReadDocComments() { return nReadDocComments; }

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

// consume one token, error if not (one of) the expected
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

// Also ::ident, scope::ident
string eatIdentifier(void|int allowScopePrefix) {
  //  SHOW(({lookAhead(0),lookAhead(1),lookAhead(2)}));
  string scope = scopeModules[lookAhead(0)] && lookAhead(1) == "::"
    ? readToken()
    : "";
  string colons = peekToken() == "::" ? readToken() : "";
  //  werror("scope == %O ,colons == %O\n", scope, colons);

  if (strlen(scope + colons) && !allowScopePrefix)
    parseError("scope prefix not allowed");
  string s = peekToken();
  if (!isIdent(s))
    parseError("expected identifier, got %O", s);
  readToken();
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

// also parses stuff preceded by "scope::" or "::"
string|void parseIdents() {
  string result = "";
  if (peekToken() == "." || peekToken() == "::")
    result = readToken();
  else if (isIdent(peekToken())) {
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
  if (strlen(s) && s[0] == '"')
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

Type parseType() {
  string s = peekToken();
  switch(s) {
    case "float":
      eat("float");
      return FloatType();
    case "mixed":
      eat("mixed");
      return MixedType();
    case "program":
      eat("program");
      return ProgramType();
    case "string":
      eat("string");
      return StringType();
    case "void":
      eat("void");
      return VoidType();

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
    case ".":
      return parseObject();
    default:
      if (isIdent(s))
        return parseObject();
      else
        return 0;
  }
}

// If allowLiterals != 0 then you can write a literal or Pike idents
// as an argument, like:
//    void convert("jpeg", Image image, float quality)
//
// For a literal arg, the argname[] string contains the literal and
// the corresponding argtypes element is 0
// NOTE: expects that the arglist is followed by a ")".
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
    if (!typ)
      literal = parseLiteral();
    else if (peekToken() == "...") {
      typ = VarargsType(typ);
      eat("...");
    }
    string identifier = 0;
    if (isIdent(peekToken()))
      identifier = readToken();

    if (typ && identifier) {
      argnames += ({ identifier });
      argtypes += ({ typ });
    }
    else {
      if (typ) {
        // it's an identifier 'Foo.Bar.gazonk' designating a constant that
        // has been mistaken for an object type ...
        if (typ->name != "object")
          parseError("Expected type, idents or literal constant");
        literal = typ->classname;
      }
      if (literal && !identifier) {
        argnames += ({ literal });
        argtypes += ({ 0 });
      }
      else
        parseError("Expected type, idents or literal constant");
    }
    if (peekToken() == ")")
      return ({ argnames, argtypes });
    eat(",");
  }
}

array parseCreateArgList() {
  array(string) argnames = ({});
  array(Type) argtypes = ({});
  array(array(string)) argmodifiers = ({});
  for (;;) {
    array(string) m = parseModifiers();
    Type t = parseOrType();
    if (!t)
      return ({ argnames, argtypes, argmodifiers });
    if (peekToken() == "...") {
      t = VarargsType(t);
      eat("...");
    }
    argnames += ({ eatIdentifier() });
    argtypes += ({ t });
    argmodifiers += ({ m });
    if (peekToken() != ",")
      return ({ argnames, argtypes, argmodifiers });
    eat(",");
  }
}

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

string eatLiteral() {
  return parseLiteral() || parseError("expected literal");
}

// parseDecl() reads ONLY THE HEAD, NOT the ";" or "{" .. "}" !!!

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
    skipUntil(";");
    return i;
  }
  else if (s == "class") {
    Class c = Class();
    c->position = position;
    c->modifiers = modifiers;
    readToken();
    c->name = eatIdentifier();
    if (peekToken() == "(") {
      eat("(");
      [c->createArgNames, c->createArgTypes, c->createArgModifiers] =
        parseCreateArgList();
      eat(")");
    }
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
      c->typedefType = parseOrType();
    }
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

// special() removes all whitespaces except for \n, which it
// puts in separate "\n" strings in the array
// It also removes all comments that are not doc comments,
// and removes all preprocessor stuff.
// FIXME: how do we handle multiline strings: #" ... " ???
static private array(string) special(array(string) in) {
  array(string) ret = ({ });
  foreach (in, string s) {
    // remove blanks that are not "\n"
    // separate multiple adjacent "\n"
    int c = String.count(s, "\n");
    if (c)
      ret += ({ "\n" }) * c;
    else if (strlen(replace(s, ({" ", "\t", "\r" }), ({ "","","" }) )))
      ret += ({ s });
  }
  return ret;
}

array(array(string)|array(int)) tokenize(string s, string filename, int line) {
  array(string) a = special(Parser.Pike.split(s)) + ({ EOF });

  array(string) t = ({ });
  array(int) p = ({ } );

  for(int i = 0; i < sizeof(a); ++i) {
    string s = a[i];
    int pos = line;

    line += sizeof(s / "\n") - 1;

    // remove preprocessor directives:
    if (strlen(s) > 1 && s[0..0] == "#")
      continue;
    // remove non-doc comments
    if (strlen(s) >= 2 &&
        (s[0..1] == "/*" || s[0..1] == "//") &&
        !isDocComment(s))
      continue;

    t += ({ s });
    p += ({ pos });
  }
  return ({ t, p });
}

void setTokens(array(string) t, array(int) p) {
  tokens = t;
  positions = p;
  tokenPtr = 0;
#if TOKEN_DEBUG
  werror("PikeParser::setTokens(), tokens = \n%O\n", tokens);
#endif
}

// create(string, filename, firstline)
// create(array(Token))
static void create(string|void s,
                   string|SourcePosition|void filename,
                   int|void line)
{
  if (s) {
    if (objectp(filename)) {
      line = filename->firstline;
      filename = filename->filename;
    }
    if (!line)
      error("PikeParser::create() called without line arg.\n");

    [tokens, positions] = tokenize(s, filename, line);
  }
  else
    tokens = ({});
#if TOKEN_DEBUG
  werror("PikeParser::create(), tokens = \n%O\n", tokens);
#endif
  tokenPtr = 0;
}
