// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: AbstractParser.pike,v 1.5 2001/06/22 01:28:35 nilsson Exp $

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
