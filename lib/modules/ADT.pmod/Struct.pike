//
// Struct ADT
// By Martin Nilsson
//

#pike __REAL_VERSION__

//! Implements a struct which can be used for serialization and
//! deserialization of data.
//! @example
//!   class ID3 {
//!     inherit ADT.Struct;
//!     Item head = Chars(3);
//!     Item title = Chars(30);
//!     Item artist = Chars(30);
//!     Item album = Chars(30);
//!     Item year = Chars(4);
//!     Item comment = Chars(30);
//!     Item genre = Byte();
//!   }
//!
//!   Stdio.File f = Stdio.File("foo.mp3");
//!   f->seek(-128);
//!   ADT.Struct tag = ID3(f);
//!   if(tag->head=="TAG") {
//!     write("Title: %s\n", tag->title);
//!     tag->title = "A new title" + "\0"*19;
//!     f->seek(-128);
//!     f->write( (string)tag );
//!   }
//!
//! @example
//!   class HollerithString {
//!     inherit ADT.Struct;
//!     Item strlen = Word();
//!     Item str = Chars(strlen);
//!   }

protected local array(Item) items = ({});
protected local mapping(string:Item) names = ([]);

constant is_struct = 1;
constant is_item = 1;
int id = ADT.get_item_id();

//! @decl void create(void|string|Stdio.File data)
//! @param data
//!   Data to be decoded and populate the struct. Can
//!   either be a file object or a string.
optional protected void create(void|string|object file) {
  foreach(::_indices(this, 0), string index) {
    mixed val = ::`[](index, this, 0);
    if(objectp(val) && val->is_item) names[index]=val;
  }
  items = values(names);
  sort(items->id, items);

  if(file) decode(file);
}

//! @decl void decode(string|Stdio.File data)
//! Decodes @[data] according to the struct and populates
//! the struct variables. The @[data] can either be a file
//! object or a string.
void decode(string|object file) {
  if(stringp(file)) file = Stdio.FakeFile(file);
  items->decode(file);
}

//! Serializes the struct into a string. This string is equal
//! to the string fed to @[decode] if nothing in the struct
//! has been altered.
string encode() {
  return items->encode()*"";
}


// --- LFUN overloading.

//! @decl protected mixed `[](string item)
//! @decl protected mixed `->(string item)
//! The struct can be indexed by item name to get the
//! associated value.

//! @decl protected mixed `[]=(string item)
//! @decl protected mixed `->=(string item)
//! It is possible to assign a new value to a struct
//! item by indexing it by name and assign a value.

protected mixed `[](string id) {
  if(names[id]) return names[id]->get();
  return ::`[](id, this, 0);
}

this_program get() { return this; }

protected mixed `[]=(string id, mixed value) {
  if(names[id]) names[id]->set(value);
  return id;
}

protected function `-> = `[];
protected function `->= = `[]=;

//! The indices of a struct is the name of the struct items.
protected array(string) _indices() {
  array ret = indices(names);
  sort(values(names)->id, ret);
  return ret;
}

//! The values of a struct is the values of the struct items.
protected array _values() {
  return items->get();
}

//! The size of the struct object is the number of bytes
//! allocated for the struct.
protected int _sizeof() {
  if(!sizeof(items)) return 0;
  return `+( @sizeof(items[*]) );
}

//! The struct can be casted into a string, which is eqivivalent
//! to running @[encode], or into an array. When casted into an
//! array each array element is the encoded value of that struct
//! item.
protected mixed cast(string to) {
  switch(to) {
  case "string": return encode();
  case "array": return items->encode();
  }
  return UNDEFINED;
}


// --- Virtual struct item class

//! Interface class for struct items.
class Item {
  int id = ADT.get_item_id();
  constant is_item=1;

  protected mixed value;
  int size;

  //! @ignore
  void decode(object);
  string encode();
  //! @endignore

  void set(mixed in) { value=in; }
  mixed get() { return value; }

  protected int _sizeof() { return size; }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, value);
  }
}


// --- Struct items.

//! One byte, integer value between 0 and 255.
class Byte {
  inherit Item;
  int size = 1;
  protected int(0..255) value;

  //! The byte can be initialized with an optional value.
  protected void create(void|int(0..255) initial_value) {
    set(initial_value);
  }

  void set(int(0..255) in) {
    if(in<0 || in>255) error("Value %d out of bound (0..255).\n", in);
    value = in;
  }
  void decode(object f) {
    sscanf(f->read(1), "%c", value);
  }
  string encode() {
    return sprintf("%c", value);
  }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d/%O)", this_program, value,
			     (string)({value}));
  }
}

//! One byte, signed integer value between -128 and 127.
class SByte {
  inherit Item;
  int size = 1;
  protected int(-128..127) value;

  //! The byte can be initialized with an optional value.
  protected void create(void|int(-128..127) initial_value) {
    set(initial_value);
  }

  void set(int(-128..127) in) {
    if(in<-128 || in>127) error("Value %d out of bound (-128..127).\n", in);
    value = in;
  }
  void decode(object f) {
    sscanf(f->read(1), "%+1c", value);
  }
  string encode() {
    return sprintf("%1c", value);
  }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d/%O)", this_program, value,
			     (string)({value}));
  }
}

//! One word (2 bytes) in network order, integer value between 0 and 65535.
//! @seealso
//! @[Drow]
class Word {
  inherit Item;
  int size = 2;
  protected int(0..) value;

  //! The word can be initialized with an optional value.
  protected void create(void|int(0..65535) initial_value) {
    set(initial_value);
  }

