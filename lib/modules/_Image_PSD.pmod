inherit Image._PSD;

class Layer
{
  string mode;
  int opacity;
  object image;
  object alpha;
  
  int flags;
  int xoffset, yoffset;
  int width, height;

  int mask_flags;
  int mask_xoffset, mask_yoffset;
  int mask_width, mask_height;


  Layer copy()
  {
    Layer l = Layer();
    l->mode = mode;
    l->opacity = opacity;
    l->image = image;
    l->alpha = alpha;
    l->flags = flags;
    l->xoffset = xoffset;
    l->yoffset = yoffset;
    l->width = width;
    l->height = height;
    return l;
  }


  Layer get_opaqued( int opaque_value )
  {
    Layer res = copy();
    if(opaque_value != 255)
    {
      if(res->alpha)
        res->alpha *= opaque_value/255.0;
      else
        res->alpha = Image.image(width,height,
                                 opaque_value,opaque_value,opaque_value);
    }
    return res;
  }
}

int foo;
Layer decode_layer(mapping layer, mapping i)
{
  int stt = gethrtime();
  Layer l = Layer();
  int use_cmap;
  l->opacity = layer->opacity;
  l->width = layer->right-layer->left;
  l->height = layer->bottom-layer->top;
  l->xoffset = layer->left;
  l->yoffset = layer->top;
  l->image = Image.image( l->width, l->height );
  l->mode = layer->mode;
  l->flags = layer->flags;

  l->mask_width = layer->mask_right-layer->mask_left;
  l->mask_height = layer->mask_bottom-layer->mask_top;
  l->mask_xoffset = layer->mask_left;
  l->mask_yoffset = layer->mask_top;

  if( !(l->mask_flags & 1 ) ) // pos relative to layer
  {
    l->mask_xoffset -= l->xoffset;
    l->mask_yoffset -= l->yoffset;
  }
  array colors;
  int inverted;

  switch(i->mode)
  {
   case RGB:
     array lays = ({});
     foreach( layer->channels, mapping c )
     {
       string mode;
       switch( (int)c->id )
       {
        case 0:
          mode = "red"; 
          break;
        case 1:
          mode = "green"; 
          break;
        case 2:
          mode = "blue"; 
          break;
       }
       if( mode )
       {
         int st = gethrtime();
         if( !sizeof(lays) )
           lays += ({ 
             Image.Layer(___decode_image_channel(l->width, l->height,
                                                 c->data))
           });
         else
           lays += (({ Image.Layer( ([
             "image":___decode_image_channel(l->width, l->height, c->data),
             "alpha_value":1.0,
             "mode":mode, 
           ]) )
           }));
         werror(mode+" took %4.5f seconds\n", (gethrtime()-st)/1000000.0 );
         c->data = 0;
       }
     }
     int st = gethrtime();
     l->image = Image.lay( lays )->image();
     werror("combine took %4.5f seconds\n", (gethrtime()-st)/1000000.0 );
     break;

   case CMYK:
     inverted = 1;
     colors = ({ ({255,0,0,}),
                 ({0,255,0,}),
                 ({0,0,255,}),
               }) + ({ 255,255,255 }) * 24;
     l->image = Image.image( l->width, l->height, 255, 255, 255);
     break;

   case Indexed:
     use_cmap = 1;
     break;
   default:
     werror("Unsupported layer format mode (for now), using greyscale\n");
   case Greyscale:
     colors = ({
       255,255,255
     })*24;
     break;
  }
  int st = gethrtime();
  foreach(layer->channels, mapping c)
  {
    object tmp;

    if( !colors && (c->id >= 0 ))
      continue;

    if( c->id != -2)
      tmp = ___decode_image_channel(l->width, l->height, c->data);
    else
      tmp = ___decode_image_channel(l->mask_width,l->mask_height,c->data);
    switch( c->id )
    {
     default:
       if(!use_cmap)
       {
         if(inverted)
           l->image -= tmp*colors[c->id%sizeof(colors)];
         else
           l->image += tmp*colors[c->id%sizeof(colors)];
       } else {
         __apply_cmap( tmp, i->color_data );
         l->image = tmp;
       }
       break;
     case -1: /* alpha */
       if(!l->alpha)
         l->alpha = tmp;
       else
         l->alpha *= tmp;
       break;
     case -2: /* user mask */
       if(!(l->mask_flags & 2 )) /* layer mask disabled */
       {
         array pad_color = ({255,255,255});
         if( (l->mask_flags & 4 ) ) /* invert mask */
           tmp = tmp->invert();

         tmp = tmp->copy( -l->mask_xoffset, -l->mask_yoffset, 
                          l->image->xsize()-1, l->image->ysize()-1,
                          @pad_color )
           ->copy(0,0,l->image->xsize()-1,l->image->ysize()-1,
                  @pad_color);
         if(!l->alpha)
           l->alpha = tmp;
         else
           l->alpha *= tmp;
         break;
       }
    }
    c->data = 0;
  }
  werror("alpha/mask took %4.5f seconds\n", (gethrtime()-st)/1000000.0 );
  werror("TOTAL took %4.5f seconds\n\n", (gethrtime()-stt)/1000000.0 );
  return l;
}

