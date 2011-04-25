/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* AutoDoc generated from OpenGL man pages */

/*!@module GL
 *!
 *!Not implemented OpenGL methods:
 *!
 *!@xml{<matrix>
 *!<r><c>glAreTexturesResident</c></r>
 *!<r><c>glBitmap</c></r>
 *!<r><c>glBlendColorEXT</c></r>
 *!<r><c>glCallLists</c></r>
 *!<r><c>glColorPointer</c></r>
 *!<r><c>glDeleteTextures</c></r>
 *!<r><c>glDrawElements</c></r>
 *!<r><c>glEdgeFlagPointer</c></r>
 *!<r><c>glEdgeFlagv</c></r>
 *!<r><c>glEvalMesh</c></r>
 *!<r><c>glFeedbackBuffer</c></r>
 *!<r><c>glGenTextures</c></r>
 *!<r><c>glGetBooleanv</c></r>
 *!<r><c>glGetClipPlane</c></r>
 *!<r><c>glGetDoublev</c></r>
 *!<r><c>glGetFloatv</c></r>
 *!<r><c>glGetIntegerv</c></r>
 *!<r><c>glGetLight</c></r>
 *!<r><c>glGetMap</c></r>
 *!<r><c>glGetMaterial</c></r>
 *!<r><c>glGetPixelMap</c></r>
 *!<r><c>glGetPointerv</c></r>
 *!<r><c>glGetPolygonStipple</c></r>
 *!<r><c>glGetTexEnv</c></r>
 *!<r><c>glGetTexGen</c></r>
 *!<r><c>glGetTexImage</c></r>
 *!<r><c>glGetTexLevelParameter</c></r>
 *!<r><c>glGetTexParameter</c></r>
 *!<r><c>glIndexPointer</c></r>
 *!<r><c>glInterleavedArrays</c></r>
 *!<r><c>glMap1</c></r>
 *!<r><c>glMap2</c></r>
 *!<r><c>glMapGrid</c></r>
 *!<r><c>glNormalPointer</c></r>
 *!<r><c>glPixelMap</c></r>
 *!<r><c>glPixelStore</c></r>
 *!<r><c>glPixelTransfer</c></r>
 *!<r><c>glPolygonStipple</c></r>
 *!<r><c>glPrioritizeTextures</c></r>
 *!<r><c>glReadPixels</c></r>
 *!<r><c>glRect</c></r>
 *!<r><c>glSelectBuffer</c></r>
 *!<r><c>glTexCoordPointer</c></r>
 *!<r><c>glTexImage1D</c></r>
 *!<r><c>glTexSubImage1D</c></r>
 *!<r><c>glVertexPointer</c></r>
 *!</matrix>@}
 *!
 */


/*!@decl void glAccum(int op, float value)
 *!
 *!The accumulation buffer is an extended-range color buffer.
 *!Images are not rendered into it.
 *!Rather,
 *!images rendered into one of the color buffers
 *!are added to the contents of the accumulation buffer after rendering.
 *!Effects such as antialiasing (of points, lines, and polygons),
 *!motion blur,
 *!and depth of field can be created
 *!by accumulating images generated with different transformation matrices.
 *!
 *!Each pixel in the accumulation buffer consists of
 *!red, green, blue, and alpha values.
 *!The number of bits per component in the accumulation buffer
 *!depends on the implementation. You can examine this number
 *!by calling @[glGetIntegerv] four times,
 *!with arguments @[GL_ACCUM_RED_BITS],
 *!@[GL_ACCUM_GREEN_BITS],
 *!@[GL_ACCUM_BLUE_BITS],
 *!and @[GL_ACCUM_ALPHA_BITS].
 *!Regardless of the number of bits per component,
 *!the range of values stored by each component is [-1, 1].
 *!The accumulation buffer pixels are mapped one-to-one with frame buffer pixels.
 *!
 *!@[glAccum] operates on the accumulation buffer.
 *!The first argument, @i{op@},
 *!is a symbolic constant that selects an accumulation buffer operation.
 *!The second argument, @i{value@},
 *!is a floating-point value to be used in that operation.
 *!Five operations are specified:
 *!@[GL_ACCUM], @[GL_LOAD], @[GL_ADD],
 *!@[GL_MULT], and @[GL_RETURN].
 *!
 *!All accumulation buffer operations are limited
 *!to the area of the current scissor box and applied identically to
 *!the red, green, blue, and alpha components of each pixel.
 *!If a @[glAccum] operation results in a value outside the range [-1, 1], 
 *!the contents of an accumulation buffer pixel component are undefined.
 *!
 *!The operations are as follows:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_ACCUM</ref>
 *!</c><c>Obtains R, G, B, and A values
 *!from the buffer currently selected for reading (see <ref>glReadBuffer</ref>).
 *!Each component value is divided by 2n1,
 *!where n is the number of bits allocated to each color component
 *!in the currently selected buffer.
 *!The result is a floating-point value in the range [0, 1],
 *!which is multiplied by <i>value</i> and added to the corresponding pixel component
 *!in the accumulation buffer,
 *!thereby updating the accumulation buffer.
 *!</c></r>
 *!<r><c><ref>GL_LOAD</ref> 
 *!</c><c>Similar to <ref>GL_ACCUM</ref>,
 *!except that the current value in the accumulation buffer is not used
 *!in the calculation of the new value.
 *!That is, the R, G, B, and A values from the currently selected buffer
 *!are divided by  2n1,
 *!multiplied by <i>value</i>,
 *!and then stored in the corresponding accumulation buffer cell,
 *!overwriting the current value.
 *!</c></r>
 *!<r><c><ref>GL_ADD</ref> 
 *!</c><c>Adds <i>value</i> to each R, G, B, and A
 *!in the accumulation buffer. 
 *!</c></r>
 *!<r><c><ref>GL_MULT</ref> 
 *!</c><c>Multiplies each R, G, B, and A
 *!in the accumulation buffer by <i>value</i> and returns the scaled component
 *!to its corresponding accumulation buffer location.
 *!</c></r>
 *!<r><c><ref>GL_RETURN</ref> 
 *!</c><c>Transfers accumulation buffer values
 *!to the color buffer or buffers currently selected for writing.
 *!Each R, G, B, and A component is multiplied by <i>value</i>,
 *!then multiplied by  2n1,
 *!clamped to the range [0, 2n1 ], and stored
 *!in the corresponding display buffer cell.
 *!The only fragment operations that are applied to this transfer are
 *!pixel ownership,
 *!scissor,
 *!dithering,
 *!and color writemasks.
 *!</c></r>
 *!</matrix>@}
 *!
 *!To clear the accumulation buffer, call @[glClearAccum] with R, G, B,
 *!and A values to set it to, then call @[glClear] with the
 *!accumulation buffer enabled. 
 *!
 *!@param op
 *!
 *!Specifies the accumulation buffer operation.
 *!Symbolic constants
 *!@[GL_ACCUM],
 *!@[GL_LOAD],
 *!@[GL_ADD],
 *!@[GL_MULT],
 *!and
 *!@[GL_RETURN] are accepted.
 *!
 *!@param value
 *!
 *!Specifies a floating-point value used in the accumulation buffer operation.
 *!@i{op@} determines how @i{value@} is used.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{op@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if there is no accumulation buffer.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glAccum]
 *!is executed between the execution of
 *!@[glBegin] and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glAlphaFunc(int func, float ref)
 *!
 *!The alpha test discards fragments depending on the outcome of a comparison
 *!between an incoming fragment's alpha value and a constant reference value.
 *!@[glAlphaFunc] specifies the reference value and the comparison function.
 *!The comparison is performed only if alpha testing is enabled. By
 *!default, it is not enabled. 
 *!(See 
 *!@[glEnable] and @[glDisable] of @[GL_ALPHA_TEST].)
 *!
 *!@i{func@} and @i{ref@} specify the conditions under which
 *!the pixel is drawn.
 *!The incoming alpha value is compared to @i{ref@}
 *!using the function specified by @i{func@}.
 *!If the value passes the comparison,
 *!the incoming fragment is drawn
 *!if it also passes subsequent stencil and depth buffer tests. 
 *!If the value fails the comparison,
 *!no change is made to the frame buffer at that pixel location. The
 *!comparison functions are as follows: 
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_NEVER</ref>
 *!</c><c>Never passes. 
 *!</c></r>
 *!<r><c><ref>GL_LESS</ref>
 *!</c><c>Passes if the incoming alpha value is less than the reference value.
 *!</c></r>
 *!<r><c><ref>GL_EQUAL</ref>
 *!</c><c>Passes if the incoming alpha value is equal to the reference value.
 *!</c></r>
 *!<r><c><ref>GL_LEQUAL</ref>
 *!</c><c>Passes if the incoming alpha value is less than or equal to the reference value.
 *!</c></r>
 *!<r><c><ref>GL_GREATER</ref> 
 *!</c><c>Passes if the incoming alpha value is greater than the reference value.
 *!</c></r>
 *!<r><c><ref>GL_NOTEQUAL</ref>
 *!</c><c>Passes if the incoming alpha value is not equal to the reference value.
 *!</c></r>
 *!<r><c><ref>GL_GEQUAL</ref>
 *!</c><c>Passes if the incoming alpha value is greater than or equal to
 *!the reference value.
 *!</c></r>
 *!<r><c><ref>GL_ALWAYS</ref>
 *!</c><c>Always passes (initial value). 
 *!</c></r>
 *!</matrix>@}
 *!
 *!@[glAlphaFunc] operates on all pixel write operations,
 *!including those resulting from the scan conversion of points,
 *!lines,
 *!polygons,
 *!and bitmaps,
 *!and from pixel draw and copy operations.
 *!@[glAlphaFunc] does not affect screen clear operations.
 *!
 *!@param func
 *!
 *!Specifies the alpha comparison function.
 *!Symbolic constants
 *!@[GL_NEVER],
 *!@[GL_LESS],
 *!@[GL_EQUAL],
 *!@[GL_LEQUAL],
 *!@[GL_GREATER],
 *!@[GL_NOTEQUAL],
 *!@[GL_GEQUAL], and
 *!@[GL_ALWAYS] are accepted. The initial value is @[GL_ALWAYS].
 *!
 *!@param ref
 *!
 *!Specifies the reference value that incoming alpha values are compared to.
 *!This value is clamped to the range 0 through 1,
 *!where 0 represents the lowest possible alpha value
 *!and 1 the highest possible value.
 *!The initial reference value
 *!is 0.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{func@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glAlphaFunc]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glArrayElement(int i)
 *!
 *!@[glArrayElement] commands are used within @[glBegin]/@[glEnd] pairs to
 *!specify vertex and attribute data for point, line, and polygon
 *!primitives. If @[GL_VERTEX_ARRAY] is enabled when @[glArrayElement] is called, a
 *!single vertex is drawn, using 
 *!vertex and attribute data taken from location @i{i@} of the enabled
 *!arrays. If @[GL_VERTEX_ARRAY] is not enabled, no drawing occurs but
 *!the attributes corresponding to the enabled arrays are modified. 
 *!
 *!Use @[glArrayElement] to construct primitives by indexing vertex data, rather than
 *!by streaming through arrays of data in first-to-last order. Because
 *!each call specifies only a single vertex, it is possible to explicitly
 *!specify per-primitive attributes such as a single normal per
 *!individual triangle.
 *!
 *!Changes made to array data between the execution of @[glBegin] and the 
 *!corresponding execution of @[glEnd] may affect calls to @[glArrayElement] that are made
 *!within the same @[glBegin]/@[glEnd] period in non-sequential ways.
 *!That is, a call to 
 *!
 *!@[glArrayElement] that precedes a change to array data may 
 *!access the changed data, and a call that follows a change to array data 
 *!may access original data.
 *!
 *!@param i
 *!
 *!Specifies an index into the enabled vertex data arrays. 
 *!
 *!
 */

/*!@decl void glBindTexture(int target, int texture)
 *!
 *!@[glBindTexture] lets you create or use a named texture. Calling @[glBindTexture] with 
 *!
 *!@i{target@} set to
 *!@[GL_TEXTURE_1D] or @[GL_TEXTURE_2D] and @i{texture@} set to the name
 *!of the newtexture binds the texture name to the target.  
 *!When a texture is bound to a target, the previous binding for that
 *!target is automatically broken.
 *!
 *!Texture names are unsigned integers. The value zero is reserved to
 *!represent the default texture for each texture target.
 *!Texture names and the corresponding texture contents are local to
 *!the shared display-list space (see @[glXCreateContext]) of the current
 *!GL rendering context;
 *!two rendering contexts share texture names only if they
 *!also share display lists.
 *!
 *!You may use @[glGenTextures] to generate a set of new texture names.
 *!
 *!When a texture is first bound, it assumes the dimensionality of its
 *!target:  A texture first bound to @[GL_TEXTURE_1D] becomes
 *!1-dimensional and a texture first bound to @[GL_TEXTURE_2D] becomes
 *!2-dimensional. The state of a 1-dimensional texture
 *!immediately after it is first bound is equivalent to the state of the
 *!default @[GL_TEXTURE_1D] at GL initialization, and similarly for 
 *!2-dimensional textures.
 *!
 *!While a texture is bound, GL operations on the target to which it is
 *!bound affect the bound texture, and queries of the target to which it
 *!is bound return state from the bound texture. If texture mapping of
 *!the dimensionality of the target to which a texture is bound is
 *!active, the bound texture is used.
 *!In effect, the texture targets become aliases for the textures currently
 *!bound to them, and the texture name zero refers to the default textures
 *!that were bound to them at initialization.
 *!
 *!A texture binding created with @[glBindTexture] remains active until a different
 *!texture is bound to the same target, or until the bound texture is
 *!deleted with @[glDeleteTextures].
 *!
 *!Once created, a named texture may be re-bound to the target of the
 *!matching dimensionality as often as needed.
 *!It is usually much faster to use @[glBindTexture] to bind an existing named
 *!texture to one of the texture targets than it is to reload the texture image
 *!using @[glTexImage1D] or @[glTexImage2D].
 *!For additional control over performance, use
 *!@[glPrioritizeTextures].
 *!
 *!@[glBindTexture] is included in display lists.
 *!
 *!@param target
 *!
 *!Specifies the target to which the texture is bound.
 *!Must be either
 *!@[GL_TEXTURE_1D] or
 *!@[GL_TEXTURE_2D].
 *!
 *!@param texture
 *!
 *!Specifies the name of a texture. 
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{target@} is not one of the allowable
 *!values.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @i{texture@} has a dimensionality
 *!which doesn't match that of @i{target@}.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glBindTexture] is executed
 *!between the execution of @[glBegin] and the corresponding
 *!execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glBlendFunc(int sfactor, int dfactor)
 *!
 *!In RGBA mode, pixels can be drawn using a function that blends
 *!the incoming (source) RGBA values with the RGBA values
 *!that are already in the frame buffer (the destination values).
 *!Blending is initially disabled.
 *!Use @[glEnable] and @[glDisable] with argument @[GL_BLEND]
 *!to enable and disable blending.
 *!
 *!@[glBlendFunc] defines the operation of blending when it is enabled.
 *!@i{sfactor@} specifies which of nine methods is used to scale the
 *!source color components.
 *!@i{dfactor@} specifies which of eight methods is used to scale the
 *!destination color components.
 *!The eleven possible methods are described in the following table.
 *!Each method defines four scale factors,
 *!one each for red, green, blue, and alpha.
 *!
 *!In the table and in subsequent equations, source and destination
 *!color components are referred to as
 *!(R sub s , G sub s , B sub s , A sub s ) and
 *!(R sub d , G sub d , B sub d , A sub d ).
 *!They are understood to have integer values between 0 and
 *!(k sub R , k sub G , k sub B , k sub A ),
 *!where
 *!
 *!.RS
 *!.ce
 *!k sub c ~=~ 2 sup m sub c - 1 
 *!.RE
 *!
 *!and
 *!(m sub R , m sub G , m sub B , m sub A )
 *!is the number of red,
 *!green,
 *!blue,
 *!and alpha bitplanes.
 *!
 *!Source and destination scale factors are referred to as
 *!(s sub R , s sub G , s sub B , s sub A ) and
 *!(d sub R , d sub G , d sub B , d sub A ).
 *!The scale factors described in the table,
 *!denoted (f sub R , f sub G , f sub B , f sub A ),
 *!represent either source or destination factors.
 *!All scale factors have range [0,1].
 *!
 *!.TS
 *!center box ;
 *!ci | ci
 *!c | c .
 *!parameter	(f sub R , ~~ f sub G , ~~ f sub B , ~~ f sub A ) 
 *!=
 *!@[GL_ZERO]	(0, ~0, ~0, ~0 )
 *!@[GL_ONE]	(1, ~1, ~1, ~1 )
 *!@[GL_SRC_COLOR]	(R sub s / k sub R , ~G sub s / k sub G , ~B sub s / k sub B , ~A sub s / k sub A )
 *!@[GL_ONE_MINUS_SRC_COLOR]	(1, ~1, ~1, ~1 ) ~-~ (R sub s / k sub R , ~G sub s / k sub G , ~B sub s / k sub B , ~A sub s / k sub A )
 *!@[GL_DST_COLOR]	(R sub d / k sub R , ~G sub d / k sub G , ~B sub d / k sub B , ~A sub d / k sub A )
 *!@[GL_ONE_MINUS_DST_COLOR]	(1, ~1, ~1, ~1 ) ~-~ (R sub d / k sub R , ~G sub d / k sub G , ~B sub d / k sub B , ~A sub d / k sub A )
 *!@[GL_SRC_ALPHA]	(A sub s / k sub A , ~A sub s / k sub A , ~A sub s / k sub A , ~A sub s / k sub A )
 *!@[GL_ONE_MINUS_SRC_ALPHA]	(1, ~1, ~1, ~1 ) ~-~ (A sub s / k sub A , ~A sub s / k sub A , ~A sub s / k sub A , ~A sub s / k sub A )
 *!@[GL_DST_ALPHA]	(A sub d / k sub A , ~A sub d / k sub A , ~A sub d / k sub A , ~A sub d / k sub A )
 *!@[GL_ONE_MINUS_DST_ALPHA]	(1, ~1, ~1, ~1 ) ~-~ (A sub d / k sub A , ~A sub d / k sub A , ~A sub d / k sub A , ~A sub d / k sub A )
 *!@[GL_SRC_ALPHA_SATURATE]	(i, ~i, ~i, ~1 )
 *!.TE
 *!.sp
 *!In the table,
 *!
 *!.RS
 *!.nf
 *!
 *!i ~=~  min (A sub s , ~k sub A - A sub d ) ~/~ k sub A
 *!.fi
 *!.RE
 *!
 *!To determine the blended RGBA values of a pixel when drawing in RGBA mode,
 *!the system uses the following equations:
 *!
 *!.RS
 *!.nf
 *!
 *!R sub d ~=~ min ( k sub R , ~~ R sub s s sub R + R sub d d sub R )
 *!G sub d ~=~ min ( k sub G , ~~ G sub s s sub G + G sub d d sub G )
 *!B sub d ~=~ min ( k sub B , ~~ B sub s s sub B + B sub d d sub B )
 *!A sub d ~=~ min ( k sub A , ~~ A sub s s sub A + A sub d d sub A )
 *!.fi
 *!.RE
 *!
 *!Despite the apparent precision of the above equations,
 *!blending arithmetic is not exactly specified,
 *!because blending operates with imprecise integer color values.
 *!However,
 *!a blend factor that should be equal to 1
 *!is guaranteed not to modify its multiplicand,
 *!and a blend factor equal to 0 reduces its multiplicand to 0.
 *!For example,
 *!when @i{sfactor@} is @[GL_SRC_ALPHA],
 *!@i{dfactor@} is @[GL_ONE_MINUS_SRC_ALPHA],
 *!and A sub s is equal to k sub A,
 *!the equations reduce to simple replacement:
 *!
 *!.RS
 *!.nf
 *!
 *!R sub d ~=~ R sub s
 *!G sub d ~=~ G sub s
 *!B sub d ~=~ B sub s
 *!A sub d ~=~ A sub s
 *!.fi
 *!.RE
 *!
 *!
 *!@param sfactor
 *!
 *!Specifies how the red, green, blue,
 *!and alpha source blending factors are computed.
 *!Nine symbolic constants are accepted:
 *!@[GL_ZERO],
 *!@[GL_ONE],
 *!@[GL_DST_COLOR],
 *!@[GL_ONE_MINUS_DST_COLOR],
 *!@[GL_SRC_ALPHA],
 *!@[GL_ONE_MINUS_SRC_ALPHA],
 *!@[GL_DST_ALPHA],
 *!@[GL_ONE_MINUS_DST_ALPHA], and
 *!@[GL_SRC_ALPHA_SATURATE]. The initial value is @[GL_ONE].
 *!
 *!@param dfactor
 *!
 *!Specifies how the red, green, blue,
 *!and alpha destination blending factors are computed.
 *!Eight symbolic constants are accepted:
 *!@[GL_ZERO],
 *!@[GL_ONE],
 *!@[GL_SRC_COLOR],
 *!@[GL_ONE_MINUS_SRC_COLOR],
 *!@[GL_SRC_ALPHA],
 *!@[GL_ONE_MINUS_SRC_ALPHA],
 *!@[GL_DST_ALPHA], and
 *!@[GL_ONE_MINUS_DST_ALPHA]. The initial value is @[GL_ZERO].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if either @i{sfactor@} or @i{dfactor@} is not an
 *!accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glBlendFunc]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glCallList(int list)
 *!
 *!@[glCallList] causes the named display list to be executed.
 *!The commands saved in the display list are executed in order,
 *!just as if they were called without using a display list.
 *!If @i{list@} has not been defined as a display list,
 *!@[glCallList] is ignored.
 *!
 *!@[glCallList] can appear inside a display list.
 *!To avoid the possibility of infinite recursion resulting from display lists
 *!calling one another,
 *!a limit is placed on the nesting level of display
 *!lists during display-list execution.
 *!This limit is at least 64, and it depends on the implementation.
 *!
 *!GL state is not saved and restored across a call to @[glCallList].
 *!Thus,
 *!changes made to GL state during the execution of a display list
 *!remain after execution of the display list is completed.
 *!Use @[glPushAttrib],
 *!@[glPopAttrib],
 *!@[glPushMatrix],
 *!and @[glPopMatrix] to preserve GL state across @[glCallList] calls.
 *!
 *!@param list
 *!
 *!Specifies the integer name of the display list to be executed.
 *!
 *!
 */

/*!@decl void glClear(int mask)
 *!
 *!@[glClear] sets the bitplane area of the window to values previously selected
 *!by @[glClearColor], @[glClearIndex], @[glClearDepth], 
 *!@[glClearStencil], and @[glClearAccum].
 *!Multiple color buffers can be cleared simultaneously by selecting
 *!more than one buffer at a time using @[glDrawBuffer].
 *!
 *!The pixel ownership test,
 *!the scissor test,
 *!dithering, and the buffer writemasks affect the operation of @[glClear].
 *!The scissor box bounds the cleared region.
 *!Alpha function,
 *!blend function,
 *!logical operation,
 *!stenciling,
 *!texture mapping,
 *!and depth-buffering are ignored by @[glClear].
 *!
 *!@[glClear] takes a single argument that is the bitwise OR of several
 *!values indicating which buffer is to be cleared.
 *!
 *!The values are as follows: 
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_COLOR_BUFFER_BIT</ref>
 *!</c><c>Indicates the buffers currently enabled for color
 *!writing.
 *!</c></r>
 *!<r><c><ref>GL_DEPTH_BUFFER_BIT</ref>
 *!</c><c>Indicates the depth buffer.
 *!</c></r>
 *!<r><c><ref>GL_ACCUM_BUFFER_BIT</ref>
 *!</c><c>Indicates the accumulation buffer. 
 *!</c></r>
 *!<r><c><ref>GL_STENCIL_BUFFER_BIT</ref>
 *!</c><c>Indicates the stencil buffer.
 *!</c></r>
 *!</matrix>@}
 *!
 *!The value to which each buffer is cleared depends on the setting of the
 *!clear value for that buffer.  
 *!
 *!@param mask
 *!
 *!Bitwise OR of masks that indicate the buffers to be cleared.
 *!The four masks are
 *!@[GL_COLOR_BUFFER_BIT],
 *!@[GL_DEPTH_BUFFER_BIT],
 *!@[GL_ACCUM_BUFFER_BIT], and
 *!@[GL_STENCIL_BUFFER_BIT].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if any bit other than the four defined
 *!bits is set in @i{mask@}.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glClear]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glClearAccum(float|array(float) red, float|void green, float|void blue, float|void alpha)
 *!
 *!@[glClearAccum] specifies the red, green, blue, and alpha values used by @[glClear] 
 *!to clear the accumulation buffer.
 *!
 *!Values specified by @[glClearAccum] are clamped to the 
 *!range [-1,1].
 *!
 *!@param red
 *!
 *!Specify the red, green, blue, and alpha values used when the
 *!accumulation buffer is cleared.
 *!The initial values are all 0.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glClearAccum]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glClearColor(float|array(float) red, float|void green, float|void blue, float|void alpha)
 *!
 *!@[glClearColor] specifies the red,
 *!green,
 *!blue,
 *!and alpha values used by @[glClear] to clear the color buffers.
 *!Values specified by @[glClearColor] are clamped to the range [0,1].
 *!
 *!@param red
 *!
 *!Specify the red, green, blue, and alpha values used when the
 *!color buffers are cleared.
 *!The initial values are all 0.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glClearColor]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glClearDepth(float depth)
 *!
 *!@[glClearDepth] specifies the depth value used by @[glClear] to clear the depth buffer.
 *!Values specified by @[glClearDepth] are clamped to the range [0,1].
 *!
 *!@param depth
 *!
 *!Specifies the depth value used when the depth buffer is cleared. The
 *!initial value is 1.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glClearDepth]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glClearIndex(float c)
 *!
 *!@[glClearIndex] specifies the index used by @[glClear]
 *!to clear the color index buffers.
 *!@i{c@} is not clamped.
 *!Rather,
 *!@i{c@} is converted to a fixed-point value with unspecified precision
 *!to the right of the binary point.
 *!The integer part of this value is then masked with 2 sup m -1,
 *!where m is the number of bits in a color index stored in the frame buffer.
 *!
 *!@param c
 *!
 *!Specifies the index used when the color index buffers are cleared.
 *!The initial value is 0.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glClearIndex]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glClearStencil(int s)
 *!
 *!@[glClearStencil] specifies the index used by @[glClear] to clear the stencil buffer.
 *!@i{s@} is masked with 2 sup m - 1,
 *!where m is the number of bits in the stencil buffer.
 *!
 *!@param s
 *!
 *!Specifies the index used when the stencil buffer is cleared.
 *!The initial value is 0.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glClearStencil]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glColor(float|int red, float|int green, float|int blue, float|int|void alpha)
 *!@decl void glColor(array(float|int) rgb)
 *!
 *!The GL stores both a current single-valued color index
 *!and a current four-valued RGBA color. If no alpha value has been give, 1.0 (full intensity)
 *!is implied.
 *!
 *!Current color values are stored in floating-point format,
 *!with unspecified mantissa and exponent sizes.
 *!Unsigned integer color components,
 *!when specified,
 *!are linearly mapped to floating-point values such that the largest
 *!representable value maps to 1.0 (full intensity),
 *!and 0 maps to 0.0 (zero intensity).
 *!Signed integer color components,
 *!when specified,
 *!are linearly mapped to floating-point values such that the most positive
 *!representable value maps to 1.0,
 *!and the most negative representable value maps to -1.0. (Note that
 *!this mapping does not convert 0 precisely to 0.0.)
 *!Floating-point values are mapped directly.
 *!
 *!Neither floating-point nor signed integer values are clamped
 *!to the range [0,1] before the current color is updated.
 *!However,
 *!color components are clamped to this range before they are interpolated
 *!or written into a color buffer.
 *!
 *!@param red
 *!
 *!Specify new red, green, and blue values for the current color.
 *!
 *!@param alpha
 *!
 *!Specifies a new alpha value for the current color.
 *!
 */

