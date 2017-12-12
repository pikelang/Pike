#pike __REAL_VERSION__
#pragma strict_types

//! Lightweight IPv4 and IPv6 address type.

#define IPV4MAX	0xffffffff

constant is_inet = 1;

//!
int address;

//!
int masklen;

//!
protected void create(string ip) {
  masklen = -1;
  sscanf(ip, "%s/%d", ip, masklen);
  address = NetUtils.string_to_ip(ip);
  if (masklen < 0)
    masklen = 128;
  else if (!has_value(ip, ":"))
    masklen += 12 * 8;
}
variant protected void create(int ip, void|int masklen) {
  address = ip;
  this::masklen = zero_type(masklen) ? 16*8 : 12*8 + masklen;
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

//!
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
  switch (fmt) {
    case 'O': return sprintf("Inet(%s)", (string)this);
    case 's': return (string)this;
  }
  return sprintf(sprintf("%%*%c", fmt), params, 0);
}
