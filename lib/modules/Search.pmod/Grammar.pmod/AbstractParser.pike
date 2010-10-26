// $Id$
#pike __REAL_VERSION__

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
