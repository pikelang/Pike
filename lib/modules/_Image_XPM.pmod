inherit Image._XPM;
#if 0
#define TE( X )  write("XPM profile: %-20s ... ", (X));

int old_time = gethrtime();
#define TI() old_time = gethrtime();
#define TD( X ) do{\
 write("%3.02f\n", (gethrtime()-old_time)/1000000.0);\
 TE(X);  \
 old_time = gethrtime(); \
} while(0);
#else
#define TI()
#define TD(X)
#define TE(X)
#endif


mapping _decode( string what, void|mapping opts )
{
  array e;
  array data;
  mapping retopts = ([ ]);
  if(!opts)
    opts = ([]);

  TE("Scan for header");
  if(sscanf(what, "%*s/*%*[ \t]XPM%*[ \t]*/%*s{%s", what)  != 5)
    error("This is not a XPM image (1)\n");

  if(strlen(what)<100000)
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
  what = 0;
  int len = sizeof(data);
  int r, sp;

  TD("Trim");
  for(int i = 0; i<len; i++)
    if(strlen(data[i]) &&
       data[i][0] != '/' && 
       (sp=search(data[i], "\"")) != -1)
      data[r++] = (data[i][sp+1..]/"\"")[0];

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
  mixed colors = ([]);
  if(sizeof(data) < ncolors+2)
    error("Too few elements in array to decode color values\n");

  colors = sort(data[1..ncolors]);
  TD("Creating images");
  object i = Image.image( width, height );
  object a = Image.image( width, height,255,255,255 );
  mixed cs = opts->colorspace;
  array c;
  TD("Decoding image");
  for(int y = 0; y<height && y<sizeof(data); y++)
    _xpm_write_row( y, i, a, data[ncolors+y+1]/cpp, colors );
  TD("Done");
  retopts->image = i;
  retopts->alpha = a;
  return retopts;
}

mapping decode( string what )
{
  return _decode(what)->image;
}
