// ID3.pmod
//
//  $Id: ID3.pmod,v 1.20 2004/04/06 21:44:57 nilsson Exp $
//

#pike __REAL_VERSION__

//! ID3 decoder/encoder.
//! Supports versions 1.0, 1.1, 2.2-2.4.
//! For more info see http://www.id3.org
//!
//! @note
//! Note that this implementation is far
//! from complete and that interface changes
//! might be neccessary during the implementation
//! of the full standard.

//
// Author: Martin Nilsson
//
// Added by Honza Petrous:
//  - v1 support
//  - v2.3 and v2.2 basic support
//



// ID3v2

#define TEST(X,Y) !!((X) & 1<<(Y))

//! A wrapper around a @[Stdio.File] object that provides a read
//! limit capability.
class Buffer(Stdio.File buffer) {

  static string peek_data;
  static int limit=-1;

  //! Read @[bytes] bytes from the buffer. Throw an exception
  //! if @[bytes] is bigger than the number of bytes left in the
  //! buffer before reaching the limit set by @[set_limit].
  string read(int bytes) {
    if(limit!=-1) {
      if(limit-bytes<0)
        error( "Tag ended unexpextedly.\n" );
      limit -= bytes;
    }
    if(peek_data) {
      string ret = peek_data + buffer->read(bytes-1);
      peek_data = 0;
      return ret;
    }
    return buffer->read(bytes);
  }

  //! Preview the next byte. Technically it is read from the
  //! encapsulated buffer and put locally to avoid seeking.
  string peek() {
    if(!limit) error( "End of tag reached.\n" );
    if(!peek_data) peek_data = buffer->read(1);
    return peek_data;
  }

  //! Set an artificial EOF @[bytes] bytes further into the
  //! buffer.
  void set_limit(int bytes) {
    limit = bytes;
  }

  //! The number of bytes left before reaching the limit set by
  //! @[set_limit].
  int bytes_left() {
    return limit;
  }
}

//! Decodes a synchsafe integer, generated according to
//! ID3v2.4.0-structure section 6.2.
//! @seealso
//!   @[int_to_synchsafe]
int synchsafe_to_int(array(int) bytes) {
  int res;
  foreach(bytes, int byte) {
    if(byte==255) error("Synchsafe integer not synchsafe.\n");
    res = res << 7 | byte;
  }
  return res;
}

//! Encodes a integer to a synchsafe integer according to
//! ID3v2.4.0-structure section 6.2.
//! @seealso
//!   @[synchsafe_to_int]
array(int) int_to_synchsafe(int in, void|int no_bytes) {
  array res = ({});
  do {
    res += ({ in&127 });
   } while(in>>=7);
  if(no_bytes)
    res += ({0})*(no_bytes-sizeof(res)); // Throws when res>no_bytes
  return reverse(res);
}

//! Represents an ID3v2 header.
class TagHeader {

  int(2..2)   major_version = 2;
  int(2..4)   minor_version = 4;
  int(0..255) sub_version = 0;

  int(0..1) flag_unsynchronisation = 0;
  int(0..1) flag_extended_header = 0;
  int(0..1) flag_experimental = 0;
  int(0..1) flag_footer = 0;

  int tag_size;

  //!
  void create(void|Buffer buffer) {
    if(buffer) decode(buffer);
  }

  //! Decode a tag header from @[buffer] and store its data in
  //! this object.
  void decode(Buffer buffer) {
    string data = buffer->read(10);
    if(!has_prefix(data, "ID3"))
      error( "Header has wrong identifier. Expected \"ID3\", got %O.\n",
	     data[..2] );

    array bytes = (array(int))data[3..];
    if( ! (< 2, 3, 4 >)[bytes[0]] )
      error( "Can only handle ID3v2.{2,3,4}.x. Got ID3v2.%d.%d\n",
	     bytes[0], bytes[1] );

    minor_version = bytes[0];
    if( minor_version == 255 )
      error( "Illegal minor version.\n");
    sub_version = bytes[1];
    if( sub_version == 255 )
      error( "Illegal sub version.\n");
    decode_flags(bytes[2]);
    tag_size = synchsafe_to_int( bytes[3..] );
  }

  static void decode_flags( int byte ) {
    if(byte&0b1111)
      error( "Unknown flag set in tag header flag field. (%08b)\n", byte );

    flag_unsynchronisation = TEST(byte,7);
    flag_extended_header = TEST(byte,6);
    flag_experimental = TEST(byte,5);
    flag_footer = TEST(byte,4);
  }

