inherit Image._XCF;

class PathPoint
{
  int type;
  float x;
  float y;
}

class Path
{
  string name;
  int ptype;
  int tattoo;
  int closed;
  int state;
  int locked;
  array (PathPoint) points = ({});
}

class Guide
{
  int pos;
  int vertical;
  void create(string data)
  {
    sscanf(data, "%4c%c", pos,vertical);
  }
}

class Parasite
{
  string name;
  int flags;
  string data;

  void create( string _n, int _f, string _d )
  {
    name = _n;
    data = _d;
    flags = _f;
  }
}

array(Parasite) decode_parasites( string data )
{
  array res = ({});
  while(strlen(data))
  {
    int slen, flags;
    string value, name;
    sscanf(data, "%4c", slen);
    name = data[..slen-2];
    data = data[slen..];
    sscanf(data, "%4c%4c", flags, slen);
    res += ({ Parasite( name,flags,data[8..slen+8-1] ) }); 
    data = data[slen+8..];
  }
  return res;
}

#define FLAG(X,Y) case PROP_##X: sscanf(p->data, "%4c", flags->Y); break;
#define INT(X,Y) case PROP_##X: sscanf(p->data, "%4c", Y); break;

class Hierarchy
{
  Image.image img;
  Image.image alpha;
  int width;
  int height;
  int bpp;
  
  void create( int x, int y, int bp, array tiles, int compression,
               Image.colortable cmap)
  {
    width = x;
    height = y;
    bpp = bp;
    img = Image.image( x, y, 0,0,0 );
    alpha = Image.image( x, y, 255,255,255 );
    switch(compression)
    {
     case COMPRESS_NONE:
     case COMPRESS_RLE:
       _decode_tiles( img,alpha,tiles,compression,bpp,cmap );
       break;
     default:
       error("Image tile compression type not supported\n");
    }
  }
}

int iid;
Hierarchy decode_image_data( mapping what, object i )
{
  Hierarchy h = 
    Hierarchy( what->width, what->height, what->bpp,
               what->tiles, i->compression, i->colormap );


#if 1
  object bg = Image.image( what->width, what->height )->test();
  bg = bg->paste_mask( h->img, h->alpha );
  rm("/tmp/xcftest_"+iid);
  Stdio.write_file( "/tmp/xcftest_"+iid++, Image.PNM.encode( bg ));
#endif
  return h;
}

class Channel
{
  string name;
  int width;
  int height;
  int opacity;
  int r, g, b;
  int tattoo;
  Hierarchy image_data;
  object parent;
  mapping flags = ([]);
  array (Parasite) parasites;

  void decode_properties( array properties )
  {
    foreach(properties, mapping p)
    {
      switch(p->type)
      {
       case PROP_ACTIVE_CHANNEL:
         parent->active_channel = this_object();
         break;
       case PROP_SELECTION:
         parent->selection = this_object();
         break;
         INT(OPACITY,opacity);
         FLAG(VISIBLE,visible);
         FLAG(LINKED,linked);
         FLAG(PRESERVE_TRANSPARENCY,preserve_transparency);
         FLAG(EDIT_MASK,edit_mask);
         FLAG(SHOW_MASKED,show_masked);
         INT(TATTOO,tattoo);
       case PROP_COLOR:
         sscanf( p->data, "%c%c%c", r, g, b);
         break;
         
       case PROP_PARASITES:
         parasites = decode_parasites( p->data );
         break;
      }
    }
  }


  void create( mapping d, object p )
  {

    parent = p;
    name = d->name;
    width = d->width;
    height = d->height;
    image_data = decode_image_data( d->image_data, parent );
    if(d->properties) decode_properties( d->properties );
  }
}


class LayerMask
{
  inherit Channel;
}

class Layer
{
  string name;
  int opacity;
  int type;
  int mode;
  int tattoo;
  mapping flags = ([]);
  int width, height;
  int xoffset, yoffset;
  array (Parasite) parasites;
  LayerMask mask;
  Hierarchy image;

  object parent;

  void decode_properties( array properties )
  {
    foreach( properties, mapping p)
    {
      switch(p->type)
      {
       case PROP_ACTIVE_LAYER:
         parent->active_layer = this_object();
         break;
       case PROP_SELECTION:
         parent->selection = this_object();
         break;
       INT(OPACITY,opacity);
       FLAG(VISIBLE,visible);
       FLAG(SHOW_MASKED,show_masked);
       INT(TATTOO,tattoo);
       case PROP_PARASITES:
         parasites = decode_parasites( p->data );
         break;
      }
    }
  }

  void create( mapping data, object pa )
  {
    parent = pa;
    name = data->name;
    type = data->type;
    width = data->width;
    height = data->height;
    decode_properties( data->properties );
    image = decode_image_data( data->image_data, pa );
    if(data->mask)
      mask = LayerMask( data->mask, pa );
  }
}



class GimpImage
{
  int width;
  int height;
  int compression;
  int type;
  int tattoo_state;
  float xres = 72.0;
  float yres = 72.0;
  int res_unit;
  Image.colortable colormap;
  Image.colortable meta_colormap;
  array(Layer) layers = ({});
  array(Channel) channels = ({});
  array(Guide) guides = ({});
  array(Parasite) parasites = ({});
  array(Path) paths = ({});

  Layer active_layer;
  Channel active_channel;
  Channel selection;


  static string read_point_bz1( string data, Path path )
  {
    object p = PathPoint( );
    int x, y;
    sscanf(data, "%4c%4c%4c%s", p->type, x, y);
    p->x = (float)x;
    p->y = (float)y;
    return data;
  }

  static string read_point_bz2( string data, Path path )
  {
    object p = PathPoint( );
    sscanf(data, "%4c%4F%4F%s", p->type, p->x, p->y);
    return data;
  }

  static string decode_one_path( string data, Path path )
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
       sscanf(data, "%4%4cc%s", path->ptype, path->tattoo, data );
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
    sscanf( data, "%4c%4c%s", last_selected_row, num_paths, data );
    while(num_paths--)
    {
      Path path = Path();
      data = decode_one_path( data, path );
      res += ({ path });
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
           meta_colormap = colormap = Image.colortable( p->data ); 
         else
           meta_colormap = Image.colortable( p->data );
         break;
       case PROP_COMPRESSION: compression = p->data[0];             break;
       case PROP_GUIDES:
         guides = Array.map(p->data/5,Guide);
         break;
       case PROP_RESOLUTION:
         sscanf( p->data, "%4f%4f", xres,yres);
         if (xres < 1e-5 || xres> 1e+5 || yres<1e-5 || yres>1e+5)
           xres = yres = 72.0;
         break;
       case PROP_TATTOO:   
         sscanf(p->data, "%4c", tattoo_state );     
         break;
       case PROP_PARASITES:
         parasites = decode_parasites( p->data );
         break;
       case PROP_UNIT:
         sscanf(p->data, "%4c", res_unit );     
         break;
       case PROP_PATHS:
         paths = decode_paths( p->data );
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
      layers += ({ Layer( l, this_object() ) });
    foreach(data->channels, mapping c )
      channels += ({ Channel( c, this_object() ) });
  }
}



GimpImage __decode( string|mapping what )
{
  if(stringp(what))
    what = ___decode( what );
  return GimpImage(what);
}


mapping _decode( string|mapping what )
{
  GimpImage data = __decode( what );
  /* This is rather non-trivial.. */
}


object decode( string what )
{
  return _decode( what )->image;
}
