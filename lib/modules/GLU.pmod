/*
 * $Id: GLU.pmod,v 1.2 1999/08/10 17:13:31 grubba Exp $
 *
 * GL Utilities module.
 */
#if constant(GL)
import GL;

#ifndef M_PI
#define M_PI 3.1415926536
#endif
#define EPS 0.00001

void gluLookAt(float eyex, float eyey, float eyez,
	       float centerx, float centery, float centerz,
	       float upx, float upy, float upz)
{

  array(float) m=allocate(16);
  array(float) x=allocate(3), y=allocate(3), z=allocate(3);
  float mag;
  
  /* Make rotation matrix */
  
  /* Z vector */
  z[0] = eyex - centerx;
  z[1] = eyey - centery;
  z[2] = eyez - centerz;
  mag = sqrt( z[0]*z[0] + z[1]*z[1] + z[2]*z[2] );
  if (mag) {  /* mpichler, 19950515 */
    z[0] /= mag;
    z[1] /= mag;
    z[2] /= mag;
  }
  
  /* Y vector */
  y[0] = upx;
  y[1] = upy;
  y[2] = upz;
  
  /* X vector = Y cross Z */
  x[0] =  y[1]*z[2] - y[2]*z[1];
  x[1] = -y[0]*z[2] + y[2]*z[0];
  x[2] =  y[0]*z[1] - y[1]*z[0];
  
  /* Recompute Y = Z cross X */
  y[0] =  z[1]*x[2] - z[2]*x[1];
  y[1] = -z[0]*x[2] + z[2]*x[0];
  y[2] =  z[0]*x[1] - z[1]*x[0];
  
   /* mpichler, 19950515 */
  /* cross product gives area of parallelogram, which is < 1.0 for
   * non-perpendicular unit-length vectors; so normalize x, y here
   */
  
  mag = sqrt( x[0]*x[0] + x[1]*x[1] + x[2]*x[2] );
  if (mag) {
    x[0] /= mag;
    x[1] /= mag;
    x[2] /= mag;
  }
  
  mag = sqrt( y[0]*y[0] + y[1]*y[1] + y[2]*y[2] );
  if (mag) {
    y[0] /= mag;
    y[1] /= mag;
    y[2] /= mag;
  }
  
#define M(row,col)  m[col*4+row]
  M(0,0) = x[0];  M(0,1) = x[1];  M(0,2) = x[2];  M(0,3) = 0.0;
  M(1,0) = y[0];  M(1,1) = y[1];  M(1,2) = y[2];  M(1,3) = 0.0;
  M(2,0) = z[0];  M(2,1) = z[1];  M(2,2) = z[2];  M(2,3) = 0.0;
  M(3,0) = 0.0;   M(3,1) = 0.0;   M(3,2) = 0.0;   M(3,3) = 1.0;
#undef M
  glMultMatrix( m );
  
  /* Translate Eye to Origin */
  glTranslate( -eyex, -eyey, -eyez );
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
