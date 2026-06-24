#pike __REAL_VERSION__

//! Parser and evaluator of the JSONPath syntax.
//!
//! @seealso
//!   @rfc{9535@}

local protected {
  class Expression
  {
    array apply(mixed json_value, mixed root_value)
    {
      return ({ json_value });
    }
  }

  typedef int|string|Expression Selector;

  class RootExpression
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      return ({ root_value });
    }

    protected string _sprintf(int|void c)
    {
      return "$";
    }
  }

  RootExpression Dollar = RootExpression();

  class WildExpression
  {
    inherit Expression;

    protected array json_values(mixed val)
    {
      if (arrayp(val) || mappingp(val)) {
        return values(val);
      }
      return ({});
    }

    array apply(mixed json_value, mixed root_value)
    {
      return map(json_values(json_value), json_values) * ({});
    }

    protected string _sprintf(int|void c)
    {
      return "*";
    }
  }

  WildExpression Wild = WildExpression();

  class DescendantExpression
  {
    inherit Expression;

    Query sub_expression;

    protected void create(ADT.Stack tokens)
    {
      // Prepend "@." to the token stream.
      tokens->push(".");
      tokens->push("@");
      sub_expression = Query(tokens, 1);
    }

    array apply(mixed json_value, mixed root_value)
    {
      array ret = ({});
      if (mappingp(json_value) || arrayp(json_value)) {
        array v = sub_expression->apply(json_value, root_value);
        ret += v;

        foreach(values(json_value), mixed j) {
          ret += apply(j, root_value);
        }
      }
      return ret;
    }

    protected string _sprintf(int|void c)
    {
      return sprintf(".%O", sub_expression);
    }
  }

  class Filter(Expression filter)
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      return sizeof(filter->apply(json_value, root_value)) ?
        ({ json_value }) : ({});
    }
  }

  // Index a JSON-value with a string or an int.
  protected array index_value(mixed val, string|int s)
  {
    if (mappingp(val)) {
      val = val[s];
      if (undefinedp(val)) return ({});
      return ({ val });
    }

    if (arrayp(val)) {
      if (intp(s)) {
        return val[s..s];
      }
      return map(val, index_value, s) * ({});
    }

    return ({});
  }

  //! Apply a compiled selector expression on a JSON-compatible value.
  //!
  //! @param selectors
  //!   Compiled selector expression.
  //!
  //! @param json_value
  //!   Value to operate on.
  //!
  //! @param root_value
  //!   Value that the @tt{'$'@}-operator should refer to.
  //!   Defaults to @[json_value]. Typically only relevant
  //!   when operating on sub-expressions.
  //!
  //! @returns
  //!   Returns an array with the matching selected values.
  //!   If no values matched, the empty array @expr{({})@}
  //!   will thus be returned.
  array apply_selector(array(Selector) selectors,
                       mixed json_value, mixed|void root_value)
  {
    if (undefinedp(root_value)) root_value = json_value;
    json_value = ({ json_value });

    foreach(selectors, Selector s) {
      array next_jvs = ({});
      foreach(json_value, mixed jv) {
        if (objectp(s)) {
          next_jvs += s->apply(jv, root_value);
        } else {
          if (intp(jv) || stringp(jv)) {
            // Not found.
            continue;
          }

          next_jvs += index_value(jv, s);
        }
      }

      json_value = next_jvs;
    }

    return json_value;
  }

  class MultiSelectExpression(array(Selector) selectors)
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      return map(selectors/1, apply_selector, json_value, root_value) * ({});
    }
  }
}

//! JSONPath compiler and runtime.
//!
//! @h "How to use"
//!   @dl
//!     @item
//!       Create an object of this class for each JSONPath expression
//!       you want to evaluate.
//!     @item
//!       Call @[apply()] with each of the JSON values you want to
//!       evaluate it for. Note that @[apply()] returns an array
//!       of all matching values.
//!   @enddl
class Query
{
  protected array(Selector) selectors = ({});

  //! Compile a JSONPath expression.
  //!
  //! @param jsonpath
  //!   JSONPath expression.
  //!
  //! @param is_expr
  //!   Flag indicating that this is a sub-expression
  //!   (and thus may use the @tt{'@@'@}-operator in
  //!   addition to the @tt{'$'@}-operator).
  //!
  //! @seealso
  //!   @[apply()]
  protected void create(string|array(string)|ADT.Stack jsonpath,
                        int(1bit)|void is_expr)
  {
    if (objectp(jsonpath)) {
      selectors = parse_tokens(jsonpath, is_expr);
      return;
    }

    array(string) tokens =
      arrayp(jsonpath)? jsonpath: Parser.ECMAScript.split(jsonpath);
    // Strip whitespace.
    tokens = predef::filter(tokens,
                            lambda(string token) {
                              return sizeof(token) &&
                                !(< ' ', '\t', '\n', '\r' >)[token[0]];
                            });
    // Group parenthesis.
    tokens = Parser.Pike.group(tokens);
    ADT.Stack token_stack = ADT.Stack();
    token_stack->set_stack(reverse(tokens));
    selectors = parse_tokens(token_stack, is_expr);
  }

  protected void expect(ADT.Stack tokens, string expected)
  {
    string t = tokens->pop();
    if (t != expected) {
      error("Invalid expression. Expected %q.\n", expected);
    }
  }

