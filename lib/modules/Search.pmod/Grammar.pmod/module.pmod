// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: module.pmod,v 1.12 2004/08/07 15:27:00 js Exp $

//! Abstract parse tree node.
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
    return indentArray(children->print(), indent);
  }

  string print() {
    string indent = " " * (strlen(op) + 2);
    return sprintf("(%s %s)", op, printChildren(indent));
  }

  static string _sprintf(int t) {
    return t=='O' && "ParseNode" + print();
  }
}

//! And node.
class AndNode {
  inherit ParseNode;
  string op = "and";
}

//! Or node.
class OrNode {
  inherit ParseNode;
  string op = "or";
}

//! Date node.
class DateNode {
  inherit ParseNode;
  string op = "date";
  array operator;
  string date;
  string print() { return sprintf("(%s %O %O)", op, date, operator); }
}

//! Text node.
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

//!
multiset(string) getDefaultFields() {
  return (< "anchor", "any", "body", "keywords", "title", "url",  >);
}

// AND merge: Can merge all nodes with - or + bef. each thing.
// OR merge: Can merge all nodes without any - or +.
static private array(TextNode) mergeTextNodes(array(TextNode) a, string op) {
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

//!
ParseNode optimize(ParseNode node, string|void parentOp) {
  if (!node)
    return 0;
  node->children = map(node->children, optimize, node->op) - ({0});
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
      if (sizeof(newChildren) == 1)
        return newChildren[0];
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
      if (sizeof(newChildren) == 1)
        return newChildren[0];
      break;
    case "date":
      if (!node->date || node->date == "")
        return 0;
      break;
    case "text":
      if( node->words==({}) && node->plusWords==({}) && node->minusWords==({})
	  && node->phrases==({}) && node->plusPhrases==({}) &&
	  node->minusPhrases==({}) )
	return 0;
  }
  if (newChildren)
    node->children = newChildren;
  return node;
}

static private void _validate(ParseNode node, ParseNode parent) {
  map(node->children, _validate, node);
  switch (node->op) {
    case "date":
      if (!parent || parent->op != "and")
	throw ("date must restrict query");
      break;
    case "and":
      break;
    case "text":
      if (sizeof(node->minusWords) || sizeof(node->minusPhrases)) {
        if (!sizeof(node->plusWords)
            && !sizeof(node->plusPhrases)
            && !sizeof(node->words)
            && !sizeof(node->phrases))
          throw ("negative query not allowed");
      }
  }
}

// Returns 0 if OK, a string with error message if error
string validate(ParseNode node) {
  if (!node)  // A null query is also valid.
    return 0;
  mixed err = catch (_validate(node, 0));
  if (err)
    if (stringp(err))
      return err;
    else
      throw (err);
  return 0;
}

//!
void remove_stop_words(ParseNode node, array(string) stop_words) {
  if (!node || !sizeof(stop_words))
    return;
  low_remove_stop_words(node, stop_words);
}

static void low_remove_stop_words(ParseNode node, array(string) stop_words) {
  switch (node->op) {
    case "or":
    case "and":
      foreach(node->children, ParseNode c)
	remove_stop_words(c, stop_words);
      break;
      
    case "text":
      node->plusWords  -= stop_words;
      node->minusWords -= stop_words;
      node->words      -= stop_words;
      break;
  }
}