/*!@decl void glColorMask(int red, int green, int blue, int alpha)
 *!
 *!@[glColorMask] specifies whether the individual color components in the frame buffer
 *!can or cannot be written.
 *!If @i{red@} is @[GL_FALSE],
 *!for example,
 *!no change is made to the red component of any pixel in any of the
 *!color buffers,
 *!regardless of the drawing operation attempted.
 *!
 *!Changes to individual bits of components cannot be controlled.
 *!Rather,
 *!changes are either enabled or disabled for entire color components.
 *!
 *!@param red
 *!
 *!Specify whether red, green, blue, and alpha can or cannot be written
 *!into the frame buffer.
 *!The initial values are all @[GL_TRUE],
 *!indicating that the color components can be written.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glColorMask]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glColorMaterial(int face, int mode)
 *!
 *!@[glColorMaterial] specifies which material parameters track the current color.
 *!When @[GL_COLOR_MATERIAL] is enabled,
 *!the material parameter or parameters specified by @i{mode@},
 *!of the material or materials specified by @i{face@},
 *!track the current color at all times.
 *!
 *!To enable and disable @[GL_COLOR_MATERIAL], call
 *!@[glEnable] and @[glDisable] with argument @[GL_COLOR_MATERIAL].
 *!@[GL_COLOR_MATERIAL] is initially disabled.
 *!
 *!@param face
 *!
 *!Specifies whether front,
 *!back,
 *!or both front and back material parameters should track the current color.
 *!Accepted values are
 *!@[GL_FRONT],
 *!@[GL_BACK],
 *!and @[GL_FRONT_AND_BACK].
 *!The initial value is @[GL_FRONT_AND_BACK].
 *!
 *!@param mode
 *!
 *!Specifies which of several material parameters track the current color.
 *!Accepted values are
 *!@[GL_EMISSION],
 *!@[GL_AMBIENT],
 *!@[GL_DIFFUSE],
 *!@[GL_SPECULAR],
 *!and @[GL_AMBIENT_AND_DIFFUSE].
 *!The initial value is @[GL_AMBIENT_AND_DIFFUSE].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{face@} or @i{mode@} is not an
 *!accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glColorMaterial] is executed between
 *!the execution of @[glBegin] and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glCopyPixels(int x, int y, int width, int height, int type)
 *!
 *!@[glCopyPixels] copies a screen-aligned rectangle of pixels
 *!from the specified frame buffer location to a region relative to the
 *!current raster position.
 *!Its operation is well defined only if the entire pixel source region
 *!is within the exposed portion of the window.
 *!Results of copies from outside the window,
 *!or from regions of the window that are not exposed,
 *!are hardware dependent and undefined.
 *!
 *!@i{x@} and @i{y@} specify the window coordinates of
 *!the lower left corner of the rectangular region to be copied.
 *!@i{width@} and @i{height@} specify the dimensions of the
 *!rectangular region to be copied.
 *!Both @i{width@} and @i{height@} must not be negative.
 *!
 *!Several parameters control the processing of the pixel data
 *!while it is being copied.
 *!These parameters are set with three commands:
 *!@[glPixelTransfer],
 *!@[glPixelMap], and
 *!@[glPixelZoom].
 *!This reference page describes the effects on @[glCopyPixels] of most,
 *!but not all, of the parameters specified by these three commands.
 *!
 *!@[glCopyPixels] copies values from each pixel with the lower left-hand corner at
 *!(@i{x@} + i, @i{y@} + j) for 0\(<=i<@i{width@}  and 0\(<=j<@i{height@}.
 *!This pixel is said to be the ith pixel in the jth row. 
 *!Pixels are copied in row order from the lowest to the highest row,
 *!left to right in each row.
 *!
 *!@i{type@} specifies whether color, depth, or stencil data is to be copied.
 *!The details of the transfer for each data type are as follows:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_COLOR</ref>
 *!</c><c>Indices or RGBA colors are read from the buffer currently specified as the
 *!read source buffer (see <ref>glReadBuffer</ref>).
 *!If the GL is in color index mode,
 *!each index that is read from this buffer is converted
 *!to a fixed-point format with an unspecified
 *!number of bits to the right of the binary point.
 *!Each index is then shifted left by <ref>GL_INDEX_SHIFT</ref> bits,
 *!and added to <ref>GL_INDEX_OFFSET</ref>.
 *!If <ref>GL_INDEX_SHIFT</ref> is negative,
 *!the shift is to the right.
 *!In either case, zero bits fill otherwise unspecified bit locations in the
 *!result.
 *!If <ref>GL_MAP_COLOR</ref> is true,
 *!the index is replaced with the value that it references in lookup table
 *!<ref>GL_PIXEL_MAP_I_TO_I</ref>.
 *!Whether the lookup replacement of the index is done or not,
 *!the integer part of the index is then ANDed with 2 sup b -1,
 *!where b is the number of bits in a color index buffer.
 *!
 *!If the GL is in RGBA mode,
 *!the red, green, blue, and alpha components of each pixel that is read
 *!are converted to an internal floating-point format with unspecified
 *!precision.
 *!The conversion maps the largest representable component value to 1.0,
 *!and component value 0 to 0.0.
 *!The resulting floating-point color values are then multiplied
 *!by GL_c_SCALE and added to GL_c_BIAS,
 *!where <i>c</i> is RED, GREEN, BLUE, and ALPHA 
 *!for the respective color components.
 *!The results are clamped to the range [0,1].
 *!If <ref>GL_MAP_COLOR</ref> is true,
 *!each color component is scaled by the size of lookup table
 *!<ref>GL_PIXEL_MAP_c_TO_c</ref>,
 *!then replaced by the value that it references in that table.
 *!<i>c</i> is R, G, B, or A.
 *!
 *!The GL then converts the resulting indices or RGBA colors to fragments
 *!by attaching the current raster position <i>z</i> coordinate and
 *!texture coordinates to each pixel,
 *!then assigning window coordinates
 *!(x sub r + i , y sub r + j),
 *!where (x sub r , y sub r) is the current raster position,
 *!and the pixel was the ith pixel in the jth row.
 *!These pixel fragments are then treated just like the fragments generated by
 *!rasterizing points, lines, or polygons.
 *!Texture mapping,
 *!fog,
 *!and all the fragment operations are applied before the fragments are written
 *!to the frame buffer.
 *!</c></r>
 *!<r><c><ref>GL_DEPTH</ref>
 *!</c><c>Depth values are read from the depth buffer and
 *!converted directly to an internal floating-point format
 *!with unspecified precision.
 *!The resulting floating-point depth value is then multiplied
 *!by <ref>GL_DEPTH_SCALE</ref> and added to <ref>GL_DEPTH_BIAS</ref>.
 *!The result is clamped to the range [0,1].
 *!
 *!The GL then converts the resulting depth components to fragments
 *!by attaching the current raster position color or color index and
 *!texture coordinates to each pixel,
 *!then assigning window coordinates
 *!(x sub r + i , y sub r + j),
 *!where (x sub r , y sub r) is the current raster position,
 *!and the pixel was the ith pixel in the jth row.
 *!These pixel fragments are then treated just like the fragments generated by
 *!rasterizing points, lines, or polygons.
 *!Texture mapping,
 *!fog,
 *!and all the fragment operations are applied before the fragments are written
 *!to the frame buffer.
 *!</c></r>
 *!<r><c><ref>GL_STENCIL</ref>
 *!</c><c>Stencil indices are read from the stencil buffer and
 *!converted to an internal fixed-point format
 *!with an unspecified number of bits to the right of the binary point.
 *!Each fixed-point index is then shifted left by <ref>GL_INDEX_SHIFT</ref> bits,
 *!and added to <ref>GL_INDEX_OFFSET</ref>.
 *!If <ref>GL_INDEX_SHIFT</ref> is negative,
 *!the shift is to the right.
 *!In either case, zero bits fill otherwise unspecified bit locations in the
 *!result.
 *!If <ref>GL_MAP_STENCIL</ref> is true,
 *!the index is replaced with the value that it references in lookup table
 *!<ref>GL_PIXEL_MAP_S_TO_S</ref>.
 *!Whether the lookup replacement of the index is done or not,
 *!the integer part of the index is then ANDed with 2 sup b -1,
 *!where b is the number of bits in the stencil buffer.
 *!The resulting stencil indices are then written to the stencil buffer
 *!such that the index read from the ith location of the jth row
 *!is written to location
 *!(x sub r + i , y sub r + j),
 *!where (x sub r , y sub r) is the current raster position.
 *!Only the pixel ownership test,
 *!the scissor test,
 *!and the stencil writemask affect these write operations.
 *!</c></r>
 *!</matrix>@}
 *!
 *!The rasterization described thus far assumes pixel zoom factors of 1.0.
 *!If 
 *!
 *!@[glPixelZoom] is used to change the x and y pixel zoom factors,
 *!pixels are converted to fragments as follows.
 *!If (x sub r, y sub r) is the current raster position,
 *!and a given pixel is in the ith location in the jth row of the source
 *!pixel rectangle,
 *!then fragments are generated for pixels whose centers are in the rectangle
 *!with corners at
 *!
 *!.ce
 *!(x sub r + zoom sub x i, y sub r + zoom sub y j)
 *!.sp .5
 *!.ce
 *! and 
 *!.sp .5
 *!.ce
 *!(x sub r + zoom sub x (i + 1), y sub r + zoom sub y ( j + 1 ))
 *!
 *!where zoom sub x is the value of @[GL_ZOOM_X] and 
 *!zoom sub y is the value of @[GL_ZOOM_Y].
 *!
 *!@param x
 *!
 *!Specify the window coordinates of the lower left corner
 *!of the rectangular region of pixels to be copied.
 *!
 *!@param width
 *!
 *!Specify the dimensions of the rectangular region of pixels to be copied.
 *!Both must be nonnegative.
 *!
 *!@param type
 *!
 *!Specifies whether color values,
 *!depth values,
 *!or stencil values are to be copied.
 *!Symbolic constants
 *!@[GL_COLOR],
 *!@[GL_DEPTH],
 *!and @[GL_STENCIL] are accepted.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{type@} is not an accepted value.
 *!
 *!@[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@} is negative.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @i{type@} is @[GL_DEPTH]
 *!and there is no depth buffer.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @i{type@} is @[GL_STENCIL]
 *!and there is no stencil buffer.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glCopyPixels]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glCopyTexImage1D(int target, int level, int internalFormat, int x, int y, int width, int border)
 *!
 *!@[glCopyTexImage1D] defines a one-dimensional texture image with pixels from the current
 *!@[GL_READ_BUFFER].
 *!
 *!The screen-aligned pixel row with left corner at ("x", "y")
 *!and with a length of "width"~+~2~*~"border" 
 *!defines the texture array
 *!at the mipmap level specified by @i{level@}.
 *!@i{internalFormat@} specifies the internal format of the texture array.
 *!
 *!The pixels in the row are processed exactly as if
 *!@[glCopyPixels] had been called, but the process stops just before
 *!final conversion.
 *!At this point all pixel component values are clamped to the range [0,\ 1]
 *!and then converted to the texture's internal format for storage in the texel
 *!array.
 *!
 *!Pixel ordering is such that lower x screen coordinates correspond to 
 *!lower texture coordinates.
 *!
 *!If any of the pixels within the specified row of the current
 *!@[GL_READ_BUFFER] are outside the window associated with the current
 *!rendering context, then the values obtained for those pixels are undefined.
 *!
 *!@param target
 *!
 *!Specifies the target texture.
 *!Must be @[GL_TEXTURE_1D].
 *!
 *!@param level
 *!
 *!Specifies the level-of-detail number.
 *!Level 0 is the base image level.
 *!Level @i{n@} is the @i{n@}th mipmap reduction image.
 *!
 *!@param internalFormat
 *!
 *!Specifies the internal format of the texture.
 *!Must be one of the following symbolic constants:
 *!@[GL_ALPHA],
 *!@[GL_ALPHA4],
 *!@[GL_ALPHA8],
 *!@[GL_ALPHA12],
 *!@[GL_ALPHA16],
 *!@[GL_LUMINANCE],
 *!@[GL_LUMINANCE4],
 *!@[GL_LUMINANCE8],
 *!@[GL_LUMINANCE12],
 *!@[GL_LUMINANCE16],
 *!@[GL_LUMINANCE_ALPHA],
 *!@[GL_LUMINANCE4_ALPHA4],
 *!@[GL_LUMINANCE6_ALPHA2],
 *!@[GL_LUMINANCE8_ALPHA8],
 *!@[GL_LUMINANCE12_ALPHA4],
 *!@[GL_LUMINANCE12_ALPHA12],
 *!@[GL_LUMINANCE16_ALPHA16],
 *!@[GL_INTENSITY],
 *!@[GL_INTENSITY4],
 *!@[GL_INTENSITY8],
 *!@[GL_INTENSITY12],
 *!@[GL_INTENSITY16],
 *!@[GL_RGB],
 *!@[GL_R3_G3_B2],
 *!@[GL_RGB4], 
 *!@[GL_RGB5],
 *!@[GL_RGB8],
 *!@[GL_RGB10],
 *!@[GL_RGB12],
 *!@[GL_RGB16],
 *!@[GL_RGBA],
 *!@[GL_RGBA2],
 *!@[GL_RGBA4],
 *!@[GL_RGB5_A1],
 *!@[GL_RGBA8],
 *!@[GL_RGB10_A2],
 *!@[GL_RGBA12], or
 *!@[GL_RGBA16].
 *!
 *!@param x
 *!
 *!Specify the window coordinates of the left corner
 *!of the row of pixels to be copied.
 *!
 *!@param width
 *!
 *!Specifies the width of the texture image.
 *!Must be 0 or 2**n ~+~ 2*@i{border@} for some integer n.
 *!The height of the texture image is 1.
 *!
 *!@param border
 *!
 *!Specifies the width of the border.
 *!Must be either 0 or 1.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{target@} is not one of the
 *!allowable values.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *!.P 
 *!@[GL_INVALID_VALUE] may be generated if @i{level@} is greater
 *!than log sub 2 max,
 *!where max is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *!.P 
 *!@[GL_INVALID_VALUE] is generated if @i{internalFormat@} is not an
 *!allowable value.  
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{width@} is less than 0
 *!or greater than 
 *!2 + @[GL_MAX_TEXTURE_SIZE],
 *!or if it cannot be represented as 2 ** n ~+~ 2~*~("border") 
 *!for some integer value of @i{n@}.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{border@} is not 0 or 1.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glCopyTexImage1D]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glCopyTexImage2D(int target, int level, int internalFormat, int x, int y, int width, int height, int border)
 *!
 *!@[glCopyTexImage2D] defines a two-dimensional texture image with pixels from the current
 *!@[GL_READ_BUFFER].
 *!
 *!The screen-aligned pixel rectangle with lower left corner at (@i{x@},
 *!@i{y@}) and with a width of @i{width@}~+~2~*~@i{border@} and a height of
 *!@i{height@}~+~2~*~@i{border@} 
 *!defines the texture array
 *!at the mipmap level specified by @i{level@}.
 *!@i{internalFormat@} specifies the internal format of the texture array.
 *!
 *!The pixels in the rectangle are processed exactly as if
 *!@[glCopyPixels] had been called, but the process stops just before
 *!final conversion.
 *!At this point all pixel component values are clamped to the range [0,1]
 *!and then converted to the texture's internal format for storage in the texel
 *!array.
 *!
 *!Pixel ordering is such that lower x and y screen coordinates correspond to 
 *!lower s and t texture coordinates.
 *!
 *!If any of the pixels within the specified rectangle of the current
 *!@[GL_READ_BUFFER] are outside the window associated with the current
 *!rendering context, then the values obtained for those pixels are undefined.
 *!
 *!
 *!@param target
 *!
 *!Specifies the target texture.
 *!Must be @[GL_TEXTURE_2D].
 *!
 *!@param level
 *!
 *!Specifies the level-of-detail number.
 *!Level 0 is the base image level.
 *!Level @i{n@} is the @i{n@}th mipmap reduction image.
 *!
 *!@param internalFormat
 *!
 *!Specifies the internal format of the texture.
 *!Must be one of the following symbolic constants:
 *!@[GL_ALPHA],
 *!@[GL_ALPHA4],
 *!@[GL_ALPHA8],
 *!@[GL_ALPHA12],
 *!@[GL_ALPHA16],
 *!@[GL_LUMINANCE],
 *!@[GL_LUMINANCE4],
 *!@[GL_LUMINANCE8],
 *!@[GL_LUMINANCE12],
 *!@[GL_LUMINANCE16],
 *!@[GL_LUMINANCE_ALPHA],
 *!@[GL_LUMINANCE4_ALPHA4],
 *!@[GL_LUMINANCE6_ALPHA2],
 *!@[GL_LUMINANCE8_ALPHA8],
 *!@[GL_LUMINANCE12_ALPHA4],
 *!@[GL_LUMINANCE12_ALPHA12],
 *!@[GL_LUMINANCE16_ALPHA16],
 *!@[GL_INTENSITY],
 *!@[GL_INTENSITY4],
 *!@[GL_INTENSITY8],
 *!@[GL_INTENSITY12],
 *!@[GL_INTENSITY16],
 *!@[GL_RGB],
 *!@[GL_R3_G3_B2],
 *!@[GL_RGB4],
 *!@[GL_RGB5],
 *!@[GL_RGB8],
 *!@[GL_RGB10],
 *!@[GL_RGB12],
 *!@[GL_RGB16],
 *!@[GL_RGBA],
 *!@[GL_RGBA2],
 *!@[GL_RGBA4],
 *!@[GL_RGB5_A1],
 *!@[GL_RGBA8],
 *!@[GL_RGB10_A2],
 *!@[GL_RGBA12], or
 *!@[GL_RGBA16].
 *!
 *!@param x
 *!
 *!Specify the window coordinates of the lower left corner
 *!of the rectangular region of pixels to be copied.
 *!
 *!@param width
 *!
 *!Specifies the width of the texture image.
 *!Must be 0 or 2**n ~+~ 2*@i{border@} for some integer n.
 *!
 *!@param height
 *!
 *!Specifies the height of the texture image.
 *!Must be 0 or 2**m ~+~ 2*@i{border@} for some integer m.
 *!
 *!@param border
 *!
 *!Specifies the width of the border.
 *!Must be either 0 or 1.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{target@} is not @[GL_TEXTURE_2D].
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *!.P 
 *!@[GL_INVALID_VALUE] may be generated if @i{level@} is greater
 *!than log sub 2 max,
 *!where max is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{width@} or @i{height@} is less than 0,
 *!greater than 2~+~@[GL_MAX_TEXTURE_SIZE], or if @i{width@} or @i{height@} cannot be
 *!represented as 2**k ~+~ 2~*~@i{border@} for some integer
 *!k.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{border@} is not 0 or 1.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{internalFormat@} is not one of the
 *!allowable values.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glCopyTexImage2D] is executed
 *!between the execution of @[glBegin] and the corresponding
 *!execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glCopyTexSubImage1D(int target, int level, int xoffset, int x, int y, int width)
 *!
 *!@[glCopyTexSubImage1D] replaces a portion of a one-dimensional
 *!texture image with pixels from the current @[GL_READ_BUFFER] (rather
 *!than from main memory, as is the case for @[glTexSubImage1D]).
 *!
 *!The screen-aligned pixel row with left corner at (@i{x@},\ @i{y@}), and with
 *!length @i{width@} replaces the portion of the
 *!texture array with x indices @i{xoffset@} through "xoffset" ~+~ "width" ~-~ 1,
 *!inclusive. The destination in the texture array may not 
 *!include any texels outside the texture array as it was 
 *!originally specified.
 *!
 *!The pixels in the row are processed exactly as if
 *!@[glCopyPixels] had been called, but the process stops just before
 *!final conversion.
 *!At this point all pixel component values are clamped to the range [0,\ 1]
 *!and then converted to the texture's internal format for storage in the texel
 *!array.
 *!
 *!It is not an error to specify a subtexture with zero width, but
 *!such a specification has no effect.
 *!If any of the pixels within the specified row of the current
 *!@[GL_READ_BUFFER] are outside the read window associated with the current
 *!rendering context, then the values obtained for those pixels are undefined.
 *!
 *!No change is made to the @i{internalformat@}, @i{width@},
 *!or @i{border@} parameters of the specified texture
 *!array or to texel values outside the specified subregion.
 *!
 *!@param target
 *!
 *!Specifies the target texture.
 *!Must be @[GL_TEXTURE_1D].
 *!
 *!@param level
 *!
 *!Specifies the level-of-detail number.
 *!Level 0 is the base image level.
 *!Level @i{n@} is the @i{n@}th mipmap reduction image.
 *!
 *!@param xoffset
 *!
 *!Specifies the texel offset within the texture array.
 *!
 *!@param x
 *!
 *!Specify the window coordinates of the left corner
 *!of the row of pixels to be copied.
 *!
 *!@param width
 *!
 *!Specifies the width of the texture subimage.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{target@} is not @[GL_TEXTURE_1D].
 *!
 *!@[GL_INVALID_OPERATION] is generated if the texture array has not
 *!been defined by a previous @[glTexImage1D] or @[glCopyTexImage1D] operation.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *!.P 
 *!@[GL_INVALID_VALUE] may be generated if @i{level@}>log sub 2@i{ max@},
 *!where @i{max@} is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{y@} ~<~ ~-b
 *!or if @i{width@} ~<~ ~-b, where b 
 *!is the border width of the texture array.
 *!
 *!@[GL_INVALID_VALUE] is generated if "xoffset" ~<~ ~-b, or 
 *!("xoffset"~+~"width") ~>~ (w-b),
 *!where w is the @[GL_TEXTURE_WIDTH], and b is the @[GL_TEXTURE_BORDER] 
 *!of the texture image being modified.
 *!Note that w includes twice the border width.
 *!
 *!
 *!
 */

/*!@decl void glCopyTexSubImage2D(int target, int level, int xoffset, int yoffset, int x, int y, int width, int height)
 *!
 *!@[glCopyTexSubImage2D] replaces a rectangular portion of a two-dimensional
 *!texture image with pixels from the current @[GL_READ_BUFFER] (rather
 *!than from main memory, as is the case for @[glTexSubImage2D]).
 *!
 *!The screen-aligned pixel rectangle with lower left corner at
 *!(@i{x@},\ @i{y@}) and with
 *!width @i{width@} and height @i{height@} replaces the portion of the
 *!texture array with x indices @i{xoffset@} through @i{xoffset@}~+~@i{width@}~-~1,
 *!inclusive, and y indices @i{yoffset@} through @i{yoffset@}~+~@i{height@}~-~1,
 *!inclusive, at the mipmap level specified by @i{level@}.
 *!
 *!The pixels in the rectangle are processed exactly as if
 *!@[glCopyPixels] had been called, but the process stops just before
 *!final conversion.
 *!At this point, all pixel component values are clamped to the range [0,\ 1]
 *!and then converted to the texture's internal format for storage in the texel
 *!array.
 *!
 *!The destination rectangle in the texture array may not include any texels
 *!outside the texture array as it was originally specified.
 *!It is not an error to specify a subtexture with zero width or height, but
 *!such a specification has no effect.
 *!
 *!If any of the pixels within the specified rectangle of the current
 *!@[GL_READ_BUFFER] are outside the read window associated with the current
 *!rendering context, then the values obtained for those pixels are undefined.
 *!
 *!No change is made to the @i{internalformat@}, @i{width@},
 *!@i{height@}, or @i{border@} parameters of the specified texture
 *!array or to texel values outside the specified subregion.
 *!
 *!@param target
 *!
 *!Specifies the target texture.
 *!Must be @[GL_TEXTURE_2D] 
 *!
 *!@param level
 *!
 *!Specifies the level-of-detail number.
 *!Level 0 is the base image level.
 *!Level @i{n@} is the @i{n@}th mipmap reduction image.
 *!
 *!@param xoffset
 *!
 *!Specifies a texel offset in the x direction within the texture array.
 *!
 *!@param yoffset
 *!
 *!Specifies a texel offset in the y direction within the texture array.
 *!
 *!@param x
 *!
 *!Specify the window coordinates of the lower left corner
 *!of the rectangular region of pixels to be copied.
 *!
 *!@param width
 *!
 *!Specifies the width of the texture subimage.
 *!
 *!@param height
 *!
 *!Specifies the height of the texture subimage.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{target@} is not @[GL_TEXTURE_2D]. 
 *!
 *!@[GL_INVALID_OPERATION] is generated if the texture array has not
 *!been defined by a previous @[glTexImage2D] or @[glCopyTexImage2D] operation.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *!.P 
 *!@[GL_INVALID_VALUE] may be generated if @i{level@} is greater
 *!than log sub 2 max,
 *!where max is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{x@} ~<~ ~-b or if
 *!@i{y@} ~<~ ~-b, 
 *!where b is the border width of the texture array.
 *!
 *!@[GL_INVALID_VALUE] is generated if "xoffset" ~<~ -b,
 *!(@i{xoffset@}~+~@i{width@})~>~(w ~-~b), 
 *!@i{yoffset@}~<~ ~-b, or 
 *!(@i{yoffset@}~+~@i{height@})~>~(h ~-~b),
 *!where w is the @[GL_TEXTURE_WIDTH], 
 *!h is the  @[GL_TEXTURE_HEIGHT], 
 *!and b is the @[GL_TEXTURE_BORDER]
 *!of the texture image being modified.
 *!Note that w and h
 *!include twice the border width.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glCopyTexSubImage2D] is executed
 *!between the execution of @[glBegin] and the corresponding
 *!execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glDeleteLists(int list, int range)
 *!
 *!@[glDeleteLists] causes a contiguous group of display lists to be deleted.
 *!@i{list@} is the name of the first display list to be deleted,
 *!and @i{range@} is the number of display lists to delete.
 *!All display lists \fId\fP with @i{list@} \(<= \fId\fP \(<= @i{list@} + @i{range@} - 1
 *!are deleted.
 *!
 *!All storage locations allocated to the specified display lists are freed,
 *!and the names are available for reuse at a later time.
 *!Names within the range that do not have an associated display list are ignored.
 *!If @i{range@} is 0, nothing happens.
 *!
 *!@param list
 *!
 *!Specifies the integer name of the first display list to delete.
 *!
 *!@param range
 *!
 *!Specifies the number of display lists to delete.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{range@} is negative.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glDeleteLists]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glDepthMask(int flag)
 *!
 *!@[glDepthMask] specifies whether the depth buffer is enabled for writing.
 *!If @i{flag@} is @[GL_FALSE],
 *!depth buffer writing is disabled.
 *!Otherwise, it is enabled.
 *!Initially, depth buffer writing is enabled.
 *!
 *!@param flag
 *!
 *!Specifies whether the depth buffer is enabled for writing.
 *!If @i{flag@} is @[GL_FALSE],
 *!depth buffer writing is disabled.
 *!Otherwise, it is enabled.
 *!Initially, depth buffer writing is enabled.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glDepthMask]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glDepthRange(float zNear, float zFar)
 *!
 *!After clipping and division by @i{w@},
 *!depth coordinates range from -1 to 1,
 *!corresponding to the near and far clipping planes.
 *!@[glDepthRange] specifies a linear mapping of the normalized depth coordinates
 *!in this range to window depth coordinates.
 *!Regardless of the actual depth buffer implementation,
 *!window coordinate depth values are treated as though they range
 *!from 0 through 1 (like color components).
 *!Thus,
 *!the values accepted by @[glDepthRange] are both clamped to this range
 *!before they are accepted.
 *!
 *!The setting of (0,1) maps the near plane to 0 and
 *!the far plane to 1.
 *!With this mapping,
 *!the depth buffer range is fully utilized.
 *!
 *!@param zNear
 *!
 *!Specifies the mapping of the near clipping plane to window coordinates.
 *!The initial value is 0.
 *!
 *!@param zFar
 *!
 *!Specifies the mapping of the far clipping plane to window coordinates.
 *!The initial value is 1.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glDepthRange]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glDrawArrays(int mode, int first, int count)
 *!
 *!@[glDrawArrays] specifies multiple geometric primitives
 *!with very few subroutine calls. Instead of calling a GL procedure
 *!to pass each individual vertex, normal, texture coordinate, edge
 *!flag, or color, you can prespecify
 *!separate arrays of vertexes, normals, and colors and use them to
 *!construct a sequence of primitives with a single
 *!call to @[glDrawArrays].
 *!
 *!When @[glDrawArrays] is called, it uses @i{count@} sequential elements from each
 *!enabled array to construct a sequence of geometric primitives,
 *!beginning with element @i{first@}. @i{mode@} specifies what kind of
 *!primitives are constructed, and how the array elements
 *!construct those primitives. If @[GL_VERTEX_ARRAY] is not enabled, no
 *!geometric primitives are generated.
 *!
 *!Vertex attributes that are modified by @[glDrawArrays] have an
 *!unspecified value after @[glDrawArrays] returns. For example, if
 *!@[GL_COLOR_ARRAY] is enabled, the value of the current color is
 *!undefined after @[glDrawArrays] executes. Attributes that aren't
 *!modified remain well defined.
 *!
 *!@param mode
 *!
 *!Specifies what kind of primitives to render.
 *!Symbolic constants
 *!@[GL_POINTS],
 *!@[GL_LINE_STRIP],
 *!@[GL_LINE_LOOP],
 *!@[GL_LINES],
 *!@[GL_TRIANGLE_STRIP],
 *!@[GL_TRIANGLE_FAN],
 *!@[GL_TRIANGLES],
 *!@[GL_QUAD_STRIP],
 *!@[GL_QUADS],
 *!and @[GL_POLYGON] are accepted. 
 *!
 *!@param first
 *!
 *!Specifies the starting index in the enabled arrays.
 *!
 *!@param count
 *!
 *!Specifies the number of indices to be rendered.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{count@} is negative.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glDrawArrays] is executed between
 *!the execution of @[glBegin] and the corresponding @[glEnd].
 *!
 *!
 */

/*!@decl void glDrawPixels(object|mapping(string:object) width, object|mapping(string:object) height, object|mapping(string:object) format, object|mapping(string:object) type, object|mapping(string:object) *pixels)
 *!
 *!@[glDrawPixels] reads pixel data from memory and writes it into the frame buffer
 *!relative to the current raster position.
 *!Use @[glRasterPos] to set the current raster position; use
 *!@[glGet] with argument @[GL_CURRENT_RASTER_POSITION]
 *!to query the raster position.
 *!
 *!Several parameters define the encoding of pixel data in memory
 *!and control the processing of the pixel data
 *!before it is placed in the frame buffer.
 *!These parameters are set with four commands:
 *!@[glPixelStore],
 *!@[glPixelTransfer],
 *!@[glPixelMap], and @[glPixelZoom].
 *!This reference page describes the effects on @[glDrawPixels] of many,
 *!but not all, of the parameters specified by these four commands.
 *!
 *!Data is read from @i{pixels@} as a sequence of signed or unsigned bytes,
 *!signed or unsigned shorts,
 *!signed or unsigned integers,
 *!or single-precision floating-point values,
 *!depending on @i{type@}.
 *!Each of these bytes, shorts, integers, or floating-point values is
 *!interpreted as one color or depth component,
 *!or one index,
 *!depending on @i{format@}.
 *!Indices are always treated individually.
 *!Color components are treated as groups of one,
 *!two,
 *!three,
 *!or four values,
 *!again based on @i{format@}.
 *!Both individual indices and groups of components are
 *!referred to as pixels.
 *!If @i{type@} is @[GL_BITMAP],
 *!the data must be unsigned bytes,
 *!and @i{format@} must be either @[GL_COLOR_INDEX] or @[GL_STENCIL_INDEX].
 *!Each unsigned byte is treated as eight 1-bit pixels,
 *!with bit ordering determined by
 *!@[GL_UNPACK_LSB_FIRST] (see @[glPixelStore]).
 *!
 *!@i{width@}times@i{height@} pixels are read from memory,
 *!starting at location @i{pixels@}.
 *!By default, these pixels are taken from adjacent memory locations,
 *!except that after all @i{width@} pixels are read,
 *!the read pointer is advanced to the next four-byte boundary.
 *!The four-byte row alignment is specified by @[glPixelStore] with
 *!argument @[GL_UNPACK_ALIGNMENT],
 *!and it can be set to one, two, four, or eight bytes.
 *!Other pixel store parameters specify different read pointer advancements,
 *!both before the first pixel is read
 *!and after all @i{width@} pixels are read.
 *!See the 
 *!
 *!@[glPixelStore] reference page for details on these options.
 *!
 *!The @i{width@}times@i{height@} pixels that are read from memory are
 *!each operated on in the same way,
 *!based on the values of several parameters specified by @[glPixelTransfer]
 *!and @[glPixelMap].
 *!The details of these operations,
 *!as well as the target buffer into which the pixels are drawn,
 *!are specific to the format of the pixels,
 *!as specified by @i{format@}.
 *!@i{format@} can assume one of eleven symbolic values:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_COLOR_INDEX</ref>
 *!</c><c>Each pixel is a single value,
 *!a color index.
 *!It is converted to fixed-point format,
 *!with an unspecified number of bits to the right of the binary point,
 *!regardless of the memory data type.
 *!Floating-point values convert to true fixed-point values.
 *!Signed and unsigned integer data is converted with all fraction bits
 *!set to 0.
 *!Bitmap data convert to either 0 or 1.
 *!
 *!Each fixed-point index is then shifted left by <ref>GL_INDEX_SHIFT</ref> bits
 *!and added to <ref>GL_INDEX_OFFSET</ref>.
 *!If <ref>GL_INDEX_SHIFT</ref> is negative,
 *!the shift is to the right.
 *!In either case, zero bits fill otherwise unspecified bit locations in the
 *!result.
 *!
 *!If the GL is in RGBA mode,
 *!the resulting index is converted to an RGBA pixel 
 *!with the help of the <ref>GL_PIXEL_MAP_I_TO_R</ref>, 
 *!<ref>GL_PIXEL_MAP_I_TO_G</ref>,
 *!<ref>GL_PIXEL_MAP_I_TO_B</ref>,
 *!and <ref>GL_PIXEL_MAP_I_TO_A</ref> tables.
 *!If the GL is in color index mode,
 *!and if <ref>GL_MAP_COLOR</ref> is true,
 *!the index is replaced with the value that it references in lookup table
 *!<ref>GL_PIXEL_MAP_I_TO_I</ref>.
 *!Whether the lookup replacement of the index is done or not,
 *!the integer part of the index is then ANDed with 2 sup b -1,
 *!where b is the number of bits in a color index buffer.
 *!
 *!The GL then converts the resulting indices or RGBA colors to fragments
 *!by attaching the current raster position <i>z</i> coordinate and
 *!texture coordinates to each pixel,
 *!then assigning x and y window coordinates to the nth fragment such that
 *!.sp
 *!.RS
 *!.ce
 *!x sub n ~=~ x sub r ~+~ n ~ roman mod ~ "width" 
 *!.sp 
 *!.ce
 *!y sub n ~=~ y sub r ~+~ \(lf ~ n / "width" ~ \(rf
 *!.ce 0
 *!.sp
 *!.RE
 *!
 *!where (x sub r , y sub r) is the current raster position.
 *!These pixel fragments are then treated just like the fragments generated by
 *!rasterizing points, lines, or polygons.
 *!Texture mapping,
 *!fog,
 *!and all the fragment operations are applied before the fragments are written
 *!to the frame buffer.
 *!</c></r>
 *!<r><c><ref>GL_STENCIL_INDEX</ref>
 *!</c><c>Each pixel is a single value,
 *!a stencil index.
 *!It is converted to fixed-point format,
 *!with an unspecified number of bits to the right of the binary point,
 *!regardless of the memory data type.
 *!Floating-point values convert to true fixed-point values.
 *!Signed and unsigned integer data is converted with all fraction bits
 *!set to 0.
 *!Bitmap data convert to either 0 or 1.
 *!
 *!Each fixed-point index is then shifted left by <ref>GL_INDEX_SHIFT</ref> bits,
 *!and added to <ref>GL_INDEX_OFFSET</ref>.
 *!If <ref>GL_INDEX_SHIFT</ref> is negative,
 *!the shift is to the right.
 *!In either case, zero bits fill otherwise unspecified bit locations in the
 *!result.
 *!If <ref>GL_MAP_STENCIL</ref> is true,
 *!the index is replaced with the value that it references in lookup table
 *!<ref>GL_PIXEL_MAP_S_TO_S</ref>.
 *!Whether the lookup replacement of the index is done or not,
 *!the integer part of the index is then ANDed with 2 sup b -1,
 *!where b is the number of bits in the stencil buffer.
 *!The resulting stencil indices are then written to the stencil buffer
 *!such that the nth index is written to location
 *!</c></r>
 *!</matrix>@}
 *!
 *!.RS
 *!.ce
 *!x sub n ~=~ x sub r ~+~ n ~ roman mod ~ "width" 
 *!.sp
 *!.ce
 *!y sub n ~=~ y sub r ~+~ \(lf ~ n / "width" ~ \(rf
 *!.fi
 *!.sp
 *!.RE
 *!
 *!where (x sub r , y sub r) is the current raster position.
 *!Only the pixel ownership test,
 *!the scissor test,
 *!and the stencil writemask affect these write operations.
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_DEPTH_COMPONENT</ref>
 *!</c><c>Each pixel is a single-depth component.
 *!Floating-point data is converted directly to an internal floating-point
 *!format with unspecified precision.
 *!Signed integer data is mapped linearly to the internal floating-point
 *!format such that the most positive representable integer value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Unsigned integer data is mapped similarly:
 *!the largest integer value maps to 1.0,
 *!and 0 maps to 0.0.
 *!The resulting floating-point depth value is then multiplied by
 *!by <ref>GL_DEPTH_SCALE</ref> and added to <ref>GL_DEPTH_BIAS</ref>.
 *!The result is clamped to the range [0,1].
 *!
 *!The GL then converts the resulting depth components to fragments
 *!by attaching the current raster position color or color index and
 *!texture coordinates to each pixel,
 *!then assigning x and y window coordinates to the nth fragment such that
 *!</c></r>
 *!</matrix>@}
 *!
 *!.RS
 *!.ce
 *!x sub n ~=~ x sub r ~+~ n ~ roman mod ~ "width" 
 *!.sp
 *!.ce
 *!y sub n ~=~ y sub r ~+~ \(lf ~ n / "width" ~ \(rf
 *!.ce 0
 *!.sp
 *!.RE
 *!
 *!where (x sub r , y sub r) is the current raster position.
 *!These pixel fragments are then treated just like the fragments generated by
 *!rasterizing points, lines, or polygons.
 *!Texture mapping,
 *!fog,
 *!and all the fragment operations are applied before the fragments are written
 *!to the frame buffer.
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_RGBA</ref>
 *!</c><c>Each pixel is a four-component group: for <ref>GL_RGBA</ref>, the red
 *!component is first, followed by green, followed by blue, followed by 
 *!alpha.
 *!Floating-point values are converted directly to an internal floating-point
 *!format with unspecified precision.
 *!Signed integer values are mapped linearly to the internal floating-point
 *!format such that the most positive representable integer value maps to 1.0,
 *!and the most negative representable value maps to -1.0. (Note that
 *!this mapping does not convert 0 precisely to 0.0.)
 *!Unsigned integer data is mapped similarly:
 *!the largest integer value maps to 1.0,
 *!and 0 maps to 0.0.
 *!The resulting floating-point color values are then multiplied
 *!by <ref>GL_c_SCALE</ref> and added to <ref>GL_c_BIAS</ref>,
 *!where <i>c</i> is RED, GREEN, BLUE, and ALPHA
 *!for the respective color components.
 *!The results are clamped to the range [0,1].
 *!
 *!If <ref>GL_MAP_COLOR</ref> is true,
 *!each color component is scaled by the size of lookup table
 *!<ref>GL_PIXEL_MAP_c_TO_c</ref>,
 *!then replaced by the value that it references in that table.
 *!<i>c</i> is R, G, B, or A respectively.
 *!
 *!The GL then converts the resulting RGBA colors to fragments
 *!by attaching the current raster position <i>z</i> coordinate and
 *!texture coordinates to each pixel,
 *!then assigning x and y window coordinates to the nth fragment such that
 *!</c></r>
 *!</matrix>@}
 *!
 *!.RS
 *!.ce
 *!x sub n ~=~ x sub r ~+~ n ~ roman mod ~ "width" 
 *!.sp
 *!.ce
 *!y sub n ~=~ y sub r ~+~ \(lf ~ n / "width" ~ \(rf
 *!.ce 0
 *!.sp
 *!.RE
 *!
 *!where (x sub r , y sub r) is the current raster position.
 *!These pixel fragments are then treated just like the fragments generated by
 *!rasterizing points, lines, or polygons.
 *!Texture mapping,
 *!fog,
 *!and all the fragment operations are applied before the fragments are written
 *!to the frame buffer.
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_RED</ref>
 *!</c><c>Each pixel is a single red component.
 *!This component is converted to the internal floating-point format in
 *!the same way the red component of an RGBA pixel is. It is
 *!then converted to an RGBA pixel with green and blue set to 0,
 *!and alpha set to 1.
 *!After this conversion, the pixel is treated as if it had been read
 *!as an RGBA pixel.
 *!</c></r>
 *!<r><c><ref>GL_GREEN</ref>
 *!</c><c>Each pixel is a single green component.
 *!This component is converted to the internal floating-point format in
 *!the same way the green component of an RGBA pixel is.
 *!It is then converted to an RGBA pixel with red and blue set to 0,
 *!and alpha set to 1.
 *!After this conversion, the pixel is treated as if it had been read
 *!as an RGBA pixel.
 *!</c></r>
 *!<r><c><ref>GL_BLUE</ref>
 *!</c><c>Each pixel is a single blue component.
 *!This component is converted to the internal floating-point format in
 *!the same way the blue component of an RGBA pixel is.
 *!It is then converted to an RGBA pixel with red and green set to 0,
 *!and alpha set to 1.
 *!After this conversion, the pixel is treated as if it had been read
 *!as an RGBA pixel.
 *!</c></r>
 *!<r><c><ref>GL_ALPHA</ref>
 *!</c><c>Each pixel is a single alpha component.
 *!This component is converted to the internal floating-point format in
 *!the same way the alpha component of an RGBA pixel is.
 *!It is then converted to an RGBA pixel with red, green, and blue set to 0.
 *!After this conversion, the pixel is treated as if it had been read
 *!as an RGBA pixel.
 *!</c></r>
 *!<r><c><ref>GL_RGB</ref>
 *!</c><c>Each pixel is a three-component group:
 *!red first, followed by green, followed by blue.
 *!Each component is converted to the internal floating-point format in
 *!the same way the red, green, and blue components of an RGBA pixel are.
 *!The color triple is converted to an RGBA pixel with alpha set to 1.
 *!After this conversion, the pixel is treated as if it had been read
 *!as an RGBA pixel.
 *!</c></r>
 *!<r><c><ref>GL_LUMINANCE</ref>
 *!</c><c>Each pixel is a single luminance component.
 *!This component is converted to the internal floating-point format in
 *!the same way the red component of an RGBA pixel is.
 *!It is then converted to an RGBA pixel with red, green, and blue set to the
 *!converted luminance value,
 *!and alpha set to 1.
 *!After this conversion, the pixel is treated as if it had been read
 *!as an RGBA pixel.
 *!</c></r>
 *!<r><c><ref>GL_LUMINANCE_ALPHA</ref>
 *!</c><c>Each pixel is a two-component group:
 *!luminance first, followed by alpha.
 *!The two components are converted to the internal floating-point format in
 *!the same way the red component of an RGBA pixel is.
 *!They are then converted to an RGBA pixel with red, green, and blue set to the
 *!converted luminance value,
 *!and alpha set to the converted alpha value.
 *!After this conversion, the pixel is treated as if it had been read
 *!as an RGBA pixel.
 *!</c></r>
 *!</matrix>@}
 *!
 *!The following table summarizes the meaning of the valid constants for the
 *!@i{type@} parameter:
 *!.sp 2
 *!.TS
 *!center box ;
 *!ci | ci
 *!c | c .
 *!type	corresponding type
 *!=
 *!GL_UNSIGNED_BYTE	unsigned 8-bit integer
 *!GL_BYTE	signed 8-bit integer
 *!GL_BITMAP	single bits in unsigned 8-bit integers
 *!GL_UNSIGNED_SHORT	unsigned 16-bit integer
 *!GL_SHORT	signed 16-bit integer
 *!GL_UNSIGNED_INT	unsigned 32-bit integer
 *!GL_INT	32-bit integer
 *!GL_FLOAT	single-precision floating-point 
 *!.TE
 *!.sp
 *!
 *!The rasterization described so far assumes pixel zoom factors of 1.
 *!If 
 *!
 *!@[glPixelZoom] is used to change the x and y pixel zoom factors,
 *!pixels are converted to fragments as follows.
 *!If (x sub r, y sub r) is the current raster position,
 *!and a given pixel is in the nth column and mth row
 *!of the pixel rectangle,
 *!then fragments are generated for pixels whose centers are in the rectangle
 *!with corners at
 *!.sp
 *!.RS
 *!.ce
 *!(x sub r + zoom sub x n, y sub r + zoom sub y m) 
 *!.sp
 *!.ce
 *!(x sub r + zoom sub x (n + 1), y sub r + zoom sub y ( m + 1 ))
 *!.ce 0
 *!.sp
 *!.RE
 *!
 *!where zoom sub x is the value of @[GL_ZOOM_X] and 
 *!zoom sub y is the value of @[GL_ZOOM_Y].
 *!
 *!@param width
 *!
 *!Specify the dimensions of the pixel rectangle to be written
 *!into the frame buffer.
 *!
 *!@param format
 *!
 *!Specifies the format of the pixel data.
 *!Symbolic constants
 *!@[GL_COLOR_INDEX],
 *!@[GL_STENCIL_INDEX],
 *!@[GL_DEPTH_COMPONENT],
 *!@[GL_RGBA],
 *!@[GL_RED],
 *!@[GL_GREEN],
 *!@[GL_BLUE],
 *!@[GL_ALPHA],
 *!@[GL_RGB],
 *!@[GL_LUMINANCE], and
 *!@[GL_LUMINANCE_ALPHA] are accepted.
 *!
 *!@param type
 *!
 *!Specifies the data type for @i{pixels@}.
 *!Symbolic constants
 *!@[GL_UNSIGNED_BYTE],
 *!@[GL_BYTE],
 *!@[GL_BITMAP],
 *!@[GL_UNSIGNED_SHORT],
 *!@[GL_SHORT],
 *!@[GL_UNSIGNED_INT],
 *!@[GL_INT], and
 *!@[GL_FLOAT] are accepted.
 *!
 *!@param pixels
 *!
 *!Specifies a pointer to the pixel data.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@} is negative.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{format@} or @i{type@} is not one of
 *!the accepted values.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @i{format@} is
 *!@[GL_RED],
 *!@[GL_GREEN],
 *!@[GL_BLUE],
 *!@[GL_ALPHA],
 *!@[GL_RGB],
 *!@[GL_RGBA],
 *!@[GL_LUMINANCE],
 *!or
 *!@[GL_LUMINANCE_ALPHA],
 *!and the GL is in color index mode.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *!@i{format@} is not either @[GL_COLOR_INDEX] or @[GL_STENCIL_INDEX].
 *!
 *!@[GL_INVALID_OPERATION] is generated if @i{format@} is @[GL_STENCIL_INDEX]
 *!and there is no stencil buffer.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glDrawPixels]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glEdgeFlag(int flag)
 *!
 *!Each vertex of a polygon,
 *!separate triangle,
 *!or separate quadrilateral specified between a @[glBegin]/@[glEnd] pair
 *!is marked as the start of either a boundary or nonboundary edge.
 *!If the current edge flag is true when the vertex is specified,
 *!the vertex is marked as the start of a boundary edge.
 *!Otherwise, the vertex is marked as the start of a nonboundary edge.
 *!@[glEdgeFlag] sets the edge flag bit to @[GL_TRUE] if @i{flag@} is @[GL_TRUE],
 *!and to @[GL_FALSE] otherwise.
 *!
 *!The vertices of connected triangles and connected quadrilaterals are always
 *!marked as boundary,
 *!regardless of the value of the edge flag.
 *!
 *!Boundary and nonboundary edge flags on vertices are significant only if
 *!@[GL_POLYGON_MODE] is set to @[GL_POINT] or @[GL_LINE].
 *!See @[glPolygonMode].
 *!
 *!@param flag
 *!
 *!Specifies the current edge flag value,
 *!either @[GL_TRUE] or @[GL_FALSE]. The initial value is @[GL_TRUE].
 *!
 *!
 *!@param flag
 *!
 *!Specifies a pointer to an array that contains a single boolean element,
 *!which replaces the current edge flag value.
 *!
 *!
 */

