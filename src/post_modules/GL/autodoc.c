
/* AutoDoc generated from OpenGL man pages
$Id: autodoc.c,v 1.1 2002/02/12 23:57:42 nilsson Exp $ */

/*! @module GL
 */

/*!@decl void glEnd()
 *!
 *!@[glBegin] and @[glEnd] delimit the vertices that define a primitive or
 *!a group of like primitives.
 *!@[glBegin] accepts a single argument that specifies in which of ten ways the
 *!vertices are interpreted.
 *!Taking @i{n@} as an integer count starting at one,
 *!and @i{N@} as the total number of vertices specified,
 *!the interpretations are as follows:
 *!@xml{<matrix>
 *!<r><c><ref>GL_POINTS</ref>
 *!</c><c>Treats each vertex as a single point.
 *!Vertex <i>n</i> defines point <i>n</i>.
 *!<i>N</i> points are drawn.
 *!</c></r>
 *!<r><c><ref>GL_LINES</ref>
 *!</c><c>Treats each pair of vertices as an independent line segment.
 *!Vertices <i>2n-1</i> and <i>2n</i> define line <i>n</i>.
 *!<i>N/2</i> lines are drawn.
 *!</c></r>
 *!<r><c><ref>GL_LINE_STRIP</ref>
 *!</c><c>Draws a connected group of line segments from the first vertex
 *!to the last.
 *!Vertices <i>n</i> and <i>n+1</i> define line <i>n</i>.
 *!<i>N-1</i> lines are drawn.
 *!</c></r>
 *!<r><c><ref>GL_LINE_LOOP</ref>
 *!</c><c>Draws a connected group of line segments from the first vertex
 *!to the last,
 *!then back to the first.
 *!Vertices <i>n</i> and <i>n+1</i> define line <i>n</i>.
 *!The last line, however, is defined by vertices <i>N</i> and <i>1</i>.
 *!<i>N</i> lines are drawn.
 *!</c></r>
 *!<r><c><ref>GL_TRIANGLES</ref>
 *!</c><c>Treats each triplet of vertices as an independent triangle.
 *!Vertices <i>3n-2</i>,
 *!<i>3n-1</i>,
 *!and <i>3n</i> define triangle <i>n</i>.
 *!<i>N/3</i> triangles are drawn.
 *!.BP
 *!</c></r>
 *!<r><c><ref>GL_TRIANGLE_STRIP</ref>
 *!</c><c>Draws a connected group of triangles.
 *!One triangle is defined for each vertex presented after the first two vertices.
 *!For odd <i>n</i>, vertices <i>n</i>,
 *!<i>n+1</i>,
 *!and <i>n+2</i> define triangle <i>n</i>.
 *!For even <i>n</i>,
 *!vertices <i>n+1</i>,
 *!<i>n</i>,
 *!and <i>n+2</i> define triangle <i>n</i>.
 *!<i>N-2</i> triangles are drawn.
 *!</c></r>
 *!<r><c><ref>GL_TRIANGLE_FAN</ref>
 *!</c><c>Draws a connected group of triangles.
 *!One triangle is defined for each vertex presented after the first two vertices.
 *!Vertices <i>1</i>,
 *!<i>n+1</i>,
 *!and <i>n+2</i> define triangle <i>n</i>.
 *!<i>N-2</i> triangles are drawn.
 *!</c></r>
 *!<r><c><ref>GL_QUADS</ref>
 *!</c><c>Treats each group of four vertices as an independent quadrilateral.
 *!Vertices <i>4n-3</i>,
 *!<i>4n-2</i>,
 *!<i>4n-1</i>,
 *!and <i>4n</i> define quadrilateral <i>n</i>.
 *!<i>N/4</i> quadrilaterals are drawn.
 *!</c></r>
 *!<r><c><ref>GL_QUAD_STRIP</ref>
 *!</c><c>Draws a connected group of quadrilaterals.
 *!One quadrilateral is defined for each pair of vertices presented
 *!after the first pair.
 *!Vertices <i>2n-1</i>,
 *!<i>2n</i>,
 *!<i>2n+2</i>,
 *!and <i>2n+1</i> define quadrilateral <i>n</i>.
 *!<i>N/2-1</i> quadrilaterals are drawn.
 *!Note that the order in which vertices are used to construct a quadrilateral
 *!from strip data is different from that used with independent data.
 *!</c></r>
 *!<r><c><ref>GL_POLYGON</ref>
 *!</c><c>Draws a single,
 *!convex polygon.
 *!Vertices <i>1</i> through <i>N</i> define this polygon.
 *!</c></r>
 *!</matrix>@}
 *!
 *!Only a subset of GL commands can be used between @[glBegin] and @[glEnd].
 *!The commands are
 *!@[glVertex],
 *!@[glColor],
 *!@[glIndex],
 *!@[glNormal],
 *!@[glTexCoord],
 *!@[glEvalCoord],
 *!@[glEvalPoint],
 *!@[glArrayElement],
 *!@[glMaterial], and
 *!@[glEdgeFlag].
 *!Also,
 *!it is acceptable to use
 *!@[glCallList] or
 *!@[glCallLists] to execute
 *!display lists that include only the preceding commands.
 *!If any other GL command is executed between @[glBegin] and @[glEnd],
 *!the error flag is set and the command is ignored.
 *!
 *!Regardless of the value chosen for @i{mode@},
 *!there is no limit to the number of vertices that can be defined
 *!between @[glBegin] and @[glEnd].
 *!Lines,
 *!triangles,
 *!quadrilaterals,
 *!and polygons that are incompletely specified are not drawn.
 *!Incomplete specification results when either too few vertices are
 *!provided to specify even a single primitive or when an incorrect multiple 
 *!of vertices is specified. The incomplete primitive is ignored; the rest are drawn.
 *!
 *!The minimum specification of vertices
 *!for each primitive is as follows:
 *!1 for a point,
 *!2 for a line,
 *!3 for a triangle,
 *!4 for a quadrilateral,
 *!and 3 for a polygon.
 *!Modes that require a certain multiple of vertices are
 *!@[GL_LINES] (2),
 *!@[GL_TRIANGLES] (3),
 *!@[GL_QUADS] (4),
 *!and @[GL_QUAD_STRIP] (2).
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies the primitive or primitives that will be created from vertices
 *!presented between @[glBegin] and the subsequent @[glEnd].
 *!Ten symbolic constants are accepted:
 *!@[GL_POINTS],
 *!@[GL_LINES],
 *!@[GL_LINE_STRIP],
 *!@[GL_LINE_LOOP],
 *!@[GL_TRIANGLES],
 *!@[GL_TRIANGLE_STRIP],
 *!@[GL_TRIANGLE_FAN],
 *!@[GL_QUADS],
 *!@[GL_QUAD_STRIP], and
 *!@[GL_POLYGON].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is set to an unaccepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glBegin] is executed between a 
 *!@[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glEnd] is executed without being
 *!preceded by a @[glBegin].
 *!
 *!@[GL_INVALID_OPERATION] is generated if a command other than
 *!@[glVertex],
 *!@[glColor],
 *!@[glIndex],
 *!@[glNormal],
 *!@[glTexCoord],
 *!@[glEvalCoord],
 *!@[glEvalPoint],
 *!@[glArrayElement],
 *!@[glMaterial],
 *!@[glEdgeFlag],
 *!@[glCallList], or
 *!@[glCallLists] is executed between
 *!the execution of @[glBegin] and the corresponding
 *!execution @[glEnd].
 *!
 *!Execution of 
 *!@[glEnableClientState],
 *!@[glDisableClientState],
 *!@[glEdgeFlagPointer],
 *!@[glTexCoordPointer],
 *!@[glColorPointer],
 *!@[glIndexPointer],
 *!@[glNormalPointer],
 *!
 *!@[glVertexPointer],
 *!@[glInterleavedArrays], or
 *!@[glPixelStore] is not allowed after a call to @[glBegin] and before
 *!the corresponding call to @[glEnd],
 *!but an error may or may not be generated.
 *!
 *!
 */

/*!@decl void glEndList()
 *!
 *!Display lists are groups of GL commands that have been stored
 *!for subsequent execution.
 *!Display lists are created with @[glNewList].
 *!All subsequent commands are placed in the display list,
 *!in the order issued,
 *!until @[glEndList] is called. 
 *!
 *!@[glNewList] has two arguments.
 *!The first argument,
 *!@i{list@},
 *!is a positive integer that becomes the unique name for the display list.
 *!Names can be created and reserved with @[glGenLists]
 *!and tested for uniqueness with @[glIsList].
 *!The second argument,
 *!@i{mode@},
 *!is a symbolic constant that can assume one of two values: 
 *!@xml{<matrix>
 *!<r><c><ref>GL_COMPILE</ref>
 *!</c><c>Commands are merely compiled.
 *!</c></r>
 *!<r><c><ref>GL_COMPILE_AND_EXECUTE</ref>
 *!</c><c>Commands are executed as they are compiled into the display list.
 *!</c></r>
 *!</matrix>@}
 *!
 *!Certain commands are not compiled into the display list
 *!but are executed immediately,
 *!regardless of the display-list mode.
 *!These commands are
 *!@[glColorPointer],
 *!@[glDeleteLists],
 *!@[glDisableClientState],
 *!@[glEdgeFlagPointer],
 *!@[glEnableClientState],
 *!@[glFeedbackBuffer],
 *!@[glFinish],
 *!@[glFlush],
 *!@[glGenLists],
 *!@[glIndexPointer],
 *!@[glInterleavedArrays],
 *!@[glIsEnabled],
 *!@[glIsList],
 *!@[glNormalPointer],
 *!@[glPopClientAttrib],
 *!@[glPixelStore],
 *!@[glPushClientAttrib],
 *!@[glReadPixels],
 *!@[glRenderMode],
 *!@[glSelectBuffer],
 *!@[glTexCoordPointer],
 *!@[glVertexPointer],
 *!and all of the @[glGet] commands.
 *!
 *!Similarly, 
 *!@[glTexImage2D] and @[glTexImage1D] 
 *!are executed immediately and not compiled into the display list when their
 *!first argument is @[GL_PROXY_TEXTURE_2D] or
 *!@[GL_PROXY_TEXTURE_1D], respectively.
 *!
 *!When @[glEndList] is encountered,
 *!the display-list definition is completed by associating the list
 *!with the unique name @i{list@}
 *!(specified in the @[glNewList] command). 
 *!If a display list with name @i{list@} already exists,
 *!it is replaced only when @[glEndList] is called.
 *!
 *!@param list
 *!
 *!@i{list@}
 *!Specifies the display-list name.
 *!
 *!@param mode
 *!
 *!Specifies the compilation mode,
 *!which can be
 *!@[GL_COMPILE] or
 *!@[GL_COMPILE_AND_EXECUTE].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{list@} is 0.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glEndList] is called
 *!without a preceding @[glNewList], 
 *!or if @[glNewList] is called while a display list is being defined.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glNewList] or @[glEndList]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!@[GL_OUT_OF_MEMORY] is generated if there is insufficient memory to
 *!compile the display list. If the GL version is 1.1 or greater, no
 *!change is made to the previous contents of the display list, if any,
 *!and no other change is made to the GL state. (It is as if no attempt
 *!had been made to create the new display list.) 
 *!
 *!
 */