mapping __decode( mapping|string what, mapping|void options )
{
  mapping data;
  if(mappingp(what))
    data = what;
  else 
    data = ___decode( what );
  what=0;
  array rl = ({});
  foreach( data->layers, mapping l )
    rl += ({ decode_layer( l, data ) });

  data->layers = rl;
  return data;
}


array(object) decode_background( mapping data, array bg )
{
  object img;
  if( data->image_data )
    img = ___decode_image_data(data->width,       data->height, 
                               data->channels,    data->mode,
                               data->compression, data->image_data,
                               data->color_data);
  return ({ img, 0 });
}

string translate_mode( string mode )
{
  switch( mode )
  {
   case "norm":      return "normal";
   case "mul ":      return "multiply";
   case "add ":      return "add";
   case "diff":      return "difference";
   case "sub ":      return "subtract";
   case "diss":      return "dissolve";
   case "scrn":      return "screen";
   case "over":      return "overlay";


   case "div ":      return "divide"; //?
   case "idiv":      return "divide"; //?
   case "dark":      return "min";
   case "lite":      return "max";

   case "hue ":      return "hue";
   case "sat ":      return "saturation";
   case "colr":      return "color";
   case "lum ":      return "value";

   case "smud":     
     werror("WARNING: Unsupported mode (smudge). Skipping layer\n");
     return 0;

// WARNING: PSD: Unsupported mode: sLit
// WARNING: PSD: Unsupported mode: hLit


   default:
     werror("WARNING: PSD: Unsupported mode: "+mode+"\n");
     werror("Skipping layer\n");
     return 0;
  }
}

array decode_layers( string|mapping what, mapping|void opts )
{

  if(!opts) opts = ([]);

  if(!mappingp( what ) )
    what = __decode( what );
  
  mapping lopts = ([ "tiled":1, ]);
  if( opts->background )
  {
    lopts->image = Image.Image( 32, 32, opts->background );
    lopts->alpha = Image.Image( 32, 32, Image.Color.white );
    lopts->alpha_value = 1.0;
  } else {
    lopts->image = Image.Image( 32, 32, Image.Color.black );
    lopts->alpha = Image.Image( 32, 32, Image.Color.black );
    lopts->alpha_value = 0.0;
  }

  [object img,object alpha] = decode_background( what, opts->background );
  if( img )
  {
    lopts->image = img;
    if( alpha )
      lopts->alpha = alpha;
    else
      lopts->alpha = 0;
    lopts->alpha_value = 1.0;
  }
  array layers = ({ Image.Layer( lopts ) });

  foreach(reverse(what->layers), object l)
  {
    if( string m = translate_mode( l->mode ) )
    {
      Image.Layer lay = Image.Layer( l->image,
                                   l->alpha,
                                   m );
      l->image = 0; l->alpha = 0;
      lay->set_alpha_value( l->opacity / 255.0 );
      lay->set_offset( l->xoffset, l->yoffset );
      layers += ({ lay });
    }
  }
  return layers;
}

mapping _decode( string|mapping what, mapping|void opts )
{
// mixed e = catch{
  mapping data;
  if(!opts) opts = ([]);
  if(mappingp(what))
    data = what;
  else 
    data = __decode( what );
  what=0;

  Image.Layer res = Image.lay(decode_layers( data, opts ),
                              0,0,data->width,data->height );
  Image.Image img = res->image();
  Image.Image alpha = res->alpha();

  return 
  ([
    "image":img,
    "alpha":alpha,
  ]);
// };
//  werror(describe_backtrace(e));
}
