static mapping parse_color( array color )
{
  mapping res = ([]);
  for(int i=0; i<sizeof(color); i+=2 )
    if(lower_case(color[i+1]) != "none")
      res[color[i]] = Colors.parse_color( color[i+1] );
  return res;
}

static array find_color( mapping in, string space )
{
  return in && (in[space||"s"] || in->c || in->g || in->g4 || in->m);
} 

mapping _decode( string what, void|mapping opts )
{
  array data;
  mapping retopts = ([ ]);
  if(!opts)
    opts = ([]);
  if(sscanf(what, "%*s/*%*[ \t]XPM%*[ \t]*/%*[ \t]\n%s", what)  != 5)
    error("This is not a XPM image (1)\n");
  if(sscanf(what, "%*s{%s", what) != 2)
    error("This is not a XPM image (2)\n");

  sscanf(what, "%s\n/* XPM */", what ) ||
    sscanf(what, "%s\n/*\tXPM\t*/", what ) ||
    sscanf(what, "%s\n/*\tXPM */", what )||
    sscanf(what, "%s\n/* XPM */", what );

  what = reverse(what);
  if(sscanf(what, "%*s}%s", what) != 2)
    error("This is not a XPM image (3)\n");
  what = reverse(what);

  
  master()->set_inhibit_compile_errors(1);
  if(catch {
    data = compile_string( "mixed foo(){ return ({"+what+"}); }" )()->foo();
  })
    error("This is not a XPM image\n");
  master()->set_inhibit_compile_errors(0);
  
  array values = (array(int))(replace(data[0], "\t", " ")/" "-({""}));
  data = data[1..];
  int width = values[0];
  int height = values[1];
  int ncolors = values[2];
  int cpp = values[3];
  if(sizeof(values)>5)
  {
    retopts->hot_x = values[4];
    retopts->hot_y = values[5];
  }
  mapping colors = ([]);
  for(int i = 0; i<ncolors; i++ )
  {
    if(i > sizeof(data))
      error("Too few elements in array to decode color values\n");
    string color_id = data[i][0..cpp-1];
    array color = replace(data[i][cpp..], "\t", " ")/" "-({""});
    colors[color_id] = parse_color( color );
  }
  data = data[ncolors..];

  object i = Image.image( width, height );
  object a = Image.image( width, height,255,255,255 );

  for(int y = 0; y<height && y<sizeof(data); y++)
  {
    string line = data[y];
    for(int x = 0; x<width ; x++)
    {
      array c;
      if( c = find_color(colors[line[x*cpp..x*cpp+cpp-1]], opts->colorspace) )
        i->setpixel( x, y, @c );
      else
        a->setpixel( x, y, 0, 0, 0 );
    }
  }
  retopts->image = i;
  retopts->alpha = a;
  return retopts;
}

mapping decode( string what )
{
  return _decode(what)->image;
}
