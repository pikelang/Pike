
static inherit Search.Grammar.AbstractParser;
static inherit Search.Grammar.Lexer;
static private inherit "./module.pmod";
//static constant ParseNode = Search.Grammar.ParseNode;
//static constant OrNode    = Search.Grammar.OrNode;
//static constant AndNode   = Search.Grammar.AndNode;
//static constant TextNode  = Search.Grammar.TextNode;

#include "debug.h"

// =========================================================================
//   GRAMMAR FOR IMPLICIT <anything>
// =========================================================================
//
// START           : query
//                 ;
// 
// query           : query 'or' expr0
//                 | expr0
//                 ;
// 
// expr0           : expr0 expr1                 // imlicit AND
//                 | expr0 'and' expr1           // (on this level)
//                 | expr1
//                 ;
//
// expr1           : expr2
//                 ;
// 
// expr2           : expr3
//                 | field ':' expr3
//                 | 'date' ':' date
//                 | '(' query ')'
//                 ;
// 
// date            : (word <not followed by ':'>
//                    | '-' | ':' | <unknown character> )*
//                 ;
//
// NOTE: when looking for an operator here (expr3 - expr5), we have to check
// that it is not followed by a "field:", or "(".
// 
// expr3           : expr3 'or' expr4
//                 | expr4
//                 | <empty>
//                 ;
//
// expr4           : expr4 'and' expr5
//                 | expr5
//                 ;
// 
// expr5           : expr5 expr6
//                 | expr6
//                 ;
//
// expr6           : '-' expr7
//                 | '+' expr7
//                 | expr7
//                 ;
// 
// expr7           : word
//                 | phrase
//                 ;

static array(array(Token|string)) tokens;
static array(string) fieldstack;
mapping(string:mixed) options;

static array(Token|string) peek(void|int lookahead) {
  if (lookahead >= sizeof(tokens))
    lookahead = sizeof(tokens) - 1;
  return tokens[lookahead];
}

static void advance() {
  if (sizeof(tokens) > 1)
    tokens = tokens[1 .. ];
}

static int lookingAtFieldStart(void|int offset) {
  multiset(string) fields = options["fields"];
  //  SHOW(tokens);
  return peek(offset)[0] == TOKEN_TEXT
    && fields[ lower_case(peek(offset)[1]) ]
    && peek(offset + 1)[0] == TOKEN_COLON;
}

static void create(mapping(string:mixed)|void opt) {
  options = opt || ([ "implicit" : "or" ]);
  if (!options["fields"])
    options["fields"] = getDefaultFields();
}

ParseNode parse(string q) {
  fieldstack = ({ "any" });
  tokens = tokenize(q);
  return parseQuery();
}

static ParseNode parseQuery() {
  //  TRACE;
  ParseNode or = OrNode();
  for (;;) {
    ParseNode n = parseExpr0();
    or->addChild(n);
    if (peek()[0] == TOKEN_OR)
      advance();
    else
      break;
  }
  if (sizeof(or->children) == 1)
    return or->children[0];
  return or;
}

static ParseNode parseExpr0() {
  //  TRACE;
  ParseNode and = AndNode();
  for (;;) {
    ParseNode n = parseExpr1();
    and->addChild(n);
    if (peek()[0] == TOKEN_AND)
      advance();
    else if ((< TOKEN_END,
                TOKEN_RPAREN,
                TOKEN_OR >)[ peek()[0] ])
      break;
    // implicit AND
  }
  if (sizeof(and->children) == 1)
    return and->children[0];
  return and;
}

static ParseNode parseExpr1() {
  //  TRACE;
  return parseExpr2();
}

static ParseNode parseExpr2() {
  //  TRACE;

  // field ':' expr3
  if (lookingAtFieldStart())
  {
    //    TRACE;
    fieldstack = ({ peek()[1] }) + fieldstack;
    advance();
    advance();
    ParseNode n = fieldstack[0] == "date"
      ? parseDate()
      : parseExpr3();
    fieldstack = fieldstack[1 .. ];
    return n;
  }

  // '(' query ')'
  if (peek()[0] == TOKEN_LPAREN) {
    advance();
    ParseNode n = parseQuery();
    if (peek()[0] == TOKEN_RPAREN)
      advance();
    return n;
  }
  return parseExpr3();
}

