#pike __REAL_VERSION__

inherit Image._XCF;

#define SIGNED(X) if(X>=(1<<31)) X=-((1<<32)-X)

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
    sscanf(data, "%4c%c", pos,vertical);vertical--;
    SIGNED(pos);
  }
}

class Parasite( string name, int flags, string data ) { }

array(Parasite) decode_parasites( mixed data )
{
  array res = ({});
  data = (string)data;
  while(sizeof(data))
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

#define FLAG(X,Y) case PROP_##X: flags->Y=p->data->get_int(0); break;
#define INT(X,Y) case PROP_##X:  Y = p->data->get_uint( 0 ); break;
#define SINT(X,Y) case PROP_##X: Y = p->data->get_int( 0 ); break;

int id;

class Hierarchy( int width, int height, int bpp, array tiles,
                 int compression, Image.Colortable ct )
{
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

int iid;
Hierarchy decode_image_data( mapping what, object i )
{
  Hierarchy h =
    Hierarchy( what->width, what->height, what->bpp,
               what->tiles, i->compression, i->colormap );
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
  Image.Colortable colormap;
  Image.Colortable meta_colormap;
  array(Layer) layers = ({});            // bottom-to-top
  array(Channel) channels = ({});       // unspecified order, really
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
    SIGNED(x);
    SIGNED(y);
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
      layers += ({ Layer( l, this_object() ) });
    foreach(data->channels, mapping c )
      channels += ({ Channel( c, this_object() ) });
  }
}



GimpImage __decode( string|mapping what )
{
  if(stringp(what)) what = ___decode( what );
  return GimpImage(what);
}

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

array decode_layers( string|object|mapping what, mapping|void opts, 
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

mapping _decode( string|mapping|object(GimpImage) what, mapping|void opts )
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


object decode( string what,mapping|void opts )
{
  return _decode( what,opts )->image;
}
