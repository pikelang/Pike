/*
 * Glue for the ODBC-module
 */

#pike __REAL_VERSION__
#require constant(Odbc.odbc)

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
constant dont_dump_program = 1;

inherit Odbc.odbc;

int|object big_query(object|string q, mapping(string|int:mixed)|void bindings)
{  
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q, bindings, this));
}

int|object big_typed_query(object|string q,
			   mapping(string|int:mixed)|void bindings)
{
  if (!bindings)
    return ::big_typed_query(q);
  return ::big_typed_query(.sql_util.emulate_bindings(q, bindings, this));
}

constant list_dbs = Odbc.list_dbs;

//!
class typed_result
{
  inherit ::this_program;

  //! Value to use to represent NULL.
  mixed _null_value = Val.null;

  //! Helper function that scales @[mantissa] by a
  //! factor @expr{10->pow(scale)@}.
  //!
  //! @returns
  //!   Returns an @[Gmp.mpq] object if @[scale] is negative,
  //!   and otherwise an integer (bignum).
  object(Gmp.mpq)|int scale_numeric(int mantissa, int scale)
  {
    if (!scale) return mantissa;
    if (scale > 0) {
      return mantissa * 10->pow(scale);
    }
    return Gmp.mpq(mantissa, 10->pow(-scale));
  }
}
