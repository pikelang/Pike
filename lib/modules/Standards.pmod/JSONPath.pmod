#pike __REAL_VERSION__

//! Parser and evaluator of the JSONPath syntax.
//!
//! @seealso
//!   @rfc{9535@}

local protected {

  //! Base class for all expressions.
  class Expression
  {
    protected array json_values(mixed val)
    {
      if (arrayp(val) || mappingp(val)) {
        array ret = values(val);
        if (mappingp(val)) {
          // Extension: Use a stable order.
          sort(indices(val), ret);
        }
        return ret;
      }
      return ({});
    }

    //! Apply the expression on a JSON value.
    //!
    //! @returns
    //!   Returns an array with the matching values.
    //!   If no values matched the empty array @expr{({})@} will be returned.
    array apply(mixed json_value, mixed root_value)
    {
      return ({ json_value });
    }
  }

  //! Type for selectors.
  //! @mixed
  //!    @type int|string
  //!      Integers and strings are short hand for indexing
  //!      the current @tt{json_value@}(s) with the same.
  //!
  //!    @type Expression
  //!      @[Expression()->apply()] is called and its return
  //!      value becomes the new @tt{json_value@}(s).
  //! @endmixed
  typedef int|string|Expression Selector;

  //! Expresion selecting the @tt{root_value@}.
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

  //! Expresion selecting the @tt{root_value@}.
  class HereExpression
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      return json_values(json_value);
    }

    protected string _sprintf(int|void c)
    {
      return "@";
    }
  }

  HereExpression At = HereExpression();

  //! Expression evaluating to a literal value.
  class LiteralExpression(mixed value)
  {
    array apply(mixed json_value, mixed root_value)
    {
      if (undefinedp(value)) return ({});
      return ({ value });
    }

    protected string _sprintf(int|void c)
    {
      if (stringp(value)) {
        return sprintf("%%%q", value);
      } else {
        return sprintf("%%%O", value);
      }
    }
  }

  class LogicalOrExpression(Expression e1, Expression e2)
  {
    array apply(mixed json_value, mixed root_value)
    {
      array ret = e1->apply(json_value, root_value);
      if (sizeof(ret)) return ret;
      return e2->apply(json_value, root_value);
    }
  }

  class LogicalAndExpression(Expression e1, Expression e2)
  {
    array apply(mixed json_value, mixed root_value)
    {
      array ret = e1->apply(json_value, root_value);
      if (!sizeof(ret)) return ret;
      return e2->apply(json_value, root_value);
    }
  }

  class NotExpression(Expression e)
  {
    array apply(mixed json_value, mixed root_value)
    {
      array ret = e->apply(json_value, root_value);
      if (sizeof(ret)) return ({});
      return ({ Val.true });
    }
  }

  //! Expression evaluating to all values at the current level.
  class WildExpression
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      if (arrayp(json_value)) return json_value;
      return json_values(json_value);
    }

    protected string _sprintf(int|void c)
    {
      return "*";
    }
  }

  WildExpression Wild = WildExpression();

  //! Expression evaluating to all values at all depths.
  class DescendantExpression
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      array ret = ({ json_value });
      if (mappingp(json_value) || arrayp(json_value)) {
        array vals = values(json_value);
        if (mappingp(json_value)) {
          // Extension: Use a stable order.
          sort(indices(json_value), vals);
        }

        foreach(vals, mixed j) {
          ret += apply(j, root_value);
        }
      }
      return ret;
    }

    protected string _sprintf(int|void c)
    {
      return "..";
    }
  }

  DescendantExpression Descend = DescendantExpression();

  //! Expression filtering the contents of @tt{json_value@}
  //! with respect to if the @[filter]-expression matches.
  class FilterExpression(Expression filter)
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      array ret = ({});
      foreach(json_values(json_value)/1, mixed val) {
        if (sizeof(filter->apply(val, root_value))) {
          ret += val;
        }
      }
      return ret;
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

    if (arrayp(val) || stringp(val)) {
      if (intp(s)) {
        if (s < 0) s += sizeof(val);
        val = val[s..s];
        if (!arrayp(val)) val = ({ val });
        return val;
      }
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

  //! Select multiple values in sequence.
  class MultiSelectExpression(array(Selector) selectors)
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      return map(selectors/1, apply_selector, json_value, root_value) * ({});
    }
  }

  //! Slice an array.
  class SliceExpression(int start, int end, int step)
  {
    inherit Expression;

    array apply(mixed json_value, mixed root_value)
    {
      if (arrayp(json_value) || stringp(json_value)) {
        int s = start;
        if (undefinedp(s)) {
          s = (step >= 0) ? 0 : -1;
        }
        if (s < 0) s += sizeof(json_value);
        if (s < 0) s = 0;
        if (s > sizeof(json_value)) s = sizeof(json_value);

        int e = end;
        if (undefinedp(end)) {
          e = (step >= 0) ? sizeof(json_value)-1 : 0;
        } else {
          if (e < 0) {
            e += sizeof(json_value);
          }
          // Adjust e to be the last (potentially) included element.
          if (step > 0) {
            e--;
          } else {
            e++;
          }
        }

        if (step > 0) {
          if (s > e) {
            return ({});
          }
          json_value = json_value[s..e];
          if (step == 1) {
            if (stringp(json_value)) {
              return ({ json_value });
            }
            return json_value;
          }
          if (arrayp(json_value)) {
            return column(json_value/step, 0);
          } else {
            return ({ map(json_value/step, `[], 0, 0) * "" });
          }
        }
        if (s < e) {
          return ({});
        }
        if (step == -1) {
          json_value = reverse(json_value[e..s]);
          if (stringp(json_value)) {
            return ({ json_value });
          }
          return json_value;
        }

        if (stringp(json_value)) {
          string ret = "";
          for(int i = s; i >= e; i += step) {
            ret += json_value[i..i];
          }
          return ({ ret });
        }

        array ret = ({});
        for(int i = s; i >= e; i += step) {
          ret += json_value[i..i];
        }
        return ret;
      }
      return ({});
    }
  }

  class FunctionCallExpression
  {
    inherit Expression;

    string fun_name;
    array(Expression) fun_args = ({});

    // RFC 9535 2.3.5.2.2
    array jsonpath__eq(array arg1, array arg2)
    {
      if (equal(arg1, arg2)) {
        return ({ Val.true });
      }
      return ({});
    }

    array jsonpath__ne(array arg1, array arg2)
    {
      if (!equal(arg1, arg2)) {
        return ({ Val.true });
      }
      return ({});
    }

    array jsonpath__ge(array arg1, array arg2)
    {
      if (equal(arg1, arg2)) {
        return ({ Val.true });
      }
      if (arg1 && sizeof(arg1) && arg2 && sizeof(arg2)) {
        catch {
          if (arg1[0] >= arg2[0]) {
            return ({ Val.true });
          }
        };
      }
      return ({});
    }

    array jsonpath__le(array arg1, array arg2)
    {
      if (equal(arg1, arg2)) {
        return ({ Val.true });
      }
      if (arg1 && sizeof(arg2) && arg2 && sizeof(arg2)) {
        catch {
          if (arg1[0] <= arg2[0]) {
            return ({ Val.true });
          }
        };
      }
      return ({});
    }

    array jsonpath__gt(array arg1, array arg2)
    {
      if (!arg1 || !sizeof(arg1) || !arg2 || !sizeof(arg2)) {
        return ({});
      }
      catch {
        if (arg1[0] > arg2[0]) {
          return ({ Val.true });
        }
      };
      return ({});
    }

    array jsonpath__lt(array arg1, array arg2)
    {
      if (!arg1 || !sizeof(arg1) || !arg2 || !sizeof(arg2)) {
        return ({});
      }
      catch {
        if (arg1[0] < arg2[0]) {
          return ({ Val.true });
        }
      };
      return ({});
    }

    // RFC 9535 2.4.4
    array jsonpath_length(array arg)
    {
      if (!arg || !sizeof(arg)) return ({});
      mixed x = arg[0];
      if (arrayp(x) || stringp(x) || mappingp(x)) {
        return ({ sizeof(x) });
      }
      return ({});
    }

    // RFC 9535 2.4.5
    array jsonpath_count(array arg)
    {
      if (!arg) return ({});	// Too few arguments.
      return ({ sizeof(arg) });
    }

#if constant(Regexp.PCRE.Widestring)
    // RFC 9535 2.4.6
    array jsonpath_match(array str_arg, array regexp_arg)
    {
      if (!str_arg || !sizeof(str_arg)) return ({});
      string str = str_arg[0];
      if (!stringp(str)) return ({});
      if (!regexp_arg || !sizeof(regexp_arg)) return ({});
      string regexp = regexp_arg[0];
      if (!stringp(regexp)) return ({});
      catch {
        Regexp.PCRE pcre = Regexp.PCRE.Widestring("^" + regexp + "$");
        if (pcre->match(str)) {
          return ({ Val.true });
        }
      };
      return ({});
    }

    // RFC 9535 2.4.7
    array jsonpath_search(array str_arg, array regexp_arg)
    {
      if (!str_arg || !sizeof(str_arg)) return ({});
      string str = str_arg[0];
      if (!stringp(str)) return ({});
      if (!regexp_arg || !sizeof(regexp_arg)) return ({});
      string regexp = regexp_arg[0];
      if (!stringp(regexp)) return ({});
      catch {
        Regexp.PCRE pcre = Regexp.PCRE.Widestring(regexp);
        if (pcre->match(str)) {
          return ({ Val.true });
        }
      };
      return ({});
    }
#endif // Regexp.PCRE.Widestring

    // RFC 9535 2.4.8
    array jsonpath_value(array arg)
    {
      if (!arg) return ({});
      return arg[..1];
    }

    protected void create(string fun_name, array(Expression) fun_args)
    {
      this::fun_name = "jsonpath_" + fun_name;
      this::fun_args = fun_args;
      if (!this[this::fun_name]) {
        error("Unimplemented function: %s().\n", fun_name);
      }
    }

    array apply(mixed json_value, mixed root_value)
    {
      array args = fun_args->apply(json_value, root_value);
      return this[fun_name](@args);
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

  protected void syntax_error(ADT.Stack tokens, string|void expected)
  {
    if (!sizeof(tokens)) {
      if (expected) {
        error("Unexpected end of expression. Expected %q.\n", expected);
      }
      error("Unexpected end of expression.\n");
    }
    string|array token = tokens->peek();
    if (arrayp(token)) {
      token = token[0];
    }
    if (expected) {
      error("Invalid expression. Expected %q, got %q.\n", expected, token);
    }
    error("Invalid expression. Unexpected %q.\n", token);
  }

  protected void expect(ADT.Stack tokens, string expected)
  {
    string t = sizeof(tokens) && tokens->peek();
    if (t == expected) {
      tokens->pop();
      return;
    }
    syntax_error(tokens, expected);
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

  protected Expression parse_comparable(ADT.Stack tokens)
  {
    // Parse literal or singular-query or function-expr
    // or number or string-literal or true or false or null

    string|array token = tokens->peek();
    switch(token) {
    case "!": // paren-expr or test-expr
      tokens->pop();
      return NotExpression(parse_comparable(tokens));

    case "false":
      tokens->pop();
      return LiteralExpression(UNDEFINED);

    case "null": // RFC 9535 2.6
    case "true":
      return LiteralExpression(eval_token(tokens->pop()));

    case "-":
      tokens->pop();
      // NB: Always followed by an unsigned number.
      return LiteralExpression(-eval_token(tokens->pop()));

    case "$":
    case "@":
      // singular-query or filter-query.
      return Query(tokens, 1);

    default:
      if (arrayp(token) && (token[0] == "(")) {
        // paren-expr
        tokens->pop();
        ADT.Stack token_stack = ADT.Stack();
        token_stack->set_stack(reverse(token[1..<1]));
        return parse_logical_expr(token_stack);
      }
      if (stringp(token)) {
        int c = token[0];
        if ((c == '\'') || (c == '\"') ||
            ((token[0] >= '0') && (token[0] <= '9'))) {
          // string literal or number.
          return LiteralExpression(eval_token(tokens->pop()));
        }
        if ((c >= 'a') && (c <= 'z')) {
          // function-name.
          string fun_name = token;
          tokens->pop();
          token = tokens->pop();
          if (!arrayp(token) || (token[0] != "(")) {
            tokens->push(token);
            syntax_error(tokens, "(");
          }
          ADT.Stack token_stack = ADT.Stack();
          token_stack->set_stack(reverse(token[1..<1]));
          if (sizeof(token_stack)) {
            token_stack->push(",");
          }
          array(Expression) fun_args = ({});
          while (sizeof(token_stack)) {
            expect(token_stack, ",");
            fun_args += ({ parse_logical_expr(token_stack) });
          }
          return FunctionCallExpression(fun_name, fun_args);
        }
      }
      break;
    }
    syntax_error(tokens);
  }

  protected Expression parse_basic_expr(ADT.Stack tokens)
  {
    // Parse basic-expr or paren-expr or comparison-expr or test-expr
    // or filter-query

    Expression e = parse_comparable(tokens);
    while (sizeof(tokens)) {
      string|array token = tokens->peek();
      string cmp_op = ([
        "==": "_eq",
        "!=": "_ne",
        "<=": "_le",
        ">=": "_ge",
        "<": "_lt",
        ">": "_gt",
      ])[token];
      if (!cmp_op) break;

      tokens->pop();
      e = FunctionCallExpression(cmp_op,
                                 ({ e, parse_comparable(tokens) }));
    }
    if (!sizeof(tokens) || (< ",", "&&", "||" >)[tokens->peek()]) {
      return e;
    }
    syntax_error(tokens);
  }

  protected Expression parse_logical_and_expr(ADT.Stack tokens)
  {
    // Parse logical-and-expr

    Expression e = parse_basic_expr(tokens);
    while (sizeof(tokens) && tokens->peek() == "&&") {
      tokens->pop();
      Expression e2 = parse_basic_expr(tokens);
      e = LogicalAndExpression(e, e2);
    }
    return e;
  }

  protected Expression parse_logical_expr(ADT.Stack tokens)
  {
    // Parse logical-expr or logical-or-expr

    Expression e = parse_logical_and_expr(tokens);
    while (sizeof(tokens) && tokens->peek() == "||") {
      tokens->pop();
      Expression e2 = parse_logical_and_expr(tokens);
      e = LogicalOrExpression(e, e2);
    }
    if (sizeof(tokens) && (tokens->peek() != ",")) {
      syntax_error(tokens);
    }
    return e;
  }

  protected Selector parse_selector(ADT.Stack tokens)
  {
    // Parse selector or name-selector or wildcard-selector
    // or slice-selector or index-selector or filter-selector
    // or string-literal
    int val = UNDEFINED;
    switch(tokens->peek()[0..0]) {
    case "0": case "1": case "2": case "3": case "4":
    case "5": case "6": case "7": case "8": case "9": case "-":
      // index-selector/slice-selector
      val = parse_int(tokens);
    case ":":
      // Potential slice-selector
      switch (sizeof(tokens) && tokens->peek()) {
      case ":":
        tokens->pop();
        int end = UNDEFINED;
        if (sizeof(tokens) && (tokens->peek() != ":") &&
            (tokens->peek() != ",")) {
          end = parse_int(tokens);
        }
        int step = 1;
        if (sizeof(tokens) && (tokens->peek() == ":")) {
          tokens->pop();
          if (sizeof(tokens) && (tokens->peek() != ",")) {
            step = parse_int(tokens);
          }
        }
        if (!step) {
          return LiteralExpression(UNDEFINED);
        }
        return SliceExpression(val, end, step);
      }
      return val;

    case "\"": case "\'":
      // name-selector or string-literal
      return eval_token(tokens->pop());
    case "?":
      // filter-selector
      tokens->pop();
      return FilterExpression(parse_logical_expr(tokens));
    case "*":
      // wildcard-selector
      tokens->pop();
      return Wild;
    case "$":
    case "@":
    case "%":
      return Query(tokens, 1);
    }
    syntax_error(tokens);
  }

  protected Selector compile_segment(ADT.Stack tokens,
                                     int(1bit)|void is_expr)
  {
    // Parse child-segment or descendant-segment or bracketed-selection

    array(string)|string t = tokens->pop();
    switch(t[0..0]) {
    case ".":
      // .wildcard-selector/.member-name-shorthand/..
      t = tokens->pop();
      switch(t) {
      case ".":
        // descendant-segment
        if (sizeof(tokens) && !arrayp(tokens->peek())) {
          tokens->push(".");
        }
        return Descend;
      case "*":
        // wildcard-selector
        return Wild;
      default:
        if (stringp(t)) {
          // If valid, this is a member-name-shorthand.
          return t;
        }
      }
      tokens->push(t);
      syntax_error(tokens);
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
        return MultiSelectExpression(selectors);
      }
      break;
    }
    tokens->push(t);
    if (!is_expr) {
      syntax_error(tokens);
    }
    return UNDEFINED;
  }

  protected array(Selector) parse_tokens(ADT.Stack tokens,
                                         int(1bit)|void is_expr)
  {
    // Parse a jsonpath-query or rel-query (aka filter-query).
    // Also supports the Pike literal value rooted extension.
    array(Selector) res = ({});

    string|array(string|array) token = tokens->pop();
    switch(token) {
    case "$":
      res += ({ Dollar });
      break;
    case "@":
      if (!is_expr) {
        tokens->push(token);
        syntax_error(tokens);
      }
      res += ({ At });
      break;
    default:
      if (arrayp(token) && (token[0] == "(")) {
        ADT.Stack token_stack = ADT.Stack();
        token_stack->set_stack(reverse(token[1..<1]));
        res = parse_tokens(token_stack, is_expr);
      } else {
        tokens->push(token);
        syntax_error(tokens);
      }
      break;
    }

    while (sizeof(tokens)) {
      Selector s = compile_segment(tokens, is_expr);
      if (undefinedp(s)) break;
      res += ({ s });
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
//!
//! @seealso
//!   @[Query]
class EmbeddedQuery
{
  //! Interleaved string fragments and JSONPath queries.
  protected array(string|Query) fragments = ({});

  //! Compile a template with @tt{{{}}@}-embedded JSONPath queries.
  //!
  //! @param template
  //!   Template to compile.
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

  protected string dwim_encode(mixed val)
  {
    if (stringp(val)) return val;
    return Standards.JSON.encode(val);
  }

  //! Apply the compiled template on a JSON value.
  //!
  //! @param json_val
  //!   JSON-value to apply the template on.
  //!
  //! @param separator
  //!   Separator in case of multiple values.
  //!   Defaults to @expr{", "@}.
  //!
  //! @note
  //!   Values that are not strings are inserted as JSON values.
  //!
  //! @returns
  //!   Returns the template applied on @[json_val].
  string apply(mixed json_val, string|void separator)
  {
    if (!separator) separator = ", ";
    String.Buffer buf = String.Buffer();
    foreach(fragments, string|Query frag) {
      if (stringp(frag)) {
        buf->add(frag);
      } else {
        buf->add(map(frag->apply(json_val), dwim_encode) * separator);
      }
    }
    return buf->get();
  }
}