/*!@decl void glEvalCoord(float|int|array(float|int) u, float|int|void v)
 *!
 *!@[glEvalCoord] evaluates enabled one-dimensional maps at argument
 *!@[u] or two-dimensional maps using two domain values,
 *!@[u] and @[v].
 *!To define a map, call @[glMap1] and @[glMap2]; to enable and
 *!disable it, call @[glEnable] and @[glDisable].
 *!
 *!When one of the @[glEvalCoord] commands is issued,
 *!all currently enabled maps of the indicated dimension are evaluated.
 *!Then,
 *!for each enabled map,
 *!it is as if the corresponding GL command had been issued with the
 *!computed value.
 *!That is,
 *!if @[GL_MAP1_INDEX] or
 *!@[GL_MAP2_INDEX] is enabled,
 *!a @[glIndex] command is simulated.
 *!If @[GL_MAP1_COLOR_4] or
 *!@[GL_MAP2_COLOR_4] is enabled,
 *!a @[glColor] command is simulated.
 *!If @[GL_MAP1_NORMAL] or @[GL_MAP2_NORMAL] is enabled,
 *!a normal vector is produced,
 *!and if any of
 *!@[GL_MAP1_TEXTURE_COORD_1],
 *!@[GL_MAP1_TEXTURE_COORD_2],
 *!@[GL_MAP1_TEXTURE_COORD_3], 
 *!@[GL_MAP1_TEXTURE_COORD_4],
 *!@[GL_MAP2_TEXTURE_COORD_1],
 *!@[GL_MAP2_TEXTURE_COORD_2],
 *!@[GL_MAP2_TEXTURE_COORD_3], or
 *!@[GL_MAP2_TEXTURE_COORD_4] is enabled, then an appropriate @[glTexCoord] command is simulated.
 *!
 *!For color,
 *!color index,
 *!normal,
 *!and texture coordinates the GL uses evaluated values instead of current values for those evaluations
 *!that are enabled,
 *!and current values otherwise,
 *!However,
 *!the evaluated values do not update the current values.
 *!Thus, if @[glVertex] commands are interspersed with @[glEvalCoord]
 *!commands, the color,
 *!normal,
 *!and texture coordinates associated with the @[glVertex] commands are not
 *!affected by the values generated by the @[glEvalCoord] commands,
 *!but only by the most recent
 *!@[glColor],
 *!@[glIndex],
 *!@[glNormal], and
 *!@[glTexCoord] commands.
 *!
 *!No commands are issued for maps that are not enabled.
 *!If more than one texture evaluation is enabled for a particular dimension
 *!(for example, @[GL_MAP2_TEXTURE_COORD_1] and
 *!@[GL_MAP2_TEXTURE_COORD_2]),
 *!then only the evaluation of the map that produces the larger
 *!number of coordinates
 *!(in this case, @[GL_MAP2_TEXTURE_COORD_2])
 *!is carried out.
 *!@[GL_MAP1_VERTEX_4] overrides @[GL_MAP1_VERTEX_3],
 *!and
 *!@[GL_MAP2_VERTEX_4] overrides @[GL_MAP2_VERTEX_3],
 *!in the same manner.
 *!If neither a three- nor a four-component vertex map is enabled for the
 *!specified dimension,
 *!the @[glEvalCoord] command is ignored.
 *!
 *!If you have enabled automatic normal generation,
 *!by calling @[glEnable] with argument @[GL_AUTO_NORMAL],
 *!@[glEvalCoord] generates surface normals analytically,
 *!regardless of the contents or enabling of the @[GL_MAP2_NORMAL] map.
 *!Let
 *!.sp
 *!.nf
 *!    Pp   Pp
 *!m = -- X --
 *!    Pu   Pv
 *!.sp 
 *!.fi
 *!
 *!Then the generated normal  n  is 
 *!
 *!
 *!n = m over ~ over { ||  m || }
 *!
 *!.sp
 *!
 *!If automatic normal generation is disabled,
 *!the corresponding normal map @[GL_MAP2_NORMAL],
 *!if enabled,
 *!is used to produce a normal.
 *!If neither automatic normal generation nor a normal map is enabled,
 *!no normal is generated for 
 *!@[glEvalCoord] commands.
 *!
 *!@param u
 *!
 *!Specifies a value that is the domain coordinate u to the basis function
 *!defined in a previous @[glMap1] or @[glMap2] command.
 *!
 *!@param v
 *!
 *!Specifies a value that is the domain coordinate v to the basis function
 *!defined in a previous @[glMap2] command.
 *!
 *!
 *!@param u
 *!
 *!Specifies a pointer to an array containing
 *!either one or two domain coordinates.
 *!The first coordinate is u.
 *!The second coordinate is v.
 *!
 *!
 */

/*!@decl void glEvalPoint(int|array(int) i, int|void j)
 *!
 *!@[glMapGrid] and @[glEvalMesh] are used in tandem to efficiently
 *!generate and evaluate a series of evenly spaced map domain values.
 *!@[glEvalPoint] can be used to evaluate a single grid point in the same gridspace
 *!that is traversed by @[glEvalMesh].
 *!Calling @[glEvalPoint] is equivalent to calling
 *!.nf
 *!
 *!glEvalCoord1(i . DELTA u + u );
 *!                            1
 *!where 
 *!
 *!DELTA u = (u - u  ) / n
 *!            2   1 
 *!
 *!and n, u , and u
 *!        1       2
 *!
 *!.fi
 *!are the arguments to the most recent @[glMapGrid1] command.
 *!The one absolute numeric requirement is that if i~=~n,
 *!then the value computed from 
 *!.nf
 *!i . DELTA u + u  is exactly u .
 *!               1             2
 *!
 *!.fi
 *!
 *!In the two-dimensional case,
 *!@[glEvalPoint],
 *!let 
 *!.nf
 *!DELTA u = (u  - u )/n
 *!            2    1
 *!
 *!DELTA v = (v  - v )/m
 *!            2    1
 *!
 *!where n, u , u , m, v , and v 
 *!          1   2      1       2
 *!
 *!.fi
 *!are the arguments to the most recent @[glMapGrid2] command.
 *!Then the @[glEvalPoint] command is equivalent to calling
 *!.nf
 *!
 *!glEvalCoord2(i .  DELTA u + u , j . DELTA v + v );
 *!                             1                 1
 *!
 *!.fi
 *!The only absolute numeric requirements are that if i~=~n,
 *!then the value computed from 
 *!.nf
 *!
 *!i . DELTA u + u  is exactly u ,
 *!               1             2
 *!.fi
 *!and if j~=~m,
 *!then the value computed from 
 *!.nf
 *!
 *!j cdot DELTA v + v  is exactly v .
 *!                  1             2
 *!
 *!@param i
 *!
 *!Specifies the integer value for grid domain variable i.
 *!
 *!@param j
 *!
 *!Specifies the integer value for grid domain variable j
 *!(@[glEvalPoint] only).
 *!
 *!
 */

/*!@decl void glFog(int pname, float|int|array(float|int) param)
 *!
 *!Fog is initially disabled.
 *!While enabled, fog affects rasterized geometry,
 *!bitmaps, and pixel blocks, but not buffer clear operations. To enable
 *!and disable fog, call @[glEnable] and @[glDisable] with argument
 *!@[GL_FOG]. 
 *!
 *!@[glFog] assigns the value or values in @i{params@} to the fog parameter
 *!specified by @i{pname@}.
 *!The following values are accepted for @i{pname@}:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_FOG_MODE</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies
 *!the equation to be used to compute the fog blend factor, f.
 *!Three symbolic constants are accepted:
 *!<ref>GL_LINEAR</ref>,
 *!<ref>GL_EXP</ref>,
 *!and <ref>GL_EXP2</ref>.
 *!The equations corresponding to these symbolic constants are defined below.
 *!The initial fog mode is <ref>GL_EXP</ref>.
 *!</c></r>
 *!<r><c><ref>GL_FOG_DENSITY</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies density,
 *!the fog density used in both exponential fog equations.
 *!Only nonnegative densities are accepted.
 *!The initial fog density is 1.
 *!</c></r>
 *!<r><c><ref>GL_FOG_START</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies start,
 *!the near distance used in the linear fog equation.
 *!The initial near distance is 0.
 *!</c></r>
 *!<r><c><ref>GL_FOG_END</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies end,
 *!the far distance used in the linear fog equation.
 *!The initial far distance is 1.
 *!</c></r>
 *!<r><c><ref>GL_FOG_INDEX</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies
 *!i sub f,
 *!the fog color index.
 *!The initial fog index is 0.
 *!</c></r>
 *!<r><c><ref>GL_FOG_COLOR</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!C sub f, the fog color.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!After conversion,
 *!all color components are clamped to the range [0,1].
 *!The initial fog color is (0, 0, 0, 0).
 *!</c></r>
 *!</matrix>@}
 *!
 *!Fog blends a fog color with each rasterized pixel fragment's posttexturing
 *!color using a blending factor f.
 *!Factor f is computed in one of three ways,
 *!depending on the fog mode.
 *!Let z be the distance in eye coordinates from the origin to the fragment
 *!being fogged.
 *!The equation for @[GL_LINEAR] fog is
 *!.ce
 *!
 *!.EQ
 *!f ~=~ {end ~-~ z} over {end ~-~ start}
 *!.EN
 *!
 *!.RE
 *!
 *!The equation for @[GL_EXP] fog is
 *!.ce
 *!
 *!.EQ
 *!f ~=~ e ** (-(density ~cdot~ z))
 *!.EN
 *!
 *!
 *!The equation for @[GL_EXP2] fog is
 *!.ce
 *!
 *!.EQ
 *!f ~=~ e ** (-(density ~cdot~ z) ** 2)
 *!.EN
 *!
 *!
 *!Regardless of the fog mode,
 *!f is clamped to the range [0,1] after it is computed.
 *!Then,
 *!if the GL is in RGBA color mode,
 *!the fragment's color C sub r is replaced by
 *!.sp
 *!.ce
 *!.EQ
 *!{C sub r} prime ~=~ f C sub r + (1 - f) C sub f 
 *!.EN
 *!
 *!In color index mode, the fragment's color index i sub r is replaced by
 *!.sp
 *!.ce
 *!.EQ
 *!{i sub r} prime ~=~ i sub r + (1 - f) i sub f 
 *!.EN
 *!
 *!
 *!@param pname
 *!
 *!Specifies a single-valued fog parameter.
 *!@[GL_FOG_MODE],
 *!@[GL_FOG_DENSITY],
 *!@[GL_FOG_START],
 *!@[GL_FOG_END],
 *!and
 *!@[GL_FOG_INDEX]
 *!are accepted.
 *!
 *!@param param
 *!
 *!Specifies the value that @i{pname@} will be set to.
 *!
 *!
 *!@param pname
 *!
 *!Specifies a fog parameter.
 *!@[GL_FOG_MODE],
 *!@[GL_FOG_DENSITY],
 *!@[GL_FOG_START],
 *!@[GL_FOG_END],
 *!@[GL_FOG_INDEX],
 *!and
 *!@[GL_FOG_COLOR]
 *!are accepted.
 *!
 *!@param params
 *!
 *!Specifies the value or values to be assigned to @i{pname@}.
 *!@[GL_FOG_COLOR] requires an array of four values.
 *!All other parameters accept an array containing only a single value.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{pname@} is not an accepted value,
 *!or if @i{pname@} is @[GL_FOG_MODE] and @i{params@} is not an accepted value.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{pname@} is @[GL_FOG_DENSITY], 
 *!and @i{params@} is negative.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glFog]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glFrustum(float left, float right, float bottom, float top, float zNear, float zFar)
 *!
 *!@[glFrustum] describes a perspective matrix that produces a perspective projection.
 *!The current matrix (see @[glMatrixMode]) is multiplied by this matrix
 *!and the result replaces the current matrix, as if
 *!@[glMultMatrix] were called with the following matrix
 *!as its argument:
 *!
 *!.sp 5
 *!.ce
 *!.EQ
 *!down 130 {left ( ~~ matrix {
 *!   ccol { {{2 ~ "zNear"} over {"right" - "left"}} above 0 above 0 above 0 }
 *!   ccol { 0 above {{2 ~ "zNear"} over {"top" - "bottom"}} ~ above 0 above 0 }
 *!   ccol { A ~~~~ above B ~~~~ above C ~~~~ above -1 ~~~~}
 *!   ccol { 0 above 0 above D above 0}
 *!}  ~~~ right )}
 *!.EN
 *!.sp
 *!.ce
 *!.EQ
 *!down 130
 *!{A ~=~ {"right" + "left"} over {"right" - "left"}}
 *!.EN
 *!.sp
 *!.ce
 *!.EQ
 *!down 130
 *!{B ~=~ {"top" + "bottom"} over {"top" - "bottom"}}
 *!.EN
 *!.sp
 *!.ce
 *!.EQ
 *!down 130
 *!{C ~=~ -{{"zFar" + "zNear"} over {"zFar" - "zNear"}}}
 *!.EN
 *!.sp
 *!.ce
 *!.EQ
 *!down 130
 *!{D ~=~ -{{2 ~ "zFar" ~ "zNear"} over {"zFar" - "zNear"}}}
 *!.EN
 *!.sp
 *!
 *!Typically, the matrix mode is @[GL_PROJECTION], and
 *!(@i{left@}, @i{bottom@}, -@i{zNear@}) and (@i{right@}, @i{top@},  -@i{zNear@})
 *!specify the points on the near clipping plane that are mapped
 *!to the lower left and upper right corners of the window,
 *!assuming that the eye is located at (0, 0, 0).
 *!-@i{zFar@} specifies the location of the far clipping plane.
 *!Both @i{zNear@} and @i{zFar@} must be positive.
 *!
 *!Use @[glPushMatrix] and @[glPopMatrix] to save and restore
 *!the current matrix stack.
 *!
 *!@param left
 *!
 *!Specify the coordinates for the left and right vertical clipping planes.
 *!
 *!@param bottom
 *!
 *!Specify the coordinates for the bottom and top horizontal clipping planes.
 *!
 *!@param zNear
 *!
 *!Specify the distances to the near and far depth clipping planes.
 *!Both distances must be positive.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{zNear@} or @i{zFar@} is not positive.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glFrustum]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl int glGenLists(int range)
 *!
 *!@[glGenLists] has one argument, @i{range@}.
 *!It returns an integer @i{n@} such that @i{range@} contiguous
 *!empty display lists,
 *!named @i{n@}, @i{n@}+1, ..., @i{n@}+@i{range@} -1,
 *!are created.
 *!If @i{range@} is 0,
 *!if there is no group of @i{range@} contiguous names available,
 *!or if any error is generated,
 *!no display lists are generated,
 *!and 0 is returned.
 *!
 *!@param range
 *!
 *!Specifies the number of contiguous empty display lists
 *!to be generated.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{range@} is negative.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glGenLists]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl int glGetError()
 *!
 *!@[glGetError] returns the value of the error flag.
 *!Each detectable error is assigned a numeric code and symbolic name.
 *!When an error occurs,
 *!the error flag is set to the appropriate error code value.
 *!No other errors are recorded until @[glGetError] is called,
 *!the error code is returned,
 *!and the flag is reset to @[GL_NO_ERROR].
 *!If a call to @[glGetError] returns @[GL_NO_ERROR],
 *!there has been no detectable error since the last call to @[glGetError],
 *!or since the GL was initialized.
 *!
 *!To allow for distributed implementations,
 *!there may be several error flags.
 *!If any single error flag has recorded an error,
 *!the value of that flag is returned
 *!and that flag is reset to @[GL_NO_ERROR]
 *!when @[glGetError] is called.
 *!If more than one flag has recorded an error,
 *!@[glGetError] returns and clears an arbitrary error flag value.
 *!Thus, @[glGetError] should always be called in a loop,
 *!until it returns @[GL_NO_ERROR],
 *!if all error flags are to be reset.
 *!
 *!Initially, all error flags are set to @[GL_NO_ERROR].
 *!
 *!The following errors are currently defined:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_NO_ERROR</ref>
 *!</c><c>No error has been recorded.
 *!The value of this symbolic constant is guaranteed to be 0.
 *!</c></r>
 *!<r><c><ref>GL_INVALID_ENUM</ref>
 *!</c><c>An unacceptable value is specified for an enumerated argument.
 *!The offending command is ignored,
 *!and has no other side effect than to set the error flag.
 *!</c></r>
 *!<r><c><ref>GL_INVALID_VALUE</ref>
 *!</c><c>A numeric argument is out of range.
 *!The offending command is ignored,
 *!and has no other side effect than to set the error flag.
 *!</c></r>
 *!<r><c><ref>GL_INVALID_OPERATION</ref>
 *!</c><c>The specified operation is not allowed in the current state.
 *!The offending command is ignored,
 *!and has no other side effect than to set the error flag.
 *!</c></r>
 *!<r><c><ref>GL_STACK_OVERFLOW</ref>
 *!</c><c>This command would cause a stack overflow.
 *!The offending command is ignored,
 *!and has no other side effect than to set the error flag.
 *!</c></r>
 *!<r><c><ref>GL_STACK_UNDERFLOW</ref>
 *!</c><c>This command would cause a stack underflow.
 *!The offending command is ignored,
 *!and has no other side effect than to set the error flag.
 *!</c></r>
 *!<r><c><ref>GL_OUT_OF_MEMORY</ref>
 *!</c><c>There is not enough memory left to execute the command.
 *!The state of the GL is undefined,
 *!except for the state of the error flags,
 *!after this error is recorded.
 *!</c></r>
 *!</matrix>@}
 *!
 *!When an error flag is set,
 *!results of a GL operation are undefined only if @[GL_OUT_OF_MEMORY]
 *!has occurred.
 *!In all other cases,
 *!the command generating the error is ignored and has no effect on the GL state
 *!or frame buffer contents.
 *!If the generating command returns a value, it returns 0.  
 *!If @[glGetError] itself generates an error, it returns 0. 
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glGetError]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!In this case @[glGetError] returns 0.
 *!
 *!
 *!
 */

/*!@decl string glGetString(int name)
 *!
 *!@[glGetString] returns a pointer to a static string
 *!describing some aspect of the current GL connection.
 *!@i{name@} can be one of the following:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_VENDOR</ref>
 *!</c><c>Returns the company responsible for this GL implementation.
 *!This name does not change from release to release.
 *!</c></r>
 *!<r><c><ref>GL_RENDERER</ref>
 *!</c><c>Returns the name of the renderer.
 *!This name is typically specific to a particular configuration of a hardware
 *!platform.
 *!It does not change from release to release.
 *!</c></r>
 *!<r><c><ref>GL_VERSION</ref>
 *!</c><c>Returns a version or release number.
 *!</c></r>
 *!<r><c><ref>GL_EXTENSIONS</ref>
 *!</c><c>Returns a space-separated list of supported extensions to GL.
 *!</c></r>
 *!</matrix>@}
 *!
 *!Because the GL does not include queries for the performance
 *!characteristics of an implementation, some applications are written to
 *!recognize known platforms and modify their GL usage based on known
 *!performance characteristics of these platforms.
 *!Strings @[GL_VENDOR] and @[GL_RENDERER] together uniquely specify
 *!a platform. They do not change from release to release and should be used
 *!by platform-recognition algorithms. 
 *!
 *!Some applications want to make use of features that
 *!are not part of the standard GL. These features
 *!may be implemented as extensions to the standard GL.
 *!The @[GL_EXTENSIONS] string is a space-separated
 *!list of supported GL extensions.
 *!(Extension names never contain a space character.)
 *!
 *!The @[GL_VERSION] string begins with a version number.
 *!The version number uses one
 *!of these forms: 
 *!
 *!@i{major_number.minor_number@}  
 *!
 *!@i{major_number.minor_number.release_number@}
 *!
 *!Vendor-specific information may follow the version
 *!number. Its format depends on the implementation, but 
 *!a space always separates the version number and 
 *!the vendor-specific information.
 *!
 *!All strings are null-terminated.
 *!
 *!@param name
 *!
 *!Specifies a symbolic constant, one of 
 *!@[GL_VENDOR], @[GL_RENDERER], @[GL_VERSION], or @[GL_EXTENSIONS].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{name@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glGetString]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 *!
 */

/*!@decl void glHint(int target, int mode)
 *!
 *!Certain aspects of GL behavior,
 *!when there is room for interpretation,
 *!can be controlled with hints.
 *!A hint is specified with two arguments.
 *!@i{target@} is a symbolic 
 *!constant indicating the behavior to be controlled,
 *!and @i{mode@} is another symbolic constant indicating the desired
 *!behavior. The initial value for each @i{target@} is @[GL_DONT_CARE]. 
 *!@i{mode@} can be one of the following:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_FASTEST</ref>
 *!</c><c>The most efficient option should be chosen.
 *!</c></r>
 *!<r><c><ref>GL_NICEST</ref>
 *!</c><c>The most correct,
 *!or highest quality,
 *!option should be chosen.
 *!</c></r>
 *!<r><c><ref>GL_DONT_CARE</ref>
 *!</c><c>No preference.
 *!</c></r>
 *!</matrix>@}
 *!
 *!Though the implementation aspects that can be hinted are well defined,
 *!the interpretation of the hints depends on the implementation.
 *!The hint aspects that can be specified with @i{target@},
 *!along with suggested semantics,
 *!are as follows:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_FOG_HINT</ref>
 *!</c><c>Indicates the accuracy of fog calculation.
 *!If per-pixel fog calculation is not efficiently supported
 *!by the GL implementation,
 *!hinting <ref>GL_DONT_CARE</ref> or <ref>GL_FASTEST</ref> can result in per-vertex
 *!calculation of fog effects.
 *!</c></r>
 *!<r><c><ref>GL_LINE_SMOOTH_HINT</ref>
 *!</c><c>Indicates the sampling quality of antialiased lines.
 *!If a larger filter function is applied, hinting <ref>GL_NICEST</ref> can
 *!result in more pixel fragments being generated during rasterization,
 *!</c></r>
 *!<r><c><ref>GL_PERSPECTIVE_CORRECTION_HINT</ref>
 *!</c><c>Indicates the quality of color and texture coordinate interpolation.
 *!If perspective-corrected parameter interpolation is not efficiently supported
 *!by the GL implementation,
 *!hinting <ref>GL_DONT_CARE</ref> or <ref>GL_FASTEST</ref> can result in simple linear
 *!interpolation of colors and/or texture coordinates.
 *!</c></r>
 *!<r><c><ref>GL_POINT_SMOOTH_HINT</ref>
 *!</c><c>Indicates the sampling quality of antialiased points.
 *!If a larger filter function is applied, hinting <ref>GL_NICEST</ref> can
 *!result in more pixel fragments being generated during rasterization, 
 *!</c></r>
 *!<r><c><ref>GL_POLYGON_SMOOTH_HINT</ref>
 *!</c><c>Indicates the sampling quality of antialiased polygons.
 *!Hinting <ref>GL_NICEST</ref> can result in more pixel fragments being generated
 *!during rasterization,
 *!if a larger filter function is applied.
 *!</c></r></matrix>@}
 *!
 *!
 *!@param target
 *!
 *!Specifies a symbolic constant indicating the behavior to be controlled.
 *!@[GL_FOG_HINT],	
 *!@[GL_LINE_SMOOTH_HINT],
 *!@[GL_PERSPECTIVE_CORRECTION_HINT],
 *!@[GL_POINT_SMOOTH_HINT], and
 *!@[GL_POLYGON_SMOOTH_HINT] are accepted. 
 *!
 *!@param mode
 *!
 *!Specifies a symbolic constant indicating the desired behavior.
 *!@[GL_FASTEST], 
 *!@[GL_NICEST], and 
 *!@[GL_DONT_CARE] are accepted. 
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if either @i{target@} or @i{mode@} is not
 *!an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glHint]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 *!
 */

/*!@decl void glIndex(float|int c)
 *!
 *!@[glIndex] updates the current (single-valued) color index.
 *!It takes one argument, the new value for the current color index.
 *!
 *!The current index is stored as a floating-point value. 
 *!Integer values are converted directly to floating-point values,
 *!with no special mapping.
 *!The initial value is 1. 
 *!
 *!Index values outside the representable range of the color index buffer
 *!are not clamped.
 *!However,
 *!before an index is dithered (if enabled) and written to the frame buffer,
 *!it is converted to fixed-point format.
 *!Any bits in the integer portion of the resulting fixed-point value
 *!that do not correspond to bits in the frame buffer are masked out.
 *!
 *!@param c
 *!
 *!Specifies the new value for the current color index.
 *!
 *!
 *!
 *!@param c
 *!
 *!Specifies a pointer to a one-element array that contains
 *!the new value for the current color index.
 *!
 *!
 */

