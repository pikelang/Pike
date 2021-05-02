#pike __REAL_VERSION__
#pragma strict_types

//!   Lightweight IPv4 and IPv6 address type that stores the netmask
//!   and allows simple arithmetic and comparison operators.

#define IPV4MAX	0xffffffff

constant is_inet = 1;

//! IP address converted to an integer.
//! IPv4 addresses will be 32-bit or less.
int address;

//! Defines the netmask.  For both IPv4 and IPv6 addresses,
//! @[masklen] will be 128 in case of full addresses.
int masklen;

array(int) _encode() {
  return ({address, masklen});
}

void _decode(array(int) x) {
  address = x[0];
  masklen = x[1];
}

protected int __hash() {
  return address;
}

//! @param ip
//!   A string defining an IPv4 or IPv6 address
//!   with an optional @expr{masklen@} attached.
//!   If the address is in IPv6 notation, the range
//!   of the @expr{masklen@} is expected to be
//!   between @expr{0@} and @expr{128@}.
//!   If the address is in IPv4 notation, the range
//!   of the @expr{masklen@} is expected to be
//!   between @expr{0@} and @expr{32@}.
variant protected void create(string ip) {
  masklen = -1;
  sscanf(ip, "%s/%d", ip, masklen);
  address = NetUtils.string_to_ip(ip);
  if (masklen < 0)
    masklen = 128;
  else if (!has_value(ip, ":"))
    masklen += 12 * 8;
}

//! @param ip
//!   An IPv4 or IPv6 ip address.
//! @param masklen
//!   Defines the netmask, always in the range between @expr{0@}
//!   and @expr{128@}; i.e. for IPv4 addresses you should
//!   add @expr{12 * 8@} to the actual IPv4-masklen.
variant protected void create(int ip, void|int masklen) {
  address = ip;
  this::masklen = zero_type(masklen) ? 16*8 : masklen;
}
variant protected void create() {
}

protected mixed `+(mixed that) {
  if (!intp(that))
    error("Cannot add %O\n", that);
  this_program n = this_program();
  n->masklen = masklen;
  n->address = address + [int]that;
  return n;
}

protected mixed `-(mixed that) {
  if (intp(that))
    return this + -[int]that;
  else if (objectp(that) && ([object]that)->is_inet)
    return this->address - [int]([object]that)->address;
  error("Cannot add %O\n", that);
}

protected mixed `~() {
  this_program n = this_program();
  n->masklen = masklen;
  n->address = ~address;
  return n;
}

protected mixed `|(mixed that) {
  this_program n = this_program();
  n->masklen = max(masklen, [int]([object]that)->masklen);
  n->address = address | [int]([object]that)->address;
  return n;
}

protected mixed `&(mixed that) {
  this_program n = this_program();
  n->masklen = min(masklen, [int]([object]that)->masklen);
  n->address = address & [int]([object]that)->address;
  return n;
}

protected int(0..1) `<(mixed that) {
  return objectp(that) ? address < [int]([object]that)->address
   : intp(that) && address < [int]that;
}

protected int(0..1) `>(mixed that) {
  return objectp(that) ? address > [int]([object]that)->address
   : intp(that) && address > [int]that;
}

protected int(0..1) `==(mixed that) {
  return objectp(that)
   ? address == [int]([object]that)->address
      && masklen == [int]([object]that)->masklen
   : address == [int]that;
}

private int(0..1) containsorequal(int thataddress, int lmask) {
  lmask = 128 - lmask;
  lmask = ~0 << [int(0..)] lmask;
  return (address & lmask) == (thataddress & lmask);
}

//! Is contained by.
protected int(0..1) `<<(mixed that) {
  if (objectp(that)) {
    int lmask = [int]([object]that)->masklen;
    if (masklen > lmask)
      return containsorequal([int]([object]that)->address, lmask);
  }
  return 0;
}

//! Contains.
protected int(0..1) `>>(mixed that) {
  if (objectp(that)) {
    if (masklen < [int]([object]that)->masklen)
      return containsorequal([int]([object]that)->address, masklen);
  } else if (intp(that) && masklen < 128)
    return containsorequal([int]that, masklen);
  return 0;
}

public string encode_json() {
  return sprintf("\"%s\"", this);
}

protected mixed cast(string to) {
  switch (to) {
    case "string":
      return NetUtils.ip_to_string(address)
       + (masklen == 128 ? "" :
                    sprintf("/%d", masklen - (address <= IPV4MAX && 12*8)));
    case "int":
      return address;
    default:
      return UNDEFINED;
  }
}

protected string _sprintf(int fmt, mapping(string:mixed) params) {
  if (!this)					// Not in destructed objects
    return "(destructed)";
  switch (fmt) {
    case 'O': return sprintf("Inet(%s)", (string)this);
    case 's': return (string)this;
  }
  return sprintf(sprintf("%%*%c", fmt), params, 0);
}
