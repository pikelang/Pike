//
// $Id: BitBuffer.pike,v 1.2 2004/02/24 07:46:36 nilsson Exp $

//! Implements a FIFO bit buffer, i.e. a buffer that operates on bits
//! instead of bytes. It is not designed for performance, but as a way
//! to handle complicated file formats and other standards where you
//! may need to work on unaligned data units of sub byte size, without
//! having to fry your brain while keeping track of all the bits
//! yourself.


#pike __REAL_VERSION__

static string data = "";
static int dptr;

//! The buffer can be initialized with initial data during creation.
void create(void|string _data) {
  if(_data) feed(_data);
}

//! Adds full bytes to the buffer.
this_program feed( string x ) {
  if(String.width(x)!=8) error("Only eight bits wide characters allowed.\n");
  if(bob)
    foreach(x; int p; int c)
      put(c,8);
  else
    data += x;
  return this;
}

//! Drains the buffer of all full (8-bits wide) bytes.
string drain() {
  if(_sizeof()<8)
    return "";
  string d;
  if(bib+bob==0)
    d = data;
  else {
    String.Buffer b = String.Buffer(sizeof(data)+1);
    while(_sizeof()>8)
      b->putchar(get(8));
    d = b->get();
  }
  data = "";
  dptr = 0;
  return d;
}

static int out_buffer, bib;

//! Get @[bits] from the buffer.
//! @note
//!   No checks are made to see if there are enough bits in the
//!   buffer.
int get( int bits ) {
  if(sizeof(data))
    while( bib < bits ) {
      out_buffer = (out_buffer<<8) | data[dptr];
      dptr++;
      bib+=8; // Eight bit strings.
    }
  else if(bib<bits) {
    int n = bits-bib; // We need n bits more.
    out_buffer = (out_buffer<<n) | (in_buffer>>(bob-n));
    bib += n;
    in_buffer &= pow(2,bob-n)-1;
    bob -= n;
  }

  bib-=bits;
  int res = out_buffer & ((1<<bits)-1);
  out_buffer>>=bits;

  if(dptr>64) {
    data = data[dptr..];
    dptr = 0;
  }
  return res;
}

static int in_buffer, bob;

//! Put @[bits] number of bits with the value @[value] into the
//! buffer.
this_program put( int value, int bits ) {
  //  werror("\n%O %O %O\n", bob, bib, bits);
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
int _sizeof() {
  return sizeof(data)*8+bib+bob-dptr*8;
}
