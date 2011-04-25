/*
 * Graph sub-module providing draw functions.
 * $Id$
 */

#pike __REAL_VERSION__

#define CAP_BUTT       0
#define CAP_ROUND      1
#define CAP_PROJECTING 2

#define JOIN_MITER     0
#define JOIN_ROUND     1
#define JOIN_BEVEL     2


#define CAPSTEPS  10
#define JOINSTEPS 5


constant PI = 3.1415926535897932384626433832795080;

/*
 * Some optimizations for the cappings.
 *
 * /grubba (who got tired of BG beeing so slow)
 */

static array(float) init_cap_sin_table()
{
  array(float) s_t = allocate(CAPSTEPS);

  for (int i = 0; i < CAPSTEPS; i++) {
    s_t[i] = sin(PI*i/(CAPSTEPS-1));
  }
  return(s_t);
}

static array(float) cap_sin_table = init_cap_sin_table();

static array(float) init_cap_cos_table()
{
  array(float) c_t = allocate(CAPSTEPS);

  for (int i = 0; i < CAPSTEPS; i++) {
    c_t[i] = cos(PI*i/(CAPSTEPS-1));
  }
  return(c_t);
}

static array(float) cap_cos_table = init_cap_cos_table();



static private array(float) xyreverse(array(float) a)
{
  array(float) r = reverse(a);
  int n = sizeof(r)/2;
  while(n--) {
    float t = r[n<<1];
    r[n<<1] = r[(n<<1)+1];
    r[(n<<1)+1] = t;
  }
  return r;
}

array(array(float)) make_polygon_from_line(float h, array(float) coords,
				   int|void cap_style, int|void join_style)
{
  int points = sizeof(coords)/2;
  int closed = points>2 &&
    coords[0] == coords[(points-1)<<1] &&
    coords[1] == coords[((points-1)<<1)+1];
  if(closed)
    --points;
  int point;
  float sx = h/2, sy = 0.0;
  array(float) left = ({ }), right = ({ });

  for(point=0; point<points; point++) {

    float ox = coords[point<<1], oy = coords[(point<<1)+1];
    int t = (point==points-1 ? (closed? 0 : point) : point+1);
    float tx = coords[t<<1], ty = coords[(t<<1)+1];
    float dx = tx - ox, dy = ty - oy, dd = sqrt(dx*dx + dy*dy);
    if(dd > 0.0) {
      sx = (-dy*h) / (dd*2);
      sy = (dx*h) / (dd*2);
    }

    if(point == 0 && !closed) {

      /* Initial cap */

      switch(cap_style) {
      case CAP_BUTT:
	left += ({ ox+sx, oy+sy });
	right += ({ ox-sx, oy-sy });
	break;
      case CAP_PROJECTING:
	left += ({ ox+sx-sy, oy+sy+sx });
	right += ({ ox-sx-sy, oy-sy+sx });
	break;
      case CAP_ROUND:
	array(float) initial_cap = allocate(CAPSTEPS*2);
	
	int j=0;
	for(int i=0; i<CAPSTEPS; i++) {
	  initial_cap[j++] = ox + sx*cap_cos_table[i] - sy*cap_sin_table[i];
	  initial_cap[j++] = oy + sy*cap_cos_table[i] + sx*cap_sin_table[i];
	}
	right += initial_cap;
	break;
      }

    }

    if(closed || point<points-1) {

      /* Interconnecting segment and join */

      if(point == points-2 && !closed)
	/* Let the final cap generate the segment */
	continue;

      int t2 = (t==points-1 ? 0 : t+1);
      float t2x = coords[t2<<1], t2y = coords[(t2<<1)+1];
      float d2x = t2x - tx, d2y = t2y - ty, d2d = sqrt(d2x*d2x + d2y*d2y);
      float s2x, s2y;
      if(d2d > 0.0) {
	s2x = (-d2y*h) / (d2d*2);
	s2y = (d2x*h) / (d2d*2);
      } else {
	s2x = sx;
	s2y = sy;
      }

      float mdiv = (sx*s2y-sy*s2x); 
      if(mdiv == 0.0) {
	left += ({ tx+sx, ty+sy, tx+s2x, ty+s2y });
	right += ({ tx-sx, ty-sy, tx-s2x, ty-s2y });
      } else {
	float m = (s2y*(sy-s2y)+s2x*(sx-s2x))/mdiv;

	/* Left join */

	switch(mdiv<0.0 && join_style) {
	case JOIN_MITER:
	  left += ({ tx+sx+sy*m, ty+sy-sx*m });
	  break;
	case JOIN_BEVEL:
	  left += ({ tx+sx, ty+sy, tx+s2x, ty+s2y });
	  break;
	case JOIN_ROUND:
	  float theta0 = acos((sx*s2x+sy*s2y)/(sx*sx+sy*sy));
	  for(int i=0; i<JOINSTEPS; i++) {
	    float theta = theta0*i/(JOINSTEPS-1);
	    float sint = sin(theta), cost = cos(theta);
	    left += ({ tx+sx*cost+sy*sint, ty+sy*cost-sx*sint });
	  }
	  break;
	}

	/* Right join */

	switch(mdiv>0.0 && join_style) {
	case JOIN_MITER:
	  right += ({ tx-sx-sy*m, ty-sy+sx*m });
	  break;
	case JOIN_BEVEL:
	  right += ({ tx-sx, ty-sy, tx-s2x, ty-s2y });
	  break;
	case JOIN_ROUND:
	  float theta0 = -acos((sx*s2x+sy*s2y)/(sx*sx+sy*sy));
	  for(int i=0; i<JOINSTEPS; i++) {
	    float theta = theta0*i/(JOINSTEPS-1);
	    float sint = sin(theta), cost = cos(theta);
	    right += ({ tx-sx*cost-sy*sint, ty-sy*cost+sx*sint });
	  }
	  break;
	}
      }

    } else {

      /* Final cap */

      switch(cap_style) {
      case CAP_BUTT:
	left += ({ ox+sx, oy+sy });
	right += ({ ox-sx, oy-sy });
	break;
      case CAP_PROJECTING:
	left += ({ ox+sx+sy, oy+sy-sx });
	right += ({ ox-sx+sy, oy-sy-sx });
	break;
      case CAP_ROUND:
	array(float) end_cap = allocate(CAPSTEPS*2);
	
	int j=0;
	for(int i=0; i<CAPSTEPS; i++) {
	  end_cap[j++] = ox - sx*cap_cos_table[i] + sy*cap_sin_table[i];
	  end_cap[j++] = oy - sy*cap_cos_table[i] - sx*cap_sin_table[i];
	}
	right += end_cap;
	break;
      }

    }

  }

  if(closed)
    return ({ left, right });
  else
    return ({ left + xyreverse(right) });

}
