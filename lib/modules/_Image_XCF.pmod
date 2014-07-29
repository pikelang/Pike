#pike __REAL_VERSION__

//! @appears Image.XCF
//! eXperimental Computing Facility (aka GIMP native) format.

inherit Image._XCF;

#define SIGNED(X) if(X>=(1<<31)) X=-((1<<32)-X)


//! @decl constant NORMAL_MODE;
//! @decl constant DISSOLVE_MODE;
//! @decl constant BEHIND_MODE;
//! @decl constant MULTIPLY_MODE;
//! @decl constant SCREEN_MODE;
//! @decl constant OVERLAY_MODE;
//! @decl constant DIFFERENCE_MODE;
//! @decl constant ADDITION_MODE;
//! @decl constant SUBTRACT_MODE;
//! @decl constant DARKEN_ONLY_MODE;
//! @decl constant LIGHTEN_ONLY_MODE;
//! @decl constant HUE_MODE;
//! @decl constant SATURATION_MODE;
//! @decl constant COLOR_MODE;
//! @decl constant VALUE_MODE;
//! @decl constant DIVIDE_MODE;
//! @decl constant REPLACE_MODE;
//! @decl constant ERASE_MODE;
//!
//! The GIMP image layer modes

//! @decl constant Red;
//! @decl constant Green;
//! @decl constant Blue;
//! @decl constant Gray;
//! @decl constant Indexed;
//! @decl constant Auxillary;
//!
//! The GIMP channel types


class PathPoint
//! A point on a path
{

  int type;
//!

  float x;
  float y;
//!

}

class Path
//! A path 
{

  string name;
//!

  int ptype;
//!

  int tattoo;
//!

  int closed;
//!

  int state;
//!

  int locked;
//!

  array (PathPoint) points = ({});
//!
}

class Guide
//! A single guideline
{
  int pos;
//!

  int vertical;
//!

  void create(string data)
  {
    sscanf(data, "%4c%c", pos,vertical);vertical--;
    SIGNED(pos);
  }
}

//!
class Parasite( string name, int flags, string data ) { }

array(Parasite) decode_parasites( mixed data )
{
  array res = ({});
  data = (string)data;
  while(sizeof(data))
  {
    int slen, flags;
    sscanf(data, "%4c", slen);
    string name = data[..slen-2];
    data = data[slen..];
    sscanf(data, "%4c%4c", flags, slen);
    res += ({ Parasite( name,flags,data[8..slen+8-1] ) });
    data = data[slen+8..];
  }
  return res;
}

#define FLAG(X,Y) case PROP_##X: flags->Y=p->data->get_int(0); break;
#define INT(X,Y) case PROP_##X:  Y = p->data->get_uint( 0 ); break;
#define SINT(X,Y) case PROP_##X: Y = p->data->get_int( 0 ); break;

//!
class Hierarchy( int width, int height, int bpp, array tiles,
                 int compression, Image.Colortable ct )
{

//!
  Image.Layer get_layer( int|void shrink )
  {
    Image.Image img, alpha;
    if(!shrink)
      shrink = 1;
    img = Image.Image( width/shrink, height/shrink );
    if( !(bpp & 1 ) )
      alpha = Image.Image( width/shrink, height/shrink );
    _decode_tiles( img,alpha,tiles,compression,bpp,ct, shrink,width,height );
    return Image.Layer( img, alpha );
  }
}

Hierarchy decode_image_data( mapping what, object i )
{
  Hierarchy h =
    Hierarchy( what->width, what->height, what->bpp,
               what->tiles, i->compression, i->colormap );
  return h;
}

