// A very special purpose Pike parser that can parse some selected
// elements of the Pike language...

#define DEB werror("###PikeParser.Pike: %d\n", __LINE__);

static inherit .PikeObjects;
static inherit "module.pmod";

constant EOF = "";

static mapping(string : string) quote =
  (["\n" : "\\n", "\\" : "\\\\" ]);
static string quoteString(string s) {
  return "\"" + (quote[s] || s) + "\"";
}

static int parseError(string message, mixed ... args) {
  message = sprintf(message, @args);
  werror("parseError! \n");
  // werror(sprintf("%s\n", describe_backtrace(backtrace())));
  message = "PikeParser error: " + message;
  throw ( ({ message, currentline }) );
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

//========================================================================
// PARSING OF PIKE SOURCE FILES
//========================================================================

int currentline = 0;
string currentfile = "";

static private int tokenPtr = 0;
static private array(string) tokens;
constant WITH_NL = 1;

string peekToken(int | void with_newlines) {
  if (tokenPtr >= sizeof(tokens))
    return EOF;
  string s;
  if (with_newlines)
    return tokens[tokenPtr];
  else {
    int i = tokenPtr;
    while (tokens[i] == "\n")
      ++i;
    s = tokens[i];
  }
  //  werror("    peek: %O  %s\n", s, with_newlines ? "(WNL)" : "");
  return s;
}

static int nReadDocComments = 0;
int getReadDocComments() { return nReadDocComments; }
string readToken(int | void with_newlines) {
  if (tokenPtr >= sizeof(tokens))
    return EOF;
  string s;
  if (with_newlines) {
    if ( (s = tokens[tokenPtr++]) == "\n")
      ++currentline;
  }
  else {
    while (tokens[tokenPtr] == "\n") {
      ++tokenPtr;
      ++currentline;
    }
    s = tokens[tokenPtr++];
  }
  if (isDocComment(s))
    ++nReadDocComments;
  //  werror("    read: %O  %s\n", s, with_newlines ? "(WNL)" : "");
  return s;
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

// Also ::ident, predef::ident, lfun::ident
string eatIdentifier(void|int allowScopePrefix) {
  string scope = peekToken() == "predef" || peekToken() == "lfun"
    ? readToken() : "";
  string colons = peekToken() == "::" ? readToken() : "";
  //  werror("scope == %O ,colons == %O\n", scope, colons);

  if (scope != "" && colons == "")
    parseError("%s must be followed by ::", scope);
  if (strlen(scope + colons) && !allowScopePrefix)
    parseError("scope prefix not allowed");
  string s = peekToken();
  if (!isIdent(s))
    parseError("expected identifier, got %O", s);
  readToken();
  return scope + colons + s;
}

Type|void parseOrType() {
  array(Type) a = ({ parseType() });
  while (peekToken() == "|") {
    readToken();
    a += ({ parseType() });
  }
  if (sizeof(a) == 1)
    return a[0];
  OrType or = OrType();
  or->types = a;
  return or;
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
    readToken();
    f->argtypes = ({ parseOrType() });
    while (peekToken() == ",") {
      readToken();
      f->argtypes += ({ parseOrType() });
    }
    if (peekToken() == ":") {
      readToken();
      f->returntype = parseOrType();
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

// also parses stuff preceded by "predef::" or "::"
string|void parseIdents() {
  string result = "";
  if (peekToken() == "." || peekToken() == "::")
    result = readToken();
  else if (peekToken() == "predef") {
    result = readToken();
    if (peekToken() != "::")
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
  int firstline = currentline;
  array(string) modifiers = parseModifiers();
  string s = peekToken(WITH_NL);
  while (s == "\n") {
    readToken(WITH_NL);
    s = peekToken(WITH_NL);
  }
  if (s == "class") {
    Class c = Class();
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
  else if (s == "constant") {
    Constant c = Constant();
    c->modifiers = modifiers;
    readToken();
    c->name = eatIdentifier();
    eat("=");
    // TODO: parse the expression ???
    //   added parsing only of types...
    //   a constant value will just be ignored.
    c->typedefType = parseOrType();

    skipUntil( (< ";", EOF >) );
    return c;
  }
  else if (s == "inherit") {
    Inherit i = Inherit();
    i->modifiers = modifiers;
    readToken();
    i->classname = parseProgramName();
    if (peekToken() == ":") {
      readToken();
      i->name = eatIdentifier();
    }
    return i;
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
    // remove preprocessor directives:
    if (strlen(s) > 1 && s[0..0] == "#")
      continue;
    // remove non-doc comments
    if (strlen(s) >= 2 &&
        (s[0..1] == "/*" || s[0..1] == "//") &&
       !isDocComment(s))
      continue;
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

static private array(string) tokenize(string s) {
  return special(Parser.Pike.split(s)) + ({ EOF });
}

// create(string, filename)

static void create(string s, void|string filename, void|int line) {
  currentfile = filename || "";
  currentline = line ? line : 1;
  tokens = tokenize(s);
  tokenPtr = 0;
}
