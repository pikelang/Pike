import "../../";

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

  string _sprintf() {
    return "ParseNode" + print();
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

static multiset getDefaultFields() {
  return (< "anchor", "any", "body", "keywords", "title", "url",  >);
}

// Splits a string into its words...
// TODO: Fix a better phrase function....
static array(string) splitPhrase(string phrase) {
  return phrase / " " - ({ "" });
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

ParseNode optimize(ParseNode node, string|void parentOp) {
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

static private void v(ParseNode node, ParseNode parent) {
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

// Returns 0 if OK, a string with error message if error
string validate(ParseNode node) {
  if (!node)  // A null query is also valid.
    return 0;
  mixed err = catch (v(node, 0));
  if (err)
    if (stringp(err))
      return err;
    else
      throw (err);
  return 0;
}