class Channel
//!
{
  string name;
//!

  int width;
  int height;
//!

  int opacity;
//!

  int r, g, b;
//!

  int tattoo;
//!

  Hierarchy image_data;
//!

  object parent;
//!

  mapping flags = ([]);
//!

  array (Parasite) parasites;
//!

  void decode_properties( array properties )
  {
    foreach(properties, mapping p)
    {
      switch(p->type)
      {
       case PROP_ACTIVE_CHANNEL:
         parent->active_channel = this;
         break;
       case PROP_SELECTION:
         parent->selection = this;
         break;
         INT(OPACITY,opacity);
         FLAG(VISIBLE,visible);
         FLAG(LINKED,linked);
         FLAG(PRESERVE_TRANSPARENCY,preserve_transparency);
         FLAG(EDIT_MASK,edit_mask);
         FLAG(SHOW_MASKED,show_masked);
         INT(TATTOO,tattoo);
       case PROP_COLOR:
         sscanf( (string)p->data, "%c%c%c", r, g, b);
         break;

       case PROP_PARASITES:
         parasites = decode_parasites( (string)p->data );
         break;
      }
    }
  }


  void create( mapping d, object p )
  {

    parent = p;
    name = (string)d->name;
    width = d->width;
    height = d->height;
    image_data = decode_image_data( d->image_data, parent );
    if(d->properties) decode_properties( d->properties );
  }
}


class LayerMask
//!
{
  inherit Channel;
}

//!
class Layer
{
//!
  string name;

//!
  int opacity;

//!
  int type;

//!
  int mode;

//!
  int tattoo;

//!
  mapping(string:int) flags = ([]);

//!
  int width, height;

//!
  int xoffset, yoffset;

//!
  array (Parasite) parasites;

//!
  LayerMask mask;

//!
  Hierarchy image;

//!
  GimpImage parent;

  void decode_properties( array properties )
  {
    foreach( properties, mapping p)
    {
      switch(p->type)
      {
       case PROP_ACTIVE_LAYER:
         parent->active_layer = this;
         break;
       case PROP_SELECTION:
         parent->selection = this;
         break;
       case PROP_OFFSETS:
         xoffset = p->data->get_int( 0 );
         yoffset = p->data->get_int( 1 );
         break;
       INT(OPACITY,opacity);
       FLAG(VISIBLE,visible);
       FLAG(LINKED,linked);
       FLAG(PRESERVE_TRANSPARENCY,preserve_transparency);
       FLAG(APPLY_MASK,apply_mask);
       FLAG(EDIT_MASK,edit_mask);
       FLAG(SHOW_MASK,show_mask);
       INT(MODE,mode);
       INT(TATTOO,tattoo);
       case PROP_PARASITES:
         parasites = decode_parasites( (string)p->data );
         break;
      }
    }
  }

  void create( mapping data, object pa )
  {
    parent = pa;
    name = (string)data->name;
    type = data->type;
    width = data->width;
    height = data->height;
    decode_properties( data->properties );
    image = decode_image_data( data->image_data, pa );
    if(data->mask)
      mask = LayerMask( data->mask, pa );
  }
}

//!
class GimpImage
{

  int width;
  int height;
//!

  int compression;
//!

  int type;
//!

  int tattoo_state;
//!

  float xres = 72.0;
  float yres = 72.0;
//!

  int res_unit;
//!

  Image.Colortable colormap;
//!

  Image.Colortable meta_colormap;
//!

  array(Layer) layers = ({});            // bottom-to-top
//!

  array(Channel) channels = ({});       // unspecified order, really
//!

  array(Guide) guides = ({});
//!

  array(Parasite) parasites = ({});
//!

  array(Path) paths = ({});
//!

  Layer active_layer;
//!

  Channel active_channel;
//!

  Channel selection;
//!

  protected string read_point_bz1( string data, Path path )
  {
    object p = PathPoint( );
    int x, y;
    sscanf(data, "%4c%4c%4c%s", p->type, x, y, data);
    SIGNED(x);
    SIGNED(y);
    p->x = (float)x;
    p->y = (float)y;
    return data;
  }

  protected string read_point_bz2( string data, Path path )
  {
    object p = PathPoint( );
    sscanf(data, "%4c%4F%4F%s", p->type, p->x, p->y, data);
    return data;
  }

  protected string decode_one_path( string data, Path path )
  {
    int nlen, version, num_points;
    sscanf(data, "%4c", nlen );
    path->name = data[..nlen-2];
    data = data[nlen..];
    sscanf(data, "%4c%4c%4c%4c%4c",
           path->locked, path->state, path->closed, num_points, version);
    switch(version)
    {
     case 1:
       while(num_points--)
         data = read_point_bz1( data, path );
       break;
     case 2:
       sscanf(data, "%4c%s", path->ptype, data );
       while(num_points--)
         data = read_point_bz2( data, path );
       break;
     case 3:
       sscanf(data, "%4c%4c%s", path->ptype, path->tattoo, data );
       while(num_points--)
         data = read_point_bz2( data, path );
       break;
     default:
       data ="";
    }
    return data;
  }

