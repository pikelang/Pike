/* decode.pike
 *
 * Rudimentary support for decoding ASN.1 encoded data.
 *
 * $Id: decode.pike,v 1.2 1998/04/21 21:20:35 nisse Exp $
 */

/* BER/DER decoder
 *
 * Values are represented as arrays ({ tag, value }).
 * Tag is either an integer tag number, or a string, in case
 * the tag recognized.
 *
 * Values are strings, integers, or arrays */

inherit ADT.struct;

array get_asn1()
{
  int|string tag = get_uint(1);
  int len;
  string contents;
  mixed value;
    
#ifdef SSL3_DEBUG
  werror(sprintf("decoding tag %x\n", tag));
#endif
  if ( (tag & 0x1f) == 0x1f)
    throw( ({ "high tag numbers is not supported\n", backtrace() }) );
  int len = get_uint(1);
  if (len & 0x80)
    len = get_uint(len & 0x7f);
    
#ifdef SSL3_DEBUG
  werror(sprintf("len : %d\n", len));
#endif
  contents = get_fix_string(len);
#ifdef SSL3_DEBUG
  werror(sprintf("contents: %O\n", contents));
#endif
  value = contents; /* Default is no conversion */
  if (tag & 0x20)
  {
    object seq = object_program(this_object())(contents);
    value = ({ });
    while(! seq->is_empty())
    {
      array elem = seq->get_asn1();
#ifdef SSL3_DEBUG
      // werror(sprintf("elem: %O\n", elem));
#endif
      value += ({ elem });
    }
  }
  switch(tag & 0xdf)
  {
  case 1: /* Boolean */
    if (strlen(contents) != 1)
      throw( ({ "SSL.asn1: Invalid boolean value.\n", backtrace() }) );
    tag = "BOOLEAN";
    value = !!contents[0];
    break;
  case 2: /* Integer */
    tag = "INTEGER";
    value = Gmp.mpz(contents, 256);
    if (contents[0] & 0x80)  /* Negative */
      value -= Gmp.pow(256, strlen(contents));
    break;
  case 3: /* Bit string */
    tag = "BIT STRING";
    break;
  case 4: /* Octet string */
    tag = "OCTET STRING";
    break;
  case 5: /* Null */
    if (strlen(contents))
      throw( ({ "SSL.asn1: Invalid NULL value.\n", backtrace() }) );
    tag = "NULL";
    value = 0;
    break;
  case 6: /* Object id */
  {
    tag = "Identifier";
    if (contents[0] < 120)
      value = ({ contents[0] / 40, contents[0] % 40 });
    else
      value = ({ 2, contents[0] - 80 });
    int index = 1;
    while(index < strlen(contents))
    {
      int id = 0;
      do
      {
	id = id << 7 | (contents[index] & 0x7f);
      } while(contents[index++] & 0x80);
      value += ({ id });
    }
    break;
  }
  case 9: /* Real */
    tag = "REAL";
    break;
  case 10: /* Enumerated */
    tag = "ENUMERATED";
    break;
  case 16: /* Sequence */
    tag = "SEQUENCE";
    break;
  case 17: /* Set */
    tag = "SET";
    break;
  default: /* Keep numeric tag */
    break;
  }
      
  return ({ tag, value });
}
