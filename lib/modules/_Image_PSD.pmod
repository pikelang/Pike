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

Layer decode_layer(mapping layer, mapping i)
{
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

  array colors;
  switch(i->mode)
  {
   case Greyscale:
     colors = ({
       255,255,255
     })*24;
     break;
   case RGB:
     colors = ({ ({255,0,0,}),
                 ({0,255,0,}),
                 ({0,0,255,}),
               }) + ({ 255,255,255 }) * 24;
     break;
   case Indexed:
     use_cmap = 1;
     error("Tell per to fix colormap support\n");
     break;
   default:
     werror("Unsupported mode (for now), using greyscale\n");
     colors = ({
       255,255,255
     })*24;
     break;
  }
  foreach(layer->channels, mapping c)
  {
    object tmp =___decode_image_channel(l->width,l->height,c->data);
       
    switch(c->id)
    {
     default:
       l->image->paste_alpha_color( tmp, @colors[c->id%24] );
       break;
     case -1: /* alpha */
       l->alpha = tmp;
       break;
     case -2: /* user mask */
       if(!l->alpha)
         l->alpha = tmp;
       else
         l->alpha *= tmp;
       break;
    }
  }
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
    rl += ({ decode_layer( l,data ) });
  data->layers = rl;
  return data;
}

#define PASTE_ALPHA(X,Y)                                                \
        if(Y)                                                           \
          img->paste_mask( X, Y, l->xoffset, l->yoffset );              \
        else                                                            \
          img->paste( X, l->xoffset, l->yoffset );                      \
        if(Y&&alpha&&++alpha_used)                                      \
          alpha->paste_alpha_color(Y,255,255,255,l->xoffset, l->yoffset ); \
        else if(alpha)                                                 \
         alpha->box(l->xoffset,l->yoffset,l->xoffset+l->width-1,l->yoffset+l->height-1,255,255,255)

#define IMG_SLICE(l,h) img->copy(l->xoffset,l->yoffset,l->xoffset+h->width-1,l->yoffset+h->height-1)


array(object) decode_background( mapping data, array bg )
{
  object img, alpha;
  if( !bg )
    alpha = Image.image(data->width, data->height);

//   if( data->image_data )
//     img = ___decode_image_data(data->width, data->height, 
//                                data->channels, data->image_data);
//   else
    img = Image.image( data->width, data->height,
                       @((bg&&(array)bg)||({255,255,255})));
  return ({ img, alpha });
}

mapping _decode( string|mapping what, mapping|void opts )
{
  mapping data;
  if(!opts) opts = ([]);
  if(mappingp(what))
    data = what;
  else 
    data = __decode( what );
  what=0;
  object alpha, img;
  int alpha_used;
  [img,alpha] = decode_background( data, opts->background );

  foreach(reverse(data->layers), object l)
  {
//     if((l->flags & LAYER_FLAG_VISIBLE)|| opts->draw_all_layers)
    {
      Layer h = l->get_opaqued( l->opacity );

      switch( l->mode )
      {
      case "norm":
        PASTE_ALPHA(h->image,h->alpha);
        break;


      case "mult":
        object oi = IMG_SLICE(l,h);
        oi *= h->image;
        PASTE_ALPHA(oi,h->alpha);
        break;

      case "add ":
        object oi = IMG_SLICE(l,h);
        oi += h->image;
        PASTE_ALPHA(oi,h->alpha);
        break;

      case "diff":
        object oi = IMG_SLICE(l,h);
        oi -= h->image;
        PASTE_ALPHA(oi,h->alpha);
        break;
       
       default:
        if(!opts->ignore_unknown_layer_modes)
        {
          werror("Layer mode "+l->mode+" not yet implemented,  "
               " asuming 'norm'\n");
          PASTE_ALPHA(h->image,h->alpha);
        }
        break;
      }
    }
  }

  return 
  ([
    "image":img,
    "alpha":alpha,
  ]);
}