/*!@decl void glFinish()
 *!
 *!@[glFinish] does not return until the effects of all previously
 *!called GL commands are complete.
 *!Such effects include all changes to GL state,
 *!all changes to connection state,
 *!and all changes to the frame buffer contents.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glFinish] is executed between
 *!the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glFlush()
 *!
 *!Different GL implementations buffer commands in several different locations,
 *!including network buffers and the graphics accelerator itself.
 *!@[glFlush] empties all of these buffers,
 *!causing all issued commands to be executed as quickly as
 *!they are accepted by the actual rendering engine.
 *!Though this execution may not be completed in any particular
 *!time period,
 *!it does complete in finite time.
 *!
 *!Because any GL program might be executed over a network,
 *!or on an accelerator that buffers commands,
 *!all programs should call @[glFlush] whenever they count on having
 *!all of their previously issued commands completed.
 *!For example,
 *!call @[glFlush] before waiting for user input that depends on
 *!the generated image. 
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glFlush]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glInitNames()
 *!
 *!The name stack is used during selection mode to allow sets of rendering
 *!commands to be uniquely identified.
 *!It consists of an ordered set of unsigned integers.
 *!@[glInitNames] causes the name stack to be initialized to its default empty state.
 *!
 *!The name stack is always empty while the render mode is not @[GL_SELECT].
 *!Calls to @[glInitNames] while the render mode is not @[GL_SELECT] are ignored.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glInitNames]
 *!is executed between the execution of @[glBegin] and the corresponding execution of
 *!@[glEnd].
 *!
 *!
 */

/*!@decl void glLoadIdentity()
 *!
 *!@[glLoadIdentity] replaces the current matrix with the identity matrix.
 *!It is semantically equivalent to calling @[glLoadMatrix]
 *!with the identity matrix
 *!
 *!.ce
 *!
 *!.EQ
 *!left (  down 20 { ~ matrix {
 *!   ccol { 1 above 0 above 0 above 0~ }
 *!   ccol { 0 above 1 above 0 above 0~ }
 *!   ccol { 0 above 0 above 1 above 0~ }
 *!   ccol { 0 above 0 above 0 above 1 }
 *!} } ~~ right )
 *!.EN
 *!
 *!
 *!but in some cases it is more efficient.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLoadIdentity]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPopAttrib()
 *!
 *!@[glPushAttrib] takes one argument,
 *!a mask that indicates which groups of state variables
 *!to save on the attribute stack.
 *!Symbolic constants are used to set bits in the mask.
 *!@i{mask@}
 *!is typically constructed by ORing several of these constants together.
 *!The special mask
 *!@[GL_ALL_ATTRIB_BITS]
 *!can be used to save all stackable states.
 *!
 *!The symbolic mask constants and their associated GL state are as follows
 *!(the second column lists which attributes are saved):
 *!
 *!.TS
 *!;
 *!l l .
 *!@[GL_ACCUM_BUFFER_BIT]	Accumulation buffer clear value
 *!
 *!@[GL_COLOR_BUFFER_BIT]	@[GL_ALPHA_TEST] enable bit
 *!	Alpha test function and reference value
 *!	@[GL_BLEND] enable bit
 *!	Blending source and destination functions
 *!	Constant blend color
 *!	Blending equation
 *!	@[GL_DITHER] enable bit
 *!	@[GL_DRAW_BUFFER] setting
 *!	@[GL_COLOR_LOGIC_OP] enable bit
 *!	@[GL_INDEX_LOGIC_OP] enable bit
 *!	Logic op function
 *!	Color mode and index mode clear values
 *!	Color mode and index mode writemasks
 *!
 *!@[GL_CURRENT_BIT]	Current RGBA color
 *!	Current color index
 *!	Current normal vector
 *!	Current texture coordinates
 *!	Current raster position
 *!	@[GL_CURRENT_RASTER_POSITION_VALID] flag
 *!	RGBA color associated with current raster position
 *!	Color index associated with current raster position
 *!	Texture coordinates associated with current raster position
 *!	@[GL_EDGE_FLAG] flag
 *!
 *!@[GL_DEPTH_BUFFER_BIT]	@[GL_DEPTH_TEST] enable bit
 *!	Depth buffer test function
 *!	Depth buffer clear value
 *!	@[GL_DEPTH_WRITEMASK] enable bit
 *!
 *!@[GL_ENABLE_BIT]	@[GL_ALPHA_TEST] flag
 *!	@[GL_AUTO_NORMAL] flag
 *!	@[GL_BLEND] flag
 *!	Enable bits for the user-definable clipping planes
 *!	@[GL_COLOR_MATERIAL]
 *!	@[GL_CULL_FACE] flag
 *!	@[GL_DEPTH_TEST] flag
 *!	@[GL_DITHER] flag
 *!	@[GL_FOG] flag
 *!	@[GL_LIGHT]@i{i@} where 0\ <= @i{i@}<@[GL_MAX_LIGHTS]
 *!	@[GL_LIGHTING] flag
 *!	@[GL_LINE_SMOOTH] flag
 *!	@[GL_LINE_STIPPLE] flag
 *!	@[GL_COLOR_LOGIC_OP] flag
 *!	@[GL_INDEX_LOGIC_OP] flag
 *!	@[GL_MAP1_]@i{x@} where @i{x@} is a map type
 *!	@[GL_MAP2_]@i{x@} where @i{x@} is a map type
 *!	@[GL_NORMALIZE] flag
 *!	@[GL_POINT_SMOOTH] flag
 *!	@[GL_POLYGON_OFFSET_LINE] flag
 *!	@[GL_POLYGON_OFFSET_FILL] flag
 *!	@[GL_POLYGON_OFFSET_POINT] flag
 *!	@[GL_POLYGON_SMOOTH] flag
 *!	@[GL_POLYGON_STIPPLE] flag
 *!	@[GL_SCISSOR_TEST] flag
 *!	@[GL_STENCIL_TEST] flag
 *!	@[GL_TEXTURE_1D] flag
 *!	@[GL_TEXTURE_2D] flag
 *!	Flags @[GL_TEXTURE_GEN_]@i{x@} where @i{x@} is S, T, R, or Q 
 *!
 *!@[GL_EVAL_BIT]	@[GL_MAP1_]@i{x@} enable bits, where @i{x@} is a map type
 *!	@[GL_MAP2_]@i{x@} enable bits, where @i{x@} is a map type
 *!	1D grid endpoints and divisions
 *!	2D grid endpoints and divisions
 *!	@[GL_AUTO_NORMAL] enable bit
 *!
 *!@[GL_FOG_BIT]	@[GL_FOG] enable bit
 *!	Fog color
 *!	Fog density
 *!	Linear fog start
 *!	Linear fog end
 *!	Fog index
 *!	@[GL_FOG_MODE] value
 *!
 *!@[GL_HINT_BIT]	@[GL_PERSPECTIVE_CORRECTION_HINT] setting
 *!	@[GL_POINT_SMOOTH_HINT] setting
 *!	@[GL_LINE_SMOOTH_HINT] setting
 *!	@[GL_POLYGON_SMOOTH_HINT] setting
 *!	@[GL_FOG_HINT] setting
 *!
 *!@[GL_LIGHTING_BIT]	@[GL_COLOR_MATERIAL] enable bit
 *!	@[GL_COLOR_MATERIAL_FACE] value
 *!	Color material parameters that are tracking the current color
 *!	Ambient scene color
 *!	@[GL_LIGHT_MODEL_LOCAL_VIEWER] value
 *!	@[GL_LIGHT_MODEL_TWO_SIDE] setting
 *!	@[GL_LIGHTING] enable bit
 *!	Enable bit for each light
 *!	Ambient, diffuse, and specular intensity for each light
 *!	Direction, position, exponent, and cutoff angle for each light
 *!	Constant, linear, and quadratic attenuation factors for each light
 *!	Ambient, diffuse, specular, and emissive color for each material
 *!	Ambient, diffuse, and specular color indices for each material
 *!	Specular exponent for each material
 *!	@[GL_SHADE_MODEL] setting
 *!
 *!@[GL_LINE_BIT]	@[GL_LINE_SMOOTH] flag
 *!	@[GL_LINE_STIPPLE] enable bit
 *!	Line stipple pattern and repeat counter
 *!	Line width
 *!
 *!@[GL_LIST_BIT]	@[GL_LIST_BASE] setting
 *!
 *!@[GL_PIXEL_MODE_BIT]	@[GL_RED_BIAS] and @[GL_RED_SCALE] settings
 *!	@[GL_GREEN_BIAS] and @[GL_GREEN_SCALE] values
 *!	@[GL_BLUE_BIAS] and @[GL_BLUE_SCALE]
 *!	@[GL_ALPHA_BIAS] and @[GL_ALPHA_SCALE]
 *!	@[GL_DEPTH_BIAS] and @[GL_DEPTH_SCALE]
 *!	@[GL_INDEX_OFFSET] and @[GL_INDEX_SHIFT] values
 *!	@[GL_MAP_COLOR] and @[GL_MAP_STENCIL] flags
 *!	@[GL_ZOOM_X] and @[GL_ZOOM_Y] factors
 *!	@[GL_READ_BUFFER] setting
 *!
 *!@[GL_POINT_BIT]	@[GL_POINT_SMOOTH] flag
 *!	Point size
 *!
 *!@[GL_POLYGON_BIT]	@[GL_CULL_FACE] enable bit
 *!	@[GL_CULL_FACE_MODE] value
 *!	@[GL_FRONT_FACE] indicator
 *!	@[GL_POLYGON_MODE] setting
 *!	@[GL_POLYGON_SMOOTH] flag
 *!	@[GL_POLYGON_STIPPLE] enable bit
 *!	@[GL_POLYGON_OFFSET_FILL] flag
 *!	@[GL_POLYGON_OFFSET_LINE] flag
 *!	@[GL_POLYGON_OFFSET_POINT] flag
 *!	@[GL_POLYGON_OFFSET_FACTOR]
 *!	@[GL_POLYGON_OFFSET_UNITS]
 *!
 *!@[GL_POLYGON_STIPPLE_BIT]	Polygon stipple image
 *!
 *!@[GL_SCISSOR_BIT]	@[GL_SCISSOR_TEST] flag
 *!	Scissor box
 *!
 *!@[GL_STENCIL_BUFFER_BIT]	@[GL_STENCIL_TEST] enable bit
 *!	Stencil function and reference value
 *!	Stencil value mask
 *!	Stencil fail, pass, and depth buffer pass actions
 *!	Stencil buffer clear value
 *!	Stencil buffer writemask
 *!
 *!@[GL_TEXTURE_BIT]	Enable bits for the four texture coordinates
 *!	Border color for each texture image
 *!	Minification function for each texture image
 *!	Magnification function for each texture image
 *!	Texture coordinates and wrap mode for each texture image
 *!	Color and mode for each texture environment
 *!	Enable bits @[GL_TEXTURE_GEN_]@i{x@}, @i{x@} is S, T, R, and Q 
 *!	@[GL_TEXTURE_GEN_MODE] setting for S, T, R, and Q
 *!	@[glTexGen] plane equations for S, T, R, and Q
 *!	Current texture bindings (for example, @[GL_TEXTURE_2D_BINDING])
 *!
 *!@[GL_TRANSFORM_BIT]	Coefficients of the six clipping planes
 *!	Enable bits for the user-definable clipping planes
 *!	@[GL_MATRIX_MODE] value
 *!	@[GL_NORMALIZE] flag
 *!
 *!@[GL_VIEWPORT_BIT]	Depth range (near and far)
 *!	Viewport origin and extent
 *!.TE
 *!
 *!@[glPopAttrib] restores the values of the state variables saved with the last
 *!
 *!@[glPushAttrib] command.
 *!Those not saved are left unchanged.
 *!
 *!It is an error to push attributes onto a full stack,
 *!or to pop attributes off an empty stack.
 *!In either case, the error flag is set
 *!and no other change is made to GL state.
 *!
 *!Initially, the attribute stack is empty.
 *!
 *!@param mask
 *!
 *!@i{mask@}
 *!Specifies a mask that indicates which attributes to save. Values for
 *!@i{mask@} are listed below.
 *!
 *!@throws
 *!
 *!@[GL_STACK_OVERFLOW] is generated if @[glPushAttrib] is called while
 *!the attribute stack is full.
 *!
 *!@[GL_STACK_UNDERFLOW] is generated if @[glPopAttrib] is called while
 *!the attribute stack is empty.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPushAttrib] or @[glPopAttrib]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPopClientAttrib()
 *!
 *!@[glPushClientAttrib] takes one argument,
 *!a mask that indicates which groups of client-state variables
 *!to save on the client attribute stack. 
 *!Symbolic constants are used to set bits in the mask.
 *!@i{mask@}
 *!is typically constructed by OR'ing several of these constants together.
 *!The special mask
 *!@[GL_CLIENT_ALL_ATTRIB_BITS]
 *!can be used to save all stackable client state.
 *!
 *!The symbolic mask constants and their associated GL client state are as follows
 *!(the second column lists which attributes are saved):
 *!
 *!@[GL_CLIENT_PIXEL_STORE_BIT]	Pixel storage modes
 *!
 *!@[GL_CLIENT_VERTEX_ARRAY_BIT]	Vertex arrays (and enables)
 *!
 *!@[glPopClientAttrib] restores the values of the client-state variables 
 *!saved with the last @[glPushClientAttrib].
 *!Those not saved are left unchanged.
 *!
 *!It is an error to push attributes onto a full client attribute stack,
 *!or to pop attributes off an empty stack.
 *!In either case, the error flag is set,
 *!and no other change is made to GL state.
 *!
 *!Initially, the client attribute stack is empty.
 *!
 *!@param mask
 *!
 *!@i{mask@}
 *!Specifies a mask that indicates which attributes to save.  Values for
 *!@i{mask@} are listed below.
 *!
 *!@throws
 *!
 *!@[GL_STACK_OVERFLOW] is generated if @[glPushClientAttrib] is called while
 *!the attribute stack is full.
 *!
 *!@[GL_STACK_UNDERFLOW] is generated if @[glPopClientAttrib] is called while
 *!the attribute stack is empty.
 *!
 *!
 */

