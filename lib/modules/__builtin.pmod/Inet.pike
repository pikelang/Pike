#pike __REAL_VERSION__
#pragma strict_types

//! Lightweight IPv4 and IPv6 address type.

constant is_inet = 1;

//!
int address;

//!
int masklen;

//!
protected void create(string ip) {
  int ip0, ip1, ip2, ip3, m1, m2, m3;
  string mask;
  masklen = 32;
  switch (sscanf(ip, "%d.%d.%d.%d/%s", ip0, ip1, ip2, ip3, mask)) {
    default:
      masklen = 16*8;
      sscanf(ip, "%s/%d", ip, masklen);
      address = 0;
      foreach (Protocols.IPv6.parse_addr(ip); ; int val)
        address = address << 16 | val;
      break;
    case 5:
      switch (sscanf(mask, "%d.%d.%d.%d", masklen, m1, m2, m3)) {
        default:
          error("Unparseable IP %O\n", ip);
        case 4:
          m1 = ((masklen << 8 | m1) << 8 | m2) << 8 | m3;
          masklen = 32;
          while (!(m1 & 1) && --masklen)
            m1 >>= 1;
        case 1:
          break;
      }
    case 4:
      masklen += 12*8;
      address = ((ip0 << 8 | ip1) << 8 | ip2) << 8 | ip3;
  }
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
    case "string": {
      array(int(16bit)) split = ({});
      int addr = address;
      do
        split += ({ addr & 0xffff });
      while ((addr >>= 16) && sizeof(split) < 16);
      if (sizeof(split) <= 2) {
        if (sizeof(split) == 1)
          split += ({0});
        return sprintf("%d.%d.%d.%d/%d", split[1] >> 8, split[1] & 255,
         split[0] >> 8, split[0] & 255, masklen - 12*8);
      } else {
        while (sizeof(split) < 8)
          split += ({0});
        split = reverse(split);
        return Protocols.IPv6.format_addr_short(split)
         + sprintf("/%d", masklen);
      }
    }
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
