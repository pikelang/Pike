#pike __REAL_VERSION__

/* Note: I'm following the convention from Image.XCF here to not
   document all optional arguments. Makes the functions look easier to
   use to the casual Pike programmer, so it might be a good idea, but
   I'm not sure. /Zino */

//! @appears Image.PSD

//! @ignore
 inherit Image._PSD;
//! @endignore

class Layer
{
  string mode, name;
  int opacity;
  object image;
  object alpha;
  
  int flags;
  int xoffset, yoffset;
  int width, height;

  int mask_flags;
  int mask_xoffset, mask_yoffset;
  int mask_width, mask_height;
}


int foo;
Layer decode_layer(mapping layer, mapping i)
{
//   int stt = gethrtime();
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
  l->name = layer->name;
 
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
//          int st = gethrtime();
         if( !sizeof(lays) )
           lays += ({ 
             Image.Layer(___decode_image_channel(l->width,
						 l->height,
                                                 c->data))
           });
         else
           lays += (({ Image.Layer( ([
             "image":___decode_image_channel(l->width, l->height, c->data),
//             "alpha_value":1.0,
             "mode":mode, 
	   ]) )
           }));
//          werror(mode+" took %4.5f seconds\n", (gethrtime()-st)/1000000.0 );
         c->data = 0;
       }
     }
//      int st = gethrtime();
     l->image = Image.lay( lays )->image();
//      werror("combine took %4.5f seconds\n", (gethrtime()-st)/1000000.0 );
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
     werror("Unsupported layer format mode ("+i->mode+"), using greyscale\n");
   case Greyscale:
     colors = ({
       255,255,255
     })*24;
     break;
  }
//   int st = gethrtime();
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
//   werror(" mode %s image %O alpha %O\n",
// 	 l->mode, l->image, l->alpha );
//   werror("alpha/mask took %4.5f seconds\n", (gethrtime()-st)/1000000.0 );
//   werror("TOTAL took %4.5f seconds\n\n", (gethrtime()-stt)/1000000.0 );
  return l;
}

//! @decl mapping __decode(string|mapping data)
//! 
//! Decodes a PSD image to a mapping.
/* FIXME: Someone with free time could check psd.c for what is
   acctually in the mapping. See XCF.___decode for reference, but note
   that it is not the same mapping. */
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


array(object) decode_background( mapping data )
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
   case "dark":      return "min";
   case "lite":      return "max";
   case "hue ":      return "hue";
   case "div ":      return "idivide";
   case "hLit":      return "hardlight";

   case "colr":      return "color"; // Not 100% like photoshop.
   case "lum ":      return "value"; // Not 100% like photoshop.
   case "sat ":      return "saturation";// Not 100% like photoshop.

      // Colorburn, not really multiply, but the best aproximation soo far.
    case "idiv":
     return "multiply";

     // Exclusion. Not really difference, but very close.
    case "smud":     
     return "difference";

     // Soft light. Not really supported yet. For now, use hardlight with lower
     // opacity. Gives a rather good aproximation.
    case "sLit":     
     return "hardlight";

   default:
     werror("WARNING: PSD: Unsupported mode: "+mode+". Skipping layer\n");
     return 0;
  }
}

//! @decl array(Image.Layer) decode_layers( string data, mapping|void options )
//!
//! Decodes a PSD image to an array of Image.Layer objects
//!
//! Takes the same aptions mapping as @[_decode], note especially 
//! "draw_all_layers":1.
//!
//! The layer object have the following extra variables (to be queried
//! using @[Image.Layer()->get_misc_value]):
//!
//! @string
//!   @value "image_guides"
//!     Returns array containing guide definitions.
//!   @value "name"
//!     Returns string containing the name of the layer.
//!   @value "visible"
//!     FIXME: Undocumented.
//!   @value "active"
//!     FIXME: Undocumented.
//! @endstring
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
  }

  object img, alpha;
  if( !what->layers || !sizeof(what->layers))
  {
    [ img, alpha ] = decode_background( what );
    if( img )
    {
      lopts->image = img;
      if( alpha )
        lopts->alpha = alpha;
      else
        lopts->alpha = 0;
      lopts->alpha_value = 1.0;
    }
  }
  array layers;
  Image.Layer lay;
  if( lopts->image )
  {
    layers = ({ (lay=Image.Layer( lopts )) });
    lay->set_misc_value( "visible", 1 );
    lay->set_misc_value( "name",    "Background" );
    lay->set_misc_value( "image_guides", what->resources->guides );
  }
  else
    layers = ({});

  foreach(reverse(what->layers), object l)
  {
    if( !(l->flags & LAYER_FLAG_INVISIBLE) || opts->draw_all_layers )
    {
      if( string m = translate_mode( l->mode ) )
      {
	lay = Image.Layer( l->image, l->alpha, m );

        lay->set_misc_value( "visible", !(l->flags & LAYER_FLAG_INVISIBLE) );
        lay->set_misc_value( "name",      l->name );
        lay->set_misc_value( "image_guides", what->resources->guides );
	if( l->mode == "sLit" )
	  l->opacity /= 3;

	l->image = 0; l->alpha = 0;
        
	if( l->opacity != 255 )
        {
	  float lo =  l->opacity / 255.0;
          if( lay->alpha() )
            lay->set_image( lay->image(), lay->alpha()*lo );
          else
            lay->set_image( lay->image(), Image.Image( lay->xsize(),
                                                       lay->yszize(),
                                                       (int)(255*lo),
                                                       (int)(255*lo),
                                                       (int)(255*lo)));
        }
	lay->set_offset( l->xoffset, l->yoffset );
	layers += ({ lay });
      }
    }
  }
//   werror("%O\n", layers );
  layers->set_misc_value( "size", ({ what->width, what->height }) );
  return layers;
}

//! @decl mapping _decode(string|mapping data, mapping|void options)
//!
//! Decodes a PSD image to a mapping, with at least an
//! 'image' and possibly an 'alpha' object. Data is either a PSD image, or
//! a mapping (as received from @[__decode])
//!
//! Supported options
//!
//! @string
//!   @value "background"
//!     ({r,g,b})||Image.Color object
//!
//!   @value "draw_all_layers:1"
//!     Draw invisible layers as well
//!
//!   @value "draw_guides:1"
//!     Draw the guides
//!
//!   @value "draw_selection:1"
//!     Mark the selection using an overlay
//!
//!   @value "ignore_unknown_layer_modes:1"
//!     Do not asume 'Normal' for unknown layer modes.
//!
//!   @value "mark_layers:1"
//!     Draw an outline around all (drawn) layers
//!
//!   @value "mark_layer_names:Image.Font object"
//!     Write the name of all layers using the font object,
//!
//!   @value "mark_active_layer:1"
//!     Draw an outline around the active layer
//! @endstring
//!
//! @note
//!   Throws upon error in data. For more information, see @[__decode]
mapping _decode( string|mapping what, mapping|void opts )
{
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
}

//! @decl Image.Image decode(string data)
//!   Decodes a PSD image to a single image object.
//!
//! @note
//!   Throws upon error in data. To get access to more information like
//!   alpha channels and layer names, see @[_decode] and @[__decode].
Image.Image decode( string|mapping what, mapping|void opts )
{
  mapping data;
  if(!opts) opts = ([]);
  if(mappingp(what))
    data = what;
  else 
    data = __decode( what );
  what=0;

  Image.Layer res = Image.lay(decode_layers( data, opts ),
                              0,0,data->width,data->height );
  return res->image();
}
