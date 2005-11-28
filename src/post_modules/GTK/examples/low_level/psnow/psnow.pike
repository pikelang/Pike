// This is a program that operates on a very low level. It draws
// things in the root window.


// The lack of comments is somewhat intentional.  Basically: Draw a
// lot of snow falling on the background, more or less like the old
// xsnow program.

// This program is the only GTK program I have written that is not
// event driven. There is not much GTK in here, though, this program
// use almost exclusively GDK function calls. /per

GDK.Window root;
float mdx, mdy; // Size of the root window.
int windy, edx; // is it windy today? edx is the wind direction.

#define I(X) Image.PNM.decode(Stdio.read_bytes("snow0" #X ".pbm"))->invert()->scale(1.5)

array (GDK.GC) snow_flake_gcs = ({});
array (Image.image) snow_flakes=({I(0), I(1), I(2),  I(3), I(4), I(5), I(6)});

#define SPEED 1.0

class Snowflake
{
  GDK.GC gc;
  float x, y, dx, dy, dx2;
  int ox, oy, xsize, ysize;


  void init_flake()
  {
    x = (float)random(root->xsize()-8);
    dx = (random(20)/(10.0*SPEED))-2.0;
    dy = (random(90)/(10.0*SPEED))+0.5;
    y = 0.0;
    ox = oy = 0;
  }

  void step()
  {
    dx += (random(10)-4.5)/2.0*SPEED;
    if(abs(dx) > 10.0) dx = 10.0*sgn(dx);
    x += dx; y += dy;
    if(windy)
    {
      if(windy > 100) dx2 += edx * 0.2;
      else if(windy<=100) dx2 -= edx * 0.2;
      x+=dx2;
    }
    int ix = (int)x;
    int iy = (int)y;

    if(ix != ox || iy != oy)
    {
      if(y > mdy)
        init_flake();
      else if(x<0.0)
        x = mdx;
      else if(x>mdx)
        x = 0.0;
      root->clear( ox,oy, xsize, ysize );
      ox = ix; oy = iy;
      gc->set_clip_origin(ix,iy);
      root->draw_rectangle(gc,1,ix,iy,xsize,ysize);
    }
    if( collided( this ) )
      init_flake();
  }

  void create(int q)
  {
    int num = random(7);
    xsize = snow_flakes[num]->xsize();  ysize = snow_flakes[num]->ysize();
    gc = snow_flake_gcs[ num ];
    init_flake();
  }
}

array snow = ({});
void make_some_snow()
{
  GDK.Color white = GDK.Color( 255,255,255 );
  for(int i = 0; i<sizeof(snow_flakes); i++)
  {
    GDK.Bitmap b = GDK.Bitmap( snow_flakes[i] );
    snow_flake_gcs += ({ GDK.GC( root ) });
    snow_flake_gcs[i]->set_clip_mask( b );
    snow_flake_gcs[i]->set_foreground( white );
  }
  snow = map(allocate(100),Snowflake);
}

void move_snow()
{
  for(;;)
  {
    if(!windy)
    {
      if(!random(1000))
      {
        windy = 200;
        edx = (random(2)?-1:1);
      }
    } else
      windy--;
    snow->step();
    GTK.low_flush();
    sleep(0.025/SPEED);
  }
}

GDK.Region region;

int collided( Snowflake f )
{
  int x = (int)f->x, y = (int)f->y;
  int w = (int)f->xsize;
  int h = (int)f->ysize;
   if( region->point_in( x, y+h ) || region->point_in( x+w, y+h ))
   {
       // Expand region somewhat, unless it's at the top.
       if( y > 100 && x > 10)
       {
	   
	   region |= GDK.Rectangle( x+w/2, y+h/2+h/4, w/2, h/2 );
       }
//   if( y > 1200 )
     return 1;
   }
  return 0;
}


int main()
{
  GTK.setup_gtk();
  root = GTK.root_window();
  region = GDK.Region();


  // Is there any 'fake' root windows out there to draw the snow in?
  array(mapping) maybe
    =root->children()->get_property(GDK.Atom.__SWM_VROOT)-({0});
  if(sizeof(maybe))
      root = GDK.Window( maybe[0]->data[0] );

  mapping rg = root->get_geometry();
  foreach( root->children(), object o )
  {
    if(o->is_visible())
    {
      mapping g = o->get_geometry();
      if(g->width >= rg->width*0.7)
        continue;
      region |= GDK.Rectangle( g->x, g->y, g->width, g->height );
    }
  }
  region |= GDK.Rectangle( 0, rg->height-1, rg->width, 1 );
  mdx = (float)root->xsize(); mdy = (float)root->ysize();
  make_some_snow();
  move_snow();
}
