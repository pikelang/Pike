inherit Search.Grammar.Lexer;

#include "debug.h"

class ParseNode {
  string op = "<node>";
  array(ParseNode) children = ({});
  void addChild(ParseNode n) { if (n) children += ({ n }); }

  static string indentArray(array(string) stuff, string indent) {
    return map(stuff,
               lambda(string s) {
                 return replace(s, "\n", "\n" + indent);
               }) * ("\n" + indent);
  }

  static string printChildren(string indent) {
    return indentArray(map(children,
                           lambda(ParseNode n) { return n->print(); }
                          ),
                       indent);
  }

  string print() {
    string indent = " " * (strlen(op) + 2);
    return sprintf("(%s %s)", op, printChildren(indent));
  }
}

class AndNode {
  inherit ParseNode;
  string op = "and";
}

class OrNode {
  inherit ParseNode;
  string op = "or";
}

class DateNode {
  inherit ParseNode;
  string op = "date";
  string date;
  string print() { return sprintf("(%s %O)", op, date); }
}

class TextNode {
  inherit ParseNode;
  string op = "text";
  string field;
  array(string) words = ({});
  array(string) plusWords = ({});
  array(string) minusWords = ({});
  array(array(string)) phrases = ({});
  array(array(string)) plusPhrases = ({});
  array(array(string)) minusPhrases = ({});
  string print() {
    array(string) a = ({ "" });

    foreach (words, string w)      a += ({ w });
    foreach (plusWords, string w)  a += ({ "+" + w });
    foreach (minusWords, string w) a += ({ "-" + w });

    foreach (phrases, array(string) p)      a += ({ "\"" + p * " " + "\"" });
    foreach (plusPhrases, array(string) p)  a += ({ "+\"" + p * " " + "\"" });
    foreach (minusPhrases, array(string) p) a += ({ "-\"" + p * " " + "\"" });
    return sprintf("(%s %O %s)", op, field,
                   indentArray(a, " " * (strlen(op) + 2)));
  }
}

static int isFieldSpecWord(string word) {
  return (<
    "any",
    "date",
    "title",
    "description",
    "url",
    "keywords",
  >) [lower_case(word)];
}

// Splits a string into its words...
// TODO: Fix a better phrase function....
static array(string) splitPhrase(string phrase) {
  return phrase / " " - ({ "" });
}

// AND merge: Can merge all nodes with - or + bef. each thing.
// OR merge: Can merge all nodes without any - or +.
static array(TextNode) mergeTextNodes(array(TextNode) a, string op) {
  array(TextNode) result = ({});
  mapping(string:array(TextNode)) fields = ([]);
  foreach (a, TextNode t)
    fields[t->field] = (fields[t->field] || ({ })) + ({ t });

  // Only merge nodes in the same field
  foreach (indices(fields), string field) {
    array(TextNode) unMerged = ({});
    TextNode merged = 0;
    foreach (fields[field], TextNode t) {
      int canMerge = 0;
      if (op == "and")
        canMerge = (sizeof(t->words) == 0
                    && sizeof(t->phrases) == 0);
      else if (op == "or")
        canMerge = (sizeof(t->plusWords) == 0
                    && sizeof(t->plusPhrases) == 0
                    && sizeof(t->minusWords) == 0
                    && sizeof(t->minusPhrases) == 0);
      if (canMerge) {
        merged = merged || TextNode();
        merged->field = field;
        merged->words += t->words;
        merged->plusWords += t->plusWords;
        merged->minusWords += t->minusWords;
        merged->phrases += t->phrases;
        merged->plusPhrases += t->plusPhrases;
        merged->minusPhrases += t->minusPhrases;
      }
      else
        unMerged += ({ t });
    }
    result += unMerged;
    if (merged)
      result += ({ merged });
  }
  return result;
}

public ParseNode optimize(ParseNode node, string|void parentOp) {
  if (!node)
    return 0;
  node->children = filter(map(node->children, optimize, node->op),
                          lambda(ParseNode n) {
                            return n != 0;
                          });
  array(ParseNode) newChildren = 0;
  switch (node->op) {
    case "and":
      if (!sizeof(node->children))
        return 0;
      newChildren = ({});
      // Check if we can merge TextNodes with the same field
      {
        array(TextNode) toMerge = ({});
        foreach (node->children, ParseNode child) {
          if (child->op == "and")
            newChildren += child->children;
          else if (child->op == "text")
            toMerge += ({ child });
          else
            newChildren += ({ child });
        }
        newChildren += mergeTextNodes(toMerge, "and");
      }
      break;
    case "or":
      if (!sizeof(node->children))
        return 0;
      newChildren = ({});

      {
        array(TextNode) toMerge = ({});
        foreach (node->children, ParseNode child) {
          if (child->op == "or")
            newChildren += child->children;
          else if (child->op == "text")
            toMerge += ({ child });
          else
            newChildren += ({ child });
        }
        newChildren += mergeTextNodes(toMerge, "or");
      }
      break;
    case "date":
      if (!node->date || node->date == "")
        return 0;
      break;
  }
  if (newChildren)
    node->children = newChildren;
  return node;
}

static void v(ParseNode node, ParseNode parent) {
  map(node->children, v, node);
  switch (node->op) {
    case "date":
      if (!parent || parent->op != "and")
        throw ("date must restrict query");
      break;
    case "and":
      break;
    case "text":
      if (node->minusWords || node->minusPhrases) {
        if (!sizeof(node->plusWords)
            && !sizeof(node->plusPhrases)
            && !sizeof(node->words)
            && !sizeof(node->phrases))
          throw ("negative query not allowed");
      }
  }
}

public string validate(ParseNode node) {
  mixed err = catch (v(node, 0));
  if (err)
    if (stringp(err))
      return err;
    else
      throw (err);
  return 0;
}

