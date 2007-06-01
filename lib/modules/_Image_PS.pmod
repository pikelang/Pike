#pike __REAL_VERSION__

//! @appears Image.PS
//! Codec for the Adobe page description language PostScript.
//! Uses Ghostscript for decoding or built-in support.

static string find_in_path( string file )
{
  string path=getenv("PATH");
  foreach(path ? path/":" : ({}) , path)
    if(file_stat(path=combine_path(path,file)))
      return path;
}

//! Decodes the postscript @[data] into an image object
//! using Ghostscript.
//! @param options
//!   Optional decoding parameters.
//!   @mapping
//!     @member int "dpi"
//!       The resolution the image should be rendered in.
//!       Defaults to 100.
//!     @member string "device"
//!       The selected Ghostscript device. Defaults to "ppmraw".
//!     @member string "binary"
//!       Path to the Ghostscript binary to be used. If missing
//!       the environment paths will be searched for a file "gs"
//!       to be used instead.
//!     @member int(0..1) "force_gs"
//!	   Forces use of Ghostscript for EPS files instead
//!	   of Pikes native support.
//!	@member int(0..1) "eps_crop"
//!	   Use -dEPSCrop option to Ghostscript to crop the
//!	   BoundingBox for a EPS file.
//!	@member int(0..1) "cie_color"
//!	   Use -dUseCIEColor option to Ghostscript for
//!	   mapping color values through a CIE color space.
//!     @member string "file"
//!        Filename to read. If this is specified, it will be
//!        passed along to the @tt{gs@} binary, so that it can
//!        read the file directly. If this is specified @[data]
//!        may be set to @expr{0@} (zero).
//!   @endmapping
//!
//! @note
//!   Some versions of @tt{gs@} on MacOS X have problems with
//!   reading files on stdin. If this occurrs, try writing to
//!   a plain file and specifying the @tt{file@} option.
//!
//! @note
//!   @tt{gs@} versions 7.x and earlier don't support rendering
//!   of EPSes if they are specified with the @tt{file@} option.
//!   If this is a problem, upgrade to @tt{gs@} version 8.x or
//!   later.
object decode( string data, mapping|void options )
{
  if(!options) options = ([]);
  if (data) {
    if(has_prefix(data, "\xc5\xd0\xd3\xc6")) {
      // DOS EPS Binary Header.
      int ps_start, ps_len, meta_start, meta_len, tiff_start, tiff_len, sum;
      sscanf(data, "%*4c%-4c%-4c%-4c%-4c%-4c%-4c%-2c",
	     ps_start, ps_len, meta_start, meta_len,
	     tiff_start, tiff_len, sum);
#if constant(Image.TIFF.decode)
      if (tiff_start && tiff_len) {
	return Image.TIFF.decode(data[tiff_start..tiff_start + tiff_len-1]);
      }
#endif
      data = data[ps_start..ps_start+ps_len-1];
    }
    if(data[0..3] != "%!PS")
      error("This is not a postscript file!\n");

    if(!options->force_gs)
    {
      if (has_prefix(data, "%!PS-Adobe-3.0 EPSF-3.0")) {
	int width, height, bits, ncols;
	int nbws, width2, encoding;
	string init_tag;
	if ((sscanf(data,
		    "%*s%%ImageData:%*[ ]%d%*[ ]%d%*[ ]%d%*[ ]%d%"
		    "*[ ]%d%*[ ]%d%*[ ]%d%*[ ]\"%s\"",
		    width, height, bits, ncols, 
		    nbws, width2, encoding, init_tag) > 7) &&
	    (width == width2) && (width > 0) && (height > 0) && (bits == 8) &&
	    (< 1, 2 >)[encoding]) {
	  // Image data present.
	  int len;
	  string term;
	  string raw;
	  if ((sscanf(data, "%*s%%%%BeginBinary:%*[ ]%d%[\r\n]%s",
		      len, term, raw) == 5) &&
	      (len>0) && has_prefix(raw, init_tag + term)) {
	    raw = raw[sizeof(init_tag+term)..len-1];
	    if (encoding == 2) {
	      // Decode the hex data.
#if constant(String.hex2string)
	      raw = String.hex2string(raw - term);
#else
	      raw += Crypto.hex_to_string(raw - term);
#endif
	    }
	    if (sizeof(raw) == width*height*(ncols+nbws)) {
	      array(string) rows = raw/width;
	      if ((<3,4>)[ncols]) {
		// RGB or CMYK image.
		array(string) channels = allocate(ncols, "");
		int c;
		for (c = 0; c < ncols; c++) {
		  int i;
		  for (i = c; i < sizeof(rows); i += ncols+nbws) {
		    channels[c] += rows[i];
		  }
		}
		if (ncols == 3) {
		  return Image.Image(width, height, "rgb", @channels);
		}
		return Image.Image(width, height, "adjusted_cmyk", @channels);
	      }
	      string grey = "";
	      int i;
	      for(i = ncols; i < sizeof(rows); i += ncols+nbws) {
		grey += rows[i];
	      }
	      return Image.Image(width, height, "rgb", grey, grey, grey);
	    }
	  }
	}
      }
    }
  }

