#pike __REAL_VERSION__

//! A very special purpose Pike parser that can parse some selected
//! elements of the Pike language...

#include "./debug.h"
#define TOKEN_DEBUG 0

protected inherit .PikeObjects;

protected .Flags flags = .FLAG_NORMAL;

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
   "inline", "optional", "variant",

   "__async__", "__generator__",
>);

protected multiset(string) scopeModules =
(< "predef", "top", "lfun", "efun" >);

//! Skip the next token if it is a member of the @[tokens] set.
//!
//! @note
//!   The newline token (@expr{"\n"@}) is not skipped implicitly
//!   by this function.
//!
//! @seealso
//!   @[readToken()], @[peekToken()], @[eat()], @[skipUntil()]
void skip(multiset(string)|string tokens) {
  if (multisetp(tokens))
    while (tokens[peekToken(WITH_NL)]) readToken(WITH_NL);
  else
    while (peekToken(WITH_NL) == tokens) readToken(WITH_NL);
}

//! Skip passed a matched pair of parenthesis, brackets or braces.
array(string) skipBlock()
{
  string left = peekToken();
  string right = matchTokens[left];
  if (!right)        // there was no block to skip !!
    return ({});
  int level = 1;
  array(string) res = ({});
  res += ({ readToken() });
  string s = peekToken();
  while (s != EOF) {
    if (s == left)
      ++level;
    else if (s == right && !--level) {
      res += ({ readToken() });
      return res;
    }
    res += ({ readToken() });
    s = peekToken();
  }
  parseError("expected " + quoteString(right));
  return ({});
}

//! Skip tokens until one of @[tokens] is the next to read.
//!
//! @note
//!   The newline token (@expr{"\n"@}) is not skipped implicitly
//!   by this function.
//!
//! @seealso
//!   @[skip()]
array(string) skipUntil(multiset(string)|string tokens) {
  string s = peekToken(WITH_NL);
  array(string) res = ({});
  if (stringp(tokens)) {
    while (s != tokens) {
      if (s == EOF)
        parseError("expected " + quoteString(tokens));
      if (matchTokens[s])
        res += skipBlock();
      else
        res += ({ readToken(WITH_NL) });
      s = peekToken(WITH_NL);
    }
  }
  else {
    while (!tokens[s]) {
      if (s == EOF)
        parseError("expected one of " +
                   Array.map(Array.uniq(indices(tokens)), quoteString) * ", ");
      if (matchTokens[s])
        res += skipBlock();
      else
        res += ({ readToken(WITH_NL) });
      s = peekToken(WITH_NL);
    }
  }
  return res;
}

//! Skip past any newlines.
void skipNewlines() {
  while (peekToken(WITH_NL) == "\n")
    readToken(WITH_NL);
}

//========================================================================
// PARSING OF PIKE SOURCE FILES
//========================================================================

//! The current position in the source.
SourcePosition currentPosition = 0;

class ParseError
{
  inherit Error.Generic;
  constant error_type = "pike_parse_error";
  constant is_pike_parse_error = 1;

  string filename;
  int pos;

  string parse_message;
  // The bare message, without filename and pos etc. Also without
  // trailing newline.

  protected void create (string filename, int pos, string message,
			 void|array bt)
  {
    this::filename = filename;
    this::pos = pos;
    parse_message = message;
    ::create (sprintf ("PikeParser: %s:%d: %s\n", filename, pos, message), bt);
  }
}

protected int parseError(string message, mixed ... args) {
  if (sizeof (args))
    message = sprintf(message, @args);
  // werror("parseError! \n");
  // werror("%s\n", describe_backtrace(backtrace()));
  throw (ParseError (filename, positions[tokenPtr], message,
		     backtrace()[..<1]));
}

private int tokenPtr = 0;
private array(string) tokens;
private array(int) positions;
private string filename;

//! Newline indicator flag value.
constant WITH_NL = 1;

//! Peek @[offset] tokens ahead, skipping newlines,
//! unless @[with_newlines] is set.
//!
//! @seealso
//!   @[peekToken()]
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
//!   @[readToken()], @[lookAhead()]
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

//! @returns
//!   Returns the number of documentation comments
//!   that have been returned by @[readToken()].
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

