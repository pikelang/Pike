//
// Struct ADT
// By Martin Nilsson
// $Id: Struct.pike,v 1.1 2003/01/03 16:49:15 nilsson Exp $
//

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
//!     Item genre = Byte();
//!     Item Comment = Chars(30);
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

static int item_counter;
static array items;

//! @decl void create(void|string|Stdio.File data)
//! @param data
//!   Data to be decoded and populate the struct. Can
//!   either be a file object or a string.
void create(void|string|object file) {
  items = values(this_object());
  items = filter(items, lambda(mixed i) {
			    if(!objectp(i)) return 0;
			    return i->is_item;
			  } );
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

static mixed `[](string id) {
  mixed x = ::`[](id, 2);
  if(objectp(x) && x->is_item) return x->get();
  return x;
}

static mixed `[]=(string id, mixed value) {
  mixed x = ::`[](id, 2);
  if(objectp(x) && x->is_item)
    x->set(value);
  else
    ::`[]=(id,value);
}

static function `-> = `[];
static function `->= = `[]=;

// _indices and _values would be nice, but I don't know how to do them.

//! The size of the struct object is the number of bytes
//! allocated for the struct.
static int _sizeof() {
  return `+( @items->size );
}

//! The struct can be casted into a string, which is eqivivalent
//! to running @[encode], or into an array. When casted into an
//! array each array element is the encoded value of that struct
//! item.
static mixed cast(string to) {
  switch(to) {
  case "string": return encode();
  case "array": return items->encode();
  }
}

// --- Virtual struct item class

//! Interface class for struct items.
class Item {
  int id = item_counter++;
  constant is_item=1;

  static mixed value;
  int size;

  //! @ignore
  void decode(object);
  string encode();
  //! @endignore

  void set(mixed in) { value=in; }
  mixed get() { return value; }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, value);
  }
}


// --- Struct items.

//! One byte, integer value between 0 and 255.
class Byte {
  inherit Item;
  static int(0..255) value;
  int size = 1;

  void set(int(0..255) in) {
    if(in<0 || in>255) error("Value out of bound\n");
    value = in;
  }
  void decode(object f) { sscanf(f->read(1), "%c", value); }
  string encode() { return sprintf("%c", value); }

  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d/%O)", this_program, value, (string)({value}));
  }
}

//! One word in network order, integer value between 0 and 65535.
class Word {
  inherit Byte;
  int size = 2;
  static int(0..65535) value;

  void set(int(0..65535) in) {
    if(in<0 || in>65535) error("Value out of bound.\n");
    value = in;
  }
  void decode(object f) { sscanf(f->read(2), "%2c", value); }
  string encode() { return sprintf("%2c", value); }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d)", this_program, value);
  }
}

//! A string of bytes.
class Chars {
  inherit Item;
  int size;
  static string value;

  //! @decl static void create(int size)
  //! The number of bytes that are part of this struct item.
  static void create(int _size) { size = _size; }

  void set(string in) {
    if(sizeof(in)!=size) error("String has wrong size.\n");
    if(String.width(in)!=8) error("Wide strings not allowed.\n");
    value = in;
  }
  void decode(object f) { value=f->read(size); }
  string encode() { return value; }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, value);
  }
}

string _sprintf(int t) {
  return t=='O' && sprintf("%O(%O)", this_program, items);
}
