// $Id$

#pike 7.5

class Regexp
{
  inherit global.Regexp.SimpleRegexp;

#if constant (_Regexp_PCRE._pcre)
  // A bit ugly way to provide access to Regexp.PCRE, but it's less
  // intrusive than the method used in later versions.
  constant PCRE = _Regexp_PCRE;
#endif
}

constant _module_value = Regexp;
