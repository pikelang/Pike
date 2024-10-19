#charset utf-8
#pike __REAL_VERSION__

//   Imagedimensionreadermodule for Pike.
//   Created by Johan Schön, <js@roxen.com>.
//
//   This software is based in part on the work of the Independent JPEG Group.

//! @appears Image.Dims
//! Reads the dimensions of images of various image formats without
//! decoding the actual image.

#define M_SOF0  0xC0		/* Start Of Frame N */
#define M_SOF1  0xC1		/* N indicates which compression process */
#define M_SOF2  0xC2		/* Only SOF0-SOF2 are now in common use */
#define M_SOF3  0xC3
#define M_SOF5  0xC5		/* NB: codes C4 and CC are NOT SOF markers */
#define M_SOF6  0xC6
#define M_SOF7  0xC7
#define M_SOF9  0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI   0xD8		/* Start Of Image (beginning of datastream) */
#define M_EOI   0xD9		/* End Of Image (end of datastream) */
#define M_SOS   0xDA		/* Start Of Scan (begins compressed data) */
#define M_COM   0xFE		/* COMment */
#define M_APP1  0xE1

protected int(0..255) read_1_byte(Stdio.Stream f)
{
  return f->read(1)[0];
}

protected int(0..65535) read_2_bytes(Stdio.Stream f)
{
  int c;
  sscanf( f->read(2), "%2c", c );
  return c;
}


/*
 * Read the initial marker, which should be SOI.
 * For a JFIF file, the first two bytes of the file should be literally
 * 0xFF M_SOI.  To be more general, we could use next_marker, but if the
 * input file weren't actually JPEG at all, next_marker might read the whole
 * file and then return a misleading error message...
 */

protected int first_marker(Stdio.Stream f)
{
  int c1, c2;

  sscanf(f->read(2), "%c%c", c1, c2);
  if (c1==0xFF||c2==M_SOI) return 1;
  return 0;
}

/*
 * Find the next JPEG marker and return its marker code.
 * We expect at least one FF byte, possibly more if the compressor used FFs
 * to pad the file.
 * There could also be non-FF garbage between markers.  The treatment of such
 * garbage is unspecified; we choose to skip over it but emit a warning msg.
 * NB: this routine must not be used after seeing SOS marker, since it will
 * not deal correctly with FF/00 sequences in the compressed image data...
 */

protected int next_marker(Stdio.Stream f)
{
  // Find 0xFF byte; count and skip any non-FFs.
  int c = read_1_byte(f);
  while (c != 0xFF)
    c = read_1_byte(f);

  // Get marker code byte, swallowing any duplicate FF bytes.  Extra FFs
  // are legal as pad bytes.
  do {
    c = read_1_byte(f);
  } while (c == 0xFF);
  return c;
}

/* Skip over an unknown or uninteresting variable-length marker */
protected int skip_variable(Stdio.Stream f)
{
  int length = read_2_bytes(f);
  if (length < 2) return 0;  // Length includes itself, so must be at least 2.
  length -= 2;
  f->seek(length, Stdio.SEEK_CUR);
  return 1;
}

/* Get the width, height and orientation from a JPEG file */
protected mapping|zero get_JPEG_internal(Stdio.BlockFile f)
{
  if (!first_marker(f))
    return 0;

  mapping exif;

  /* Scan miscellaneous markers until we reach SOS. */
  for (;;)
  {
    switch (next_marker(f)) {
    case M_SOF0:		/* Baseline */
    case M_SOF1:		/* Extended sequential, Huffman */
    case M_SOF2:		/* Progressive, Huffman */
    case M_SOF3:		/* Lossless, Huffman */
    case M_SOF5:		/* Differential sequential, Huffman */
    case M_SOF6:		/* Differential progressive, Huffman */
    case M_SOF7:		/* Differential lossless, Huffman */
    case M_SOF9:		/* Extended sequential, arithmetic */
    case M_SOF10:		/* Progressive, arithmetic */
    case M_SOF11:		/* Lossless, arithmetic */
    case M_SOF13:		/* Differential sequential, arithmetic */
    case M_SOF14:		/* Differential progressive, arithmetic */
    case M_SOF15:		/* Differential lossless, arithmetic */
      int image_height, image_width;
      sscanf(f->read(7), "%*3s%2c%2c", image_height, image_width);
      return ([ "width": image_width, "height": image_height, "exif": exif ]);
      break;

    case M_APP1:
      int pos = f->tell();
      f->seek(pos-2);
      catch {
        exif = Standards.EXIF.get_properties(f, ([ 0x0112 :
                                                   ({ "Orientation" }) ]));
      };
      f->seek(pos);
      if(!skip_variable(f)) return 0;
      break;

    case M_SOS:			/* stop before hitting compressed data */
      return 0;

    case M_EOI:			/* in case it's a tables-only JPEG stream */
      return 0;

    default:			/* Anything else just gets skipped */
      if(!skip_variable(f)) return 0; // we assume it has a parameter count...
      break;
    }
  }
}