static ParseNode parseExpr3() {
  //  TRACE;
  if (lookingAtFieldStart())
    return 0;
  ParseNode or = OrNode();
  for (;;) {
    ParseNode n = parseExpr4();
    or->addChild(n);
    if (peek()[0] == TOKEN_OR)
      if (lookingAtFieldStart(1))
        break; // it was a higher level OR
      else
        advance();
    else
      break;
  }
  if (sizeof(or->children) == 1)
    return or->children[0];
  return or;
}

static ParseNode parseExpr4() {
  //  TRACE;
  ParseNode and = AndNode();
  for (;;) {
    ParseNode n = parseExpr5();
    and->addChild(n);
    // NOTE: No implicit and here!
    if (peek()[0] == TOKEN_AND
        && !(lookingAtFieldStart(1)            // it was a higher level AND
             || peek(1)[0] == TOKEN_LPAREN))
      advance();
    else
      break;
  }
  if (sizeof(and->children) == 1)
    return and->children[0];
  return and;
}

static ParseNode parseExpr5() {
  //  TRACE;
  ParseNode text = TextNode();
  text->field = fieldstack[0];
  for (;;) {
    parseExpr6(text);
    if ( (< TOKEN_END,
            TOKEN_RPAREN,
            TOKEN_AND,
            TOKEN_OR >) [ peek()[0] ]
         || lookingAtFieldStart()
         || (peek()[0] == TOKEN_LPAREN))
      break; // it was a higher level IMPLICIT AND
    if (peek()[0] == TOKEN_OR)
      if (lookingAtFieldStart(1)
          || peek(1)[0] == TOKEN_LPAREN)
        break;   // it was a higher level OR
      else
        advance();
  }
  if (sizeof(text->words)
      || sizeof(text->phrases)
      || sizeof(text->plusWords)
      || sizeof(text->plusPhrases)
      || sizeof(text->minusWords)
      || sizeof(text->minusPhrases))
    return text;
  return 0;
}

static void parseExpr6(TextNode node) {
  //  TRACE;
  int prefix = 0;

  if (peek()[0] == TOKEN_MINUS) {
    advance();
    prefix = '-';
  }
  else if (peek()[0] == TOKEN_PLUS) {
    advance();
    prefix = '+';
  }

  if (!prefix && options["implicit"] == "and")
    prefix = '+';

  while (!(< TOKEN_TEXT, TOKEN_END >) [ peek()[0] ])
    advance();   // ... ?????????  or something smarter ?????

  if (peek()[0] == TOKEN_TEXT) {
    string text = peek()[1];
    advance();
    array(string) words = Unicode.split_words_and_normalize(text);
    if (!words || !sizeof(words))
      return;
    if (sizeof(words) == 1)
      switch (prefix) {
        case '+': node->plusWords += words;  break;
        case '-': node->minusWords += words; break;
        default:  node->words += words;      break;
      }
    else if (sizeof(words) > 1)
      switch (prefix) {
        case '+': node->plusPhrases += ({ words });  break;
        case '-': node->minusPhrases += ({ words }); break;
        default:  node->phrases += ({ words });      break;
      }
  }
}

static ParseNode parseDate() {
  //  TRACE;
  DateNode n = DateNode();
  n->date = "";
loop:
  for (;;) {
    switch (peek()[0]) {
      case TOKEN_TEXT:
        if (lookingAtFieldStart())
          break loop;  // it's a field specifier
        break;
      case TOKEN_MINUS:
      case TOKEN_COLON:
        break;
      default:
        break loop;
    }
    n->date += peek()[2];  // with spaces preserved!
    advance();
  }
  return n;
}