  protected string normalize_token(string token)
  {
    if (token[0] == '\'') {
      if (!has_value(token, "\"")) {
        return "\"" + token[1..<1] + "\"";
      }

      string new_token = "\"";
      int q = 0;
      foreach((token/1)[1..<1], string s) {
        if ((s == "\"") && !q) {
          new_token += "\\";
        } else if (q) {
          q = 0;
        } else if (s == "\\") {
          q = 1;
        }
        new_token += s;
      }
      if (q) {
        // Shouldn't happen...
        new_token += "\\";
      }

      return new_token + "\"";
    }

    return token;
  }

  protected mixed eval_token(string token)
  {
    token = normalize_token(token);
    return Standards.JSON.decode(token);
  }

  protected int parse_int(ADT.Stack tokens)
  {
    string token = tokens->pop();
    if (token == "-") {
      return -((int)(tokens->pop()));
    }
    return (int)token;
  }

  protected Expression parse_filter_expression(ADT.Stack tokens)
  {
  }

  protected Selector parse_selector(ADT.Stack tokens)
  {
    switch(tokens->peek()[0..0]) {
    case "0": case "1": case "2": case "3": case "4":
    case "5": case "6": case "7": case "8": case "9": case "-":
      // index-selector/slice-selector
      int val = parse_int(tokens);
      switch (sizeof(tokens) && tokens->peek()) {
      case ":":
        // slice-selector
        break;
      default:
        return val;
      }
      break;
    case "\"": case "\'":
      // name-selector
      return eval_token(tokens->pop());
    case "?":
      // filter-selector
      tokens->pop();
      return parse_filter_expression(tokens);
    case "*":
      // wildcard-selector
      tokens->pop();
      return Wild;
    }
    error("Invalid expression.\n");
  }

  protected Selector compile_segment(ADT.Stack tokens)
  {
    // Parse child-segment or descendant-segment

    array(string)|string t = tokens->pop();
    switch(t[0..0]) {
    case ".":
      // .wildcard-selector/.member-name-shorthand/..
      t = tokens->pop();
      switch(t) {
      case ".":
        // descendant-segment
        return DescendantExpression(tokens);
      case "*":
        return Wild;
      default:
        if (stringp(t)) return t;
        error("Invalid expression.\n");
      }
      break;

    default:
      if (arrayp(t) && (t[0] == "[")) {
        // bracketed-selection
        array(Selector) selectors = ({});
        ADT.Stack token_stack = ADT.Stack();
        token_stack->set_stack(reverse(t[1..<1]));
        token_stack->push(",");

        do {
          expect(token_stack, ",");
          selectors += ({ parse_selector(token_stack) });
        } while (sizeof(token_stack));

        if (sizeof(selectors) == 1) {
          return selectors[0];
        }
        if (!sizeof(selectors)) {
          // Probably not reached.
          error("Invalid expression.\n");
        }
        return MultiSelectExpression(selectors);
      } else {
        error("Invalid expression.\n");
      }
      break;
    }
  }

  protected array(Selector) parse_tokens(ADT.Stack tokens,
                                         int(1bit)|void is_expr)
  {
    array(Selector) res = ({});

    string|array(string|array) token = tokens->pop();
    switch(token) {
    case "$":
      res += ({ Dollar });
      break;
    case "@":
      if (!is_expr) {
        error("Invalid expression!\n");
      }
      // NB: Current node is the default.
      break;
    default:
      if (arrayp(token) && (token[0] == "(")) {
        ADT.Stack token_stack = ADT.Stack();
        token_stack->set_stack(reverse(token[1..<1]));
        res = parse_tokens(token_stack, is_expr);
      } else {
        error("Invalid expression.\n");
      }
      break;
    }

    while (sizeof(tokens)) {
      res += ({ compile_segment(tokens) });
    }

    return res;
  }

  //! Apply the compiled expression on a JSON-compatible value.
  //!
  //! @param json_value
  //!   Value to operate on.
  //!
  //! @param root_value
  //!   Value that the @tt{'$'@}-operator should refer to.
  //!   Defaults to @[json_value]. Typically only relevant
  //!   when operating on sub-expressions.
  //!
  //! @returns
  //!   Returns an array with the matching selected values.
  //!   If no values matched, the empty array @expr{({})@}
  //!   will thus be returned.
  array apply(mixed json_value, mixed|void root_value)
  {
    return apply_selector(selectors, json_value, root_value);
  }

  protected string _sprintf(int|void c)
  {
    array(Selector) s = selectors;
    if (!sizeof(s) || (s[0] != Dollar)) {
      s = ({ "@" }) + s;
    }
    return map(s, lambda(mixed x) {
      if (stringp(x)) return x;
      return sprintf("%O", x);
    }) * ".";
  }
}

//! Handles text substitutions where the template
//! text contains embedded JSONPath queries inside
//! @expr{"{{"@} and @expr{"}}"@}-braces.
class EmbeddedQuery
{
  //! Interleaved string fragments and JSONPath queries.
  array(string|Query) fragments = ({});

  protected void create(string template)
  {
    // FIXME: Quick and dirty. Does not consider nesting or quoting.
    foreach(template/"{{"; int i; string frag) {
      if (i) {
        array(string) a = frag/"}}";
        if (sizeof(a) >= 2) {
          fragments += ({ Query(a[0]) });
          frag = a[1..] * "}}";
        }
      }
      if (sizeof(frag)) {
        fragments += ({ frag });
      }
    }
  }

  string dwim_encode(mixed val)
  {
    if (stringp(val)) return val;
    return Standards.JSON.encode(val);
  }

  string apply(mixed json_val)
  {
    String.Buffer buf = String.Buffer();
    foreach(fragments, string|Query frag) {
      if (stringp(frag)) {
        buf->add(frag);
      } else {
        buf->add(map(frag->apply(json_val), dwim_encode) * ", ");
      }
    }
    return buf->get();
  }
}
