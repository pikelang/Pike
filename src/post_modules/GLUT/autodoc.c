/*! @module GLUT
 *!
 *! @decl void glutAddMenuEntry(string,int)
 *!
 *! @decl void glutAddSubMenu(string,int)
 *!
 *! @decl void glutAttachMenu(int)
 *!
 *! @decl void glutButtonBoxFunc(function)
 *!
 *! @decl void glutChangeToMenuEntry(int,string,int)
 *!
 *! @decl void glutChangeToSubMenu(int,string,int)
 *!
 *! @decl void glutCopyColormap(int)
 *!
 *! @decl int glutCreateMenu(function)
 *!
 *! @decl int glutCreateSubWindow(int,int,int,int,int)
 *!
 *! @decl int glutCreateWindow(string)
 *!
 *! @decl void glutDestroyMenu(int)
 *!
 *! @decl void glutDestroyWindow(int)
 *!
 *! @decl void glutDetachMenu(int)
 *!
 *! @decl int glutDeviceGet(int)
 *!
 *! @decl void glutDialsFunc(function)
 *!
 *! @decl void glutDisplayFunc(function)
 *!
 *! @decl int glutEnterGameMode()
 *!
 *! @decl void glutEntryFunc(function)
 *!
 *! @decl void glutEstablishOverlay()
 *!
 *! @decl int glutExtensionSupported(string)
 *!
 *! @decl void glutForceJoystickFunc()
 *!
 *! @decl void glutFullScreen()
 *!
 *! @decl int glutGameModeGet(int)
 *!
 *! @decl void glutGameModeString(string)
 *!
 *! @decl int glutGet(int)
 *!
 *! @decl float glutGetColor(int,int)
 *!
 *! @decl int glutGetMenu()
 *!
 *! @decl int glutGetModifiers()
 *!
 *! @decl int glutGetWindow()
 *!
 *! @decl void glutHideOverlay()
 *!
 *! @decl void glutHideWindow()
 *!
 *! @decl void glutIconifyWindow()
 *!
 *! @decl void glutIdleFunc(function)
 *!
 *! @decl void glutIgnoreKeyRepeat(int)
 *!
 *! @decl void glutInitDisplayMode(int)
 *!
 *! @decl void glutInitDisplayString(string)
 *!
 *! @decl void glutInitWindowPosition(int,int)
 *!
 *! @decl void glutInitWindowSize(int,int)
 *!
 *! @decl void glutJoystickFunc(function,int)
 *!
 *! @decl void glutKeyboardFunc(function)
 *!
 *! @decl void glutKeyboardUpFunc(function)
 *!
 *! @decl int glutLayerGet(int)
 *!
 *! @decl void glutLeaveGameMode()
 *!
 *! @decl void glutMainLoop()
 *!
 *! @decl void glutMenuStateFunc(function)
 *!
 *! @decl void glutMenuStatusFunc(function)
 *!
 *! @decl void glutMotionFunc(function)
 *!
 *! @decl void glutMouseFunc(function)
 *!
 *! @decl void glutOverlayDisplayFunc(function)
 *!
 *! @decl void glutPassiveMotionFunc(function)
 *!
 *! @decl void glutPopWindow()
 *!
 *! @decl void glutPositionWindow(int,int)
 *!
 *! @decl void glutPostOverlayRedisplay()
 *!
 *! @decl void glutPostRedisplay()
 *!
 *! @decl void glutPostWindowOverlayRedisplay(int)
 *!
 *! @decl void glutPostWindowRedisplay(int)
 *!
 *! @decl void glutPushWindow()
 *!
 *! @decl void glutRemoveMenuItem(int)
 *!
 *! @decl void glutRemoveOverlay()
 *!
 *! @decl void glutReportErrors()
 *!
 *! @decl void glutReshapeFunc(function)
 *!
 *! @decl void glutReshapeWindow(int,int)
 *!
 *! @decl void glutSetColor(int,float,float,float)
 *!
 *! @decl void glutSetCursor(int)
 *!
 *! @decl void glutSetIconTitle(string)
 *!
 *! @decl void glutSetKeyRepeat(int)
 *!
 *! @decl void glutSetMenu(int)
 *!
 *! @decl void glutSetWindow(int)
 *!
 *! @decl void glutSetWindowTitle(string)
 *!
 *! @decl void glutSetupVideoResizing()
 *!
 *! @decl void glutShowOverlay()
 *!
 *! @decl void glutShowWindow()
 *!
 *! @decl void glutSolidCone(float,float,int,int)
 *!
 *! @decl void glutSolidCube(float)
 *!
 *! @decl void glutSolidDodecahedron()
 *!
 *! @decl void glutSolidIcosahedron()
 *!
 *! @decl void glutSolidOctahedron()
 *!
 *! @decl void glutSolidSphere(float,int,int)
 *!
 *! @decl void glutSolidTeapot(float)
 *!
 *! @decl void glutSolidTetrahedron()
 *!
 *! @decl void glutSolidTorus(float,float,int,int)
 *!
 *! @decl void glutSpaceballButtonFunc(function)
 *!
 *! @decl void glutSpaceballMotionFunc(function)
 *!
 *! @decl void glutSpaceballRotateFunc(function)
 *!
 *! @decl void glutSpecialFunc(function)
 *!
 *! @decl void glutSpecialUpFunc(function)
 *!
 *! @decl void glutStopVideoResizing()
 *!
 *! @decl void glutSwapBuffers()
 *!
 *! @decl void glutTabletButtonFunc(function)
 *!
 *! @decl void glutTabletMotionFunc(function)
 *!
 *! @decl void glutTimerFunc(int,function,int)
 *!
 *! @decl void glutUseLayer(int)
 *!
 *! @decl void glutVideoPan(int,int,int,int)
 *!
 *! @decl void glutVideoResize(int,int,int,int)
 *!
 *! @decl int glutVideoResizeGet(int)
 *!
 *! @decl void glutVisibilityFunc(function)
 *!
 *! @decl void glutWarpPointer(int,int)
 *!
 *! @decl void glutWindowStatusFunc(function)
 *!
 *! @decl void glutWireCone(float,float,int,int)
 *!
 *! @decl void glutWireCube(float)
 *!
 *! @decl void glutWireDodecahedron()
 *!
 *! @decl void glutWireIcosahedron()
 *!
 *! @decl void glutWireOctahedron()
 *!
 *! @decl void glutWireSphere(float,int,int)
 *!
 *! @decl void glutWireTeapot(float)
 *!
 *! @decl void glutWireTetrahedron()
 *!
 *! @decl void glutWireTorus(float,float,int,int)
 *!
 *! @decl constant GLUT_ACCUM
*!
 *! @decl constant GLUT_ACTIVE_ALT
*!
 *! @decl constant GLUT_ACTIVE_CTRL
*!
 *! @decl constant GLUT_ACTIVE_SHIFT
*!
 *! @decl constant GLUT_ALPHA
*!
 *! @decl constant GLUT_BLUE
*!
 *! @decl constant GLUT_CURSOR_BOTTOM_LEFT_CORNER
*!
 *! @decl constant GLUT_CURSOR_BOTTOM_RIGHT_CORNER
*!
 *! @decl constant GLUT_CURSOR_BOTTOM_SIDE
*!
 *! @decl constant GLUT_CURSOR_CROSSHAIR
*!
 *! @decl constant GLUT_CURSOR_CYCLE
*!
 *! @decl constant GLUT_CURSOR_DESTROY
*!
 *! @decl constant GLUT_CURSOR_FULL_CROSSHAIR
*!
 *! @decl constant GLUT_CURSOR_HELP
*!
 *! @decl constant GLUT_CURSOR_INFO
*!
 *! @decl constant GLUT_CURSOR_INHERIT
*!
 *! @decl constant GLUT_CURSOR_LEFT_ARROW
*!
 *! @decl constant GLUT_CURSOR_LEFT_RIGHT
*!
 *! @decl constant GLUT_CURSOR_LEFT_SIDE
*!
 *! @decl constant GLUT_CURSOR_NONE
*!
 *! @decl constant GLUT_CURSOR_RIGHT_ARROW
*!
 *! @decl constant GLUT_CURSOR_RIGHT_SIDE
*!
 *! @decl constant GLUT_CURSOR_SPRAY
*!
 *! @decl constant GLUT_CURSOR_TEXT
*!
 *! @decl constant GLUT_CURSOR_TOP_LEFT_CORNER
*!
 *! @decl constant GLUT_CURSOR_TOP_RIGHT_CORNER
*!
 *! @decl constant GLUT_CURSOR_TOP_SIDE
*!
 *! @decl constant GLUT_CURSOR_UP_DOWN
*!
 *! @decl constant GLUT_CURSOR_WAIT
*!
 *! @decl constant GLUT_DEPTH
*!
 *! @decl constant GLUT_DEVICE_IGNORE_KEY_REPEAT
*!
 *! @decl constant GLUT_DEVICE_KEY_REPEAT
*!
 *! @decl constant GLUT_DISPLAY_MODE_POSSIBLE
*!
 *! @decl constant GLUT_DOUBLE
*!
 *! @decl constant GLUT_DOWN
*!
 *! @decl constant GLUT_ELAPSED_TIME
*!
 *! @decl constant GLUT_ENTERED
*!
 *! @decl constant GLUT_FULLY_COVERED
*!
 *! @decl constant GLUT_FULLY_RETAINED
*!
 *! @decl constant GLUT_GAME_MODE_ACTIVE
*!
 *! @decl constant GLUT_GAME_MODE_DISPLAY_CHANGED
*!
 *! @decl constant GLUT_GAME_MODE_HEIGHT
*!
 *! @decl constant GLUT_GAME_MODE_PIXEL_DEPTH
*!
 *! @decl constant GLUT_GAME_MODE_POSSIBLE
*!
 *! @decl constant GLUT_GAME_MODE_REFRESH_RATE
*!
 *! @decl constant GLUT_GAME_MODE_WIDTH
*!
 *! @decl constant GLUT_GREEN
*!
 *! @decl constant GLUT_HAS_DIAL_AND_BUTTON_BOX
*!
 *! @decl constant GLUT_HAS_JOYSTICK
*!
 *! @decl constant GLUT_HAS_KEYBOARD
*!
 *! @decl constant GLUT_HAS_MOUSE
*!
 *! @decl constant GLUT_HAS_OVERLAY
*!
 *! @decl constant GLUT_HAS_SPACEBALL
*!
 *! @decl constant GLUT_HAS_TABLET
*!
 *! @decl constant GLUT_HIDDEN
*!
 *! @decl constant GLUT_INDEX
*!
 *! @decl constant GLUT_INIT_DISPLAY_MODE
*!
 *! @decl constant GLUT_INIT_WINDOW_HEIGHT
*!
 *! @decl constant GLUT_INIT_WINDOW_WIDTH
*!
 *! @decl constant GLUT_INIT_WINDOW_X
*!
 *! @decl constant GLUT_INIT_WINDOW_Y
*!
 *! @decl constant GLUT_JOYSTICK_AXES
*!
 *! @decl constant GLUT_JOYSTICK_BUTTONS
*!
 *! @decl constant GLUT_JOYSTICK_BUTTON_A
*!
 *! @decl constant GLUT_JOYSTICK_BUTTON_B
*!
 *! @decl constant GLUT_JOYSTICK_BUTTON_C
*!
 *! @decl constant GLUT_JOYSTICK_BUTTON_D
*!
 *! @decl constant GLUT_JOYSTICK_POLL_RATE
*!
 *! @decl constant GLUT_KEY_DOWN
*!
 *! @decl constant GLUT_KEY_END
*!
 *! @decl constant GLUT_KEY_F1
*!
 *! @decl constant GLUT_KEY_F10
*!
 *! @decl constant GLUT_KEY_F11
*!
 *! @decl constant GLUT_KEY_F12
*!
 *! @decl constant GLUT_KEY_F2
*!
 *! @decl constant GLUT_KEY_F3
*!
 *! @decl constant GLUT_KEY_F4
*!
 *! @decl constant GLUT_KEY_F5
*!
 *! @decl constant GLUT_KEY_F6
*!
 *! @decl constant GLUT_KEY_F7
*!
 *! @decl constant GLUT_KEY_F8
*!
 *! @decl constant GLUT_KEY_F9
*!
 *! @decl constant GLUT_KEY_HOME
*!
 *! @decl constant GLUT_KEY_INSERT
*!
 *! @decl constant GLUT_KEY_LEFT
*!
 *! @decl constant GLUT_KEY_PAGE_DOWN
*!
 *! @decl constant GLUT_KEY_PAGE_UP
*!
 *! @decl constant GLUT_KEY_REPEAT_DEFAULT
*!
 *! @decl constant GLUT_KEY_REPEAT_OFF
*!
 *! @decl constant GLUT_KEY_REPEAT_ON
*!
 *! @decl constant GLUT_KEY_RIGHT
*!
 *! @decl constant GLUT_KEY_UP
*!
 *! @decl constant GLUT_LAYER_IN_USE
*!
 *! @decl constant GLUT_LEFT
*!
 *! @decl constant GLUT_LEFT_BUTTON
*!
 *! @decl constant GLUT_LUMINANCE
*!
 *! @decl constant GLUT_MENU_IN_USE
*!
 *! @decl constant GLUT_MENU_NOT_IN_USE
*!
 *! @decl constant GLUT_MENU_NUM_ITEMS
*!
 *! @decl constant GLUT_MIDDLE_BUTTON
*!
 *! @decl constant GLUT_MULTISAMPLE
*!
 *! @decl constant GLUT_NORMAL
*!
 *! @decl constant GLUT_NORMAL_DAMAGED
*!
 *! @decl constant GLUT_NOT_VISIBLE
*!
 *! @decl constant GLUT_NUM_BUTTON_BOX_BUTTONS
*!
 *! @decl constant GLUT_NUM_DIALS
*!
 *! @decl constant GLUT_NUM_MOUSE_BUTTONS
*!
 *! @decl constant GLUT_NUM_SPACEBALL_BUTTONS
*!
 *! @decl constant GLUT_NUM_TABLET_BUTTONS
*!
 *! @decl constant GLUT_OVERLAY
*!
 *! @decl constant GLUT_OVERLAY_DAMAGED
*!
 *! @decl constant GLUT_OVERLAY_POSSIBLE
*!
 *! @decl constant GLUT_OWNS_JOYSTICK
*!
 *! @decl constant GLUT_PARTIALLY_RETAINED
*!
 *! @decl constant GLUT_RED
*!
 *! @decl constant GLUT_RGB
*!
 *! @decl constant GLUT_RGBA
*!
 *! @decl constant GLUT_RIGHT_BUTTON
*!
 *! @decl constant GLUT_SCREEN_HEIGHT
*!
 *! @decl constant GLUT_SCREEN_HEIGHT_MM
*!
 *! @decl constant GLUT_SCREEN_WIDTH
*!
 *! @decl constant GLUT_SCREEN_WIDTH_MM
*!
 *! @decl constant GLUT_SINGLE
*!
 *! @decl constant GLUT_STENCIL
*!
 *! @decl constant GLUT_STEREO
*!
 *! @decl constant GLUT_TRANSPARENT_INDEX
*!
 *! @decl constant GLUT_UP
*!
 *! @decl constant GLUT_VIDEO_RESIZE_HEIGHT
*!
 *! @decl constant GLUT_VIDEO_RESIZE_HEIGHT_DELTA
*!
 *! @decl constant GLUT_VIDEO_RESIZE_IN_USE
*!
 *! @decl constant GLUT_VIDEO_RESIZE_POSSIBLE
*!
 *! @decl constant GLUT_VIDEO_RESIZE_WIDTH
*!
 *! @decl constant GLUT_VIDEO_RESIZE_WIDTH_DELTA
*!
 *! @decl constant GLUT_VIDEO_RESIZE_X
*!
 *! @decl constant GLUT_VIDEO_RESIZE_X_DELTA
*!
 *! @decl constant GLUT_VIDEO_RESIZE_Y
*!
 *! @decl constant GLUT_VIDEO_RESIZE_Y_DELTA
*!
 *! @decl constant GLUT_VISIBLE
*!
 *! @decl constant GLUT_WINDOW_ACCUM_ALPHA_SIZE
*!
 *! @decl constant GLUT_WINDOW_ACCUM_BLUE_SIZE
*!
 *! @decl constant GLUT_WINDOW_ACCUM_GREEN_SIZE
*!
 *! @decl constant GLUT_WINDOW_ACCUM_RED_SIZE
*!
 *! @decl constant GLUT_WINDOW_ALPHA_SIZE
*!
 *! @decl constant GLUT_WINDOW_BLUE_SIZE
*!
 *! @decl constant GLUT_WINDOW_BUFFER_SIZE
*!
 *! @decl constant GLUT_WINDOW_COLORMAP_SIZE
*!
 *! @decl constant GLUT_WINDOW_CURSOR
*!
 *! @decl constant GLUT_WINDOW_DEPTH_SIZE
*!
 *! @decl constant GLUT_WINDOW_DOUBLEBUFFER
*!
 *! @decl constant GLUT_WINDOW_FORMAT_ID
*!
 *! @decl constant GLUT_WINDOW_GREEN_SIZE
*!
 *! @decl constant GLUT_WINDOW_HEIGHT
*!
 *! @decl constant GLUT_WINDOW_NUM_CHILDREN
*!
 *! @decl constant GLUT_WINDOW_NUM_SAMPLES
*!
 *! @decl constant GLUT_WINDOW_PARENT
*!
 *! @decl constant GLUT_WINDOW_RED_SIZE
*!
 *! @decl constant GLUT_WINDOW_RGBA
*!
 *! @decl constant GLUT_WINDOW_STENCIL_SIZE
*!
 *! @decl constant GLUT_WINDOW_STEREO
*!
 *! @decl constant GLUT_WINDOW_WIDTH
*!
 *! @decl constant GLUT_WINDOW_X
*!
 *! @decl constant GLUT_WINDOW_Y
*!
 *! @endmodule
 */