/*!@decl void glPopMatrix()
 *!
 *!There is a stack of matrices for each of the matrix modes.
 *!In @[GL_MODELVIEW] mode,
 *!the stack depth is at least 32.
 *!In the other two modes,
 *!@[GL_PROJECTION] and @[GL_TEXTURE],
 *!the depth is at least 2.
 *!The current matrix in any mode is the matrix on the top of the stack
 *!for that mode.
 *!
 *!@[glPushMatrix] pushes the current matrix stack down by one,
 *!duplicating the current matrix.
 *!That is,
 *!after a @[glPushMatrix] call,
 *!the matrix on top of the stack is identical to the one below it.
 *!
 *!@[glPopMatrix] pops the current matrix stack,
 *!replacing the current matrix with the one below it on the stack. 
 *!
 *!Initially, each of the stacks contains one matrix, an identity matrix.
 *!
 *!It is an error to push a full matrix stack,
 *!or to pop a matrix stack that contains only a single matrix.
 *!In either case, the error flag is set
 *!and no other change is made to GL state.
 *!
 *!@throws
 *!
 *!@[GL_STACK_OVERFLOW] is generated if @[glPushMatrix] is called while
 *!the current matrix stack is full.
 *!
 *!@[GL_STACK_UNDERFLOW] is generated if @[glPopMatrix] is called while
 *!the current matrix stack contains only a single matrix.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPushMatrix] or @[glPopMatrix]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPopName()
 *!
 *!The name stack is used during selection mode to allow sets of rendering
 *!commands to be uniquely identified.
 *!It consists of an ordered set of unsigned integers and is initially empty.
 *!
 *!@[glPushName] causes @i{name@} to be pushed onto the name stack.
 *!@[glPopName] pops one name off the top of the stack. 
 *!
 *!The maximum name stack depth is implementation-dependent; call
 *!@[GL_MAX_NAME_STACK_DEPTH] to find out the value for a particular
 *!implementation. It is an
 *!error to push a name onto a full stack, 
 *!or to pop a name off an empty stack.
 *!It is also an error to manipulate the name stack between the execution of
 *!@[glBegin] and the corresponding execution of @[glEnd].
 *!In any of these cases, the error flag is set and no other change is
 *!made to GL state.
 *!
 *!The name stack is always empty while the render mode is not @[GL_SELECT].
 *!Calls to @[glPushName] or @[glPopName] while the render mode is not
 *!@[GL_SELECT] are ignored.
 *!
 *!@param name
 *!
 *!@i{name@}
 *!Specifies a name that will be pushed onto the name stack.
 *!
 *!@throws
 *!
 *!@[GL_STACK_OVERFLOW] is generated if @[glPushName] is called while the
 *!name stack is full.
 *!
 *!@[GL_STACK_UNDERFLOW] is generated if @[glPopName] is called while the
 *!name stack is empty.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPushName] or @[glPopName]
 *!is executed between a call to @[glBegin] and the corresponding call to
 *!@[glEnd].
 *!
 *!
 */