//! Consume one token, error if not (one of) the expected in @[token].
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
  string scope = "";
  if ((lookAhead(1) == "::") &&
      (scopeModules[lookAhead(0)] || isVersion(lookAhead(0)))) {
    scope = readToken();
  }
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

//! Parse a union type.
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

//! Parse a mapping type.
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

//! Parse an array type.
ArrayType parseArray() {
  eat("array");
  ArrayType a = ArrayType();
  if (peekToken() == "(") {
    readToken();
    Type t;
    if ((peekToken()[0] >= '0') && (peekToken()[0] <= '9')) {
      t = IntType();
      if ((lookAhead(1) == ":") &&
	  !has_suffix(peekToken(), "bit") &&
	  !has_suffix(peekToken(), "bits")) {
	// Integer literal followed by a ':'.
	t->min = t->max = eatLiteral();
      } else {
	lowParseRange(t);
      }
    } else if (peekToken() == "..") {
      t = IntType();
      lowParseRange(t);
    } else if (peekToken() != ":") {
      t = parseOrType();
    }
    if (peekToken() == ":") {
      a->length = t;
      eat(":");
      if (peekToken() != ")") {
	a->valuetype = parseOrType();
      }
    } else {
      a->valuetype = t;
    }
    eat(")");
  }
  return a;
}

//! Parse a multiset type.
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

//! Parse a function type.
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


StringType|IntType lowParseRange(StringType|IntType s)
{
  string tk;

  switch (peekToken()) {
  case "zero":
    eat("zero");
    s->min = s->max = "0";
    return s;
  case "..":
    break;
  default:
    s->min = eatLiteral();
    if(sscanf(s->min, "%sbit", s->min) ||
       (<"bit","bits">)[(tk = peekToken())] )
    {
      if (tk) eat(tk);

      s->max = (string)((1<<(int)s->min)-1);
      s->min = "0";
      return s;
    }
  }

  eat("..");

  if (!(<")", ",", ":">)[peekToken()])
    s->max = eatLiteral();

  return s;
}

StringType|IntType parseRange(StringType|IntType s)
{
  if (peekToken() == "(") {
    readToken();

    lowParseRange(s);

    eat(")");
  }
  return s;
}

//! Parse an integer type.
IntType parseInt() {
  eat("int");
  return parseRange(IntType());
}

//! Parse a string type.
StringType parseString() {
  eat("string");
  return parseRange(StringType());
}

//! Parse a '.'-separated identitifer string.
//!
//! @note
//!   Also parses stuff preceded by @expr{"scope::"@} or @expr{"::"@}
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

array(string|Type) parseGenericDecl()
{
  string name = peekToken();

  if (lower_case(name) == upper_case(name)) {
    parseError("Invalid generic declaration.");
  }

  Type t = parseOrType();
  string sym = peekToken();
  Type def = t;

  if (!t || (t && (< "=", ",", ">", ";", EOF >)[sym])) {
    t = def = MixedType();
  } else {
    name = eatIdentifier();
    sym = peekToken();
  }

  if (sym == "=") {
    eat("=");
    def = parseOrType();
  }

  if (peekToken() == ",") {
    eat(",");
  }

  return ({ name, t, def });
}

array(array(string|Type))|zero parseGenericsDecl()
{
  if ((peekToken() != "(") || (lookAhead(1) != "<")) return 0;

  eat("(");
  eat("<");

  array(array(string|Type)) generics = ({});
  while ((peekToken() != ">") && (peekToken() != EOF)) {
    generics += ({ parseGenericDecl() });
  }

  eat(">");
  eat(")");

  return generics;
}

array(Type)|zero parseOptionalBindings()
{
  if ((peekToken() != "(") || (lookAhead(1) != "<")) return 0;

  eat("(");
  eat("<");

  array(Type) res = ({});
  while (peekToken() != ">") {
    Type t = parseOrType();
    if (t) {
      res += ({ t });
    }
    if ((peekToken() == ",") || !t) {
      eat(",");
    }
  }

  eat(">");
  eat(")");

  return res;
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
      obj->bindings = parseOptionalBindings();
      eat(")");
    }
  }
  else
    obj->classname = parseProgramName();
  obj->bindings = obj->bindings || parseOptionalBindings();
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
    eat(s);
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

