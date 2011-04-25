// ID3.pmod
//
//  $Id$
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

class Buffer(Stdio.File buffer) {

  private string peek_data;
  private int limit;

  string read(int bytes) {
    if(limit) {
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

  string peek() {
    if(!peek_data) peek_data = buffer->read(1);
    return peek_data;
  }

  void set_limit(int bytes) {
    limit = bytes;
  }

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
  foreach(bytes, int byte)
    res = res << 7 | byte;
  return res;
}

//! Encodes a integer to a synchsafe integer according to
//! ID3v2.4.0-structure section 6.2.
//! @seealso
//!   @[synchsafe_to_int]
array(int) int_to_synchsafe(int in, void|int no_bytes) {
  array res = ({});
  while(in>>=7)
    res += ({ in&127 });
  return reverse(res);
}

class TagHeader {

  int(2..2)   major_version = 2;
  int(2..4)   minor_version = 4;
  int(0..255) sub_version = 0;

  int(0..1) flag_unsynchronisation = 0;
  int(0..1) flag_extended_header = 0;
  int(0..1) flag_experimental = 0;
  int(0..1) flag_footer = 0;

  int tag_size;

  void create(void|Buffer buffer) {
    if(buffer) decode(buffer);
  }

  void decode(Buffer buffer) {
    string data = buffer->read(10);
    if(!has_prefix(data, "ID3"))
      error( "Header has wrong identifier. Expected \"ID3\", got %O.\n", data[..2] );

    array bytes = (array(int))data[3..];
    if( ! (< 2, 3, 4 >)[bytes[0]] )
      error( "Can only handle ID3v2.{2,3,4}.x. Got ID3v2."+bytes[0]+"."+bytes[1]+"\n" );

    minor_version = bytes[0];
    sub_version = bytes[1];
    decode_flags(bytes[2]);
    tag_size = synchsafe_to_int( bytes[3..] );
  }

  void decode_flags( int byte ) {
    if(byte&0b1111)
      error( "Unknown flag set in tag header flag field.\n" );

    flag_unsynchronisation = TEST(byte,7);
    flag_extended_header = TEST(byte,6);
    flag_experimental = TEST(byte,5);
    flag_footer = TEST(byte,4);
  }

  int encode_flags() {
    return flag_unsynchronisation<<7 + flag_extended_header<<6 +
      flag_experimental<<5 + flag_footer<<4;
  }

  string encode() {
    return sprintf("ID3%c%c%c", minor_version, sub_version, encode_flags()) +
      (string)int_to_synchsafe(tag_size);
  }

  int set_flag_unsynchronisation(array(Frame) frames) {
    if(sizeof( frames->flag_unsynchronisation - ({ 0 }) ) == sizeof(frames))
      return flag_unsynchronisation = 1;
    return flag_unsynchronisation = 0;
  }
}

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
  TagHeader header;

  void create(void|Buffer buffer, void|TagHeader thd) {
    if(thd) header = thd;
    if(buffer) decode(buffer);
  }

  void decode_flags(string bytes) {
    int flags = bytes[0];
    flag_is_update = TEST(flags,6);
    flag_crc = TEST(flags,5);
    flag_restrictions = TEST(flags,4);
  }

  string encode_flags() {
    return sprintf("%c", flag_is_update<<6 + flag_crc<<5 +
		   flag_restrictions<<4);
  }

  string decode_restrictions(int data) {
    restr_size = (data & 0b11000000) >> 6;
    restr_encoding = TEST(data, 5);
    restr_field_size = (data & 0b00011000) >> 3;
    restr_img_enc = TEST(data, 2);
    restr_img_size = data & 0b00000011;
  }

  string encode_restrictions() {
    return sprintf("%c", restr_size<<6 + restr_encoding<<5 +
		   restr_field_size<<3 + restr_img_enc<<2 +
		   restr_img_size);
  }

  void decode(Buffer buffer) {
    size = synchsafe_to_int( (array)buffer->read(4) );
    int flagbytes = buffer->read(1)[0];
    decode_flags(buffer->read(flagbytes));

    if(flag_crc)
      crc = synchsafe_to_int((array(int))buffer->read(5));
    // FIXME: There is no validation of the CRC of the file...

    if(flag_restrictions)
      decode_restrictions(buffer->read(1)[0]);
  }

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
//! encodes all the strings with the same method, selected as in @[encode_string].
//! The first element in the resulting array is the selected method, while the
//! following elements are the encoded strings.
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


class FrameData {

  private string original_data;
  void create(void|string data) {
    original_data = data;
    if(data) decode(data);
  }

  int(0..1) changed() {
    return original_data && (encode() != original_data);
  }

  void decode(string data);
  string encode();
}

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
    if( (< "TIT1", "TIT2", "TIT3", "TALB", "TOAL", "TSST", "TSRC",
	   "TPE1", "TPE2", "TPE3", "TPE4", "TOPE", "TEXT", "TOLY",
	   "TCOM", "TENC", "TYER" >)[id] )
      return Frame_TextPlain;
    if( (< "TMCL", "TIPL" >)[id] )
      return Frame_TextMapping;
    if( (< "TRCK" >)[id] )
      return Frame_TRCK;
    // FIXME: Look for program in this module.
    return Frame_Dummy;
  }

