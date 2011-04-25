// AutoCAD R13/R14/R2000 DWG file decoder
// $Id$

#pike __REAL_VERSION__

//! @appears Image.DWG
//! This module decodes the thumbnail raster images embedded in AutoCAD DWG files for
//! AutoCAD version R13, R14 and R2000 (which equals to file version 12, 14 and 15).
//! Implemented according to specifications from @url{http://www.opendwg.org/@}.

static constant start = "\x1F\x25\x6D\x07\xD4\x36\x28\x28\x9D\x57\xCA\x3F\x9D\x44\x10\x2B";

static inline int read_RL(string data, int pos) {
  int r;
  sscanf(data[pos..pos+3], "%-4c", r);
  return r;
}

//! Decodes the DWG @[data] into a mapping.
//! @mapping
//!   @member int "version"
//!     The version of the DWG file. One of 12, 14 and 15.
//!   @member array(string) "bmp"
//!     An array with the raw BMP data.
//!   @member array(string) "wmf"
//!     An array with the raw WMF data.
//! @endmapping
//!
//! @throws
//!   This functions throws an error when decoding somehow fails.
mapping __decode(string data) {

  // decode version id
  if( data[0..3]!="AC10" )
    error("Unknown format\n");
  int version = (int)data[4..5];
  if( !(< 12, 14, 15 >)[version] )
    error("Only DWG for AutoCAD R13, R14 and R2000 are supported."
	  " (File id %d)\n", version);

  // Get image data block offset
  int pos = read_RL(data, 0x0d);

  // Check sentinel
  if( data[pos..pos+15]!=start )
    error("Error while decoding DWG preview. (Corrupt sentinel)\n");
  pos += 16;

  // Image data block size
  pos += 4;

  // Image counter
  int i = data[pos++];

  array bmps  = ({});
  array wmfs = ({});

  for(; i; i--) {
    int(1..3) code = data[pos++];
    switch(code) {
    case 1:
      int h_start = read_RL(data, pos);
      pos += 4;
      int h_size = read_RL(data, pos);
      pos += 4;
      //      werror( "%O\n", sizeof(data[h_start..h_start+h_size-1]) );
      break;

    case 2:
      int bmp_start = read_RL(data, pos);
      pos += 4;
      int bmp_size = read_RL(data, pos);
      pos += 4;
      // For some reason AutoCAD thought it wise to skip the first
      // 14 bytes of the BMP file. "BM", 4 bytes data size, 4 bytes
      // reserved and 4 bytes offset to image data.
      string header = sprintf("BM%-4c\0\0\0\0\0\0\0\0", bmp_size+14);
      bmps += ({ header+data[bmp_start..bmp_start+bmp_size-1] });
      break;

    case 3:
      int wmf_start = read_RL(data, pos);
      pos += 4;
      int wmf_size = read_RL(data, pos);
      pos += 4;
      wmfs += ({ data[wmf_start..wmf_start+wmf_size-1] });
      break;

    default:
      error("Error while decoding DWG preview. (Unknown code %d)\n", code);
    }
  }

  return ([ "version" : version,
	    "bmp" : bmps,
	    "wmf" : wmfs ]);
}

static Image.Image get_first_image( mapping data ) {
  if( !sizeof(data->bmp) )
    error("No bitmap previews available.\n");
  foreach(data->bmp, string bmp) {
    catch {
      return Image.BMP.decode(bmp);
    };
  }
  error("Failed to decode any of the previews.\n");
}

//! Works like @[__decode], but in addition it has the element
//! @tt{"image"@} in the result mapping, containing the first
//! successfully decoded bitmap image.
//! to the result of decode(data).
//! @throws
//!   If no preview was stored, or no preview could
//!   be decoded an error is thrown.
mapping _decode(string data) {
  mapping res = __decode(data);
  res->image = get_first_image(res);
  return res;
}

//! Returns the first successfully decoded bitmap image.
//! @throws
//!   If no preview was stored, or no preview could
//!   be decoded an error is thrown.
Image.Image decode(string data) {
  return _decode(data)->image;
}