  Stdio.File fd = Stdio.File();
  object fd2 = fd->pipe();

  Stdio.File fd3 = Stdio.File();
  object fd4 = fd3->pipe();

  array command = ({
    options->binary||find_in_path("gs")||("/bin/sh -c gs "),
    "-quiet",
    "-sDEVICE="+(options->device||"ppmraw"),
    "-r"+(options->dpi||100),
    "-dBATCH",
    "-dNOPAUSE"});

  if(options->eps_crop)
    command += ({"-dEPSCrop"});
  
  if(options->cie_color)
    command += ({"-dUseCIEColor"});
  
  command += ({
    "-sOutputFile=-",
    options->file || "-",
    "-c",
    "quit",
  });

  Process.Process pid = Process.create_process( command, ([
    "stdin":fd,
    "stdout":fd4,
    "stderr":fd4,
  ]));
  fd->close();
  fd4->close();

  // FIXME: Create a new backend instead of using the default.

  // Kill the gs binary after 30 seconds in case it hangs.
  mixed co = call_out(lambda(Process.Process pid) {
			if (!pid->status()) {
			  pid->kill(9);
			}
		      }, 30, pid);
  if(!options->file)
  {
    if(!has_value(data, "showpage"))
      data += "\nshowpage\n";
    Stdio.sendfile(({ data }), 0, 0, sizeof(data), 0, fd2,
		   lambda(int n) {
		     fd2->close();
		   });
  } else {
    fd2->close();
  }
  string output = "";
  fd3->set_nonblocking(lambda(mixed ignored, string s) {
			 output += s;
		       }, 0,
		       lambda() {
			 fd3->close();
			 destruct(fd3);
		       });
  mixed err = catch {
    while (fd3) {
      Pike.DefaultBackend(1.0);
    }
  };
#if 0
  if (err) {
    werror("Backend failed: %s\n", describe_backtrace(err));
  }
#endif
  remove_call_out(co);
  int ret_code = pid->wait();
  if(ret_code)
    error("Ghostscript failed with exit code: %O:\n%s\n", ret_code, output);
  object i= Image.PNM.decode( output );

  if (data) {

    if(data && sscanf(data, "%*s\n%%%%BoundingBox: %s\n", string bbox) == 2 && !options->eps_crop)
    {
      int x0,x1,y0,y1;
      sscanf(bbox, "%d %d %d %d", x0,y0,x1,y1 );
      int llx = (int)((x0/72.0) * (options->dpi||100)+0.01);
      int lly = (int)((y0/72.0) * (options->dpi||100)+0.01);
      int urx = (int)((x1/72.0) * (options->dpi||100)+0.01);
      int ury = (int)((y1/72.0) * (options->dpi||100)+0.01);

      if(urx && ury)
	i = i->mirrory()->copy(llx,lly,urx,ury)->mirrory();
    }
  }
  return i;
}

//! Calls decode and returns the image in the "image"
//! index of the mapping. This method is present for
//! API reasons.
mapping _decode( string data, mapping|void options )
{
  return ([ "image":decode( data,options ) ]);
}



//! Encodes the image object @[img] as a postscript 3.0 file.
//! @param options
//!   Optional extra encoding parameters.
//!   @mapping
//!     @member int "dpi"
//!       The resolution of the encoded image. Defaults to 100.
//!     @member int(0..1) "eps"
//!       If the resulting image should be an eps instead of ps.
//!       Defaults to 0, no.
//!   @endmapping
string encode(  object img, mapping|void options )
{
  int w = img->xsize(), h = img->ysize();
  string i = (string)img;
  float scl = 72.0 / ((options&&options->dpi)||100);
  img = 0;
  string res;
  res =("%!PS-Adobe-3.0\n"
        "%%DocumentData: Clean8Bit\n"
	"%%BoundingBox: 0 0 "+(int)ceil(w*scl)+" "+(int)ceil(h*scl)+"\n"
	"%%EndComments\n"
	"%%BeginProlog\n"
	"30 dict begin\n"
	"/tmpstr 256 string def\n"
	"/dnl{currentfile tmpstr readline pop pop}bind def\n"
	"/di{dnl gsave scale 2 copy pop string/pxdat exch def\n"
	" 2 copy 8 [4 2 roll dup 0 0 4 2 roll neg 0 3 2 roll] {currentfile\n"
	" pxdat readstring pop}false 3 colorimage grestore}bind def\n"
	"%%EndProlog\n");

  res += sprintf("%d %d %f %f di\n", w, h, scl*w, scl*h);
  res +="%%BeginData "+sizeof(i)+" Binary Bytes\n" + i +"\n%%EndData\n";

  if( !options || !options->eps )
    res += "showpage\n";
  res += "%%Trailer\n" "end\n" "%%EOF\n";
  return res;
}

//! Same as encode. Present for API reasons.
function _encode = encode;