/*!@decl void glPushMatrix()
 *!
 *!There is a stack of matrices for each of the matrix modes.
 *!In @[GL_MODELVIEW] mode,
 *!the stack depth is at least 32.
 *!In the other two modes,
 *!@[GL_PROJECTION] and @[GL_TEXTURE],
 *!the depth is at least 2.
 *!The current matrix in any mode is the matrix on the top of the stack
 *!for that mode.
 *!
 *!@[glPushMatrix] pushes the current matrix stack down by one,
 *!duplicating the current matrix.
 *!That is,
 *!after a @[glPushMatrix] call,
 *!the matrix on top of the stack is identical to the one below it.
 *!
 *!@[glPopMatrix] pops the current matrix stack,
 *!replacing the current matrix with the one below it on the stack. 
 *!
 *!Initially, each of the stacks contains one matrix, an identity matrix.
 *!
 *!It is an error to push a full matrix stack,
 *!or to pop a matrix stack that contains only a single matrix.
 *!In either case, the error flag is set
 *!and no other change is made to GL state.
 *!
 *!@throws
 *!
 *!@[GL_STACK_OVERFLOW] is generated if @[glPushMatrix] is called while
 *!the current matrix stack is full.
 *!
 *!@[GL_STACK_UNDERFLOW] is generated if @[glPopMatrix] is called while
 *!the current matrix stack contains only a single matrix.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPushMatrix] or @[glPopMatrix]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glBegin(int mode)
 *!
 *!@[glBegin] and @[glEnd] delimit the vertices that define a primitive or
 *!a group of like primitives.
 *!@[glBegin] accepts a single argument that specifies in which of ten ways the
 *!vertices are interpreted.
 *!Taking @i{n@} as an integer count starting at one,
 *!and @i{N@} as the total number of vertices specified,
 *!the interpretations are as follows:
 *!@xml{<matrix>
 *!<r><c><ref>GL_POINTS</ref>
 *!</c><c>Treats each vertex as a single point.
 *!Vertex <i>n</i> defines point <i>n</i>.
 *!<i>N</i> points are drawn.
 *!</c></r>
 *!<r><c><ref>GL_LINES</ref>
 *!</c><c>Treats each pair of vertices as an independent line segment.
 *!Vertices <i>2n-1</i> and <i>2n</i> define line <i>n</i>.
 *!<i>N/2</i> lines are drawn.
 *!</c></r>
 *!<r><c><ref>GL_LINE_STRIP</ref>
 *!</c><c>Draws a connected group of line segments from the first vertex
 *!to the last.
 *!Vertices <i>n</i> and <i>n+1</i> define line <i>n</i>.
 *!<i>N-1</i> lines are drawn.
 *!</c></r>
 *!<r><c><ref>GL_LINE_LOOP</ref>
 *!</c><c>Draws a connected group of line segments from the first vertex
 *!to the last,
 *!then back to the first.
 *!Vertices <i>n</i> and <i>n+1</i> define line <i>n</i>.
 *!The last line, however, is defined by vertices <i>N</i> and <i>1</i>.
 *!<i>N</i> lines are drawn.
 *!</c></r>
 *!<r><c><ref>GL_TRIANGLES</ref>
 *!</c><c>Treats each triplet of vertices as an independent triangle.
 *!Vertices <i>3n-2</i>,
 *!<i>3n-1</i>,
 *!and <i>3n</i> define triangle <i>n</i>.
 *!<i>N/3</i> triangles are drawn.
 *!.BP
 *!</c></r>
 *!<r><c><ref>GL_TRIANGLE_STRIP</ref>
 *!</c><c>Draws a connected group of triangles.
 *!One triangle is defined for each vertex presented after the first two vertices.
 *!For odd <i>n</i>, vertices <i>n</i>,
 *!<i>n+1</i>,
 *!and <i>n+2</i> define triangle <i>n</i>.
 *!For even <i>n</i>,
 *!vertices <i>n+1</i>,
 *!<i>n</i>,
 *!and <i>n+2</i> define triangle <i>n</i>.
 *!<i>N-2</i> triangles are drawn.
 *!</c></r>
 *!<r><c><ref>GL_TRIANGLE_FAN</ref>
 *!</c><c>Draws a connected group of triangles.
 *!One triangle is defined for each vertex presented after the first two vertices.
 *!Vertices <i>1</i>,
 *!<i>n+1</i>,
 *!and <i>n+2</i> define triangle <i>n</i>.
 *!<i>N-2</i> triangles are drawn.
 *!</c></r>
 *!<r><c><ref>GL_QUADS</ref>
 *!</c><c>Treats each group of four vertices as an independent quadrilateral.
 *!Vertices <i>4n-3</i>,
 *!<i>4n-2</i>,
 *!<i>4n-1</i>,
 *!and <i>4n</i> define quadrilateral <i>n</i>.
 *!<i>N/4</i> quadrilaterals are drawn.
 *!</c></r>
 *!<r><c><ref>GL_QUAD_STRIP</ref>
 *!</c><c>Draws a connected group of quadrilaterals.
 *!One quadrilateral is defined for each pair of vertices presented
 *!after the first pair.
 *!Vertices <i>2n-1</i>,
 *!<i>2n</i>,
 *!<i>2n+2</i>,
 *!and <i>2n+1</i> define quadrilateral <i>n</i>.
 *!<i>N/2-1</i> quadrilaterals are drawn.
 *!Note that the order in which vertices are used to construct a quadrilateral
 *!from strip data is different from that used with independent data.
 *!</c></r>
 *!<r><c><ref>GL_POLYGON</ref>
 *!</c><c>Draws a single,
 *!convex polygon.
 *!Vertices <i>1</i> through <i>N</i> define this polygon.
 *!</c></r>
 *!</matrix>@}
 *!
 *!Only a subset of GL commands can be used between @[glBegin] and @[glEnd].
 *!The commands are
 *!@[glVertex],
 *!@[glColor],
 *!@[glIndex],
 *!@[glNormal],
 *!@[glTexCoord],
 *!@[glEvalCoord],
 *!@[glEvalPoint],
 *!@[glArrayElement],
 *!@[glMaterial], and
 *!@[glEdgeFlag].
 *!Also,
 *!it is acceptable to use
 *!@[glCallList] or
 *!@[glCallLists] to execute
 *!display lists that include only the preceding commands.
 *!If any other GL command is executed between @[glBegin] and @[glEnd],
 *!the error flag is set and the command is ignored.
 *!
 *!Regardless of the value chosen for @i{mode@},
 *!there is no limit to the number of vertices that can be defined
 *!between @[glBegin] and @[glEnd].
 *!Lines,
 *!triangles,
 *!quadrilaterals,
 *!and polygons that are incompletely specified are not drawn.
 *!Incomplete specification results when either too few vertices are
 *!provided to specify even a single primitive or when an incorrect multiple 
 *!of vertices is specified. The incomplete primitive is ignored; the rest are drawn.
 *!
 *!The minimum specification of vertices
 *!for each primitive is as follows:
 *!1 for a point,
 *!2 for a line,
 *!3 for a triangle,
 *!4 for a quadrilateral,
 *!and 3 for a polygon.
 *!Modes that require a certain multiple of vertices are
 *!@[GL_LINES] (2),
 *!@[GL_TRIANGLES] (3),
 *!@[GL_QUADS] (4),
 *!and @[GL_QUAD_STRIP] (2).
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies the primitive or primitives that will be created from vertices
 *!presented between @[glBegin] and the subsequent @[glEnd].
 *!Ten symbolic constants are accepted:
 *!@[GL_POINTS],
 *!@[GL_LINES],
 *!@[GL_LINE_STRIP],
 *!@[GL_LINE_LOOP],
 *!@[GL_TRIANGLES],
 *!@[GL_TRIANGLE_STRIP],
 *!@[GL_TRIANGLE_FAN],
 *!@[GL_QUADS],
 *!@[GL_QUAD_STRIP], and
 *!@[GL_POLYGON].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is set to an unaccepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glBegin] is executed between a 
 *!@[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glEnd] is executed without being
 *!preceded by a @[glBegin].
 *!
 *!@[GL_INVALID_OPERATION] is generated if a command other than
 *!@[glVertex],
 *!@[glColor],
 *!@[glIndex],
 *!@[glNormal],
 *!@[glTexCoord],
 *!@[glEvalCoord],
 *!@[glEvalPoint],
 *!@[glArrayElement],
 *!@[glMaterial],
 *!@[glEdgeFlag],
 *!@[glCallList], or
 *!@[glCallLists] is executed between
 *!the execution of @[glBegin] and the corresponding
 *!execution @[glEnd].
 *!
 *!Execution of 
 *!@[glEnableClientState],
 *!@[glDisableClientState],
 *!@[glEdgeFlagPointer],
 *!@[glTexCoordPointer],
 *!@[glColorPointer],
 *!@[glIndexPointer],
 *!@[glNormalPointer],
 *!
 *!@[glVertexPointer],
 *!@[glInterleavedArrays], or
 *!@[glPixelStore] is not allowed after a call to @[glBegin] and before
 *!the corresponding call to @[glEnd],
 *!but an error may or may not be generated.
 *!
 *!
 */