/*!@decl void glIndexMask(int mask)
 *!
 *!@[glIndexMask] controls the writing of individual bits in the color index buffers.
 *!The least significant n bits of @i{mask@},
 *!where n is the number of bits in a color index buffer,
 *!specify a mask.
 *!Where a 1 (one) appears in the mask,
 *!it's possible to write to the corresponding bit in the color index
 *!buffer (or buffers). 
 *!Where a 0 (zero) appears,
 *!the corresponding bit is write-protected.
 *!
 *!This mask is used only in color index mode,
 *!and it affects only the buffers currently selected for writing
 *!(see @[glDrawBuffer]).
 *!Initially, all bits are enabled for writing.
 *!
 *!@param mask
 *!
 *!Specifies a bit mask to enable and disable the writing of individual bits
 *!in the color index buffers.
 *!Initially, the mask is all 1's.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glIndexMask]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl int glIsEnabled(int cap)
 *!
 *!@[glIsEnabled] returns @[GL_TRUE] if @i{cap@} is an enabled capability
 *!and returns @[GL_FALSE] otherwise.
 *!Initially all capabilities except @[GL_DITHER] are disabled;
 *!@[GL_DITHER] is initially enabled.
 *!
 *!The following capabilities are accepted for @i{cap@}:
 *!
 *!.TS
 *!lb lb
 *!l l l.
 *!Constant	See
 *!_
 *!
 *!@[GL_ALPHA_TEST]	@[glAlphaFunc]
 *!@[GL_AUTO_NORMAL]	@[glEvalCoord]
 *!@[GL_BLEND]	@[glBlendFunc], @[glLogicOp]
 *!@[GL_CLIP_PLANE]@i{i@}	@[glClipPlane]
 *!@[GL_COLOR_ARRAY]	@[glColorPointer]
 *!@[GL_COLOR_LOGIC_OP]	@[glLogicOp]
 *!@[GL_COLOR_MATERIAL]	@[glColorMaterial]
 *!@[GL_CULL_FACE]	@[glCullFace]
 *!@[GL_DEPTH_TEST]	@[glDepthFunc], @[glDepthRange]
 *!@[GL_DITHER]	@[glEnable]
 *!@[GL_EDGE_FLAG_ARRAY]	@[glEdgeFlagPointer]
 *!@[GL_FOG]	@[glFog]
 *!@[GL_INDEX_ARRAY]	@[glIndexPointer]
 *!@[GL_INDEX_LOGIC_OP]	@[glLogicOp]
 *!@[GL_LIGHT]@i{i@}	@[glLightModel], @[glLight]
 *!@[GL_LIGHTING]	@[glMaterial], @[glLightModel], @[glLight]
 *!@[GL_LINE_SMOOTH]	@[glLineWidth]
 *!@[GL_LINE_STIPPLE]	@[glLineStipple]
 *!@[GL_MAP1_COLOR_4]	@[glMap1], @[glMap2]
 *!@[GL_MAP2_TEXTURE_COORD_2]	@[glMap2]
 *!@[GL_MAP2_TEXTURE_COORD_3]	@[glMap2]
 *!@[GL_MAP2_TEXTURE_COORD_4]	@[glMap2]
 *!@[GL_MAP2_VERTEX_3]	@[glMap2]
 *!@[GL_MAP2_VERTEX_4]	@[glMap2]
 *!@[GL_NORMAL_ARRAY]	@[glNormalPointer]
 *!@[GL_NORMALIZE]	@[glNormal]
 *!@[GL_POINT_SMOOTH]	@[glPointSize]
 *!@[GL_POLYGON_SMOOTH]	@[glPolygonMode]
 *!@[GL_POLYGON_OFFSET_FILL] 	@[glPolygonOffset]
 *!@[GL_POLYGON_OFFSET_LINE] 	@[glPolygonOffset]
 *!@[GL_POLYGON_OFFSET_POINT] 	@[glPolygonOffset]
 *!@[GL_POLYGON_STIPPLE]	@[glPolygonStipple]
 *!@[GL_SCISSOR_TEST]	@[glScissor]
 *!@[GL_STENCIL_TEST]	@[glStencilFunc], @[glStencilOp]
 *!@[GL_TEXTURE_1D]	@[glTexImage1D]
 *!@[GL_TEXTURE_2D]	@[glTexImage2D]
 *!@[GL_TEXTURE_COORD_ARRAY]	@[glTexCoordPointer]
 *!@[GL_TEXTURE_GEN_Q]	@[glTexGen]
 *!@[GL_TEXTURE_GEN_R]	@[glTexGen]
 *!@[GL_TEXTURE_GEN_S]	@[glTexGen]
 *!@[GL_TEXTURE_GEN_T]	@[glTexGen]
 *!@[GL_VERTEX_ARRAY]	@[glVertexPointer]
 *!.TE
 *!
 *!@param cap
 *!
 *!Specifies a symbolic constant indicating a GL capability.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{cap@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glIsEnabled]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl int glIsList(int list)
 *!
 *!@[glIsList] returns @[GL_TRUE] if @i{list@} is the name
 *!of a display list and returns @[GL_FALSE] otherwise.
 *!
 *!@param list
 *!
 *!Specifies a potential display-list name.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glIsList]
 *!is executed between the execution of
 *!@[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl int glIsTexture(int texture)
 *!
 *!@[glIsTexture] returns @[GL_TRUE] if @i{texture@} is currently the name of a texture.
 *!If @i{texture@} is zero, or is a non-zero value that is not currently the
 *!name of a texture, or if an error occurs, @[glIsTexture] returns @[GL_FALSE].
 *!
 *!@param texture
 *!
 *!Specifies a value that may be the name of a texture.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glIsTexture] is executed
 *!between the execution of @[glBegin] and the corresponding
 *!execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glLight(int light, int pname, float|int|array(float|int) param)
 *!
 *!@[glLight] sets the values of individual light source parameters.
 *!@i{light@} names the light and is a symbolic name of the form @[GL_LIGHT]i,
 *!where 0 \(<= i < @[GL_MAX_LIGHTS].
 *!@i{pname@} specifies one of ten light source parameters,
 *!again by symbolic name.
 *!@i{params@} is either a single value or a pointer to an array that contains
 *!the new values.
 *!
 *!To enable and disable lighting calculation, call @[glEnable]
 *!and @[glDisable] with argument @[GL_LIGHTING]. Lighting is
 *!initially disabled.
 *!When it is enabled,
 *!light sources that are enabled contribute to the lighting calculation.
 *!Light source i is enabled and disabled using @[glEnable] and
 *!@[glDisable] with argument @[GL_LIGHT]i. 
 *!
 *!The ten light parameters are as follows:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_AMBIENT</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the ambient RGBA intensity of the light.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!The initial ambient light intensity is (0, 0, 0, 1).
 *!</c></r>
 *!<r><c><ref>GL_DIFFUSE</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the diffuse RGBA intensity of the light.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!The initial value
 *!for <ref>GL_LIGHT0</ref> is (1, 1, 1, 1); for other lights, the
 *!initial value is (0, 0, 0, 0). 
 *!</c></r>
 *!<r><c><ref>GL_SPECULAR</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the specular RGBA intensity of the light.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!The initial value
 *!for <ref>GL_LIGHT0</ref> is (1, 1, 1, 1); for other lights, the
 *!initial value is (0, 0, 0, 0). 
 *!</c></r>
 *!<r><c><ref>GL_POSITION</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the position of the light in homogeneous object coordinates.
 *!Both integer and floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!
 *!The position is transformed by the modelview matrix when
 *!<ref>glLight</ref> is called (just as if it were a point),
 *!and it is stored in eye coordinates.
 *!If the w component of the position is 0,
 *!the light is treated as a directional source.
 *!Diffuse and specular lighting calculations take the light's direction,
 *!but not its actual position,
 *!into account,
 *!and attenuation is disabled.
 *!Otherwise,
 *!diffuse and specular lighting calculations are based on the actual location
 *!of the light in eye coordinates,
 *!and attenuation is enabled.
 *!The initial position is (0, 0, 1, 0);
 *!thus, the initial light source is directional,
 *!parallel to, and in the direction of the -z axis.
 *!</c></r>
 *!<r><c><ref>GL_SPOT_DIRECTION</ref>
 *!</c><c><i>params</i> contains three integer or floating-point values that specify
 *!the direction of the light in homogeneous object coordinates.
 *!Both integer and floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!
 *!The spot direction is transformed by the inverse of the modelview matrix when
 *!<ref>glLight</ref> is called (just as if it were a normal),
 *!and it is stored in eye coordinates.
 *!It is significant only when <ref>GL_SPOT_CUTOFF</ref> is not 180,
 *!which it is initially.
 *!The initial direction is (0, 0, -1).
 *!</c></r>
 *!<r><c><ref>GL_SPOT_EXPONENT</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies
 *!the intensity distribution of the light.
 *!Integer and floating-point values are mapped directly.
 *!Only values in the range [0,128] are accepted.
 *!
 *!Effective light intensity is attenuated by the cosine of the angle between
 *!the direction of the light and the direction from the light to the vertex
 *!being lighted,
 *!raised to the power of the spot exponent.
 *!Thus, higher spot exponents result in a more focused light source,
 *!regardless of the spot cutoff angle (see <ref>GL_SPOT_CUTOFF</ref>, next paragraph).
 *!The initial spot exponent is 0,
 *!resulting in uniform light distribution.
 *!</c></r>
 *!<r><c><ref>GL_SPOT_CUTOFF</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies
 *!the maximum spread angle of a light source.
 *!Integer and floating-point values are mapped directly.
 *!Only values in the range [0,90] and the special value 180
 *!are accepted.
 *!If the angle between the direction of the light and the direction from the
 *!light to the vertex being lighted is greater than the spot cutoff angle,
 *!the light is completely masked.
 *!Otherwise, its intensity is controlled by the spot exponent and the
 *!attenuation factors.
 *!The initial spot cutoff is 180,
 *!resulting in uniform light distribution.
 *!</c></r>
 *!<r><c><ref>GL_CONSTANT_ATTENUATION</ref>
 *!</c><c></c></r>
 *!<r><c><ref>GL_LINEAR_ATTENUATION </ref>
 *!</c><c></c></r>
 *!<r><c><ref>GL_QUADRATIC_ATTENUATION</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies
 *!one of the three light attenuation factors.
 *!Integer and floating-point values are mapped directly.
 *!Only nonnegative values are accepted.
 *!If the light is positional,
 *!rather than directional,
 *!its intensity is attenuated by the reciprocal of the sum of the constant
 *!factor, the linear factor times the distance between the light
 *!and the vertex being lighted,
 *!and the quadratic factor times the square of the same distance.
 *!The initial attenuation factors are (1, 0, 0),
 *!resulting in no attenuation.
 *!</c></r></matrix>@}
 *!
 *!
 *!@param light
 *!
 *!Specifies a light.
 *!The number of lights depends on the implementation,
 *!but at least eight lights are supported.
 *!They are identified by symbolic names of the form @[GL_LIGHT]i
 *!where 0 \(<=  i  < @[GL_MAX_LIGHTS].
 *!
 *!@param pname
 *!
 *!Specifies a single-valued light source parameter for @i{light@}.
 *!@[GL_SPOT_EXPONENT],
 *!@[GL_SPOT_CUTOFF],
 *!@[GL_CONSTANT_ATTENUATION],
 *!@[GL_LINEAR_ATTENUATION], and
 *!@[GL_QUADRATIC_ATTENUATION] are accepted.
 *!
 *!@param param
 *!
 *!Specifies the value that parameter @i{pname@} of light source @i{light@}
 *!will be set to.
 *!
 *!
 *!@param light
 *!
 *!Specifies a light.
 *!The number of lights depends on the implementation, but
 *!at least eight lights are supported.
 *!They are identified by symbolic names of the form @[GL_LIGHT]i
 *!where 0 \(<=  i  < @[GL_MAX_LIGHTS].
 *!
 *!@param pname
 *!
 *!Specifies a light source parameter for @i{light@}.
 *!@[GL_AMBIENT],
 *!@[GL_DIFFUSE],
 *!@[GL_SPECULAR],
 *!@[GL_POSITION],
 *!@[GL_SPOT_CUTOFF],
 *!@[GL_SPOT_DIRECTION],
 *!@[GL_SPOT_EXPONENT],
 *!@[GL_CONSTANT_ATTENUATION],
 *!@[GL_LINEAR_ATTENUATION], and
 *!@[GL_QUADRATIC_ATTENUATION] are accepted.
 *!
 *!@param params
 *!
 *!Specifies a pointer to the value or values that parameter @i{pname@}
 *!of light source @i{light@} will be set to.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if either @i{light@} or @i{pname@}
 *!is not an accepted value.
 *!
 *!@[GL_INVALID_VALUE] is generated if a spot exponent value is specified
 *!outside the range [0,128],
 *!or if spot cutoff is specified outside the range [0,90] (except for the
 *!special value 180),
 *!or if a negative attenuation factor is specified.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLight] is executed between
 *!the execution of
 *!@[glBegin] and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glLightModel(int pname, float|int|array(float|int) param)
 *!
 *!@[glLightModel] sets the lighting model parameter.
 *!@i{pname@} names a parameter and @i{params@} gives the new value.
 *!There are three lighting model parameters:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_LIGHT_MODEL_AMBIENT</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the ambient RGBA intensity of the entire scene.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!The initial ambient scene intensity is (0.2, 0.2, 0.2, 1.0).
 *!</c></r>
 *!<r><c><ref>GL_LIGHT_MODEL_LOCAL_VIEWER</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies
 *!how specular reflection angles are computed.
 *!If <i>params</i> is 0 (or 0.0), specular reflection angles take the
 *!view direction to be parallel to and in the direction of the -<i>z</i> axis,
 *!regardless of the location of the vertex in eye coordinates.
 *!Otherwise, specular reflections are computed from the origin
 *!of the eye coordinate system.
 *!The initial value is 0.
 *!</c></r>
 *!<r><c><ref>GL_LIGHT_MODEL_TWO_SIDE</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies
 *!whether one- or two-sided lighting calculations are done for polygons.
 *!It has no effect on the lighting calculations for points,
 *!lines,
 *!or bitmaps.
 *!If <i>params</i> is 0 (or 0.0), one-sided lighting is specified,
 *!and only the <i>front</i> material parameters are used in the
 *!lighting equation.
 *!Otherwise, two-sided lighting is specified.
 *!In this case, vertices of back-facing polygons are lighted using the
 *!<i>back</i> material parameters,
 *!and have their normals reversed before the lighting equation is evaluated.
 *!Vertices of front-facing polygons are always lighted using the
 *!<i>front</i> material parameters,
 *!with no change to their normals. The initial value is 0.
 *!</c></r>
 *!</matrix>@}
 *!
 *!In RGBA mode, the lighted color of a vertex is the sum of
 *!the material emission intensity,
 *!the product of the material ambient reflectance and the lighting model full-scene 
 *!ambient intensity,
 *!and the contribution of each enabled light source.
 *!Each light source contributes the sum of three terms:
 *!ambient, diffuse, and specular.
 *!The ambient light source contribution is the product of the material ambient
 *!reflectance and the light's ambient intensity.
 *!The diffuse light source contribution is the product of the material diffuse
 *!reflectance,
 *!the light's diffuse intensity,
 *!and the dot product of the vertex's normal with the normalized vector from
 *!the vertex to the light source.
 *!The specular light source contribution is the product of the material specular
 *!reflectance,
 *!the light's specular intensity,
 *!and the dot product of the normalized vertex-to-eye and vertex-to-light
 *!vectors,
 *!raised to the power of the shininess of the material.
 *!All three light source contributions are attenuated equally based on
 *!the distance from the vertex to the light source and on light source
 *!direction, spread exponent, and spread cutoff angle.
 *!All dot products are replaced with 0 if they evaluate to a negative value.
 *!
 *!The alpha component of the resulting lighted color is set to the alpha value
 *!of the material diffuse reflectance.
 *!
 *!In color index mode,
 *!the value of the lighted index of a vertex ranges from the ambient
 *!to the specular values passed to @[glMaterial] using @[GL_COLOR_INDEXES].
 *!Diffuse and specular coefficients,
 *!computed with a (.30, .59, .11) weighting of the lights' colors,
 *!the shininess of the material,
 *!and the same reflection and attenuation equations as in the RGBA case,
 *!determine how much above ambient the resulting index is.
 *!
 *!@param pname
 *!
 *!Specifies a single-valued lighting model parameter.
 *!@[GL_LIGHT_MODEL_LOCAL_VIEWER] and
 *!@[GL_LIGHT_MODEL_TWO_SIDE] are accepted.
 *!
 *!@param param
 *!
 *!Specifies the value that @i{param@} will be set to.
 *!
 *!
 *!@param pname
 *!
 *!Specifies a lighting model parameter.
 *!@[GL_LIGHT_MODEL_AMBIENT],
 *!@[GL_LIGHT_MODEL_LOCAL_VIEWER], and
 *!@[GL_LIGHT_MODEL_TWO_SIDE] are accepted.
 *!
 *!@param params
 *!
 *!Specifies a pointer to the value or values that @i{params@} will be set to.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{pname@} is not an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLightModel] is executed between
 *!the execution of @[glBegin] and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glLineStipple(int factor, int pattern)
 *!
 *!Line stippling masks out certain fragments produced by rasterization;
 *!those fragments will not be drawn.
 *!The masking is achieved by using three parameters:
 *!the 16-bit line stipple pattern @i{pattern@},
 *!the repeat count @i{factor@},
 *!and an integer stipple counter s. 
 *!
 *!Counter s is reset to 0 whenever @[glBegin] is called,
 *!and before each line segment of a @[glBegin](@[GL_LINES])/@[glEnd]
 *!sequence is generated.
 *!It is incremented after each fragment of a unit width aliased line segment is generated,
 *!or after each i fragments of an i width line segment are generated.
 *!The i fragments associated with count s are masked out if
 *!.sp
 *!.ce
 *!@i{pattern@} bit (s ~/~ "factor") ~roman mod~ 16 
 *!.sp
 *!is 0, otherwise these fragments are sent to the frame buffer.
 *!Bit zero of @i{pattern@} is the least significant bit.
 *!
 *!Antialiased lines are treated as a sequence of 1 times width rectangles
 *!for purposes of stippling.
 *!Whether rectagle s is rasterized or not depends on the fragment rule
 *!described for aliased lines,
 *!counting rectangles rather than groups of fragments.
 *!
 *!To enable and disable line stippling, call @[glEnable] and @[glDisable]
 *!with argument @[GL_LINE_STIPPLE].
 *!When enabled,
 *!the line stipple pattern is applied as described above.
 *!When disabled,
 *!it is as if the pattern were all 1's.
 *!Initially, line stippling is disabled.
 *!
 *!@param factor
 *!
 *!Specifies a multiplier for each bit in the line stipple pattern.
 *!If @i{factor@} is 3,
 *!for example,
 *!each bit in the pattern is used three times
 *!before the next bit in the pattern is used.
 *!@i{factor@} is clamped to the range [1, 256] and defaults to 1.
 *!
 *!@param pattern
 *!
 *!Specifies a 16-bit integer whose bit pattern determines
 *!which fragments of a line will be drawn when the line is rasterized.
 *!Bit zero is used first; the default pattern is all 1's.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLineStipple]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glLineWidth(float width)
 *!
 *!@[glLineWidth] specifies the rasterized width of both aliased and antialiased
 *!lines.
 *!Using a line width other than 1 has different effects,
 *!depending on whether line antialiasing is enabled.
 *!To enable and disable line antialiasing, call 
 *!@[glEnable] and @[glDisable]
 *!with argument @[GL_LINE_SMOOTH]. Line antialiasing is initially
 *!disabled. 
 *!
 *!If line antialiasing is disabled,
 *!the actual width is determined by rounding the supplied width
 *!to the nearest integer.
 *!(If the rounding results in the value 0,
 *!it is as if the line width were 1.)
 *!If
 *!.nf
 *!| DELTA x | >= | DELTA y |,
 *!.fi
 *!@i{i@} pixels are filled in each column that is rasterized,
 *!where @i{i@} is the rounded value of @i{width@}.
 *!Otherwise,
 *!@i{i@} pixels are filled in each row that is rasterized.
 *!
 *!If antialiasing is enabled,
 *!line rasterization produces a fragment for each pixel square
 *!that intersects the region lying within the rectangle having width
 *!equal to the current line width,
 *!length equal to the actual length of the line,
 *!and centered on the mathematical line segment.
 *!The coverage value for each fragment is the window coordinate area
 *!of the intersection of the rectangular region with the corresponding
 *!pixel square.
 *!This value is saved and used in the final rasterization step. 
 *!
 *!Not all widths can be supported when line antialiasing is enabled. 
 *!If an unsupported width is requested,
 *!the nearest supported width is used.
 *!Only width 1 is guaranteed to be supported;
 *!others depend on the implementation.
 *!To query the range of supported widths and the size difference between
 *!supported widths within the range, call
 *!@[glGet] with arguments
 *!@[GL_LINE_WIDTH_RANGE] and
 *!@[GL_LINE_WIDTH_GRANULARITY]. 
 *!
 *!@param width
 *!
 *!Specifies the width of rasterized lines.
 *!The initial value is 1.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{width@} is less than or equal to 0.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLineWidth]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glListBase(int base)
 *!
 *!@[glCallLists] specifies an array of offsets.
 *!Display-list names are generated by adding @i{base@} to each offset.
 *!Names that reference valid display lists are executed;
 *!the others are ignored.
 *!
 *!@param base
 *!
 *!Specifies an integer offset that will be added to @[glCallLists]
 *!offsets to generate display-list names.
 *!The initial value is 0.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glListBase]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glLoadMatrix(array(float|int) *m)
 *!
 *!@[glLoadMatrix] replaces the current matrix with the one whose elements are specified by
 *!@i{m@}.
 *!The current matrix is the projection matrix,
 *!modelview matrix,
 *!or texture matrix,
 *!depending on the current matrix mode
 *!(see @[glMatrixMode]).
 *!
 *!The current matrix, M,  defines a transformation of coordinates.
 *!For instance, assume M refers to the modelview matrix.
 *!If  v ~=~ (v[0], v[1], v[2], v[3]) is the set of object coordinates
 *!of a vertex,
 *!and @i{m@} points to an array of 16 
 *!single- or double-precision
 *!floating-point values m[0], m[1],. . .,m[15],
 *!then the modelview transformation M(v) does the following:
 *!
 *!
 *!.ce
 *!.EQ
 *!down 130
 *!{M(v)  ~ = ~ 
 *!{{ left (  matrix {
 *!   ccol { ~m[0] above m[1] above m[2] above m[3] ~}
 *!   ccol { ~m[4] above m[5] above m[6] above m[7] ~}
 *!   ccol { ~m[8] above m[9] above m[10] above m[11] ~}
 *!   ccol { ~m[12]~ above m[13]~ above m[14]~ above m[15]~}
 *!} right ) } ~~ times ~~
 *!{left ( matrix {
 *!ccol { ~v[0]~ above ~v[1]~ above ~v[2]~ above ~v[3]~ }
 *!} right )} }}
 *!.EN
 *!
 *!.sp
 *!
 *!Where 'times' denotes matrix multiplication.
 *!
 *!Projection and texture transformations are similarly defined.
 *!
 *!@param m
 *!
 *!Specifies a pointer to 16 consecutive values, which are used as the
 *!elements of a 4 times 4 column-major matrix. 
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLoadMatrix]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glLoadName(int name)
 *!
 *!The name stack is used during selection mode to allow sets of rendering
 *!commands to be uniquely identified.
 *!It consists of an ordered set of unsigned integers.
 *!@[glLoadName] causes @i{name@} to replace the value on the top of the name stack,
 *!which is initially empty.
 *!
 *!The name stack is always empty while the render mode is not @[GL_SELECT].
 *!Calls to @[glLoadName] while the render mode is not @[GL_SELECT] are ignored.
 *!
 *!@param name
 *!
 *!Specifies a name that will replace the top value on the name stack.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLoadName] is called while the
 *!name stack is empty.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glLoadName] is executed between
 *!the execution of @[glBegin] and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glMaterial(int face, int pname, float|int|array(float|int) param)
 *!
 *!@[glMaterial] assigns values to material parameters.
 *!There are two matched sets of material parameters.
 *!One,
 *!the @i{front-facing@} set,
 *!is used to shade points,
 *!lines,
 *!bitmaps,
 *!and all polygons
 *!(when two-sided lighting is disabled),
 *!or just front-facing polygons
 *!(when two-sided lighting is enabled).
 *!The other set,
 *!@i{back-facing@},
 *!is used to shade back-facing polygons only when two-sided lighting is enabled.
 *!Refer to the @[glLightModel] reference page for details concerning one- and
 *!two-sided lighting calculations.
 *!
 *!@[glMaterial] takes three arguments.
 *!The first,
 *!@i{face@},
 *!specifies whether the
 *!@[GL_FRONT] materials, the
 *!@[GL_BACK] materials, or both
 *!@[GL_FRONT_AND_BACK] materials will be modified.
 *!The second,
 *!@i{pname@},
 *!specifies which of several parameters in one or both sets will be modified.
 *!The third,
 *!@i{params@},
 *!specifies what value or values will be assigned to the specified parameter.
 *!
 *!Material parameters are used in the lighting equation that is optionally
 *!applied to each vertex.
 *!The equation is discussed in the @[glLightModel] reference page.
 *!The parameters that can be specified using @[glMaterial],
 *!and their interpretations by the lighting equation, are as follows:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_AMBIENT</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the ambient RGBA reflectance of the material.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!The initial ambient reflectance for both front- and back-facing materials
 *!is (0.2, 0.2, 0.2, 1.0).
 *!</c></r>
 *!<r><c><ref>GL_DIFFUSE</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the diffuse RGBA reflectance of the material.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!The initial diffuse reflectance for both front- and back-facing materials
 *!is (0.8, 0.8, 0.8, 1.0).
 *!</c></r>
 *!<r><c><ref>GL_SPECULAR</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the specular RGBA reflectance of the material.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!The initial specular reflectance for both front- and back-facing materials
 *!is (0, 0, 0, 1).
 *!</c></r>
 *!<r><c><ref>GL_EMISSION</ref>
 *!</c><c><i>params</i> contains four integer or floating-point values that specify
 *!the RGBA emitted light intensity of the material.
 *!Integer values are mapped linearly such that the most positive representable
 *!value maps to 1.0,
 *!and the most negative representable value maps to -1.0.
 *!Floating-point values are mapped directly.
 *!Neither integer nor floating-point values are clamped.
 *!The initial emission intensity for both front- and back-facing materials
 *!is (0, 0, 0, 1).
 *!</c></r>
 *!<r><c><ref>GL_SHININESS</ref>
 *!</c><c><i>params</i> is a single integer or floating-point value that specifies
 *!the RGBA specular exponent of the material.
 *!Integer and floating-point values are mapped directly.
 *!Only values in the range [0,128] are accepted.
 *!The initial specular exponent for both front- and back-facing materials
 *!is 0.
 *!</c></r>
 *!<r><c><ref>GL_AMBIENT_AND_DIFFUSE</ref>
 *!</c><c>Equivalent to calling <ref>glMaterial</ref> twice with the same parameter values,
 *!once with <ref>GL_AMBIENT</ref> and once with <ref>GL_DIFFUSE</ref>.
 *!</c></r>
 *!<r><c><ref>GL_COLOR_INDEXES</ref>
 *!</c><c><i>params</i> contains three integer or floating-point values specifying
 *!the color indices for ambient,
 *!diffuse,
 *!and specular lighting.
 *!These three values,
 *!and <ref>GL_SHININESS</ref>,
 *!are the only material values used by the color index mode lighting equation.
 *!Refer to the <ref>glLightModel</ref> reference page for a discussion 
 *!of color index lighting.
 *!</c></r></matrix>@}
 *!
 *!
 *!@param face
 *!
 *!Specifies which face or faces are being updated.
 *!Must be one of
 *!@[GL_FRONT],
 *!@[GL_BACK], or
 *!@[GL_FRONT_AND_BACK].
 *!
 *!@param pname
 *!
 *!Specifies the single-valued material parameter of the face or faces
 *!that is being updated.
 *!Must be @[GL_SHININESS].
 *!
 *!@param param
 *!
 *!Specifies the value that parameter @[GL_SHININESS] will be set to.
 *!
 *!
 *!@param face
 *!
 *!Specifies which face or faces are being updated.
 *!Must be one of
 *!@[GL_FRONT],
 *!@[GL_BACK], or
 *!@[GL_FRONT_AND_BACK].
 *!
 *!@param pname
 *!
 *!Specifies the material parameter of the face or faces that is being updated.
 *!Must be one of
 *!@[GL_AMBIENT],
 *!@[GL_DIFFUSE],
 *!@[GL_SPECULAR],
 *!@[GL_EMISSION],
 *!@[GL_SHININESS],
 *!@[GL_AMBIENT_AND_DIFFUSE],  or
 *!@[GL_COLOR_INDEXES].
 *!
 *!@param params
 *!
 *!Specifies a pointer to the value or values that @i{pname@} will be set to.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if either @i{face@} or @i{pname@} is not
 *!an accepted value.
 *!
 *!@[GL_INVALID_VALUE] is generated if a specular exponent outside the range
 *![0,128] is specified.
 *!
 *!
 */

/*!@decl void glMultMatrix(array(float|int) *m)
 *!
 *!@[glMultMatrix] multiplies the current matrix with the one specified using @i{m@}, and
 *!replaces the current matrix with the product.
 *!
 *!The current matrix is determined by the current matrix mode (see @[glMatrixMode]). It is either the projection matrix,
 *!modelview matrix,
 *!or the texture matrix.
 *!
 *!@param m
 *!
 *!Points to 16 consecutive values that are used as the elements of 
 *!a 4 times 4 column-major matrix. 
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glMultMatrix]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glNewList(int list, int mode)
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
 *!
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

/*!@decl void glNormal(float|int|array(float|int) nx, float|int|void ny, float|int|void nz)
 *!
 *!The current normal is set to the given coordinates
 *!whenever @[glNormal] is issued.
 *!Byte, short, or integer arguments are converted to floating-point
 *!format with a linear mapping that maps the most positive representable integer
 *!value to 1.0,
 *!and the most negative representable integer value to -1.0.
 *!
 *!Normals specified with @[glNormal] need not have unit length.
 *!If normalization is enabled,
 *!then normals specified with @[glNormal] are normalized after transformation.
 *!To enable and disable normalization, call @[glEnable] and @[glDisable]
 *!with the argument @[GL_NORMALIZE].
 *!Normalization is initially disabled.
 *!
 *!@param nx
 *!
 *!Specify the x, y, and z coordinates of the new current normal.
 *!The initial value of the current normal is the unit vector, (0, 0, 1).
 *!
 *!
 *!
 *!@param v
 *!
 *!Specifies a pointer to an array of three elements:
 *!the x, y, and z coordinates of the new current normal.
 *!
 *!
 *!
 */

