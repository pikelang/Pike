array funcEV = ({
  "glEnable",
  "glDisable",
  "glShadeModel",
  "glFrontFace",
  "glCullFace",
  "glMatrixMode",
  "glBegin",
});
array funcV = ({
  "glLoadIdentity",
  "glEnd",
  "glEndList",
  "glFlush",
  "glPushMatrix",
  "glPopMatrix",
});
array func_misc = ({
  ({"glClearColor", "V+FFFF"}),
  ({"glFrustum", "VDDDDDD"}),
  ({"glOrtho", "VDDDDDD"}),
  ({"glViewport", "VIIII"}),
  ({"glTranslate", "V!RRR"}),
  ({"glScale", "V!RRR"}),
  ({"glRotate", "V!RRRR"}),
  ({"glLight","VEE@Q"}),
  ({"glMaterial","VEE@Q"}),
  ({"glFog","VE@Q"}),
  ({"glLightModel","VE@Q"}),
  ({"glGenLists","II"}),
  ({"glNewList","VIE"}),
  ({"glCallList","VI"}),
  ({"glNormal","V#ZZZ"}),
  ({"glVertex","V+ZZZ"}),
  ({"glColor","V+ZZZZ"}),
  ({"glClear","VB"}),
  ({"glIsEnabled","OE"}),
  ({"glBlendFunc","VEE"}),
});
mapping func_cat = ([
  "VE":funcEV,
  "V":funcV,
]);
