// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Lexer.pmod,v 1.7 2004/08/07 15:27:00 js Exp $

// Lexer for search queries

public enum Token {
  TOKEN_END = 0,

  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_COLON,

  TOKEN_EQUAL,
  TOKEN_LESSEQUAL,
  TOKEN_GREATEREQUAL,
  TOKEN_NOTEQUAL,  // <> or !=
  TOKEN_LESS,
  TOKEN_GREATER,

  TOKEN_UNKNOWN,

  TOKEN_LPAREN,
  TOKEN_RPAREN,
//  TOKEN_LBRACKET,
//  TOKEN_RBRACKET,

  TOKEN_TEXT,     // word or phrase

  TOKEN_AND,
  TOKEN_OR,
}

static mapping(string : Token) keywords = ([
  //  "not" : TOKEN_NOT,
  "and" : TOKEN_AND,
  "or" : TOKEN_OR,
]);

// These characters cannot be part of a word, even if they are preceded by
// word characters.
static multiset(int) specialChars = (<
  ':', '(', ')',
>);

int isWhiteSpace(int ch) { return ch == '\t' || ch == ' '; }

//!   Tokenizes a query into tokens for later use by a parser.
//! @param query
//!   The query to tokenize.
//! @returns
//!   An array containing the tokens:
//!     @tt{ ({ ({ TOKEN_WORD, "foo" }), ... }) @}
//!   Or, in case of an error, a string with the error message.
public string|array(array(Token|string)) tokenize(string query) {
  array(array(Token|string)) result = ({});
  int len = strlen(query);
  query += "\0";

  int pos = 0;

  for (;;) {
    string x = query[pos .. pos];
#define EMIT(tok) EMIT2(tok,x)
#define EMIT2(tok,str) result += ({ ({ tok, str, str }) })
    switch (x) {
      case "\0":
        EMIT(TOKEN_END);
        return result;
      case "\t":
      case " ":
        // whitespace ignored.
        if (sizeof(result))
          result[-1][2] += x;
        break;
      case "\"":
      case "\'":
        string s;
        int end = search(query, x, pos + 1);
        if (end < 0) {
          s = query[pos + 1 .. len - 1];
          pos = len - 1;
        }
        else {
          s = query[pos + 1 .. end - 1];
          pos = end;
        }
        EMIT2(TOKEN_TEXT, s);
        break;
      case "+": EMIT(TOKEN_PLUS);       break;
      case "-": EMIT(TOKEN_MINUS);      break;
      case "=": EMIT(TOKEN_EQUAL);      break;
      case "(": EMIT(TOKEN_LPAREN);     break;
      case ")": EMIT(TOKEN_RPAREN);     break;
      // case "[": EMIT(TOKEN_LBRACKET);   break;
      // case "]": EMIT(TOKEN_RBRACKET);   break;
      case ":": EMIT(TOKEN_COLON);      break;
      case "<":
	if (query[pos + 1] == '=') {
	  ++pos;
	  EMIT2(TOKEN_LESSEQUAL, "=>");
	}
	else if (query[pos + 1] == '>') {
	  ++pos;
	  EMIT2(TOKEN_NOTEQUAL, "<>");
	}
	else
	  EMIT(TOKEN_LESS);
	break;
      case ">":
	if (query[pos + 1] == '=') {
	  ++pos;
	  EMIT2(TOKEN_GREATEREQUAL, ">=");
	}
	else
	  EMIT(TOKEN_GREATER);
	break;
      case "!":
	if (query[pos + 1] == '=') {
	  ++pos;
	  EMIT2(TOKEN_NOTEQUAL, "!=");
	}
	else
	  EMIT(TOKEN_UNKNOWN);
	break;
      default:
        {
        int i = pos + 1;
        while (query[i] && !isWhiteSpace(query[i]) && !specialChars[query[i]])
          ++i;
        string word = query[pos .. i - 1];
        string lword = Unicode.normalize(lower_case(word), "KD");
        if (keywords[lword])
          EMIT2(keywords[lword], word);
        else
          EMIT2(TOKEN_TEXT, word);
        pos = i - 1;
        }
    }
    ++pos;
  }

#undef EMIT
#undef EMIT2

}