/*!@decl void glOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
 *!
 *!@[glOrtho] describes a transformation that produces a parallel projection.
 *!The current matrix (see @[glMatrixMode]) is multiplied by this matrix
 *!and the result replaces the current matrix, as if
 *!@[glMultMatrix] were called with the following matrix
 *!as its argument:
 *!.sp
 *!.ce
 *!.EQ
 *!left (  matrix {
 *!   ccol { {2 over {"right" - "left"}} above 0 above 0 above 0 }
 *!   ccol { 0 above {2 over {"top" - "bottom"}} above 0 above 0 }
 *!   ccol { 0 above 0 above {-2 over {"zFar" - "zNear"}}  above 0 }
 *!   ccol { {t sub x}~ above {t sub y}~ above {t sub z}~ above 1~ }
 *!} right )
 *!.EN
 *!
 *!where
 *!.ce
 *!.EQ
 *!t sub x ~=~ -{{"right" + "left"} over {"right" - "left"}}
 *!.EN
 *!
 *!.ce
 *!.EQ
 *!t sub y ~=~ -{{"top" + "bottom"} over {"top" - "bottom"}}
 *!.EN
 *!
 *!.ce
 *!.EQ
 *!t sub z ~=~ -{{"zFar" + "zNear"} over {"zFar" - "zNear"}}
 *!.EN
 *!
 *!.RE
 *!
 *!Typically, the matrix mode is @[GL_PROJECTION], and 
 *!(@i{left@}, @i{bottom@},  -@i{zNear@}) and (@i{right@}, @i{top@},  -@i{zNear@})
 *!specify the points on the near clipping plane that are mapped
 *!to the lower left and upper right corners of the window,
 *!respectively,
 *!assuming that the eye is located at (0, 0, 0).
 *!-@i{zFar@} specifies the location of the far clipping plane.
 *!Both @i{zNear@} and @i{zFar@} can be either positive or negative.
 *!
 *!Use @[glPushMatrix] and @[glPopMatrix] to save and restore
 *!the current matrix stack.
 *!
 *!@param left
 *!
 *!Specify the coordinates for the left and right vertical clipping planes.
 *!
 *!@param bottom
 *!
 *!Specify the coordinates for the bottom and top horizontal clipping planes.
 *!
 *!@param zNear
 *!
 *!Specify the distances to the nearer and farther depth clipping planes.
 *!These values are negative if the plane is to be behind the viewer.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glOrtho]
 *!is executed between the execution of 
 *!@[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPassThrough(float token)
 *!
 *!
 *!Feedback is a GL render mode.
 *!The mode is selected by calling
 *!@[glRenderMode] with @[GL_FEEDBACK].
 *!When the GL is in feedback mode,
 *!no pixels are produced by rasterization.
 *!Instead,
 *!information about primitives that would have been rasterized
 *!is fed back to the application using the GL.
 *!See the @[glFeedbackBuffer] reference page for a description of the
 *!feedback buffer and the values in it.  
 *!
 *!@[glPassThrough] inserts a user-defined marker in the feedback buffer
 *!when it is executed in feedback mode.  
 *!@i{token@} is returned as if it were a primitive;
 *!it is indicated with its own unique identifying value:
 *!@[GL_PASS_THROUGH_TOKEN].
 *!The order of @[glPassThrough] commands with respect to the specification
 *!of graphics primitives is maintained.  
 *!
 *!@param token
 *!
 *!Specifies a marker value to be placed in the feedback buffer
 *!following a @[GL_PASS_THROUGH_TOKEN].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPassThrough] is executed between
 *!the execution of @[glBegin] and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPixelZoom(float xfactor, float yfactor)
 *!
 *!@[glPixelZoom] specifies values for the x and y zoom factors.
 *!During the execution of @[glDrawPixels] or @[glCopyPixels],
 *!if (xr , yr ) is the current raster position,
 *!and a given element is in the mth row and nth column of the pixel rectangle,
 *!then pixels whose centers are in the rectangle with corners at 
 *!.sp
 *!.ce
 *!(xr ~+~ n cdot "xfactor", yr ~+~ m cdot "yfactor") 
 *!.sp
 *!.ce
 *!(xr ~+~ (n+1) cdot "xfactor", yr ~+~ (m+1) cdot "yfactor")
 *!.sp
 *!are candidates for replacement.
 *!Any pixel whose center lies on the bottom or left edge of this rectangular
 *!region is also modified.
 *!
 *!Pixel zoom factors are not limited to positive values.
 *!Negative zoom factors reflect the resulting image about the current
 *!raster position.
 *!
 *!@param xfactor
 *!
 *!Specify the x and y zoom factors for pixel write operations.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPixelZoom]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPointSize(float size)
 *!
 *!@[glPointSize] specifies the rasterized diameter of both aliased and antialiased
 *!points.
 *!Using a point size other than 1 has different effects,
 *!depending on whether point antialiasing is enabled.
 *!To enable and disable point antialiasing, call 
 *!@[glEnable] and @[glDisable]
 *!with argument @[GL_POINT_SMOOTH]. Point antialiasing is initially disabled.
 *!
 *!If point antialiasing is disabled,
 *!the actual size is determined by rounding the supplied size
 *!to the nearest integer.
 *!(If the rounding results in the value 0,
 *!it is as if the point size were 1.)
 *!If the rounded size is odd,
 *!then the center point
 *!( x ,  y )
 *!of the pixel fragment that represents the point is computed as
 *!.sp
 *!.ce
 *!(  \(lf ~ x sub w ~ \(rf ~+~ .5 ,  \(lf ~ y sub w ~ \(rf ~+~ .5 )
 *!.sp
 *!where w subscripts indicate window coordinates.
 *!All pixels that lie within the square grid of the rounded size centered at
 *!( x ,  y )
 *!make up the fragment.
 *!If the size is even,
 *!the center point is
 *!.sp
 *!.ce
 *!(  \(lf ~ x sub w ~+~ .5 ~ \(rf,  \(lf ~ y sub w ~+~ .5 ~ \(rf )
 *!.sp
 *!and the rasterized fragment's centers are the half-integer window coordinates
 *!within the square of the rounded size centered at
 *!( x ,  y ).
 *!All pixel fragments produced in rasterizing a nonantialiased point are
 *!assigned the same associated data,
 *!that of the vertex corresponding to the point.
 *!
 *!If antialiasing is enabled,
 *!then point rasterization produces a fragment for each pixel square
 *!that intersects the region lying within the circle having diameter
 *!equal to the current point size and centered at the point's
 *!( x sub w ,  y sub w ).
 *!The coverage value for each fragment is the window coordinate area
 *!of the intersection of the circular region with the corresponding pixel square.
 *!This value is saved and used in the final rasterization step. 
 *!The data associated with each fragment is the data associated with 
 *!the point being rasterized.
 *!
 *!Not all sizes are supported when point antialiasing is enabled. 
 *!If an unsupported size is requested,
 *!the nearest supported size is used.
 *!Only size 1 is guaranteed to be supported;
 *!others depend on the implementation.
 *!To query the range of supported sizes and the size difference between
 *!supported sizes within the range, call
 *!@[glGet] with arguments
 *!@[GL_POINT_SIZE_RANGE] and
 *!@[GL_POINT_SIZE_GRANULARITY].
 *!
 *!@param size
 *!
 *!Specifies the diameter of rasterized points.
 *!The initial value is 1.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{size@} is less than or equal to 0.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPointSize]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPolygonMode(int face, int mode)
 *!
 *!@[glPolygonMode] controls the interpretation of polygons for rasterization.
 *!@i{face@} describes which polygons @i{mode@} applies to:
 *!front-facing polygons (@[GL_FRONT]),
 *!back-facing polygons (@[GL_BACK]),
 *!or both (@[GL_FRONT_AND_BACK]).
 *!The polygon mode affects only the final rasterization of polygons.
 *!In particular,
 *!a polygon's vertices are lit and
 *!the polygon is clipped and possibly culled before these modes are applied.
 *!
 *!Three modes are defined and can be specified in @i{mode@}:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_POINT</ref>
 *!</c><c>Polygon vertices that are marked as the start of a boundary edge
 *!are drawn as points.
 *!Point attributes such as
 *!<ref>GL_POINT_SIZE</ref> and
 *!<ref>GL_POINT_SMOOTH</ref> control
 *!the rasterization of the points.
 *!Polygon rasterization attributes other than <ref>GL_POLYGON_MODE</ref> have no effect.
 *!</c></r>
 *!<r><c><ref>GL_LINE</ref>
 *!</c><c>Boundary edges of the polygon are drawn as line segments.
 *!They are treated as connected line segments for line stippling;
 *!the line stipple counter and pattern are not reset between segments
 *!(see <ref>glLineStipple</ref>).
 *!Line attributes such as
 *!<ref>GL_LINE_WIDTH</ref> and
 *!<ref>GL_LINE_SMOOTH</ref> control
 *!the rasterization of the lines.
 *!Polygon rasterization attributes other than <ref>GL_POLYGON_MODE</ref> have no effect.
 *!</c></r>
 *!<r><c><ref>GL_FILL</ref>
 *!</c><c>The interior of the polygon is filled.
 *!Polygon attributes such as
 *!<ref>GL_POLYGON_STIPPLE</ref> and
 *!<ref>GL_POLYGON_SMOOTH</ref> control the rasterization of the polygon.
 *!</c></r></matrix>@}
 *!
 *!
 *!@param face
 *!
 *!Specifies the polygons that @i{mode@} applies to.
 *!Must be
 *!@[GL_FRONT] for front-facing polygons,
 *!@[GL_BACK] for back-facing polygons,
 *!or @[GL_FRONT_AND_BACK] for front- and back-facing polygons.
 *!
 *!@param mode
 *!
 *!Specifies how polygons will be rasterized.
 *!Accepted values are
 *!@[GL_POINT],
 *!@[GL_LINE], and
 *!@[GL_FILL].
 *!The initial value is @[GL_FILL] for both front- and back-facing polygons.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if either @i{face@} or @i{mode@} is not
 *!an accepted value.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPolygonMode]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPolygonOffset(float factor, float units)
 *!
 *!When @[GL_POLYGON_OFFSET] is enabled, each
 *!fragment's @i{depth@} value will be offset after it is interpolated
 *!from the @i{depth@} values of the appropriate vertices.
 *!The value of the offset is "factor" ~*~ DZ ~~+~~ r ~*~ "units",
 *!where DZ~ is a measurement of the change in depth relative to the screen 
 *!area of the polygon, and r is the smallest value that is guaranteed to
 *!produce a resolvable offset for a given implementation.
 *!The offset is added before the depth test is performed and before
 *!the value is written into the depth buffer.
 *!
 *!@[glPolygonOffset] is useful for rendering hidden-line images, for applying decals 
 *!to surfaces, and for rendering solids with highlighted edges.
 *!
 *!@param factor
 *!
 *!Specifies a scale factor that is used to create a variable
 *!depth offset for each polygon. The initial value is 0.
 *!
 *!@param units
 *!
 *!Is multiplied by an implementation-specific value to
 *!create a constant depth offset. The initial value is 0.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glPolygonOffset] is executed
 *!between the execution of @[glBegin] and the corresponding
 *!execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glPushAttrib(int mask)
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

/*!@decl void glPushClientAttrib(int mask)
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

/*!@decl void glPushName(int name)
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

/*!@decl void glRasterPos(float|int x, float|int y, float|int|void z, float|int|void w)
 *!@decl void glRasterPos(array(float|int) pos)
 *!
 *!The GL maintains a 3D position in window coordinates.
 *!This position,
 *!called the raster position,
 *!is used to position pixel and bitmap write operations. It is
 *!maintained with subpixel accuracy.
 *!See @[glBitmap], @[glDrawPixels], and @[glCopyPixels].
 *!
 *!The current raster position consists of three window coordinates
 *!(x, y, z),
 *!a clip coordinate value (w),
 *!an eye coordinate distance,
 *!a valid bit,
 *!and associated color data and texture coordinates.
 *!The w coordinate is a clip coordinate,
 *!because w is not projected to window coordinates.
 *!The variable z defaults to 0 and w defaults to 1.
 *!
 *!The object coordinates presented by @[glRasterPos] are treated just like those
 *!of a @[glVertex] command:
 *!They are transformed by the current modelview and projection matrices
 *!and passed to the clipping stage.
 *!If the vertex is not culled,
 *!then it is projected and scaled to window coordinates,
 *!which become the new current raster position,
 *!and the @[GL_CURRENT_RASTER_POSITION_VALID] flag is set.
 *!If the vertex 
 *!.I is
 *!culled,
 *!then the valid bit is cleared and the current raster position
 *!and associated color and texture coordinates are undefined.
 *!
 *!The current raster position also includes some associated color data
 *!and texture coordinates.
 *!If lighting is enabled,
 *!then @[GL_CURRENT_RASTER_COLOR]
 *!(in RGBA mode)
 *!or @[GL_CURRENT_RASTER_INDEX]
 *!(in color index mode)
 *!is set to the color produced by the lighting calculation
 *!(see @[glLight], @[glLightModel], and 
 *!
 *!@[glShadeModel]).
 *!If lighting is disabled, 
 *!current color
 *!(in RGBA mode, state variable @[GL_CURRENT_COLOR])
 *!or color index
 *!(in color index mode, state variable @[GL_CURRENT_INDEX])
 *!is used to update the current raster color.
 *!
 *!Likewise,
 *!@[GL_CURRENT_RASTER_TEXTURE_COORDS] is updated as a function
 *!of @[GL_CURRENT_TEXTURE_COORDS],
 *!based on the texture matrix and the texture generation functions
 *!(see @[glTexGen]).
 *!Finally,
 *!the distance from the origin of the eye coordinate system to the
 *!vertex as transformed by only the modelview matrix replaces
 *!@[GL_CURRENT_RASTER_DISTANCE].
 *!
 *!Initially, the current raster position is (0, 0, 0, 1),
 *!the current raster distance is 0,
 *!the valid bit is set,
 *!the associated RGBA color is (1, 1, 1, 1),
 *!the associated color index is 1,
 *!and the associated texture coordinates are (0, 0, 0, 1).
 *!In RGBA mode,
 *!@[GL_CURRENT_RASTER_INDEX] is always 1;
 *!in color index mode,
 *!the current raster RGBA color always maintains its initial value.
 *!
 *!@param x
 *!
 *!Specify the x, y, z, and w object coordinates
 *!(if present)
 *!for the raster position.
 *!
 *!
 *!@param v
 *!
 *!Specifies a pointer to an array of two,
 *!three,
 *!or four elements,
 *!specifying x, y, z, and w coordinates, respectively.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glRasterPos]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glRotate(float|int|array(float|int) angle, float|int|void x, float|int|void y, float|int|void z)
 *!
 *!@[glRotate] produces a rotation of @i{angle@} degrees around 
 *!the vector ("x", "y", "z").
 *!The current matrix (see @[glMatrixMode]) is multiplied by a rotation 
 *!matrix with the product
 *!replacing the current matrix, as if @[glMultMatrix] were called
 *!with the following matrix as its argument:
 *!
 *!.ce
 *!.EQ 
 *!left ( ~ down 20 matrix {
 *!   ccol { "x" "x" (1 - c)+ c above  "y" "x" (1 - c)+ "z" s above   "x" "z" (1 - c)-"y" s above ~0 }
 *!   ccol {"x" "y" (1 - c)-"z" s above  "y" "y"  (1 - c)+ c above   "y" "z" (1 - c)+ "x" s above ~0 }
 *!   ccol {  "x" "z" (1 - c)+ "y" s above  "y" "z" (1 - c)- "x" s above "z" "z" (1 - c) + c above ~0 }
 *!   ccol { ~0 above ~0 above ~0 above ~1}
 *!} ~~ right )
 *!.EN
 *!
 *!.sp
 *!Where c ~=~ cos("angle"), s ~=~ sine("angle"), and 
 *!||(~"x", "y", "z"~)|| ~=~ 1 (if not, the GL 
 *!will normalize this vector).
 *!.sp
 *!.sp
 *!
 *!If the matrix mode is either @[GL_MODELVIEW] or @[GL_PROJECTION],
 *!all objects drawn after @[glRotate] is called are rotated.
 *!Use @[glPushMatrix] and @[glPopMatrix] to save and restore
 *!the unrotated coordinate system.
 *!
 *!@param angle
 *!
 *!Specifies the angle of rotation, in degrees.
 *!
 *!@param x
 *!
 *!Specify the @i{x@}, @i{y@}, and @i{z@} coordinates of a vector, respectively. 
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glRotate]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glScale(float|int|array(float|int) x, float|int|void y, float|int|void z)
 *!
 *!@[glScale] produces a nonuniform scaling along the @i{x@}, @i{y@}, and
 *!@i{z@} axes. 
 *!The three parameters indicate the desired scale factor along
 *!each of the three axes.
 *!
 *!The current matrix
 *!(see @[glMatrixMode])
 *!is multiplied by this scale matrix,
 *!and the product replaces the current matrix
 *!as if @[glScale] were called with the following matrix
 *!as its argument:
 *!
 *!
 *!.ce
 *!.EQ 
 *!left ( ~ down 20 matrix {
 *!   ccol { ~"x" above ~0 above ~0 above ~0 }
 *!   ccol { ~0 above ~"y" above ~0 above ~0 }
 *!   ccol { ~0 above ~0 above ~"z" above ~0 }
 *!   ccol { ~0 above ~0 above ~0 above ~1}
 *!} ~~ right )
 *!.EN
 *!.sp
 *!If the matrix mode is either @[GL_MODELVIEW] or @[GL_PROJECTION],
 *!all objects drawn after @[glScale] is called are scaled.
 *!
 *!Use @[glPushMatrix] and @[glPopMatrix] to save and restore
 *!the unscaled coordinate system.
 *!
 *!@param x
 *!
 *!Specify scale factors along the @i{x@}, @i{y@}, and @i{z@} axes, respectively.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glScale]
 *!is executed between the execution of 
 *!@[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glScissor(int x, int y, int width, int height)
 *!
 *!@[glScissor] defines a rectangle, called the scissor box,
 *!in window coordinates.
 *!The first two arguments,
 *!@i{x@} and @i{y@},
 *!specify the lower left corner of the box.
 *!@i{width@} and @i{height@} specify the width and height of the box. 
 *!
 *!To enable and disable the scissor test, call
 *!@[glEnable] and @[glDisable] with argument
 *!@[GL_SCISSOR_TEST]. The test is initially disabled. 
 *!While the test is enabled, only pixels that lie within the scissor box
 *!can be modified by drawing commands.
 *!Window coordinates have integer values at the shared corners of
 *!frame buffer pixels.
 *!\f7glScissor(0,0,1,1)\fP allows modification of only the lower left
 *!pixel in the window, and \f7glScissor(0,0,0,0)\fP doesn't allow
 *!modification of any pixels in the window.  
 *!
 *!When the scissor test is disabled,
 *!it is as though the scissor box includes the entire window.
 *!
 *!@param x
 *!
 *!Specify the lower left corner of the scissor box.
 *!Initially (0, 0).
 *!
 *!@param width
 *!
 *!Specify the width and height of the scissor box.
 *!When a GL context is first attached to a window,
 *!@i{width@} and @i{height@} are set to the dimensions of that window.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@} is negative.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glScissor]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glStencilFunc(int func, int ref, int mask)
 *!
 *!Stenciling,
 *!like depth-buffering,
 *!enables and disables drawing on a per-pixel basis.
 *!You draw into the stencil planes using GL drawing primitives,
 *!then render geometry and images,
 *!using the stencil planes to mask out portions of the screen.
 *!Stenciling is typically used in multipass rendering algorithms
 *!to achieve special effects,
 *!such as decals,
 *!outlining,
 *!and constructive solid geometry rendering.
 *!
 *!The stencil test conditionally eliminates a pixel based on the outcome
 *!of a comparison between the reference value
 *!and the value in the stencil buffer.
 *!To enable and disable the test, call @[glEnable] and @[glDisable]
 *!with argument @[GL_STENCIL_TEST].
 *!To specify actions based on the outcome of the stencil test, call
 *!@[glStencilOp].
 *!
 *!@i{func@} is a symbolic constant that determines the stencil comparison function.
 *!It accepts one of eight values,
 *!shown in the following list.
 *!@i{ref@} is an integer reference value that is used in the stencil comparison.
 *!It is clamped to the range [0,2 sup n - 1],
 *!where n is the number of bitplanes in the stencil buffer.
 *!@i{mask@} is bitwise ANDed with both the reference value
 *!and the stored stencil value,
 *!with the ANDed values participating in the comparison.
 *!.P 
 *!If @i{stencil@} represents the value stored in the corresponding
 *!stencil buffer location,
 *!the following list shows the effect of each comparison function
 *!that can be specified by @i{func@}.
 *!Only if the comparison succeeds is the pixel passed through
 *!to the next stage in the rasterization process
 *!(see @[glStencilOp]).
 *!All tests treat @i{stencil@} values as unsigned integers in the range
 *![0,2 sup n - 1],
 *!where n is the number of bitplanes in the stencil buffer.
 *!
 *!The following values are accepted by @i{func@}:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_NEVER</ref>
 *!</c><c>Always fails.
 *!</c></r>
 *!<r><c><ref>GL_LESS</ref>
 *!</c><c>Passes if ( <i>ref</i> &amp; <i>mask</i> ) &lt; ( <i>stencil</i> &amp; <i>mask</i> ). 
 *!</c></r>
 *!<r><c><ref>GL_LEQUAL</ref>
 *!</c><c>Passes if ( <i>ref</i> &amp; <i>mask</i> ) \(&lt;= ( <i>stencil</i> &amp; <i>mask</i> ).
 *!</c></r>
 *!<r><c><ref>GL_GREATER</ref>
 *!</c><c>Passes if ( <i>ref</i> &amp; <i>mask</i> ) &gt; ( <i>stencil</i> &amp; <i>mask</i> ).
 *!</c></r>
 *!<r><c><ref>GL_GEQUAL</ref>
 *!</c><c>Passes if ( <i>ref</i> &amp; <i>mask</i> ) \(&gt;= ( <i>stencil</i> &amp; <i>mask</i> ).
 *!</c></r>
 *!<r><c><ref>GL_EQUAL</ref>
 *!</c><c>Passes if ( <i>ref</i> &amp; <i>mask</i> ) = ( <i>stencil</i> &amp; <i>mask</i> ).
 *!</c></r>
 *!<r><c><ref>GL_NOTEQUAL</ref>
 *!</c><c>Passes if ( <i>ref</i> &amp; <i>mask</i> ) \(!=  ( <i>stencil</i> &amp; <i>mask</i> ).
 *!</c></r>
 *!<r><c><ref>GL_ALWAYS</ref>
 *!</c><c>Always passes.
 *!</c></r></matrix>@}
 *!
 *!
 *!@param func
 *!
 *!Specifies the test function.
 *!Eight tokens are valid:
 *!@[GL_NEVER],
 *!@[GL_LESS],
 *!@[GL_LEQUAL],
 *!@[GL_GREATER],
 *!@[GL_GEQUAL],
 *!@[GL_EQUAL],
 *!@[GL_NOTEQUAL], and
 *!@[GL_ALWAYS]. The initial value is @[GL_ALWAYS]. 
 *!
 *!@param ref
 *!
 *!Specifies the reference value for the stencil test.
 *!@i{ref@} is clamped to the range [0,2 sup n - 1],
 *!where n is the number of bitplanes in the stencil buffer. The
 *!initial value is 0.
 *!
 *!@param mask
 *!
 *!Specifies a mask that is ANDed with both the reference value
 *!and the stored stencil value when the test is done. The initial value
 *!is all 1's. 
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{func@} is not one of the eight
 *!accepted values.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glStencilFunc]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glStencilMask(int mask)
 *!
 *!@[glStencilMask] controls the writing of individual bits in the stencil planes.
 *!The least significant n bits of @i{mask@},
 *!where n is the number of bits in the stencil buffer,
 *!specify a mask.
 *!Where a 1 appears in the mask,
 *!it's possible to write to the corresponding bit in the stencil buffer.
 *!Where a 0 appears,
 *!the corresponding bit is write-protected.
 *!Initially, all bits are enabled for writing.
 *!
 *!@param mask
 *!
 *!Specifies a bit mask to enable and disable writing of individual bits
 *!in the stencil planes.
 *!Initially, the mask is all 1's.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glStencilMask]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glStencilOp(int fail, int zfail, int zpass)
 *!
 *!Stenciling,
 *!like depth-buffering,
 *!enables and disables drawing on a per-pixel basis.
 *!You draw into the stencil planes using GL drawing primitives,
 *!then render geometry and images,
 *!using the stencil planes to mask out portions of the screen.
 *!Stenciling is typically used in multipass rendering algorithms
 *!to achieve special effects,
 *!such as decals,
 *!outlining,
 *!and constructive solid geometry rendering.
 *!
 *!The stencil test conditionally eliminates a pixel based on the outcome
 *!of a comparison between the value in the stencil buffer and a
 *!reference value. To enable and disable the test, call @[glEnable]
 *!and @[glDisable] with argument
 *!@[GL_STENCIL_TEST]; to control it, call @[glStencilFunc].
 *!
 *!@[glStencilOp] takes three arguments that indicate what happens
 *!to the stored stencil value while stenciling is enabled.
 *!If the stencil test fails,
 *!no change is made to the pixel's color or depth buffers,
 *!and @i{fail@} specifies what happens to the stencil buffer contents.
 *!The following six actions are possible.
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_KEEP</ref>
 *!</c><c>Keeps the current value.
 *!</c></r>
 *!<r><c><ref>GL_ZERO</ref>
 *!</c><c>Sets the stencil buffer value to 0.
 *!</c></r>
 *!<r><c><ref>GL_REPLACE</ref>
 *!</c><c>Sets the stencil buffer value to <i>ref</i>,
 *!as specified by <ref>glStencilFunc</ref>.
 *!</c></r>
 *!<r><c><ref>GL_INCR</ref>
 *!</c><c>Increments the current stencil buffer value.
 *!Clamps to the maximum representable unsigned value.
 *!</c></r>
 *!<r><c><ref>GL_DECR</ref>
 *!</c><c>Decrements the current stencil buffer value.
 *!Clamps to 0.
 *!</c></r>
 *!<r><c><ref>GL_INVERT</ref>
 *!</c><c>Bitwise inverts the current stencil buffer value.
 *!</c></r>
 *!</matrix>@}
 *!
 *!Stencil buffer values are treated as unsigned integers.
 *!When incremented and decremented,
 *!values are clamped to 0 and 2 sup n - 1,
 *!where n is the value returned by querying @[GL_STENCIL_BITS].
 *!
 *!The other two arguments to @[glStencilOp] specify stencil buffer actions
 *!that depend on whether subsequent depth buffer tests succeed (@i{zpass@})
 *!or fail (@i{zfail@}) (see 
 *!
 *!@[glDepthFunc]).
 *!The actions are specified using the same six symbolic constants as @i{fail@}.
 *!Note that @i{zfail@} is ignored when there is no depth buffer,
 *!or when the depth buffer is not enabled.
 *!In these cases, @i{fail@} and @i{zpass@} specify stencil action when the
 *!stencil test fails and passes,
 *!respectively.
 *!
 *!@param fail
 *!
 *!Specifies the action to take when the stencil test fails.
 *!Six symbolic constants are accepted:
 *!@[GL_KEEP],
 *!@[GL_ZERO],
 *!@[GL_REPLACE],
 *!@[GL_INCR],
 *!@[GL_DECR], and
 *!@[GL_INVERT]. The initial value is @[GL_KEEP]. 
 *!
 *!@param zfail
 *!
 *!Specifies the stencil action when the stencil test passes,
 *!but the depth test fails.
 *!@i{zfail@} accepts the same symbolic constants as @i{fail@}. The initial value
 *!is @[GL_KEEP].
 *!
 *!@param zpass
 *!
 *!Specifies the stencil action when both the stencil test and the depth
 *!test pass, or when the stencil test passes and either there is no
 *!depth buffer or depth testing is not enabled.
 *!@i{zpass@} accepts the same symbolic constants as @i{fail@}. The initial value
 *!is @[GL_KEEP].
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{fail@},
 *!@i{zfail@}, or @i{zpass@} is any value other than the six defined constant values.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glStencilOp]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glTexCoord(float|int|array(float|int) s, float|int|void t, float|int|void r, float|int|void q)
 *!
 *!@[glTexCoord] specifies texture coordinates in
 *!one,
 *!two,
 *!three, or
 *!four dimensions.  
 *!@[glTexCoord1] sets the current texture coordinates to
 *!(@i{s@}, 0, 0, 1);
 *!a call to 
 *!
 *!@[glTexCoord2] sets them to
 *!(@i{s@}, @i{t@}, 0, 1).
 *!Similarly, @[glTexCoord3] specifies the texture coordinates as
 *!(@i{s@}, @i{t@}, @i{r@}, 1), and
 *!@[glTexCoord4] defines all four components explicitly as
 *!(@i{s@}, @i{t@}, @i{r@}, @i{q@}). 
 *!
 *!The current texture coordinates are part of the data
 *!that is associated with each vertex and with the current
 *!raster position. 
 *!Initially, the values for 
 *!@i{s@},
 *!@i{t@},
 *!@i{r@}, and
 *!@i{q@}
 *!are (0, 0, 0, 1). 
 *!
 *!
 *!@param s
 *!
 *!Specify @i{s@}, @i{t@}, @i{r@}, and @i{q@}  texture coordinates.
 *!Not all parameters are present in all forms of the command.
 *!
 *!
 *!@param v
 *!
 *!Specifies a pointer to an array of one, two, three, or four elements,
 *!which in turn specify the
 *!@i{s@},
 *!@i{t@},
 *!@i{r@}, and
 *!@i{q@} texture coordinates.
 *!
 *!
 */

