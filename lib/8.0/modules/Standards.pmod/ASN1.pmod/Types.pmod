#pike 8.1

#if 0
#define WERROR werror
#else
#define WERROR(x ...)
#endif

inherit Standards.ASN1.Types : pre;

protected class BaseMixin
{
  extern string der;
  void end_decode_constructed(int length) { }

  string(0..255) record_der(string(0..255) s)
  {
    return (der = s);
  }
}

class Object
{
  inherit pre::Object;
  inherit BaseMixin;
}

protected class CompoundMixin
{
  extern string _sprintf(int);
  string debug_string() {
    WERROR("asn1_compound[%s]->debug_string(), elements = %O\n",
	   type_name, elements);
    return _sprintf('O');
  }
}

class Compound
{
  inherit pre::Compound;
  inherit BaseMixin;
  inherit CompoundMixin;
}

protected class StringMixin
{
  extern string _sprintf(int);
  string debug_string() {
    WERROR("asn1_string[%s]->debug_string(), value = %O\n", type_name, value);
    return _sprintf('O');
  }
}

class String
{
  inherit pre::String;
  inherit BaseMixin;
  inherit StringMixin;
}

class Boolean
{
  inherit pre::String;
  inherit BaseMixin;

  string debug_string() {
    return value ? "TRUE" : "FALSE";
  }
}

protected class IntegerMixin
{
  extern Gmp.mpz value;
  string debug_string() {
    return sprintf("INTEGER (%d) %s", value->size(), value->digits());
  }
}

class Integer
{
  inherit pre::Integer;
  inherit BaseMixin;
  inherit IntegerMixin;
}

class Enumerated
{
  inherit pre::Enumerated;
  inherit BaseMixin;
  inherit IntegerMixin;
}

class Real
{
  inherit pre::Real;
  inherit BaseMixin;

  string debug_string()
  {
    return sprintf("REAL %O", value);
  }
}

class BitString
{
  inherit pre::BitString;
  inherit BaseMixin;

  string debug_string() {
    return sprintf("BIT STRING (%d) %s",
		   sizeof(value||"") * 8 - unused,
		   ([object(Gmp.mpz)](Gmp.mpz(value||"", 256) >> unused))
		   ->digits(2));
  }
}

class OctetString
{
  inherit pre::OctetString;
  inherit BaseMixin;
  inherit StringMixin;
}

class Null
{
  inherit pre::Null;
  inherit BaseMixin;

  string debug_string() { return "NULL"; }
}

class Identifier
{
  inherit pre::Identifier;
  inherit BaseMixin;

  string debug_string() {
    if(!id) return "IDENTIFIER 0";
    return "IDENTIFIER " + (array(string)) id * ".";
  }
}

class UTF8String
{
  inherit pre::UTF8String;
  inherit BaseMixin;
  inherit StringMixin;
}

class Sequence
{
  inherit pre::Sequence;
  inherit BaseMixin;
  inherit CompoundMixin;
}

class Set
{
  inherit pre::Set;
  inherit BaseMixin;
  inherit CompoundMixin;
}

class PrintableString
{
  inherit pre::PrintableString;
  inherit BaseMixin;
  inherit StringMixin;
}

class BrokenTeletexString
{
  inherit pre::BrokenTeletexString;
  inherit BaseMixin;
  inherit StringMixin;
}

//! @class TeletexString
//!
//! This used to be a string encoded with the T.61 character coding,
//! and has been obsolete since the early 1990:s.
//!
//! @seealso
//!   @[BrokenTeletexString]

//! @decl inherit BrokenTeletexString

//! @ignore
program(BrokenTeletexString) TeletexString = BrokenTeletexString;
//! @endignore

//! @endclass

class IA5String
{
  inherit pre::IA5String;
  inherit BaseMixin;
  inherit StringMixin;
}

class VisibleString
{
  inherit pre::VisibleString;
  inherit BaseMixin;
  inherit StringMixin;
}

class UTC
{
  inherit pre::UTC;
  inherit BaseMixin;
  inherit StringMixin;
}

class GeneralizedTime
{
  inherit pre::GeneralizedTime;
  inherit BaseMixin;
  inherit StringMixin;
}

class UniversalString
{
  inherit pre::UniversalString;
  inherit BaseMixin;
  inherit StringMixin;
}

class BMPString
{
  inherit pre::BMPString;
  inherit BaseMixin;
  inherit StringMixin;
}

class MetaExplicit
{
  inherit pre::MetaExplicit : scary;

  class `()
  {
    inherit scary::`();
    inherit BaseMixin;
    inherit CompoundMixin;

    string debug_string() {
      return type_name + "[" + (int) real_tag + "]";
    }
  }
}