static void lowlevel(string s, mixed ... args) {
  werror(s, @args);
  werror("\n");
}

public void execute(ParseNode q) {
  switch (q->op) {
    case "and":
      {
      int first = 1;
      foreach (q->children, ParseNode child)
        if (child->op != "date") {
          execute(child);
          if (!first)
            lowlevel("AND");
          else
            first = 0;
        }
      foreach (q->children, ParseNode child)
        if (child->op == "date")
          execute(child);
      }
      break;
    case "or":
      int first = 1;
      foreach (q->children, ParseNode child) {
        execute(child);
        if (!first)
          lowlevel("OR");
        else
          first = 0;
      }
      break;
    case "date":
      lowlevel("DATE_FILTER %O", q->date);
      break;
    case "text":
      {
      int hasPlus = sizeof(q->plusWords) || sizeof(q->plusPhrases);
      int hasOrdinary = sizeof(q->words) || sizeof(q->phrases);
      int hasMinus = sizeof(q->minusWords) || sizeof(q->minusPhrases);
      if (hasPlus) {
        int first = 1;
        if (sizeof(q->plusWords)) {
          lowlevel("QUERY_AND     field:%O %{ %O%}", q->field, q->plusWords);
          first = 0;
        }
        foreach (q->plusPhrases, array(string) ph) {
          lowlevel("QUERY_PHRASE  field:%O %{ %O%}", q->field, ph);
          if (first)
            first = 0;
          else
            lowlevel("AND");
        }
      }
      if (hasOrdinary) {
        int first = 1;
        if (sizeof(q->words)) {
          lowlevel("QUERY_OR      field:%O %{ %O%}", q->field, q->words);
          first = 0;
        }
        foreach (q->phrases, array(string) ph) {
          lowlevel("QUERY_PHRASE  field:%O %{ %O%}", q->field, ph);
          if (first)
            first = 0;
          else
            lowlevel("OR");
        }
      }

      if (hasPlus && hasOrdinary)
        lowlevel("UPRANK");          // the XXX operation :)

      if (hasMinus) {
        int first = 1;
        if (sizeof(q->minusWords)) {
          lowlevel("QUERY_OR      field:%O %{ %O%}", q->field, q->minusWords);
          first = 0;
        }
        foreach (q->minusPhrases, array(string) ph) {
          lowlevel("QUERY_PHRASE  field:%O %{ %O%}", q->field, ph);
          if (first)
            first = 0;
          else
            lowlevel("OR");
        }
        lowlevel("SUB"); // Only - words are not allowed
      }

      break;
      }
  } // switch (q->op)
}

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

public class Parser {

  static array(array(Token|string)) tokens;
  static array(string) fieldstack;
  mapping(string:string) options;

  static array(Token|string) peek(void|int lookahead) {
    if (lookahead >= sizeof(tokens))
      lookahead = sizeof(tokens) - 1;
    return tokens[lookahead];
  }

  static void advance() {
    if (sizeof(tokens) > 1)
      tokens = tokens[1 .. ];
  }

  static void create(mapping(string:string)|void opt) {
    options = opt || ([]);
  }

  ParseNode parse(string q) {
    fieldstack = ({ "any" });
    tokens = tokenize(q);
    return parseQuery();
  }

  static ParseNode parseQuery() {
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
    return parseExpr2();
  }

  static ParseNode parseExpr2() {

    // field ':' expr3
    if (peek()[0] == TOKEN_WORD
        && isFieldSpecWord(peek()[1])
        && peek(1)[0] == TOKEN_COLON)
    {
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
    if (peek()[0] == TOKEN_WORD
        && peek(1)[0] == TOKEN_COLON)
      return 0;
    ParseNode or = OrNode();
    for (;;) {
      ParseNode n = parseExpr4();
      or->addChild(n);
      if (peek()[0] == TOKEN_OR)
        if (peek(1)[0] == TOKEN_WORD
            && peek(2)[0] == TOKEN_COLON)
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
    ParseNode and = AndNode();
    for (;;) {
      ParseNode n = parseExpr5();
      and->addChild(n);
      // NOTE: No implicit and here!
      if (peek()[0] == TOKEN_AND
          && !(peek(1)[0] == TOKEN_WORD
               && peek(2)[0] == TOKEN_COLON    // it was a higher level AND
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
    ParseNode text = TextNode();
    text->field = fieldstack[0];
    for (;;) {
      parseExpr6(text);
      if ( (< TOKEN_END,
              TOKEN_RPAREN,
              TOKEN_AND,
              TOKEN_OR >) [ peek()[0] ]
           || (peek()[0] == TOKEN_WORD
               && peek(1)[0] == TOKEN_COLON)
           || (peek()[0] == TOKEN_LPAREN))
        break; // it was a higher level IMPLICIT AND
      if (peek()[0] == TOKEN_OR)
        if (peek(1)[0] == TOKEN_WORD
            && peek(2)[0] == TOKEN_COLON
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

    while (!(< TOKEN_PHRASE,
               TOKEN_WORD,
               TOKEN_END >) [ peek()[0] ])
      advance();   // ... ?????????  or something smarter ?????

    if (peek()[0] == TOKEN_PHRASE
        || peek()[0] == TOKEN_WORD) {
      string phrase = peek()[1];
      advance();
      array(string) words = splitPhrase(phrase);
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
    DateNode n = DateNode();
    n->date = "";
  loop:
    for (;;) {
      switch (peek()[0]) {
        case TOKEN_WORD:
          if (isFieldSpecWord(peek()[1])
              && peek(1)[0] == TOKEN_COLON)
            break loop;  // it's a field specifier
          break;
        case TOKEN_UNKNOWN:
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

}  // class Parser
