array func_misc = 
({
   ({ "glutGetColor","FII" }),             
   // GLfloat glutGetColor(int ndx, int component);

   ({ "glutCreateMenu", "IC(I)" }),        
   // int glutCreateMenu(void (GLUTCALLBACK *)(int));

   ({ "glutCreateSubWindow", "IIIIII" }),  
   // int glutCreateSubWindow(int win, int x, int y, int width, int height);

   ({ "glutCreateWindow", "IS" }),         
   // int glutCreateWindow(const char *title);

   ({ "glutDeviceGet", "IE" }),            
   // int glutDeviceGet(GLenum type);

   ({ "glutEnterGameMode", "I" }),         
   // int glutEnterGameMode(void);

   ({ "glutExtensionSupported","IS" }),    
   // int glutExtensionSupported(const char *name);

   ({ "glutGameModeGet","IE" }),           
   // int glutGameModeGet(GLenum mode);

   ({ "glutGet","IE" }),                   
   // int glutGet(GLenum type);

   ({ "glutGetMenu","I" }),                
   // int glutGetMenu(void);

   ({ "glutGetModifiers","I" }),           
   // int glutGetModifiers(void);

   ({ "glutGetWindow","I" }),             
   // int glutGetWindow(void);

   ({ "glutLayerGet","IE" }),              
   // int glutLayerGet(GLenum type);

   ({ "glutVideoResizeGet","IE" }),        
   // int glutVideoResizeGet(GLenum param);

   ({ "glutAddMenuEntry","VSI" }),         
   // void glutAddMenuEntry(const char *label, int value);

   ({ "glutAddSubMenu","VSI" }),           
   // void glutAddSubMenu(const char *label, int submenu);

   ({ "glutAttachMenu","VI" }),            
   // void glutAttachMenu(int button);

   ({ "glutButtonBoxFunc","VC(II)" }),     
   // void glutButtonBoxFunc(void (GLUTCALLBACK * func)(int button, int state));

   ({ "glutChangeToMenuEntry","VISI" }),   
   // void glutChangeToMenuEntry(int item, const char *label, int value);

   ({ "glutChangeToSubMenu","VISI" }),     
   // void glutChangeToSubMenu(int item, const char *label, int submenu);

   ({ "glutCopyColormap","VI" }),          
   // void glutCopyColormap(int win);

   ({ "glutDestroyMenu","VI" }),           
   // void glutDestroyMenu(int menu);

   ({ "glutDestroyWindow","VI" }),         
   // void glutDestroyWindow(int win);

   ({ "glutDetachMenu","VI" }),            
   // void glutDetachMenu(int button);

   ({ "glutDialsFunc","VC(II)" }),         
   // void glutDialsFunc(void (GLUTCALLBACK * func)(int dial, int value));

   ({ "glutDisplayFunc","VC()" }),        
   // void glutDisplayFunc(void (GLUTCALLBACK * func)(void));

   ({ "glutEntryFunc","VC(I)" }),             
   // void glutEntryFunc(void (GLUTCALLBACK * func)(int state));

   ({ "glutEstablishOverlay","V" }),       
   // void glutEstablishOverlay(void);

   ({ "glutForceJoystickFunc","V" }),      
   // void glutForceJoystickFunc(void);

   ({ "glutFullScreen","V" }),             
   // void glutFullScreen(void);

   ({ "glutGameModeString","VS" }),        
   // void glutGameModeString(const char *string);

   ({ "glutHideOverlay","V" }),            
   // void glutHideOverlay(void);

   ({ "glutHideWindow","V" }),            
   // void glutHideWindow(void);

   ({ "glutIconifyWindow","V" }),          
   // void glutIconifyWindow(void);

   ({ "glutIdleFunc","VC()" }),           
   // void glutIdleFunc(void (GLUTCALLBACK * func)(void));

   ({ "glutIgnoreKeyRepeat","VI" }),       
   // void glutIgnoreKeyRepeat(int ignore);

//   ({ "glutInit","VII" }),                 
   // void glutInit(int *argcp, char **argv);

   ({ "glutInitDisplayMode","VU" }),       
   // void glutInitDisplayMode(unsigned int mode);

   ({ "glutInitDisplayString","VS" }),     
   // void glutInitDisplayString(const char *string);

   ({ "glutInitWindowPosition","VII" }),   
   // void glutInitWindowPosition(int x, int y);

   ({ "glutInitWindowSize","VII" }),       
   // void glutInitWindowSize(int width, int height);

   ({ "glutJoystickFunc","VC(UIII)I" }),   
   // void glutJoystickFunc(void (GLUTCALLBACK * func)(unsigned int buttonMask, int x, int y, int z), int pollInterval);

   ({ "glutKeyboardFunc","VC(UII)" }),     
   // void glutKeyboardFunc(void (GLUTCALLBACK * func)(unsigned char key, int x, int y));

   ({ "glutKeyboardUpFunc","VC(UII)" }),  
   // void glutKeyboardUpFunc(void (GLUTCALLBACK * func)(unsigned char key, int x, int y));

   ({ "glutLeaveGameMode","V" }),          
   // void glutLeaveGameMode(void);

   ({ "glutMainLoop","V" }),               
   // void glutMainLoop(void);

   ({ "glutMenuStateFunc","VC(I)" }),      
   // void glutMenuStateFunc(void (GLUTCALLBACK * func)(int state));

   ({ "glutMenuStatusFunc","VC(III)" }),   
   // void glutMenuStatusFunc(void (GLUTCALLBACK * func)(int status, int x, int y));

   ({ "glutMotionFunc","VC(II)" }),        
   // void glutMotionFunc(void (GLUTCALLBACK * func)(int x, int y));

   ({ "glutMouseFunc","VC(IIII)" }),       
   // void glutMouseFunc(void (GLUTCALLBACK * func)(int button, int state, int x, int y));

   ({ "glutOverlayDisplayFunc","VC()" }), 
   // void glutOverlayDisplayFunc(void (GLUTCALLBACK * func)(void));

   ({ "glutPassiveMotionFunc","VC(II)" }), 
   // void glutPassiveMotionFunc(void (GLUTCALLBACK * func)(int x, int y));

   ({ "glutPopWindow","V" }),              
   // void glutPopWindow(void);

   ({ "glutPositionWindow","VII" }),       
   // void glutPositionWindow(int x, int y);

   ({ "glutPostOverlayRedisplay","V" }),   
   // void glutPostOverlayRedisplay(void);

   ({ "glutPostRedisplay","V" }),          
   // void glutPostRedisplay(void);

   ({ "glutPostWindowOverlayRedisplay","VI" }),    
   // void glutPostWindowOverlayRedisplay(int win);

   ({ "glutPostWindowRedisplay","VI" }),   
   // void glutPostWindowRedisplay(int win);

   ({ "glutPushWindow","V" }),             
   // void glutPushWindow(void);

   ({ "glutRemoveMenuItem","VI" }),        
   // void glutRemoveMenuItem(int item);

   ({ "glutRemoveOverlay","V" }),          
   // void glutRemoveOverlay(void);

   ({ "glutReportErrors","V" }),           
   // void glutReportErrors(void);

   ({ "glutReshapeFunc","VC(II)" }),       
   // void glutReshapeFunc(void (GLUTCALLBACK * func)(int width, int height));

   ({ "glutReshapeWindow","VII" }),      
   // void glutReshapeWindow(int width, int height);

   ({ "glutSetColor","VIFFF" }),           
   // void glutSetColor(int, GLfloat red, GLfloat green, GLfloat blue);

   ({ "glutSetCursor","VI" }),             
   // void glutSetCursor(int cursor);

   ({ "glutSetIconTitle","VS" }),          
   // void glutSetIconTitle(const char *title);

   ({ "glutSetKeyRepeat","VI" }),          
   // void glutSetKeyRepeat(int repeatMode);

   ({ "glutSetMenu","VI" }),               
   // void glutSetMenu(int menu);

   ({ "glutSetWindow","VI" }),             
   // void glutSetWindow(int win);

   ({ "glutSetWindowTitle","VS" }),        
   // void glutSetWindowTitle(const char *title);

   ({ "glutSetupVideoResizing","V" }),     
   // void glutSetupVideoResizing(void);

   ({ "glutShowOverlay","V" }),            
   // void glutShowOverlay(void);

   ({ "glutShowWindow","V" }),             
   // void glutShowWindow(void);

   ({ "glutSolidCone","VRRII" }),          
   // void glutSolidCone(GLdouble base, GLdouble height, GLint slices, GLint stacks);

   ({ "glutSolidCube","VR" }),             
   // void glutSolidCube(GLdouble size);

   ({ "glutSolidDodecahedron","V" }),      
   // void glutSolidDodecahedron(void);

   ({ "glutSolidIcosahedron","V" }),       
   // void glutSolidIcosahedron(void);

   ({ "glutSolidOctahedron","V" }),        
   // void glutSolidOctahedron(void);

   ({ "glutSolidSphere","VRII" }),         
   // void glutSolidSphere(GLdouble radius, GLint slices, GLint stacks);

   ({ "glutSolidTeapot","VR" }),           
   // void glutSolidTeapot(GLdouble size);

   ({ "glutSolidTetrahedron","V" }),       
   // void glutSolidTetrahedron(void);

   ({ "glutSolidTorus","VRRII" }),         
   // void glutSolidTorus(GLdouble innerRadius, GLdouble outerRadius, GLint sides, GLint rings);

   ({ "glutSpaceballButtonFunc","VC(II)"}),        
   // void glutSpaceballButtonFunc(void (GLUTCALLBACK * func)(int button, int state));

   ({ "glutSpaceballMotionFunc","VC(III)" }),      
   // void glutSpaceballMotionFunc(void (GLUTCALLBACK * func)(int x, int y, int z));

   ({ "glutSpaceballRotateFunc","VC(III)" }),      
   // void glutSpaceballRotateFunc(void (GLUTCALLBACK * func)(int x, int y, int z));

   ({ "glutSpecialFunc","VC(III)" }),      
   // void glutSpecialFunc(void (GLUTCALLBACK * func)(int key, int x, int y));

   ({ "glutSpecialUpFunc","VC(III)" }),    
   // void glutSpecialUpFunc(void (GLUTCALLBACK * func)(int key, int x, int y));

   ({ "glutStopVideoResizing","V" }),      
   // void glutStopVideoResizing(void);

   ({ "glutSwapBuffers","V" }),            
   // void glutSwapBuffers(void);

   ({ "glutTabletButtonFunc","VC(IIII)" }),
   // void glutTabletButtonFunc(void (GLUTCALLBACK * func)(int button, int state, int x, int y));

   ({ "glutTabletMotionFunc","VC(II)" }),  
   // void glutTabletMotionFunc(void (GLUTCALLBACK * func)(int x, int y));

   ({ "glutTimerFunc","VUC(I)I" }),        
   // void glutTimerFunc(unsigned int millis, void (GLUTCALLBACK * func)(int value), int value);

   ({ "glutUseLayer","VE" }),              
   // void glutUseLayer(GLenum layer);

   ({ "glutVideoPan","VIIII" }),         
   // void glutVideoPan(int x, int y, int width, int height);

   ({ "glutVideoResize","VIIII" }),      
   // void glutVideoResize(int x, int y, int width, int height);

   ({ "glutVisibilityFunc","VC(I)" }),     
   // void glutVisibilityFunc(void (GLUTCALLBACK * func)(int state));

   ({ "glutWarpPointer","VII" }),        
   // void glutWarpPointer(int x, int y);

   ({ "glutWindowStatusFunc","VC(I)" }),   
   // void glutWindowStatusFunc(void (GLUTCALLBACK * func)(int state));

   ({ "glutWireCone","VRRII" }),           
   // void glutWireCone(GLdouble base, GLdouble height, GLint slices, GLint stacks);

   ({ "glutWireCube","VR" }),              
   // void glutWireCube(GLdouble size);

   ({ "glutWireDodecahedron","V" }),       
   // void glutWireDodecahedron(void);

   ({ "glutWireIcosahedron","V" }),        
   // void glutWireIcosahedron(void);

   ({ "glutWireOctahedron","V" }),         
   // void glutWireOctahedron(void);

   ({ "glutWireSphere","VRII" }),          
   // void glutWireSphere(GLdouble radius, GLint slices, GLint stacks);

   ({ "glutWireTeapot","VR" }),            
   // void glutWireTeapot(GLdouble size);

   ({ "glutWireTetrahedron","V" }),        
   // void glutWireTetrahedron(void);

   ({ "glutWireTorus","VRRII" }),          
   // void glutWireTorus(GLdouble innerRadius, GLdouble outerRadius, GLint sides, GLint rings);
});


/* Not implemented:

int glutBitmapLength(void *font, const unsigned char *string);
int glutBitmapWidth(void *font, int character);
int glutStrokeLength(void *font, const unsigned char *string);
int glutStrokeWidth(void *font, int character);
void glutBitmapCharacter(void *font, int character);
void glutStrokeCharacter(void *font, int character);

*/
