// This file is part of Roxen Search
// Copyright © 2001 - 2009, Roxen IS. All rights reserved.
//
// $Id: AbstractParser.pike,v 1.8 2009/05/25 18:26:52 mast Exp $

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
