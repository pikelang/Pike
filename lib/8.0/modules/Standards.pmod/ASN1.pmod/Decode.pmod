#pike 8.1

#if 0
#define WERROR werror
#else
#define WERROR(x ...)
#endif

inherit Standards.ASN1.Decode : pre;

class constructed
{
  inherit Constructed;

  void end_decode_constructed(int length) { }

  string(0..255) record_der(string(0..255) s)
  {
    return (der = s);
  }

  string debug_string() {
    WERROR("asn1_compound[%s]->debug_string(), elements = %O\n",
	   type_name, elements);
    return _sprintf('O');
  }

  protected void create(int tag_n_cls, string(8bit) raw,
                        array(.Types.Object) elements)
  {
    ::create(tag_n_cls >> 6, tag_n_cls & 0x1f, raw, elements);
  }
}

class primitive
{
  inherit Primitive;

  void end_decode_constructed(int length) { }

  string(0..255) record_der(string(0..255) s)
  {
    return (der = s);
  }

  string debug_string() {
    return sprintf("primitive(%d)", get_combined_tag());
  }

  protected void create(int tag_n_cls, string(8bit) raw)
  {
    ::create(tag_n_cls >> 6, tag_n_cls & 0x1f, raw);
  }
}