AttributeType parseExperimental()
{
  eat("__experimental__");
  AttributeType t = AttributeType();
  t->attribute = "\"experimental\"";
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

object(Type)|zero parseType() {
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
    case "__unknown__":
      eat("__unknown__");
      return UnknownType();

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
    case "__experimental__":
      return parseExperimental();
    case ".":
      return parseObject();
    default:
      if (isIdent(s) || (lookAhead(1) == "::" && isVersion(s)))
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
  for (;;) {
    if (peekToken() == ")")
      return ({ argnames, argtypes });
    Type typ = parseOrType();
    if (typ && typ->name == "void" && peekToken() == ")")
      return ({ argnames, argtypes });
    string|zero literal = 0;
    string|zero identifier = 0;
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

      if (peekToken() == "=") {
	// FIXME: Consider documenting the default value in the
	//        trivial case (ie literal constant).
	eat("=");
	skipUntil((< ",", ")", ";", EOF >));

	// Adjust the type to be '|void'.
	if (argtypes[-1]->types) {
	  // Or-type.
	  argtypes[-1]->types += ({ VoidType() });
	} else {
	  typ = OrType();
	  typ->types = ({ argtypes[-1], VoidType() });
	  argtypes[-1] = typ;
	}
      }
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

//! Parse a single annotation from the token stream.
object(Annotation)|zero parseAnnotation()
{
  if (peekToken() != "@") return 0;
  eat("@");
  return Annotation(skipUntil((< ":", ";", EOF >)));
}

//! Parse a set of annotations from the token stream.
array(Annotation) parseAnnotations()
{
  array(Annotation) annotations = ({});
  Annotation a;
  while (a = parseAnnotation()) {
    eat(":");
  }
  return annotations;
}

//! Parse a set of modifiers from the token stream.
array(string) parseModifiers() {
  string s = peekToken();
  array(string) mods = ({ });
  while (modifiers[s]) {
    // Canonicalize some aliases.
    s = ([ "nomask":"final",
	   "static":"protected",
	   "inline":"local",
    ])[s] || s;
    if (!has_value(mods, s)) {
      mods += ({ s });
    }
    readToken();
    s = peekToken();
  }
  if (sizeof(mods) > 1) {
    // Clean up implied modifiers.
    if (has_value(mods, "private")) {
      // private implies local & protected.
      mods -= ({ "local", "protected", });
    }
    if (has_value(mods, "final")) {
      // final implies local.
      mods -= ({ "local" });
    }
    if (has_value(mods, "extern")) {
      // extern implies optional.
      mods -= ({ "optional" });
    }
  }
  return mods;
}

//! Parse the next literal constant (if any) from the token stream.
string|zero parseLiteral() {
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

object(Type)|zero literalType (string literal)
//! Returns the type of a literal. Currently only recognizes the top
//! level type. Currently does not thoroughly check that the literal
//! is syntactically valid.
{
  if (sizeof (literal))
    switch (literal[0]) {
      case '\'': return IntType(); // Character constant.
      case '"': return StringType();
      case '(':
	if (sizeof (literal) > 1)
	  switch (literal[1]) {
	    case '{': return ArrayType();
	    case '[': return MappingType();
	    case '<': return MultisetType();
	  }
	break;
      default:
	if (sscanf (literal, "%*D%*c") == 1)
	  return IntType();
	if (sscanf (literal, "%*f%*c") == 1)
	  return FloatType();
    }
  // Unrecognized format. Add an option to trig a parse error instead?
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
PikeObject|array(PikeObject)|Annotation parseDecl(mapping|void args) {
  args = args || ([]);
  skip("\n");
  peekToken();
  SourcePosition position = currentPosition;
  Annotation a = parseAnnotation();
  array(Annotation)|zero annotations = UNDEFINED;
  if (a) {
    switch(peekToken()) {
    case ";":
      eat(";");
      // FALL_THROUGH
    case EOF:
      return a;
    default:
      eat(":");
      annotations = ({ a }) + parseAnnotations();
      break;
    }
  }
  array(string) modifiers = parseModifiers();
  string s = peekToken(WITH_NL);
  while (s == "\n") {
    readToken(WITH_NL);
    s = peekToken(WITH_NL);
  }

  if (s == "class") {
    Class c = Class();
    c->position = position;
    c->annotations = annotations;
    c->modifiers = modifiers;
    readToken();
    c->name = eatIdentifier();
    return c;
  }
  else if (s == "{") {
    Modifier m = Modifier();
    m->position = position;
    m->annotations = annotations;
    m->modifiers = modifiers;
    return m;
  }
  else if (s == "constant") {
    Constant c = Constant();
    c->position = position;
    c->annotations = annotations;
    c->modifiers = modifiers;
    readToken();
    int save_pos = tokenPtr;
    mixed err = catch (c->type = parseOrType());
    if (err && (!objectp (err) || !err->is_pike_parse_error))
      throw (err);
    if (err || (<"=", ";", EOF>)[peekToken()]) {
      c->type = 0;
      tokenPtr = save_pos;
    }
    c->name = eatIdentifier();
    if (peekToken() == "=") {
      eat("=");
    }
    // FIXME: Warn if literal and no '='.
    if (string l = parseLiteral()) {
      // It's intentional that literalType doesn't return too
      // specific types for integers, i.e. it's int instead of e.g.
      // int(4711..4711).
      c->type = literalType (l);
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
  else if ((s == "inherit") || (s == "import")) {
    object(Inherit)|Import i;
    i = (s == "inherit")?Inherit():Import();
    i->position = position;
    i->annotations = annotations;
    i->modifiers = modifiers;
    readToken();
    i->name = i->classname = parseProgramName();
    if ((s == "inherit") && (peekToken() == ":")) {
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
    i->bindings = parseOptionalBindings();
    return i;
  }
  else if (s == "__generic__") {
    eat("__generic__");
    array(Generic) generics = ({});

    while ((peekToken() != ";") && (peekToken() != EOF)) {
      generics += ({ Generic(sizeof(generics), @parseGenericDecl()) });
    }

    return generics;
  }
  else if (s == "typedef") {
    Typedef t = Typedef();
    t->position = position;
    t->annotations = annotations;
    t->modifiers = modifiers;
    readToken();
    t->type = parseOrType();
    t->name = eatIdentifier();
    return t;
  }
  else if (s == "enum") {
    Enum e = Enum();
    e->position = position;
    e->annotations = annotations;
    e->modifiers = modifiers;
    readToken();
    if (peekToken() != "{")
      e->name = eatIdentifier();
    else
      e->name = "";
    return e;
  }
  else if ((s == "static_assert") || (s == "_Static_assert")) {
    // Static assertion. Skip.
    skipBlock();
    eat(";");
    return parseDecl(args);
  }
  else {
    Type t = parseOrType();

    if (peekToken() == "(") {
      // Probably a static assertion, or similar macro expansion.
      // Skip.
      skipBlock();
      skip(";");
      return parseDecl(args);
    }

    // only allow lfun::, predef::, :: in front of methods/variables
    string name = eatIdentifier(args["allowScopePrefix"]);

    if (peekToken() == "(") { // It's a method def
      Method m = Method();
      m->annotations = annotations;
      m->modifiers = modifiers;
      m->name = name;
      m->position = position;
      m->returntype = t;
      eat("(");
      [m->argnames, m->argtypes] = parseArgList(args["allowArgListLiterals"]);
      eat(")");
      return m;
    } else {
      array vars = ({ });
      for (;;) {
        Variable v = Variable();
	v->annotations = annotations;
        v->modifiers = modifiers;
        v->name = name;
	v->position = position;
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
    if(strlen(s) && s[-1] != '\n') s+="\n";
  array(string) a = Parser.Pike.split(s) + ({ EOF });

  array(string) t = ({ });
  array(int) p = ({ });

  for(int i = 0; i < sizeof(a); ++i) {
    string s = a[i];
    int pos = line;

    line += String.count(s, "\n");

    // remove preprocessor directives:
    if (sizeof(s) > 1 && s[0..0] == "#") {
      // But convert #pike directives to corresponding imports,
      // so that the resolver can find the correct symbols later.
      if (has_prefix(s, "#pike ") || has_prefix(s, "#pike\t")) {
        string version = String.trim(s[sizeof("#pike ")..]);
	if (version == "__REAL_VERSION__") {
	  version = "predef";
	}
	// NB: Surround the comment with whitespace, to keep
	//     it from being associated with surrounding code.
	t += ({ "\n", "//! @decl import " + version + "::\n", "\n" });
	p += ({ pos + 2, pos, pos + 2 });
      }
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
		      int|void line, .Flags|void flags)
{
  if (undefinedp(flags)) flags = .FLAG_NORMAL;
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
