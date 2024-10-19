#charset utf-8
#pike __REAL_VERSION__
#require constant(Regexp.PCRE.Widestring)

//! This is a port of the Javascript Markdown parser 'Marked'
//! @url{https://github.com/chjj/marked@}. The only method needed to
//! be used is @[parse()] which will transform Markdown text to HTML.
//!
//! For a description on Markdown, go to the web page of the inventor of
//! Markdown @url{https://daringfireball.net/projects/markdown/@}.

import Regexp.PCRE;

#define nl() (options->newline ? "\n" : "")

// Cache for compiled regexps
private mapping(string:object) __re_cache = ([]);

// Returns the precompiled regexp PATTERN from cache if it's previously used.
// Otherwise it compiles, caches and returns it.
#define REGX(PATTERN, ARGS...) \
  (__re_cache[PATTERN + #ARGS] || \
  (__re_cache[PATTERN + #ARGS] = R(PATTERN, ARGS)))

// Shortened regexp options
protected constant RM = OPTION.MULTILINE;
protected constant RI = OPTION.CASELESS;

//! Convert markdown @[md] to html
//!
//! @param options
//!  @mapping
//!   @member bool "gfm"
//!    Enable Github Flavoured Markdown.                                  (true)
//!   @member bool "tables"
//!    Enable GFM tables. Requires "gfm"                                  (true)
//!   @member bool "breaks"
//!    Enable GFM "breaks". Requires "gfm"                               (false)
//!   @member bool "pedantic"
//!    Conform to obscure parts of markdown.pl as much as possible.
//!    Don't fix any of the original markdown bugs or poor behavior.     (false)
//!   @member bool "sanitize"
//!    Sanitize the output. Ignore any HTML that has been input.         (false)
//!   @member bool "mangle"
//!    Mangle (obfuscate) autolinked email addresses                      (true)
//!   @member bool "smart_lists"
//!    Use smarter list behavior than the original markdown.              (true)
//!   @member bool "smartypants"
//!    Use "smart" typographic punctuation for things like quotes and
//!    dashes.                                                           (false)
//!   @member string "header_prefix"
//!    Add prefix to ID attributes of header tags                        (empty)
//!   @member bool "xhtml"
//!    Generate self closing XHTML tags                                  (false)
//!   @member bool "newline"
//!    Add a newline after tags. If false the output will be
//!    on one line (well, newlines in text will be kept).                (false)
//!   @member Renderer "renderer"
//!    Use this renderer to render output.                            (Renderer)
//!   @member Lexer "lexer"
//!    Use this lexer to parse blocks of text.                           (Lexer)
//!   @member InlineLexer "inline_lexer"
//!    Use this lexer to parse inline text.                        (InlineLexer)
//!   @member Parser "parser"
//!    Use this parser instead of the default.                          (Parser)
//!  @endmapping
string parse(string md, void|mapping options)
{
  options = default_options + (options||([]));
  if (!options->lexer)
    options->lexer = Lexer;
  if (!options->parser)
    options->parser = Parser;

  return options->parser(options)->parse(options->lexer(options)->lex(md));
}

protected typedef mapping(string:R|mapping(string:R)) RuleMap;

protected object noop = class {
  string source;
  array|zero split2() { return 0; }
  protected string _sprintf(int t) { return "noop()"; }
}();

protected function make_regexp(R|string regex, void|int opts)
{
  regex = (string)regex;
  function(string,string,void|bool:function|R) fun, fun2;

  R rep = REGX("(^|[^[])\\^");

  // In most generated regexps we only want to replace the first occurance
  // of `name` (and that's how replace in Javascript works by default). But
  // in Pike replace() will replace all occurances of `name` by default. We
  // work around that below, but in a few instances we want the global
  // replace() so we state that explicitly.
  fun = lambda (string name, R|string val, void|bool all) {
    if (!name || !sizeof(name)) {
      return R(regex, opts);
    }

    val = rep->replace((string)val, lambda (string a, string b) {
      return b;
    });

    if (all) {
      regex = replace(regex, name, val);
    }
    else {
      regex = replace1(regex, name, val);
    }

    return fun;
  };

  fun2 = fun;

  return fun;
}

//! Replaces the first occurance of @[from] in @[subject] to @[to]
protected string replace1(string subject, string from, string to)
{
  int p1 = search(subject, from);

  if (p1 > -1) {
    int p2 = search(subject, from, p1+1);
    if (p2 > -1) {
      subject = replace(subject[0..p2-1], from, to) + subject[p2..];
    }
    else {
      subject = replace(subject, from, to);
    }
  }

  return subject;
}

//! HTML encode @tt{<>"'@}. If @[enc] is @tt{true@} @tt{&@} will also be encoded
protected string encode_html(string html, void|bool enc)
{
  return replace(REGX((!enc ? "&(?!\\#?\\w+;)" : "&"))->replace(html, "&amp;"),
                 ([ "<"  : "&lt;",
                    ">"  : "&gt;",
                    "\"" : "&quot;",
                    "'"  : "&#39;" ]));
}

protected string decode_html(string html)
{
  return REGX("&([\\#\\w]+);")->replace(html, lambda (string a, string b) {
    b = lower_case(b);

    if (b == "colon") return ":";
    if (b[0] == '#') {
      return b[1] == 'x'
                      ? sprintf("%x", (int)b[2..])
                      : sprintf("%c", (int)b[1..]);
    }

    return "";
  });
}

protected constant default_options = ([
  "gfm"           : true,
  "tables"        : true,
  "breaks"        : false,
  "pedantic"      : false,
  "sanitize"      : false,
  "sanitizer"     : UNDEFINED,
  "mangle"        : true,
  "smart_lists"   : true,
  "silent"        : false,
  "highlight"     : UNDEFINED,
  "lang_prefix"   : "lang-",
  "smartypants"   : false,
  "header_prefix" : "",
  "renderer"      : UNDEFINED,
  "xhtml"         : false,
  "newline"       : true
]);

protected class R
{
  inherit Widestring : ws;

  protected string src;

  protected void create(R|string re, void|int options)
  {
    src = (string)re;
    ws::create(src, options);
  }

  string `source() { return src; }

  array(string)|zero split_string(string subject)
  {
    array(array(int)) offsets = ({});

    ws::matchall(subject, lambda (array a, array b) {
      offsets += ({ b });
    });

    if (!sizeof(offsets)) {
      return 0;
    }

    offsets += ({ 0 });

    array(string) ret = ({ });
    array(int) c;
    int start, stop;

    for (int i; i < sizeof(offsets); i++) {
      c    = offsets[i];
      stop = c && c[0] || sizeof(subject);
      ret += ({ subject[start .. stop] });
      if (!c) break;
      start = c[1];
    }

    return ret;
  }

  array(string) split_all(string subject)
  {
    array(string) ret = ({});

    ws::matchall(subject, lambda (array a, array b) {
      ret += ({ a[0] });
    });

    return ret;
  }

  protected string _sprintf(int t)
  {
    switch (t)
    {
      case 's':
        return source;

      case 'O':
        return "object(" + source + ")";

      default:
        error("Unhandled sprintf_format (%%%c)! ", t);
        break;
    }
  }

  protected string cast(string how)
  {
    return src;
  }
}

private RuleMap block_rules;
RuleMap get_block_rules()
{
  if (block_rules)
    return block_rules;

  RuleMap block = ([
    "newline"    : R("^\\n+"),
    "code"       : R("^( {4}[^\\n]+\\n*)+"),
    "hr"         : R("^( *[-*_]){3,} *(?:\\n+|$)"),
    "heading"    : R("^ *(\\#{1,6}) *([^\\n]+?) *\\#* *(?:\\n+|$)"),
    "lheading"   : R("^([^\\n]+)\\n *(=|-){2,} *(?:\\n+|$)"),
    "blockquote" : R("^( *>[^\\n]+(\\n(?!def)[^\\n]+)*\\n*)+"),
    "list"       : R("^( *)(bull) [\\s\\S]+?(?:hr|def|\\n{2,}(?! )(?!\\1bull )\\n*|\\s*$)"),
    "html"       : R("^ *(?:comment *(?:\\n|\\s*$)|closed *(?:\\n{2,}|\\s*$)|closing *(?:\\n{2,}|\\s*$))"),
    "def"        : R("^ *\\[([^\\]]+)\\]: *<?([^\\s>]+)>?(?: +[\"(]([^\\n]+)[\")])? *(?:\\n+|$)"),
    "paragraph"  : R("^((?:[^\\n]+\\n?(?!hr|heading|lheading|blockquote|tag|def))+)\\n*"),
    "text"       : R("^[^\\n]+"),
    "table"      : noop,
    "fences"     : noop,
    "nptable"    : noop,
  ]);

  block->bullet = R("(?:[*+-]|\\d+\\.)");
  block->item =
    make_regexp("^( *)(bull) [^\\n]*(?:\\n(?!\\1bull )[^\\n]*)*", RM)
               ("bull", block->bullet, true)
               ();

  block->list =
    make_regexp(block->list)
               ("bull", block->bullet, true)
               ("hr", "\\n+(?=\\1?(?:[-*_] *){3,}(?:\\n+|$))")
               ("def", "\\n+(?=" + block->def->source + ")")
               ();

  block->blockquote =
    make_regexp(block->blockquote)
               ("def", block->def)
               ();

  block->_tag = R("(?!(?:"
                  "a|em|strong|small|s|cite|q|dfn|abbr|data|time|code"
                  "|var|samp|kbd|sub|sup|i|b|u|mark|ruby|rt|rp|bdi|bdo"
                  "|span|br|wbr|ins|del|img)\\b)\\w+(?!:/|[^\\w\\s@]*@)\\b");

  block->html =
    make_regexp(block->html)
               ("comment", "<!--[\\s\\S]*?-->")
               ("closed",  "<(tag)[\\s\\S]+?</\\1>")
               ("closing", "<tag(?:\"[^\"]*\"|'[^']*'|[^'\">])*?>")
               ("tag",     block->_tag, true)
               ();

  block->paragraph =
    make_regexp(block->paragraph)
               ("hr",         block->hr)
               ("lheading",   block->lheading)
               ("heading",    block->heading)
               ("blockquote", block->blockquote)
               ("tag",        "<" +  (string)block->_tag, true)
               ("def",        block->def)
               ();

  block->normal = copy_value(block);
  block->gfm = block->normal + ([
    "fences"    : R("^ *(`{3,}|~{3,})[ \\.]*(\\S+)? *\\n([\\s\\S]*?)\\s*\\1 *(?:\\n+|$)"),
    "paragraph" : R("^"),
    "heading"   : R("^ *(\\#{1,6}) +([^\\n]+?) *\\#* *(?:\\n+|$)")
  ]);

  block->gfm->paragraph =
    make_regexp(block->paragraph)
               ("(?!", "(?!"
                       + replace1(block->gfm->fences->source, "\\1", "\\2")
                       + "|"
                       + replace1(block->list->source, "\\1", "\\3") + "|")
               ();

  block->tables = block->gfm + ([
    "nptable" : R("^ *(\\S.*\\|.*)\\n *([-:]+ *\\|[-| :]*)\\n((?:.*\\|.*(?:\\n|$))*)\\n*"),
    "table"   : R("^ *\\|(.+)\\n *\\|( *[-:]+[-| :]*)\\n((?: *\\|.*(?:\\n|$))*)\\n*")
  ]);

  return block_rules = block;
}

private RuleMap inline_rules;
RuleMap get_inline_rules()
{
  if (inline_rules)
    return inline_rules;

  RuleMap inl = ([
    "escape"   : R("^\\\\([\\\\`*{}\\[\\]()\\#+\\-.!_>])"),
    "autolink" : R("^<([^ >]+(@|:/)[^ >]+)>"),
    "tag"      : R("^<!--[\\s\\S]*?-->|^</?\\w+(?:\"[^\"]*\"|'[^']*'|[^'\">])*?>"),
    "link"     : R("^!?\\[(inside)\\]\\(href\\)"),
    "reflink"  : R("^!?\\[(inside)\\]\\s*\\[([^\\]]*)\\]"),
    "nolink"   : R("^!?\\[((?:\\[[^\\]]*\\]|[^\\[\\]])*)\\]"),
    "strong"   : R("^__([\\s\\S]+?)__(?!_)|^\\*\\*([\\s\\S]+?)\\*\\*(?!\\*)"),
    "em"       : R("^\\b_((?:[^_]|__)+?)_\\b|^\\*((?:\\*\\*|[\\s\\S])+?)\\*(?!\\*)"),
    "code"     : R("^(`+)\\s*([\\s\\S]*?[^`])\\s*\\1(?!`)"),
    "br"       : R("^ {2,}\\n(?!\\s*$)"),
    "text"     : R("^[\\s\\S]+?(?=[\\\\<!\\[_*`]| {2,}\\n|$)"),
    "del"      : noop,
    "url"      : noop
  ]);

  inl->_inside = R("(?:\\[[^\\]]*\\]|[^\\[\\]]|\\](?=[^\\[]*\\]))*");
  inl->_href = R("\\s*<?([\\s\\S]*?)>?(?:\\s+['\"]([\\s\\S]*?)['\"])?\\s*");
  inl->link =
    make_regexp(inl->link)
               ("inside", inl->_inside)
               ("href", inl->_href)
               ();
  inl->reflink =
    make_regexp(inl->reflink)
               ("inside", inl->_inside)
               ();

  inl->normal = copy_value(inl);

  inl->gfm = inl->normal + ([
    "escape" : make_regexp(inl->escape)("])", "~|])")(),
    "url"    : R("^(https?://[^\\s<]+[^<.,:;\"')\\]\\s])"),
    "del"    : R("^~~(?=\\S)([\\s\\S]*?\\S)~~"),
    "text"   : make_regexp(inl->text)
                          ("]|", "~]|")
                          ("|", "|https?://|")
                          ()
    ]);

  inl->breaks = inl->gfm + ([
    "br"   : make_regexp(inl->br)("{2,}", "*")(),
    "text" : make_regexp(inl->gfm->text)("{2,}", "*")()
  ]);

  return inline_rules = inl;
}

//! Top-level parsing handler. It's usually easier to replace the Renderer
//! instead.
class Parser
{
  protected mapping options;
  protected array tokens = ({});
  protected mapping token;
  protected Renderer renderer;
  protected InlineLexer inline_lexer;

  protected void create(void|mapping opts)
  {
    options = opts || default_options;

    if (!options->renderer)
      options->renderer = Renderer;

    if (!options->inline_lexer)
      options->inline_lexer = InlineLexer;

    renderer = options->renderer = options->renderer(options);
  }

  private int pos = -1;

  #define next() (token = tokens[++pos])
  #define peek() tokens[pos + 1]

  //!
  string parse(Lexer src)
  {
    token = UNDEFINED;
    inline_lexer = options->inline_lexer(src->links, options);
    tokens = src->tokens;
    tokens += ({ 0 }); // sentinel

    String.Buffer out = String.Buffer();
    function add = out->add;

    while (next()) {
      add(tok());
    }

    return out->get();
  }

  //!
  protected string parse_text()
  {
    string body = token->text;

    while (peek()->type == "text") {
      body += (options->newline ? "\n" : " ") + (next()->text||"");
    }

    return inline_lexer->output(body);
  }

  //! Render a token (or group of tokens) to a string.
  protected string tok()
  {
    switch (token->type)
    {
      case "space":
        return "";

      case "hr":
        return renderer->hr();

      case "heading":
        return renderer->heading(inline_lexer->output(token->text),
                                 token->depth,
                                 token->text,
                                 token);
      case "code":
        return renderer->code(token->text,
                              token->lang,
                              token->escaped,
                              token);

      case "table":
        string header = "", body = "", cell = "";
        array(string) row;

        for (int i; i < sizeof(token->header); i++) {
          cell += renderer->tablecell(
            inline_lexer->output(token->header[i]),
            ([ "header" : true, "align" : token->align[i] ]),
            ([]),
          );
        }

        header += renderer->tablerow(cell, ([]));

        for (int i; i < sizeof(token->cells); i++) {
          row = token->cells[i];
          cell = "";

          for (int j; j < sizeof(row); j++) {
            cell += renderer->tablecell(
              inline_lexer->output(row[j]),
              ([ "header" : false, "align" : token->align[j] ]),
              ([]),
            );
          }

          body += renderer->tablerow(cell, ([]));
        }

        return renderer->table(header, body, token);

      case "blockquote_start":
        body = "";

        while (next()->type != "blockquote_end") {
          body += tok();
        }

        return renderer->blockquote(body, token);

      case "list_start":
        bool ordered = token->ordered;
        body = "";

        while (next()->type != "list_end") {
          body += tok();
        }

        return renderer->list(body, ordered, token);

      case "list_item_start":
        body = "";

        while (next()->type != "list_item_end") {
          body += token->type == "text" ? parse_text() : tok();
        }

        return renderer->listitem(body, token);

      case "loose_item_start":
        body = "";

        while (next()->type != "list_item_end") {
          body += tok();
        }

        return renderer->listitem(body, token);


      case "html":
        string html = !token->pre && !options->pedantic
                        ? inline_lexer->output(token->text)
                        : token->text;
        return renderer->html(html, token) + nl();

      case "paragraph":
        return renderer->paragraph(inline_lexer->output(token->text), token);

      case "text":
        return renderer->paragraph(parse_text(), token);

      default:
        error("Unhandled token: %O\n", token);
        break;
    }
  }
}

#define SRC_SUBSTR()  src = src[sizeof(cap[0]) .. ]
#define PUSH_TOKEN(T) _tokens += ({ T })

//! Block-level lexer (parses paragraphs, lists, tables, etc).
class Lexer
{
  protected array(mapping) _tokens = ({});
  protected mapping token_links = ([]);
  protected mapping options;
  protected RuleMap rules, block;

  //!
  mapping `links() { return token_links; }
  array(mapping) `tokens() { return _tokens; }

  protected void create(void|mapping opts)
  {
    options = opts || default_options;
    block = get_block_rules();
    rules = block->normal;

    if (options->gfm) {
      if (options->tables) {
        rules = get_block_rules()->tables;
      }
      else {
        rules = get_block_rules()->gfm;
      }
    }
  }

  //! Main lexing entry point. Subclass Lexer and override this to add
  //! post-processing or other changes.
  this_program lex(string src)
  {
    src = replace(src, ([ "\r\n"   : "\n",
                          "\r"     : "\n",
                          "\t"     : "    ",
                          "\u00a0" : " ",
                          "\u2424" : "\n" ]));
    tokenize(src, true);
    return this;
  }

  protected array tokenize(string src, bool|int(0..1) top, void|bool bq)
  {
    src = REGX("^ +$", RM)->replace(src, "");
    array(string) cap;
    bool next;

    // NB: Some versions of PCRE do not support having the end marker ($)
    //     in character classes. Add a terminating "\n" end sentinel if
    //     there isn't already one there.
    //     Note that this will not affect the output with newer versions
    //     of PCRE as a single ending "\n" is ignored.
    //
    //     Once affected version is PCRE 5.0 as distributed by Redhat FC4.
    if (sizeof(src) && !has_suffix(src, "\n")) {
      src += "\n";
    }

    while (src && sizeof(src)) {
      // Newline, (assignment)
      if (cap = rules->newline->split2(src)) {
        SRC_SUBSTR();
        if (sizeof(cap[0]) > 1) {
          PUSH_TOKEN(([ "type" : "space" ]));
        }
      }

      // Code
      if (cap = rules->code->split2(src)) {
        SRC_SUBSTR();
        string tmp = REGX("^ {4}", RM)->replace(cap[0], "");
        PUSH_TOKEN(([ "type" : "code",
                      "text" : !options->pedantic ?
                                REGX("\n+$")->replace(tmp, "") :
                                tmp ]));
        continue;
      }

      // fences (gfm)
      if (cap = rules->fences->split2(src)) {
        SRC_SUBSTR();
        PUSH_TOKEN(([ "type": "code",
                      "lang": cap[2],
                      "text": has_index(cap, 3) && cap[3] || "" ]));
        continue;
      }

      // heading
      if (cap = rules->heading->split2(src)) {
        SRC_SUBSTR();
        PUSH_TOKEN(([ "type"  : "heading",
                      "depth" : sizeof(cap[1]),
                      "text"  : cap[2] ]));
        continue;
      }

      // table no leading pipe (gfm)
      if (top && (cap = rules->nptable->split2(src))) {
        SRC_SUBSTR();

        mapping item = ([
          "type"   : "table",
          "header" : REGX(" *\\| *")->split_string(REGX("^ *| *\\| *$")
                                                   ->replace(cap[1], "")),
          "align"  : REGX(" *\\| *")->split_string(REGX("^ *|\\| *$")
                                                   ->replace(cap[2], "")),
          "cells"  : REGX("\\n$")->replace(cap[3], "") / "\n"
        ]);

        for (int i; i < sizeof(item->align); i++) {
          if (REGX("^ *-+: *$")->match(item->align[i])) {
            item->align[i] = "right";
          }
          else if (REGX("^ *:-+: *$")->match(item->align[i])) {
            item->align[i] = "center";
          }
          else if (REGX("^ *:-+ *$")->match(item->align[i])) {
            item->align[i] = "left";
          }
          else {
            item->align[i] = UNDEFINED;
          }
        }

        for (int i; i < sizeof(item->cells); i++) {
          item->cells[i] = REGX(" *\\| *")->split_string(item->cells[i]);
        }

        PUSH_TOKEN(item);

        continue;
      }

      // lheading
      if (cap = rules->lheading->split2(src)) {
        SRC_SUBSTR();
        PUSH_TOKEN(([ "type"  : "heading",
                      "depth" : cap[2] == "=" ? 1 : 2,
                      "text"  : cap[1] ]));
        continue;
      }

      // hr
      if (cap = rules->hr->split2(src)) {
        SRC_SUBSTR();
        PUSH_TOKEN(([ "type": "hr" ]));
        continue;
      }

      // blockquote
      if (cap = rules->blockquote->split2(src)) {
        SRC_SUBSTR();
        PUSH_TOKEN(([ "type": "blockquote_start" ]));

        string tmp = REGX("^ *> ?", RM)->replace(cap[0], "");

        // Pass `top` to keep the current
        // "toplevel" state. This is exactly
        // how markdown.pl works.
        tokenize(tmp, top, true);

        PUSH_TOKEN(([ "type": "blockquote_end" ]));

        continue;
      }

      // list
      if (cap = rules->list->split2(src)) {
        string bull = cap[2];

        SRC_SUBSTR();
        PUSH_TOKEN(([ "type"    : "list_start",
                      "ordered" : sizeof(bull) > 1 ]));

        // Get each top-level item.
        cap = rules->item->split_all(cap[0]); //cap[0].match(rules->item);

        next = false;
        int l = sizeof(cap);
        int i = 0;

        for (; i < l; i++) {
          string item = cap[i];

          // Remove the list item's bullet
          // so it is seen as the next token.
          int space = sizeof(item);
          item = REGX("^ *([*+-]|\\d+\\.) +")->replace(item, "");

          // Outdent whatever the
          // list item contains. Hacky.
          if (search(item, "\n ") > -1) {
            space -= sizeof(item);
            item = !options->pedantic
                   ? REGX("^ {1," + space + "}", RM)->replace(item, "")
                   : REGX("^ {1,4}", RM)->replace(item, "");
          }

          // Determine whether the next list item belongs here.
          // Backpedal if it does not belong in this list.
          if (options->smart_lists && i != l - 1) {
            string b = block->bullet->split2(cap[i+1])[0];
            if (bull != b && !(sizeof(bull) > 1 && sizeof(b) > 1)) {
              src = (cap[i+1..] * "\n") + src;
              i = l - 1;
            }
          }

          // Determine whether item is loose or not.
          // Use: /(^|\n)(?! )[^\n]+\n\n(?!\s*$)/
          // for discount behavior.
          bool loose = next || REGX("\\n\\n(?!\\s*$)")->match(item);

          if (i != l - 1) {
            next = item[-1] == '\n';
            if (!loose) loose = next;
          }

          PUSH_TOKEN(([ "type" : loose
                                 ? "loose_item_start"
                                 : "list_item_start" ]));
          // Recurse.
          tokenize(item, false, bq);

          PUSH_TOKEN(([ "type" : "list_item_end" ]));
        }

        PUSH_TOKEN(([ "type" : "list_end" ]));

        continue;
      }

      // html
      if (cap = rules->html->split2(src)) {
        SRC_SUBSTR();
        PUSH_TOKEN(([ "type" : options->sanitize ? "paragraph" : "html",
                      "pre"  : !options->sanitizer
                                  && (cap[1] == "pre"
                                      || cap[1] == "script"
                                      || cap[1] == "style"),
                      "text" : cap[0] ]));
        continue;
      }

      // def
      if ((!bq && top) && (cap = rules->def->split2(src))) {
        SRC_SUBSTR();
        token_links[lower_case(cap[1])] = ([
          "href":  cap[2],
          "title": cap[3]
        ]);
        continue;
      }

      // table (gfm)
      if (top && (cap = rules->table->split2(src))) {
        SRC_SUBSTR();

        mapping item = ([
          "type"   : "table",
          "header" : REGX(" *\\| *")->split_string(REGX("^ *| *\\| *$")
                                                   ->replace(cap[1], "")),
          "align"  : REGX(" *\\| *")->split_string(REGX("^ *|\\| *$")
                                                   ->replace(cap[2], "")),
          "cells"  : REGX("(?: *\\| *)?\\n$")->replace(cap[3], "") / "\n"
        ]);

        for (int i; i < sizeof(item->align); i++) {
          if (REGX("^ *-+: *$")->match(item->align[i])) {
            item->align[i] = "right";
          }
          else if (REGX("^ *:-+: *$")->match(item->align[i])) {
            item->align[i] = "center";
          }
          else if (REGX("^ *:-+ *$")->match(item->align[i])) {
            item->align[i] = "left";
          }
          else {
            item->align[i] = UNDEFINED;
          }
        }

        for (int i; i < sizeof(item->cells); i++) {
          item->cells[i] =
            REGX(" *\\| *")->split_string(
              REGX("^ *\\| *| *\\| *$")->replace(item->cells[i], ""));
        }

        PUSH_TOKEN(item);

        continue;
      }

      // top-level paragraph
      if (top && (cap = rules->paragraph->split2(src))) {
        SRC_SUBSTR();
        PUSH_TOKEN(([ "type" : "paragraph",
                      "text" : (cap[1][-1] == '\n')
                                ? cap[1][..<1]
                                : cap[1] ]));
        continue;
      }

      // text
      if (cap = rules->text->split2(src)) {
        // Top-level should never reach here.
        SRC_SUBSTR();
        PUSH_TOKEN(([ "type": "text", "text": cap[0] ]));

        continue;
      }

      if (sizeof(src)) {
        string tmp = src;

        if (sizeof(tmp) > 30) {
          tmp = tmp[..30] + "[...]";
        }

        tmp = "[...]" + tmp;
        error("'Infinite loop%s! ", tmp);
      }
    }

    return _tokens;
  }
}

//! Lexer used for inline text (eg bold text inside a paragraph).
class InlineLexer
{
  protected mapping options, rules, links;
  protected Renderer renderer;

  protected void create(mapping lnks, void|mapping opts)
  {
    options  = opts || default_options;
    links    = lnks;
    rules    = get_inline_rules()->normal;
    renderer = options->renderer || Renderer;

    if (!links) {
      error("Tokens array requires a `links` property! ");
    }

    if (options->gfm) {
      if (options->breaks) {
        rules = get_inline_rules()->breaks;
      }
      else {
        rules = get_inline_rules()->gfm;
      }
    }
    else if (options->pedantic) {
      rules = get_inline_rules()->pedantic;
    }
  }

  private bool in_link = false;

  //! Parse some inline Markdown and return the corresponding HTML.
  string output(string src)
  {
    String.Buffer buf = String.Buffer();
    function add = buf->add;
    string link, href, text;
    array cap;

    while (src && sizeof(src)) {
      // escape
      if (cap = rules->escape->split2(src)) {
        SRC_SUBSTR();
        add(cap[1]);
        continue;
      }

      // autolink
      if (cap = rules->autolink->split2(src)) {
        SRC_SUBSTR();

        if (cap[2] == "@") {
          text = cap[1][6] == ':'
                               ? mangle(cap[1][7..])
                               : mangle(cap[1]);
          href = mangle("mailto:") + text;
        }
        else {
          text = encode_html(cap[1]);
          href = text;
        }

        add(renderer->link(href, UNDEFINED, text, ([])));
        continue;
      }

      // url (gfm)
      if (!in_link && (cap = rules->url->split2(src))) {
        SRC_SUBSTR();
        text = encode_html(cap[1]);
        href = text;
        add(renderer->link(href, UNDEFINED, text, ([])));
        continue;
      }

      // tag
      if (cap = rules->tag->split2(src)) {
        if (!in_link && REGX("^<a ", RI)->match(cap[0])) {
          in_link = true;
        }
        else if (in_link && REGX("^<\\/a>", RI)->match(cap[0])) {
          in_link = false;
        }

        SRC_SUBSTR();
        add(options->sanitize ? options->sanitizer
                                ? options->sanitizer(cap[0])
                                : encode_html(cap[0])
                              : cap[0]);
        continue;
      }

      // link
      if (cap = rules->link->split2(src)) {
        SRC_SUBSTR();
        in_link = true;
        add(output_link(cap, ([ "href"  : cap[2],
                                "title" : cap[3] ])));
        in_link = false;
        continue;
      }

      // reflink, nolink
      if ((cap = rules->reflink->split2(src)) ||
          (cap = rules->nolink->split2(src)))
      {
        SRC_SUBSTR();
        link = sizeof(cap) > 2 && cap[2] || REGX("\\s+")->replace(cap[1], " ");
        mapping lnk = links[lower_case(link)];
        if (!lnk || !lnk->href) {
          add(cap[0][0..0]);
          src = cap[0][1..] + src;
          continue;
        }
        in_link = true;
        add(output_link(cap, lnk));
        in_link = false;
        continue;
      }

      // strong
      if (cap = rules->strong->split2(src)) {
        SRC_SUBSTR();
        add(renderer->strong(output(cap[2] || cap[1]), ([])));
        continue;
      }

      // em
      if (cap = rules->em->split2(src)) {
        SRC_SUBSTR();
        add(renderer->em(output(cap[2] || cap[1]), ([])));
        continue;
      }

      // code
      if (cap = rules->code->split2(src)) {
        SRC_SUBSTR();
        add(renderer->codespan(encode_html(cap[2], true), ([])));
        continue;
      }

      // br
      if (cap = rules->br->split2(src)) {
        SRC_SUBSTR();
        add(renderer->br(([])));
        continue;
      }

      // del (gfm)
      if (cap = rules->del->split2(src)) {
        SRC_SUBSTR();
        add(renderer->del(output(cap[1]), ([])));
        continue;
      }

      // text
      if (cap = rules->text->split2(src)) {
        SRC_SUBSTR();
        add(renderer->text(encode_html(smartypants(cap[0])), ([])));
        continue;
      }

      if (sizeof(src))
        error("Infinite loop! ");
    }

    return buf->get();
  }

  protected string smartypants(string text)
  {
    if (!options->smartypants) return text;

    // em-dashes, en-dashes
    text = replace(text, ([ "---" : "&mdash;", "--" : "&ndash;" ]));
    // opening singles
    text = REGX("(^|[-—/(\\[{\"\\s])'")->replace(text,
                                                lambda (string a, string b) {
                                                  return b + "&lsquo;";
                                                });
    // closing singles & apostrophes
    text = replace(text, "'", "&rsquo;");
    // opening doubles
    text = REGX("(^|[-—/(\\[{‘\\s])\"")->replace(text,
                                                 lambda (string a, string b) {
                                                   return b + "&ldquo;";
                                                 });
    // closing doubles
    text = replace(text, "\"", "&rdquo;");
    // ellipses
    text = REGX("\\.{3}")->replace(text, "&hellip;");

    return text;
  }

  protected string output_link(array cap, mapping link)
  {
    string href = encode_html(link->href);
    string title = link->title ? encode_html(link->title) : UNDEFINED;

    return cap[0][0] != '!'
                         ? renderer->link(href, title, output(cap[1]), ([]))
                         : renderer->image(href, title, encode_html(cap[1]), ([]));
  }

  protected string mangle(string s)
  {
    if (!options->mangle)
      return s;

    string out = "";
    int l = sizeof(s), i = 0;
    string ch;

    for (; i < l; i++) {
      ch = s[i..i];
      if (random(1.0) > 0.5 || s[i] == '@') {
        ch = sprintf("&#x%x;", s[i]);
      }
      out += ch;
    }

    return out;
  }
}

//!
class Renderer
{
  //! Markdown renderer. Subclass this to customize the output of @[parse].

  protected mapping options;

  protected void create(void|mapping opts)
  {
    options = opts;
  }

  //! Collect additional attributes from the token and render them as HTML
  //! attributes. Default attributes can be provided.
  string attrs(mapping token, mapping|void dflt)
  {
    mapping att = filter(dflt || ([]), stringp);
    foreach (token; string key; mixed val) {
      if (has_prefix(key, "attr_") && stringp(val)) {
        att[key[5..]] = val;
      }
    }
    return sprintf("%{ %s=%q%}", sort((array)att)); //TODO: Properly escape HTML
  }

  //!
  string code(string code, string lang, bool escaped, mapping token)
  {
    if (options->highlight) {
      string out = options->highlight(code, lang);
      if (out && out != code) {
        code = out;
        escaped = true;
      }
    }

    if (!lang) {
      return sprintf("<pre%s><code>%s%s</code></pre>", attrs(token),
                     !escaped ? encode_html(code, true) : code,
                     nl());
    }

    return sprintf(
      "<pre%s><code class='%s%s'>%s%s</code></pre>", attrs(token),
      options->lang_prefix,
      encode_html(lang),
      !escaped ? encode_html(code, true) : code,
      nl()
    );
  }

  //!
  string blockquote(string text, mapping token)
  {
    return sprintf("<blockquote%s>%s%s</blockquote>%[1]s", attrs(token), nl(), text);
  }

  //!
  string hr() { return options->xhtml ? "<hr/>" + nl() : "<hr>" + nl(); }

  //!
  string heading(string text, int level, string raw, mapping token)
  {
    string id = options->header_prefix + lower_case(REGX("[^\\w]+")->replace(raw, "-"));
    return "<h"
      + level
      + attrs(token, (["id": id]))
      + ">"
      + text
      + "</h"
      + level
      + ">" + nl();
  }

  //!
  string list(string body, void|bool ordered, mapping token)
  {
    return sprintf("<%s%s>%s%s</%[0]s>%[2]s", ordered ? "ol" : "ul", attrs(token), nl(), body);
  }

  //!
  string listitem(string text, mapping token)
  {
    return sprintf("<li%s>%s</li>%s", attrs(token), text, nl());
  }

  //!
  string paragraph(string text, mapping token)
  {
    return sprintf("<p%s>%s</p>%s", attrs(token), text, nl());
  }

  //!
  string table(string header, string body, mapping token)
  {
    return sprintf(
      "<table%s>%s<thead>%[1]s%s</thead>%[1]s"
      "<tbody>%[1]s%s</tbody>%[1]s</table>%[1]s",
      attrs(token), nl(), header, body
    );
  }

  //!
  string tablerow(string row, mapping token)
  {
    return sprintf("<tr%s>%s%s</tr>%[1]s", attrs(token), nl(), row);
  }

  //!
  string tablecell(string cell, mapping flags, mapping token)
  {
    mapping align = flags->align && (["style": "text-align:" + flags->align]);
    return sprintf(
      "<%s%s>%s</%[0]s>%s",
      flags->header ? "th" : "td",
      attrs(token, align),
      cell,
      nl()
    );
  }

  //!
  string html(string text, mapping token)  { return text; }
  string text(string t, mapping token)     { return t; }
  string strong(string t, mapping token)   { return sprintf("<strong%s>%s</strong>", attrs(token), t); }
  string em(string t, mapping token)       { return sprintf("<em%s>%s</em>", attrs(token), t); }
  string del(string t, mapping token)      { return sprintf("<del%s>%s</del>", attrs(token), t); }
  string codespan(string t, mapping token) { return sprintf("<code%s>%s</code>", attrs(token), t); }
  string br(mapping token)                 { return options->xhtml ? "<br/>" : "<br>"; }

  //!
  string link(string href, string|zero title, string text, mapping token)
  {
    if (options->sanitize) {
      mixed e = catch {
        string t = Protocols.HTTP.uri_decode(decode_html(href));
        t = REGX("[^\\w:]")->replace(t, "");
        if (has_prefix(t, "javascript:") || has_prefix(t, "vbscript:")) {
          return "";
        }
      };

      if (e) {
        return "";
      }
    }

    return sprintf(
      "<a%s>%s</a>",
      attrs(token, (["href": href, "title": title])),
      text
    );
  }

  //!
  string image(string url, string title, string text, mapping token)
  {
    string i = sprintf("<img src='%s' alt='%s'", url, text);
    if (text) i += sprintf(" title='%s'", text);
    return i + (options->xhtml ? "/>" : ">");
  }
}
