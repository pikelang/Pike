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
  ({"glTranslate", "V!RRR"}),
  ({"glScale", "V!RRR"}),
  ({"glRotate", "V!RRRR"}),
  ({"glLight","VEE@Q"}),
  ({"glMaterial","VEE@Q"}),
  ({"glGenLists","II"}),
  ({"glNewList","VIE"}),
  ({"glCallList","VI"}),
  ({"glNormal","V#ZZZ"}),
  ({"glVertex","V+ZZZ"}),
  ({"glClear","VB"}),
});
mapping func_cat = ([
  "VE":funcEV,
  "V":funcV,
]);