  static int encode_flags() {
    return flag_unsynchronisation<<7 + flag_extended_header<<6 +
      flag_experimental<<5 + flag_footer<<4;
  }

  //! Encode the data in this tag and return as a string.
  string encode() {
    return sprintf("ID3%c%c%c", minor_version, sub_version, encode_flags()) +
      (string)int_to_synchsafe(tag_size,4);
  }

  //! Should the unsynchronisation flag be set or not?
  int(0..1) set_flag_unsynchronisation(array(Frame) frames) {
    if(sizeof( frames->flag_unsynchronisation - ({ 0 }) ) == sizeof(frames))
      return flag_unsynchronisation = 1;
    return flag_unsynchronisation = 0;
  }
}

//!
class ExtendedHeader {

  int(0..1) flag_is_update = 0;
  int(0..1) flag_crc = 0;
  int(0..1) flag_restrictions = 0;

  int crc;

  int(0..3) restr_size = 0;
  int(0..1) restr_encoding = 0;
  int(0..3) restr_field_size = 0;
  int(0..1) restr_img_enc = 0;
  int(0..3) restr_img_size = 0;

  int size;
  string data;

  //!
  void create(void|Buffer buffer) {
    if(buffer) decode(buffer);
  }

  static void decode_flags(string bytes) {
    int flags = bytes[0];
    flag_is_update = TEST(flags,6);
    flag_crc = TEST(flags,5);
    flag_restrictions = TEST(flags,4);
  }

  static string encode_flags() {
    return sprintf("%c", flag_is_update<<6 + flag_crc<<5 +
		   flag_restrictions<<4);
  }

  static string decode_restrictions(int data) {
    restr_size = (data & 0b11000000) >> 6;
    restr_encoding = TEST(data, 5);
    restr_field_size = (data & 0b00011000) >> 3;
    restr_img_enc = TEST(data, 2);
    restr_img_size = data & 0b00000011;
  }

  static string encode_restrictions() {
    return sprintf("%c", restr_size<<6 + restr_encoding<<5 +
		   restr_field_size<<3 + restr_img_enc<<2 +
		   restr_img_size);
  }

  //!
  void decode(Buffer buffer) {
    size = synchsafe_to_int( (array)buffer->read(4) );
    int flagbytes = buffer->read(1)[0];
    if(!flagbytes) error("At least one flag byte is required.\n");
    decode_flags(buffer->read(flagbytes));

    if(flag_crc)
      crc = synchsafe_to_int((array(int))buffer->read(5));
    // FIXME: There is no validation of the CRC of the file...

    if(flag_restrictions)
      decode_restrictions(buffer->read(1)[0]);
  }

  //!
  string encode() {
    string ret = encode_flags();
    if(flag_is_update) ret += "\0";
    if(flag_crc) ret += (string)int_to_synchsafe(crc);
    if(flag_restrictions) ret += encode_restrictions();
    return ret;
  }
}

//! Reverses the effects of unsyncronisation
//! done according to ID3v2.4.0-structure section 6.1.
//! @seealso
//!   @[unsynchronise]
string resynchronise(string in) {
  return replace(in, "\0xff\0x00", "\0xff");
}

//! Unsynchronises the string according to
//! ID3v2.4.0-structure section 6.1.
//! @seealso
//!   @[resynchronise]
string unsynchronise(string in) {
  return replace(in, ([ "\0xff":"\0xff\0x00",
			"\0xff\0x00":"\0xff\0x00\0x00" ]) );
}

//! Decodes the string @[in] from the @[type], according to
//! ID3v2.4.0-structure section 4, into a wide string.
//! @seealso
//!   @[encode_string]
string decode_string(string in, int type) {
  switch(type) {
  case 0:
    return in;
  case 1:
    return unicode_to_string(in);
  case 2:
    return unicode_to_string("\xfe\xff" + in);
  case 3:
    return utf8_to_string(in);
  }
  error( "Unknown text encoding "+type+".\n" );
}

//! Encodes the string @[in] to an int-string pair, where the
//! integer is the encoding mode, according to ID3v2.4.0-structure,
//! and the string is the encoded string. This function tries to
//! minimize the size of the encoded string by selecting the most
//! apropriate encoding method.
//! @seealso
//!   @[decode_string], @[encode_strings]
array(string|int) encode_string(string in) {
  if(String.width(in)>8) return ({ 3, string_to_utf8(in) });
  return ({ 0, in });
}

