/*
 * $Id: top.c,v 1.1 1998/11/02 23:54:39 marcus Exp $
 *
 */

#include "global.h"

RCSID("$Id: top.c,v 1.1 1998/11/02 23:54:39 marcus Exp $");
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "error.h"

#include <GL/gl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

static GLXContext glxc;
static Display *dpy;
static Window win;

static void f_InitializeAWindowPlease()
{
  XVisualInfo *vis;
  static int attrib[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 1, None };
  int scr;
  Window root;
  XSetWindowAttributes attr;
  Colormap cmap;
  unsigned long black;
  XEvent event;

  dpy = XOpenDisplay(NULL);
  if(dpy == NULL)
    error("Cannot connect to X server %s", XDisplayName(NULL));

  scr = DefaultScreen(dpy);
  root = RootWindow(dpy, scr);
  vis = glXChooseVisual(dpy, scr, attrib);
  if(vis == NULL)
    error("glxChooseVisual failed.");

  cmap = XCreateColormap(dpy, root, vis->visual,
			 (vis->class&1)? AllocAll:AllocNone);
   if(vis->class&1)
    black = 0;
   else {
    XColor c;
    c.pixel = 0;
    c.red = 0;
    c.green = 0;
    c.blue = 0;
    c.flags = DoRed|DoGreen|DoBlue;
    if(XAllocColor(dpy, cmap, &c))
      black = c.pixel;
    else
      black = 0;
  }
  attr.colormap = cmap;
  attr.background_pixel = black;
  attr.border_pixel = black;
  win = XCreateWindow(dpy, root, 0, 0, 1152, 900, 1, vis->depth, InputOutput,
		      vis->visual, CWColormap|CWBackPixel|CWBorderPixel,
		      &attr);
  XSelectInput(dpy, win, StructureNotifyMask);
  glxc = glXCreateContext(dpy, vis, NULL, True);
  if(!glXMakeCurrent(dpy, win, glxc))
    error("glxMakeCurrent failed.");
  XMapRaised(dpy, win);
  do
    XNextEvent(dpy, &event);
  while(event.type != MapNotify || event.xmap.window != win);
}

static void f_UpdateTheWindowAndCheckForEvents()
{
  glXSwapBuffers(dpy, win);
  glXWaitGL();
  XSync(dpy, False);
  /* sleep(3600); */
}



void pike_module_init( void )
{
  extern void add_auto_funcs(void);

  add_function_constant( "InitializeAWindowPlease", f_InitializeAWindowPlease,
			 "function(:void)", OPT_SIDE_EFFECT );
  add_function_constant( "UpdateTheWindowAndCheckForEvents",
			 f_UpdateTheWindowAndCheckForEvents,
			 "function(:void)", OPT_SIDE_EFFECT );

  add_auto_funcs();
}


void pike_module_exit( void )
{
}

