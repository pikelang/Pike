#pike __REAL_VERSION__

//! @decl array(int(0..255)) rgb_to_hsv(array(int(0..255)) rgb)
//! @decl array(int(0..255)) rgb_to_hsv(int(0..255) r, int(0..255) g, int(0..255) b)
//!
//! This function returns the HSV value of the color
//! described by the provided RGB value. It is essentially
//! calling Image.Color.rgb(r,g,b)->hsv().
//!
//! @seealso
//! @[Colors.hsv_to_rgb()]
//! @[Image.Color.Color.hsv()]
//!
array(int(0..255)) rgb_to_hsv(array(int(0..255))|int(0..255) r,
			      int(0..255)|void g, int(0..255)|void b)
{
  if(arrayp(r)) return Image.Color.rgb(@r)->hsv();
  return Image.Color.rgb(r,g,b)->hsv();
}

//! @decl array(int(0..255)) hsv_to_rgb(array(int(0..255)) hsv)
//! @decl array(int(0..255)) hsv_to_rgb(int(0..255) h, int(0..255) s, int(0..255) v)
//!
//! This function returns the RGB value of the color
//! described by the provided HSV value. It is essentially
//! calling Image.Color.hsv(h,s,v)->rgb().
//!
//! @seealso
//! @[Colors.rgb_to_hsv()]
//! @[Image.Color.hsv()]
//!
array(int(0..255)) hsv_to_rgb(array(int(0..255))|int(0..255) h,
			      int(0..255)|void s, int(0..255)|void v)
{
  if(arrayp(h)) return Image.Color.hsv(@h)->rgb();
  return Image.Color.hsv(h,s,v)->rgb();
}

//! @decl array(int(0..100)) rgb_to_cmyk(array(int(0..255)) rgb)
//! @decl array(int(0..100)) rgb_to_cmyk(int(0..255) r, int(0..255) g, int(0..255) b)
//!
//! This function returns the CMYK value of the color
//! described by the provided RGB value. It is essentially
//! calling Image.Color.rgb(r,g,b)->cmyk().
//!
//! @seealso
//! @[Colors.cmyk_to_rgb()]
//! @[Image.Color.Color.cmyk()]
//!
array(int(0..100)) rgb_to_cmyk(array(int(0..255))|int(0..255) r,
			       int(0..255)|void g, int(0..255)|void b)
{
  array(float) cmyk;
  if(arrayp(r))
    cmyk = Image.Color.rgb(@r)->cmyk();
  else
    cmyk = Image.Color.rgb(r,g,b)->cmyk();
  return (array(int))map(cmyk, round);
}

//! @decl array(int(0..255)) cmyk_to_rgb(array(int(0..100)) cmyk)
//! @decl array(int(0..255)) cmyk_to_rgb(int(0..100) c, int(0..100) m, int(0..100) y, int(0..100) k)
//!
//! This function return the RGB value of the color
//! describe by the provided CMYK value. It is essentially
//! calling Image.Color.cmyk(c,m,y,k)->rgb()
//!
//! @seealso
//! @[Colors.rgb_to_cmyk()]
//! @[Image.Color.cmyk()]
//!
array(int) cmyk_to_rgb(array(int)|int c, int|void m, int|void y, int|void k)
{
  if(arrayp(c)) return Image.Color.cmyk(@c)->rgb();
  return Image.Color.cmyk(c,m,y,k)->rgb();
}

//! This function returns the RGB values that corresponds to the
//! color that is provided by name to the function. It is
//! essentially calling @[Image.Color.guess()], but returns the
//! value for black if it failes.
//!
array(int(0..255)) parse_color(string name)
{
  Image.Color.Color color;
  if(!name || !strlen(name)) return ({ 0,0,0 }); // Odd color...

  if(color=Image.Color.guess(name)) return color->rgb();

  name = replace(lower_case(name), "gray", "grey");
  if(color=Image.Color.guess(name)) return color->rgb();

  // Lets call it black and be happy..... :-)
  return ({ 0,0,0 });
}

//! Tries to find a name to color described by  the provided RGB
//! values. Partially an inverse function to @[Colors.parse_color()],
//! although it can not find all the names that @[Colors.parse_color()]
//! can find RGB values for. Returns the colors rgb hex value prepended
//! with "#" upon failure.
//!
string color_name(array(int(0..255)) rgb)
{
  if(!arrayp(rgb) || sizeof(rgb)!=3) return "-";
  string name = Image.Color(@rgb)->name();
  return name;
}