  array(Path) decode_paths( string data )
  {
    int last_selected_row;
    int num_paths;
    array res = ({});
    if( stringp( data ) )
    {
      sscanf( data, "%4c%4c%s", last_selected_row, num_paths, data );
      while(num_paths--)
      {
        Path path = Path();
        data = decode_one_path( data, path );
        res += ({ path });
      }
    }
    return res;
  }

  void decode_properties(array props)
  {
    foreach( props, mapping p)
    {
      switch(p->type)
      {
       case PROP_COLORMAP:
         if(type == INDEXED)
           meta_colormap = colormap = Image.Colortable( (string)p->data );
         else
           meta_colormap = Image.Colortable( (string)p->data );
         break;
       case PROP_COMPRESSION:
         compression = ((string)p->data)[0];
         break;
       case PROP_GUIDES:
         guides = Array.map(((string)p->data)/5,Guide);
         break;
       case PROP_RESOLUTION:
         sscanf( (string)p->data, "%4F%4F", xres,yres);
         if (xres < 1e-5 || xres> 1e+5 || yres<1e-5 || yres>1e+5)
           xres = yres = 72.0;
         break;
       case PROP_TATTOO:
         tattoo_state = p->data->get_int( 0 );
         break;
       case PROP_PARASITES:
         parasites = decode_parasites( (string)p->data );
         break;
       case PROP_UNIT:
         res_unit = p->data->get_int( 0 );
         break;
       case PROP_PATHS:
         paths = decode_paths( (string)p->data );
         break;
       case PROP_USER_UNIT:
         /* NYI */
         break;
      }
    }
  }

  void create( mapping data )
  {
    type = data->type;
    decode_properties( data->properties );
    width = data->width;
    height = data->height;
    foreach(data->layers, mapping l )
      layers += ({ Layer( l, this ) });
    foreach(data->channels, mapping c )
      channels += ({ Channel( c, this ) });
  }
}


//!
GimpImage __decode( string|mapping what )
{
  if(stringp(what)) what = ___decode( what );
  return GimpImage(what);
}

//! Return information about the given image (xsize, ysize and type).
//! Somewhat inefficient, it actually decodes the image completely.
mapping decode_header( string|mapping|object(GimpImage) data )
{
  if( !objectp(data) )
  {
    if( stringp( data ) ) data = ___decode( data );
    data = GimpImage( data );
  }
  return ([
    "type":"image/x-gimp-image",
    "xsize":data->width,
    "ysize":data->height,
  ]);
}

//! Convert a GIMP layer mode to an @[Image.Layer] layer mode
string translate_mode( int mode )
{
  switch( mode )
  {
   case NORMAL_MODE:      return "normal";
   case MULTIPLY_MODE:    return "multiply";
   case ADDITION_MODE:    return "add";
   case DIFFERENCE_MODE:  return "difference";
   case SUBTRACT_MODE:    return "subtract";
   case DIVIDE_MODE:      return "divide";
   case DISSOLVE_MODE:    return "dissolve";
   case DARKEN_ONLY_MODE: return "min";
   case LIGHTEN_ONLY_MODE:return "max";
   case HUE_MODE:         return "hue";
   case SATURATION_MODE:  return "saturation";
   case COLOR_MODE:       return "color";
   case VALUE_MODE:       return "value";
   case SCREEN_MODE:      return "screen";
   case OVERLAY_MODE:     return "overlay";

   default:
     werror("WARNING: XCF: Unsupported mode: "+mode+"\n");
     return "normal";
  }
}

