#pike __REAL_VERSION__

//! @appears Image.XPM
//! X PixMap image format.
//! Specifically, XPM3.
//!
//! XPM1 and XPM2 are not supported.  (XPM3 has been the standard since
//! 1991).

inherit Image._XPM;
#if 0
# define TE( X )  werror("XPM profile: %-20s ... ", (X));
int old_time,start_time;
# define TI(X) start_time=old_time=gethrtime();TE(X)
# define TD( X ) do{\
 werror("%3.02f (%3.02f)\n", (gethrtime()-old_time)/1000000.0,(gethrtime()-start_time)/1000000.0);\
 TE(X);  \
 old_time = gethrtime(); \
} while(0);
#else
# define TI(X)
# define TD(X)
# define TE(X)
#endif

//! @decl mapping _decode(string bytes, mapping|void options)
//! @belongs Image.XPM
//!
//! Decode the given XPM image.
//!
//! @mapping
//! @member Image.Image "img"
//! @member Image.Image "alpha"
//! @member string "format"
//! @endmapping
//!
//! No options are currently supported.

mapping _decode( string what, mapping opts = ([]) )
{
  array data;
  mapping retopts = ([ ]);
  retopts->format = "image/xpm";
  TI("Scan for header");
  if(sscanf(what, "%*s/*%*[ \t]XPM%*[ \t]*/%*s{%s", what)  != 5)
    error("This is not a XPM image (1)\n");

  if(sizeof(what)<100000)
  {
    TD("Extra scan for small images");
    sscanf(what, "%s\n/* XPM */", what ) ||
      sscanf(what, "%s\n/*\tXPM\t*/", what ) ||
      sscanf(what, "%s\n/*\tXPM */", what )||
      sscanf(what, "%s\n/* XPM */", what );

    /* This is just a sanity check... */
    what = reverse(what);
    if(sscanf(what, "%*s}%s", what) != 2)
      error("This is not a XPM image (3)\n");
    what = reverse(what);
  }

  TD("Explode");
  data = what/"\n";
  what = "";
  int len = sizeof(data);

  TD("Trim");
  _xpm_trim_rows( data );

  array values = (array(int))(replace(data[0], "\t", " ")/" "-({""}));
  int width = values[0];
  int height = values[1];
  if(!width || !height)
    error("Width or height == 0\n");

  int ncolors = values[2];
  int cpp = values[3];
  if(sizeof(values)>5)
  {
    retopts->hot_x = values[4];
    retopts->hot_y = values[5];
  }
  TD("Colors");
  if(sizeof(data) < ncolors+2)
    error("Too few elements in array to decode color values\n");
  array colors;

  colors = sort(data[1..ncolors]);
  TD("Creating images");
  object i = Image.Image( width, height );
  object a = Image.Image( width, height,255,255,255 );
  TD("Decoding image");

  _xpm_write_rows( i,a,cpp,colors,data );

  TD("Done");

  retopts->image = i;
  retopts->alpha = a;
  return retopts;
}


constant ok = ({
  "`",  ".",  "+",  "@",  "#",  "$",  "%",  "*",  "=",  "-",  ";",  ">",  ",",
  "'",  ")",  "!",  "~",  "{",  "]",  "^",  "/",  "(",  "_",  ":",  "<",  "[",
  "}",  "|",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "0",  "a",
  "b",  "c",  "d",  "e",  "f",  "g",  "h",  "i",  "j",  "k",  "l",  "m",  "n",
  "o",  "p",  "q",  "r",  "s",  "t",  "u",  "v",  "w",  "x",  "y",  "z",  "A",
  "B",  "C",  "D",  "E",  "F",  "G",  "H",  "I",  "J",  "K",  "L",  "M",  "N",
  "O",  "P",  "Q",  "R",  "S",  "T",  "U",  "V",  "W",  "X",  "Y",  "Z",  " ",
});

private array|zero cmap_t;

//! @decl string encode(Image.Image image, mapping|void options)
//! @belongs Image.XPM
//!
//! Encode the given image as a XPM image.
//!
//! This will, at least currently, always produce a
//! 2-characters-per-pixel XPM.
//!
//! The supported options are:
//! @mapping
//! @member string(1..128) "name"
//! @member Image.Colortable "colortable"
//! @member Image.Image "alpha"
//! @member string(1..128) "comment"
//! @endmapping

string encode( object what, mapping options = ([]) )
{
  int x,y,q;
  TI("Encode init");
  if(!cmap_t)
  {
    cmap_t = allocate( 8100 );
    for(x=0; x<90; x++)
      for(y=0; y<90; y++)
        cmap_t[q++] = ok[x]+ok[y];
  }
  TD("Encode CT");
  if(!options->colortable)
  {
    options->colortable = Image.Colortable( what, 8089 );
    options->colortable->rigid( 25, 25, 25 );
    options->colortable->floyd_steinberg();
  }
  if(!options->name)
    options->name = "image";
  int alpha_used;
  TD("Encode map");
  array rows=options->colortable->index(what)/what->xsize();

  TD("Encode colors");
  int ncolors;
  array colors = ({});
  int n;
  foreach(((string)options->colortable)/3, mixed c)
  {
    colors += ({sprintf("%s c #%02x%02x%02x",cmap_t[n++],c[0],c[1],c[2])});
    ncolors++;
  }

  TD("Encode alpha");
  if(options->alpha)
  {
    object ac = Image.Colortable( ({ Image.Color.white, Image.Color.black }) );
    array q = ac->index( options->alpha )/what->xsize();
    string alpha_color = " ";
    string minus_ett = " ";
    alpha_color[0] = ncolors;
    minus_ett[0] = -1;
    for(y=0; y<sizeof(q); y++)
      rows[y]=replace(rows[y]|replace(q[y],"\1",minus_ett),minus_ett,alpha_color );
  }
  if(alpha_used)
  {
    colors += ({cmap_t[ncolors]+" c None"});
    ncolors++;
  }


  TD("Encode color output");
  string res =
    "/* XPM */\n"+
    (options->comment?
     "/* "+replace(options->comment, ({"/*", "*/"}), ({"/", "/"}))+" */\n":"")+
    "static char* "+options->name+" = {\n"
    "\""+what->xsize()+" "+what->ysize()+" "+ncolors+" 2\",\n";
  foreach(colors, string c)
    res += "\""+c+"\",\n";
  TD(sprintf("Encode %d rows", sizeof(rows)));

  q = sizeof(rows);
  foreach(rows, string row)
  {
    string r = "";
    int i;
    r += "\"";
    for(i=0; i<sizeof(row); i++)
      r += cmap_t[row[i]];
    res += r+"\",\n";
  }

  TD(sprintf("Encoded %d rows", sizeof(rows)));
  res = res+"};\n";
  TE("Done");
  return res;
}

//! @decl Image.Image decode(string bytes)
//! @belongs Image.XPM

object decode( string what )
{
  return _decode(what)->image;
}
