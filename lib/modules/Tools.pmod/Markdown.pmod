#charset utf-8
#pike __REAL_VERSION__
#require constant(Parser.Markdown)

//! This is a small stub entrypoint to @[Parser.Markdown] - it is exactly
//! equivalent to calling @[Parser.Markdown.parse]().

// TODO: Should this be marked as deprecated?
constant parse = Parser.Markdown.parse;
