
//! Implements a FIFO bit buffer, i.e. a buffer that operates on bits
//! instead of bytes. It is not designed for performance, but as a way
//! to handle complicated file formats and other standards where you
//! may need to work on unaligned data units of sub byte size, without
//! having to fry your brain while keeping track of all the bits
//! yourself.
//!
//! @example
//!   > ADT.BitBuffer b=ADT.BitBuffer();
//!   > b->put1(2);
//!   (1) Result: ADT.BitBuffer(11)
//!   > b->put0(15);
//!   (2) Result: ADT.BitBuffer("\300\0"0)
//!   > b->drain();
//!   (3) Result: "\300\0"
//!   > sizeof(b);
//!   (4) Result: 1


#pike __REAL_VERSION__

protected string(8bit) data = "";
protected int dptr;

//! The buffer can be initialized with initial data during creation.
protected void create(void|string(8bit) data) {
  if(data) feed(data);
}

//! Adds full bytes to the buffer.
this_program feed( string(8bit) x ) {
  if(String.width(x)!=8) error("Only eight bits wide characters allowed.\n");
  if(bob)
    foreach(x;; int c)
      put(c,8);
  else
  {
      if(dptr) {
          data = data[dptr..];
          dptr = 0;
      }
      data += x;
  }
  return this;
}


//! Drains the buffer of all full (8-bits wide) bytes.
string(8bit) drain() {
  if(_sizeof()<8)
    return "";
  string(8bit) d;
  if(bib+bob==0)
    d = data;
  else {
    String.Buffer b = String.Buffer(sizeof(data)+1);
    while(_sizeof()>=8)
      b->putchar(get(8));
    d = b->get();
  }
  data = "";
  dptr = 0;
  return d;
}

protected int out_buffer, bib;

#define pow2(X) (1<<(X))

//! Get @[bits] from the buffer.
//!
//! @throws
//!   Throws an error in case of data underflow.
//!
//! @note
//!   The bits are extracted with the most significant bit first.
int get( int bits ) {

  while( bib<bits && sizeof(data)-dptr ) {
    out_buffer = (out_buffer<<8) | data[dptr];
    dptr++;
    bib+=8; // Eight bit strings.
  }

  if(bib<bits) {
    int n = bits-bib; // We need n bits more.
    if(bob<n) error("BitBuffer undeflow.\n");
    out_buffer = (out_buffer<<n) | (in_buffer>>(bob-n));
    bib += n;
    in_buffer &= pow2(bob-n)-1;
    bob -= n;
  }

  int res = out_buffer>>(bib-bits);
  out_buffer &= pow2(bib-bits)-1;
  bib-=bits;

  return res;
}

//! Reads @[bytes] (or less) bytes from the buffer and returns as
//! string.
string(8bit) read( void|int bytes )
{
  if( undefinedp(bytes) )
    bytes = _sizeof()/8;
  else
    bytes = min(bytes, _sizeof()/8);
  String.Buffer buf = String.Buffer(bytes);
  while(bytes--)
    buf->putchar( get(8) );
  return (string(8bit))buf;
}

protected int in_buffer, bob;

//! Put @[bits] number of bits with the value @[value] into the
//! buffer.
//!
//! @note
//!   @[value] must not be larger than what can be stored with the
//!   number of bits given in @[bits].
//!
//! @note
//!   The bits are added to the buffer with the most significant
//!   bit first.
this_program put( int value, int bits ) {
  //  werror("\n%O %O %O\n", bob, bib, bits);
  if(dptr) {
    data = data[dptr..];
    dptr = 0;
  }
  if(!bits) return this;
  bob += bits;
  in_buffer <<= bits;
  in_buffer |= value;
  while( bob >= 8 ) {
    data += sprintf("%c", in_buffer>>(bob-8));
    bob -= 8;
    in_buffer &= ((1<<bob)-1);
  }
  return this;
}

//! Put @[bits] number of 0 bits into the buffer.
this_program put0( int bits ) {

  if(bits<8) {
    put(0, bits);
    return this;
  }

  // Fill to boundery
  int t = 8-bob;
  put(0, t);
  bits -= t;
  if(!bits) return this;

  // Add full bytes.
  feed( "\0"*(bits/8) );
  bits %= 8;
  if(!bits) return this;

  // Add trailing bits.
  put(0, bits);
  return this;
}

//! Put @[bits] number of 1 bits into the buffer.
this_program put1( int bits ) {

  if(bits<8) {
    put( (1<<bits)-1, bits );
    return this;
  }

  // Fill to boundery
  int t = 8-bob;
  put( (1<<t)-1, t);
  bits -= t;
  if(!bits) return this;

  // Add full bytes.
  feed( "\xff"*(bits/8) );
  bits %= 8;
  if(!bits) return this;

  // Add trailing bits.
  put( (1<<bits)-1, bits );
  return this;
}

//! @[sizeof()] will return the number of bits in the buffer.
protected int _sizeof() {
  return sizeof(data)*8+bib+bob-dptr*8;
}

protected string _sprintf(int t) {
  if(t!='O') return UNDEFINED;
  string ret = sprintf("%O(", this_program);
  if(bib) ret += reverse(sprintf("%0"+bib+"b", out_buffer));
  if(sizeof(data)-dptr) ret += sprintf("%O", data[dptr..]);
  if(bob) ret += sprintf("%0"+bob+"b", in_buffer);
  return ret + ")";
}
