//
// This file needs to support old pikes that e.g. don't understand
// "#pike". Some of them fail when they see an unknown cpp directive.
//
// #pike __REAL_VERSION__
//

//! This module parses and tokenizes Pike source code.

protected constant splitter = Parser._parser._Pike.tokenize;
inherit "C.pmod";