  void set(int(0..) in) {
    if(in<0 || in>~((-1)<<size*8))
      error("Value %d out of bound (0..%d).\n",
	    in, ~((-1)<<size*8));
    value = in;
  }
  void decode(object f) { sscanf(f->read(size), "%"+size+"c", value); }
  string encode() { return sprintf("%"+size+"c", value); }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d)", this_program, value);
  }
}

//! One word (2 bytes) in network order, signed integer value between
//! 0 and 65535.
class SWord {
  inherit Item;
  int size = 2;
  protected int value;

  //! The word can be initialized with an optional value.
  protected void create(void|int(-32768..32767) initial_value) {
    set(initial_value);
  }

  void set(int in) {
    int negmask;

    for (int i; i < size; i++) {
      negmask = (negmask << 8) | 0xff;
    }
    if(in<-(~(1<<size*8-1)&negmask) || in>~((-1)<<size*8-1))
      error("Value %d out of bound (%d..%d).\n",
	    in, -(~(1<<size*8-1)&negmask), ~((-1)<<size*8-1));
    value = in;
  }
  void decode(object f) { sscanf(f->read(size), "%+"+size+"c", value); }
  string encode() { return sprintf("%"+size+"c", value); }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d)", this_program, value);
  }
}

//! One word (2 bytes) in intel order, integer value between 0 and 65535.
//! @seealso
//! @[Word]
class Drow {
  inherit Word;
  void decode(object f) { sscanf(f->read(size), "%-"+size+"c", value); }
  string encode() { return sprintf("%-"+size+"c", value); }
}

//! One longword (4 bytes) in network order, integer value between 0 and 2^32.
//! @seealso
//! @[Gnol]
class Long {
  inherit Word;
  int size = 4;

  //! The longword can be initialized with an optional value.
  protected void create(void|int(0..) initial_value) {
    set(initial_value);
  }
}

//! One longword (4 bytes) in network order, signed integer value
//! -(2^31) <= x < 2^31-1.
class SLong {
  inherit SWord;
  int size = 4;

  //! The longword can be initialized with an optional value.
  protected void create(void|int initial_value) {
    set(initial_value);
  }
}

//! One longword (4 bytes) in intel order, integer value between 0 and 2^32.
//! @seealso
//! @[Long]
class Gnol {
  inherit Drow;
  int size = 4;

  //! The longword can be initialized with an optional value.
  protected void create(void|int(0..) initial_value) {
    set(initial_value);
  }
}


//! A string of bytes.
class Chars {
  inherit Item;
  protected Item dynsize;
  protected string value;

  //! @decl protected void create(int|Item size, void|string value)
  //! @[size] is the number of bytes that are part of this struct
  //! item, or optionally an earlier Item that will be looked up in
  //! runtime.
  //! The initial value of the char string is @[value] or,
  //! if not provided, a string of zero bytes.
  protected void create(int|Item _size, void|string _value) {
    if(intp(_size))
      size = _size;
    else
    {
      dynsize = _size;
      size = dynsize->get();
    }

    if(_value)
      set(_value);
    else
      value = "\0"*size;
  }

  void set(string in) {
    if(!dynsize && sizeof(in)!=size)
      error("String has wrong size (%d instead of %d).\n",
	    sizeof(in), size);
    if(String.width(in)!=8) error("Wide strings not allowed.\n");
    if(dynsize) dynsize->set(sizeof(in));
    value = in;
  }
  void decode(object f) {
    if(objectp(dynsize))
      size =  dynsize->get();
    value=f->read(size);
    if(!value || sizeof(value)!=size) error("End of data reached.\n");
  }
  string encode() { return value; }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, value);
  }
}

class Varchars {
  inherit Chars;
  protected int min,max;

  protected void create(void|int _min, void|int _max,
			string _value = " " * _min) {
    min = _min;
    max = _max;
    set(_value);
  }

  void set(string in) {
    if(sizeof(in)<min) error("String is too short.\n");
    if(max && sizeof(in)>max) error("String is too long.\n");
    if(String.width(in)!=8) error("Wide strings not allowed.\n");
    size = sizeof(in)+1;
    value = in;
  }

  void decode(object f) {
    String.Buffer buf = String.Buffer( predef::max(256,min) );
    string next;
    do {
      next = f->read(1);
    } while (next && next!="\0" && buf->add(next));
    set(buf->get());
  }
  string encode() {
    return value + "\0";
  }
}

//! Alias for @[SByte]
class int8   { inherit SByte; }

//! Alias for @[Byte]
class uint8  { inherit Byte; }

//! Alias for @[SWord]
class int16  { inherit SWord; }

//! Alias for @[Word]
class uint16 { inherit Word; }

//! Alias for @[SLong]
class int32  { inherit SLong; }

//! Alias for @[Long]
class uint32 { inherit Long; }

//! 64 bit signed integer.
class int64  {
  inherit SLong;
  int size = 8;
  void set(int v) {
    value = v;
  }
}

//! 64 bit unsigned integer.
class uint64 {
  inherit Long;
  int size = 8;
  void set(int v) {
    value = v;
  }
}

protected string _sprintf(int t) {
  if(t!='O') return UNDEFINED;
  string ret = sprintf("%O(\n", this_program);
  foreach(items, Item item) {
    string i = sprintf("    %s : %s\n", search(names,item),
		       (sprintf("%O", item)/"->")[-1]);
    if(item->is_struct)
      i = (i/"\n")[..<1]*"\n    "+"\n";
    ret += i;
  }
  return ret + ")";
}