/*!@decl void glCullFace(int mode)
 *!
 *!@[glCullFace] specifies whether front- or back-facing facets are culled
 *!(as specified by @i{mode@}) when facet culling is enabled. Facet
 *!culling is initially disabled.
 *!To enable and disable facet culling, call the
 *!@[glEnable] and @[glDisable] commands
 *!with the argument @[GL_CULL_FACE].
 *!Facets include triangles,
 *!quadrilaterals,
 *!polygons,
 *!and rectangles.
 *!
 *!@[glFrontFace] specifies which of the clockwise and counterclockwise facets
 *!are front-facing and back-facing.
 *!See @[glFrontFace].
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies whether front- or back-facing facets are candidates for culling.
 *!Symbolic constants
 *!@[GL_FRONT], @[GL_BACK], and @[GL_FRONT_AND_BACK] are accepted.
 *!The initial value is @[GL_BACK].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glCullFace]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glDepthFunc(int func)
 *!
 *!@[glDepthFunc] specifies the function used to compare each incoming pixel depth value
 *!with the depth value present in the depth buffer.
 *!The comparison is performed only if depth testing is enabled.
 *!(See @[glEnable] and @[glDisable] of @[GL_DEPTH_TEST].)
 *!
 *!@i{func@} specifies the conditions under which the pixel will be drawn.
 *!The comparison functions are as follows:
 *!@xml{<matrix>
 *!<r><c><ref>GL_NEVER</ref>
 *!</c><c>Never passes. 
 *!</c></r>
 *!<r><c><ref>GL_LESS</ref>
 *!</c><c>Passes if the incoming depth value is less than the stored depth value.
 *!</c></r>
 *!<r><c><ref>GL_EQUAL</ref>
 *!</c><c>Passes if the incoming depth value is equal to the stored depth value.
 *!</c></r>
 *!<r><c><ref>GL_LEQUAL</ref>
 *!</c><c>Passes if the incoming depth value is less than or equal to
 *!the stored depth value.
 *!</c></r>
 *!<r><c><ref>GL_GREATER</ref> 
 *!</c><c>Passes if the incoming depth value is greater than the stored depth value.
 *!</c></r>
 *!<r><c><ref>GL_NOTEQUAL</ref>
 *!</c><c>Passes if the incoming depth value is not equal to the stored depth value.
 *!</c></r>
 *!<r><c><ref>GL_GEQUAL</ref>
 *!</c><c>Passes if the incoming depth value is greater than or equal to
 *!the stored depth value.
 *!</c></r>
 *!<r><c><ref>GL_ALWAYS</ref>
 *!</c><c>Always passes. 
 *!</c></r>
 *!</matrix>@}
 *!
 *!The initial value of @i{func@} is @[GL_LESS].
 *!Initially, depth testing is disabled.
 *!.NOTES
 *!Even if the depth buffer exists and the depth mask is non-zero, the
 *!depth buffer is not updated if the depth test is disabled. 
 *!
 *!@param func
 *!
 *!@i{func@}
 *!Specifies the depth comparison function.
 *!Symbolic constants
 *!@[GL_NEVER],
 *!@[GL_LESS],
 *!@[GL_EQUAL],
 *!@[GL_LEQUAL],
 *!@[GL_GREATER],
 *!@[GL_NOTEQUAL],
 *!@[GL_GEQUAL], and
 *!@[GL_ALWAYS] are accepted.
 *!The initial value is @[GL_LESS].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{func@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glDepthFunc]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glDisable(int cap)
 *!
 *!@[glEnable] and @[glDisable] enable and disable various capabilities.
 *!Use @[glIsEnabled] or @[glGet] to determine the current setting
 *!of any capability. The initial value for each capability with the
 *!exception of @[GL_DITHER] is @[GL_FALSE]. The initial value for
 *!@[GL_DITHER] is @[GL_TRUE]. 
 *!
 *!Both @[glEnable] and @[glDisable] take a single argument, @i{cap@},
 *!which can assume one of the following values:
 *!@xml{<matrix>
 *!<r><c><ref>GL_ALPHA_TEST</ref>
 *!</c><c>If enabled,
 *!do alpha testing. See
 *!<ref>glAlphaFunc</ref>.
 *!</c></r>
 *!<r><c><ref>GL_AUTO_NORMAL</ref>
 *!</c><c>If enabled,
 *!generate normal vectors when either
 *!<ref>GL_MAP2_VERTEX_3</ref> or
 *!<ref>GL_MAP2_VERTEX_4</ref> is used to generate vertices. 
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_BLEND</ref>
 *!</c><c>If enabled,
 *!blend the incoming RGBA color values with the values in the color
 *!buffers. See <ref>glBlendFunc</ref>.
 *!</c></r>
 *!<r><c><ref>GL_CLIP_PLANE</ref><i>i</i>
 *!</c><c>If enabled,
 *!clip geometry against user-defined clipping plane <i>i</i>.
 *!See <ref>glClipPlane</ref>.
 *!</c></r>
 *!<r><c><ref>GL_COLOR_LOGIC_OP</ref>
 *!</c><c>If enabled,
 *!apply the currently selected logical operation to the incoming RGBA
 *!color and color buffer values. See <ref>glLogicOp</ref>.
 *!</c></r>
 *!<r><c><ref>GL_COLOR_MATERIAL</ref>
 *!</c><c>If enabled,
 *!have one or more material parameters track the current color.
 *!See <ref>glColorMaterial</ref>.
 *!</c></r>
 *!<r><c><ref>GL_CULL_FACE</ref>
 *!</c><c>If enabled,
 *!cull polygons based on their winding in window coordinates. 
 *!See <ref>glCullFace</ref>.
 *!</c></r>
 *!<r><c><ref>GL_DEPTH_TEST</ref>
 *!</c><c>If enabled,
 *!do depth comparisons and update the depth buffer. Note that even if
 *!the depth buffer exists and the depth mask is non-zero, the 
 *!depth buffer is not updated if the depth test is disabled. See
 *!<ref>glDepthFunc</ref> and 
 *!
 *!<ref>glDepthRange</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_DITHER</ref>	
 *!</c><c>If enabled,
 *!dither color components or indices before they are written to the
 *!color buffer.
 *!</c></r>
 *!<r><c><ref>GL_FOG</ref>
 *!</c><c>If enabled,
 *!blend a fog color into the posttexturing color.
 *!See <ref>glFog</ref>.
 *!</c></r>
 *!<r><c><ref>GL_INDEX_LOGIC_OP</ref>
 *!</c><c>If enabled,
 *!apply the currently selected logical operation to the incoming index and color
 *!buffer indices. See 
 *!
 *!<ref>glLogicOp</ref>.
 *!</c></r>
 *!<r><c><ref>GL_LIGHT</ref><i>i</i>
 *!</c><c>If enabled,
 *!include light <i>i</i> in the evaluation of the lighting
 *!equation. See <ref>glLightModel</ref> and <ref>glLight</ref>.
 *!</c></r>
 *!<r><c><ref>GL_LIGHTING</ref>
 *!</c><c>If enabled,
 *!use the current lighting parameters to compute the vertex color or index.
 *!Otherwise, simply associate the current color or index with each
 *!vertex. See 
 *!
 *!<ref>glMaterial</ref>, <ref>glLightModel</ref>, and <ref>glLight</ref>.
 *!</c></r>
 *!<r><c><ref>GL_LINE_SMOOTH</ref>
 *!</c><c>If enabled,
 *!draw lines with correct filtering.
 *!Otherwise,
 *!draw aliased lines.
 *!See <ref>glLineWidth</ref>.
 *!</c></r>
 *!<r><c><ref>GL_LINE_STIPPLE</ref>
 *!</c><c>If enabled,
 *!use the current line stipple pattern when drawing lines. See
 *!<ref>glLineStipple</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_MAP1_COLOR_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate RGBA values.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_INDEX</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate color indices.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_NORMAL</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate normals.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_1</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>s</i>
 *!texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_2</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>s</i> and
 *!<i>t</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>s</i>,
 *!<i>t</i>, and
 *!<i>r</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>s</i>,
 *!<i>t</i>,
 *!<i>r</i>, and
 *!<i>q</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_VERTEX_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>x</i>, <i>y</i>, and <i>z</i> vertex coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_VERTEX_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!homogeneous
 *!<i>x</i>,
 *!<i>y</i>,
 *!<i>z</i>, and
 *!<i>w</i> vertex coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_COLOR_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate RGBA values.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_INDEX</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate color indices.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_NORMAL</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate normals.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_1</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>s</i>
 *!texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_2</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>s</i> and
 *!<i>t</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>s</i>,
 *!<i>t</i>, and
 *!<i>r</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>s</i>,
 *!<i>t</i>,
 *!<i>r</i>, and
 *!<i>q</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_VERTEX_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>x</i>, <i>y</i>, and <i>z</i> vertex coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_VERTEX_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!homogeneous
 *!<i>x</i>,
 *!<i>y</i>,
 *!<i>z</i>, and
 *!<i>w</i> vertex coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_NORMALIZE</ref>
 *!</c><c>If enabled,
 *!normal vectors specified with <ref>glNormal</ref> are scaled to unit length
 *!after transformation. See <ref>glNormal</ref>.
 *!</c></r>
 *!<r><c><ref>GL_POINT_SMOOTH</ref>
 *!</c><c>If enabled,
 *!draw points with proper filtering.
 *!Otherwise,
 *!draw aliased points.
 *!See <ref>glPointSize</ref>.
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_OFFSET_FILL</ref>
 *!</c><c>If enabled, and if the polygon is rendered in
 *!<ref>GL_FILL</ref> mode, an offset is added to depth values of a polygon's
 *!fragments before the depth comparison is performed. See
 *!<ref>glPolygonOffset</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_OFFSET_LINE</ref>
 *!</c><c>If enabled, and if the polygon is rendered in
 *!<ref>GL_LINE</ref> mode, an offset is added to depth values of a polygon's
 *!fragments before the depth comparison is performed. See <ref>glPolygonOffset</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_OFFSET_POINT</ref>
 *!</c><c>If enabled, an offset is added to depth values of a polygon's fragments
 *!before the depth comparison is performed, if the polygon is rendered in 
 *!<ref>GL_POINT</ref> mode. See 
 *!
 *!<ref>glPolygonOffset</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_SMOOTH</ref>
 *!</c><c>If enabled, draw polygons with proper filtering.
 *!Otherwise, draw aliased polygons. For correct anti-aliased polygons,
 *!an alpha buffer is needed and the polygons must be sorted front to
 *!back. 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_STIPPLE</ref>
 *!</c><c>If enabled,
 *!use the current polygon stipple pattern when rendering
 *!polygons. See <ref>glPolygonStipple</ref>.
 *!</c></r>
 *!<r><c><ref>GL_SCISSOR_TEST</ref>
 *!</c><c>If enabled,
 *!discard fragments that are outside the scissor rectangle. 
 *!See <ref>glScissor</ref>.
 *!</c></r>
 *!<r><c><ref>GL_STENCIL_TEST</ref>
 *!</c><c>If enabled,
 *!do stencil testing and update the stencil buffer. 
 *!See <ref>glStencilFunc</ref> and <ref>glStencilOp</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_1D</ref>
 *!</c><c>If enabled,
 *!one-dimensional texturing is performed
 *!(unless two-dimensional texturing is also enabled). See <ref>glTexImage1D</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_2D</ref>
 *!</c><c>If enabled,
 *!two-dimensional texturing is performed. See <ref>glTexImage2D</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_GEN_Q</ref>
 *!</c><c>If enabled,
 *!the <i>q</i> texture coordinate is computed using
 *!the texture generation function defined with <ref>glTexGen</ref>.
 *!Otherwise, the current <i>q</i> texture coordinate is used.
 *!See <ref>glTexGen</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_GEN_R</ref>
 *!</c><c>If enabled,
 *!the <i>r</i> texture coordinate is computed using
 *!the texture generation function defined with <ref>glTexGen</ref>.
 *!Otherwise, the current <i>r</i> texture coordinate is used.
 *!See <ref>glTexGen</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_GEN_S</ref>
 *!</c><c>If enabled,
 *!the <i>s</i> texture coordinate is computed using
 *!the texture generation function defined with <ref>glTexGen</ref>.
 *!Otherwise, the current <i>s</i> texture coordinate is used. 
 *!See <ref>glTexGen</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_GEN_T</ref>
 *!</c><c>If enabled,
 *!the <i>t</i> texture coordinate is computed using
 *!the texture generation function defined with <ref>glTexGen</ref>.
 *!Otherwise, the current <i>t</i> texture coordinate is used.
 *!See <ref>glTexGen</ref>.
 *!</c></r></matrix>@}
 *!
 *!
 *!@param cap
 *!
 *!@i{cap@}
 *!Specifies a symbolic constant indicating a GL capability.
 *!
 *!
 *!@param cap
 *!
 *!Specifies a symbolic constant indicating a GL capability.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{cap@} is not one of the values
 *!listed previously.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glEnable] or @[glDisable]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glDisableClientState(int cap)
 *!
 *!@[glEnableClientState] and @[glDisableClientState]
 *!enable or disable individual client-side capabilities. By default, all
 *!client-side capabilities are disabled.
 *!Both 
 *!@[glEnableClientState] and @[glDisableClientState] take a
 *!single argument, @i{cap@}, which can assume one of the following
 *!values: 
 *!@xml{<matrix>
 *!<r><c><ref>GL_COLOR_ARRAY</ref>
 *!</c><c>If enabled, the color array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or 
 *!<ref>glDrawElement</ref> is called. See
 *!<ref>glColorPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_EDGE_FLAG_ARRAY</ref>
 *!</c><c>If enabled, the edge flag array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or <ref>glDrawElements</ref> is called. See
 *!<ref>glEdgeFlagPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_INDEX_ARRAY</ref>
 *!</c><c>If enabled, the index array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or 
 *!<ref>glDrawElements</ref> is called. See
 *!<ref>glIndexPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_NORMAL_ARRAY</ref>
 *!</c><c>If enabled, the normal array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or <ref>glDrawElements</ref> is called. See
 *!<ref>glNormalPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_COORD_ARRAY</ref>
 *!</c><c>If enabled, the texture coordinate array is enabled for writing and
 *!used for rendering when <ref>glDrawArrays</ref> or <ref>glDrawElements</ref> is
 *!called. See <ref>glTexCoordPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_VERTEX_ARRAY</ref>
 *!</c><c>If enabled, the vertex array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or
 *!<ref>glDrawElements</ref> is called. See
 *!<ref>glVertexPointer</ref>. 
 *!</c></r></matrix>@}
 *!
 *!
 *!@param cap
 *!
 *!@i{cap@}
 *!Specifies the capability to enable.
 *!Symbolic constants
 *!@[GL_COLOR_ARRAY],
 *!@[GL_EDGE_FLAG_ARRAY],
 *!@[GL_INDEX_ARRAY],
 *!@[GL_NORMAL_ARRAY],
 *!@[GL_TEXTURE_COORD_ARRAY], and
 *!@[GL_VERTEX_ARRAY]
 *!are accepted. 
 *!
 *!
 *!@param cap
 *!
 *!Specifies the capability to disable.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{cap@} is not an accepted value.
 *!
 *!@[glEnableClientState] is not allowed between the execution of @[glBegin] and the
 *!corresponding @[glEnd], but an error may or may not be generated. If
 *!no error is generated, the behavior is undefined.
 *!
 *!
 */