//! @decl array(Image.Layer) decode_layers( string gimp_image_data, mapping(string:mixed)|void opts )
//! @decl array(Image.Layer) decode_layers( GimpImage gimp_image, mapping(string:mixed)|void opts )
//! @decl array(Image.Layer) decode_layers( mapping(string:mixed) gimp_image_chunks, mapping(string:mixed)|void opts )
//!
//! Decode the image data given as the first argument. If it is a
//! string, it is a gimp image file.
//!
//! If it is a mapping it is the value you get when calling @[___decode()]
//!
//! The options can contain one or more of these options:
//! @mapping
//! @member bool draw_all_layers
//!   If included, all layers will be decoded, even the non-visible ones.
//! @member Image.Color background
//!   If included, include a solid background layer with the given color
//! @member int shrink_fact
//!   Shrink the image by a factor of X. Useful for previews.
//! @endmapping
//!
//! The layers have a number of extra properties set on them:
//!
//! @mapping
//! @member int image_xres
//! @member int image_yres
//! @member Image.Colormap image_colormap
//! @member array(Guide) image_guides
//! @member array(Parasite) image_parasites
//!  Values, global to all layers, copied from the GimpImage.
//!  Still present in all layers
//!
//! @member string name
//! @member bool visible
//! @member bool active
//! @member int tatoo
//! @member array(Parasite) parasites
//! @endmapping
array(Image.Layer) decode_layers( string|GimpImage|mapping what, mapping|void opts,
                                  int|void concat )
{
  if(!opts) opts = ([]);
  int shrink = (opts->shrink_fact||1);

  if(!objectp( what ) )
    what = __decode( what );

  array layers = ({});
  if( opts->background )
  {
    mapping lopts = ([ "tiled":1, ]);
    lopts->image = Image.Image( 32, 32, opts->background );
    lopts->alpha = Image.Image( 32, 32, Image.Color.white );
    lopts->alpha_value = 1.0;
    layers = ({ Image.Layer( lopts ) });
  }

  foreach(what->layers, object l)
    if((l->flags->visible && l->opacity > 0) || opts->draw_all_layers)
    {
      Image.Layer lay = l->image->get_layer( shrink );
      lay->set_mode( translate_mode( l->mode ) );
      if( l->opacity != 255 )
      {
        if( lay->alpha() )
          lay->set_image( lay->image(), lay->alpha()*(l->opacity/255.0) );
        else
          lay->set_image( lay->image(), Image.Image( lay->xsize(),
                                                     lay->yszize(),
                                                     l->opacity,
                                                     l->opacity,
                                                     l->opacity));
      }
      lay->set_offset( l->xoffset/shrink, l->yoffset/shrink );

      if(l->mask && l->flags->apply_mask)
      {
        object a = lay->alpha();
        object a2 =l->mask->image_data->get_layer(shrink)->image();
        int x = lay->image()->xsize( );
        int y = lay->image()->ysize( );
        if( a2->xsize() != x || a2->ysize() != y )
          a2 = a2->copy( 0,0, x, y, 255,255,255 );

        if(a)
          a *= l->mask->image_data->get_layer(shrink)->image();
        else
          a = a2;
        lay->set_image( lay->image(), a );
      }
      layers += ({ lay });


      if(!concat) /* No reason to set these flags if they are just
                     going to be ignored. */
      {
        /* Not really layer related */
        lay->set_misc_value( "image_xres", l->parent->xres/shrink );
        lay->set_misc_value( "image_yres", l->parent->yres/shrink );
        lay->set_misc_value( "image_colormap", l->parent->colormap );
        lay->set_misc_value( "image_guides", l->parent->guides );
        lay->set_misc_value( "image_parasites", l->parent->parasites );

        /* But these are. :) */
        lay->set_misc_value( "name", l->name );
        lay->set_misc_value( "tattoo", l->tattoo );
        lay->set_misc_value( "parasites", l->parasites );
        lay->set_misc_value( "visible", l->flags->visible );
        if( l == l->parent->active_layer )
          lay->set_misc_value( "active", 1 );
      }
    }

  return layers;
}