//! Encodes several strings in the same way as @[encode_string], but
//! encodes all the strings with the same method, selected as in
//! @[encode_string]. The first element in the resulting array is the
//! selected method, while the following elements are the encoded strings.
//! @seealso
//!   @[decode_string], @[encode_string]
array(string|int) encode_strings(array(string) in) {
  int width;
  foreach(in, string text)
    width = min(width, String.width(text));
  if(width>8)
    return ({ 3 }) + map(in, string_to_utf8);
  return ({ 0 }) + in;
}

//! Abstract class for frame data.
class FrameData {

  private string original_data;

  //!
  void create(void|string data) {
    original_data = data;
    if(data) decode(data);
  }

  //! Is the content altered?
  int(0..1) changed() {
    return original_data && (encode() != original_data);
  }

  void decode(string data);
  string encode();
}

//!
class Frame {

  string id;
  int size;
  TagHeader header;

  int(0..1) flag_tag_alteration;
  int(0..1) flag_file_alteration;
  int(0..1) flag_read_only;

  int(0..1) flag_grouping;
  string group_id;

  int(0..1) flag_compression;

  int(0..1) flag_encryption;
  string encryption_id;

  int(0..1) flag_unsynchronisation;
  int(0..1) flag_dli;

  FrameData data;

  void create(string|Buffer in, TagHeader thd) {
    header = thd;
    if(in) {
      decode(in);
      return;
    }

    data = get_frame_data(id)();
    // Setting default alteration flags according to ID3v2.4.0-frames
    // section 3.
    flag_tag_alteration = 0;
    flag_file_alteration = (< "ASPI", "AENC", "ETCO", "EQU2",
			      "MLLT", "POSS", "SEEK", "SYLT",
			      "SYTC", "RVA2", "TENC", "TLEN" >)[id];
  }

  program get_frame_data(string id) {
    if( (< "TIT1", "TIT2", "TIT3", "TALB", "TOAL", "TSSE", "TSST",
	   "TSRC", "TPE1", "TPE2", "TPE3", "TPE4", "TOPE", "TEXT",
	   "TOLY", "TCOM", "TENC", "TLAN", "TMOO", "TOWN", "TRSN",
	   "TRSO", "TOFN", "TSOA", "TSOP", "TSOT" >)[id] )
      return Frame_TextPlain;

    if( (< "TMCL", "TIPL" >)[id] )
      return Frame_TextMapping;

    if( (< "TBPM", "TLEN", "TDLY",
	   /* 2.3.0 */
	   "TYER" >)[id] )
      return Frame_TextInteger;

    if( (< "TDEN", "TDOR", "TDRC", "TDRL", "TDTG" >)[id] )
      return Frame_TextTimestamp;

    if( (< "WCOM", "WCOP", "WOAF", "WOAR", "WOAS", "WPAY", "WPUB" >)[id] )
      return Frame_Link;

    if(global::this["Frame_"+id]) return global::this["Frame_"+id];

    // FIXME: Look for program in this module.
    return Frame_Dummy;
  }

  void decode_flags( int flag0, int flag1 ) {
    flag_tag_alteration = flag0 & 1<<6;
    flag_file_alteration = flag0 & 1<<5;
    flag_read_only = flag0 & 1<<4;

    if(flag0 & 0b10001111)
      error( "Unknown flag in first frame flag field. (%08b)\n", flag0 );

    flag_grouping = flag1 & 1<<6;
    flag_compression = flag1 & 1<<3;
    flag_encryption = flag1 & 1<<2;
    flag_unsynchronisation = flag1 & 1<<1;
    flag_dli = flag1 & 1;

    if(flag1 & 0b10110000)
      error( "Unknown flag in second frame flag field (%08b).\n", flag1 );
  }

  array(int) encode_flags() {
    return ({ flag_tag_alteration<<6 + flag_file_alteration<<5 +
	      flag_read_only<<4,
	      flag_grouping<<6 + flag_compression<<3 + flag_encryption<<2 +
	      flag_unsynchronisation<<1 + flag_dli });
  }


