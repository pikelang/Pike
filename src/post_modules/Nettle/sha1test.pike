
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
    ADT.BitBuffer buf = ADT.BitBuffer();
    for(int i=2; i<sizeof(m); i++) {
      if(b)
	buf->put1(m[i]);
      else
	buf->put0(m[i]);
      b = !b;
    }
    return buf->drain();
  }

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
      if(o_s!=m->section+"/"+h->section)
	o_s = m->section+"/"+h->section;
      string msg = m->get_message();
      if(!msg) break;
      tests++;
      if( Crypto.SHA1->hash(msg)!=h->get_hash() ) {
	werror("Fail %d %d\n", h->row, m->row);
	fails++;
      }
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