//! @decl mapping(string:Image.Image|string) _decode( string gimp_image_data, mapping(string:mixed)|void opts )
//! @decl mapping(string:Image.Image|string) _decode( GimpImage gimp_image, mapping(string:mixed)|void opts )
//! @decl mapping(string:Image.Image|string) _decode( mapping(string:mixed) gimp_image_chunks, mapping(string:mixed)|void opts )
//!
//! Decode the image data given as the first argument. If it is a
//! string, it is a gimp image file.
//!
//! If it is a mapping it is the value you get when calling @[___decode()]
//!
//! The options can contain one or more of these options:
//!
//! @mapping
//! @member bool draw_all_layers
//!   If included, all layers will be decoded, even the non-visible ones.
//! @member int shrink_fact
//!   Shrink the image by a factor of X. Useful for previews.
//! @member bool draw_guides
//!   If true, draw the vertical and horizontal guides
//! @member bool mark_layers
//!   If true, draw boxes around layers
//! @member bool mark_layer_names
//!   If true, draw layer names in the image
//! @member bool mark_active_layer
//!   If true, highlight the active layer
//! @endmapping
//!
//! The return value has this format:
//! @mapping
//! @member string type
//! "image/x-gimp-image"
//! @member Image.Image img
//! The image
//! @member Image.Image alpha
//! The alpha channel
//! @endmapping
mapping(string:string|Image.Image) _decode( string|mapping|GimpImage what, mapping(string:mixed)|void opts )
{
  if(!opts) opts = ([]);
  GimpImage data;
  if( objectp( what ) )
    data = what;
  else
    data = __decode( what );
  what = 0;

  int shrink = (opts->shrink_fact||1);
  Image.Layer res = Image.lay(decode_layers( data, opts, 1 ),
                              0,0,data->width/shrink,data->height/shrink );
  Image.Image img = res->image();
  Image.Image alpha = res->alpha();
  res = 0;

  if(opts->draw_guides)
    foreach( data->guides, Guide g )
      if(g->vertical)
      {
        img->line( g->pos/shrink, 0, g->pos/shrink, img->ysize(), 0,155,0 );
        if( alpha )
          alpha->line( g->pos/shrink,0,g->pos/shrink,img->ysize(), 255,255,255 );
      }
      else
      {
        img->line( 0,g->pos/shrink,  img->xsize(), g->pos/shrink, 0,155,0 );
        if( alpha )
          alpha->line(  0,g->pos/shrink, img->xsize(),g->pos/shrink, 255,255,255 );
      }

//   if(opts->draw_selection)
//     if(data->selection)
//       img->paste_alpha_color( data->selection->image_data->img*0.25,
//                               data->selection->r, data->selection->g,
//                               data->selection->b );

  if(opts->mark_layers)
  {
    foreach(data->layers, Layer l)
    {
      if(l->flags->visible || opts->draw_all_layers)
      {
        int x1 = l->xoffset/shrink;
        int x2 = (l->xoffset+l->width)/shrink;
        int y1 = (l->yoffset)/shrink;
        int y2 = (l->yoffset+l->height)/shrink;
        img->setcolor(0,0,255);
        img->line( x1,y1,x2,y1 );
        img->line( x2,y1,x2,y2 );
        img->line( x2,y2,x1,y2 );
        img->line( x1,y2,x1,y1 );
        if(alpha)
        {
          alpha->setcolor(0,0,255);
          alpha->line( x1,y1,x2,y1 );
          alpha->line( x2,y1,x2,y2 );
          alpha->line( x2,y2,x1,y2 );
          alpha->line( x1,y2,x1,y1 );
        }
      }
    }
  }

  if(opts->mark_layer_names)
  {
    foreach(data->layers, Layer l)
    {
      if(l->flags->visible || opts->draw_all_layers)
      {
        int x, y;
        int x1 = l->xoffset/shrink;
        int y1 = (l->yoffset+3)/shrink;
        object a = opts->mark_layer_names->write( l->name );
        for( x=-1; x<2; x++ )
          for( y=-1; y<2; y++ )
          {
            img->paste_alpha_color( a, 0,0,0, x1+x, y1+y );
            if(alpha)
              alpha->paste_alpha_color(a,255,255,255, x1+x,y1+y );
          }
        img->paste_alpha_color( a, 255,255,255, x1, y1 );
      }
    }
  }

  if(opts->mark_active_layer)
  {
    if(data->active_layer)
    {
      int x1 = data->active_layer->xoffset/shrink;
      int x2 = (data->active_layer->xoffset+data->active_layer->width)/shrink;
      int y1 = data->active_layer->yoffset/shrink;
      int y2 = (data->active_layer->yoffset+data->active_layer->height)/shrink;
      img->setcolor(255,0,0);
      img->line( x1,y1,x2,y1 );
      img->line( x2,y1,x2,y2 );
      img->line( x2,y2,x1,y2 );
      img->line( x1,y2,x1,y1 );
      if(alpha)
      {
        alpha->setcolor(0,0,255);
        alpha->line( x1,y1,x2,y1 );
        alpha->line( x2,y1,x2,y2 );
        alpha->line( x2,y2,x1,y2 );
        alpha->line( x1,y2,x1,y1 );
      }
    }
  }
//   Array.map( data->layers, lambda(object o) { destruct(o); } );
//   destruct( data );
  return
  ([
    "type":"image/x-gimp-image",
    "image":img,
    "alpha":alpha,
  ]);
}