  void decode(Buffer buffer) {
    string asize;

    switch(header->minor_version) {
      case 4:
      case 3:
	id = buffer->read(4);
	asize =  buffer->read(4);
	if(header->minor_version == 4)
          size = synchsafe_to_int( (array(int))asize );
	else
          sscanf(asize, "%4c", size);
	array flags = array_sscanf( buffer->read(2), "%c%c");
	decode_flags( flags[0], flags[1] );
	break;
      case 2:
	id = buffer->read(3);
	asize =  buffer->read(3);
        sscanf(asize, "%3c", size);
	break;
    }

    string _data = buffer->read(size);
    int hd_size = flag_grouping + flag_encryption + 4*flag_dli;
    string hd_data = _data[..hd_size-1];
    _data = _data[hd_size..];

    if(flag_unsynchronisation)
      _data = resynchronise(_data);

    if(flag_encryption)
      error( "Encrypted frames not supported.\n" );

    if(flag_compression)
#if constant(Gz.inflate)
      _data = Gz.inflate()->inflate(_data);
#else
    error( "Frame is Gz compress, but Pike lacks Gz support.\n" );
#endif

    if(flag_grouping)
      group_id = hd_data[0..0];

    //    werror("%s: %O\n", id, _data);
    data = get_frame_data(id)( _data );
  }

  mapping get() {
    switch(id) {
    default:
      error( "No frame identifier available.\n" );
    }
  }

  string encode() {
    string block = data->encode();

    if(flag_compression)
#if constant(Gz.deflate)
      block = Gz.deflate()->deflate(block);
#else
      error( "Pike lacks Gz support.\n" );
#endif

    // FIXME: Encryption goes here

    if(flag_unsynchronisation)
      block = unsynchronise(block);

    if(flag_dli)
      block = sprintf("%c%c%c%c%s", @int_to_synchsafe(sizeof(block)),
		      block);

    // FIXME: Encryption id goes here

    if(flag_grouping && group_id)
      block = group_id + block;

    return id + (string)int_to_synchsafe(sizeof(block),4) +
      sprintf("%c%c", @encode_flags()) + block;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s)", this_program, id);
  }
}

//! ID3 version 2 (2.2, 2.3, 2.4) Tags
class Tagv2 {

  TagHeader header;
  ExtendedHeader extended_header;
  array(Frame) frames = ({});
  int padding;

  int(0..1) best_effort;
  array(string) errors = ({});

  // convenience
  mapping(string:array(Frame)) frame_map = ([]);

  //!
  void create(void|Buffer|Stdio.File buffer, void|int(0..1) _best_effort) {
    best_effort = _best_effort;
    if(buffer) {
      if(!buffer->set_limit)
	buffer = Buffer(buffer);
      decode(buffer);
      return;
    }
    else {
      header = TagHeader();
      extended_header = ExtendedHeader();
    }
  }

  void handle_error(array error) {
    if(!error) return;
    if(best_effort)
      errors += ({ sprintf("%O: %s", error[1][-1], error[0]) });
    else
      throw(error);
  }

