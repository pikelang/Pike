array funcEV = ({
  "glBegin",
  "glCullFace",

  "glDisable",
  "glEnable",
  "glFrontFace",
  "glMatrixMode",
  "glShadeModel",
});
array funcV = ({
  "glEnd",

  "glEndList",
  "glFlush",
  "glLoadIdentity",
  "glPopMatrix",
  "glPushMatrix",
});
array func_misc = ({
  ({"glAccum", "VEF"}),
  ({"glAlphaFunc", "VEF"}),
  ({"glArrayElement", "VI"}),
  ({"glBindTexture","VEI"}),
  ({"glBlendFunc", "VEE"}),
  ({"glCallList","VI"}),
  ({"glClear","VB"}),
  ({"glClearAccum", "V+FFFF"}),
  ({"glClearColor", "V+FFFF"}),
  ({"glClearDepth", "VD"}),
  ({"glClearIndex", "VF"}),
  ({"glClearStencil", "VI"}),
  ({"glClipPlane", "VE=DDDD"}),
  ({"glColor", "V+ZZZZ"}),
  ({"glColorMask", "VOOOO"}),
  ({"glColorMaterial", "VEE"}),
  ({"glCopyPixels", "VIIIIE"}),
  ({"glCopyTexImage1D", "VEIEIIII"}),
  ({"glCopyTexImage2D", "VEIEIIIII"}),
  ({"glCopyTexSubImage1D", "VEIIIII"}),
  ({"glCopyTexSubImage2D", "VEIIIIIII"}),
  ({"glDeleteLists", "VII"}),

  ({"glFrustum", "VDDDDDD"}),
  ({"glOrtho", "VDDDDDD"}),
  ({"glViewport", "VIIII"}),
  ({"glTranslate", "V!RRR"}),
  ({"glScale", "V!RRR"}),
  ({"glRotate", "V!RRRR"}),
  ({"glMaterial","VEE@Q"}),
  ({"glFog","VE@Q"}),
  ({"glLightModel","VE@Q"}),
  ({"glGenLists","II"}),
  ({"glNewList","VIE"}),
  ({"glNormal","V#ZZZ"}),
  ({"glVertex","V+ZZZ"}),
  ({"glIsEnabled","OE"}),
  ({"glTexEnv","VEE@Q"}),
  ({"glTexParameter","VEE@Q"}),
  ({"glTexCoord", "V+Z"}),
  ({"glTexGen","VEE@Z"}),
  ({"glTexImage2D","VEIIwhIfti"}),
});
mapping func_cat = ([
  "VE":funcEV,
  "V":funcV,
]);
/*
  Not implemented:

  glAreTexturesResident
  glBitmap
  glCallLists
  glColorPointer

*/
