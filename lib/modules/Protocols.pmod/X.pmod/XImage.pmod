#include "error.h"

// Image --> X module.
// Needs: Pike 0.6


// Base class.
class XImage
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

  void copy(mixed ... args)
  {
    return image->copy( @args );
  }

  varargs object paste(object what, int x, int y)
  {
    image->paste(what, x, y);
    clear_caches(x,y,what->xsize(),what->ysize());
    redraw(x,y,what->xsize(),what->ysize());
  }

  varargs object paste_mask(object what, object mask, int x, int y)
  {
    image->paste_mask(what, mask, x, y);
    clear_caches(x,y,what->xsize(),what->ysize());
    redraw(x,y,what->xsize(),what->ysize());
  }

  void set_image(object to)
  {
    image = to;
    clear_caches(0,0,image->xsize(),image->ysize());
    redraw(0,0,image->xsize(),image->ysize());
  }

  mixed `->( string ind )
  {
    mixed x;
    if((x = `[](this_object(),ind)))
      return x;
    x = funcall(image[ind], this_object() );
  }
  
}


// Steels a few callbacks from the window.

class WindowImage
{
  inherit XImage;
  object (Types.Window) window;
  object (Types.RootWindow) root; // extends window
  object (Types.Visual) visual;
  object (Types.Colormap) colormap;
  object (Image.colortable) ccol;
  object (Types.GC) dgc;

  int best;

  int depth, bpp;
  function converter;
  int linepad;
  int rmask, gmask, bmask;

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

    Array.map((array)wanted,
	    lambda(array o){
	      return colormap->AllocColor(o[0]*257,o[1]*257,o[2]*257);
	    });

    foreach(values(colormap->alloced), mapping m)
      if(m && m->pixel > max_pixel)
	max_pixel = m->pixel;

    array res = allocate( max_pixel+1 );

    foreach(values(colormap->alloced), mapping m)
      if(m) res[ m->pixel ] = ({ m->red/257, m->green/257, m->blue/257 });
// What to do with unallocated colors?
    res = replace( res, 0, ({ 255,0,255 }) );

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
    int lheight = height;
    int slice = (window->display->maxRequestSize/depth)*32 / width;
    if(x+width > image->xsize())
      width = image->xsize()-x;
    if(width<=0) return;

    if(y+lheight > image->ysize())
    {
      height = image->ysize()-y;
      lheight = image->ysize()-y;
    }
    if(lheight<=0) return;

    while(lheight>0)
    {
      lheight -= slice;
      if(lheight < 0) slice += lheight;
      object mimg = image->copy(x,y,x+width-1,y+slice-1);

      if(rmask)
	window->PutImage( dgc, depth, x, y, width, slice,
			  converter( mimg, bpp, linepad, rmask, gmask, bmask,
				     @(ccol?({ccol}):({}))));
      else
      {
	if(!ccol) ccol = allocate_colortable();
	string data = converter( mimg, bpp, linepad, depth, ccol );
	window->PutImage( dgc, bpp, x, y, width, slice, data );
      }
      y += slice;
    }
  }

  void exposed(mixed event)
  {
    werror("expose...");
    redraw(event->x, event->y, event->width, event->height);
    werror("done\n");
  }


  void create(object (Types.Window) w)
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
    } else
      visual = w->visual;



    if(root->visual == visual)
    {
      werror("using default visual\n");
      colormap = root->defaultColorMap;
    }
    else
      colormap = w->CreateColormap( visual, 0 );
    rmask = visual->redMask;
    gmask = visual->greenMask;
    bmask = visual->blueMask;
    depth = visual->depth;
    bpp = visual->bitsPerRGB;
    linepad = 32; // FIXME!
    
    switch(_Xlib.visual_classes[visual->c_class])
    {
     case "StaticGray":  
       ccol = Image.colortable(0,0,0, ({0,0,0}), ({255,255,255}), 1<<depth);
       converter = Image.X.encode_pseudocolor;
       break;
     case "GrayScale":   
       ccol = Image.colortable(0,0,0, ({0,0,0}), ({255,255,255}), 1<<depth);
       converter = Image.X.encode_pseudocolor;
       break;
     case "PseudoColor": 
       ccol = 0;
       converter = Image.X.encode_pseudocolor;
       break;
     case "StaticColor": 
     case "TrueColor":
       if(depth < 16)
       {
	 ccol = Image.colortable(1<<depth, 1<<rmask, 1<<gmask, 1<<bmask);
	 ccol->ordered();
       }
       converter = Image.X.encode_truecolor_masks;
       break;
     case "DirectColor":
       error("Cannot handle Direct Color visual yet."); 
       break;
    }
    dgc = window->CreateGC();

    w->set_event_callback("Expose", exposed); // internal callback...
    w->SelectInput("Exposure");
  }
}

class PixmapImage
{
  inherit XImage;
  object pixmap;
  object (Types.Window) window, root;
  object (Types.Visual) visual;
  object (Types.Colormap) colormap;

  int depth;
  function converter;
  int linepad;
  int rmask, gmask, bmask;


  void create(object (Types.Pixmap) p, object (Types.Window) w)
  {
    pixmap = p;
    window = w;
  }
}