  void decode_flags( int flag0, int flag1 ) {
    flag_tag_alteration = flag0 & 1<<6;
    flag_file_alteration = flag0 & 1<<5;
    flag_read_only = flag0 & 1<<4;

    if(flag0 & 0b10001111)
      error( "Unknown flag in frame flag field.\n" );

    flag_grouping = flag1 & 1<<6;
    flag_compression = flag1 & 1<<3;
    flag_encryption = flag1 & 1<<2;
    flag_unsynchronisation = flag1 & 1<<1;
    flag_dli = flag1 & 1;

    if(flag1 & 0b10110000)
      error( "Unknown flag in frame flag field.\n" );
  }

  array(int) encode_flags() {
    return ({ flag_tag_alteration<<6 + flag_file_alteration<<5 + flag_read_only<<4,
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

    data = get_frame_data(id)( _data );
  }

  mapping get() {
    switch(id) {
    default:
      throw( ({ "No frame identifier available.\n", backtrace() }) );
    }
  }

  void encode() {
    //hop@string block = data;
    mixed block = data;

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

    return id + (string)int_to_synchsafe(sizeof(block)) + 
      sprintf("%c%c", @encode_flags()) +
      block;
  }
}

//! ID3 version 2 (2.2, 2.3, 2.4) Tags
class Tagv2 {

  TagHeader header;
  ExtendedHeader extended_header;
  array(Frame) frames = ({});
  int padding;

  void create(void|Buffer buffer) {
    if(buffer) {
      decode(buffer);
      return;
    }
    header = TagHeader();
    extended_header = ExtendedHeader();
  }

  void decode(Buffer buffer) {
    buffer->set_limit(10);
    header = TagHeader(buffer);
    buffer->set_limit(header->tag_size);
    if(header->flag_extended_header)
      extended_header = ExtendedHeader(buffer, header);
    while(buffer->bytes_left() && buffer->peek()!= "\0")
      frames += ({ Frame(buffer, header) });
    padding = buffer->bytes_left();

    // Verify that padding is all zero.
    if(padding && `+(@(array(int))buffer->read(padding)))
      error( "Non-zero value in padding.\n" );

    // FIXME: footer
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
}

array terminator = ({ "\0", "\0\0", "\0" });


// --- Tags -----------------------------------------------------------------------

class Frame_UFID {
  inherit FrameData;

  string system;
  string id;

  void decode(string data) {
    if( sscanf(data, "%s\0%s", system, id) != 2 )
      error( "Malformed UFID frame.\n" );
  }

  string encode() {
    if(!system) error( "Can not encode UFID frame. Missing owner identifier (system).\n" );
    if(!id) error( "Can not encode UFID frame. Missing identifier.\n" );
    return system + "\0" + id;
  }
}

class Frame_TextPlain {
  inherit FrameData;

  private array(string) texts;

  void decode(string data) {
    int encoding;
    if(!sizeof(data)) error( "Malformed text frame. Missing encoding byte.\n" );
    sscanf(data, "%c%s", encoding, data);
    texts = map(data / terminator[encoding], decode_string, encoding);
  }

  void encode() {
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
}

class Frame_TextAofB {
  inherit FrameData;

  array(int) low_decode(string data) {
    if(!sizeof(data)) error( "Malformed text frame. Missing encoding byte.\n" );
    data = decode_string(data[1..], data[0]);
    int a,b;
    sscanf(data, "%s\0", data);
    sscanf(data, "%d/%d", a, b);

    // Not against the spec, really...
    if(a > b) error( "(A/B) A bigger than B.\n" );
    if(!a) error( "Element is zero.\n" );

    return ({ a, b });
  }

  string low_encode(int a, int b) {
    if(a) return "\0"+a+"/"+b;
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
}

class Frame_TextMapping {
  inherit FrameData;
  // FIXME: Order is not preserved.

  mapping list = ([]);

  void decode(string data) {
    if(!sizeof(data)) error( "Malformed text frame. Missing encoding byte.\n" );
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
#endif

}


//! ID3 version 1.0 or 1.1 tag
class Tagv1 {

  array frames = ({});
  mapping header =
    ([ "major_version": 1,
       "minor_version": 0,
       "sub_version": 0
    ]);

  void create(Stdio.File file) {

    object st = file->stat();
    string buf;
    int i;

    if(!st)
      error("File not open.\n");
    if(st->size < 130)
      error("File is too small.\n");
    if(file->seek(st->size - 128) < 0)
      error("File seek error.\n");
    if(!(buf = file->read(128)) || sizeof(buf) < 128)
      error("File read error.\n");
    if(buf[..2] != "TAG")
      error("Tag is missing.\n");

    buf = buf[3..];
    foreach(({ ({ 0, 29, "title" }), ({ 30, 59, "artist" }),
    	       ({ 60, 89, "album" }), ({ 90, 93, "year" }),
	       ({ 94, 123, "comment" }), ({ 124, 124, "genre" }) }),
	    array el1)
      frames += ({ Framev1(buf[el1[0]..el1[1]], el1[2]) });

    if(buf[122] == 0 && buf[123] != 0) {
      frames += ({ Framev1(buf[123..123], "track") });
      header->minor_version = 1;
    }

  }

}

class Framev1 {
  
  string id;
  int size;
  FrameDatav1 data;

  void create(string buffer, string name) {
    id = name;
    data = FrameDatav1(buffer, name);
    size = strlen(buffer);
  }

}

class FrameDatav1 {
  private string frame_data;
  private string id;

  void create(string buffer, string name) {
    int i;

    frame_data = buffer;
    id = name;
    if((i = search(buffer, "\0")) > -1)
      frame_data = i ? buffer[..i-1] : "";
  }

  string get_string() {
    if(id == "track" || id == "genre")
      return (string)frame_data[0];
    return frame_data;
  }

}

//! ID3 tag object
//!
//! Tries to find version 2 tags in file and then version 1 tag.
//!
//! @note
//!  Version 1 tag is searched only if version 2 isn't there.
//!
//! @seealso
//!  @[Tagv2], @[Tagv1]
class Tag {

  object tag;

  void create(Stdio.File fd) {

    catch(tag = Tagv2(Buffer(fd)));
    if(tag)
      return;
    catch(tag = Tagv1(fd));
    if(tag)
      return;
    error("No ID3 tag was found in file.\n");
  }

  mixed `->(string index) {

    if(index == "version")
      return ""+tag->header->major_version+"."+tag->header->minor_version+
      	    "."+tag->header->sub_version;

    if(index == "friendly_values")
      return friendly_values;

    return tag[index];
  }

  mixed _indices() {
    return indices(tag);
  }

  //! Returns tag values in friendly manner
  //!
  //! @note
  //!  Only version 1 equivalent of version 2 tags
  //!  are returned.
  mapping friendly_values() {
    if(tag->header->major_version == 1)
      return mkmapping(tag->frames->id,
		       tag->frames->data->get_string());

    mapping rv = ([]);
    foreach(tag->frames, object fr1)
      switch(upper_case(fr1->id)) {
	case "TPE1":
        case "TP1":
	  catch(rv->artist = fr1->data->get_string());
	  break;
        case "TALB":
        case "TAL":
	  catch(rv->album = fr1->data->get_string());
	  break;
        case "TIT2":
        case "TT2":
	  catch(rv->title = fr1->data->get_string());
          break;
	case "TCON":
        case "TCM":
	  catch(rv->genre = fr1->data->get_string());
          break;
        case "TYER":
        case "TYE":
          catch(rv->year = fr1->data->get_string());
          break;
        case "TRCK":
        case "TRK":
          catch(rv->track = fr1->data->get_string());
          break;
      }
     return rv;
   }

}
