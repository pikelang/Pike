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
  int mask_default_color;
}


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
  l->mask_flags = layer->mask_flags;
  l->mask_default_color = layer->mask_default_color;
 
  l->mask_width = layer->mask_right-layer->mask_left;
  l->mask_height = layer->mask_bottom-layer->mask_top;
  l->mask_xoffset = layer->mask_left;
  l->mask_yoffset = layer->mask_top;

  if(l->mask_flags & 1) // pos relative to layer
  {
    l->mask_xoffset += l->xoffset;
    l->mask_yoffset += l->yoffset;
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
       if(!(l->mask_flags & 2)) /* layer mask disabled */
       {
	 array pad_color = ({ l->mask_default_color }) * 3;
	 int x0 = l->xoffset - l->mask_xoffset;
	 int y0 = l->yoffset - l->mask_yoffset;
	 tmp = tmp->copy(x0, y0,
			 x0 + l->image->xsize() - 1,
			 y0 + l->image->ysize() - 1,
			 @pad_color);
	 
         if(l->mask_flags & 4) /* invert mask */
           tmp = tmp->invert();
	 
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
//! Decodes a PSD image to a mapping, defined as follows.
//!
//! @mapping
//!   @member int(1..24) "channels"
//!     The number of channels in the image, including any alpha channels.
//!   @member int(1..30000) "height"
//!   @member int(1..30000) "width"
//!     The image dimensions.
//!   @member int(0..1) "compression"
//!     1 if the image is compressed, 0 if not.
//!   @member int(1..1)|int(8..8)|int(16..16) "depth"
//!     The number of bits per channel.
//!   @member int(0..4)|int(7..9) "mode"
//!     The color mode of the file.
//!     @int
//!       @value 0
//!         Bitmap
//!       @value 1
//!         Greyscale
//!       @value 2
//!         Indexed
//!       @value 3
//!         RGB
//!       @value 4
//!         CMYK
//!       @value 7
//!         Multichannel
//!       @value 8
//!         Duotone
//!       @value 9
//!         Lab
//!     @endint
//!   @member string "color_data"
//!     Raw color data.
//!   @member string "image_data"
//!     Ram image data.
//!   @member  mapping(string|int:mixed) "resources"
//!     Additional image data. Se mappping below.
//!   @member array(mapping) "layers"
//!     An array with the layers of the image. See mapping below.
//! @endmapping
//!
//! The resources mapping. Unknown resources will be identified
//! by their ID number (as an int).
//! @mapping
//!   @member string "caption"
//!     Image caption.
//!   @member string "url"
//!     Image associated URL.
//!   @member int "active_layer"
//!     Which layer is active.
//!   @member array(mapping(string:int)) "guides"
//!     An array with all guides stored in the image file.
//!     @mapping
//!       @member int "pos"
//!         The position of the guide.
//!       @member int(0..1) "vertical"
//!         1 if the guide is vertical, 0 if it is horizontal.
//!     @endmapping
//!   @member mapping(string:int) "resinof"
//!     Resolution information
//!     @mapping
//!       @member int "hres"
//!       @member int "hres_unit"
//!       @member int "width_unit"
//!       @member int "vres"
//!       @member int "vres_unit"
//!       @member int "height_unit"
//!         FIXME: Document these.
//!     @endmapping
//! @endmapping
//!
//! The layer mapping:
//! @mapping
//!   @member int "top"
//!   @member int "left"
//!   @member int "right"
//!   @member int "bottom"
//!     The rectangle containing the contents of the layer.
//!   @member int "mask_top"
//!   @member int "mask_left"
//!   @member int "mask_right"
//!   @member int "mask_bottom"
//!   @member int "mask_flags"
//!     FIXME: Document these
//!   @member int(0..255) "opacity"
//!     0=transparent, 255=opaque.
//!   @member int "clipping"
//!     0=base, 1=non-base.
//!   @member int "flags"
//!     bit 0=transparency protected
//!     bit 1=visible
//!   @member string "mode"
//!     Blend mode.
//!     @string
//!       @value "norm"
//!         Normal
//!       @value "dark"
//!         Darken
//!       @value "lite"
//!         Lighten
//!       @value "hue "
//!         Hue
//!       @value "sat "
//!         Saturation
//!       @value "colr"
//!         Color
//!       @value "lum "
//!         Luminosity
//!       @value "mul "
//!         Multiply
//!       @value "scrn"
//!         Screen
//!       @value "diss"
//!         Dissolve
//!       @value "over"
//!         Overlay
//!       @value "hLit"
//!         Hard light
//!       @value "sLit"
//!         Soft light
//!       @value "diff"
//!         Difference
//!     @endstring
//!   @member string "extra_data"
//!     Raw extra data.
//!   @member string "name"
//!     The name of the layer
//!   @member array(mapping(string:int|string)) "channels"
//!     The channels of the layer. Each array element is a
//!     mapping as follows
//!     @mapping
//!       @member int "id"
//!         The ID of the channel
//!         @int
//!           @value -2
//!             User supplied layer mask
//!           @value -1
//!             Transparency mask
//!           @value 0
//!             Red
//!           @value 1
//!             Green
//!           @value 2
//!             Blue
//!         @endint
//!       @member string "data"
//!         The image data
//!     @endmapping
//! @endmapping

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
//! "draw_all_layers":1, but implements "crop_to_bounds" which preserves
//! the bounding box for the whole image (i.e. discards data that extend
//! outside of the global bounds).
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
//!     Is 1 of the layer is visible and 0 if it is hidden.
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

	if (opts->crop_to_bounds) {
	  //  Crop/expand this layer so it matches the image bounds.
	  //  This will lose data which extends beyond the image bounds
	  //  but keeps the image dimensions consistent.
	  int x0 = -l->xoffset, y0 = -l->yoffset;
	  int x1 = x0 + what->width - 1, y1 = y0 + what->height - 1;
	  Image.Image new_img =
	    lay->image()->copy(x0, y0, x1, y1);
	  Image.Image new_alpha =
	    lay->alpha() &&
	    lay->alpha()->copy(x0, y0, x1, y1);
	  lay->set_image(new_img, new_alpha);
	} else
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
//! @param options
//! @mapping
//!   @member array(int)|Image.Color "background"
//!     Sets the background to the given color. Arrays should be in
//!     the format ({r,g,b}).
//!
//!   @member int(0..1) "draw_all_layers"
//!     Draw invisible layers as well.
//!
//!   @member int(0..1) "draw_guides"
//!     Draw the guides.
//!
//!   @member int(0..1) "draw_selection"
//!     Mark the selection using an overlay.
//!
//!   @member int(0..1) "ignore_unknown_layer_modes"
//!     Do not asume 'Normal' for unknown layer modes.
//!
//!   @member int(0..1) "mark_layers"
//!     Draw an outline around all (drawn) layers.
//!
//!   @member Image.Font "mark_layer_names"
//!     Write the name of all layers using the font object,
//!
//!   @member int(0..1) "mark_active_layer"
//!     Draw an outline around the active layer
//! @endmapping
//!
//! @returns
//! @mapping
//!   @member Image.Image "image"
//!     The image object.
//!   @member Image.Image "alpha"
//!     The alpha channel image object.
//! @endmapping
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
