#require Regexp.PCRE

//! Markdown module.
//!
//! For a description on Markdown, go to the web page of the inventor of
//! Markdown @url{https://daringfireball.net/projects/markdown/@}.

//! Convert Markdown text @[md] to HTML via the @[Parser.Markdown.Marked]
//! module. For available options see @[Parser.Markdown.Marked.parse()].
string marked(string md, void|mapping options)
{
  return .Marked.parse(md, options);
}
