/*
 * $Id: GLU.pmod,v 1.3 1999/10/25 18:31:43 mirar Exp $
 *
 * GL Utilities module.
 */
#if constant(GL)
import GL;
import Math;

#ifndef M_PI
#define M_PI 3.1415926536
#endif
#define EPS 0.00001

void gluLookAt(float|object eye,float|object center,float|object up,
	       float ... old_api)
{
  Matrix x,y,z;
  float mag;

  if (!objectp(eye))
  {
     eye=Matrix( ({eye,center,up }) );
     center=Matrix( old_api[..2] );
     up=Matrix( old_api[3..5] );
  }
  
  /* Make rotation matrix */
  
  z=(eye-center)->normv();   /* Z vector */
  y=up;                      /* Y vector */
  x=y->cross(z);             /* X vector = Y cross Z */
  y=z->cross(x);             /* Recompute Y = Z cross X */
  
  /* mpichler, 19950515 */
  /* cross product gives area of parallelogram, which is < 1.0 for
   * non-perpendicular unit-length vectors; so normalize x, y here
   */
  
  x=x->normv(); // normalize
  y=y->normv(); // normalize

  array m=Array.transpose(({ @(x->vect()), 0.0,
			     @(y->vect()), 0.0,
			     @(z->vect()), 0.0,
			     0.0, 0.0, 0.0, 1.0 })/4)*({}); 
  
  werror("%O\n",Matrix(m/4));
//   glMultMatrix( m );
  
  /* Translate Eye to Origin */
//   glTranslate( ((array)(-1*eye))[0] );
}  

void gluOrtho2D(float left, float right,
		float bottom, float top)
{
  glOrtho( left, right, bottom, top, -1.0, 1.0 );
}

void gluPerspective(float fovy, float aspect,
		    float zNear, float zFar)
{
  float xmin, xmax, ymin, ymax;
  
  ymax = zNear * tan( fovy * M_PI / 360.0 );
  ymin = -ymax;
  
  xmin = ymin * aspect;
  xmax = ymax * aspect;
  
  glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


void gluPickMatrix(float x, float y,
		   float width, float height,
		   array(int) viewport)
{
  array(float) m=allocate(16);
  float sx, sy;
  float tx, ty;
  
  sx = viewport[2] / width;
  sy = viewport[3] / height;
  tx = (viewport[2] + 2.0 * (viewport[0] - x)) / width;
  ty = (viewport[3] + 2.0 * (viewport[1] - y)) / height;
  
#define M(row,col)  m[col*4+row]
  M(0,0) = sx;   M(0,1) = 0.0;  M(0,2) = 0.0;  M(0,3) = tx;
  M(1,0) = 0.0;  M(1,1) = sy;   M(1,2) = 0.0;  M(1,3) = ty;
  M(2,0) = 0.0;  M(2,1) = 0.0;  M(2,2) = 1.0;  M(2,3) = 0.0;
  M(3,0) = 0.0;  M(3,1) = 0.0;  M(3,2) = 0.0;  M(3,3) = 1.0;
#undef M
  
  glMultMatrix( m );
}


static void transform_point(array(float) out, array(float)m,
			    array(float) in)
{
#define M(row,col)  m[col*4+row]
  out[0] = M(0,0) * in[0] + M(0,1) * in[1] + M(0,2) * in[2] + M(0,3) * in[3];
  out[1] = M(1,0) * in[0] + M(1,1) * in[1] + M(1,2) * in[2] + M(1,3) * in[3];
  out[2] = M(2,0) * in[0] + M(2,1) * in[1] + M(2,2) * in[2] + M(2,3) * in[3];
  out[3] = M(3,0) * in[0] + M(3,1) * in[1] + M(3,2) * in[2] + M(3,3) * in[3];
#undef M
}


array(float) gluProject(float objx, float objy,
			float objz, array(float) model,
			array(float) proj, array(int) viewport)

{
  array(float) in=allocate(4),out=allocate(4);

  in[0]=objx; in[1]=objy; in[2]=objz; in[3]=1.0;
  transform_point(out,model,in);
  transform_point(in,proj,out);

  if (in[3]==0.0)
    return 0;

  in[0]/=in[3]; in[1]/=in[3]; in[2]/=in[3];

  return ({ viewport[0]+(1+in[0])*viewport[2]/2,
	    viewport[1]+(1+in[1])*viewport[3]/2,
	    (1+in[2])/2 });
}


// array(float) gluUnProject(float winx,float winy,float winz,
// 			  array(float) model, array(float) proj,
// 			  array(int) viewport)
// {
//   array(float)
//     m=allocate(16),
//     A=allocate(16),
//     in=allocate(4),
//     out=allocate(4);
  
//   in[0]=(winx-viewport[0])*2/viewport[2] - 1.0;
//   in[1]=(winy-viewport[1])*2/viewport[3] - 1.0;
//   in[2]=2*winz - 1.0;
//   in[3]=1.0;
  
//   matmul(A,proj,model);
//   invert_matrix(A,m);
  
//   transform_point(out,m,in);
//   if (out[3]==0.0)
//     return GL_FALSE;
//   return ({ out[0]/out[3], out[1]/out[3], out[2]/out[3] });
// }

#endif /* constant(GL) */
