

class BitBuffer {
  static string data;
  static int out_buffer;
  static int bib, dptr;

  void create(void|string _data) {
    data = _data || "";
  }

  string feed( string x ) {
    data = data[dptr..]+x;
    dptr=0;
  }

  string drain() {
    string d = data;
    data = "";
    dptr = 0;
    return d;
  }

  int get( int bits ) {
    while( bib < bits ) {
      out_buffer = (out_buffer<<8) | data[dptr];
      dptr++;
      bib+=8; // Utgår från att strängarna är 8bittars.
    }
    bib-=bits;
    int res = out_buffer & ((1<<bits)-1);
    out_buffer>>=bits;
    return res;
  }

  static int in_buffer, bob;
  void put( int value, int bits ) {
    bob += bits;
    in_buffer <<= bits;
    in_buffer |= value;
    while( bob >= 8 ) {
      data += sprintf("%c", in_buffer>>(bob-8));
      bob -= 8;
      in_buffer &= ((1<<bob)-1);
    }
  }

  void put0( int bits ) {

    if(bits<8) {
      put(0, bits);
      return;
    }

    // Fill to boundery
    int t = 8-bob;
    put(0, t);
    bits -= t;
    if(!bits) return;

    // Add full bytes.
    feed( "\0"*(bits/8) );
    bits %= 8;
    if(!bits) return;

    // Add trailing bits.
    put(0, bits);
  }

  void put1( int bits ) {

    if(bits<8) {
      put( (1<<bits)-1, bits );
      return;
    }

    // Fill to boundery
    int t = 8-bob;
    put( (1<<t)-1, t);
    bits -= t;
    if(!bits) return;

    // Add full bytes.
    feed( "\xff"*(bits/8) );
    bits %= 8;
    if(!bits) return;

    // Add trailing bits.
    put( (1<<bits)-1, bits );
  }
}

class File {

  int row;
  array rows;
  string section;

  void create(string h) {
    rows = h/"\n";
    for(row; row<sizeof(rows); row++) {
      string r = rows[row];
      if(!sizeof(r) || r[0]=='#') continue;
      if(r[0]=='H') {
	sscanf(r, "H>%s<H", section);
	break;
      }
      error("Unknown directive.\n");
    }
    if(!section)
      error("Could not parse section.\n");
  }

  int in_D;

  string get_row() {
    if(!in_D)
      for(row; row<sizeof(rows); row++) {
	string r = rows[row];
	if(!sizeof(r) || r[0]=='#') continue;
	if(r[0]=='H') {
	  sscanf(r, "H>%s<H", section);
	  continue;
	}
	if(has_prefix(r, "D>")) break;
	if(has_prefix(r, "<D")) continue;
	error("Unknown directive %O.\n", r);
      }
    in_D = 1;

    row++;
    if(row>=sizeof(rows)) return 0;
    string r = rows[row];
    if(has_prefix(r, "<D")) {
      in_D = 0;
      return get_row();
    }
    string hash;
    while(!sscanf(r, "%s ^", hash))
      r += rows[++row];
    return hash;
  }
}

class Hashes {
  inherit File;

  string get_hash() {
    string row = get_row();
    if(!row) return 0;
    return String.hex2string(row);
  }
}

class Messages {
  inherit File;

  string _get_message() {
    string row = get_row();
    if(!row) return 0;
    array m = row/" ";
    m = (array(int))m;
    if(sizeof(m)!=m[0]+2) error("Error in format.\n");
    int b = m[1];
    if( !(<1,0>)[b] ) error("Error in format.\n");
    object buf = BitBuffer();
    for(int i=2; i<sizeof(m); i++) {
      //      for(int j; j<m[i]; j++)
      //	buf->put(b,1);
      if(b)
	buf->put1(m[i]);
      else
	buf->put0(m[i]);
      b = !b;
    }
    return buf->drain();
  }

  int rnd;
  string seed;

  string get_message() {
    string m = _get_message();
    if(section=="SHS Type 3 Strings") { // Skip the pseudorandom test.
      return 0;
    }
    return m;
  }
}

class Test {

  Hashes h;
  Messages m;

  void create(string msg, string hsh) {
    msg -= "\r";
    hsh -= "\r";
    m = Messages(msg);
    h = Hashes(hsh);
  }

  array(int) test() {
    int tests, fails;
    string o_s;
    while(1) {
      if(o_s!=m->section+"/"+h->section) {
	o_s = m->section+"/"+h->section;
	//	werror("\n%O\n", o_s);
      }
      string msg = m->get_message();
      if(!msg) break;
      tests++;
      if( Crypto.SHA1->hash(msg)!=h->get_hash() ) {
	werror("Fail %d %d\n", h->row, m->row);
	fails++;
      }
      //      werror(".");
    }
    return ({ tests, fails });
  }
}


void main() {
  Test t = Test(#string "byte-messages.sha1",
		#string "byte-hashes.sha1");
  array res = t->test();
  write("%d %d", res[0], res[1]);
  return 0;
}
