#pike __REAL_VERSION__

string find_in_path( string file )
{
  string path=getenv("PATH");
  foreach(path ? path/":" : ({}) , path)
    if(file_stat(path=combine_path(path,file)))
      return path;
}

object decode( string data, mapping|void options )
{
  if(!options) options = ([]);
  object fd, fd2;
  object fd3, fd4;
  string bbox;
  int llx, lly;
  int urx, ury;
  fd = Stdio.File();
  fd2 = fd->pipe();

  fd3 = Stdio.File();
  fd4 = fd3->pipe();

  if(data[0..3] != "%!PS")
    error("This is not a postscript file!\n");

  if(sscanf(data, "%*s\n%%%%BoundingBox: %s\n", bbox) == 2)
  {
    int x0,x1,y0,y1;
    sscanf(bbox, "%d %d %d %d", x0,y0,x1,y1 );
    llx = (int)((x0/72.0) * (options->dpi||100)+0.01);
    lly = (int)((y0/72.0) * (options->dpi||100)+0.01);
    urx = (int)((x1/72.0) * (options->dpi||100)+0.01);
    ury = (int)((y1/72.0) * (options->dpi||100)+0.01);
  }

  array command = ({
    find_in_path("gs")||("/bin/sh -c gs "),
    "-quiet",
    "-sDEVICE="+(options->device||"ppmraw"),
    "-r"+(options->dpi||100),
    "-dNOPAUSE",
    "-sOutputFile=-",
    "-",
    "-c quit 2>/dev/null" 
  });

//   werror("running "+(command*" ")+"\n");
  Process.create_process( command, ([
    "stdin":fd,
    "stdout":fd3,
    "stderr":fd3,
  ]));
  destruct(fd);
  destruct(fd3);
  fd2->write( data );
  if(search(data, "showpage") == -1)
    fd2->write( "\nshowpage\n" );
  destruct(fd2);
  object i= Image.PNM.decode( fd4->read() );
  
  if(urx && ury)
    i = i->mirrory()->copy(llx,lly,urx,ury)->mirrory();
  return i;
}

mapping _decode( string data, mapping|void options )
{
  return ([ "image":decode( data,options ) ]);
}




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

function _encode = encode;
