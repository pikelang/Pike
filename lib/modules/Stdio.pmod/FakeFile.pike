// $Id: FakeFile.pike,v 1.9 2004/03/10 19:18:48 nilsson Exp $
#pike __REAL_VERSION__

//! A string wrapper that pretends to be a @[Stdio.File] object.


//! This constant can be used to distinguish a FakeFile object
//! from a real @[Stdio.File] object.
constant is_fake_file = 1;

static string data;
static int ptr;
static int(0..1) r;
static int(0..1) w;

//! @seealso
//!   @[Stdio.File()->close()]
int close(void|string direction) {
  direction = lower_case(direction);
  int cr = has_value(direction, "r");
  int cw = has_value(direction, "w");

  if(cr) {
    if(!r) error("not open");
    r = 0;
  }

  if(cw) {
    if(!w) error("not open");
    w = 0;
  }
  return 1;
}

//! @decl void create(string data, void|string type, void|int pointer)
//! @seealso
//!   @[Stdio.File()->create()]
void create(string _data, void|string type, int|void _ptr) {
  if(!_data) error("No data string given to FakeFile.\n");
  data = _data;
  ptr = _ptr;
  if(type) {
    type = lower_case(type);
    if(has_value(type, "r"))
      r = 1;
    if(has_value(type, "w"))
      w = 1;
  }
  else
    r = w = 1;
}

static string make_type_str() {
  string type = "";
  if(r) type += "r";
  if(w) type += "w";
  return type;
}

//! @seealso
//!   @[Stdio.File()->dup()]
this_program dup() {
  return this_program(data, make_type_str(), ptr);
}

//! Always returns 0.
//! @seealso
//!   @[Stdio.File()->errno()]
int errno() { return 0; }

//! @seealso
//!   @[Stdio.File()->line_iterator()]
String.SplitIterator line_iterator(int|void trim) {
  if(trim)
    return String.SplitIterator( data-"\r", '\n' );
  return String.SplitIterator( data, '\n' );
}

static mixed id;

//! @seealso
//!   @[Stdio.File()->query_id()]
mixed query_id() { return id; }

//! @seealso
//!   @[Stdio.File()->set_id()]
void set_id(mixed _id) { id = _id; }

//! @seealso
//!   @[Stdio.File()->read_function()]
function(:string) read_function(int nbytes) {
  return lambda() { read(nbytes); };
}

//! @seealso
//!   @[Stdio.File()->peek()]
int(-1..1) peek(int|float|void timeout) {
  if(!r) return -1;
  if(ptr >= sizeof(data)) return 0;
  return 1;
}

//! Always returns 0.
//! @seealso
//!   @[Stdio.File()->query_address()]
string query_address(void|int(0..1) is_local) { return 0; }

//! @seealso
//!   @[Stdio.File()->read()]
string read(void|int(0..) len, void|int(0..1) not_all) {
  if(!r) return 0;
  int start = ptr;
  int end;

  if(len>sizeof(data)) len=sizeof(data);
  if(zero_type(len))
    end = sizeof(data)-1;
  else
    end = start+len-1;
  ptr = end+1;

  return data[start..end];
}

//! @seealso
//!   @[Stdio.File()->seek()]
int seek(int pos, void|int mult, void|int add) {
  if(mult)
    pos = pos*mult+add;
  if(pos<0)
    pos = sizeof(data)+pos;
  ptr = pos;
}

//! Always returns 1.
//! @seealso
//!   @[Stdio.File()->sync()]
int(1..1) sync() { return 1; }

//! @seealso
//!   @[Stdio.File()->tell()]
int tell() { return ptr; }

//! @seealso
//!   @[Stdio.File()->truncate()]
int(0..1) truncate(int length) {
  data = data[..length-1];
  return sizeof(data)==length;
}

//! @seealso
//!   @[Stdio.File()->write()]
int(-1..) write(string|array(string) str, mixed ... extra) {
  if(!w) return -1;
  if(arrayp(str)) str=str*"";
  if(sizeof(extra)) str=sprintf(str, @extra);

  if(ptr==sizeof(data)) {
    data += str;
    ptr = sizeof(data);
  }
  else if(sizeof(str)==1)
    data[ptr++] = str[0];
  else {
    data = data[..ptr-1] + str + data[ptr+sizeof(str)..];
    ptr += sizeof(str);
  }
  return sizeof(str);
}

string _sprintf(int t) {
  return t=='O' && sprintf("%O(%d,%O)", this_program, sizeof(data),
			   make_type_str());
}


// FakeFile specials.

//! A FakeFile can be casted to a string.
mixed cast(string to) {
  switch(to) {
  case "string": return data;
  case "object": return this;
  }
  error("Can not cast object to %O.\n", to);
}

//! Sizeof on a FakeFile returns the size of its contents.
int(0..) _sizeof() {
  return sizeof(data);
}

//! @ignore

#define NOPE(X) mixed X (mixed ... args) { error("This is a FakeFile. %s is not available.\n", #X); }
NOPE(assign);
NOPE(async_connect);
NOPE(connect);
NOPE(connect_unix);
NOPE(open);
NOPE(open_socket);
NOPE(pipe);
NOPE(query_close_callback);
NOPE(query_read_callback);
NOPE(query_read_oob_callback);
NOPE(query_write_callback);
NOPE(query_write_oob_callback);
NOPE(set_blocking);
NOPE(set_blocking_keep_callbacks);
NOPE(set_close_callback);
NOPE(set_nonblocking);
NOPE(set_nonblocking_keep_callbacks);
NOPE(set_read_callback);
NOPE(set_read_oob_callback);
NOPE(set_write_callback);
NOPE(set_write_oob_callback);
NOPE(tcgetattr);
NOPE(tcsetattr);

// Stdio.Fd
NOPE(dup2);
NOPE(lock); // We could implement this
NOPE(mode); // We could implement this
NOPE(proxy); // We could implement this
NOPE(query_fd);
NOPE(read_oob);
NOPE(set_close_on_exec);
NOPE(set_keepalive);
NOPE(stat); // We could implement this
NOPE(trylock); // We could implement this
NOPE(write_oob);

//! @endignore