/*!@decl void glTexEnv(int target, int pname, float|int|array(float|int) param)
 *!
 *!A texture environment specifies how texture values are interpreted
 *!when a fragment is textured.
 *!@i{target@} must be @[GL_TEXTURE_ENV].
 *!@i{pname@} can be either @[GL_TEXTURE_ENV_MODE] or @[GL_TEXTURE_ENV_COLOR].
 *!
 *!If @i{pname@} is @[GL_TEXTURE_ENV_MODE],
 *!then @i{params@} is (or points to) the symbolic name of a texture function.
 *!Four texture functions may be specified:
 *!@[GL_MODULATE], 
 *!@[GL_DECAL], 
 *!@[GL_BLEND], and
 *!@[GL_REPLACE].
 *!
 *!A texture function acts on the fragment to be textured using
 *!the texture image value that applies to the fragment
 *!(see @[glTexParameter])
 *!and produces an RGBA color for that fragment.
 *!The following table shows how the RGBA color is produced for each
 *!of the three texture functions that can be chosen.
 *!C is a triple of color values (RGB) and A is the associated alpha value.
 *!RGBA values extracted from a texture image are in the range [0,1].
 *!The subscript f refers to the incoming fragment,
 *!the subscript t to the texture image,
 *!the subscript c to the texture environment color,
 *!and subscript v indicates a value produced by the texture function.
 *!
 *!A texture image can have up to four components per texture element
 *!(see @[glTexImage1D], @[glTexImage2D], @[glCopyTexImage1D], and
 *!@[glCopyTexImage2D]). 
 *!In a one-component image,
 *!L sub t indicates that single component.
 *!A two-component image uses L sub t and A sub t.
 *!A three-component image has only a color value, C sub t.
 *!A four-component image has both a color value C sub t
 *!and an alpha value A sub t.
 *!
 *!.ne
 *!.TS
 *!center box tab(:) ;
 *!ci || ci s s s
 *!ci || c c c c
 *!c || c | c | c | c.
 *!Base internal:Texture functions       
 *!format:@[GL_MODULATE]:@[GL_DECAL]:@[GL_BLEND]:@[GL_REPLACE]
 *!=
 *!@[GL_ALPHA]:C sub v = C sub f:undefined:C sub v =  C sub f:C sub v = C sub f
 *!\^ :A sub v = A sub f A sub t:\^:A sub v = A sub f:A sub v = A sub t
 *!_
 *!@[GL_LUMINANCE]:C sub v = L sub t C sub f:undefined:C sub v = ( 1 - L sub t ) C sub f:C sub v = L sub t
 *!1: : :+ L sub t C sub c:
 *!: : : : 
 *!: A sub v = A sub f:\^: A sub v = A sub f:A sub v = A sub f
 *!_
 *!@[GL_LUMINANCE]:C sub v = L sub t C sub f:undefined:C sub v = ( 1 - L sub t ) C sub f :C sub v = L sub t
 *!\@[_ALPHA]: : : + L sub t C sub c
 *!2: : : :
 *!:A sub v = A sub t A sub f:\^:A sub v = A sub t A sub f:A sub v = A sub t
 *!_
 *!@[GL_INTENSITY]:C sub v = C sub f I sub t:undefined:C sub v = ( 1 - I sub t ) C sub f :C sub v = I sub t
 *!: : :+ I sub t C sub c
 *!: : : :
 *!\^ :A sub v = A sub f I sub t:\^:A sub v = ( 1 - I sub t ) A sub f :A sub v = I sub t
 *!: : :+ I sub t A sub c:
 *!_
 *!@[GL_RGB]:C sub v = C sub t C sub f:C sub v = C sub t:C sub v = (1 - C sub t) C sub f :C sub v = C sub t
 *!3: : : + C sub t C sub c
 *!: : : :
 *!:A sub v = A sub f:A sub v = A sub f:A sub v = A sub f:A sub v = A sub f
 *!_
 *!@[GL_RGBA]:C sub v = C sub t C sub f:C sub v = ( 1 - A sub t ) C sub f :C sub v = (1 - C sub t) C sub f :C sub v = C sub t
 *!4: :+ A sub t C sub t: + C sub t C sub c
 *!: : : :
 *!:A sub v = A sub t A sub f:A sub v = A sub f:A sub v = A sub t A sub f:A sub v = A sub t
 *!.TE
 *!.sp
 *!If @i{pname@} is @[GL_TEXTURE_ENV_COLOR],
 *!@i{params@} is a pointer to an array that holds an RGBA color consisting of four
 *!values.
 *!Integer color components are interpreted linearly such that the most
 *!positive integer maps to 1.0,
 *!and the most negative integer maps to -1.0.
 *!The values are clamped to the range [0,1] when they are specified.
 *!C sub c takes these four values.
 *!
 *!@[GL_TEXTURE_ENV_MODE] defaults to @[GL_MODULATE] and
 *!@[GL_TEXTURE_ENV_COLOR] defaults to (0, 0, 0, 0).
 *!
 *!@param target
 *!
 *!Specifies a texture environment.
 *!Must be @[GL_TEXTURE_ENV].
 *!
 *!@param pname
 *!
 *!Specifies the symbolic name of a single-valued texture environment parameter.
 *!Must be @[GL_TEXTURE_ENV_MODE].
 *!
 *!@param param
 *!
 *!Specifies a single symbolic constant, one of @[GL_MODULATE], 
 *!@[GL_DECAL], @[GL_BLEND], or @[GL_REPLACE].
 *!
 *!
 *!@param target
 *!
 *!Specifies a texture environment.
 *!Must be @[GL_TEXTURE_ENV].
 *!
 *!@param pname
 *!
 *!Specifies the symbolic name of a texture environment parameter.
 *!Accepted values are @[GL_TEXTURE_ENV_MODE] and @[GL_TEXTURE_ENV_COLOR].
 *!
 *!@param params
 *!
 *!Specifies a pointer to a parameter array that contains
 *!either a single symbolic constant or an RGBA color.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated when @i{target@} or @i{pname@} is not
 *!one of the accepted defined values,
 *!or when @i{params@} should have a defined constant value
 *!(based on the value of @i{pname@})
 *!and does not.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glTexEnv]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glTexGen(int coord, int pname, float|int|array(float|int) param)
 *!
 *!@[glTexGen] selects a texture-coordinate generation function
 *!or supplies coefficients for one of the functions.
 *!@i{coord@} names one of the (@i{s@}, @i{t@}, @i{r@}, @i{q@}) texture
 *!coordinates; it must be one of the symbols
 *!@[GL_S],
 *!@[GL_T],
 *!@[GL_R], or
 *!@[GL_Q].
 *!@i{pname@} must be one of three symbolic constants:
 *!@[GL_TEXTURE_GEN_MODE],
 *!@[GL_OBJECT_PLANE], or
 *!@[GL_EYE_PLANE]. 
 *!If @i{pname@} is @[GL_TEXTURE_GEN_MODE],
 *!then @i{params@} chooses a mode,
 *!one of
 *!@[GL_OBJECT_LINEAR],
 *!@[GL_EYE_LINEAR], or
 *!@[GL_SPHERE_MAP]. 
 *!If @i{pname@} is either @[GL_OBJECT_PLANE] or @[GL_EYE_PLANE],
 *!@i{params@} contains coefficients for the corresponding
 *!texture generation function.
 *!.P 
 *!If the texture generation function is @[GL_OBJECT_LINEAR],
 *!the function
 *!
 *!.ce
 *!g = p sub 1 x sub o + p sub 2 y sub o + p sub 3 z sub o + p sub 4 w sub o
 *!
 *!
 *!is used, where g is the value computed for the coordinate named in @i{coord@},
 *!p sub 1,
 *!p sub 2,
 *!p sub 3,
 *!and
 *!p sub 4 are the four values supplied in @i{params@}, and
 *!x sub o,
 *!y sub o,
 *!z sub o, and
 *!w sub o are the object coordinates of the vertex.
 *!This function can be used, for example, to texture-map terrain using sea level
 *!as a reference plane
 *!(defined by p sub 1, p sub 2, p sub 3, and p sub 4). 
 *!The altitude of a terrain vertex is computed by the @[GL_OBJECT_LINEAR]
 *!coordinate generation function as its distance from sea level; 
 *!that altitude can then be used to index the texture image to map white snow
 *!onto peaks and green grass onto foothills.
 *!
 *!If the texture generation function is @[GL_EYE_LINEAR], the function
 *!
 *!.ce
 *!g = {p sub 1} sup prime ~x sub e + {p sub 2} sup prime ~y sub e + {p sub 3} sup prime ~z sub e + {p sub 4} sup prime ~w sub e
 *!
 *!
 *!is used, where 
 *!
 *!.ce
 *!$( {p sub 1} sup prime
 *!~~{p sub 2} sup prime~~{p sub 3} sup prime~~
 *!{{p sub 4}sup prime}) = ( p sub 1~~ p sub 2~~ p sub 3~~ p sub 4 ) ~M sup -1$
 *!
 *!
 *!and
 *!x sub e,
 *!y sub e,
 *!z sub e, and
 *!w sub e are the eye coordinates of the vertex,
 *!p sub 1,
 *!p sub 2,
 *!p sub 3,
 *!and
 *!p sub 4 are the values supplied in @i{params@}, and
 *!M is the modelview matrix when @[glTexGen] is invoked.
 *!If M is poorly conditioned or singular,
 *!texture coordinates generated by the resulting function may be inaccurate
 *!or undefined.
 *!
 *!Note that the values in @i{params@} define a reference plane in eye coordinates. 
 *!The modelview matrix that is applied to them may not be the same one
 *!in effect when the polygon vertices are transformed. 
 *!This function establishes a field of texture coordinates
 *!that can produce dynamic contour lines on moving objects.
 *!
 *!If @i{pname@} is @[GL_SPHERE_MAP] and @i{coord@} is either
 *!@[GL_S] or
 *!@[GL_T],
 *!s and t texture coordinates are generated as follows. 
 *!Let @i{u@} be the unit vector pointing from the origin to the polygon vertex
 *!(in eye coordinates). 
 *!Let @i{n@} sup prime be the current normal,
 *!after transformation to eye coordinates. 
 *!Let 
 *!
 *!.ce
 *!f ~=~ ( f sub x~~f sub y~~f sub z ) sup T
 *!be the reflection vector such that
 *!
 *!.ce
 *!f ~=~  u ~-~ 2 n sup prime n sup prime sup T u
 *!
 *!
 *!Finally, let  m ~=~ 2 sqrt { f sub x sup {~2} + f sub y sup {~2} + (f sub z + 1 ) sup 2}. 
 *!Then the values assigned to the s and t texture coordinates are
 *!
 *!.ce 1
 *!s ~=~ f sub x over m ~+~ 1 over 2
 *!.sp
 *!.ce 1
 *!t ~=~ f sub y over m ~+~ 1 over 2
 *!
 *!To enable or disable a texture-coordinate generation function, call
 *!@[glEnable] or @[glDisable]
 *!with one of the symbolic texture-coordinate names
 *!(@[GL_TEXTURE_GEN_S],
 *!@[GL_TEXTURE_GEN_T],
 *!@[GL_TEXTURE_GEN_R], or
 *!@[GL_TEXTURE_GEN_Q]) as the argument. 
 *!When enabled,
 *!the specified texture coordinate is computed
 *!according to the generating function associated with that coordinate. 
 *!When disabled,
 *!subsequent vertices take the specified texture coordinate
 *!from the current set of texture coordinates. Initially, all texture
 *!generation functions are set to @[GL_EYE_LINEAR] and are disabled.
 *!Both s plane equations are (1, 0, 0, 0),
 *!both t plane equations are (0, 1, 0, 0),
 *!and all r and q plane equations are (0, 0, 0, 0).
 *!
 *!@param coord
 *!
 *!Specifies a texture coordinate.
 *!Must be one of @[GL_S], @[GL_T], @[GL_R], or @[GL_Q].
 *!
 *!@param pname
 *!
 *!Specifies the symbolic name of the texture-coordinate generation function.
 *!Must be @[GL_TEXTURE_GEN_MODE]. 
 *!
 *!@param param
 *!
 *!Specifies a single-valued texture generation parameter,
 *!one of @[GL_OBJECT_LINEAR], @[GL_EYE_LINEAR], or @[GL_SPHERE_MAP]. 
 *!
 *!
 *!@param coord
 *!
 *!Specifies a texture coordinate.
 *!Must be one of @[GL_S], @[GL_T], @[GL_R], or @[GL_Q].
 *!
 *!@param pname
 *!
 *!Specifies the symbolic name of the texture-coordinate generation function
 *!or function parameters.
 *!Must be
 *!@[GL_TEXTURE_GEN_MODE],
 *!@[GL_OBJECT_PLANE], or
 *!@[GL_EYE_PLANE]. 
 *!
 *!@param params
 *!
 *!Specifies a pointer to an array of texture generation parameters.
 *!If @i{pname@} is @[GL_TEXTURE_GEN_MODE],
 *!then the array must contain a single symbolic constant,
 *!one of
 *!@[GL_OBJECT_LINEAR],
 *!@[GL_EYE_LINEAR], or
 *!@[GL_SPHERE_MAP]. 
 *!Otherwise,
 *!@i{params@} holds the coefficients for the texture-coordinate generation function
 *!specified by @i{pname@}.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated when @i{coord@} or @i{pname@} is not an
 *!accepted defined value,
 *!or when @i{pname@} is @[GL_TEXTURE_GEN_MODE] and @i{params@} is not an
 *!accepted defined value.
 *!
 *!@[GL_INVALID_ENUM] is generated when @i{pname@} is @[GL_TEXTURE_GEN_MODE],
 *!@i{params@} is @[GL_SPHERE_MAP],
 *!and @i{coord@} is either @[GL_R] or @[GL_Q].
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glTexGen]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glTexImage2D(int target, int level, int internalformat, object|mapping(string:object) width, object|mapping(string:object) height, int border, object|mapping(string:object) format, object|mapping(string:object) type, object|mapping(string:object) *pixels)
 *!
 *!Texturing maps a portion of a specified texture image
 *!onto each graphical primitive for which texturing is enabled.
 *!To enable and disable two-dimensional texturing, call @[glEnable]
 *!and @[glDisable] with argument @[GL_TEXTURE_2D].
 *!
 *!To define texture images, call @[glTexImage2D]. 
 *!The arguments describe the parameters of the texture image,
 *!such as height,
 *!width,
 *!width of the border,
 *!level-of-detail number
 *!(see @[glTexParameter]),
 *!and number of color components provided.
 *!The last three arguments describe how the image is represented in memory;
 *!they are identical to the pixel formats used for @[glDrawPixels].
 *!
 *!If @i{target@} is @[GL_PROXY_TEXTURE_2D], no data is read from @i{pixels@}, but
 *!all of the texture image state is recalculated, checked for
 *!consistency, and checked 
 *!against the implementation's capabilities. If the implementation cannot
 *!handle a texture of the requested texture size, it sets
 *!all of the image state to 0,
 *!but does not generate an error (see @[glGetError]). To query for an
 *!entire mipmap array, use an image array level greater than or equal to
 *!1. 
 *!.P 
 *!If @i{target@} is @[GL_TEXTURE_2D],
 *!data is read from @i{pixels@} as a sequence of signed or unsigned bytes,
 *!shorts,
 *!or longs,
 *!or single-precision floating-point values,
 *!depending on @i{type@}. 
 *!These values are grouped into sets of one,
 *!two,
 *!three,
 *!or four values,
 *!depending on @i{format@},
 *!to form elements. 
 *!If @i{type@} is @[GL_BITMAP],
 *!the data is considered as a string of unsigned bytes (and
 *!@i{format@} must be @[GL_COLOR_INDEX]). 
 *!Each data byte is treated as eight 1-bit elements,
 *!with bit ordering determined by @[GL_UNPACK_LSB_FIRST]
 *!(see @[glPixelStore]).
 *!
 *!The first element corresponds to the lower left corner of the texture
 *!image.
 *!Subsequent elements progress left-to-right through the remaining texels
 *!in the lowest row of the texture image, and then in successively higher
 *!rows of the texture image.
 *!The final element corresponds to the upper right corner of the texture
 *!image.
 *!
 *!@i{format@} determines the composition of each element in @i{pixels@}.
 *!It can assume one of nine symbolic values:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_COLOR_INDEX</ref>
 *!</c><c>Each element is a single value,
 *!a color index. 
 *!The GL converts it to fixed point
 *!(with an unspecified number of zero bits to the right of the binary point),
 *!shifted left or right depending on the value and sign of <ref>GL_INDEX_SHIFT</ref>,
 *!and added to <ref>GL_INDEX_OFFSET</ref>
 *!(see 
 *!
 *!<ref>glPixelTransfer</ref>). 
 *!The resulting index is converted to a set of color components
 *!using the
 *!<ref>GL_PIXEL_MAP_I_TO_R</ref>,
 *!<ref>GL_PIXEL_MAP_I_TO_G</ref>,
 *!<ref>GL_PIXEL_MAP_I_TO_B</ref>, and
 *!<ref>GL_PIXEL_MAP_I_TO_A</ref> tables,
 *!and clamped to the range [0,1].
 *!</c></r>
 *!<r><c><ref>GL_RED</ref>
 *!</c><c>Each element is a single red component. 
 *!The GL converts it to floating point and assembles it into an RGBA element
 *!by attaching 0 for green and blue, and 1 for alpha. 
 *!Each component is then multiplied by the signed scale factor <ref>GL_c_SCALE</ref>,
 *!added to the signed bias <ref>GL_c_BIAS</ref>,
 *!and clamped to the range [0,1]
 *!(see <ref>glPixelTransfer</ref>).
 *!</c></r>
 *!<r><c><ref>GL_GREEN</ref>
 *!</c><c>Each element is a single green component. 
 *!The GL converts it to floating point and assembles it into an RGBA element
 *!by attaching 0 for red and blue, and 1 for alpha. 
 *!Each component is then multiplied by the signed scale factor <ref>GL_c_SCALE</ref>,
 *!added to the signed bias <ref>GL_c_BIAS</ref>,
 *!and clamped to the range [0,1]
 *!(see <ref>glPixelTransfer</ref>).
 *!</c></r>
 *!<r><c><ref>GL_BLUE</ref>
 *!</c><c>Each element is a single blue component. 
 *!The GL converts it to floating point and assembles it into an RGBA element
 *!by attaching 0 for red and green, and 1 for alpha. 
 *!Each component is then multiplied by the signed scale factor <ref>GL_c_SCALE</ref>,
 *!added to the signed bias <ref>GL_c_BIAS</ref>,
 *!and clamped to the range [0,1]
 *!(see <ref>glPixelTransfer</ref>).
 *!</c></r>
 *!<r><c><ref>GL_ALPHA</ref>
 *!</c><c>Each element is a single alpha component. 
 *!The GL converts it to floating point and assembles it into an RGBA element
 *!by attaching 0 for red, green, and blue.
 *!Each component is then multiplied by the signed scale factor <ref>GL_c_SCALE</ref>,
 *!added to the signed bias <ref>GL_c_BIAS</ref>,
 *!and clamped to the range [0,1]
 *!(see <ref>glPixelTransfer</ref>).
 *!</c></r>
 *!<r><c><ref>GL_RGB</ref>
 *!</c><c>Each element is an RGB triple.
 *!The GL converts it to floating point and assembles it into an RGBA element
 *!by attaching 1 for alpha.
 *!Each component is then multiplied by the signed scale factor <ref>GL_c_SCALE</ref>,
 *!added to the signed bias <ref>GL_c_BIAS</ref>,
 *!and clamped to the range [0,1]
 *!(see 
 *!
 *!<ref>glPixelTransfer</ref>).
 *!</c></r>
 *!<r><c><ref>GL_RGBA</ref>
 *!</c><c>Each element contains all four components.
 *!Each component is multiplied by the signed scale factor <ref>GL_c_SCALE</ref>,
 *!added to the signed bias <ref>GL_c_BIAS</ref>,
 *!and clamped to the range [0,1]
 *!(see <ref>glPixelTransfer</ref>).
 *!</c></r>
 *!<r><c><ref>GL_LUMINANCE</ref>
 *!</c><c>Each element is a single luminance value.
 *!The GL converts it to floating point,
 *!then assembles it into an RGBA element by replicating the luminance value
 *!three times for red, green, and blue and attaching 1 for alpha. 
 *!Each component is then multiplied by the signed scale factor <ref>GL_c_SCALE</ref>,
 *!added to the signed bias <ref>GL_c_BIAS</ref>,
 *!and clamped to the range [0,1]
 *!(see <ref>glPixelTransfer</ref>).
 *!</c></r>
 *!<r><c><ref>GL_LUMINANCE_ALPHA</ref>
 *!</c><c>Each element is a luminance/alpha pair.
 *!The GL converts it to floating point,
 *!then assembles it into an RGBA element by replicating the luminance value
 *!three times for red, green, and blue.
 *!Each component is then multiplied by the signed scale factor <ref>GL_c_SCALE</ref>,
 *!added to the signed bias <ref>GL_c_BIAS</ref>,
 *!and clamped to the range [0,1]
 *!(see 
 *!
 *!<ref>glPixelTransfer</ref>).
 *!</c></r>
 *!</matrix>@}
 *!
 *!Refer to the @[glDrawPixels] reference page for a description of
 *!the acceptable values for the @i{type@} parameter.
 *!
 *!If an application wants to store the texture at a certain
 *!resolution or in a certain format, it can request the resolution
 *!and format with @i{internalformat@}. The GL will choose an internal
 *!representation that closely approximates that requested by @i{internalformat@}, but
 *!it may not match exactly.
 *!(The representations specified by @[GL_LUMINANCE],
 *!@[GL_LUMINANCE_ALPHA], @[GL_RGB],
 *!and @[GL_RGBA] must match exactly. The numeric values 1, 2, 3, and 4
 *!may also be used to specify the above representations.)
 *!
 *!Use the @[GL_PROXY_TEXTURE_2D] target to try out a resolution and
 *!format. The implementation will
 *!update and recompute its best match for the requested storage resolution
 *!and format. To then query this state, call
 *!@[glGetTexLevelParameter].
 *!If the texture cannot be accommodated, texture state is set to 0.
 *!
 *!A one-component texture image uses only the red component of the RGBA
 *!color extracted from @i{pixels@}. 
 *!A two-component image uses the R and A values.
 *!A three-component image uses the R, G, and B values.
 *!A four-component image uses all of the RGBA components. 
 *!
 *!@param target
 *!
 *!Specifies the target texture.
 *!Must be @[GL_TEXTURE_2D] or @[GL_PROXY_TEXTURE_2D].
 *!
 *!@param level
 *!
 *!Specifies the level-of-detail number.
 *!Level 0 is the base image level.
 *!Level @i{n@} is the @i{n@}th mipmap reduction image.
 *!
 *!@param internalformat
 *!
 *!Specifies the number of color components in the texture.
 *!Must be 1, 2, 3, or 4, or one of the following symbolic constants:
 *!@[GL_ALPHA],
 *!@[GL_ALPHA4],
 *!@[GL_ALPHA8],
 *!@[GL_ALPHA12],
 *!@[GL_ALPHA16],
 *!@[GL_LUMINANCE],
 *!@[GL_LUMINANCE4],
 *!@[GL_LUMINANCE8],
 *!@[GL_LUMINANCE12],
 *!@[GL_LUMINANCE16],
 *!@[GL_LUMINANCE_ALPHA],
 *!@[GL_LUMINANCE4_ALPHA4],
 *!@[GL_LUMINANCE6_ALPHA2],
 *!@[GL_LUMINANCE8_ALPHA8],
 *!@[GL_LUMINANCE12_ALPHA4],
 *!@[GL_LUMINANCE12_ALPHA12],
 *!@[GL_LUMINANCE16_ALPHA16],
 *!@[GL_INTENSITY],
 *!@[GL_INTENSITY4],
 *!@[GL_INTENSITY8],
 *!@[GL_INTENSITY12],
 *!@[GL_INTENSITY16],
 *!@[GL_R3_G3_B2],
 *!@[GL_RGB],
 *!@[GL_RGB4],
 *!@[GL_RGB5],
 *!@[GL_RGB8],
 *!@[GL_RGB10],
 *!@[GL_RGB12],
 *!@[GL_RGB16],
 *!@[GL_RGBA],
 *!@[GL_RGBA2],
 *!@[GL_RGBA4],
 *!@[GL_RGB5_A1],
 *!@[GL_RGBA8],
 *!@[GL_RGB10_A2],
 *!@[GL_RGBA12], or
 *!@[GL_RGBA16].
 *!
 *!@param width
 *!
 *!Specifies the width of the texture image.
 *!Must be 2 sup n + 2 ( "border" ) for some integer n. All
 *!implementations support texture images that are at least 64 texels
 *!wide.
 *!
 *!@param height
 *!
 *!Specifies the height of the texture image.
 *!Must be 2 sup m + 2 ( "border" ) for some integer m. All
 *!implementations support texture images that are at least 64 texels
 *!high.
 *!
 *!@param border
 *!
 *!Specifies the width of the border.
 *!Must be either 0 or 1.
 *!
 *!@param format
 *!
 *!Specifies the format of the pixel data.
 *!The following symbolic values are accepted:
 *!@[GL_COLOR_INDEX],
 *!@[GL_RED],
 *!@[GL_GREEN],
 *!@[GL_BLUE],
 *!@[GL_ALPHA],
 *!@[GL_RGB],
 *!@[GL_RGBA],
 *!@[GL_LUMINANCE], and
 *!@[GL_LUMINANCE_ALPHA].
 *!
 *!@param type
 *!
 *!Specifies the data type of the pixel data.
 *!The following symbolic values are accepted:
 *!@[GL_UNSIGNED_BYTE],
 *!@[GL_BYTE],
 *!@[GL_BITMAP],
 *!@[GL_UNSIGNED_SHORT],
 *!@[GL_SHORT],
 *!@[GL_UNSIGNED_INT],
 *!@[GL_INT], and
 *!@[GL_FLOAT].
 *!
 *!@param pixels
 *!
 *!Specifies a pointer to the image data in memory.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{target@} is not @[GL_TEXTURE_2D]
 *!or @[GL_PROXY_TEXTURE_2D].
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{format@} is not an accepted
 *!format constant.
 *!Format constants other than @[GL_STENCIL_INDEX] and @[GL_DEPTH_COMPONENT]
 *!are accepted.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{type@} is not a type constant.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *!@i{format@} is not @[GL_COLOR_INDEX].
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *!.P 
 *!.P 
 *!@[GL_INVALID_VALUE] may be generated if @i{level@} is greater than $log
 *!sub 2$@i{max@},
 *!where @i{max@} is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{internalformat@} is not 1, 2, 3, 4, or one of the 
 *!accepted resolution and format symbolic constants.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{width@} or @i{height@} is less than 0
 *!or greater than 2 + @[GL_MAX_TEXTURE_SIZE],
 *!or if either cannot be represented as 2 sup k + 2("border") for some
 *!integer value of @i{k@}.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{border@} is not 0 or 1.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glTexImage2D]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glTexParameter(int target, int pname, float|int|array(float|int) param)
 *!
 *!Texture mapping is a technique that applies an image onto an object's surface
 *!as if the image were a decal or cellophane shrink-wrap. 
 *!The image is created in texture space,
 *!with an (s, t) coordinate system. 
 *!A texture is a one- or two-dimensional image and a set of parameters
 *!that determine how samples are derived from the image.
 *!
 *!@[glTexParameter] assigns the value or values in @i{params@} to the texture parameter
 *!specified as @i{pname@}. 
 *!@i{target@} defines the target texture,
 *!either @[GL_TEXTURE_1D] or @[GL_TEXTURE_2D].
 *!The following symbols are accepted in @i{pname@}:
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_TEXTURE_MIN_FILTER</ref>
 *!</c><c>The texture minifying function is used whenever the pixel being textured
 *!maps to an area greater than one texture element. 
 *!There are six defined minifying functions.
 *!Two of them use the nearest one or nearest four texture elements
 *!to compute the texture value. 
 *!The other four use mipmaps.
 *!
 *!A mipmap is an ordered set of arrays representing the same image
 *!at progressively lower resolutions. 
 *!If the texture has dimensions 2 sup n times 2 sup m, there are
 *! bold max ( n, m ) + 1  mipmaps. 
 *!The first mipmap is the original texture,
 *!with dimensions 2 sup n times 2 sup m. 
 *!Each subsequent mipmap has dimensions 2 sup { k - 1 } times 2 sup { l - 1 },
 *!where 2 sup k times 2 sup l are the dimensions of the previous mipmap,
 *!until either k = 0 or l=0.
 *!At that point,
 *!subsequent mipmaps have dimension  1 times 2 sup { l - 1 } 
 *!or  2 sup { k - 1} times 1  until the final mipmap,
 *!which has dimension 1 times 1. 
 *!To define the mipmaps, call <ref>glTexImage1D</ref>, <ref>glTexImage2D</ref>,
 *!<ref>glCopyTexImage1D</ref>, or <ref>glCopyTexImage2D</ref>
 *!with the <i>level</i> argument indicating the order of the mipmaps.
 *!Level 0 is the original texture;
 *!level  bold max ( n, m )  is the final 1 times 1 mipmap.
 *!
 *!<i>params</i> supplies a function for minifying the texture as one of the following:
 *!.RS 10
 *!</c></r>
 *!<r><c><ref>GL_NEAREST</ref>
 *!</c><c>Returns the value of the texture element that is nearest
 *!(in Manhattan distance)
 *!to the center of the pixel being textured.
 *!</c></r>
 *!<r><c><ref>GL_LINEAR</ref>
 *!</c><c>Returns the weighted average of the four texture elements
 *!that are closest to the center of the pixel being textured.
 *!These can include border texture elements,
 *!depending on the values of <ref>GL_TEXTURE_WRAP_S</ref> and <ref>GL_TEXTURE_WRAP_T</ref>,
 *!and on the exact mapping.
 *!</c></r>
 *!<r><c><ref>GL_NEAREST_MIPMAP_NEAREST</ref>
 *!</c><c>Chooses the mipmap that most closely matches the size of the pixel
 *!being textured and uses the <ref>GL_NEAREST</ref> criterion
 *!(the texture element nearest to the center of the pixel)
 *!to produce a texture value.
 *!</c></r>
 *!<r><c><ref>GL_LINEAR_MIPMAP_NEAREST</ref>
 *!</c><c>Chooses the mipmap that most closely matches the size of the pixel
 *!being textured and uses the <ref>GL_LINEAR</ref> criterion
 *!(a weighted average of the four texture elements that are closest
 *!to the center of the pixel)
 *!to produce a texture value.
 *!</c></r>
 *!<r><c><ref>GL_NEAREST_MIPMAP_LINEAR</ref>
 *!</c><c>Chooses the two mipmaps that most closely match the size of the pixel
 *!being textured and uses the <ref>GL_NEAREST</ref> criterion
 *!(the texture element nearest to the center of the pixel)
 *!to produce a texture value from each mipmap. 
 *!The final texture value is a weighted average of those two values.
 *!</c></r>
 *!<r><c><ref>GL_LINEAR_MIPMAP_LINEAR</ref>
 *!</c><c>Chooses the two mipmaps that most closely match the size of the pixel
 *!being textured and uses the <ref>GL_LINEAR</ref> criterion
 *!(a weighted average of the four texture elements that are closest
 *!to the center of the pixel)
 *!to produce a texture value from each mipmap.
 *!The final texture value is a weighted average of those two values.
 *!.RE
 *!
 *!As more texture elements are sampled in the minification process,
 *!fewer aliasing artifacts will be apparent. 
 *!While the <ref>GL_NEAREST</ref> and <ref>GL_LINEAR</ref> minification functions can be
 *!faster than the other four,
 *!they sample only one or four texture elements to determine the texture value
 *!of the pixel being rendered and can produce moire patterns
 *!or ragged transitions. 
 *!The initial value of <ref>GL_TEXTURE_MIN_FILTER</ref> is
 *!<ref>GL_NEAREST_MIPMAP_LINEAR</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_MAG_FILTER</ref>
 *!</c><c>The texture magnification function is used when the pixel being textured
 *!maps to an area less than or equal to one texture element.
 *!It sets the texture magnification function to either <ref>GL_NEAREST</ref>
 *!or <ref>GL_LINEAR</ref> (see below). <ref>GL_NEAREST</ref> is generally faster
 *!than <ref>GL_LINEAR</ref>, 
 *!but it can produce textured images with sharper edges
 *!because the transition between texture elements is not as smooth. 
 *!The initial value of <ref>GL_TEXTURE_MAG_FILTER</ref> is <ref>GL_LINEAR</ref>.
 *!.RS 10
 *!</c></r>
 *!<r><c><ref>GL_NEAREST</ref>
 *!</c><c>Returns the value of the texture element that is nearest
 *!(in Manhattan distance)
 *!to the center of the pixel being textured.
 *!</c></r>
 *!<r><c><ref>GL_LINEAR</ref>
 *!</c><c>Returns the weighted average of the four texture elements
 *!that are closest to the center of the pixel being textured.
 *!These can include border texture elements,
 *!depending on the values of <ref>GL_TEXTURE_WRAP_S</ref> and <ref>GL_TEXTURE_WRAP_T</ref>,
 *!and on the exact mapping.
 *!</c></r>
 *!</matrix>@}
 *!
 *!.RE
 *!
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_TEXTURE_WRAP_S</ref>
 *!</c><c>Sets the wrap parameter for texture coordinate <i>s</i> to either
 *!<ref>GL_CLAMP</ref> or <ref>GL_REPEAT</ref>.
 *!<ref>GL_CLAMP</ref> causes s coordinates to be clamped to the range [0,1]
 *!and is useful for preventing wrapping artifacts when mapping
 *!a single image onto an object. 
 *!<ref>GL_REPEAT</ref> causes the integer part of the s coordinate to be ignored;
 *!the GL uses only the fractional part,
 *!thereby creating a repeating pattern. 
 *!Border texture elements are accessed only if wrapping is set to <ref>GL_CLAMP</ref>.
 *!Initially, <ref>GL_TEXTURE_WRAP_S</ref> is set to <ref>GL_REPEAT</ref>.
 *!</c></r>
 *!</matrix>@}
 *!
 *!
 *!@xml{<matrix>
 *!<r><c><ref>GL_TEXTURE_WRAP_T</ref>
 *!</c><c>Sets the wrap parameter for texture coordinate <i>t</i> to either
 *!<ref>GL_CLAMP</ref> or <ref>GL_REPEAT</ref>.
 *!See the discussion under <ref>GL_TEXTURE_WRAP_S</ref>. 
 *!Initially, <ref>GL_TEXTURE_WRAP_T</ref> is set to <ref>GL_REPEAT</ref>.
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_BORDER_COLOR</ref>
 *!</c><c>Sets a border color.
 *!<i>params</i> contains four values that comprise the RGBA color
 *!of the texture border. 
 *!Integer color components are interpreted linearly such that the most
 *!positive integer maps to 1.0,
 *!and the most negative integer maps to -1.0.
 *!The values are clamped to the range [0,1] when they are specified.
 *!Initially, the border color is (0, 0, 0, 0).
 *!</c></r>
 *!<r><c><ref>GL_TEXTURE_PRIORITY</ref>
 *!</c><c>Specifies the texture residence priority of the currently bound texture.
 *!Permissible values are in the range [0,\ 1].
 *!See <ref>glPrioritizeTextures</ref> and <ref>glBindTexture</ref> for more information.
 *!</c></r></matrix>@}
 *!
 *!
 *!@param target
 *!
 *!Specifies the target texture,
 *!which must be either @[GL_TEXTURE_1D] or @[GL_TEXTURE_2D].
 *!
 *!@param pname
 *!
 *!Specifies the symbolic name of a single-valued texture parameter.
 *!@i{pname@} can be one of the following:
 *!@[GL_TEXTURE_MIN_FILTER],
 *!@[GL_TEXTURE_MAG_FILTER],
 *!@[GL_TEXTURE_WRAP_S], 
 *!@[GL_TEXTURE_WRAP_T], or
 *!@[GL_TEXTURE_PRIORITY].
 *!
 *!@param param
 *!
 *!Specifies the value of @i{pname@}.
 *!
 *!
 *!@param target
 *!
 *!Specifies the target texture,
 *!which must be either @[GL_TEXTURE_1D] or @[GL_TEXTURE_2D].
 *!
 *!@param pname
 *!
 *!Specifies the symbolic name of a texture parameter.
 *!@i{pname@} can be one of the following:
 *!@[GL_TEXTURE_MIN_FILTER],
 *!@[GL_TEXTURE_MAG_FILTER],
 *!@[GL_TEXTURE_WRAP_S],
 *!@[GL_TEXTURE_WRAP_T], 
 *!@[GL_TEXTURE_BORDER_COLOR], or
 *!@[GL_TEXTURE_PRIORITY].
 *!
 *!@param params
 *!
 *!Specifies a pointer to an array where the value or values of @i{pname@} are stored.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{target@} or @i{pname@} is not
 *!one of the accepted defined values.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{params@} should have a defined
 *!constant value (based on the value of @i{pname@}) and does not.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glTexParameter]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glTexSubImage2D(int target, int level, int xoffset, int yoffset, object|mapping(string:object) width, object|mapping(string:object) height, object|mapping(string:object) format, object|mapping(string:object) type, object|mapping(string:object) *pixels)
 *!
 *!Texturing maps a portion of a specified texture image
 *!onto each graphical primitive for which texturing is enabled.
 *!To enable and disable two-dimensional texturing, call @[glEnable]
 *!and @[glDisable] with argument @[GL_TEXTURE_2D].
 *!
 *!@[glTexSubImage2D] redefines a contiguous subregion of an existing two-dimensional
 *!texture image.
 *!The texels referenced by @i{pixels@} replace the portion of the
 *!existing texture array with x indices @i{xoffset@} and "xoffset"~+~"width"~-~1,
 *!inclusive,
 *!and y indices @i{yoffset@} and "yoffset"~+~"height"~-~1, inclusive. 
 *!This region may not include any texels outside the range of the
 *!texture array as it was originally specified.
 *!It is not an error to specify a subtexture with zero width or height, but
 *!such a specification has no effect.
 *!
 *!@param target
 *!
 *!Specifies the target texture.
 *!Must be @[GL_TEXTURE_2D].
 *!
 *!@param level
 *!
 *!Specifies the level-of-detail number.
 *!Level 0 is the base image level.
 *!Level @i{n@} is the @i{n@}th mipmap reduction image.
 *!
 *!@param xoffset
 *!
 *!Specifies a texel offset in the x direction within the texture array.
 *!
 *!@param yoffset
 *!
 *!Specifies a texel offset in the y direction within the texture array.
 *!
 *!@param width
 *!
 *!Specifies the width of the texture subimage.
 *!
 *!@param height
 *!
 *!Specifies the height of the texture subimage.
 *!
 *!@param format
 *!
 *!Specifies the format of the pixel data.
 *!The following symbolic values are accepted:
 *!@[GL_COLOR_INDEX],
 *!@[GL_RED],
 *!@[GL_GREEN],
 *!@[GL_BLUE],
 *!@[GL_ALPHA],
 *!@[GL_RGB],
 *!@[GL_RGBA],
 *!@[GL_LUMINANCE], and
 *!@[GL_LUMINANCE_ALPHA].
 *!
 *!@param type
 *!
 *!Specifies the data type of the pixel data.
 *!The following symbolic values are accepted:
 *!@[GL_UNSIGNED_BYTE],
 *!@[GL_BYTE],
 *!@[GL_BITMAP],
 *!@[GL_UNSIGNED_SHORT],
 *!@[GL_SHORT],
 *!@[GL_UNSIGNED_INT],
 *!@[GL_INT], and
 *!@[GL_FLOAT].
 *!
 *!@param pixels
 *!
 *!Specifies a pointer to the image data in memory.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{target@} is not @[GL_TEXTURE_2D]. 
 *!
 *!@[GL_INVALID_OPERATION] is generated if the texture array has not
 *!been defined by a previous @[glTexImage2D] operation.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *!.P 
 *!P 
 *!@[GL_INVALID_VALUE] may be generated if @i{level@} is greater
 *!than log sub 2@i{max@},
 *!where @i{max@} is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *!
 *!@[GL_INVALID_VALUE] is generated if "xoffset" ~<~ ~-b,
 *!("xoffset"~+~"width") ~>~ (w~-~b), 
 *!"yoffset" ~<~ ~-b, or ("yoffset" ~+~ "height") ~>~ (h~-~b).
 *!Where w is the @[GL_TEXTURE_WIDTH], 
 *!h is the @[GL_TEXTURE_HEIGHT], and b is the border width 
 *!of the texture image being modified.
 *!Note that w and h include twice the border width.
 *!
 *!@[GL_INVALID_VALUE] is generated if @i{width@} or @i{height@} is less than 0.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{format@} is not an accepted
 *!format constant.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{type@} is not a type constant.
 *!
 *!@[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *!@i{format@} is not @[GL_COLOR_INDEX].
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glTexSubImage2D] is executed
 *!between the execution of @[glBegin] and the corresponding
 *!execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glTranslate(float|int|array(float|int) x, float|int|void y, float|int|void z)
 *!
 *!@[glTranslate] produces a translation by 
 *!("x","y","z").
 *!The current matrix
 *!(see 
 *!
 *!@[glMatrixMode])
 *!is multiplied by this translation matrix,
 *!with the product replacing the current matrix, as if
 *!@[glMultMatrix] were called with the following matrix
 *!for its argument:
 *!.sp
 *!.ce
 *!.EQ
 *!left (  ~ down 20 matrix {
 *!   ccol { 1~~ above 0~~ above 0~~ above 0~~ }
 *!   ccol { 0~~ above 1~~ above 0~~ above 0~~ }
 *!   ccol { 0~~ above 0~~ above 1~~ above 0~~ }
 *!   ccol { "x"~ above "y"~ above "z"~ above 1}
 *!} ~~right )
 *!.EN
 *!.sp
 *!.RE
 *!If the matrix mode is either @[GL_MODELVIEW] or @[GL_PROJECTION],
 *!all objects drawn after a call to @[glTranslate] are translated.
 *!
 *!Use @[glPushMatrix] and 
 *!@[glPopMatrix] to save and restore
 *!the untranslated coordinate system.
 *!
 *!@param x
 *!
 *!Specify the @i{x@}, @i{y@}, and @i{z@} coordinates of a translation vector.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glTranslate]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
 */

/*!@decl void glVertex(float|int|array(float|int) x, float|int|void y, float|int|void z, float|int|void w)
 *!
 *!@[glVertex] commands are used within @[glBegin]/@[glEnd] pairs to specify
 *!point, line, and polygon vertices.
 *!The current color,
 *!normal,
 *!and texture coordinates are associated with the vertex when @[glVertex] is called.
 *!
 *!When only @i{x@} and @i{y@} are specified,
 *!@i{z@} defaults to 0 and @i{w@} defaults to 1.
 *!When @i{x, y,@} and @i{z@} are specified,
 *!@i{w@} defaults to 1.
 *!
 *!@param x
 *!
 *!Specify @i{x@}, @i{y@}, @i{z@}, and @i{w@} coordinates of a vertex.
 *!Not all parameters are present in all forms of the command.
 *!
 *!
 *!@param v
 *!
 *!Specifies a pointer to an array of two, three, or four elements.
 *!The elements of a two-element array are @i{x@} and @i{y@};
 *!of a three-element array, @i{x@}, @i{y@}, and @i{z@};
 *!and of a four-element array, @i{x@}, @i{y@}, @i{z@}, and @i{w@}.
 *!
 *!
 */

/*!@decl void glViewport(int x, int y, int width, int height)
 *!
 *!@[glViewport] specifies the affine transformation of x and y from
 *!normalized device coordinates to window coordinates.
 *!Let (x sub nd, y sub nd) be normalized device coordinates.
 *!Then the window coordinates (x sub w, y sub w) are computed as follows:
 *!.sp
 *!.ce
 *!.EQ
 *!x sub w ~=~ ( x sub nd + 1 ) left ( "width" over 2 right ) ~+~ "x"
 *!.EN
 *!.sp
 *!.ce
 *!.EQ
 *!y sub w ~=~ ( y sub nd + 1 ) left ( "height" over 2 right ) ~+~ "y"
 *!.EN
 *!.RE
 *!
 *!Viewport width and height are silently clamped
 *!to a range that depends on the implementation.
 *!To query this range, call @[glGet] with argument
 *!@[GL_MAX_VIEWPORT_DIMS].
 *!
 *!@param x
 *!
 *!Specify the lower left corner of the viewport rectangle,
 *!in pixels. The initial value is (0,0).
 *!
 *!@param width
 *!
 *!Specify the width and height
 *!of the viewport.
 *!When a GL context is first attached to a window,
 *!@i{width@} and @i{height@} are set to the dimensions of that window.
 *!
 *!@throws
 *!
 *!@[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@} is negative.
 *!
 *!@[GL_INVALID_OPERATION] is generated if @[glViewport]
 *!is executed between the execution of @[glBegin]
 *!and the corresponding execution of @[glEnd].
 *!
 *!
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
 *!
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
 *!
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
 *!
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
 *!
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
 *!
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
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate RGBA values.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_INDEX</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate color indices.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_NORMAL</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate normals.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_1</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>
 *!texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_2</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i> and
 *!<i>t</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>,
 *!<i>t</i>, and
 *!<i>r</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>,
 *!<i>t</i>,
 *!<i>r</i>, and
 *!<i>q</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_VERTEX_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>x</i>, <i>y</i>, and <i>z</i> vertex coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_VERTEX_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
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
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate RGBA values.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_INDEX</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate color indices.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_NORMAL</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate normals.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_1</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>
 *!texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_2</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i> and
 *!<i>t</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>,
 *!<i>t</i>, and
 *!<i>r</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>,
 *!<i>t</i>,
 *!<i>r</i>, and
 *!<i>q</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_VERTEX_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>x</i>, <i>y</i>, and <i>z</i> vertex coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_VERTEX_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
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
 *!
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
 *!
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
 *!<r><c>GL_AUX<i>i</i>
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
 *!GL_AUX@i{i@},
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
 *!
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
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate RGBA values.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_INDEX</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate color indices.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_NORMAL</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate normals.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_1</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>
 *!texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_2</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i> and
 *!<i>t</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>,
 *!<i>t</i>, and
 *!<i>r</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_TEXTURE_COORD_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>,
 *!<i>t</i>,
 *!<i>r</i>, and
 *!<i>q</i> texture coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_VERTEX_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>x</i>, <i>y</i>, and <i>z</i> vertex coordinates.
 *!See <ref>glMap1</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP1_VERTEX_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh1</ref>, and
 *!<ref>glEvalPoint</ref> generate
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
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate RGBA values.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_INDEX</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate color indices.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_NORMAL</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate normals.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_1</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>
 *!texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_2</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i> and
 *!<i>t</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>,
 *!<i>t</i>, and
 *!<i>r</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_TEXTURE_COORD_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>s</i>,
 *!<i>t</i>,
 *!<i>r</i>, and
 *!<i>q</i> texture coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_VERTEX_3</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
 *!<i>x</i>, <i>y</i>, and <i>z</i> vertex coordinates.
 *!See <ref>glMap2</ref>.
 *!</c></r>
 *!<r><c><ref>GL_MAP2_VERTEX_4</ref>
 *!</c><c>If enabled,
 *!calls to
 *!<ref>glEvalCoord</ref>,
 *!<ref>glEvalMesh2</ref>, and
 *!<ref>glEvalPoint</ref> generate
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
 *!
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
 *!
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
 *!GL_AUX@i{i@},
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
 *!
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
 *!
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
 *!primitives from 1, the GL gives each flat-shaded line segment i the
 *!computed color of vertex i + 1, its second vertex.
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
 *!@i{primitive type of polygon@} i	@i{vertex@}
 *!=
 *!Single polygon ( i == 1 )	1
 *!Triangle strip	i + 2
 *!Triangle fan	i + 2
 *!Independent triangle	 3 i
 *!Quad strip	2 i + 2
 *!Independent quad 	 4 i 
 *!.TE
 *!.sp
 *!Flat and smooth shading are specified by @[glShadeModel] with @i{mode@} set to
 *!@[GL_FLAT] and @[GL_SMOOTH], respectively.
 *!
 *!@param mode
 *!
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

/*!@decl constant GL_2D 1536
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_2_BYTES 5127
 *! Used in @[glCallLists]
 */

