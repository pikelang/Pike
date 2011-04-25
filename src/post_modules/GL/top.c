/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "config.h"

#ifdef HAVE_GL

#ifdef HAVE_WINDEF_H
#include <windows.h>
#include <windef.h>
#endif /* HAVE_WINDEF_H */
#ifdef HAVE_WINGDI_H
#include <wingdi.h>
#endif /* HAVE_WINGDI_H */
#ifdef HAVE_GL_GL_H
#include <GL/gl.h>
#endif /* HAVE_GL_GL_H */
#ifdef HAVE_GL_GLX_H
#include <GL/glx.h>
#endif /* HAVE_GL_GLX_H */

#endif /* HAVE_GL */

#include "global.h"

RCSID("$Id$");
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "pike_error.h"


#ifdef HAVE_GL

static void f_glGet(INT32 args)
{
  INT32 arg1;
  GLint i, i2[4];
  GLboolean b[4];
  GLfloat f[16];

  check_all_args("glGet", args, BIT_INT, 0);

  arg1=Pike_sp[0-args].u.integer;

  pop_n_elems(args);

  switch(arg1) {
  case GL_ACCUM_ALPHA_BITS:
  case GL_ACCUM_BLUE_BITS:
  case GL_ACCUM_GREEN_BITS:
  case GL_ACCUM_RED_BITS:
  case GL_ALPHA_BITS:
  case GL_ALPHA_TEST_FUNC:
  case GL_ATTRIB_STACK_DEPTH:
  case GL_AUX_BUFFERS:
  case GL_BLEND_DST:
  case GL_BLEND_SRC:
  case GL_BLUE_BITS:
  case GL_CLIENT_ATTRIB_STACK_DEPTH:
  case GL_COLOR_ARRAY_SIZE:
  case GL_COLOR_ARRAY_STRIDE:
  case GL_COLOR_ARRAY_TYPE:
  case GL_COLOR_MATERIAL_FACE:
  case GL_COLOR_MATERIAL_PARAMETER:
  case GL_CULL_FACE_MODE:
  case GL_DEPTH_BITS:
  case GL_DEPTH_FUNC:
  case GL_DRAW_BUFFER:
  case GL_EDGE_FLAG_ARRAY_STRIDE:
  case GL_FOG_HINT:
  case GL_FOG_MODE:
  case GL_FRONT_FACE:
  case GL_GREEN_BITS:
  case GL_INDEX_ARRAY_STRIDE:
  case GL_INDEX_ARRAY_TYPE:
  case GL_INDEX_BITS:
  case GL_INDEX_OFFSET:
  case GL_INDEX_SHIFT:
  case GL_INDEX_WRITEMASK:
  case GL_LINE_SMOOTH_HINT:
  case GL_LINE_STIPPLE_PATTERN:
  case GL_LINE_STIPPLE_REPEAT:
  case GL_LIST_BASE:
  case GL_LIST_INDEX:
  case GL_LIST_MODE:
  case GL_LOGIC_OP_MODE:
  case GL_MAP1_GRID_SEGMENTS:
  case GL_MATRIX_MODE:
  case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
  case GL_MAX_ATTRIB_STACK_DEPTH:
  case GL_MAX_CLIP_PLANES:
  case GL_MAX_EVAL_ORDER:
  case GL_MAX_LIGHTS:
  case GL_MAX_LIST_NESTING:
  case GL_MAX_MODELVIEW_STACK_DEPTH:
  case GL_MAX_NAME_STACK_DEPTH:
  case GL_MAX_PIXEL_MAP_TABLE:
  case GL_MAX_PROJECTION_STACK_DEPTH:
  case GL_MAX_TEXTURE_SIZE:
  case GL_MAX_TEXTURE_STACK_DEPTH:
  case GL_MODELVIEW_STACK_DEPTH:
  case GL_NAME_STACK_DEPTH:
  case GL_NORMAL_ARRAY_STRIDE:
  case GL_NORMAL_ARRAY_TYPE:
  case GL_PACK_ALIGNMENT:
  case GL_PACK_ROW_LENGTH:
  case GL_PACK_SKIP_PIXELS:
  case GL_PACK_SKIP_ROWS:
  case GL_PERSPECTIVE_CORRECTION_HINT:
  case GL_PIXEL_MAP_A_TO_A_SIZE:
  case GL_PIXEL_MAP_B_TO_B_SIZE:
  case GL_PIXEL_MAP_G_TO_G_SIZE:
  case GL_PIXEL_MAP_I_TO_A_SIZE:
  case GL_PIXEL_MAP_I_TO_B_SIZE:
  case GL_PIXEL_MAP_I_TO_G_SIZE:
  case GL_PIXEL_MAP_I_TO_I_SIZE:
  case GL_PIXEL_MAP_I_TO_R_SIZE:
  case GL_PIXEL_MAP_R_TO_R_SIZE:
  case GL_PIXEL_MAP_S_TO_S_SIZE:
  case GL_POINT_SMOOTH_HINT:
  case GL_POLYGON_MODE:
  case GL_POLYGON_SMOOTH_HINT:
  case GL_PROJECTION_STACK_DEPTH:
  case GL_READ_BUFFER:
  case GL_RED_BITS:
  case GL_RENDER_MODE:
  case GL_SHADE_MODEL:
  case GL_STENCIL_BITS:
  case GL_STENCIL_CLEAR_VALUE:
  case GL_STENCIL_FAIL:
  case GL_STENCIL_FUNC:
  case GL_STENCIL_PASS_DEPTH_FAIL:
  case GL_STENCIL_PASS_DEPTH_PASS:
  case GL_STENCIL_REF:
  case GL_STENCIL_VALUE_MASK:
  case GL_STENCIL_WRITEMASK:
  case GL_SUBPIXEL_BITS:
  case GL_TEXTURE_BINDING_1D:
  case GL_TEXTURE_BINDING_2D:
  case GL_TEXTURE_COORD_ARRAY_SIZE:
  case GL_TEXTURE_COORD_ARRAY_STRIDE:
  case GL_TEXTURE_COORD_ARRAY_TYPE:
  case GL_TEXTURE_STACK_DEPTH:
  case GL_UNPACK_ALIGNMENT:
  case GL_UNPACK_ROW_LENGTH:
  case GL_UNPACK_SKIP_PIXELS:
  case GL_UNPACK_SKIP_ROWS:
  case GL_VERTEX_ARRAY_SIZE:
  case GL_VERTEX_ARRAY_STRIDE:
  case GL_VERTEX_ARRAY_TYPE:
    glGetIntegerv(arg1, &i);
    push_int(i);
    break;

  case GL_MAP2_GRID_SEGMENTS:
  case GL_MAX_VIEWPORT_DIMS:
    glGetIntegerv(arg1, i2);
    push_int(i2[0]);
    push_int(i2[1]);
    f_aggregate(2);
    break;

  case GL_SCISSOR_BOX:
  case GL_VIEWPORT:
    glGetIntegerv(arg1, i2);
    push_int(i2[0]);
    push_int(i2[1]);
    push_int(i2[2]);
    push_int(i2[3]);
    f_aggregate(4);
    break;

  case GL_ALPHA_TEST:
  case GL_AUTO_NORMAL:
  case GL_BLEND:
  case GL_CLIP_PLANE0:
  case GL_CLIP_PLANE1:
  case GL_CLIP_PLANE2:
  case GL_CLIP_PLANE3:
  case GL_CLIP_PLANE4:
  case GL_CLIP_PLANE5:
  case GL_COLOR_ARRAY:
  case GL_COLOR_LOGIC_OP:
  case GL_COLOR_MATERIAL:
  case GL_CULL_FACE:
  case GL_CURRENT_RASTER_POSITION_VALID:
  case GL_DEPTH_TEST:
  case GL_DEPTH_WRITEMASK:
  case GL_DITHER:
  case GL_DOUBLEBUFFER:
  case GL_EDGE_FLAG:
  case GL_EDGE_FLAG_ARRAY:
  case GL_FOG:
  case GL_INDEX_ARRAY:
  case GL_INDEX_LOGIC_OP:
  case GL_INDEX_MODE:
  case GL_LIGHT0:
  case GL_LIGHT1:
  case GL_LIGHT2:
  case GL_LIGHT3:
  case GL_LIGHT4:
  case GL_LIGHT5:
  case GL_LIGHT6:
  case GL_LIGHT7:
  case GL_LIGHTING:
  case GL_LIGHT_MODEL_LOCAL_VIEWER:
  case GL_LIGHT_MODEL_TWO_SIDE:
  case GL_LINE_SMOOTH:
  case GL_LINE_STIPPLE:
  case GL_MAP1_COLOR_4:
  case GL_MAP1_INDEX:
  case GL_MAP1_NORMAL:
  case GL_MAP1_TEXTURE_COORD_1:
  case GL_MAP1_TEXTURE_COORD_2:
  case GL_MAP1_TEXTURE_COORD_3:
  case GL_MAP1_TEXTURE_COORD_4:
  case GL_MAP1_VERTEX_3:
  case GL_MAP1_VERTEX_4:
  case GL_MAP2_COLOR_4:
  case GL_MAP2_INDEX:
  case GL_MAP2_NORMAL:
  case GL_MAP2_TEXTURE_COORD_1:
  case GL_MAP2_TEXTURE_COORD_2:
  case GL_MAP2_TEXTURE_COORD_3:
  case GL_MAP2_TEXTURE_COORD_4:
  case GL_MAP2_VERTEX_3:
  case GL_MAP2_VERTEX_4:
  case GL_MAP_COLOR:
  case GL_MAP_STENCIL:
  case GL_NORMAL_ARRAY:
  case GL_NORMALIZE:
  case GL_PACK_LSB_FIRST:
  case GL_PACK_SWAP_BYTES:
  case GL_POINT_SMOOTH:
  case GL_POLYGON_OFFSET_FILL:
  case GL_POLYGON_OFFSET_LINE:
  case GL_POLYGON_OFFSET_POINT:
  case GL_POLYGON_SMOOTH:
  case GL_POLYGON_STIPPLE:
  case GL_RGBA_MODE:
  case GL_SCISSOR_TEST:
  case GL_STENCIL_TEST:
  case GL_STEREO:
  case GL_TEXTURE_1D:
  case GL_TEXTURE_2D:
  case GL_TEXTURE_COORD_ARRAY:
  case GL_TEXTURE_GEN_Q:
  case GL_TEXTURE_GEN_R:
  case GL_TEXTURE_GEN_S:
  case GL_TEXTURE_GEN_T:
  case GL_UNPACK_LSB_FIRST:
  case GL_UNPACK_SWAP_BYTES:
  case GL_VERTEX_ARRAY:
    glGetBooleanv(arg1, b);
    push_int(b[0]);
    break;

  case GL_ALPHA_BIAS:
  case GL_ALPHA_SCALE:
  case GL_ALPHA_TEST_REF:
  case GL_BLUE_BIAS:
  case GL_BLUE_SCALE:
  case GL_CURRENT_INDEX:
  case GL_CURRENT_RASTER_DISTANCE:
  case GL_CURRENT_RASTER_INDEX:
  case GL_DEPTH_BIAS:
  case GL_DEPTH_CLEAR_VALUE:
  case GL_DEPTH_SCALE:
  case GL_FOG_DENSITY:
  case GL_FOG_END:
  case GL_FOG_INDEX:
  case GL_FOG_START:
  case GL_GREEN_BIAS:
  case GL_GREEN_SCALE:
  case GL_INDEX_CLEAR_VALUE:
  case GL_LINE_WIDTH:
  case GL_LINE_WIDTH_GRANULARITY:
  case GL_POINT_SIZE:
  case GL_POINT_SIZE_GRANULARITY:
  case GL_POLYGON_OFFSET_FACTOR:
  case GL_POLYGON_OFFSET_UNITS:
  case GL_RED_BIAS:
  case GL_RED_SCALE:
  case GL_ZOOM_X:
  case GL_ZOOM_Y:
    glGetFloatv(arg1, f);
    push_float(f[0]);
    break;

  case GL_DEPTH_RANGE:
  case GL_LINE_WIDTH_RANGE:
  case GL_MAP1_GRID_DOMAIN:
  case GL_POINT_SIZE_RANGE:
    glGetFloatv(arg1, f);
    push_float(f[0]);
    push_float(f[1]);
    f_aggregate(2);
    break;

  case GL_CURRENT_NORMAL:
    glGetFloatv(arg1, f);
    push_float(f[0]);
    push_float(f[1]);
    push_float(f[2]);
    f_aggregate(3);
    break;

  case GL_ACCUM_CLEAR_VALUE:
  case GL_COLOR_CLEAR_VALUE:
  case GL_CURRENT_COLOR:
  case GL_CURRENT_RASTER_COLOR:
  case GL_CURRENT_RASTER_POSITION:
  case GL_CURRENT_RASTER_TEXTURE_COORDS:
  case GL_CURRENT_TEXTURE_COORDS:
  case GL_FOG_COLOR:
  case GL_LIGHT_MODEL_AMBIENT:
  case GL_MAP2_GRID_DOMAIN:
    glGetFloatv(arg1, f);
    push_float(f[0]);
    push_float(f[1]);
    push_float(f[2]);
    push_float(f[3]);
    f_aggregate(4);
    break;

  case GL_COLOR_WRITEMASK:
    glGetBooleanv(arg1, b);
    push_int(b[0]);
    push_int(b[1]);
    push_int(b[2]);
    push_int(b[3]);
    f_aggregate(4);
    break;

  case GL_MODELVIEW_MATRIX:
  case GL_PROJECTION_MATRIX:
  case GL_TEXTURE_MATRIX:
    glGetFloatv(arg1, f);
    push_float(f[0]);
    push_float(f[1]);
    push_float(f[2]);
    push_float(f[3]);
    push_float(f[4]);
    push_float(f[5]);
    push_float(f[6]);
    push_float(f[7]);
    push_float(f[8]);
    push_float(f[9]);
    push_float(f[10]);
    push_float(f[11]);
    push_float(f[12]);
    push_float(f[13]);
    push_float(f[14]);
    push_float(f[15]);
    f_aggregate(16);
    break;

  default:
    Pike_error("glGet: Unsupported parameter name\n");
  }
}

#endif /* HAVE_GL */


PIKE_MODULE_INIT
{
#ifdef HAVE_GL
  extern void GL_add_auto_funcs(void);

  add_function_constant("glGet", f_glGet,
			"function(int:int|float|array(int)|array(float))",
			OPT_SIDE_EFFECT);
  GL_add_auto_funcs();
#endif /* HAVE_GL */
}


PIKE_MODULE_EXIT
{
}
