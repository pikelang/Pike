// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: AbstractParser.pike,v 1.7 2004/08/07 15:27:00 js Exp $

//! @param options
//!   @mapping
//!     @member string "implicit"
//!       Either of the strings: "and", "or".
//!       If not supplied, default to "or".
//!     @member multiset(string) "fields"
//!       The words that should be recognized as fields.
//!       If not supplied, it should default to
//!       @tt{ Search.Grammar.getDefaultFields() @}
//!   @endmapping
static void create(void|mapping(string:mixed) options) {}

Search.Grammar.ParseNode parse(string query) {}