/*!@decl constant GL_3D 1537
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_3D_COLOR 1538
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_3D_COLOR_TEXTURE 1539
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_3_BYTES 5128
 *! Used in @[glCallLists]
 */

/*!@decl constant GL_4D_COLOR_TEXTURE 1540
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_4_BYTES 5129
 *! Used in @[glCallLists]
 */

/*!@decl constant GL_ABGR_EXT 32768
 */

/*!@decl constant GL_ACCUM 256
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glAccum], @[glGetDoublev] and @[glClear]
 */

/*!@decl constant GL_ACCUM_ALPHA_BITS 3419
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glAccum] and @[glGetDoublev]
 */

/*!@decl constant GL_ACCUM_BLUE_BITS 3418
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glAccum] and @[glGetDoublev]
 */

/*!@decl constant GL_ACCUM_BUFFER_BIT 512
 *! Used in @[glPopAttrib], @[glPushAttrib] and @[glClear]
 */

/*!@decl constant GL_ACCUM_CLEAR_VALUE 2944
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_ACCUM_GREEN_BITS 3417
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glAccum] and @[glGetDoublev]
 */

/*!@decl constant GL_ACCUM_RED_BITS 3416
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glAccum] and @[glGetDoublev]
 */

/*!@decl constant GL_ADD 260
 *! Used in @[glAccum]
 */

/*!@decl constant GL_ALL_ATTRIB_BITS 1048575
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_ALPHA 6406
 *! Used in @[glIsEnabled], @[glTexEnv], @[glGetIntegerv], @[glDrawPixels], @[glDisable], @[glCopyTexImage1D], @[glTexSubImage2D], @[glPopAttrib], @[glEnable], @[glPixelTransfer], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glAlphaFunc], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_ALPHA12 32829
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_ALPHA16 32830
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_ALPHA4 32827
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_ALPHA8 32828
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_ALPHA_BIAS 3357
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_ALPHA_BITS 3413
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_ALPHA_SCALE 3356
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_ALPHA_TEST 3008
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glAlphaFunc]
 */

/*!@decl constant GL_ALPHA_TEST_FUNC 3009
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_ALPHA_TEST_REF 3010
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_ALWAYS 519
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glAlphaFunc], @[glDepthFunc] and @[glStencilFunc]
 */

/*!@decl constant GL_AMBIENT 4608
 *! Used in @[glGetIntegerv], @[glColorMaterial], @[glGetLight], @[glGetBooleanv], @[glGetFloatv], @[glMaterial], @[glGetDoublev], @[glLight] and @[glGetMaterial]
 */

/*!@decl constant GL_AMBIENT_AND_DIFFUSE 5634
 *! Used in @[glGetIntegerv], @[glColorMaterial], @[glGetBooleanv], @[glGetFloatv], @[glMaterial] and @[glGetDoublev]
 */

/*!@decl constant GL_AND 5377
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_AND_INVERTED 5380
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_AND_REVERSE 5378
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_ATTRIB_STACK_DEPTH 2992
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_AUTO_NORMAL 3456
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_AUX0 1033
 *! Used in @[glReadBuffer]
 */

/*!@decl constant GL_AUX1 1034
 */

/*!@decl constant GL_AUX2 1035
 */

/*!@decl constant GL_AUX3 1036
 *! Used in @[glReadBuffer]
 */

/*!@decl constant GL_AUX_BUFFERS 3072
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glReadBuffer], @[glGetDoublev] and @[glDrawBuffer]
 */

/*!@decl constant GL_AVERAGE_EXT 33589
 */

/*!@decl constant GL_BACK 1029
 *! Used in @[glGetIntegerv], @[glColorMaterial], @[glCullFace], @[glPolygonMode], @[glGetBooleanv], @[glGetFloatv], @[glReadBuffer], @[glMaterial], @[glGetDoublev], @[glDrawBuffer] and @[glGetMaterial]
 */

/*!@decl constant GL_BACK_LEFT 1026
 *! Used in @[glReadBuffer] and @[glDrawBuffer]
 */

/*!@decl constant GL_BACK_RIGHT 1027
 *! Used in @[glReadBuffer] and @[glDrawBuffer]
 */

/*!@decl constant GL_BGR 32992
 */

/*!@decl constant GL_BGRA 32993
 */

/*!@decl constant GL_BITMAP 6656
 *! Used in @[glDrawPixels], @[glTexSubImage2D], @[glGetTexImage], @[glReadPixels], @[glTexImage2D], @[glFeedbackBuffer], @[glGetPolygonStipple], @[glBitmap], @[glTexSubImage1D], @[glTexImage1D] and @[glPolygonStipple]
 */

/*!@decl constant GL_BITMAP_TOKEN 1796
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_BLEND 3042
 *! Used in @[glIsEnabled], @[glTexEnv], @[glGetIntegerv], @[glBlendColorEXT], @[glBlendFunc], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_BLEND_COLOR_EXT 32773
 *! Used in @[glGetIntegerv], @[glBlendColorEXT], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_BLEND_DST 3040
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_BLEND_EQUATION_EXT 32777
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_BLEND_SRC 3041
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_BLUE 6405
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glTexSubImage2D], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_BLUE_BIAS 3355
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_BLUE_BITS 3412
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_BLUE_SCALE 3354
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_BYTE 5120
 *! Used in @[glDrawPixels], @[glColorPointer], @[glTexSubImage2D], @[glNormalPointer], @[glGetTexImage], @[glCallLists], @[glReadPixels], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_C3F_V3F 10788
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_C4F_N3F_V3F 10790
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_C4UB_V2F 10786
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_C4UB_V3F 10787
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_CCW 2305
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glFrontFace]
 */

/*!@decl constant GL_CLAMP 10496
 *! Used in @[glTexParameter]
 */

/*!@decl constant GL_CLEAR 5376
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_CLIENT_ALL_ATTRIB_BITS 4294967295
 *! Used in @[glPopClientAttrib] and @[glPushClientAttrib]
 */

/*!@decl constant GL_CLIENT_ATTRIB_STACK_DEPTH 2993
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_CLIENT_PIXEL_STORE_BIT 1
 *! Used in @[glPopClientAttrib] and @[glPushClientAttrib]
 */

/*!@decl constant GL_CLIENT_VERTEX_ARRAY_BIT 2
 *! Used in @[glPopClientAttrib] and @[glPushClientAttrib]
 */

/*!@decl constant GL_CLIP_PLANE0 12288
 */

/*!@decl constant GL_CLIP_PLANE1 12289
 */

/*!@decl constant GL_CLIP_PLANE2 12290
 */

/*!@decl constant GL_CLIP_PLANE3 12291
 */

/*!@decl constant GL_CLIP_PLANE4 12292
 */

/*!@decl constant GL_CLIP_PLANE5 12293
 */

/*!@decl constant GL_COEFF 2560
 *! Used in @[glGetMap]
 */

/*!@decl constant GL_COLOR 6144
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDrawElements], @[glDrawArrays], @[glColorMaterial], @[glDrawPixels], @[glDisable], @[glColorPointer], @[glTexSubImage2D], @[glPopAttrib], @[glEnable], @[glEnableClientState], @[glGetPointerv], @[glPixelTransfer], @[glLogicOp], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glMaterial], @[glGetDoublev], @[glReadPixels], @[glTexImage2D], @[glGetPolygonStipple], @[glBitmap], @[glCopyPixels], @[glLightModel], @[glDisableClientState], @[glTexSubImage1D], @[glTexImage1D], @[glGetMaterial], @[glClear] and @[glPolygonStipple]
 */

/*!@decl constant GL_COLOR_ARRAY 32886
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDrawElements], @[glDrawArrays], @[glColorPointer], @[glEnableClientState], @[glGetPointerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glDisableClientState]
 */

/*!@decl constant GL_COLOR_ARRAY_POINTER 32912
 *! Used in @[glGetPointerv]
 */

/*!@decl constant GL_COLOR_ARRAY_SIZE 32897
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COLOR_ARRAY_STRIDE 32899
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COLOR_ARRAY_TYPE 32898
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COLOR_BUFFER_BIT 16384
 *! Used in @[glPopAttrib], @[glPushAttrib] and @[glClear]
 */

/*!@decl constant GL_COLOR_CLEAR_VALUE 3106
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COLOR_INDEX 6400
 *! Used in @[glDrawPixels], @[glTexSubImage2D], @[glPixelTransfer], @[glGetTexImage], @[glMaterial], @[glReadPixels], @[glTexImage2D], @[glGetPolygonStipple], @[glBitmap], @[glLightModel], @[glTexSubImage1D], @[glTexImage1D], @[glGetMaterial] and @[glPolygonStipple]
 */

/*!@decl constant GL_COLOR_INDEXES 5635
 *! Used in @[glMaterial], @[glLightModel] and @[glGetMaterial]
 */

/*!@decl constant GL_COLOR_LOGIC_OP 3058
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glLogicOp], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COLOR_MATERIAL 2903
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glColorMaterial], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COLOR_MATERIAL_FACE 2901
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COLOR_MATERIAL_PARAMETER 2902
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COLOR_TABLE_ALPHA_SIZE_SGI 32989
 */

/*!@decl constant GL_COLOR_TABLE_BIAS_SGI 32983
 */

/*!@decl constant GL_COLOR_TABLE_BLUE_SIZE_SGI 32988
 */

/*!@decl constant GL_COLOR_TABLE_FORMAT_SGI 32984
 */

/*!@decl constant GL_COLOR_TABLE_GREEN_SIZE_SGI 32987
 */

/*!@decl constant GL_COLOR_TABLE_INTENSITY_SIZE_SGI 32991
 */

/*!@decl constant GL_COLOR_TABLE_LUMINANCE_SIZE_SGI 32990
 */

/*!@decl constant GL_COLOR_TABLE_RED_SIZE_SGI 32986
 */

/*!@decl constant GL_COLOR_TABLE_SCALE_SGI 32982
 */

/*!@decl constant GL_COLOR_TABLE_SGI 32976
 */

/*!@decl constant GL_COLOR_TABLE_WIDTH_SGI 32985
 */

/*!@decl constant GL_COLOR_WRITEMASK 3107
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_COMPILE 4864
 *! Used in @[glNewList] and @[glEndList]
 */

/*!@decl constant GL_COMPILE_AND_EXECUTE 4865
 *! Used in @[glNewList] and @[glEndList]
 */

/*!@decl constant GL_COMPRESSED_GEOM_ACCELERATED_SUNX 33232
 */

/*!@decl constant GL_COMPRESSED_GEOM_VERSION_SUNX 33233
 */

/*!@decl constant GL_CONSTANT_ALPHA_EXT 32771
 */

/*!@decl constant GL_CONSTANT_ATTENUATION 4615
 *! Used in @[glGetLight] and @[glLight]
 */

/*!@decl constant GL_CONSTANT_BORDER_HP 33105
 */

/*!@decl constant GL_CONSTANT_COLOR_EXT 32769
 */

/*!@decl constant GL_CONVOLUTION_1D_EXT 32784
 */

/*!@decl constant GL_CONVOLUTION_2D_EXT 32785
 */

/*!@decl constant GL_CONVOLUTION_BORDER_COLOR_HP 33108
 */

/*!@decl constant GL_CONVOLUTION_BORDER_MODE_EXT 32787
 */

/*!@decl constant GL_CONVOLUTION_FILTER_BIAS_EXT 32789
 */

/*!@decl constant GL_CONVOLUTION_FILTER_SCALE_EXT 32788
 */

/*!@decl constant GL_CONVOLUTION_FORMAT_EXT 32791
 */

/*!@decl constant GL_CONVOLUTION_HEIGHT_EXT 32793
 */

/*!@decl constant GL_CONVOLUTION_WIDTH_EXT 32792
 */

/*!@decl constant GL_COPY 5379
 *! Used in @[glGetIntegerv], @[glLogicOp], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glFeedbackBuffer]
 */

/*!@decl constant GL_COPY_INVERTED 5388
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_COPY_PIXEL_TOKEN 1798
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_CUBIC_EXT 33588
 */

/*!@decl constant GL_CULL_FACE 2884
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glCullFace], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glFrontFace]
 */

/*!@decl constant GL_CULL_FACE_MODE 2885
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_CURRENT_BIT 1
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_CURRENT_COLOR 2816
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CURRENT_INDEX 2817
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CURRENT_NORMAL 2818
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_CURRENT_RASTER_COLOR 2820
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CURRENT_RASTER_DISTANCE 2825
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CURRENT_RASTER_INDEX 2821
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CURRENT_RASTER_POSITION 2823
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CURRENT_RASTER_POSITION_VALID 2824
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CURRENT_RASTER_TEXTURE_COORDS 2822
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CURRENT_TEXTURE_COORDS 2819
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glRasterPos]
 */

/*!@decl constant GL_CW 2304
 *! Used in @[glFrontFace]
 */

/*!@decl constant GL_DECAL 8449
 *! Used in @[glTexEnv]
 */

/*!@decl constant GL_DECR 7683
 *! Used in @[glStencilOp]
 */

/*!@decl constant GL_DEPTH 6145
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDrawPixels], @[glDisable], @[glPopAttrib], @[glEnable], @[glPixelTransfer], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D], @[glCopyPixels], @[glDepthFunc], @[glTexImage1D] and @[glClear]
 */

/*!@decl constant GL_DEPTH_BIAS 3359
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels] and @[glCopyPixels]
 */

/*!@decl constant GL_DEPTH_BITS 3414
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_DEPTH_BUFFER_BIT 256
 *! Used in @[glPopAttrib], @[glPushAttrib] and @[glClear]
 */

/*!@decl constant GL_DEPTH_CLEAR_VALUE 2931
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_DEPTH_COMPONENT 6402
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_DEPTH_FUNC 2932
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_DEPTH_RANGE 2928
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_DEPTH_SCALE 3358
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels] and @[glCopyPixels]
 */

/*!@decl constant GL_DEPTH_TEST 2929
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glDepthFunc]
 */

/*!@decl constant GL_DEPTH_WRITEMASK 2930
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_DIFFUSE 4609
 *! Used in @[glColorMaterial], @[glGetLight], @[glMaterial], @[glLight] and @[glGetMaterial]
 */

/*!@decl constant GL_DITHER 3024
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_DOMAIN 2562
 *! Used in @[glGetMap]
 */

/*!@decl constant GL_DONT_CARE 4352
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glHint]
 */

/*!@decl constant GL_DOUBLE 5130
 *! Used in @[glGetIntegerv], @[glVertexPointer], @[glColorPointer], @[glNormalPointer], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glTexCoordPointer] and @[glIndexPointer]
 */

/*!@decl constant GL_DOUBLEBUFFER 3122
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_DRAW_BUFFER 3073
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_DRAW_PIXEL_TOKEN 1797
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_DST_ALPHA 772
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_DST_COLOR 774
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_EDGE_FLAG 2883
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glPopAttrib], @[glEnableClientState], @[glGetPointerv], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glEdgeFlagPointer], @[glGetDoublev] and @[glDisableClientState]
 */

/*!@decl constant GL_EDGE_FLAG_ARRAY 32889
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glEnableClientState], @[glGetPointerv], @[glGetBooleanv], @[glGetFloatv], @[glEdgeFlagPointer], @[glGetDoublev] and @[glDisableClientState]
 */

/*!@decl constant GL_EDGE_FLAG_ARRAY_POINTER 32915
 *! Used in @[glGetPointerv]
 */

/*!@decl constant GL_EDGE_FLAG_ARRAY_STRIDE 32908
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_EMISSION 5632
 *! Used in @[glColorMaterial], @[glMaterial] and @[glGetMaterial]
 */

/*!@decl constant GL_ENABLE_BIT 8192
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_EQUAL 514
 *! Used in @[glAlphaFunc], @[glDepthFunc] and @[glStencilFunc]
 */

/*!@decl constant GL_EQUIV 5385
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_EVAL_BIT 65536
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_EXP 2048
 *! Used in @[glFog], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_EXP2 2049
 *! Used in @[glFog]
 */

/*!@decl constant GL_EXTENSIONS 7939
 *! Used in @[glGetString]
 */

/*!@decl constant GL_EXT_abgr 1
 */

/*!@decl constant GL_EXT_blend_color 1
 */

/*!@decl constant GL_EXT_blend_minmax 1
 */

/*!@decl constant GL_EXT_blend_subtract 1
 */

/*!@decl constant GL_EXT_convolution 1
 */

/*!@decl constant GL_EXT_histogram 1
 */

/*!@decl constant GL_EXT_pixel_transform 1
 */

/*!@decl constant GL_EXT_rescale_normal 1
 */

/*!@decl constant GL_EXT_texture3D 1
 */

/*!@decl constant GL_EYE_LINEAR 9216
 *! Used in @[glGetTexGen] and @[glTexGen]
 */

/*!@decl constant GL_EYE_PLANE 9474
 *! Used in @[glGetTexGen] and @[glTexGen]
 */

/*!@decl constant GL_FALSE 0
 *! Used in @[glIsEnabled], @[glDepthMask], @[glIsList], @[glAreTexturesResident], @[glGetIntegerv], @[glIsTexture], @[glDisable], @[glEdgeFlag], @[glEnable], @[glColorMask], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glEdgeFlagv]
 */

/*!@decl constant GL_FASTEST 4353
 *! Used in @[glHint]
 */

/*!@decl constant GL_FEEDBACK 7169
 *! Used in @[glRenderMode], @[glPassThrough], @[glGetPointerv] and @[glFeedbackBuffer]
 */

/*!@decl constant GL_FEEDBACK_BUFFER_POINTER 3568
 *! Used in @[glGetPointerv]
 */

/*!@decl constant GL_FEEDBACK_BUFFER_SIZE 3569
 */

/*!@decl constant GL_FEEDBACK_BUFFER_TYPE 3570
 */

/*!@decl constant GL_FILL 6914
 *! Used in @[glGetIntegerv], @[glDisable], @[glEnable], @[glPolygonMode], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glEvalMesh]
 */

/*!@decl constant GL_FLAT 7424
 *! Used in @[glShadeModel]
 */

/*!@decl constant GL_FLOAT 5126
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glVertexPointer], @[glColorPointer], @[glTexSubImage2D], @[glNormalPointer], @[glGetBooleanv], @[glGetTexImage], @[glGetFloatv], @[glGetDoublev], @[glCallLists], @[glReadPixels], @[glTexImage2D], @[glTexCoordPointer], @[glIndexPointer], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_FOG 2912
 *! Used in @[glIsEnabled], @[glFog], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glHint]
 */

/*!@decl constant GL_FOG_BIT 128
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_FOG_COLOR 2918
 *! Used in @[glFog], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_FOG_DENSITY 2914
 *! Used in @[glFog], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_FOG_END 2916
 *! Used in @[glFog], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_FOG_HINT 3156
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glHint]
 */

/*!@decl constant GL_FOG_INDEX 2913
 *! Used in @[glFog], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_FOG_MODE 2917
 *! Used in @[glFog], @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_FOG_START 2915
 *! Used in @[glFog], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_FRONT 1028
 *! Used in @[glGetIntegerv], @[glColorMaterial], @[glCullFace], @[glPopAttrib], @[glPolygonMode], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glReadBuffer], @[glMaterial], @[glGetDoublev], @[glDrawBuffer] and @[glGetMaterial]
 */

/*!@decl constant GL_FRONT_AND_BACK 1032
 *! Used in @[glGetIntegerv], @[glColorMaterial], @[glCullFace], @[glPolygonMode], @[glGetBooleanv], @[glGetFloatv], @[glMaterial], @[glGetDoublev] and @[glDrawBuffer]
 */

/*!@decl constant GL_FRONT_FACE 2886
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_FRONT_LEFT 1024
 *! Used in @[glReadBuffer] and @[glDrawBuffer]
 */

/*!@decl constant GL_FRONT_RIGHT 1025
 *! Used in @[glReadBuffer] and @[glDrawBuffer]
 */

/*!@decl constant GL_FUNC_ADD_EXT 32774
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_FUNC_REVERSE_SUBTRACT_EXT 32779
 */

/*!@decl constant GL_FUNC_SUBTRACT_EXT 32778
 */

/*!@decl constant GL_GEQUAL 518
 *! Used in @[glAlphaFunc], @[glDepthFunc] and @[glStencilFunc]
 */

/*!@decl constant GL_GREATER 516
 *! Used in @[glAlphaFunc], @[glDepthFunc] and @[glStencilFunc]
 */

/*!@decl constant GL_GREEN 6404
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glTexSubImage2D], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_GREEN_BIAS 3353
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_GREEN_BITS 3411
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_GREEN_SCALE 3352
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_HINT_BIT 32768
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_HISTOGRAM_ALPHA_SIZE_EXT 32811
 */

/*!@decl constant GL_HISTOGRAM_BLUE_SIZE_EXT 32810
 */

/*!@decl constant GL_HISTOGRAM_EXT 32804
 */

/*!@decl constant GL_HISTOGRAM_FORMAT_EXT 32807
 */

/*!@decl constant GL_HISTOGRAM_GREEN_SIZE_EXT 32809
 */

/*!@decl constant GL_HISTOGRAM_LUMINANCE_SIZE_EXT 32812
 */

/*!@decl constant GL_HISTOGRAM_RED_SIZE_EXT 32808
 */

/*!@decl constant GL_HISTOGRAM_SINK_EXT 32813
 */

/*!@decl constant GL_HISTOGRAM_WIDTH_EXT 32806
 */

/*!@decl constant GL_HP_convolution_border_modes 1
 */

/*!@decl constant GL_HP_occlusion_test 1
 */

/*!@decl constant GL_IGNORE_BORDER_HP 33104
 */

/*!@decl constant GL_INCR 7682
 *! Used in @[glStencilOp]
 */

/*!@decl constant GL_INDEX_ARRAY 32887
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glEnableClientState], @[glGetPointerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glIndexPointer] and @[glDisableClientState]
 */

/*!@decl constant GL_INDEX_ARRAY_POINTER 32913
 *! Used in @[glGetPointerv]
 */

/*!@decl constant GL_INDEX_ARRAY_STRIDE 32902
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_INDEX_ARRAY_TYPE 32901
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_INDEX_BITS 3409
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_INDEX_CLEAR_VALUE 3104
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_INDEX_LOGIC_OP 3057
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glLogicOp], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_INDEX_MODE 3120
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_INDEX_OFFSET 3347
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D], @[glCopyPixels] and @[glTexImage1D]
 */

/*!@decl constant GL_INDEX_SHIFT 3346
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D], @[glCopyPixels] and @[glTexImage1D]
 */

/*!@decl constant GL_INDEX_WRITEMASK 3105
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_INT 5124
 *! Used in @[glTexEnv], @[glDrawPixels], @[glVertexPointer], @[glColorPointer], @[glCopyTexImage1D], @[glTexSubImage2D], @[glNormalPointer], @[glGetTexImage], @[glCallLists], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glTexCoordPointer], @[glIndexPointer], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_INTENSITY 32841
 *! Used in @[glTexEnv], @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_INTENSITY12 32844
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_INTENSITY16 32845
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_INTENSITY4 32842
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_INTENSITY8 32843
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_INVALID_ENUM 1280
 *! Used in @[glMatrixMode], @[glPixelStore], @[glIsEnabled], @[glGetTexLevelParameter], @[glFog], @[glTexEnv], @[glMap1], @[glRenderMode], @[glGetTexGen], @[glGetIntegerv], @[glEnd], @[glDrawElements], @[glDrawArrays], @[glCopyTexSubImage2D], @[glColorMaterial], @[glBindTexture], @[glGetClipPlane], @[glCullFace], @[glGetError], @[glClipPlane], @[glDrawPixels], @[glBlendFunc], @[glVertexPointer], @[glGetLight], @[glDisable], @[glMap2], @[glColorPointer], @[glCopyTexImage1D], @[glGetPixelMap], @[glTexSubImage2D], @[glNormalPointer], @[glEnable], @[glEnableClientState], @[glGetPointerv], @[glPixelTransfer], @[glPolygonMode], @[glGetTexParameter], @[glLogicOp], @[glGetBooleanv], @[glGetTexImage], @[glPixelMap], @[glGetFloatv], @[glGetMap], @[glAccum], @[glReadBuffer], @[glMaterial], @[glEdgeFlagPointer], @[glGetDoublev], @[glCallLists], @[glInterleavedArrays], @[glAlphaFunc], @[glShadeModel], @[glNewList], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glGetTexEnv], @[glFeedbackBuffer], @[glEndList], @[glBegin], @[glLight], @[glStencilOp], @[glTexCoordPointer], @[glCopyPixels], @[glDepthFunc], @[glLightModel], @[glIndexPointer], @[glGetString], @[glTexParameter], @[glDisableClientState], @[glTexSubImage1D], @[glDrawBuffer], @[glHint], @[glTexImage1D], @[glStencilFunc], @[glGetMaterial], @[glCopyTexSubImage1D], @[glFrontFace], @[glEvalMesh] and @[glTexGen]
 */

/*!@decl constant GL_INVALID_OPERATION 1282
 *! Used in @[glPolygonOffset], @[glPushMatrix], @[glMatrixMode], @[glLoadName], @[glPixelStore], @[glIsEnabled], @[glGetTexLevelParameter], @[glGenTextures], @[glFog], @[glTexEnv], @[glClearColor], @[glMap1], @[glPrioritizeTextures], @[glDepthMask], @[glPushName], @[glRenderMode], @[glMultMatrix], @[glIsList], @[glFrustum], @[glDepthRange], @[glAreTexturesResident], @[glDeleteTextures], @[glGetTexGen], @[glGetIntegerv], @[glEnd], @[glIsTexture], @[glDrawElements], @[glGenLists], @[glBlendColorEXT], @[glViewport], @[glDrawArrays], @[glCopyTexSubImage2D], @[glColorMaterial], @[glBindTexture], @[glGetClipPlane], @[glFinish], @[glCullFace], @[glGetError], @[glStencilMask], @[glClipPlane], @[glDrawPixels], @[glBlendFunc], @[glSelectBuffer], @[glGetLight], @[glInitNames], @[glPassThrough], @[glDisable], @[glMap2], @[glLineStipple], @[glCopyTexImage1D], @[glGetPixelMap], @[glTexSubImage2D], @[glPopAttrib], @[glEnable], @[glListBase], @[glColorMask], @[glPixelTransfer], @[glPolygonMode], @[glGetTexParameter], @[glPopName], @[glLogicOp], @[glIndexMask], @[glGetBooleanv], @[glGetTexImage], @[glPixelMap], @[glTranslate], @[glPushAttrib], @[glGetFloatv], @[glGetMap], @[glAccum], @[glReadBuffer], @[glGetDoublev], @[glClearDepth], @[glRasterPos], @[glAlphaFunc], @[glShadeModel], @[glNewList], @[glReadPixels], @[glCopyTexImage2D], @[glRect], @[glTexImage2D], @[glGetTexEnv], @[glClearIndex], @[glFeedbackBuffer], @[glRotate], @[glEndList], @[glBegin], @[glLight], @[glGetPolygonStipple], @[glStencilOp], @[glClearStencil], @[glBitmap], @[glScale], @[glCopyPixels], @[glFlush], @[glOrtho], @[glDepthFunc], @[glMapGrid], @[glLightModel], @[glGetString], @[glTexParameter], @[glScissor], @[glTexSubImage1D], @[glLoadIdentity], @[glPixelZoom], @[glDrawBuffer], @[glLineWidth], @[glHint], @[glTexImage1D], @[glStencilFunc], @[glGetMaterial], @[glClear], @[glCopyTexSubImage1D], @[glPopMatrix], @[glPointSize], @[glPolygonStipple], @[glFrontFace], @[glLoadMatrix], @[glEvalMesh], @[glTexGen], @[glClearAccum] and @[glDeleteLists]
 */

/*!@decl constant GL_INVALID_VALUE 1281
 *! Used in @[glPixelStore], @[glGetTexLevelParameter], @[glGenTextures], @[glFog], @[glMap1], @[glPrioritizeTextures], @[glFrustum], @[glAreTexturesResident], @[glDeleteTextures], @[glDrawElements], @[glGenLists], @[glViewport], @[glDrawArrays], @[glCopyTexSubImage2D], @[glGetError], @[glDrawPixels], @[glVertexPointer], @[glSelectBuffer], @[glMap2], @[glColorPointer], @[glCopyTexImage1D], @[glTexSubImage2D], @[glNormalPointer], @[glGetTexImage], @[glPixelMap], @[glMaterial], @[glCallLists], @[glInterleavedArrays], @[glNewList], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glFeedbackBuffer], @[glEndList], @[glLight], @[glBitmap], @[glTexCoordPointer], @[glCopyPixels], @[glMapGrid], @[glIndexPointer], @[glScissor], @[glTexSubImage1D], @[glLineWidth], @[glTexImage1D], @[glClear], @[glCopyTexSubImage1D], @[glPointSize] and @[glDeleteLists]
 */

/*!@decl constant GL_INVERT 5386
 *! Used in @[glLogicOp] and @[glStencilOp]
 */

/*!@decl constant GL_KEEP 7680
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glStencilOp]
 */

/*!@decl constant GL_LARGE_SUNX 33235
 */

/*!@decl constant GL_LEFT 1030
 *! Used in @[glReadBuffer] and @[glDrawBuffer]
 */

/*!@decl constant GL_LEQUAL 515
 *! Used in @[glAlphaFunc], @[glDepthFunc] and @[glStencilFunc]
 */

/*!@decl constant GL_LESS 513
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glAlphaFunc], @[glDepthFunc] and @[glStencilFunc]
 */

/*!@decl constant GL_LIGHT0 16384
 *! Used in @[glGetLight] and @[glLight]
 */

/*!@decl constant GL_LIGHT1 16385
 */

/*!@decl constant GL_LIGHT2 16386
 */

/*!@decl constant GL_LIGHT3 16387
 */

/*!@decl constant GL_LIGHT4 16388
 */

/*!@decl constant GL_LIGHT5 16389
 */

/*!@decl constant GL_LIGHT6 16390
 */

/*!@decl constant GL_LIGHT7 16391
 */

/*!@decl constant GL_LIGHTING 2896
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glLight]
 */

/*!@decl constant GL_LIGHTING_BIT 64
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_LIGHT_MODEL_AMBIENT 2899
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glLightModel]
 */

/*!@decl constant GL_LIGHT_MODEL_LOCAL_VIEWER 2897
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glLightModel]
 */

/*!@decl constant GL_LIGHT_MODEL_TWO_SIDE 2898
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glLightModel]
 */

/*!@decl constant GL_LINE 6913
 *! Used in @[glIsEnabled], @[glFog], @[glGetIntegerv], @[glEnd], @[glDrawElements], @[glDrawArrays], @[glGetLight], @[glDisable], @[glLineStipple], @[glEdgeFlag], @[glPopAttrib], @[glEnable], @[glPolygonMode], @[glGetTexParameter], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glFeedbackBuffer], @[glBegin], @[glLight], @[glTexParameter], @[glLineWidth], @[glHint], @[glEdgeFlagv] and @[glEvalMesh]
 */

/*!@decl constant GL_LINEAR 9729
 *! Used in @[glFog], @[glGetLight], @[glGetTexParameter], @[glLight] and @[glTexParameter]
 */

/*!@decl constant GL_LINEAR_ATTENUATION 4616
 *! Used in @[glGetLight] and @[glLight]
 */

/*!@decl constant GL_LINEAR_MIPMAP_LINEAR 9987
 *! Used in @[glTexParameter]
 */

/*!@decl constant GL_LINEAR_MIPMAP_NEAREST 9985
 *! Used in @[glTexParameter]
 */

/*!@decl constant GL_LINES 1
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays], @[glLineStipple], @[glBegin] and @[glEvalMesh]
 */

/*!@decl constant GL_LINE_BIT 4
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_LINE_LOOP 2
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays] and @[glBegin]
 */

/*!@decl constant GL_LINE_RESET_TOKEN 1799
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_LINE_SMOOTH 2848
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glPolygonMode], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glLineWidth] and @[glHint]
 */

/*!@decl constant GL_LINE_SMOOTH_HINT 3154
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glHint]
 */

/*!@decl constant GL_LINE_STIPPLE 2852
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glLineStipple], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_LINE_STIPPLE_PATTERN 2853
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_LINE_STIPPLE_REPEAT 2854
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_LINE_STRIP 3
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays], @[glBegin] and @[glEvalMesh]
 */

/*!@decl constant GL_LINE_TOKEN 1794
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_LINE_WIDTH 2849
 *! Used in @[glGetIntegerv], @[glPolygonMode], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glLineWidth]
 */

/*!@decl constant GL_LINE_WIDTH_GRANULARITY 2851
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glLineWidth]
 */

/*!@decl constant GL_LINE_WIDTH_RANGE 2850
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glLineWidth]
 */

/*!@decl constant GL_LIST_BASE 2866
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_LIST_BIT 131072
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_LIST_INDEX 2867
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_LIST_MODE 2864
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_LOAD 257
 *! Used in @[glAccum]
 */

/*!@decl constant GL_LOGIC_OP 3057
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_LOGIC_OP_MODE 3056
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_LUMINANCE 6409
 *! Used in @[glTexEnv], @[glDrawPixels], @[glCopyTexImage1D], @[glTexSubImage2D], @[glGetTexImage], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE12 32833
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE12_ALPHA12 32839
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE12_ALPHA4 32838
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE16 32834
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE16_ALPHA16 32840
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE4 32831
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE4_ALPHA4 32835
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE6_ALPHA2 32836
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE8 32832
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE8_ALPHA8 32837
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_LUMINANCE_ALPHA 6410
 *! Used in @[glDrawPixels], @[glCopyTexImage1D], @[glTexSubImage2D], @[glGetTexImage], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_MAP1_COLOR_4 3472
 *! Used in @[glIsEnabled], @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP1_GRID_DOMAIN 3536
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAP1_GRID_SEGMENTS 3537
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAP1_INDEX 3473
 *! Used in @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP1_NORMAL 3474
 *! Used in @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP1_TEXTURE_COORD_1 3475
 *! Used in @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP1_TEXTURE_COORD_2 3476
 *! Used in @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP1_TEXTURE_COORD_3 3477
 *! Used in @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP1_TEXTURE_COORD_4 3478
 *! Used in @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP1_VERTEX_3 3479
 *! Used in @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP1_VERTEX_4 3480
 *! Used in @[glMap1], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_COLOR_4 3504
 *! Used in @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_GRID_DOMAIN 3538
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAP2_GRID_SEGMENTS 3539
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAP2_INDEX 3505
 *! Used in @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_NORMAL 3506
 *! Used in @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_TEXTURE_COORD_1 3507
 *! Used in @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_TEXTURE_COORD_2 3508
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_TEXTURE_COORD_3 3509
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_TEXTURE_COORD_4 3510
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_VERTEX_3 3511
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP2_VERTEX_4 3512
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glMap2], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetMap], @[glGetDoublev] and @[glEvalCoord]
 */