/*!@decl void glDrawBuffer(int mode)
 *!
 *!When colors are written to the frame buffer,
 *!they are written into the color buffers specified by @[glDrawBuffer].
 *!The specifications are as follows:
 *!@xml{<matrix>
 *!<r><c><ref>GL_NONE</ref>
 *!</c><c>No color buffers are written.
 *!</c></r>
 *!<r><c><ref>GL_FRONT_LEFT</ref>
 *!</c><c>Only the front left color buffer is written.
 *!</c></r>
 *!<r><c><ref>GL_FRONT_RIGHT</ref>
 *!</c><c>Only the front right color buffer is written.
 *!</c></r>
 *!<r><c><ref>GL_BACK_LEFT</ref>
 *!</c><c>Only the back left color buffer is written.
 *!</c></r>
 *!<r><c><ref>GL_BACK_RIGHT</ref>
 *!</c><c>Only the back right color buffer is written.
 *!</c></r>
 *!<r><c><ref>GL_FRONT</ref>
 *!</c><c>Only the front left and front right color buffers are written.
 *!If there is no front right color buffer,
 *!only the front left color buffer is written.
 *!</c></r>
 *!<r><c><ref>GL_BACK</ref>
 *!</c><c>Only the back left and back right color buffers are written.
 *!If there is no back right color buffer,
 *!only the back left color buffer is written.
 *!</c></r>
 *!<r><c><ref>GL_LEFT</ref>
 *!</c><c>Only the front left and back left color buffers are written.
 *!If there is no back left color buffer,
 *!only the front left color buffer is written.
 *!</c></r>
 *!<r><c><ref>GL_RIGHT</ref>
 *!</c><c>Only the front right and back right color buffers are written.
 *!If there is no back right color buffer,
 *!only the front right color buffer is written.
 *!.BP
 *!</c></r>
 *!<r><c><ref>GL_FRONT_AND_BACK</ref>
 *!</c><c>All the front and back color buffers
 *!(front left, front right, back left, back right)
 *!are written.
 *!If there are no back color buffers,
 *!only the front left and front right color buffers are written.
 *!If there are no right color buffers,
 *!only the front left and back left color buffers are written.
 *!If there are no right or back color buffers,
 *!only the front left color buffer is written.
 *!</c></r>
 *!<r><c><ref>GL_AUX</ref><i>i</i>
 *!</c><c>Only auxiliary color buffer <i>i</i> is written.
 *!</c></r>
 *!</matrix>@}
 *!
 *!If more than one color buffer is selected for drawing,
 *!then blending or logical operations are computed and applied independently
 *!for each color buffer and can produce different results in each buffer.
 *!
 *!Monoscopic contexts include only 
 *!.I left 
 *!buffers, and stereoscopic contexts include both
 *!.I left
 *!and
 *!.I right
 *!buffers.
 *!Likewise, single-buffered contexts include only
 *!.I front
 *!buffers, and double-buffered contexts include both 
 *!.I front
 *!and
 *!.I back
 *!buffers.
 *!The context is selected at GL initialization.
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies up to four color buffers to be drawn into.
 *!Symbolic constants
 *!@[GL_NONE], 
 *!@[GL_FRONT_LEFT],
 *!@[GL_FRONT_RIGHT],
 *!@[GL_BACK_LEFT],
 *!@[GL_BACK_RIGHT],
 *!@[GL_FRONT],
 *!@[GL_BACK],
 *!@[GL_LEFT],
 *!@[GL_RIGHT],
 *!@[GL_FRONT_AND_BACK], and
 *!@[GL_AUX]@i{i@},
 *!where @i{i@} is between 0 and ``@[GL_AUX_BUFFERS]'' -1,
 *!are accepted (@[GL_AUX_BUFFERS] is not the upper limit; use @[glGet]
 *!to query the number of available aux buffers.)
 *!The initial value is @[GL_FRONT] for single-buffered contexts,
 *!and @[GL_BACK] for double-buffered contexts.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if none of the buffers indicated
 *!by @i{mode@} exists.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glDrawBuffer]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glEnable(int cap)
 *!
 *!@[glEnable] and @[glDisable] enable and disable various capabilities.
 *!Use @[glIsEnabled] or @[glGet] to determine the current setting
 *!of any capability. The initial value for each capability with the
 *!exception of @[GL_DITHER] is @[GL_FALSE]. The initial value for
 *!@[GL_DITHER] is @[GL_TRUE]. 
 *!
 *!Both @[glEnable] and @[glDisable] take a single argument, @i{cap@},
 *!which can assume one of the following values:
 *!@xml{<matrix>
 *!<r><c><ref>GL_ALPHA_TEST</ref>
 *!</c><c>If enabled,
 *!do alpha testing. See
 *!<ref>glAlphaFunc</ref>.
 *!</c></r>
 *!<r><c><ref>GL_AUTO_NORMAL</ref>
 *!</c><c>If enabled,
 *!generate normal vectors when either
 *!<ref>GL_MAP2_VERTEX_3</ref> or
 *!<ref>GL_MAP2_VERTEX_4</ref> is used to generate vertices. 
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_BLEND</ref>
 *!</c><c>If enabled,
 *!blend the incoming RGBA color values with the values in the color
 *!buffers. See <ref>glBlendFunc</ref>.
 *!</c></r>
 *!<r><c><ref>GL_CLIP_PLANE</ref><i>i</i>
 *!</c><c>If enabled,
 *!clip geometry against user-defined clipping plane <i>i</i>.
 *!See <ref>glClipPlane</ref>.
 *!</c></r>
 *!<r><c><ref>GL_COLOR_LOGIC_OP</ref>
 *!</c><c>If enabled,
 *!apply the currently selected logical operation to the incoming RGBA
 *!color and color buffer values. See <ref>glLogicOp</ref>.
 *!</c></r>
 *!<r><c><ref>GL_COLOR_MATERIAL</ref>
 *!</c><c>If enabled,
 *!have one or more material parameters track the current color.
 *!See <ref>glColorMaterial</ref>.
 *!</c></r>
 *!<r><c><ref>GL_CULL_FACE</ref>
 *!</c><c>If enabled,
 *!cull polygons based on their winding in window coordinates. 
 *!See <ref>glCullFace</ref>.
 *!</c></r>
 *!<r><c><ref>GL_DEPTH_TEST</ref>
 *!</c><c>If enabled,
 *!do depth comparisons and update the depth buffer. Note that even if
 *!the depth buffer exists and the depth mask is non-zero, the 
 *!depth buffer is not updated if the depth test is disabled. See
 *!<ref>glDepthFunc</ref> and 
 *!
 *!<ref>glDepthRange</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_DITHER</ref>	
 *!</c><c>If enabled,
 *!dither color components or indices before they are written to the
 *!color buffer.
 *!</c></r>
 *!<r><c><ref>GL_FOG</ref>
 *!</c><c>If enabled,
 *!blend a fog color into the posttexturing color.
 *!See <ref>glFog</ref>.
 *!</c></r>
 *!<r><c><ref>GL_INDEX_LOGIC_OP</ref>
 *!</c><c>If enabled,
 *!apply the currently selected logical operation to the incoming index and color
 *!buffer indices. See 
 *!
 *!<ref>glLogicOp</ref>.
 *!</c></r>
 *!<r><c><ref>GL_LIGHT</ref><i>i</i>
 *!</c><c>If enabled,
 *!include light <i>i</i> in the evaluation of the lighting
 *!equation. See <ref>glLightModel</ref> and <ref>glLight</ref>.
 *!</c></r>
 *!<r><c><ref>GL_LIGHTING</ref>
 *!</c><c>If enabled,
 *!use the current lighting parameters to compute the vertex color or index.
 *!Otherwise, simply associate the current color or index with each
 *!vertex. See 
 *!
 *!<ref>glMaterial</ref>, <ref>glLightModel</ref>, and <ref>glLight</ref>.
 *!</c></r>
 *!<r><c><ref>GL_LINE_SMOOTH</ref>
 *!</c><c>If enabled,
 *!draw lines with correct filtering.
 *!Otherwise,
 *!draw aliased lines.
 *!See <ref>glLineWidth</ref>.
 *!</c></r>
 *!<r><c><ref>GL_LINE_STIPPLE</ref>
 *!</c><c>If enabled,
 *!use the current line stipple pattern when drawing lines. See
 *!<ref>glLineStipple</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_MAP1_COLOR_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate RGBA values.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_INDEX</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate color indices.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_NORMAL</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate normals.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_1</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>s</i>
 *!texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_2</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>s</i> and
 *!<i>t</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>s</i>,
 *!<i>t</i>, and
 *!<i>r</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>s</i>,
 *!<i>t</i>,
 *!<i>r</i>, and
 *!<i>q</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_VERTEX_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!<i>x</i>, <i>y</i>, and <i>z</i> vertex coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_VERTEX_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord1</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint1</ref> generate
 *!homogeneous
 *!<i>x</i>,
 *!<i>y</i>,
 *!<i>z</i>, and
 *!<i>w</i> vertex coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_COLOR_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate RGBA values.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_INDEX</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate color indices.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_NORMAL</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate normals.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_1</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>s</i>
 *!texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_2</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>s</i> and
 *!<i>t</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>s</i>,
 *!<i>t</i>, and
 *!<i>r</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>s</i>,
 *!<i>t</i>,
 *!<i>r</i>, and
 *!<i>q</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_VERTEX_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!<i>x</i>, <i>y</i>, and <i>z</i> vertex coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_VERTEX_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord2</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint2</ref> generate
 *!homogeneous
 *!<i>x</i>,
 *!<i>y</i>,
 *!<i>z</i>, and
 *!<i>w</i> vertex coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_NORMALIZE</ref>
 *!</c><c>If enabled,
 *!normal vectors specified with <ref>glNormal</ref> are scaled to unit length
 *!after transformation. See <ref>glNormal</ref>.
 *!</c></r>
 *!<r><c><ref>GL_POINT_SMOOTH</ref>
 *!</c><c>If enabled,
 *!draw points with proper filtering.
 *!Otherwise,
 *!draw aliased points.
 *!See <ref>glPointSize</ref>.
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_OFFSET_FILL</ref>
 *!</c><c>If enabled, and if the polygon is rendered in
 *!<ref>GL_FILL</ref> mode, an offset is added to depth values of a polygon's
 *!fragments before the depth comparison is performed. See
 *!<ref>glPolygonOffset</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_OFFSET_LINE</ref>
 *!</c><c>If enabled, and if the polygon is rendered in
 *!<ref>GL_LINE</ref> mode, an offset is added to depth values of a polygon's
 *!fragments before the depth comparison is performed. See <ref>glPolygonOffset</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_OFFSET_POINT</ref>
 *!</c><c>If enabled, an offset is added to depth values of a polygon's fragments
 *!before the depth comparison is performed, if the polygon is rendered in 
 *!<ref>GL_POINT</ref> mode. See 
 *!
 *!<ref>glPolygonOffset</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_SMOOTH</ref>
 *!</c><c>If enabled, draw polygons with proper filtering.
 *!Otherwise, draw aliased polygons. For correct anti-aliased polygons,
 *!an alpha buffer is needed and the polygons must be sorted front to
 *!back. 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_STIPPLE</ref>
 *!</c><c>If enabled,
 *!use the current polygon stipple pattern when rendering
 *!polygons. See <ref>glPolygonStipple</ref>.
 *!</c></r>
 *!<r><c><ref>GL_SCISSOR_TEST</ref>
 *!</c><c>If enabled,
 *!discard fragments that are outside the scissor rectangle. 
 *!See <ref>glScissor</ref>.
 *!</c></r>
 *!<r><c><ref>GL_STENCIL_TEST</ref>
 *!</c><c>If enabled,
 *!do stencil testing and update the stencil buffer. 
 *!See <ref>glStencilFunc</ref> and <ref>glStencilOp</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_1D</ref>
 *!</c><c>If enabled,
 *!one-dimensional texturing is performed
 *!(unless two-dimensional texturing is also enabled). See <ref>glTexImage1D</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_2D</ref>
 *!</c><c>If enabled,
 *!two-dimensional texturing is performed. See <ref>glTexImage2D</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_GEN_Q</ref>
 *!</c><c>If enabled,
 *!the <i>q</i> texture coordinate is computed using
 *!the texture generation function defined with <ref>glTexGen</ref>.
 *!Otherwise, the current <i>q</i> texture coordinate is used.
 *!See <ref>glTexGen</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_GEN_R</ref>
 *!</c><c>If enabled,
 *!the <i>r</i> texture coordinate is computed using
 *!the texture generation function defined with <ref>glTexGen</ref>.
 *!Otherwise, the current <i>r</i> texture coordinate is used.
 *!See <ref>glTexGen</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_GEN_S</ref>
 *!</c><c>If enabled,
 *!the <i>s</i> texture coordinate is computed using
 *!the texture generation function defined with <ref>glTexGen</ref>.
 *!Otherwise, the current <i>s</i> texture coordinate is used. 
 *!See <ref>glTexGen</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_GEN_T</ref>
 *!</c><c>If enabled,
 *!the <i>t</i> texture coordinate is computed using
 *!the texture generation function defined with <ref>glTexGen</ref>.
 *!Otherwise, the current <i>t</i> texture coordinate is used.
 *!See <ref>glTexGen</ref>.
 *!</c></r></matrix>@}
 *!
 *!
 *!@param cap
 *!
 *!@i{cap@}
 *!Specifies a symbolic constant indicating a GL capability.
 *!
 *!
 *!@param cap
 *!
 *!Specifies a symbolic constant indicating a GL capability.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{cap@} is not one of the values
 *!listed previously.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glEnable] or @[glDisable]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glEnableClientState(int cap)
 *!
 *!@[glEnableClientState] and @[glDisableClientState]
 *!enable or disable individual client-side capabilities. By default, all
 *!client-side capabilities are disabled.
 *!Both 
 *!@[glEnableClientState] and @[glDisableClientState] take a
 *!single argument, @i{cap@}, which can assume one of the following
 *!values: 
 *!@xml{<matrix>
 *!<r><c><ref>GL_COLOR_ARRAY</ref>
 *!</c><c>If enabled, the color array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or 
 *!<ref>glDrawElement</ref> is called. See
 *!<ref>glColorPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_EDGE_FLAG_ARRAY</ref>
 *!</c><c>If enabled, the edge flag array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or <ref>glDrawElements</ref> is called. See
 *!<ref>glEdgeFlagPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_INDEX_ARRAY</ref>
 *!</c><c>If enabled, the index array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or 
 *!<ref>glDrawElements</ref> is called. See
 *!<ref>glIndexPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_NORMAL_ARRAY</ref>
 *!</c><c>If enabled, the normal array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or <ref>glDrawElements</ref> is called. See
 *!<ref>glNormalPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_COORD_ARRAY</ref>
 *!</c><c>If enabled, the texture coordinate array is enabled for writing and
 *!used for rendering when <ref>glDrawArrays</ref> or <ref>glDrawElements</ref> is
 *!called. See <ref>glTexCoordPointer</ref>. 
 *!</c></r>
 *!<r><c><ref>GL_VERTEX_ARRAY</ref>
 *!</c><c>If enabled, the vertex array is enabled for writing and used during
 *!rendering when <ref>glDrawArrays</ref> or
 *!<ref>glDrawElements</ref> is called. See
 *!<ref>glVertexPointer</ref>. 
 *!</c></r></matrix>@}
 *!
 *!
 *!@param cap
 *!
 *!@i{cap@}
 *!Specifies the capability to enable.
 *!Symbolic constants
 *!@[GL_COLOR_ARRAY],
 *!@[GL_EDGE_FLAG_ARRAY],
 *!@[GL_INDEX_ARRAY],
 *!@[GL_NORMAL_ARRAY],
 *!@[GL_TEXTURE_COORD_ARRAY], and
 *!@[GL_VERTEX_ARRAY]
 *!are accepted. 
 *!
 *!
 *!@param cap
 *!
 *!Specifies the capability to disable.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{cap@} is not an accepted value.
 *!
 *!@[glEnableClientState] is not allowed between the execution of @[glBegin] and the
 *!corresponding @[glEnd], but an error may or may not be generated. If
 *!no error is generated, the behavior is undefined.
 *!
 *!
 */

