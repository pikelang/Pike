/*
 * $Id: sql_util.pmod,v 1.13 2005/04/10 03:38:54 nilsson Exp $
 *
 * Some SQL utility functions.
 * They are kept here to avoid circular references.
 *
 * Henrik Grubbström 1999-07-01
 */

#pike __REAL_VERSION__

//! Some SQL utility functions

//! Quote a string so that it can safely be put in a query.
//!
//! @param s
//!   String to quote.
string quote(string s)
{
  return replace(s, "\'", "\'\'");
}

//! Throw an error in case an unimplemented function is called.
void fallback()
{
  error( "Function not supported in this database." );
}

//! Build a raw SQL query, given the cooked query and the variable bindings
//! It's meant to be used as an emulation engine for those drivers not
//! providing such a behaviour directly (i.e. Oracle).
//! The raw query can contain some variables (identified by prefixing
//! a colon to a name or a number (i.e. ":var" or  ":2"). They will be
//! replaced by the corresponding value in the mapping.
//!
//! @param query
//!   The query.
//!
//! @param bindings
//!   Optional mapping containing the variable bindings. Make sure that
//!   no confusion is possible in the query. If necessary, change the
//!   variables' names.
string emulate_bindings(string query, mapping(string|int:mixed)|void bindings,
                        void|object driver)
{
  array(string)k, v;
  if (!bindings)
    return query;
  function my_quote=(driver&&driver->quote?driver->quote:quote);
  v=map(values(bindings),
	lambda(mixed m) {
	  if(multisetp(m)) m = indices(m)[0];
	  return (stringp(m)? "'"+my_quote(m)+"'" : (string)m);
	});
  // Throws if mapping key is empty string.
  k=map(indices(bindings),lambda(string s){
			    return ( (stringp(s)&&s[0]==':') ?
				     s : ":"+s);
			  });
  return replace(query,k,v);
}