/*!@decl constant GL_MAP_COLOR 3344
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels] and @[glCopyPixels]
 */

/*!@decl constant GL_MAP_STENCIL 3345
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels] and @[glCopyPixels]
 */

/*!@decl constant GL_MATRIX_MODE 2976
 *! Used in @[glMatrixMode], @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_3D_TEXTURE_SIZE_EXT 32883
 */

/*!@decl constant GL_MAX_ATTRIB_STACK_DEPTH 3381
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_CLIENT_ATTRIB_STACK_DEPTH 3387
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_CLIP_PLANES 3378
 *! Used in @[glGetIntegerv], @[glGetClipPlane], @[glClipPlane], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_CONVOLUTION_HEIGHT_EXT 32795
 */

/*!@decl constant GL_MAX_CONVOLUTION_WIDTH_EXT 32794
 */

/*!@decl constant GL_MAX_EVAL_ORDER 3376
 *! Used in @[glMap1], @[glGetIntegerv], @[glMap2], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_EXT 32776
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_LIGHTS 3377
 *! Used in @[glGetIntegerv], @[glGetLight], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glLight]
 */

/*!@decl constant GL_MAX_LIST_NESTING 2865
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_MODELVIEW_STACK_DEPTH 3382
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_NAME_STACK_DEPTH 3383
 *! Used in @[glPushName], @[glGetIntegerv], @[glPopName], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_PIXEL_MAP_TABLE 3380
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT 33591
 */

/*!@decl constant GL_MAX_PROJECTION_STACK_DEPTH 3384
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_TEXTURE_SIZE 3379
 *! Used in @[glGetTexLevelParameter], @[glGetIntegerv], @[glCopyTexSubImage2D], @[glCopyTexImage1D], @[glTexSubImage2D], @[glGetBooleanv], @[glGetTexImage], @[glGetFloatv], @[glGetDoublev], @[glCopyTexImage2D], @[glTexImage2D], @[glTexSubImage1D], @[glTexImage1D] and @[glCopyTexSubImage1D]
 */

/*!@decl constant GL_MAX_TEXTURE_STACK_DEPTH 3385
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MAX_VIEWPORT_DIMS 3386
 *! Used in @[glGetIntegerv], @[glViewport], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MINMAX_EXT 32814
 */

/*!@decl constant GL_MINMAX_FORMAT_EXT 32815
 */

/*!@decl constant GL_MINMAX_SINK_EXT 32816
 */

/*!@decl constant GL_MIN_EXT 32775
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MODELVIEW 5888
 *! Used in @[glPushMatrix], @[glMatrixMode], @[glGetIntegerv], @[glGetBooleanv], @[glTranslate], @[glGetFloatv], @[glGetDoublev], @[glRotate], @[glScale] and @[glPopMatrix]
 */

/*!@decl constant GL_MODELVIEW_MATRIX 2982
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MODELVIEW_STACK_DEPTH 2979
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_MODULATE 8448
 *! Used in @[glTexEnv] and @[glGetTexEnv]
 */

/*!@decl constant GL_MULT 259
 *! Used in @[glAccum]
 */

/*!@decl constant GL_N3F_V3F 10789
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_NAME_STACK_DEPTH 3440
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_NAND 5390
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_NEAREST 9728
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_NEAREST_MIPMAP_LINEAR 9986
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_NEAREST_MIPMAP_NEAREST 9984
 *! Used in @[glTexParameter]
 */

/*!@decl constant GL_NEVER 512
 *! Used in @[glAlphaFunc], @[glDepthFunc] and @[glStencilFunc]
 */

/*!@decl constant GL_NICEST 4354
 *! Used in @[glHint]
 */

/*!@decl constant GL_NONE 0
 *! Used in @[glDrawBuffer]
 */

/*!@decl constant GL_NOOP 5381
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_NOR 5384
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glNormalPointer], @[glPopAttrib], @[glEnable], @[glEnableClientState], @[glGetPointerv], @[glLogicOp], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glNormal] and @[glDisableClientState]
 */

/*!@decl constant GL_NORMALIZE 2977
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glNormal]
 */

/*!@decl constant GL_NORMAL_ARRAY 32885
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glNormalPointer], @[glEnableClientState], @[glGetPointerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glDisableClientState]
 */

/*!@decl constant GL_NORMAL_ARRAY_POINTER 32911
 *! Used in @[glGetPointerv]
 */

/*!@decl constant GL_NORMAL_ARRAY_STRIDE 32895
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_NORMAL_ARRAY_TYPE 32894
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_NOTEQUAL 517
 *! Used in @[glAlphaFunc], @[glDepthFunc] and @[glStencilFunc]
 */

/*!@decl constant GL_NO_ERROR 0
 *! Used in @[glGetError]
 */

/*!@decl constant GL_OBJECT_LINEAR 9217
 *! Used in @[glTexGen]
 */

/*!@decl constant GL_OBJECT_PLANE 9473
 *! Used in @[glGetTexGen] and @[glTexGen]
 */

/*!@decl constant GL_OCCLUSION_RESULT_HP 33126
 */

/*!@decl constant GL_OCCLUSION_TEST_HP 33125
 */

/*!@decl constant GL_ONE 1
 *! Used in @[glGetIntegerv], @[glBlendFunc], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_ONE_MINUS_CONSTANT_ALPHA_EXT 32772
 */

/*!@decl constant GL_ONE_MINUS_CONSTANT_COLOR_EXT 32770
 */

/*!@decl constant GL_ONE_MINUS_DST_ALPHA 773
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_ONE_MINUS_DST_COLOR 775
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_ONE_MINUS_SRC_ALPHA 771
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_ONE_MINUS_SRC_COLOR 769
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_OR 5383
 *! Used in @[glLogicOp] and @[glGetMap]
 */

/*!@decl constant GL_ORDER 2561
 *! Used in @[glGetMap]
 */

/*!@decl constant GL_OR_INVERTED 5389
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_OR_REVERSE 5387
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_OUT_OF_MEMORY 1285
 *! Used in @[glGetError], @[glNewList] and @[glEndList]
 */

/*!@decl constant GL_PACK_ALIGNMENT 3333
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetTexImage], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PACK_IMAGE_HEIGHT_EXT 32876
 */

/*!@decl constant GL_PACK_LSB_FIRST 3329
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glReadPixels]
 */

/*!@decl constant GL_PACK_ROW_LENGTH 3330
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PACK_SKIP_IMAGES_EXT 32875
 */

/*!@decl constant GL_PACK_SKIP_PIXELS 3332
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PACK_SKIP_ROWS 3331
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PACK_SWAP_BYTES 3328
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glReadPixels]
 */

/*!@decl constant GL_PASS_THROUGH_TOKEN 1792
 *! Used in @[glPassThrough] and @[glFeedbackBuffer]
 */

/*!@decl constant GL_PERSPECTIVE_CORRECTION_HINT 3152
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glHint]
 */

/*!@decl constant GL_PIXEL_CUBIC_WEIGHT_EXT 33587
 */

/*!@decl constant GL_PIXEL_MAG_FILTER_EXT 33585
 */

/*!@decl constant GL_PIXEL_MAP_A_TO_A 3193
 *! Used in @[glGetIntegerv], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_A_TO_A_SIZE 3257
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_B_TO_B 3192
 *! Used in @[glGetIntegerv], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_B_TO_B_SIZE 3256
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_G_TO_G 3191
 *! Used in @[glGetIntegerv], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_G_TO_G_SIZE 3255
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_A 3189
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_A_SIZE 3253
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_B 3188
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_B_SIZE 3252
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_G 3187
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_G_SIZE 3251
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_I 3184
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv], @[glGetDoublev], @[glReadPixels] and @[glCopyPixels]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_I_SIZE 3248
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_R 3186
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_PIXEL_MAP_I_TO_R_SIZE 3250
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_R_TO_R 3190
 *! Used in @[glGetIntegerv], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_R_TO_R_SIZE 3254
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MAP_S_TO_S 3185
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glGetPixelMap], @[glPixelTransfer], @[glGetBooleanv], @[glPixelMap], @[glGetFloatv], @[glGetDoublev], @[glReadPixels] and @[glCopyPixels]
 */

/*!@decl constant GL_PIXEL_MAP_S_TO_S_SIZE 3249
 *! Used in @[glGetIntegerv], @[glPixelTransfer], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PIXEL_MIN_FILTER_EXT 33586
 */

/*!@decl constant GL_PIXEL_MODE_BIT 32
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_PIXEL_TRANSFORM_2D_EXT 33584
 */

/*!@decl constant GL_PIXEL_TRANSFORM_2D_MATRIX_EXT 33592
 */

/*!@decl constant GL_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT 33590
 */

/*!@decl constant GL_PIXEL_TRANSFORM_COLOR_TABLE_EXT 33593
 */

/*!@decl constant GL_POINT 6912
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glEnd], @[glDrawElements], @[glDrawArrays], @[glDisable], @[glEdgeFlag], @[glPopAttrib], @[glEnable], @[glPolygonMode], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glFeedbackBuffer], @[glBegin], @[glHint], @[glEdgeFlagv], @[glPointSize] and @[glEvalMesh]
 */

/*!@decl constant GL_POINTS 0
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays], @[glBegin] and @[glEvalMesh]
 */

/*!@decl constant GL_POINT_BIT 2
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_POINT_SIZE 2833
 *! Used in @[glGetIntegerv], @[glPolygonMode], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glPointSize]
 */

/*!@decl constant GL_POINT_SIZE_GRANULARITY 2835
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glPointSize]
 */

/*!@decl constant GL_POINT_SIZE_RANGE 2834
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glPointSize]
 */

/*!@decl constant GL_POINT_SMOOTH 2832
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glPolygonMode], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glHint] and @[glPointSize]
 */

/*!@decl constant GL_POINT_SMOOTH_HINT 3153
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glHint]
 */

/*!@decl constant GL_POINT_TOKEN 1793
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_POLYGON 9
 *! Used in @[glPolygonOffset], @[glIsEnabled], @[glGetIntegerv], @[glEnd], @[glDrawElements], @[glDrawArrays], @[glDisable], @[glEdgeFlag], @[glPopAttrib], @[glEnable], @[glPolygonMode], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glRect], @[glFeedbackBuffer], @[glBegin], @[glHint], @[glEdgeFlagv] and @[glPolygonStipple]
 */

/*!@decl constant GL_POLYGON_BIT 8
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_POLYGON_MODE 2880
 *! Used in @[glGetIntegerv], @[glEdgeFlag], @[glPopAttrib], @[glPolygonMode], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glEdgeFlagv]
 */

/*!@decl constant GL_POLYGON_OFFSET_FACTOR 32824
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_POLYGON_OFFSET_FILL 32823
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_POLYGON_OFFSET_LINE 10754
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_POLYGON_OFFSET_POINT 10753
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_POLYGON_OFFSET_UNITS 10752
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_POLYGON_SMOOTH 2881
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glPolygonMode], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glHint]
 */

/*!@decl constant GL_POLYGON_SMOOTH_HINT 3155
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glHint]
 */

/*!@decl constant GL_POLYGON_STIPPLE 2882
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glPolygonMode], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glPolygonStipple]
 */

/*!@decl constant GL_POLYGON_STIPPLE_BIT 16
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_POLYGON_TOKEN 1795
 *! Used in @[glFeedbackBuffer]
 */

/*!@decl constant GL_POSITION 4611
 *! Used in @[glGetLight] and @[glLight]
 */

/*!@decl constant GL_POST_CONVOLUTION_ALPHA_BIAS_EXT 32803
 */

/*!@decl constant GL_POST_CONVOLUTION_ALPHA_SCALE_EXT 32799
 */

/*!@decl constant GL_POST_CONVOLUTION_BLUE_BIAS_EXT 32802
 */

/*!@decl constant GL_POST_CONVOLUTION_BLUE_SCALE_EXT 32798
 */

/*!@decl constant GL_POST_CONVOLUTION_COLOR_TABLE_SGI 32977
 */

/*!@decl constant GL_POST_CONVOLUTION_GREEN_BIAS_EXT 32801
 */

/*!@decl constant GL_POST_CONVOLUTION_GREEN_SCALE_EXT 32797
 */

/*!@decl constant GL_POST_CONVOLUTION_RED_BIAS_EXT 32800
 */

/*!@decl constant GL_POST_CONVOLUTION_RED_SCALE_EXT 32796
 */

/*!@decl constant GL_PROJECTION 5889
 *! Used in @[glPushMatrix], @[glMatrixMode], @[glFrustum], @[glGetIntegerv], @[glGetBooleanv], @[glTranslate], @[glGetFloatv], @[glGetDoublev], @[glRotate], @[glScale], @[glOrtho] and @[glPopMatrix]
 */

/*!@decl constant GL_PROJECTION_MATRIX 2983
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PROJECTION_STACK_DEPTH 2980
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_PROXY_COLOR_TABLE_SGI 32979
 */

/*!@decl constant GL_PROXY_HISTOGRAM_EXT 32805
 */

/*!@decl constant GL_PROXY_PIXEL_TRANSFORM_COLOR_TABLE_EXT 33594
 */

/*!@decl constant GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI 32980
 */

/*!@decl constant GL_PROXY_TEXTURE_1D 32867
 *! Used in @[glGetTexLevelParameter], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glNewList], @[glEndList] and @[glTexImage1D]
 */

/*!@decl constant GL_PROXY_TEXTURE_2D 32868
 *! Used in @[glGetTexLevelParameter], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glNewList], @[glTexImage2D] and @[glEndList]
 */

/*!@decl constant GL_PROXY_TEXTURE_3D_EXT 32880
 */

/*!@decl constant GL_PROXY_TEXTURE_COLOR_TABLE_SGI 32957
 */

/*!@decl constant GL_Q 8195
 *! Used in @[glGetTexGen], @[glEnd], @[glDrawElements], @[glDrawArrays], @[glGetLight], @[glBegin], @[glLight], @[glEvalMesh] and @[glTexGen]
 */

/*!@decl constant GL_QUADRATIC_ATTENUATION 4617
 *! Used in @[glGetLight] and @[glLight]
 */

/*!@decl constant GL_QUADS 7
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays] and @[glBegin]
 */

/*!@decl constant GL_QUAD_STRIP 8
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays], @[glBegin] and @[glEvalMesh]
 */

/*!@decl constant GL_R 8194
 *! Used in @[glPixelStore], @[glTexEnv], @[glRenderMode], @[glGetTexGen], @[glGetIntegerv], @[glCopyTexSubImage2D], @[glDrawPixels], @[glCopyTexImage1D], @[glTexSubImage2D], @[glPopAttrib], @[glPixelTransfer], @[glGetTexParameter], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glAccum], @[glReadBuffer], @[glGetDoublev], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glStencilOp], @[glGetString], @[glTexParameter], @[glTexSubImage1D], @[glDrawBuffer], @[glTexImage1D], @[glCopyTexSubImage1D] and @[glTexGen]
 */

/*!@decl constant GL_R3_G3_B2 10768
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_READ_BUFFER 3074
 *! Used in @[glGetIntegerv], @[glCopyTexSubImage2D], @[glCopyTexImage1D], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glCopyTexImage2D] and @[glCopyTexSubImage1D]
 */

/*!@decl constant GL_RED 6403
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glTexSubImage2D], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_REDUCE_EXT 32790
 */

/*!@decl constant GL_RED_BIAS 3349
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_RED_BITS 3410
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_RED_SCALE 3348
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glPixelTransfer], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_RENDER 7168
 *! Used in @[glRenderMode], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glGetString]
 */

/*!@decl constant GL_RENDERER 7937
 *! Used in @[glGetString]
 */

/*!@decl constant GL_RENDER_MODE 3136
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_REPEAT 10497
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_REPLACE 7681
 *! Used in @[glTexEnv] and @[glStencilOp]
 */

/*!@decl constant GL_REPLICATE_BORDER_HP 33107
 */

/*!@decl constant GL_RESCALE_NORMAL_EXT 32826
 */

/*!@decl constant GL_RETURN 258
 *! Used in @[glAccum]
 */

/*!@decl constant GL_RGB 6407
 *! Used in @[glPixelStore], @[glTexEnv], @[glGetIntegerv], @[glDrawPixels], @[glCopyTexImage1D], @[glTexSubImage2D], @[glGetBooleanv], @[glGetTexImage], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGB10 32850
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGB10_A2 32857
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGB12 32851
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGB16 32852
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGB4 32847
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGB5 32848
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGB5_A1 32855
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGB8 32849
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGBA 6408
 *! Used in @[glTexEnv], @[glGetIntegerv], @[glDrawPixels], @[glCopyTexImage1D], @[glTexSubImage2D], @[glGetBooleanv], @[glGetTexImage], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGBA12 32858
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGBA16 32859
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGBA2 32853
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGBA4 32854
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGBA8 32856
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_RGBA_MODE 3121
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_RIGHT 1031
 *! Used in @[glReadBuffer] and @[glDrawBuffer]
 */

/*!@decl constant GL_S 8192
 *! Used in @[glPushMatrix], @[glLoadName], @[glIsEnabled], @[glPushName], @[glRenderMode], @[glGetTexGen], @[glGetIntegerv], @[glColorMaterial], @[glGetError], @[glDrawPixels], @[glBlendFunc], @[glVertexPointer], @[glSelectBuffer], @[glGetLight], @[glInitNames], @[glDisable], @[glColorPointer], @[glTexSubImage2D], @[glNormalPointer], @[glPopAttrib], @[glEnable], @[glGetPointerv], @[glPopName], @[glLogicOp], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glMaterial], @[glGetDoublev], @[glCallLists], @[glShadeModel], @[glReadPixels], @[glTexImage2D], @[glLight], @[glPopClientAttrib], @[glStencilOp], @[glTexCoordPointer], @[glCopyPixels], @[glIndexPointer], @[glPushClientAttrib], @[glScissor], @[glTexSubImage1D], @[glTexImage1D], @[glStencilFunc], @[glGetMaterial], @[glClear], @[glPopMatrix] and @[glTexGen]
 */

/*!@decl constant GL_SCISSOR_BIT 524288
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_SCISSOR_BOX 3088
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_SCISSOR_TEST 3089
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glScissor]
 */

/*!@decl constant GL_SELECT 7170
 *! Used in @[glLoadName], @[glPushName], @[glRenderMode], @[glSelectBuffer], @[glInitNames], @[glGetPointerv] and @[glPopName]
 */

/*!@decl constant GL_SELECTION_BUFFER_POINTER 3571
 *! Used in @[glGetPointerv]
 */

/*!@decl constant GL_SELECTION_BUFFER_SIZE 3572
 */

/*!@decl constant GL_SEPARABLE_2D_EXT 32786
 */

/*!@decl constant GL_SET 5391
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_SGI_color_table 1
 */

/*!@decl constant GL_SGI_texture_color_table 1
 */

/*!@decl constant GL_SHADE_MODEL 2900
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_SHININESS 5633
 *! Used in @[glMaterial] and @[glGetMaterial]
 */

/*!@decl constant GL_SHORT 5122
 *! Used in @[glDrawPixels], @[glVertexPointer], @[glColorPointer], @[glTexSubImage2D], @[glNormalPointer], @[glGetTexImage], @[glCallLists], @[glReadPixels], @[glTexImage2D], @[glTexCoordPointer], @[glIndexPointer], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_SMOOTH 7425
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glShadeModel]
 */

/*!@decl constant GL_SPECULAR 4610
 *! Used in @[glColorMaterial], @[glGetLight], @[glMaterial], @[glLight] and @[glGetMaterial]
 */

/*!@decl constant GL_SPHERE_MAP 9218
 *! Used in @[glTexGen]
 */

/*!@decl constant GL_SPOT_CUTOFF 4614
 *! Used in @[glGetLight] and @[glLight]
 */

/*!@decl constant GL_SPOT_DIRECTION 4612
 *! Used in @[glGetLight] and @[glLight]
 */

/*!@decl constant GL_SPOT_EXPONENT 4613
 *! Used in @[glGetLight] and @[glLight]
 */

/*!@decl constant GL_SRC_ALPHA 770
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_SRC_ALPHA_SATURATE 776
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_SRC_COLOR 768
 *! Used in @[glBlendFunc]
 */

/*!@decl constant GL_STACK_OVERFLOW 1283
 *! Used in @[glPushMatrix], @[glPushName], @[glGetError], @[glPopAttrib], @[glPopName], @[glPushAttrib], @[glPopClientAttrib], @[glPushClientAttrib] and @[glPopMatrix]
 */

/*!@decl constant GL_STACK_UNDERFLOW 1284
 *! Used in @[glPushMatrix], @[glPushName], @[glGetError], @[glPopAttrib], @[glPopName], @[glPushAttrib], @[glPopClientAttrib], @[glPushClientAttrib] and @[glPopMatrix]
 */

/*!@decl constant GL_STENCIL 6146
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDrawPixels], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glReadPixels], @[glTexImage2D], @[glStencilOp], @[glCopyPixels], @[glTexImage1D], @[glStencilFunc] and @[glClear]
 */

/*!@decl constant GL_STENCIL_BITS 3415
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glStencilOp]
 */

/*!@decl constant GL_STENCIL_BUFFER_BIT 1024
 *! Used in @[glPopAttrib], @[glPushAttrib] and @[glClear]
 */

/*!@decl constant GL_STENCIL_CLEAR_VALUE 2961
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_STENCIL_FAIL 2964
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_STENCIL_FUNC 2962
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_STENCIL_INDEX 6401
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels], @[glTexImage2D] and @[glTexImage1D]
 */

/*!@decl constant GL_STENCIL_PASS_DEPTH_FAIL 2965
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_STENCIL_PASS_DEPTH_PASS 2966
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_STENCIL_REF 2967
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_STENCIL_TEST 2960
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glPopAttrib], @[glEnable], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glStencilOp] and @[glStencilFunc]
 */

/*!@decl constant GL_STENCIL_VALUE_MASK 2963
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_STENCIL_WRITEMASK 2968
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_STEREO 3123
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_SUBPIXEL_BITS 3408
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_SUNX_geometry_compression 1
 */

/*!@decl constant GL_SUNX_surface_hint 1
 */

/*!@decl constant GL_SUN_convolution_border_modes 1
 */

/*!@decl constant GL_SUN_multi_draw_arrays 1
 */

/*!@decl constant GL_SURFACE_SIZE_HINT_SUNX 33234
 */

/*!@decl constant GL_T 8193
 *! Used in @[glPushMatrix], @[glMatrixMode], @[glIsEnabled], @[glGetTexLevelParameter], @[glTexEnv], @[glIsList], @[glAreTexturesResident], @[glGetTexGen], @[glGetIntegerv], @[glEnd], @[glIsTexture], @[glDrawElements], @[glDrawArrays], @[glCopyTexSubImage2D], @[glBindTexture], @[glDisable], @[glCopyTexImage1D], @[glTexSubImage2D], @[glEdgeFlag], @[glPopAttrib], @[glEnable], @[glColorMask], @[glEnableClientState], @[glGetPointerv], @[glGetTexParameter], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glInterleavedArrays], @[glReadPixels], @[glCopyTexImage2D], @[glTexImage2D], @[glGetTexEnv], @[glBegin], @[glTexCoordPointer], @[glTexParameter], @[glDisableClientState], @[glTexSubImage1D], @[glEdgeFlagv], @[glTexImage1D], @[glCopyTexSubImage1D], @[glPopMatrix] and @[glTexGen]
 */

/*!@decl constant GL_T2F_C3F_V3F 10794
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_T2F_C4F_N3F_V3F 10796
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_T2F_C4UB_V3F 10793
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_T2F_N3F_V3F 10795
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_T2F_V3F 10791
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_T4F_C4F_N3F_V4F 10797
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_T4F_V4F 10792
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_TABLE_TOO_LARGE_EXT 32817
 */

/*!@decl constant GL_TEXTURE 5890
 *! Used in @[glPushMatrix], @[glMatrixMode], @[glIsEnabled], @[glGetTexLevelParameter], @[glTexEnv], @[glAreTexturesResident], @[glGetTexGen], @[glGetIntegerv], @[glCopyTexSubImage2D], @[glBindTexture], @[glDisable], @[glCopyTexImage1D], @[glTexSubImage2D], @[glPopAttrib], @[glEnable], @[glEnableClientState], @[glGetPointerv], @[glGetTexParameter], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glCopyTexImage2D], @[glTexImage2D], @[glGetTexEnv], @[glTexCoordPointer], @[glTexParameter], @[glDisableClientState], @[glTexSubImage1D], @[glTexImage1D], @[glCopyTexSubImage1D], @[glPopMatrix] and @[glTexGen]
 */

/*!@decl constant GL_TEXTURE_1D 3552
 *! Used in @[glIsEnabled], @[glGetTexLevelParameter], @[glGetIntegerv], @[glBindTexture], @[glDisable], @[glCopyTexImage1D], @[glPopAttrib], @[glEnable], @[glGetTexParameter], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glTexParameter], @[glTexSubImage1D], @[glTexImage1D] and @[glCopyTexSubImage1D]
 */

/*!@decl constant GL_TEXTURE_2D 3553
 *! Used in @[glIsEnabled], @[glGetTexLevelParameter], @[glGetIntegerv], @[glCopyTexSubImage2D], @[glBindTexture], @[glDisable], @[glTexSubImage2D], @[glPopAttrib], @[glEnable], @[glGetTexParameter], @[glGetBooleanv], @[glGetTexImage], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev], @[glCopyTexImage2D], @[glTexImage2D] and @[glTexParameter]
 */

/*!@decl constant GL_TEXTURE_3D_EXT 32879
 */

/*!@decl constant GL_TEXTURE_ALPHA_SIZE 32863
 *! Used in @[glGetTexLevelParameter]
 */

/*!@decl constant GL_TEXTURE_BINDING_1D 32872
 */

/*!@decl constant GL_TEXTURE_BINDING_2D 32873
 */

/*!@decl constant GL_TEXTURE_BIT 262144
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_TEXTURE_BLUE_SIZE 32862
 *! Used in @[glGetTexLevelParameter]
 */

/*!@decl constant GL_TEXTURE_BORDER 4101
 *! Used in @[glGetTexLevelParameter], @[glCopyTexSubImage2D], @[glGetTexParameter], @[glTexParameter], @[glTexSubImage1D] and @[glCopyTexSubImage1D]
 */

/*!@decl constant GL_TEXTURE_BORDER_COLOR 4100
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_TEXTURE_COLOR_TABLE_SGI 32956
 */

/*!@decl constant GL_TEXTURE_COMPONENTS 4099
 */

/*!@decl constant GL_TEXTURE_COORD_ARRAY 32888
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glEnableClientState], @[glGetPointerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glTexCoordPointer] and @[glDisableClientState]
 */

/*!@decl constant GL_TEXTURE_COORD_ARRAY_POINTER 32914
 *! Used in @[glGetPointerv]
 */

/*!@decl constant GL_TEXTURE_COORD_ARRAY_SIZE 32904
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_TEXTURE_COORD_ARRAY_STRIDE 32906
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_TEXTURE_COORD_ARRAY_TYPE 32905
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_TEXTURE_DEPTH_EXT 32881
 */

/*!@decl constant GL_TEXTURE_ENV 8960
 *! Used in @[glTexEnv] and @[glGetTexEnv]
 */

/*!@decl constant GL_TEXTURE_ENV_COLOR 8705
 *! Used in @[glTexEnv] and @[glGetTexEnv]
 */

/*!@decl constant GL_TEXTURE_ENV_MODE 8704
 *! Used in @[glTexEnv] and @[glGetTexEnv]
 */

/*!@decl constant GL_TEXTURE_GEN_MODE 9472
 *! Used in @[glGetTexGen], @[glPopAttrib], @[glPushAttrib] and @[glTexGen]
 */

/*!@decl constant GL_TEXTURE_GEN_Q 3171
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glTexGen]
 */

/*!@decl constant GL_TEXTURE_GEN_R 3170
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glTexGen]
 */

/*!@decl constant GL_TEXTURE_GEN_S 3168
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glTexGen]
 */

/*!@decl constant GL_TEXTURE_GEN_T 3169
 *! Used in @[glIsEnabled], @[glGetIntegerv], @[glDisable], @[glEnable], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glTexGen]
 */

/*!@decl constant GL_TEXTURE_GREEN_SIZE 32861
 *! Used in @[glGetTexLevelParameter]
 */

/*!@decl constant GL_TEXTURE_HEIGHT 4097
 *! Used in @[glGetTexLevelParameter], @[glCopyTexSubImage2D] and @[glTexSubImage2D]
 */

/*!@decl constant GL_TEXTURE_INTENSITY_SIZE 32865
 *! Used in @[glGetTexLevelParameter]
 */

/*!@decl constant GL_TEXTURE_INTERNAL_FORMAT 4099
 *! Used in @[glGetTexLevelParameter]
 */

/*!@decl constant GL_TEXTURE_LUMINANCE_SIZE 32864
 *! Used in @[glGetTexLevelParameter]
 */

/*!@decl constant GL_TEXTURE_MAG_FILTER 10240
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_TEXTURE_MATRIX 2984
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_TEXTURE_MIN_FILTER 10241
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_TEXTURE_PRIORITY 32870
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_TEXTURE_RED_SIZE 32860
 *! Used in @[glGetTexLevelParameter]
 */

/*!@decl constant GL_TEXTURE_RESIDENT 32871
 *! Used in @[glAreTexturesResident] and @[glGetTexParameter]
 */

/*!@decl constant GL_TEXTURE_STACK_DEPTH 2981
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_TEXTURE_WIDTH 4096
 *! Used in @[glGetTexLevelParameter], @[glCopyTexSubImage2D], @[glTexSubImage2D], @[glTexSubImage1D] and @[glCopyTexSubImage1D]
 */

/*!@decl constant GL_TEXTURE_WRAP_R_EXT 32882
 */

/*!@decl constant GL_TEXTURE_WRAP_S 10242
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_TEXTURE_WRAP_T 10243
 *! Used in @[glGetTexParameter] and @[glTexParameter]
 */

/*!@decl constant GL_TRANSFORM_BIT 4096
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_TRIANGLES 4
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays] and @[glBegin]
 */

/*!@decl constant GL_TRIANGLE_FAN 6
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays] and @[glBegin]
 */

/*!@decl constant GL_TRIANGLE_STRIP 5
 *! Used in @[glEnd], @[glDrawElements], @[glDrawArrays] and @[glBegin]
 */

/*!@decl constant GL_TRUE 1
 *! Used in @[glIsEnabled], @[glIsList], @[glAreTexturesResident], @[glGetIntegerv], @[glIsTexture], @[glDisable], @[glEdgeFlag], @[glEnable], @[glColorMask], @[glGetTexParameter], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glReadPixels] and @[glEdgeFlagv]
 */

/*!@decl constant GL_UNPACK_ALIGNMENT 3317
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glDrawPixels], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_UNPACK_IMAGE_HEIGHT_EXT 32878
 */

/*!@decl constant GL_UNPACK_LSB_FIRST 3313
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glDrawPixels], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev], @[glTexImage2D], @[glTexImage1D] and @[glPolygonStipple]
 */

/*!@decl constant GL_UNPACK_ROW_LENGTH 3314
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_UNPACK_SKIP_IMAGES_EXT 32877
 */

/*!@decl constant GL_UNPACK_SKIP_PIXELS 3316
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_UNPACK_SKIP_ROWS 3315
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_UNPACK_SWAP_BYTES 3312
 *! Used in @[glPixelStore], @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glPolygonStipple]
 */

/*!@decl constant GL_UNSIGNED_BYTE 5121
 *! Used in @[glDrawElements], @[glDrawPixels], @[glColorPointer], @[glTexSubImage2D], @[glGetTexImage], @[glCallLists], @[glReadPixels], @[glTexImage2D], @[glIndexPointer], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_UNSIGNED_INT 5125
 *! Used in @[glDrawElements], @[glDrawPixels], @[glColorPointer], @[glTexSubImage2D], @[glGetTexImage], @[glCallLists], @[glReadPixels], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_UNSIGNED_INT_8_8_8_8 32821
 */

/*!@decl constant GL_UNSIGNED_INT_8_8_8_8_REV 33639
 */

/*!@decl constant GL_UNSIGNED_SHORT 5123
 *! Used in @[glDrawElements], @[glDrawPixels], @[glColorPointer], @[glTexSubImage2D], @[glGetTexImage], @[glCallLists], @[glReadPixels], @[glTexImage2D], @[glTexSubImage1D] and @[glTexImage1D]
 */

/*!@decl constant GL_V2F 10784
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_V3F 10785
 *! Used in @[glInterleavedArrays]
 */

/*!@decl constant GL_VENDOR 7936
 *! Used in @[glGetString]
 */

/*!@decl constant GL_VERSION 7938
 *! Used in @[glGetString]
 */

/*!@decl constant GL_VERSION_1_1 1
 */

/*!@decl constant GL_VERTEX_ARRAY 32884
 *! Used in @[glIsEnabled], @[glArrayElement], @[glGetIntegerv], @[glDrawElements], @[glDrawArrays], @[glVertexPointer], @[glEnableClientState], @[glGetPointerv], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glDisableClientState]
 */

/*!@decl constant GL_VERTEX_ARRAY_POINTER 32910
 *! Used in @[glGetPointerv]
 */

/*!@decl constant GL_VERTEX_ARRAY_SIZE 32890
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_VERTEX_ARRAY_STRIDE 32892
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_VERTEX_ARRAY_TYPE 32891
 *! Used in @[glGetIntegerv], @[glGetBooleanv], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_VIEWPORT 2978
 *! Used in @[glGetIntegerv], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv] and @[glGetDoublev]
 */

/*!@decl constant GL_VIEWPORT_BIT 2048
 *! Used in @[glPopAttrib] and @[glPushAttrib]
 */

/*!@decl constant GL_WRAP_BORDER_SUN 33236
 */

/*!@decl constant GL_XOR 5382
 *! Used in @[glLogicOp]
 */

/*!@decl constant GL_ZERO 0
 *! Used in @[glGetIntegerv], @[glBlendFunc], @[glGetBooleanv], @[glGetFloatv], @[glGetDoublev] and @[glStencilOp]
 */

/*!@decl constant GL_ZOOM_X 3350
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glCopyPixels]
 */

/*!@decl constant GL_ZOOM_Y 3351
 *! Used in @[glGetIntegerv], @[glDrawPixels], @[glPopAttrib], @[glGetBooleanv], @[glPushAttrib], @[glGetFloatv], @[glGetDoublev] and @[glCopyPixels]
 */


/*! @endmodule
 */