//! See @[_decode]. This function returns the image member of the
//! return mapping of that function.
Image.Image decode( string what,mapping|void opts )
{
  return _decode( what,opts )->image;
}

// --- Encoding

#if 0

#define PROP(X,Y) sprintf("%4c%4c%s", PROP_##X, sizeof(Y), (Y))
#define UINT(X)   sprintf("%4c", (X))
#define STRING(X) sprintf("%4c%s\0", sizeof(X)+1, (X))

protected string make_hiearchy(Image.Image img) {
  string data = "";
  data += UINT(img->xsize());
  data += UINT(img->ysize());
  data += UINT(3); // rgb

  // Make just one tile
  string i = (string)img;
  string tile = "";
  tile += UINT(img->xsize());
  tile += UINT(img->ysize());
  tile += UINT(sizeof(i));
  tile += i;

  data += UINT(sizeof(tile));
  data += tile;
  return data;
}

protected int make_mode(string mode) {
  switch(mode) {
  case "normal": return NORMAL_MODE;
  }
  werror("Mode %O not supported in XCF.\n", mode);
  return NORMAL_MODE;
}

protected string make_layer(Image.Image|Image.Layer img) {
  string data = "";
  data += UINT(img->xsize());
  data += UINT(img->ysize());
  data += UINT(1); // FIXME: layer type
  if(img->get_misc_value)
    data += STRING(img->get_misc_value("name"));
  else
    data += STRING(" ");

  // Layer properties
  {
    // ACTIVE_LAYER
    // SELECTION

    if(img->xoffset && img->yoffset)
      data += PROP(OFFSETS, UINT(img->xoffset()) + UINT(img->yoffset()));

    if(img->alpha_value)
      data += PROP(OPACITY, UINT(255*img->alpha_value()));
    else
      data += PROP(OPACITY, UINT(255));

    data += PROP(VISIBLE, UINT(1));

    // LINKED
    // PRESERVE_TRANSPARENCY
    // APPLY_MASK
    // EDIT_MASK
    // SHOW_MASK

    if(img->mode)
      data += PROP(MODE, UINT(make_mode(img->mode)));
    else
      data += PROP(MODE, UINT(NORMAL_MODE));

    // TATTOO
    // PARASITES

    data += PROP(END, "");
  }

  string h;
  if(img->image)
    h = make_hiearchy(img->image());
  else
    h = make_hiearchy(img);
  string lm = ""; // make_layer_mask

  data += UINT(sizeof(h)); // hiearchy size
  data += UINT(sizeof(lm)); // layer mask size

  data += lm;
  data += h;

  return data;
}

string encode(Image.Image img) {
  String.Buffer buf = String.Buffer();
  buf->add("gimp xcf file\0");

  // width, height, type
  buf->add( sprintf("%4c%4c%4c", img->xsize(), img->ysize(), 0) );

  // Properties

  // PROP_COLORMAP
  // PROP_GUIDES
  // PROP_RESOLUTION
  // PROP_TATTOO
  // PROP_PARASITES
  // PROP_UNIT
  // PROP_PATHS
  // PROP_USER_UNIT

  buf->add( PROP(COMPRESSION, UINT(0)) );
  buf->add( PROP(END,"") );

  // Layers
  if(objectp(img)) {
    string lay = make_layer(img);
    buf->add( UINT(sizeof(lay)) );
    buf->add(lay);
  }
  //  foreach(indices(layers), Image.Layer layer) {
  //  }
  buf->add( UINT(0) );

  // Channels
  buf->add( UINT(0) );

  return (string)buf;
}

#endif
