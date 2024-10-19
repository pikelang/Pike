#pike __REAL_VERSION__

//! Abstract parse tree node.
class ParseNode {
  string op = "<node>";
  array(ParseNode) children = ({});
  void addChild(ParseNode n) { if (n) children += ({ n }); }

  protected string indentArray(array(string) stuff, string indent) {
    return map(stuff,
               lambda(string s) {
                 return replace(s, "\n", "\n" + indent);
               }) * ("\n" + indent);
  }

  protected string printChildren(string indent) {
    return indentArray(children->print(), indent);
  }

  string print() {
    string indent = " " * (strlen(op) + 2);
    return sprintf("(%s %s)", op, printChildren(indent));
  }

  protected string _sprintf(int t) {
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
private array(TextNode) mergeTextNodes(array(TextNode) a, string op) {
  array(TextNode) result = ({});
  mapping(string:array(TextNode)) fields = ([]);
  foreach (a, TextNode t)
    fields[t->field] = (fields[t->field] || ({ })) + ({ t });

  // Only merge nodes in the same field
  foreach (indices(fields), string field) {
    array(TextNode) unMerged = ({});
    object(TextNode)|zero merged = 0;
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
object(ParseNode)|zero optimize(ParseNode node, string|void parentOp) {
  if (!node)
    return 0;
  node->children = map(node->children, optimize, node->op) - ({0});
  array(ParseNode)|zero newChildren = 0;

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
      else {
	//  If we have a negative word only we try to place it at the
	//  end of the children list. This will save us a fetch of all
	//  document IDs in the execution pass to compute the negation.
	int(0..1) is_minus_only(ParseNode n) {
	  return
	    n->op == "text" &&
	    !sizeof(n->plusWords) &&
	    !sizeof(n->plusPhrases) &&
	    (!sizeof(n->words) || has_value(n->words, "*"))&&
	    !sizeof(n->phrases) &&
	    (sizeof(n->minusWords) ||
	     sizeof(n->minusPhrases));
	};
	array(ParseNode) minus_only_items = filter(newChildren, is_minus_only);
	newChildren = (newChildren - minus_only_items) + minus_only_items;
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
      //  Fix "minus" queries without corresponding starting selection
      //  by adding "*" as the starting selection.
      if (sizeof(node->minusWords) || sizeof(node->minusPhrases)) {
        if (!sizeof(node->plusWords)
            && !sizeof(node->plusPhrases)
            && !sizeof(node->words)
            && !sizeof(node->phrases)) {
	  node->words = ({ "*" });
	}
      }
  }
  if (newChildren)
    node->children = newChildren;
  return node;
}

private void _validate(ParseNode node, object(ParseNode)|zero parent) {
  map(node->children, _validate, node);
  switch (node->op) {
    case "date":
      if (!parent || parent->op != "and")
	throw ("date must restrict query");
      break;
    case "and":
      break;
    case "text":
      //  NOTE: This should no longer be happening since we adjust these
      //  nodes during the optimization phase to be constructed as
      //  ("*" - phrase).
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
string|zero validate(ParseNode node) {
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

protected void low_remove_stop_words(ParseNode node, array(string) stop_words) {
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