/*!@decl void glFrontFace(int mode)
 *!
 *!In a scene composed entirely of opaque closed surfaces,
 *!back-facing polygons are never visible.
 *!Eliminating these invisible polygons has the obvious benefit
 *!of speeding up the rendering of the image.
 *!To enable and disable elimination of back-facing polygons, call @[glEnable]
 *!and @[glDisable] with argument @[GL_CULL_FACE].
 *!
 *!The projection of a polygon to window coordinates is said to have
 *!clockwise winding if an imaginary object following the path
 *!from its first vertex,
 *!its second vertex,
 *!and so on,
 *!to its last vertex,
 *!and finally back to its first vertex,
 *!moves in a clockwise direction about the interior of the polygon.
 *!The polygon's winding is said to be counterclockwise if the imaginary
 *!object following the same path moves in a counterclockwise direction
 *!about the interior of the polygon.
 *!@[glFrontFace] specifies whether polygons with clockwise winding in window coordinates,
 *!or counterclockwise winding in window coordinates,
 *!are taken to be front-facing.
 *!Passing @[GL_CCW] to @i{mode@} selects counterclockwise polygons as
 *!front-facing;
 *!@[GL_CW] selects clockwise polygons as front-facing.
 *!By default, counterclockwise polygons are taken to be front-facing.
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies the orientation of front-facing polygons.
 *!@[GL_CW] and @[GL_CCW] are accepted.
 *!The initial value is @[GL_CCW].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glFrontFace]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glLogicOp(int opcode)
 *!
 *!@[glLogicOp] specifies a logical operation that,
 *!when enabled,
 *!is applied between the incoming color index or RGBA color
 *!and the color index or RGBA color at the corresponding location in the
 *!frame buffer. 
 *!To enable or disable the logical operation, call
 *!@[glEnable] and @[glDisable]
 *!using the symbolic constant @[GL_COLOR_LOGIC_OP] for RGBA mode or
 *!@[GL_INDEX_LOGIC_OP] for color index mode. The initial value is
 *!disabled for both operations.
 *!
 *!.ne
 *!.TS
 *!center box ;
 *!ci | ci
 *!c | c .
 *!opcode	resulting value
 *!=
 *!@[GL_CLEAR]	0
 *!@[GL_SET]	1
 *!@[GL_COPY]	s
 *!@[GL_COPY_INVERTED]	~s
 *!@[GL_NOOP]	d
 *!@[GL_INVERT]	~d
 *!@[GL_AND]	s & d
 *!@[GL_NAND]	~(s & d)
 *!@[GL_OR]	s | d
 *!@[GL_NOR]	~(s | d)
 *!@[GL_XOR]	s ^ d
 *!@[GL_EQUIV]	~(s ^ d)
 *!@[GL_AND_REVERSE]	s & ~d
 *!@[GL_AND_INVERTED]	~s & d
 *!@[GL_OR_REVERSE]	s | ~d
 *!@[GL_OR_INVERTED]	~s | d
 *!.TE
 *!
 *!@i{opcode@} is a symbolic constant chosen from the list above.
 *!In the explanation of the logical operations,
 *!@i{s@} represents the incoming color index and
 *!@i{d@} represents the index in the frame buffer.
 *!Standard C-language operators are used.
 *!As these bitwise operators suggest,
 *!the logical operation is applied independently to each bit pair of the
 *!source and destination indices or colors.
 *!
 *!@param opcode
 *!
 *!@i{opcode@}
 *!Specifies a symbolic constant that selects a logical operation.
 *!The following symbols are accepted:
 *!@[GL_CLEAR],
 *!@[GL_SET],
 *!@[GL_COPY],
 *!@[GL_COPY_INVERTED],
 *!@[GL_NOOP],
 *!@[GL_INVERT],
 *!@[GL_AND],
 *!@[GL_NAND],
 *!@[GL_OR],
 *!@[GL_NOR],
 *!@[GL_XOR],
 *!@[GL_EQUIV],
 *!@[GL_AND_REVERSE],
 *!@[GL_AND_INVERTED],
 *!@[GL_OR_REVERSE], and
 *!@[GL_OR_INVERTED]. The initial value is @[GL_COPY].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{opcode@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLogicOp]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glMatrixMode(int mode)
 *!
 *!@[glMatrixMode] sets the current matrix mode.
 *!@i{mode@} can assume one of three values:
 *!@xml{<matrix>
 *!<r><c><ref>GL_MODELVIEW</ref>
 *!</c><c>Applies subsequent matrix operations to the modelview matrix stack.
 *!</c></r>
 *!<r><c><ref>GL_PROJECTION</ref>
 *!</c><c>Applies subsequent matrix operations to the projection matrix stack.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE</ref>
 *!</c><c>Applies subsequent matrix operations to the texture matrix stack.
 *!</c></r>
 *!</matrix>@}
 *!
 *!To find out which matrix stack is currently the target of all matrix
 *!operations, call @[glGet] with argument @[GL_MATRIX_MODE]. The initial
 *!value is @[GL_MODELVIEW].
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies which matrix stack is the target
 *!for subsequent matrix operations.
 *!Three values are accepted:
 *!@[GL_MODELVIEW],
 *!@[GL_PROJECTION], and
 *!@[GL_TEXTURE].
 *!The initial value is @[GL_MODELVIEW].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glMatrixMode]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glReadBuffer(int mode)
 *!
 *!@[glReadBuffer] specifies a color buffer as the source for subsequent
 *!@[glReadPixels], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *!@[glCopyTexSubImage1D], @[glCopyTexSubImage2D], and
 *!@[glCopyPixels] commands. 
 *!@i{mode@} accepts one of twelve or more predefined values.
 *!(@[GL_AUX0] through @[GL_AUX3] are always defined.)
 *!In a fully configured system,
 *!@[GL_FRONT],
 *!@[GL_LEFT], and
 *!@[GL_FRONT_LEFT] all name the front left buffer,
 *!@[GL_FRONT_RIGHT] and
 *!@[GL_RIGHT] name the front right buffer, and
 *!@[GL_BACK_LEFT] and
 *!@[GL_BACK] name the back left buffer.
 *!
 *!Nonstereo double-buffered configurations have only a front left and a
 *!back left buffer.
 *!Single-buffered configurations have a front left and a front right 
 *!buffer if stereo, and only a front left buffer if nonstereo.
 *!It is an error to specify a nonexistent buffer to @[glReadBuffer].
 *!
 *!@i{mode@} is initially @[GL_FRONT] in single-buffered configurations,
 *!and @[GL_BACK] in double-buffered configurations.
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies a color buffer.
 *!Accepted values are
 *!@[GL_FRONT_LEFT],
 *!@[GL_FRONT_RIGHT],
 *!@[GL_BACK_LEFT],
 *!@[GL_BACK_RIGHT],
 *!@[GL_FRONT],
 *!@[GL_BACK],
 *!@[GL_LEFT],
 *!@[GL_RIGHT], and
 *!@[GL_AUX]@i{i@},
 *!where @i{i@} is between 0 and @[GL_AUX_BUFFERS] -1.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is not one of the twelve
 *!(or more) accepted values.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @i{mode@} specifies a buffer
 *!that does not exist.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glReadBuffer]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glRenderMode(int mode)
 *!
 *!@[glRenderMode] sets the rasterization mode.
 *!It takes one argument,
 *!@i{mode@},
 *!which can assume one of three predefined values: 
 *!@xml{<matrix>
 *!<r><c><ref>GL_RENDER</ref>
 *!</c><c>Render mode. Primitives are rasterized,
 *!producing pixel fragments,
 *!which are written into the frame buffer.
 *!This is the normal mode
 *!and also the default mode.
 *!</c></r>
 *!<r><c><ref>GL_SELECT</ref> 
 *!</c><c>Selection mode.
 *!No pixel fragments are produced,
 *!and no change to the frame buffer contents is made.
 *!Instead,
 *!a record of the names of primitives that would have been drawn
 *!if the render mode had been <ref>GL_RENDER</ref> is returned in a select buffer,
 *!which must be created (see <ref>glSelectBuffer</ref>) before selection mode 
 *!is entered.
 *!</c></r>
 *!<r><c><ref>GL_FEEDBACK</ref>
 *!</c><c>Feedback mode.
 *!No pixel fragments are produced,
 *!and no change to the frame buffer contents is made.
 *!Instead,
 *!the coordinates and attributes of vertices that would have been drawn
 *!if the render mode had been <ref>GL_RENDER</ref> is returned in a feedback buffer,
 *!which must be created (see <ref>glFeedbackBuffer</ref>) before feedback mode
 *!is entered.
 *!</c></r>
 *!</matrix>@}
 *!
 *!The return value of @[glRenderMode] is determined by the render mode at the time
 *!@[glRenderMode] is called,
 *!rather than by @i{mode@}.
 *!The values returned for the three render modes are as follows:
 *!@xml{<matrix>
 *!<r><c><ref>GL_RENDER</ref>
 *!</c><c>0.
 *!</c></r>
 *!<r><c><ref>GL_SELECT</ref>
 *!</c><c>The number of hit records transferred to the select buffer.
 *!</c></r>
 *!<r><c><ref>GL_FEEDBACK</ref>
 *!</c><c>The number of values (not vertices) transferred to the feedback buffer.
 *!</c></r>
 *!</matrix>@}
 *!
 *!See the @[glSelectBuffer] and @[glFeedbackBuffer] reference pages for
 *!more details concerning selection and feedback operation.
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies the rasterization mode.
 *!Three values are accepted:
 *!@[GL_RENDER],
 *!@[GL_SELECT], and
 *!@[GL_FEEDBACK].
 *!The initial value is @[GL_RENDER].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is not one of the three
 *!accepted values.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glSelectBuffer] is called
 *!while the render mode is @[GL_SELECT],
 *!or if @[glRenderMode] is called with argument @[GL_SELECT] before
 *!@[glSelectBuffer] is called at least once.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glFeedbackBuffer] is called
 *!while the render mode is @[GL_FEEDBACK],
 *!or if @[glRenderMode] is called with argument @[GL_FEEDBACK] before
 *!@[glFeedbackBuffer] is called at least once.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glRenderMode]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glShadeModel(int mode)
 *!
 *!GL primitives can have either flat or smooth shading.
 *!Smooth shading,
 *!the default,
 *!causes the computed colors of vertices to be interpolated as the
 *!primitive is rasterized,
 *!typically assigning different colors to each resulting pixel fragment.
 *!Flat shading selects the computed color of just one vertex
 *!and assigns it to all the pixel fragments
 *!generated by rasterizing a single primitive.
 *!In either case, the computed color of a vertex is the result of
 *!lighting if lighting is enabled,
 *!or it is the current color at the time the vertex was specified if
 *!lighting is disabled. 
 *!
 *!Flat and smooth shading are indistinguishable for points.
 *!Starting when @[glBegin] is issued and counting vertices and
 *!primitives from 1, the GL gives each flat-shaded line segment $i$ the
 *!computed color of vertex $i + 1$, its second vertex.
 *!Counting similarly from 1,
 *!the GL gives each flat-shaded polygon the computed color of the vertex listed
 *!in the following table.
 *!This is the last vertex to specify the polygon in all cases except single
 *!polygons,
 *!where the first vertex specifies the flat-shaded color.
 *!.sp
 *!.TS
 *!center box;
 *!l | c .
 *!@i{primitive type of polygon@} $i$	@i{vertex@}
 *!=
 *!Single polygon ($ i == 1 $)	1
 *!Triangle strip	$i + 2$
 *!Triangle fan	$i + 2$
 *!Independent triangle	$ 3 i$
 *!Quad strip	$2 i + 2$
 *!Independent quad 	$ 4 i $
 *!.TE
 *!.sp
 *!Flat and smooth shading are specified by @[glShadeModel] with @i{mode@} set to
 *!@[GL_FLAT] and @[GL_SMOOTH], respectively.
 *!
 *!@param mode
 *!
 *!@i{mode@}
 *!Specifies a symbolic value representing a shading technique.
 *!Accepted values are @[GL_FLAT] and @[GL_SMOOTH].
 *!The initial value is @[GL_SMOOTH].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is any value other than
 *!@[GL_FLAT] or @[GL_SMOOTH].
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glShadeModel]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */


/*! @endmodule
 */