  void decode(Buffer buffer) {
    buffer->set_limit(10);
    header = TagHeader(buffer);
    buffer->set_limit(header->tag_size);

    mixed err = catch {
      if(header->flag_extended_header)
	extended_header = ExtendedHeader(buffer);
    };
    handle_error(err);

    while(buffer->bytes_left() && buffer->peek()!= "\0") {
      err = catch {
	frames += ({ Frame(buffer, header) });
      };
      handle_error(err);
    }

    padding = buffer->bytes_left();

    // Verify that padding is all zero.
    if(!best_effort && padding && `+(@(array(int))buffer->read(padding)))
      error( "Non-zero value in padding.\n" );

    // FIXME: footer

    build_frame_map();
  }

  void build_frame_map() {
    frame_map = ([]);
    foreach(frames, Frame f) {
      if(!frame_map[f->id])
	frame_map[f->id] = ({ f });
      else
	frame_map[f->id] += ({ f });
    }
  }

  string encode() {
    string block = (array(string))frames->encode()*"" + "\0"*padding;
    header->set_flag_unsynchronisation(frames);
    header->tag_size = sizeof(block);
    return header->encode() + block;
  }

  Tag update(Tag t) {
    if(!t->extended_header->flag_is_update) return t;
    // FIXME: Incorporate tags.
  }

  Frame get_frame(string id) {
    foreach(frames, Frame f)
      if(f->id==id) return f;
    return 0;
  }

  void clean() {
    foreach(frames; int pos; Frame f) {
      FrameData d = f->data;
      switch(f->id) {
      case "TCON":
	string s = d->get_string();
	if(s=="" || s=="misc" || s=="genre")
	  frames[pos] = 0;
	break;
      case "TCOP":
	if(d->get_string()=="")
	  frames[pos] = 0;
	break;
      case "COMM":
	if(d->short=="" && d->text=="")
	  frames[pos] = 0;
	break;
      case "TALB":
      case "TCOM":
      case "TENC":
      case "TOPE":
	if(d->get_string()=="")
	  frames[pos] = 0;
	break;
      case "WXXX":
	if(d->url=="") frames[pos] = 0;
	break;
      }
    }
    frames -= ({ 0 });
    build_frame_map();
  }

  string _sprintf(int t, mapping args) {
    if(t!='O') return 0;
    if(!header) return sprintf("ID3v2()");
    return sprintf("ID3v%d.%d.%d(%d)", header->major_version,
		   header->minor_version, header->sub_version,
		   sizeof(frames));
  }
}

array terminator = ({ "\0", "\0\0", "\0" });


// --- Tags ------------------------------------------------------------------

class Frame_UFID {
  inherit FrameData;

  string system;
  string id;

  void decode(string data) {
    if( sscanf(data, "%s\0%s", system, id) != 2 )
      error( "Malformed UFID frame.\n" );
  }

  string encode() {
    if(!system)
      error( "Can not encode UFID frame. "
	     "Missing owner identifier (system).\n" );
    if(!id) error( "Can not encode UFID frame. Missing identifier.\n" );
    return system + "\0" + id;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O/%O)", this_program, system, id);
  }
}

class Frame_TextPlain {
  inherit FrameData;

  static array(string) texts;

  void decode(string data) {
    int encoding;
    if(!sizeof(data))
      error( "Malformed text frame. Missing encoding byte.\n" );

    sscanf(data, "%c%s", encoding, data);
    texts = map(data / terminator[encoding], decode_string, encoding);

    if(sizeof(texts)>1) {
      texts -= ({ "" });
      if(!sizeof(texts)) texts = ({ "" });
    }
  }

  string encode() {
    array ret = encode_strings(texts);
    return sprintf("%c", ret[0]) + ret[1..]*terminator[ret[0]];
  }

  // Interface

  string get_string() {
    return texts[0];
  }

  array(string) get_strings() {
    return texts;
  }

  void set_string(string in) {
    texts = ({ in });
  }

  void set_strings(array(string) in) {
    texts = in;
  }

  // Needed by tag restrictions (extended header)
  int get_size() {
    return sizeof(texts*"");
  }

  static string _sprintf(int t) {
    if( t!='O' || !sizeof(texts)) return UNDEFINED;
    if(sizeof(texts)==1) return sprintf("%O(%O)", this_program, texts[0]);
    return sprintf("%O(%O)", this_program, texts);
  }
}

class Frame_TKEY {
  inherit Frame_TextPlain;

  int(0..1) verify_key(string key) {
    if(key=="o") return 1;
    if( !(< 'A','B','C','D','E','F','G' >)[key[0]] ) return 0;
    key = key[1..];
    if(!sizeof(key)) return 1;
    if( (< '#', 'b' >) ) key = key[1..];
    if(!sizeof(key)) return 1;
    if( key=="m" ) return 1;
    return 0;
  }

  static void verify_keys() {
    foreach(texts, string key)
      if(!verify_key(key))
	error("Malformed key %O\n", key);
  }

  void decode(string data) {
    ::decode(data);
    verify_keys();
  }

  string encode() {
    verify_keys();
    return ::encode();
  }
}

class Frame_TCON {
  inherit Frame_TextPlain;
}

class Frame_TFLT {
  inherit Frame_TextPlain;
}

class Frame_TMED {
  inherit Frame_TextPlain;

  void decode(string data) {
    ::decode(data);

    // Some client adds paranthesis here...
    foreach(texts; int pos; string t)
      if(sizeof(t) && t[0]=='(' && t[-1]==')')
	texts[pos] = t[1..sizeof(t)-2];
  }
}

class Frame_TCOP {
  inherit Frame_TextPlain;
}

class Frame_TPRO {
  inherit Frame_TextPlain;
}

class Frame_TextAofB {
  inherit FrameData;

  array(int) low_decode(string data) {
    if(!sizeof(data))
      error( "Malformed text frame. Missing encoding byte.\n" );
    data = decode_string(data[1..], data[0]);
    int a,b;
    sscanf(data, "%s\0", data);
    if( !sscanf(data, "%d/%d", a, b) )
      error( "Frame data doesn't contain integer.\n" );

    // Not against the spec, really...
    if(b && a > b) error( "(A/B) A bigger than B (%d/%d).\n", 1, b );
    if(!a) error( "Element is zero.\n" );

    return ({ a, b });
  }

  string low_encode(int a, int b) {
    if(b) return "\0"+a+"/"+b;
    return "\0"+a;
  }

  // Needed by tag restrictions (extended header)
  int get_size() {
    return sizeof(encode());
  }
}

class Frame_TRCK {
  inherit Frame_TextAofB;

  int track;
  int tracks;

  void decode(string data) {
    [track, tracks] = low_decode(data);
  }

  string encode() {
    return low_encode(track, tracks);
  }

  string get_string() {
    return (track ? (string)track : "") + (tracks ? "/"+tracks : "");
  }

  static string _sprintf(int t) {
    if( t!='O' ) return UNDEFINED;
    if(tracks) return sprintf("%O(%O/%O)", this_program, track,tracks);
    return sprintf("%O(%O)", this_program, track);
  }
}

class Frame_TPOS {
  inherit Frame_TextAofB;

  int part;
  int parts;

  void decode(string data) {
    [part, parts] = low_decode(data);
  }

  string encode() {
    return low_encode(part, parts);
  }

  static string _sprintf(int t) {
    if( t!='O' ) return UNDEFINED;
    if(parts) return sprintf("%O(%O/%O)", this_program, part,parts);
    return sprintf("%O(%O)", this_program, part);
  }
}

class Frame_TextMapping {
  inherit FrameData;
  // FIXME: Order is not preserved.

  mapping list = ([]);

  void decode(string data) {
    if(!sizeof(data))
      error( "Malformed text frame. Missing encoding byte.\n" );
    array tmp = data[1..]/"\0";
    tmp = map(tmp, decode_string, data[0]);
    foreach(tmp/2, array pair)
      list[pair[0]] = pair[1]; // FIXME: Doubles gets overwritten
  }

  string encode() {
    // FIXME: Character encoding.
    array tmp = ({});
    foreach(indices(list), string what)
      tmp += ({ what + "\0" + list[what] });
    return "\0" + tmp*"\0";
  }

  // Needed by tag restrictions (extended header)
  int get_size() {
    return sizeof(indices(list)*"" + values(list)*"");
  }
}

class Frame_TextInteger {
  inherit FrameData;

  int value;

  void decode(string data) {
    if(!sizeof(data))
      error( "Malformed text frame. Missing encoding byte.\n" );
    data = decode_string(data[1..], data[0]);
    int a;
    sscanf(data, "%s\0", data);
    if(!sizeof(data)) return;
    if( !sscanf(data, "%d", a) )
      error( "Frame data doesn't contain integer.\n" );

    value = a;
  }

  string encode() {
    return "\0"+value;
  }

  // Needed by tag restrictions (extended header)
  int get_size() {
    return sizeof(encode());
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, value);
  }
}

class Frame_TextTimestamp {
  inherit Frame_TextPlain;
}

class Frame_TXXX {
  inherit FrameData;

  string descr;
  string value;

  void decode(string data) {
    if(!sizeof(data))
      error( "Malformed text frame. Missing encoding byte.\n" );
    data = decode_string(data[1..], data[0]);
    sscanf(data, "%s\0%s", descr, value);
  }

  string encode() {
    array ret = encode_strings( ({ descr, value }) );
    return sprintf("%c", ret[0]) + ret[1..]*terminator[ret[0]];
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O:%O)", this_program,descr,value);
  }
}

class Frame_Link {
  inherit FrameData;

  string url;

  void decode(string data) {
    url = data;
  }

  string encode() {
    return url;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program,url);
  }
}

// "XXX", "CDDB Disc ID":id
class Frame_WXXX {
  inherit FrameData;

  string descr;
  string url;

  void decode(string data) {
    if(!sizeof(data))
      error( "Malformed text frame. Missing encoding byte.\n" );
    int enc = data[0];
    if(enc==1 || enc==2)
      sscanf(data, "%s\0\0%s", descr, url);
    else
      sscanf(data, "%s\0%s", descr, url);
    descr = decode_string(descr, enc);
    sscanf(url, "%s\0", url); // Strip trailing data
  }

  string encode() {
    array ret = encode_string(descr+"\0");
    return sprintf("%c%s%s", ret[0], ret[1], url);
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O:%O)", this_program,descr,url);
  }
}

// For unknown frames...
class Frame_Dummy {
  inherit FrameData;

  string data;

  void decode(string _data) {
    data = _data;
  }

  string encode() {
    return data;
  }

  // Needed by tag restrictions (extended header)
  // if this becomes a text frame.
  int get_size() {
    return sizeof(data);
  }

#if 0
  string get_string() {
    return "";
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program,data);
  }
#endif
}

class Frame_MCDI {
  inherit Frame_Dummy;
}

// ETCO, MLLT, SYTC, USLT, SYLT

class Frame_COMM {
  inherit FrameData;

  string lang;
  string short;
  string text;

  void decode(string data) {
    if(sizeof(data)<5)
      error( "Malformed text frame. Missing encoding byte.\n" );
    lang = data[1..3];
    data = decode_string(data[4..], data[0]);
    sscanf(data, "%s\0%s", short,text);
    sscanf(text, "%s\0", text);
  }

  string encode() {
    if(sizeof(lang)!=3) error("Malformed langauge data.\n");
    array ret = encode_strings( ({ short, text }) );
    return sprintf("%c", ret[0]) + lang + ret[1..]*terminator[ret[0]];
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O,%O:%O)", this_program,lang,short,text);
  }
}

class Frame_APIC {
  inherit FrameData;

  string mime;
  int type;
  string descr;

  string img;

  void decode(string data) {
    if(sizeof(data)<4)
      error( "Malformed text frame. Missing encoding byte.\n" );
    int enc = data[0];
    sscanf(data, "%*c%s\0%c%s"+terminator[enc]+"%s", mime,type,descr,img);
    descr = decode_string(descr,enc);
  }

  string encode() {
    array ret = encode_string(descr+"\0");
    return sprintf("%c%s\0%c%s%s", ret[0],mime,type,ret[1],img);
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O,%O,%O)", this_program,mime,descr,type);
  }
}

class Frame_POPM {
  inherit FrameData;

  string email;
  int rating;
  int count;

  void decode(string data) {
    sscanf(data, "%s\0%c%s", email, rating, data);
    count = (int)Gmp.mpz(data, 256);
  }

  string encode() {
    return sprintf("%s\0%c%s", email, rating, Gmp.mpz(count)->digits(256));
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O,%O,%O)", this_program,email,rating,count);
  }
}

class Frame_GEOB {
  inherit FrameData;

  string mime;
  string filename;
  string descr;
  string file;

  void decode(string data) {
    if(sizeof(data)<4)
      error( "Malformed text frame. Missing encoding byte.\n" );
    int enc = data[0];
    sscanf(data, "%*c%s\0%s"+terminator[enc]+"%s"+terminator[enc]+"%s",
	   mime,filename,descr,file);
    filename = decode_string(filename,enc);
    descr = decode_string(descr,enc);
  }

  string encode() {
    // FIXME
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O,%O,%O)", this_program,mime,filename,descr);
  }
}

class Frame_PRIV {
  inherit FrameData;

  string owner;
  string data;

  void decode(string d) {
    sscanf(d, "%s\0%s", owner, data);
  }

  string encode() {
    return owner + "\0" + data;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O,%O)", this_program,owner,data);
  }
}


//! ID3 version 1.0 or 1.1 tag. This is really a clumsy way of reading
//! ID3v1 tags, but it has the same interface as the v2 reader.
class Tagv1 {

  array frames = ({});
  mapping header =
    ([ "major_version": 1,
       "minor_version": 0,
       "sub_version": 0
    ]);

  static class Parser {
    inherit ADT.Struct;
    Item head = Chars(3);
    Item title = Chars(30);
    Item artist = Chars(30);
    Item album = Chars(30);
    Item year = Chars(4);
    Item comment = Chars(30);
    Item genre = Byte();
  };

  void create(Stdio.File file) {

    object st = file->stat();
    string buf;
    int i;

    if(!st)
      error("File not open.\n");
    if(st->size < 128)
      error("File is too small.\n");
    file->seek(-128);
    if(!(buf = file->read(128)) || sizeof(buf) < 128)
      error("File read error.\n");

    Parser tag = Parser(buf);
    if(tag->head != "TAG")
      error("Tag is missing.\n");

#define ADD(X) frames += ({ Framev1((string)tag->X,#X) })
    ADD(title);
    ADD(artist);
    ADD(album);
    ADD(year);
    ADD(comment);
    if(tag->comment[-2]==0 && tag->comment[-1]!=0) {
      header->minor_version = 1;
      frames += ({ Framev1((string)tag->comment[-1], "track") });
    }
    ADD(genre);
  }
}

class Framev1 {
  
  string id;
  int size;
  FrameDatav1 data;

  void create(string buffer, string name) {
    id = name;
    data = FrameDatav1(buffer, name);
    size = sizeof(buffer);
  }

  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s)", this_program, id);
  }
}

class FrameDatav1 {
  static string frame_data;
  static string id;

  void create(string buffer, string name) {
    int i;

    frame_data = buffer;
    id = name;
    sscanf(frame_data, "%s\0", frame_data);
  }

  string get_string() {
    return frame_data;
  }

  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s %O)", this_program, id, frame_data);
  }
}

//! This is a ID3 tag super object, which encapsulates all versions
//! ID3 tags. This is useful if you are only interested in the metadata
//! of a file, and care not about how it is stored or have no interest
//! in changing the data.
//!
//! @note
//!  Version 1 tag is searched only if version 2 isn't found.
//!
//! @seealso
//!  @[Tagv2], @[Tagv1]
class Tag {

  static Tagv2|Tagv1 tag;

  //! The file object @[fd] is searched for version 2 tags, and if
  //! not found, version 1 tags.
  //! @throws
  //!   If no tag was found in the file an error is thrown.
  static void create(Stdio.File fd) {

    catch(tag = Tagv2(Buffer(fd)));
    if(tag)
      return;
    tag = Tagv1(fd);
    catch(tag = Tagv1(fd));
    if(tag)
      return;
    error("No ID3 tag was found in file.\n");
  }

  //! The index operators are overloaded to index the encapsulated
  //! Tagv1 or Tagv2 object.
  static mixed `[](string index) { return `->(index); }
  static mixed `->(string index) {

    if(index == "version")
      return ""+tag->header->major_version+"."+tag->header->minor_version+
      	    "."+tag->header->sub_version;

    if(index == "friendly_values")
      return friendly_values;

    return tag[index];
  }

  //! @decl constant version
  //! The version of the encapsulated tag in the form @expr{"%d.%d.%d"@}.

  //! Indices will return the indices of the tag object.
  static array _indices() {
    return indices(tag);
  }

  //! Values will return the values of the tag object.
  static array _values() {
    return values(tag);
  }

  //! Returns tag values in a mapping. Only tag values that exists
  //! in ID3v1.1 is used. Nonexisting or undefined values will not
  //! appear in the mapping.
  //!
  //! @mapping
  //!    @member string "artist"
  //!      Takes its value from TPE1 or TP1 frames.
  //!    @member string "album"
  //!      Takes its value from TALB or TAL frames.
  //!    @member string "title"
  //!      Takes its value from TIT2 or TT2 frames.
  //!    @member string "genre"
  //!      Takes its value from TCON or TCM frames.
  //!    @member string "year"
  //!      Takes its value from TYER or TYE frames.
  //!    @member string "track"
  //!      Takes its value from TRCK or TRK frames.
  //!      The string may be either in the @expr{"%d"@} form or in
  //!      the @expr{"%d/%d"@} form.
  //! @endmapping
  mapping(string:string) friendly_values() {
    mapping(string:string) rv = ([]);

    if(tag->header->major_version == 1) {
      rv = mkmapping(tag->frames->id,
		     tag->frames->data->get_string());
      foreach(rv; string i; string v)
	if(v=="") m_delete(rv, i);
      return rv;
    }

    foreach(tag->frames, Frame fr)
      switch(fr->id) {
	case "TPE1":
        case "TP1":
	  catch(rv->artist = fr->data->get_string());
	  break;
        case "TALB":
        case "TAL":
	  catch(rv->album = fr->data->get_string());
	  break;
        case "TIT2":
        case "TT2":
	  catch(rv->title = fr->data->get_string());
          break;
	case "TCON":
        case "TCM":
	  catch(rv->genre = fr->data->get_string());
          break;
        case "TYER":
        case "TYE":
          catch(rv->year = fr->data->get_string());
          break;
        case "TRCK":
        case "TRK":
          catch(rv->track = fr->data->get_string());
          break;
      }
     return rv;
   }

  static string _sprintf(int t, mapping args) {
    return t=='O' && sprintf("Tag(%O)", tag);
  }

}