//! Reads the dimensions from a JPEG file and returns an array with
//! width and height, or if the file isn't a valid image, 0.
array(int) get_JPEG(Stdio.BlockFile f)
{
  mapping result = get_JPEG_internal(f);
  return result && ({ result->width, result->height });
}

/* Perform EXIF based dimension flipping */
protected array(int) exif_flip(array(int) dims, mapping exif)
{
  if (arrayp(dims) && exif && has_index(exif, "Orientation"))
  {
    if ((< "5", "6", "7", "8" >)[exif->Orientation])
    {
      // Picture has been rotated/flipped 90 or 270 degrees, so
      // exchange width and height to reflect that.
      int tmp = dims[1];
      dims[1] = dims[0];
      dims[0] = tmp;
    }
  }
  return dims;
}

//! Like @[get_JPEG()], but returns the dimensions flipped if
//! @[Image.JPEG.exif_decode()] would flip them
array(int) exif_get_JPEG(Stdio.BlockFile f)
{
  mapping result = get_JPEG_internal(f);
  return result && exif_flip(({ result->width, result->height }), result->exif);
}

// GIF-header:
// typedef struct _GifHeader
// {
//   // Header
//   BYTE Signature[3];     /* Header Signature (always "GIF") */
//   BYTE Version[3];       /* GIF format version("87a" or "89a") */
//   // Logical Screen Descriptor
//   WORD ScreenWidth;      /* Width of Display Screen in Pixels */
//   WORD ScreenHeight;     /* Height of Display Screen in Pixels */
//   BYTE Packed;           /* Screen and Color Map Information */
//   BYTE BackgroundColor;  /* Background Color Index */
//   BYTE AspectRatio;      /* Pixel Aspect Ratio */
// } GIFHEAD;

//! Reads the dimensions from a GIF file and returns an array with
//! width and height, or if the file isn't a valid image, 0.
array(int)|zero get_GIF(Stdio.BlockFile f)
{
  if(f->read(3)!="GIF") return 0;
  f->seek(3, Stdio.SEEK_CUR);
  return array_sscanf(f->read(4), "%-2c%-2c");
}

//! Reads the dimensions from a PNG file and returns an array with
//! width and height, or if the file isn't a valid image, 0.
array(int)|zero get_PNG(Stdio.BlockFile f)
{
  int offs=f->tell();
  if(f->read(6)!="\211PNG\r\n") return 0;
  f->seek(6, Stdio.SEEK_CUR);
  if(f->read(4)!="IHDR") return 0;
  return array_sscanf(f->read(8), "%4c%4c");
}

//! Reads the dimensions from a TIFF file and returns an array with
//! width and height, or if the file isn't a valid image, 0.
array(int)|zero get_TIFF(Stdio.BlockFile f)
{
 int|string buf;
 int entries;
 int val = 0;
 string bo2b;
 string bo4b;

 buf = f->read(2);
 if(buf == "II")
 {
   /* Byte order for Little endian */
   bo2b = "%2-c";
   bo4b = "%4-c";
 }
 else if(buf == "MM")
 {
   /* Byte order for Big endian */
   bo2b = "%2c";
   bo4b = "%4c";
 }
 else
 {
   /* Not a TIFF */
   return 0;
 }

 sscanf(f->read(2), bo2b, buf);
 if(buf != 42)
 {
   /* Wrong magic number */
   return 0;
 }

 /* offset to first IFD */
 sscanf(f->read(4), bo4b, buf);
 f->seek(buf);

 /* number of entries */
 sscanf(f->read(2), bo2b, entries);

 for(int i = 0; i < entries; i++)
 {
   sscanf(f->read(2), bo2b, int tag);
   if(tag == 256 || tag == 257)
   {
     sscanf(f->read(2), bo2b, buf);
     /* Count value must be one(1) */
     sscanf(f->read(4), bo4b, int count);
     if(count == 1)
     {
       if(buf == 3)
       {
	 /* Type short */
	 sscanf(f->read(2), bo2b, buf);
	 /* Skip to the end of the entry */
         f->seek(2, Stdio.SEEK_CUR);
       }
       else if(buf == 4)
       {
	 /* Type long */
	 sscanf(f->read(4), bo4b, buf);
       }
       else
       {
	 /* Wrong type */
	 return 0;
       }

       if(tag == 256)
       {
	 /* ImageWidth */
	 if(val == 0)
	   val = buf;
	 else
	   return ({buf, val});
       }
       else
       {
	 /* ImageLength */
	 if(val == 0)
	   val = buf;
	 else
	   return ({val, buf});
       }
     }
     else
     {
       /* Wrong count value */
       return 0;
     }
   }
   else
   {
     /* Skip to next entry */
     f->seek(10, Stdio.SEEK_CUR);
   }
 }
 /* ImageWidth and ImageLength not found */
 return 0;
}

