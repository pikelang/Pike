#include "error.h"

// Image --> X module.
// Needs: Pike 0.6


// Base class.
// FIXME: Why not inherit Image.Image directly?
// Guess five times... /Per
class Image_wrapper
{
  class funcall
  {
    function f;
    object parent;
    
    void `()( mixed ... args )
    {
      mixed q = f( @args );
      if(objectp(q)) parent->set_image( q );
      return q;
    }
    
    void create(function q, object p) { f = q;parent=p; }
  }
  
  object image; // The real image.

  void clear_caches(int x, int y, int width, int height)
  {
    // Changed by the inheriting class.
  }

  void redraw(int x, int y, int width, int height)
  {
    // Changed by the inheriting class.
  }

  object copy(mixed ... args)
  {
    return image->copy( @args );
  }

  object paste(object what, int|void x, int|void y)
  {
    image->paste(what, x, y);
    clear_caches(x,y,what->xsize(),what->ysize());
    redraw(x,y,what->xsize(),what->ysize());
  }

  object paste_mask(object what, object mask, int|void x, int|void y)
  {
    image->paste_mask(what, mask, x, y);
    clear_caches(x,y,what->xsize(),what->ysize());
    redraw(x,y,what->xsize(),what->ysize());
  }

  void set_image(object to, int|void no_redraw)
  {
    image = to;
    clear_caches(0,0,image->xsize(),image->ysize());
    if (!no_redraw)
      redraw(0,0,image->xsize(),image->ysize());
  }

  mixed `->( string ind )
  {
    mixed x;
    if((x = `[](this_object(),ind)))
      return x;
    return funcall(image[ind], this_object() );
  }
  
}

class XImage
{
  inherit Image_wrapper;

  object (Types.Window) window;
  object (Types.RootWindow) root; // extends window
  object (Types.Visual) visual;
  object (Types.Colormap) colormap;
  object (Image.colortable) ccol;
  object (Types.GC) dgc;

  int best;

  int depth, bpp;
  function converter;
  int linepad, swapbytes;
  int rmask, gmask, bmask;

  int offset_x, offset_y;
  
  void set_render(string type)
  {
    if(type == "best") best=1;
  }

  object allocate_colortable()
  {
    array wanted;
    if(best)
      wanted = image->select_colors( 100 );
    else
      wanted = image->select_colors( 32 );

    if(sizeof(wanted) < 10) 
    {
      object i = Image.image(100,100);
      i->tuned_box(0,0, 100, 50,
		   ({ ({0,255,255 }),({255,255,255 }),
		      ({0,0,255 }),({255,0,255 }) }));
      i->tuned_box(0,0, 100, 50,
		   ({ ({0,0,255 }),({255,0,255 }),
		      ({0,255,255 }),({255,255,0 }) }));
      i = i->hsv_to_rgb();
      wanted += i->select_colors(100);
    }

    int max_pixel;

    Array.map(((array)wanted+({({255,255,255}),({0,0,0})})),
	    lambda(array o){
	      return colormap->AllocColor(o[0]*257,o[1]*257,o[2]*257);
	    });

    foreach(values(colormap->alloced), mapping m)
      if(m && m->pixel > max_pixel)
	max_pixel = m->pixel;

    array res = allocate( max_pixel+1 );

    foreach(values(colormap->alloced), mapping m)
      if(m) res[ m->pixel ] = ({ m->red/257, m->green/257, m->blue/257 });

    object ct = Image.colortable( res );
    ct->cubicles(12, 12, 12);
    if(best)
      ct->floyd_steinberg();
    else
      ct->ordered();
//  werror(sprintf("colortable: %O\n", res));
    return ct;
  }
  
  void clear_caches(int x, int y, int width, int height)
  {
    // no inteligence yet...
  }

  void redraw(int x, int y, int width, int height)
  {
    int max_pixels = ((window->display->maxRequestSize - 64)*32) / bpp;
    int slice = (max_pixels / width)-1;

//     werror(sprintf("XImage->redraw: [%d, %d]+[%d, %d] (%d)\n",
// 		   x, y, width, height, slice));

    if(x+width > image->xsize())
      width = image->xsize()-x;
    if (x<0)
    {
      width += x;
      x = 0;
    }
    if(width<=0) return;

    if(y+height > image->ysize())
      height = image->ysize()-y;
    if (y<0)
    {
      height += y;
      y = 0;
    }

    if(height<=0) return;

    while(height>0)
    {
      height -= slice;
      if(height < 0) slice += height;
      object mimg = image->copy(x,y,x+width-1,y+slice-1);
      if(rmask)
      {
	string data = 
	  converter(mimg,bpp,linepad,swapbytes,rmask,gmask,bmask,
		    @(ccol?({ccol}):({})));

	window->PutImage( dgc, depth, x + offset_x, y + offset_y,
			  width, slice, data, 2 );
      }
      else
      {
	if(!ccol) ccol = allocate_colortable();
	string data = converter( mimg, bpp, linepad, depth, ccol );
	window->PutImage( dgc, bpp, x + offset_x, y + offset_y,
			  width, slice, data, 2 );
      }
      y += slice;
    }
  }

  void set_drawable(object w)
  {
    window = w;
    if(!w->visual)
    {
      object q = w->parent;
      if(q)
	do {
	  if(q->visual)
	  {
	    visual = q->visual;
	    break;
	  } else {
	    q = q->parent;
	  }
	} while(q);
      q = w;
      while(q) { root = q; q = q->parent; }

      if(!visual)
	visual = root->visual;
      root = w->root;
    } else {
      root = w->parent;
      while(root->parent) root = root->parent;
      visual = w->visual;
    }
    if(w->colormap)
      colormap = w->colormap;
    else
      colormap = root->defaultColorMap;
    
    swapbytes = !w->display->imageByteOrder;
    rmask = visual->redMask;
    gmask = visual->greenMask;
    bmask = visual->blueMask;
    depth = visual->depth;

    foreach(w->display->formats, mapping format)
      if(format->depth == depth)
      {
// 	werror(sprintf("found matching format: %O", format));
	bpp = format->bitsPerPixel;
	linepad = format->scanLinePad;
	break;
      }
    
    switch(_Xlib.visual_classes[visual->c_class])
    {
     case "StaticGray":
       ccol = Image.colortable(0,0,0, ({0,0,0}), ({255,255,255}), 1<<depth);
       converter = Image.X.encode_pseudocolor;
       break;
//        ccol = Image.colortable(0,0,0, ({0,0,0}), ({255,255,255}), 1<<depth);
//        converter = Image.X.encode_pseudocolor;
//        break;
     case "GrayScale":
     case "PseudoColor":
       ccol = 0;
       converter = Image.X.encode_pseudocolor;
       break;
     case "StaticColor":
     case "TrueColor":
       if(depth <= 16)
       {
#define BITS(Y) (Image.X.examine_mask(Y)[0])
	 ccol = Image.colortable(1<<BITS(rmask),
				 1<<BITS(gmask),
				 1<<BITS(bmask));
	 ccol->ordered();
       }
       converter = Image.X.encode_truecolor_masks;
       break;
     case "DirectColor":
       error("Cannot handle Direct Color visual yet."); 
       break;
    }
    dgc = window->CreateGC();
  }

  void set_offset(int x, int y)
  {
    offset_x = x;
    offset_y = y;
  }
}  
  
// Steels a few callbacks from the window.

class WindowImage
{
  inherit XImage;

  void exposed(mixed event)
  {
//     werror(sprintf("%O\n", event));
#ifdef BUGGY_X
    if(!event->count)
    {
      remove_call_out(redraw);
      call_out(redraw,0.1,0,0,image->xsize(),image->ysize());
    }
#else
    redraw(event->x, event->y, event->width, event->height);
#endif
  }

  void create(object(Types.Window) w)
  {
    set_drawable(w);
    w->set_event_callback("Expose", exposed); // internal callback...
    w->SelectInput("Exposure");
  }
}

class PixmapImage
{
  inherit XImage;

  void create(object (Types.Pixmap) p)
  {
    set_drawable( p );
  }
}