//! Reads the dimensions from a PSD file and returns an array with
//! width and height, or if the file isn't a valid image, 0.
array(int)|zero get_PSD(Stdio.BlockFile f)
{
  //  4 bytes signature + 2 bytes version
  if (f->read(6) != "8BPS\0\1") return 0;

  //  6 bytes reserved
  //  2 bytes channel count
  f->read(8);

  //  4 bytes height, 4 bytes width (big-endian)
  return reverse(array_sscanf(f->read(8), "%4c%4c"));
}

//! Reads the dimensions from a WebP file and returns an array with
//! width and height, or if the file isn't a valid image, 0.
array(int)|zero get_WebP(Stdio.BlockFile f)
{
  if (f->read(4) != "RIFF") return 0;
  f->read(4);
  if (f->read(4) != "WEBP") return 0;

  switch(f->read(4))
  {
  case "VP8 ":
    /* Lossy coding */
    f->read(4); // Size of chunk. Not in Google documentation.
    f->read(3); // Frame tag.
    if(f->read(3)!="\x9d\x01\x2a") return 0;
    // Ignore the scaling factor, as it is upscaling and not actual
    // image information.
    return array_sscanf(f->read(4), "%-2c%-2c")[*] & 0x3fff;
    break;
  case "VP8L":
    /* Lossless coding */
    f->read(4);
    if( f->read(1) != "/" ) return 0;

    string data = f->read(4);
    int width = (data[0] | (data[1] & 0x3f)<<8) + 1;
    int height = (data[1]>>6 | data[2]<<2 | (data[3] & 0x0f)<<10) + 1;
    return ({ width, height });
    break;
  case "VP8X":
    /* Extended VP8 */
    f->read(4);
    f->read(4); // flags and reserved
    return array_sscanf(f->read(6), "%-3c%-3c")[*]+1;
    break;
  }
  return 0;
}

//! Read dimensions from a JPEG, GIF, PNG, WebP, TIFF or PSD file and
//! return an array with width and height, or if the file isn't a
//! valid image, @expr{0@}. The argument @[file] should be file object
//! or the data from a file. The offset pointer will be assumed to be
//! at the start of the file data and will be modified by the
//! function.
//!
//! @returns
//!   @array
//!     @elem int 0
//!       Image width.
//!     @elem int 1
//!       Image height.
//!     @elem string 2
//!       Image type. Any of @expr{"gif"@}, @expr{"png"@}, @expr{"tiff"@},
//!       @expr{"jpeg"@}, @expr{"webp"@} and @expr{"psd"@}.
//!   @endarray
array(int|string)|zero get(string|Stdio.BlockFile file, int(0..1)|void exif) {

  if(stringp(file))
    file = Stdio.FakeFile(file);

  array ret;
  switch(file->read(2))
  {
  case "GI":
    if( (< "F87a", "F89a" >)[file->read(4)] )
      return array_sscanf(file->read(4), "%-2c%-2c") + ({ "gif" });
    break;

  case "\x89P":
    if(file->read(4)=="NG\r\n")
    {
      file->read(6+4); // offset+IHDR
      return array_sscanf(file->read(8), "%4c%4c") + ({ "png" });
    }
    break;

  case "8B":
    if(file->read(4)=="PS\0\1")
    {
      //  Photoshop PSD
      //
      //  4 bytes signature + 2 bytes version
      //  6 bytes reserved
      //  2 bytes channel count
      file->read(6+2);
      //  4 bytes height, 4 bytes width (big-endian)
      return reverse(array_sscanf(file->read(8), "%4c%4c")) + ({ "psd" });
    }
    break;

  case "II":
  case "MM":
    file->seek(-2, Stdio.SEEK_CUR);
    ret = get_TIFF(file);
    if(ret) return ret+({ "tiff" });
    break;

  case "\xff\xd8":
    file->seek(-2, Stdio.SEEK_CUR);
    ret = (exif? exif_get_JPEG(file) : get_JPEG(file));
    if(ret) return ret+({ "jpeg" });
    break;

  case "RI":
    file->seek(-2, Stdio.SEEK_CUR);
    ret = get_WebP(file);
    if(ret) return ret+({ "webp" });
    break;
  }

  return 0;
}

//! Like @[get()], but returns the dimensions flipped if
//! @[Image.JPEG.exif_decode()] would flip them
array(int) exif_get(string|Stdio.BlockFile file) {
  return get(file, 1);
}
