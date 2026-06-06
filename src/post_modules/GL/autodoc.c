/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* AutoDoc generated from OpenGL docbook pages */

/*! @module GL
 *! 
 *! OpenGL glue. All method and constant names have been kept close to
 *! their low level counterparts for easy adoption of OpenGL code from
 *! other languages and examples off the web. Superfluous suffixes
 *! specifying the number and types of arguments have been dropped,
 *! though.
 *! 
 *! @note
 *! All OpenGL methods have not been implemented. For a list of
 *! unimplemented methods, see @[glUnimplemented].
 */
/*! @decl private void glUnimplemented()
 *! @decl private void glActiveTexture()
 *! @decl private void glAreTexturesResident()
 *! @decl private void glAttachShader()
 *! @decl private void glBeginQuery()
 *! @decl private void glBindAttribLocation()
 *! @decl private void glBindBuffer()
 *! @decl private void glBitmap()
 *! @decl private void glBlendColor()
 *! @decl private void glBlendEquation()
 *! @decl private void glBlendEquationSeparate()
 *! @decl private void glBlendFuncSeparate()
 *! @decl private void glBufferData()
 *! @decl private void glBufferSubData()
 *! @decl private void glClientActiveTexture()
 *! @decl private void glColorSubTable()
 *! @decl private void glColorTable()
 *! @decl private void glColorTableParameter()
 *! @decl private void glCompileShader()
 *! @decl private void glCompressedTexImage1D()
 *! @decl private void glCompressedTexImage2D()
 *! @decl private void glCompressedTexImage3D()
 *! @decl private void glCompressedTexSubImage1D()
 *! @decl private void glCompressedTexSubImage2D()
 *! @decl private void glCompressedTexSubImage3D()
 *! @decl private void glConvolutionFilter1D()
 *! @decl private void glConvolutionFilter2D()
 *! @decl private void glConvolutionParameter()
 *! @decl private void glCopyColorSubTable()
 *! @decl private void glCopyColorTable()
 *! @decl private void glCopyConvolutionFilter1D()
 *! @decl private void glCopyConvolutionFilter2D()
 *! @decl private void glCopyTexSubImage3D()
 *! @decl private void glCreateProgram()
 *! @decl private void glCreateShader()
 *! @decl private void glDeleteBuffers()
 *! @decl private void glDeleteProgram()
 *! @decl private void glDeleteQueries()
 *! @decl private void glDeleteShader()
 *! @decl private void glDetachShader()
 *! @decl private void glDrawBuffers()
 *! @decl private void glDrawRangeElements()
 *! @decl private void glEnableVertexAttribArray()
 *! @decl private void glFogCoord()
 *! @decl private void glFogCoordPointer()
 *! @decl private void glGenBuffers()
 *! @decl private void glGenQueries()
 *! @decl private void glGetActiveAttrib()
 *! @decl private void glGetActiveUniform()
 *! @decl private void glGetAttachedShaders()
 *! @decl private void glGetAttribLocation()
 *! @decl private void glGetBufferParameteriv()
 *! @decl private void glGetBufferPointerv()
 *! @decl private void glGetBufferSubData()
 *! @decl private void glGetClipPlane()
 *! @decl private void glGetColorTable()
 *! @decl private void glGetColorTableParameter()
 *! @decl private void glGetCompressedTexImage()
 *! @decl private void glGetConvolutionFilter()
 *! @decl private void glGetConvolutionParameter()
 *! @decl private void glGetHistogram()
 *! @decl private void glGetHistogramParameter()
 *! @decl private void glGetLight()
 *! @decl private void glGetMap()
 *! @decl private void glGetMaterial()
 *! @decl private void glGetMinmax()
 *! @decl private void glGetMinmaxParameter()
 *! @decl private void glGetPixelMap()
 *! @decl private void glGetPointerv()
 *! @decl private void glGetPolygonStipple()
 *! @decl private void glGetProgramInfoLog()
 *! @decl private void glGetProgramiv()
 *! @decl private void glGetQueryObject()
 *! @decl private void glGetQueryiv()
 *! @decl private void glGetSeparableFilter()
 *! @decl private void glGetShaderInfoLog()
 *! @decl private void glGetShaderSource()
 *! @decl private void glGetShaderiv()
 *! @decl private void glGetTexEnv()
 *! @decl private void glGetTexGen()
 *! @decl private void glGetTexLevelParameter()
 *! @decl private void glGetTexParameter()
 *! @decl private void glGetUniform()
 *! @decl private void glGetUniformLocation()
 *! @decl private void glGetVertexAttrib()
 *! @decl private void glGetVertexAttribPointerv()
 *! @decl private void glHistogram()
 *! @decl private void glIsBuffer()
 *! @decl private void glIsProgram()
 *! @decl private void glIsQuery()
 *! @decl private void glIsShader()
 *! @decl private void glLinkProgram()
 *! @decl private void glLoadTransposeMatrix()
 *! @decl private void glMap1()
 *! @decl private void glMap2()
 *! @decl private void glMapBuffer()
 *! @decl private void glMinmax()
 *! @decl private void glMultTransposeMatrix()
 *! @decl private void glMultiDrawArrays()
 *! @decl private void glMultiDrawElements()
 *! @decl private void glMultiTexCoord()
 *! @decl private void glPixelMap()
 *! @decl private void glPixelStore()
 *! @decl private void glPixelTransfer()
 *! @decl private void glPointParameter()
 *! @decl private void glPolygonStipple()
 *! @decl private void glPrioritizeTextures()
 *! @decl private void glRect()
 *! @decl private void glResetHistogram()
 *! @decl private void glResetMinmax()
 *! @decl private void glSampleCoverage()
 *! @decl private void glSecondaryColor()
 *! @decl private void glSecondaryColorPointer()
 *! @decl private void glSeparableFilter2D()
 *! @decl private void glShaderSource()
 *! @decl private void glStencilFuncSeparate()
 *! @decl private void glStencilMaskSeparate()
 *! @decl private void glStencilOpSeparate()
 *! @decl private void glTexImage3D()
 *! @decl private void glTexSubImage3D()
 *! @decl private void glUniform()
 *! @decl private void glUseProgram()
 *! @decl private void glValidateProgram()
 *! @decl private void glVertexAttrib()
 *! @decl private void glVertexAttribPointer()
 *! @decl private void glWindowPos()
 *! @decl private void glXChooseFBConfig()
 *! @decl private void glXChooseVisual()
 *! @decl private void glXCopyContext()
 *! @decl private void glXCreateContext()
 *! @decl private void glXCreateGLXPixmap()
 *! @decl private void glXCreateNewContext()
 *! @decl private void glXCreatePbuffer()
 *! @decl private void glXCreatePixmap()
 *! @decl private void glXCreateWindow()
 *! @decl private void glXDestroyContext()
 *! @decl private void glXDestroyGLXPixmap()
 *! @decl private void glXDestroyPbuffer()
 *! @decl private void glXDestroyPixmap()
 *! @decl private void glXDestroyWindow()
 *! @decl private void glXFreeContextEXT()
 *! @decl private void glXGetClientString()
 *! @decl private void glXGetConfig()
 *! @decl private void glXGetContextIDEXT()
 *! @decl private void glXGetCurrentContext()
 *! @decl private void glXGetCurrentDisplay()
 *! @decl private void glXGetCurrentDrawable()
 *! @decl private void glXGetCurrentReadDrawable()
 *! @decl private void glXGetFBConfigAttrib()
 *! @decl private void glXGetFBConfigs()
 *! @decl private void glXGetProcAddress()
 *! @decl private void glXGetSelectedEvent()
 *! @decl private void glXGetVisualFromFBConfig()
 *! @decl private void glXImportContextEXT()
 *! @decl private void glXIsDirect()
 *! @decl private void glXMakeContextCurrent()
 *! @decl private void glXMakeCurrent()
 *! @decl private void glXQueryContext()
 *! @decl private void glXQueryContextInfoEXT()
 *! @decl private void glXQueryDrawable()
 *! @decl private void glXQueryExtension()
 *! @decl private void glXQueryExtensionsString()
 *! @decl private void glXQueryServerString()
 *! @decl private void glXQueryVersion()
 *! @decl private void glXSelectEvent()
 *! @decl private void glXSwapBuffers()
 *! @decl private void glXUseXFont()
 *! @decl private void glXWaitGL()
 *! @decl private void glXWaitX()
 *! @decl private void gluBeginCurve()
 *! @decl private void gluBeginPolygon()
 *! @decl private void gluBeginSurface()
 *! @decl private void gluBeginTrim()
 *! @decl private void gluBuild1DMipmapLevels()
 *! @decl private void gluBuild1DMipmaps()
 *! @decl private void gluBuild2DMipmapLevels()
 *! @decl private void gluBuild2DMipmaps()
 *! @decl private void gluBuild3DMipmapLevels()
 *! @decl private void gluBuild3DMipmaps()
 *! @decl private void gluCheckExtension()
 *! @decl private void gluCylinder()
 *! @decl private void gluDeleteNurbsRenderer()
 *! @decl private void gluDeleteQuadric()
 *! @decl private void gluDeleteTess()
 *! @decl private void gluDisk()
 *! @decl private void gluErrorString()
 *! @decl private void gluGetNurbsProperty()
 *! @decl private void gluGetString()
 *! @decl private void gluGetTessProperty()
 *! @decl private void gluLoadSamplingMatrices()
 *! @decl private void gluLookAt()
 *! @decl private void gluNewNurbsRenderer()
 *! @decl private void gluNewQuadric()
 *! @decl private void gluNewTess()
 *! @decl private void gluNextContour()
 *! @decl private void gluNurbsCallback()
 *! @decl private void gluNurbsCallbackData()
 *! @decl private void gluNurbsCallbackDataEXT()
 *! @decl private void gluNurbsCurve()
 *! @decl private void gluNurbsProperty()
 *! @decl private void gluNurbsSurface()
 *! @decl private void gluOrtho2D()
 *! @decl private void gluPartialDisk()
 *! @decl private void gluPerspective()
 *! @decl private void gluPickMatrix()
 *! @decl private void gluProject()
 *! @decl private void gluPwlCurve()
 *! @decl private void gluQuadricCallback()
 *! @decl private void gluQuadricDrawStyle()
 *! @decl private void gluQuadricNormals()
 *! @decl private void gluQuadricOrientation()
 *! @decl private void gluQuadricTexture()
 *! @decl private void gluScaleImage()
 *! @decl private void gluSphere()
 *! @decl private void gluTessBeginContour()
 *! @decl private void gluTessBeginPolygon()
 *! @decl private void gluTessCallback()
 *! @decl private void gluTessEndPolygon()
 *! @decl private void gluTessNormal()
 *! @decl private void gluTessProperty()
 *! @decl private void gluTessVertex()
 *! @decl private void gluUnProject()
 *! @decl private void gluUnProject4()
 *! 
 *! These OpenGL methods are still missing in the Pike @[GL] API.
 *! 
 *! @seealso
 *! @[GL]
 */


/*! @decl void glAccum(int op, float value)
 *! 
 *! The accumulation buffer is an extended-range color buffer. Images are
 *! not rendered into it. Rather, images rendered into one of the color
 *! buffers are added to the contents of the accumulation buffer after
 *! rendering. Effects such as antialiasing (of points, lines, and
 *! polygons), motion blur, and depth of field can be created by
 *! accumulating images generated with different transformation matrices.
 *! 
 *! Each pixel in the accumulation buffer consists of red, green, blue,
 *! and alpha values. The number of bits per component in the accumulation
 *! buffer depends on the implementation. You can examine this number by
 *! calling @[glGet] four times, with arguments @[GL_ACCUM_RED_BITS],
 *! @[GL_ACCUM_GREEN_BITS], @[GL_ACCUM_BLUE_BITS], and
 *! @[GL_ACCUM_ALPHA_BITS]. Regardless of the number of bits per
 *! component, the range of values stored by each component is [-1, 1].
 *! The accumulation buffer pixels are mapped one-to-one with frame buffer
 *! pixels.
 *! 
 *! @[glAccum] operates on the accumulation buffer. The first argument,
 *! @i{op@}, is a symbolic constant that selects an accumulation buffer
 *! operation. The second argument, @i{value@}, is a floating-point value
 *! to be used in that operation. Five operations are specified:
 *! @[GL_ACCUM], @[GL_LOAD], @[GL_ADD], @[GL_MULT], and @[GL_RETURN].
 *! 
 *! All accumulation buffer operations are limited to the area of the
 *! current scissor box and applied identically to the red, green, blue,
 *! and alpha components of each pixel. If a @[glAccum] operation results
 *! in a value outside the range [-1, 1], the contents of an accumulation
 *! buffer pixel component are undefined.
 *! 
 *! The operations are as follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_ACCUM</ref></c>
 *! <c><p>Obtains R, G, B, and A values from the buffer currently selected
 *! for reading (see <ref>glReadBuffer</ref>). Each component value is
 *! divided by 2<sup><i>n</i></sup>-1, where <i>n</i> is the number of
 *! bits allocated to each color component in the currently selected
 *! buffer. The result is a floating-point value in the range [0, 1],
 *! which is multiplied by <i>value</i> and added to the corresponding
 *! pixel component in the accumulation buffer, thereby updating the
 *! accumulation buffer. </p></c></r>
 *! <r><c><ref>GL_LOAD</ref></c>
 *! <c><p>Similar to <ref>GL_ACCUM</ref>, except that the current value in
 *! the accumulation buffer is not used in the calculation of the new
 *! value. That is, the R, G, B, and A values from the currently selected
 *! buffer are divided by 2<sup><i>n</i></sup>-1, multiplied by <i
 *! >value</i>, and then stored in the corresponding accumulation buffer
 *! cell, overwriting the current value. </p></c></r>
 *! <r><c><ref>GL_ADD</ref></c>
 *! <c><p>Adds <i>value</i> to each R, G, B, and A in the accumulation
 *! buffer. </p></c></r>
 *! <r><c><ref>GL_MULT</ref></c>
 *! <c><p>Multiplies each R, G, B, and A in the accumulation buffer by <i
 *! >value</i> and returns the scaled component to its corresponding
 *! accumulation buffer location. </p></c></r>
 *! <r><c><ref>GL_RETURN</ref></c>
 *! <c><p>Transfers accumulation buffer values to the color buffer or
 *! buffers currently selected for writing. Each R, G, B, and A component
 *! is multiplied by <i>value</i>, then multiplied by 2<sup><i>n</i></sup
 *! >-1, clamped to the range [0, 2<sup><i>n</i></sup>-1], and stored in
 *! the corresponding display buffer cell. The only fragment operations
 *! that are applied to this transfer are pixel ownership, scissor,
 *! dithering, and color writemasks. </p></c></r>
 *! </matrix>@}To clear the accumulation buffer, call @[glClearAccum] with
 *! R, G, B, and A values to set it to, then call @[glClear] with the
 *! accumulation buffer enabled.
 *! 
 *! @param op
 *! 
 *! Specifies the accumulation buffer operation. Symbolic constants
 *! @[GL_ACCUM], @[GL_LOAD], @[GL_ADD], @[GL_MULT], and @[GL_RETURN] are
 *! accepted.
 *! 
 *! @param value
 *! 
 *! Specifies a floating-point value used in the accumulation buffer
 *! operation. @i{op@} determines how @i{value@} is used.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{op@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if there is no accumulation
 *! buffer.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glAccum] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Only pixels within the current scissor box are updated by a @[glAccum]
 *! operation.
 *! 
 *! @seealso
 *! 
 *! @[glClear], @[glClearAccum], @[glCopyPixels], @[glDrawBuffer],
 *! @[glGet], @[glReadBuffer], @[glReadPixels], @[glScissor],
 *! @[glStencilOp]
 *! 
 */


/*! @decl void glAlphaFunc(int func, float ref)
 *! 
 *! The alpha test discards fragments depending on the outcome of a
 *! comparison between an incoming fragment's alpha value and a constant
 *! reference value. @[glAlphaFunc] specifies the reference value and the
 *! comparison function. The comparison is performed only if alpha testing
 *! is enabled. By default, it is not enabled. (See @[glEnable] and
 *! @[glDisable] of @[GL_ALPHA_TEST].)
 *! 
 *! @i{func@} and @i{ref@} specify the conditions under which the pixel is
 *! drawn. The incoming alpha value is compared to @i{ref@} using the
 *! function specified by @i{func@}. If the value passes the comparison,
 *! the incoming fragment is drawn if it also passes subsequent stencil
 *! and depth buffer tests. If the value fails the comparison, no change
 *! is made to the frame buffer at that pixel location. The comparison
 *! functions are as follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_NEVER</ref></c>
 *! <c><p>Never passes. </p></c></r>
 *! <r><c><ref>GL_LESS</ref></c>
 *! <c><p>Passes if the incoming alpha value is less than the reference
 *! value. </p></c></r>
 *! <r><c><ref>GL_EQUAL</ref></c>
 *! <c><p>Passes if the incoming alpha value is equal to the reference
 *! value. </p></c></r>
 *! <r><c><ref>GL_LEQUAL</ref></c>
 *! <c><p>Passes if the incoming alpha value is less than or equal to the
 *! reference value. </p></c></r>
 *! <r><c><ref>GL_GREATER</ref></c>
 *! <c><p>Passes if the incoming alpha value is greater than the reference
 *! value. </p></c></r>
 *! <r><c><ref>GL_NOTEQUAL</ref></c>
 *! <c><p>Passes if the incoming alpha value is not equal to the reference
 *! value. </p></c></r>
 *! <r><c><ref>GL_GEQUAL</ref></c>
 *! <c><p>Passes if the incoming alpha value is greater than or equal to
 *! the reference value. </p></c></r>
 *! <r><c><ref>GL_ALWAYS</ref></c>
 *! <c><p>Always passes (initial value). </p></c></r>
 *! </matrix>@}@[glAlphaFunc] operates on all pixel write operations,
 *! including those resulting from the scan conversion of points, lines,
 *! polygons, and bitmaps, and from pixel draw and copy operations.
 *! @[glAlphaFunc] does not affect screen clear operations.
 *! 
 *! @param func
 *! 
 *! Specifies the alpha comparison function. Symbolic constants
 *! @[GL_NEVER], @[GL_LESS], @[GL_EQUAL], @[GL_LEQUAL], @[GL_GREATER],
 *! @[GL_NOTEQUAL], @[GL_GEQUAL], and @[GL_ALWAYS] are accepted. The
 *! initial value is @[GL_ALWAYS].
 *! 
 *! @param ref
 *! 
 *! Specifies the reference value that incoming alpha values are compared
 *! to. This value is clamped to the range [0, 1], where 0 represents the
 *! lowest possible alpha value and 1 the highest possible value. The
 *! initial reference value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{func@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glAlphaFunc] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Alpha testing is performed only in RGBA mode.
 *! 
 *! @seealso
 *! 
 *! @[glBlendFunc], @[glClear], @[glDepthFunc], @[glEnable],
 *! @[glStencilFunc]
 *! 
 */


/*! @decl void glArrayElement(int i)
 *! 
 *! @[glArrayElement] commands are used within @[glBegin]/@[glEnd] pairs
 *! to specify vertex and attribute data for point, line, and polygon
 *! primitives. If @[GL_VERTEX_ARRAY] is enabled when @[glArrayElement] is
 *! called, a single vertex is drawn, using vertex and attribute data
 *! taken from location @i{i@} of the enabled arrays. If
 *! @[GL_VERTEX_ARRAY] is not enabled, no drawing occurs but the
 *! attributes corresponding to the enabled arrays are modified.
 *! 
 *! Use @[glArrayElement] to construct primitives by indexing vertex data,
 *! rather than by streaming through arrays of data in first-to-last
 *! order. Because each call specifies only a single vertex, it is
 *! possible to explicitly specify per-primitive attributes such as a
 *! single normal for each triangle.
 *! 
 *! Changes made to array data between the execution of @[glBegin] and the
 *! corresponding execution of @[glEnd] may affect calls to
 *! @[glArrayElement] that are made within the same @[glBegin]/@[glEnd]
 *! period in nonsequential ways. That is, a call to @[glArrayElement]
 *! that precedes a change to array data may access the changed data, and
 *! a call that follows a change to array data may access original data.
 *! 
 *! @param i
 *! 
 *! Specifies an index into the enabled vertex data arrays.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] may be generated if @i{i@} is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to an enabled array and the buffer object's data store is
 *! currently mapped.
 *! 
 *! @note
 *! 
 *! @[glArrayElement] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @[glArrayElement] is included in display lists. If @[glArrayElement]
 *! is entered into a display list, the necessary array data (determined
 *! by the array pointers and enables) is also entered into the display
 *! list. Because the array pointers and enables are client-side state,
 *! their values affect display lists when the lists are created, not when
 *! the lists are executed.
 *! 
 *! @seealso
 *! 
 *! @[glClientActiveTexture], @[glColorPointer], @[glDrawArrays],
 *! @[glEdgeFlagPointer], @[glFogCoordPointer], @[glGetPointerv],
 *! @[glIndexPointer], @[glInterleavedArrays], @[glNormalPointer],
 *! @[glSecondaryColorPointer], @[glTexCoordPointer], @[glVertexPointer]
 *! 
 */


/*! @decl void glBindTexture(int target, int texture)
 *! 
 *! @[glBindTexture] lets you create or use a named texture. Calling
 *! @[glBindTexture] with @i{target@} set to @[GL_TEXTURE_1D],
 *! @[GL_TEXTURE_2D], @[GL_TEXTURE_3D] or @[GL_TEXTURE_CUBE_MAP] and
 *! @i{texture@} set to the name of the new texture binds the texture name
 *! to the target. When a texture is bound to a target, the previous
 *! binding for that target is automatically broken.
 *! 
 *! Texture names are unsigned integers. The value zero is reserved to
 *! represent the default texture for each texture target. Texture names
 *! and the corresponding texture contents are local to the shared
 *! display-list space (see @[glXCreateContext]) of the current GL
 *! rendering context; two rendering contexts share texture names only if
 *! they also share display lists.
 *! 
 *! You may use @[glGenTextures] to generate a set of new texture names.
 *! 
 *! When a texture is first bound, it assumes the specified target: A
 *! texture first bound to @[GL_TEXTURE_1D] becomes one-dimensional
 *! texture, a texture first bound to @[GL_TEXTURE_2D] becomes
 *! two-dimensional texture, a texture first bound to @[GL_TEXTURE_3D]
 *! becomes three-dimensional texture, and a texture first bound to
 *! @[GL_TEXTURE_CUBE_MAP] becomes a cube-mapped texture. The state of a
 *! one-dimensional texture immediately after it is first bound is
 *! equivalent to the state of the default @[GL_TEXTURE_1D] at GL
 *! initialization, and similarly for two- and three-dimensional textures
 *! and cube-mapped textures.
 *! 
 *! While a texture is bound, GL operations on the target to which it is
 *! bound affect the bound texture, and queries of the target to which it
 *! is bound return state from the bound texture. If texture mapping is
 *! active on the target to which a texture is bound, the bound texture is
 *! used. In effect, the texture targets become aliases for the textures
 *! currently bound to them, and the texture name zero refers to the
 *! default textures that were bound to them at initialization.
 *! 
 *! A texture binding created with @[glBindTexture] remains active until a
 *! different texture is bound to the same target, or until the bound
 *! texture is deleted with @[glDeleteTextures].
 *! 
 *! Once created, a named texture may be re-bound to its same original
 *! target as often as needed. It is usually much faster to use
 *! @[glBindTexture] to bind an existing named texture to one of the
 *! texture targets than it is to reload the texture image using
 *! @[glTexImage1D], @[glTexImage2D], or @[glTexImage3D]. For additional
 *! control over performance, use @[glPrioritizeTextures].
 *! 
 *! @[glBindTexture] is included in display lists.
 *! 
 *! @param target
 *! 
 *! Specifies the target to which the texture is bound. Must be either
 *! @[GL_TEXTURE_1D], @[GL_TEXTURE_2D], @[GL_TEXTURE_3D], or
 *! @[GL_TEXTURE_CUBE_MAP].
 *! 
 *! @param texture
 *! 
 *! Specifies the name of a texture.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is not one of the
 *! allowable values.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{texture@} was previously
 *! created with a target that doesn't match that of @i{target@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glBindTexture] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glBindTexture] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @[GL_TEXTURE_CUBE_MAP] is available only if the GL version is 1.3 or
 *! greater.
 *! 
 *! @seealso
 *! 
 *! @[glAreTexturesResident], @[glDeleteTextures], @[glGenTextures],
 *! @[glGet], @[glGetTexParameter], @[glIsTexture],
 *! @[glPrioritizeTextures], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexParameter]
 *! 
 */


/*! @decl void glBlendFunc(int sfactor, int dfactor)
 *! 
 *! In RGBA mode, pixels can be drawn using a function that blends the
 *! incoming (source) RGBA values with the RGBA values that are already in
 *! the frame buffer (the destination values). Blending is initially
 *! disabled. Use @[glEnable] and @[glDisable] with argument @[GL_BLEND]
 *! to enable and disable blending.
 *! 
 *! @[glBlendFunc] defines the operation of blending when it is enabled.
 *! @i{sfactor@} specifies which method is used to scale the source color
 *! components. @i{dfactor@} specifies which method is used to scale the
 *! destination color components. The possible methods are described in
 *! the following table. Each method defines four scale factors, one each
 *! for red, green, blue, and alpha. In the table and in subsequent
 *! equations, source and destination color components are referred to as
 *! (@i{R@}@sub{@i{s@}@}, @i{G@}@sub{@i{s@}@}, @i{B@}@sub{@i{s@}@},
 *! @i{A@}@sub{@i{s@}@}) and (@i{R@}@sub{@i{d@}@}, @i{G@}@sub{@i{d@}@},
 *! @i{B@}@sub{@i{d@}@}, @i{A@}@sub{@i{d@}@}). The color specified by
 *! @[glBlendColor] is referred to as (@i{R@}@sub{@i{c@}@},
 *! @i{G@}@sub{@i{c@}@}, @i{B@}@sub{@i{c@}@}, @i{A@}@sub{@i{c@}@}). They
 *! are understood to have integer values between 0 and
 *! (@i{k@}@sub{@i{R@}@}, @i{k@}@sub{@i{G@}@}, @i{k@}@sub{@i{B@}@},
 *! @i{k@}@sub{@i{A@}@}), where
 *! 
 *! @i{k@}@sub{@i{c@}@}=2@sup{@i{m@}@sub{@i{c@}@}@}-1
 *! 
 *! and (@i{m@}@sub{@i{R@}@}, @i{m@}@sub{@i{G@}@}, @i{m@}@sub{@i{B@}@},
 *! @i{m@}@sub{@i{A@}@}) is the number of red, green, blue, and alpha
 *! bitplanes.
 *! 
 *! Source and destination scale factors are referred to as
 *! (@i{s@}@sub{@i{R@}@}, @i{s@}@sub{@i{G@}@}, @i{s@}@sub{@i{B@}@},
 *! @i{s@}@sub{@i{A@}@}) and (@i{d@}@sub{@i{R@}@}, @i{d@}@sub{@i{G@}@},
 *! @i{d@}@sub{@i{B@}@}, @i{d@}@sub{@i{A@}@}). The scale factors described
 *! in the table, denoted (@i{f@}@sub{@i{R@}@}, @i{f@}@sub{@i{G@}@},
 *! @i{f@}@sub{@i{B@}@}, @i{f@}@sub{@i{A@}@}), represent either source or
 *! destination factors. All scale factors have range [0, 1].
 *! 
 *! @xml{<matrix><r><c><b>Parameter </b></c><c><b>(<i>f</i><sub><i>R</i
 *! ></sub>, <i>f</i><sub><i>G</i></sub>, <i>f</i><sub><i>B</i></sub>, <i
 *! >f</i><sub><i>A</i></sub>)</b></c></r><r><c><ref>GL_ZERO</ref></c><c
 *! >(0, 0, 0, 0)</c></r><r><c><ref>GL_ONE</ref></c><c>(1, 1, 1, 1)</c></r
 *! ><r><c><ref>GL_SRC_COLOR</ref></c><c>(<i>R</i><sub><i>s</i></sub>/<i
 *! >k</i><sub><i>R</i></sub>, <i>G</i><sub><i>s</i></sub>/<i>k</i><sub><i
 *! >G</i></sub>, <i>B</i><sub><i>s</i></sub>/<i>k</i><sub><i>B</i></sub>,
 *! <i>A</i><sub><i>s</i></sub>/<i>k</i><sub><i>A</i></sub>)</c></r><r><c
 *! ><ref>GL_ONE_MINUS_SRC_COLOR</ref></c><c> (1, 1, 1, 1)-(<i>R</i><sub
 *! ><i>s</i></sub>/<i>k</i><sub><i>R</i></sub>, <i>G</i><sub><i>s</i
 *! ></sub>/<i>k</i><sub><i>G</i></sub>, <i>B</i><sub><i>s</i></sub>/<i
 *! >k</i><sub><i>B</i></sub>, <i>A</i><sub><i>s</i></sub>/<i>k</i><sub><i
 *! >A</i></sub>)</c></r><r><c><ref>GL_DST_COLOR</ref></c><c>(<i>R</i><sub
 *! ><i>d</i></sub>/<i>k</i><sub><i>R</i></sub>, <i>G</i><sub><i>d</i
 *! ></sub>/<i>k</i><sub><i>G</i></sub>, <i>B</i><sub><i>d</i></sub>/<i
 *! >k</i><sub><i>B</i></sub>, <i>A</i><sub><i>d</i></sub>/<i>k</i><sub><i
 *! >A</i></sub>)</c></r><r><c><ref>GL_ONE_MINUS_DST_COLOR</ref></c><c>
 *! (1, 1, 1, 1)-(<i>R</i><sub><i>d</i></sub>/<i>k</i><sub><i>R</i></sub>,
 *! <i>G</i><sub><i>d</i></sub>/<i>k</i><sub><i>G</i></sub>, <i>B</i><sub
 *! ><i>d</i></sub>/<i>k</i><sub><i>B</i></sub>, <i>A</i><sub><i>d</i
 *! ></sub>/<i>k</i><sub><i>A</i></sub>)</c></r><r><c><ref
 *! >GL_SRC_ALPHA</ref></c><c>(<i>A</i><sub><i>s</i></sub>/<i>k</i><sub><i
 *! >A</i></sub>, <i>A</i><sub><i>s</i></sub>/<i>k</i><sub><i>A</i></sub>,
 *! <i>A</i><sub><i>s</i></sub>/<i>k</i><sub><i>A</i></sub>, <i>A</i><sub
 *! ><i>s</i></sub>/<i>k</i><sub><i>A</i></sub>)</c></r><r><c><ref
 *! >GL_ONE_MINUS_SRC_ALPHA</ref></c><c> (1, 1, 1, 1)-(<i>A</i><sub><i
 *! >s</i></sub>/<i>k</i><sub><i>A</i></sub>, <i>A</i><sub><i>s</i></sub
 *! >/<i>k</i><sub><i>A</i></sub>, <i>A</i><sub><i>s</i></sub>/<i>k</i
 *! ><sub><i>A</i></sub>, <i>A</i><sub><i>s</i></sub>/<i>k</i><sub><i>A</i
 *! ></sub>)</c></r><r><c><ref>GL_DST_ALPHA</ref></c><c>(<i>A</i><sub><i
 *! >d</i></sub>/<i>k</i><sub><i>A</i></sub>, <i>A</i><sub><i>d</i></sub
 *! >/<i>k</i><sub><i>A</i></sub>, <i>A</i><sub><i>d</i></sub>/<i>k</i
 *! ><sub><i>A</i></sub>, <i>A</i><sub><i>d</i></sub>/<i>k</i><sub><i>A</i
 *! ></sub>)</c></r><r><c><ref>GL_ONE_MINUS_DST_ALPHA</ref></c><c> (1, 1,
 *! 1, 1)-(<i>A</i><sub><i>d</i></sub>/<i>k</i><sub><i>A</i></sub>, <i
 *! >A</i><sub><i>d</i></sub>/<i>k</i><sub><i>A</i></sub>, <i>A</i><sub><i
 *! >d</i></sub>/<i>k</i><sub><i>A</i></sub>, <i>A</i><sub><i>d</i></sub
 *! >/<i>k</i><sub><i>A</i></sub>)</c></r><r><c><ref
 *! >GL_CONSTANT_COLOR</ref></c><c>(<i>R</i><sub><i>c</i></sub>, <i>G</i
 *! ><sub><i>c</i></sub>, <i>B</i><sub><i>c</i></sub>, <i>A</i><sub><i
 *! >c</i></sub>)</c></r><r><c><ref>GL_ONE_MINUS_CONSTANT_COLOR</ref></c
 *! ><c> (1, 1, 1, 1)-(<i>R</i><sub><i>c</i></sub>, <i>G</i><sub><i>c</i
 *! ></sub>, <i>B</i><sub><i>c</i></sub>, <i>A</i><sub><i>c</i></sub>)</c
 *! ></r><r><c><ref>GL_CONSTANT_ALPHA</ref></c><c>(<i>A</i><sub><i>c</i
 *! ></sub>, <i>A</i><sub><i>c</i></sub>, <i>A</i><sub><i>c</i></sub>, <i
 *! >A</i><sub><i>c</i></sub>)</c></r><r><c><ref
 *! >GL_ONE_MINUS_CONSTANT_ALPHA</ref></c><c> (1, 1, 1, 1)-(<i>A</i><sub
 *! ><i>c</i></sub>, <i>A</i><sub><i>c</i></sub>, <i>A</i><sub><i>c</i
 *! ></sub>, <i>A</i><sub><i>c</i></sub>)</c></r><r><c><ref
 *! >GL_SRC_ALPHA_SATURATE</ref></c><c>(<i>i</i>, <i>i</i>, <i>i</i>,
 *! 1)</c></r></matrix>@} In the table,
 *! 
 *! @i{i@}=min(@i{A@}@sub{@i{s@}@},
 *! @i{k@}@sub{@i{A@}@}-@i{A@}@sub{@i{d@}@})/@i{k@}@sub{@i{A@}@}
 *! 
 *! To determine the blended RGBA values of a pixel when drawing in RGBA
 *! mode, the system uses the following equations:
 *! 
 *! @i{R@}@sub{@i{d@}@}=min(@i{k@}@sub{@i{R@}@},
 *! @i{R@}@sub{@i{s@}@}@i{s@}@sub{@i{R@}@}+@i{R@}@sub{@i{d@}@}@i{d@}@sub{@i{R@}@})
 *! @i{G@}@sub{@i{d@}@}=min(@i{k@}@sub{@i{G@}@},
 *! @i{G@}@sub{@i{s@}@}@i{s@}@sub{@i{G@}@}+@i{G@}@sub{@i{d@}@}@i{d@}@sub{@i{G@}@})
 *! @i{B@}@sub{@i{d@}@}=min(@i{k@}@sub{@i{B@}@},
 *! @i{B@}@sub{@i{s@}@}@i{s@}@sub{@i{B@}@}+@i{B@}@sub{@i{d@}@}@i{d@}@sub{@i{B@}@})
 *! @i{A@}@sub{@i{d@}@}=min(@i{k@}@sub{@i{A@}@},
 *! @i{A@}@sub{@i{s@}@}@i{s@}@sub{@i{A@}@}+@i{A@}@sub{@i{d@}@}@i{d@}@sub{@i{A@}@})
 *! 
 *! Despite the apparent precision of the above equations, blending
 *! arithmetic is not exactly specified, because blending operates with
 *! imprecise integer color values. However, a blend factor that should be
 *! equal to 1 is guaranteed not to modify its multiplicand, and a blend
 *! factor equal to 0 reduces its multiplicand to 0. For example, when
 *! @i{sfactor@} is @[GL_SRC_ALPHA], @i{dfactor@} is
 *! @[GL_ONE_MINUS_SRC_ALPHA], and @i{A@}@sub{@i{s@}@} is equal to
 *! @i{k@}@sub{@i{A@}@}, the equations reduce to simple replacement:
 *! 
 *! @i{R@}@sub{@i{d@}@}=@i{R@}@sub{@i{s@}@}
 *! @i{G@}@sub{@i{d@}@}=@i{G@}@sub{@i{s@}@}
 *! @i{B@}@sub{@i{d@}@}=@i{B@}@sub{@i{s@}@}
 *! @i{A@}@sub{@i{d@}@}=@i{A@}@sub{@i{s@}@}
 *! 
 *! @param sfactor
 *! 
 *! Specifies how the red, green, blue, and alpha source blending factors
 *! are computed. The following symbolic constants are accepted:
 *! @[GL_ZERO], @[GL_ONE], @[GL_SRC_COLOR], @[GL_ONE_MINUS_SRC_COLOR],
 *! @[GL_DST_COLOR], @[GL_ONE_MINUS_DST_COLOR], @[GL_SRC_ALPHA],
 *! @[GL_ONE_MINUS_SRC_ALPHA], @[GL_DST_ALPHA], @[GL_ONE_MINUS_DST_ALPHA],
 *! @[GL_CONSTANT_COLOR], @[GL_ONE_MINUS_CONSTANT_COLOR],
 *! @[GL_CONSTANT_ALPHA], @[GL_ONE_MINUS_CONSTANT_ALPHA], and
 *! @[GL_SRC_ALPHA_SATURATE]. The initial value is @[GL_ONE].
 *! 
 *! @param dfactor
 *! 
 *! Specifies how the red, green, blue, and alpha destination blending
 *! factors are computed. The following symbolic constants are accepted:
 *! @[GL_ZERO], @[GL_ONE], @[GL_SRC_COLOR], @[GL_ONE_MINUS_SRC_COLOR],
 *! @[GL_DST_COLOR], @[GL_ONE_MINUS_DST_COLOR], @[GL_SRC_ALPHA],
 *! @[GL_ONE_MINUS_SRC_ALPHA], @[GL_DST_ALPHA], @[GL_ONE_MINUS_DST_ALPHA].
 *! @[GL_CONSTANT_COLOR], @[GL_ONE_MINUS_CONSTANT_COLOR],
 *! @[GL_CONSTANT_ALPHA], and @[GL_ONE_MINUS_CONSTANT_ALPHA]. The initial
 *! value is @[GL_ZERO].
 *! 
 *! @example
 *! 
 *! Transparency is best implemented using blend function
 *! (@[GL_SRC_ALPHA], @[GL_ONE_MINUS_SRC_ALPHA]) with primitives sorted
 *! from farthest to nearest. Note that this transparency calculation does
 *! not require the presence of alpha bitplanes in the frame buffer.
 *! 
 *! Blend function (@[GL_SRC_ALPHA], @[GL_ONE_MINUS_SRC_ALPHA]) is also
 *! useful for rendering antialiased points and lines in arbitrary order.
 *! 
 *! Polygon antialiasing is optimized using blend function
 *! (@[GL_SRC_ALPHA_SATURATE], @[GL_ONE]) with polygons sorted from
 *! nearest to farthest. (See the @[glEnable], @[glDisable] reference page
 *! and the @[GL_POLYGON_SMOOTH] argument for information on polygon
 *! antialiasing.) Destination alpha bitplanes, which must be present for
 *! this blend function to operate correctly, store the accumulated
 *! coverage.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if either @i{sfactor@} or @i{dfactor@}
 *! is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glBlendFunc] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Incoming (source) alpha is correctly thought of as a material opacity,
 *! ranging from 1.0 (@i{K@}@sub{@i{A@}@}), representing complete opacity,
 *! to 0.0 (0), representing complete transparency.
 *! 
 *! When more than one color buffer is enabled for drawing, the GL
 *! performs blending separately for each enabled buffer, using the
 *! contents of that buffer for destination color. (See @[glDrawBuffer].)
 *! 
 *! Blending affects only RGBA rendering. It is ignored by color index
 *! renderers.
 *! 
 *! @[GL_CONSTANT_COLOR], @[GL_ONE_MINUS_CONSTANT_COLOR],
 *! @[GL_CONSTANT_ALPHA], @[GL_ONE_MINUS_CONSTANT_ALPHA] are available
 *! only if the GL version is 1.4 or greater or if the @tt{ARB_imaging@}
 *! is supported by your implementation.
 *! 
 *! @[GL_SRC_COLOR] and @[GL_ONE_MINUS_SRC_COLOR] are valid only for
 *! @i{sfactor@} if the GL version is 1.4 or greater.
 *! 
 *! @[GL_DST_COLOR] and @[GL_ONE_MINUS_DST_COLOR] are valid only for
 *! @i{dfactor@} if the GL version is 1.4 or greater.
 *! 
 *! @seealso
 *! 
 *! @[glAlphaFunc], @[glBlendColor], @[glBlendEquation],
 *! @[glBlendFuncSeparate], @[glClear], @[glDrawBuffer], @[glEnable],
 *! @[glLogicOp], @[glStencilFunc]
 *! 
 */


/*! @decl void glCallList(int list)
 *! 
 *! @[glCallList] causes the named display list to be executed. The
 *! commands saved in the display list are executed in order, just as if
 *! they were called without using a display list. If @i{list@} has not
 *! been defined as a display list, @[glCallList] is ignored.
 *! 
 *! @[glCallList] can appear inside a display list. To avoid the
 *! possibility of infinite recursion resulting from display lists calling
 *! one another, a limit is placed on the nesting level of display lists
 *! during display-list execution. This limit is at least 64, and it
 *! depends on the implementation.
 *! 
 *! GL state is not saved and restored across a call to @[glCallList].
 *! Thus, changes made to GL state during the execution of a display list
 *! remain after execution of the display list is completed. Use
 *! @[glPushAttrib], @[glPopAttrib], @[glPushMatrix], and @[glPopMatrix]
 *! to preserve GL state across @[glCallList] calls.
 *! 
 *! @param list
 *! 
 *! Specifies the integer name of the display list to be executed.
 *! 
 *! @note
 *! 
 *! Display lists can be executed between a call to @[glBegin] and the
 *! corresponding call to @[glEnd], as long as the display list includes
 *! only commands that are allowed in this interval.
 *! 
 *! @seealso
 *! 
 *! @[glCallLists], @[glDeleteLists], @[glGenLists], @[glNewList],
 *! @[glPushAttrib], @[glPushMatrix]
 *! 
 */


/*! @decl void glClear(int mask)
 *! 
 *! @[glClear] sets the bitplane area of the window to values previously
 *! selected by @[glClearColor], @[glClearIndex], @[glClearDepth],
 *! @[glClearStencil], and @[glClearAccum]. Multiple color buffers can be
 *! cleared simultaneously by selecting more than one buffer at a time
 *! using @[glDrawBuffer].
 *! 
 *! The pixel ownership test, the scissor test, dithering, and the buffer
 *! writemasks affect the operation of @[glClear]. The scissor box bounds
 *! the cleared region. Alpha function, blend function, logical operation,
 *! stenciling, texture mapping, and depth-buffering are ignored by
 *! @[glClear].
 *! 
 *! @[glClear] takes a single argument that is the bitwise OR of several
 *! values indicating which buffer is to be cleared.
 *! 
 *! The values are as follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_COLOR_BUFFER_BIT</ref></c>
 *! <c><p>Indicates the buffers currently enabled for color writing. </p
 *! ></c></r>
 *! <r><c><ref>GL_DEPTH_BUFFER_BIT</ref></c>
 *! <c><p>Indicates the depth buffer. </p></c></r>
 *! <r><c><ref>GL_ACCUM_BUFFER_BIT</ref></c>
 *! <c><p>Indicates the accumulation buffer. </p></c></r>
 *! <r><c><ref>GL_STENCIL_BUFFER_BIT</ref></c>
 *! <c><p>Indicates the stencil buffer. </p></c></r>
 *! </matrix>@}The value to which each buffer is cleared depends on the
 *! setting of the clear value for that buffer.
 *! 
 *! @param mask
 *! 
 *! Bitwise OR of masks that indicate the buffers to be cleared. The four
 *! masks are @[GL_COLOR_BUFFER_BIT], @[GL_DEPTH_BUFFER_BIT],
 *! @[GL_ACCUM_BUFFER_BIT], and @[GL_STENCIL_BUFFER_BIT].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if any bit other than the four
 *! defined bits is set in @i{mask@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glClear] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! If a buffer is not present, then a @[glClear] directed at that buffer
 *! has no effect.
 *! 
 *! @seealso
 *! 
 *! @[glClearAccum], @[glClearColor], @[glClearDepth], @[glClearIndex],
 *! @[glClearStencil], @[glColorMask], @[glDepthMask], @[glDrawBuffer],
 *! @[glScissor], @[glStencilMask]
 *! 
 */


/*! @decl void glClearAccum(float|array(float) red, float|void green, @
 *! float|void blue, float|void alpha)
 *! 
 *! @[glClearAccum] specifies the red, green, blue, and alpha values used
 *! by @[glClear] to clear the accumulation buffer.
 *! 
 *! Values specified by @[glClearAccum] are clamped to the range [-1, 1].
 *! 
 *! @param red
 *! @param green
 *! @param blue
 *! @param alpha
 *! 
 *! Specify the red, green, blue, and alpha values used when the
 *! accumulation buffer is cleared. The initial values are all 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glClearAccum] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glAccum], @[glClear]
 *! 
 */


/*! @decl void glClearColor(float|array(float) red, float|void green, @
 *! float|void blue, float|void alpha)
 *! 
 *! @[glClearColor] specifies the red, green, blue, and alpha values used
 *! by @[glClear] to clear the color buffers. Values specified by
 *! @[glClearColor] are clamped to the range [0, 1].
 *! 
 *! @param red
 *! @param green
 *! @param blue
 *! @param alpha
 *! 
 *! Specify the red, green, blue, and alpha values used when the color
 *! buffers are cleared. The initial values are all 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glClearColor] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glClear]
 *! 
 */


/*! @decl void glClearDepth(float depth)
 *! 
 *! @[glClearDepth] specifies the depth value used by @[glClear] to clear
 *! the depth buffer. Values specified by @[glClearDepth] are clamped to
 *! the range [0, 1].
 *! 
 *! @param depth
 *! 
 *! Specifies the depth value used when the depth buffer is cleared. The
 *! initial value is 1.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glClearDepth] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glClear]
 *! 
 */


/*! @decl void glClearIndex(float c)
 *! 
 *! @[glClearIndex] specifies the index used by @[glClear] to clear the
 *! color index buffers. @i{c@} is not clamped. Rather, @i{c@} is
 *! converted to a fixed-point value with unspecified precision to the
 *! right of the binary point. The integer part of this value is then
 *! masked with
 *! 2@sup{@i{m@}@}-1
 *! , where @i{m@} is the number of bits in a color index stored in the
 *! frame buffer.
 *! 
 *! @param c
 *! 
 *! Specifies the index used when the color index buffers are cleared. The
 *! initial value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glClearIndex] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glClear]
 *! 
 */


/*! @decl void glClearStencil(int s)
 *! 
 *! @[glClearStencil] specifies the index used by @[glClear] to clear the
 *! stencil buffer. @i{s@} is masked with
 *! 2@sup{@i{m@}@}-1
 *! , where @i{m@} is the number of bits in the stencil buffer.
 *! 
 *! @param s
 *! 
 *! Specifies the index used when the stencil buffer is cleared. The
 *! initial value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glClearStencil] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glClear], @[glStencilFunc], @[glStencilFuncSeparate],
 *! @[glStencilMask], @[glStencilMaskSeparate], @[glStencilOp],
 *! @[glStencilOpSeparate]
 *! 
 */


/*! @decl void glClipPlane(int plane, float|array(float) equation_0, @
 *! float|void equation_1, float|void equation_2, float|void equation_3)
 *! 
 *! Geometry is always clipped against the boundaries of a six-plane
 *! frustum in @i{x@}, @i{y@}, and @i{z@}. @[glClipPlane] allows the
 *! specification of additional planes, not necessarily perpendicular to
 *! the @i{x@}, @i{y@}, or @i{z@} axis, against which all geometry is
 *! clipped. To determine the maximum number of additional clipping
 *! planes, call @[glGet] with argument @[GL_MAX_CLIP_PLANES]. All
 *! implementations support at least six such clipping planes. Because the
 *! resulting clipping region is the intersection of the defined
 *! half-spaces, it is always convex.
 *! 
 *! @[glClipPlane] specifies a half-space using a four-component plane
 *! equation. When @[glClipPlane] is called, @i{equation@} is transformed
 *! by the inverse of the modelview matrix and stored in the resulting eye
 *! coordinates. Subsequent changes to the modelview matrix have no effect
 *! on the stored plane-equation components. If the dot product of the eye
 *! coordinates of a vertex with the stored plane equation components is
 *! positive or zero, the vertex is @i{in@} with respect to that clipping
 *! plane. Otherwise, it is @i{out@}.
 *! 
 *! To enable and disable clipping planes, call @[glEnable] and
 *! @[glDisable] with the argument @tt{GL_CLIP_PLANE@}@i{i@}, where @i{i@}
 *! is the plane number.
 *! 
 *! All clipping planes are initially defined as (0, 0, 0, 0) in eye
 *! coordinates and are disabled.
 *! 
 *! @param plane
 *! 
 *! Specifies which clipping plane is being positioned. Symbolic names of
 *! the form @tt{GL_CLIP_PLANE@}@i{i@}, where @i{i@} is an integer between
 *! 0 and @[GL_MAX_CLIP_PLANES]-1, are accepted.
 *! 
 *! @param equation
 *! 
 *! Specifies the address of an array of four double-precision
 *! floating-point values. These values are interpreted as a plane
 *! equation.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{plane@} is not an accepted
 *! value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glClipPlane] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! It is always the case that @tt{GL_CLIP_PLANE@}@i{i@} =
 *! @[GL_CLIP_PLANE0] + @i{i@}.
 *! 
 *! @seealso
 *! 
 *! @[glEnable]
 *! 
 */


/*! @decl void glColor(float|int red, float|int green, float|int blue, @
 *! float|int|void alpha)
 *! @decl void glColor(array(float|int) v)
 *! 
 *! The GL stores both a current single-valued color index and a current
 *! four-valued RGBA color. @[glColor] sets a new four-valued RGBA color.
 *! If no alpha value has been given, 1.0 (full intensity) is implied.
 *! 
 *! Current color values are stored in floating-point format, with
 *! unspecified mantissa and exponent sizes. Unsigned integer color
 *! components, when specified, are linearly mapped to floating-point
 *! values such that the largest representable value maps to 1.0 (full
 *! intensity), and 0 maps to 0.0 (zero intensity). Signed integer color
 *! components, when specified, are linearly mapped to floating-point
 *! values such that the most positive representable value maps to 1.0,
 *! and the most negative representable value maps to -1.0. (Note that
 *! this mapping does not convert 0 precisely to 0.0.) Floating-point
 *! values are mapped directly.
 *! 
 *! Neither floating-point nor signed integer values are clamped to the
 *! range [0, 1] before the current color is updated. However, color
 *! components are clamped to this range before they are interpolated or
 *! written into a color buffer.
 *! 
 *! @param red
 *! @param green
 *! @param blue
 *! 
 *! Specify new red, green, and blue values for the current color.
 *! 
 *! @param alpha
 *! 
 *! Specifies a new alpha value for the current color.
 *! 
 *! @param v
 *! 
 *! Specifies an array that contains red, green, blue, and (sometimes)
 *! alpha values.
 *! 
 *! @note
 *! 
 *! The initial value for the current color is (1, 1, 1, 1).
 *! 
 *! The current color can be updated at any time. In particular,
 *! @[glColor] can be called between a call to @[glBegin] and the
 *! corresponding call to @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glColorPointer], @[glIndex], @[glSecondaryColor]
 *! 
 */


/*! @decl void glColorMask(int red, int green, int blue, int alpha)
 *! 
 *! @[glColorMask] specifies whether the individual color components in
 *! the frame buffer can or cannot be written. If @i{red@} is @[GL_FALSE],
 *! for example, no change is made to the red component of any pixel in
 *! any of the color buffers, regardless of the drawing operation
 *! attempted.
 *! 
 *! Changes to individual bits of components cannot be controlled. Rather,
 *! changes are either enabled or disabled for entire color components.
 *! 
 *! @param red
 *! @param green
 *! @param blue
 *! @param alpha
 *! 
 *! Specify whether red, green, blue, and alpha can or cannot be written
 *! into the frame buffer. The initial values are all @[GL_TRUE],
 *! indicating that the color components can be written.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glColorMask] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glClear], @[glColor], @[glColorPointer], @[glDepthMask], @[glIndex],
 *! @[glIndexPointer], @[glIndexMask], @[glStencilMask]
 *! 
 */


/*! @decl void glColorMaterial(int face, int mode)
 *! 
 *! @[glColorMaterial] specifies which material parameters track the
 *! current color. When @[GL_COLOR_MATERIAL] is enabled, the material
 *! parameter or parameters specified by @i{mode@}, of the material or
 *! materials specified by @i{face@}, track the current color at all
 *! times.
 *! 
 *! To enable and disable @[GL_COLOR_MATERIAL], call @[glEnable] and
 *! @[glDisable] with argument @[GL_COLOR_MATERIAL]. @[GL_COLOR_MATERIAL]
 *! is initially disabled.
 *! 
 *! @param face
 *! 
 *! Specifies whether front, back, or both front and back material
 *! parameters should track the current color. Accepted values are
 *! @[GL_FRONT], @[GL_BACK], and @[GL_FRONT_AND_BACK]. The initial value
 *! is @[GL_FRONT_AND_BACK].
 *! 
 *! @param mode
 *! 
 *! Specifies which of several material parameters track the current
 *! color. Accepted values are @[GL_EMISSION], @[GL_AMBIENT],
 *! @[GL_DIFFUSE], @[GL_SPECULAR], and @[GL_AMBIENT_AND_DIFFUSE]. The
 *! initial value is @[GL_AMBIENT_AND_DIFFUSE].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{face@} or @i{mode@} is not an
 *! accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glColorMaterial] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glColorMaterial] makes it possible to change a subset of material
 *! parameters for each vertex using only the @[glColor] command, without
 *! calling @[glMaterial]. If only such a subset of parameters is to be
 *! specified for each vertex, calling @[glColorMaterial] is preferable to
 *! calling @[glMaterial].
 *! 
 *! Call @[glColorMaterial] before enabling @[GL_COLOR_MATERIAL].
 *! 
 *! Calling @[glDrawElements], @[glDrawArrays], or @[glDrawRangeElements]
 *! may leave the current color indeterminate, if the color array is
 *! enabled. If @[glColorMaterial] is enabled while the current color is
 *! indeterminate, the lighting material state specified by @i{face@} and
 *! @i{mode@} is also indeterminate.
 *! 
 *! If the GL version is 1.1 or greater, and @[GL_COLOR_MATERIAL] is
 *! enabled, evaluated color values affect the results of the lighting
 *! equation as if the current color were being modified, but no change is
 *! made to the tracking lighting parameter of the current color.
 *! 
 *! @seealso
 *! 
 *! @[glColor], @[glColorPointer], @[glDrawArrays], @[glDrawElements],
 *! @[glDrawRangeElements], @[glEnable], @[glLight], @[glLightModel],
 *! @[glMaterial]
 *! 
 */


/*! @decl void glCopyPixels(int x, int y, int width, int height, int type)
 *! 
 *! @[glCopyPixels] copies a screen-aligned rectangle of pixels from the
 *! specified frame buffer location to a region relative to the current
 *! raster position. Its operation is well defined only if the entire
 *! pixel source region is within the exposed portion of the window.
 *! Results of copies from outside the window, or from regions of the
 *! window that are not exposed, are hardware dependent and undefined.
 *! 
 *! @i{x@} and @i{y@} specify the window coordinates of the lower left
 *! corner of the rectangular region to be copied. @i{width@} and
 *! @i{height@} specify the dimensions of the rectangular region to be
 *! copied. Both @i{width@} and @i{height@} must not be negative.
 *! 
 *! Several parameters control the processing of the pixel data while it
 *! is being copied. These parameters are set with three commands:
 *! @[glPixelTransfer], @[glPixelMap], and @[glPixelZoom]. This reference
 *! page describes the effects on @[glCopyPixels] of most, but not all, of
 *! the parameters specified by these three commands.
 *! 
 *! @[glCopyPixels] copies values from each pixel with the lower left-hand
 *! corner at
 *! (@i{x@}+@i{i@}, @i{y@}+@i{j@})
 *! for
 *! 0<=@i{i@}<width
 *! and
 *! 0<=@i{j@}<height
 *! . This pixel is said to be the @i{i@}th pixel in the @i{j@}th row.
 *! Pixels are copied in row order from the lowest to the highest row,
 *! left to right in each row.
 *! 
 *! @i{type@} specifies whether color, depth, or stencil data is to be
 *! copied. The details of the transfer for each data type are as follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_COLOR</ref></c>
 *! <c><p>Indices or RGBA colors are read from the buffer currently
 *! specified as the read source buffer (see <ref>glReadBuffer</ref>). If
 *! the GL is in color index mode, each index that is read from this
 *! buffer is converted to a fixed-point format with an unspecified number
 *! of bits to the right of the binary point. Each index is then shifted
 *! left by <ref>GL_INDEX_SHIFT</ref> bits, and added to <ref
 *! >GL_INDEX_OFFSET</ref>. If <ref>GL_INDEX_SHIFT</ref> is negative, the
 *! shift is to the right. In either case, zero bits fill otherwise
 *! unspecified bit locations in the result. If <ref>GL_MAP_COLOR</ref> is
 *! true, the index is replaced with the value that it references in
 *! lookup table <ref>GL_PIXEL_MAP_I_TO_I</ref>. Whether the lookup
 *! replacement of the index is done or not, the integer part of the index
 *! is then ANDed with 2<sup><i>b</i></sup>-1, where <i>b</i> is the
 *! number of bits in a color index buffer. </p><p>If the GL is in RGBA
 *! mode, the red, green, blue, and alpha components of each pixel that is
 *! read are converted to an internal floating-point format with
 *! unspecified precision. The conversion maps the largest representable
 *! component value to 1.0, and component value 0 to 0.0. The resulting
 *! floating-point color values are then multiplied by <tt>GL_<i>c</i
 *! >_SCALE</tt> and added to <tt>GL_<i>c</i>_BIAS</tt>, where <i>c</i> is
 *! RED, GREEN, BLUE, and ALPHA for the respective color components. The
 *! results are clamped to the range [0,1]. If <ref>GL_MAP_COLOR</ref> is
 *! true, each color component is scaled by the size of lookup table <tt
 *! >GL_PIXEL_MAP_<i>c</i>_TO_<i>c</i></tt>, then replaced by the value
 *! that it references in that table. <i>c</i> is R, G, B, or A. </p><p>If
 *! the <tt>ARB_imaging</tt> extension is supported, the color values may
 *! be additionally processed by color-table lookups, color-matrix
 *! transformations, and convolution filters. </p><p>The GL then converts
 *! the resulting indices or RGBA colors to fragments by attaching the
 *! current raster position <i>z</i> coordinate and texture coordinates to
 *! each pixel, then assigning window coordinates (<i>x</i><sub><i>r</i
 *! ></sub>+<i>i</i>, <i>y</i><sub><i>r</i></sub>+<i>j</i>), where (<i
 *! >x</i><sub><i>r</i></sub>, <i>y</i><sub><i>r</i></sub>) is the current
 *! raster position, and the pixel was the <i>i</i>th pixel in the <i>j</i
 *! >th row. These pixel fragments are then treated just like the
 *! fragments generated by rasterizing points, lines, or polygons. Texture
 *! mapping, fog, and all the fragment operations are applied before the
 *! fragments are written to the frame buffer. </p></c></r>
 *! <r><c><ref>GL_DEPTH</ref></c>
 *! <c><p>Depth values are read from the depth buffer and converted
 *! directly to an internal floating-point format with unspecified
 *! precision. The resulting floating-point depth value is then multiplied
 *! by <ref>GL_DEPTH_SCALE</ref> and added to <ref>GL_DEPTH_BIAS</ref>.
 *! The result is clamped to the range [0,1]. </p><p>The GL then converts
 *! the resulting depth components to fragments by attaching the current
 *! raster position color or color index and texture coordinates to each
 *! pixel, then assigning window coordinates (<i>x</i><sub><i>r</i></sub
 *! >+<i>i</i>, <i>y</i><sub><i>r</i></sub>+<i>j</i>), where (<i>x</i><sub
 *! ><i>r</i></sub>, <i>y</i><sub><i>r</i></sub>) is the current raster
 *! position, and the pixel was the <i>i</i>th pixel in the <i>j</i>th
 *! row. These pixel fragments are then treated just like the fragments
 *! generated by rasterizing points, lines, or polygons. Texture mapping,
 *! fog, and all the fragment operations are applied before the fragments
 *! are written to the frame buffer. </p></c></r>
 *! <r><c><ref>GL_STENCIL</ref></c>
 *! <c><p>Stencil indices are read from the stencil buffer and converted
 *! to an internal fixed-point format with an unspecified number of bits
 *! to the right of the binary point. Each fixed-point index is then
 *! shifted left by <ref>GL_INDEX_SHIFT</ref> bits, and added to <ref
 *! >GL_INDEX_OFFSET</ref>. If <ref>GL_INDEX_SHIFT</ref> is negative, the
 *! shift is to the right. In either case, zero bits fill otherwise
 *! unspecified bit locations in the result. If <ref>GL_MAP_STENCIL</ref>
 *! is true, the index is replaced with the value that it references in
 *! lookup table <ref>GL_PIXEL_MAP_S_TO_S</ref>. Whether the lookup
 *! replacement of the index is done or not, the integer part of the index
 *! is then ANDed with 2<sup><i>b</i></sup>-1, where <i>b</i> is the
 *! number of bits in the stencil buffer. The resulting stencil indices
 *! are then written to the stencil buffer such that the index read from
 *! the <i>i</i>th location of the <i>j</i>th row is written to location
 *! (<i>x</i><sub><i>r</i></sub>+<i>i</i>, <i>y</i><sub><i>r</i></sub>+<i
 *! >j</i>), where (<i>x</i><sub><i>r</i></sub>, <i>y</i><sub><i>r</i
 *! ></sub>) is the current raster position. Only the pixel ownership
 *! test, the scissor test, and the stencil writemask affect these write
 *! operations. </p></c></r>
 *! </matrix>@}The rasterization described thus far assumes pixel zoom
 *! factors of 1.0. If @[glPixelZoom] is used to change the @i{x@} and
 *! @i{y@} pixel zoom factors, pixels are converted to fragments as
 *! follows. If (@i{x@}@sub{@i{r@}@}, @i{y@}@sub{@i{r@}@}) is the current
 *! raster position, and a given pixel is in the @i{i@}th location in the
 *! @i{j@}th row of the source pixel rectangle, then fragments are
 *! generated for pixels whose centers are in the rectangle with corners
 *! at
 *! 
 *! (@i{x@}@sub{@i{r@}@}+zoom@sub{@i{x@}@}@i{i@},
 *! @i{y@}@sub{@i{r@}@}+zoom@sub{@i{y@}@}@i{j@})
 *! 
 *! and
 *! 
 *! (@i{x@}@sub{@i{r@}@}+zoom@sub{@i{x@}@}(@i{i@}+1),
 *! @i{y@}@sub{@i{r@}@}+zoom@sub{@i{y@}@}(@i{j@}+1))
 *! 
 *! where zoom@sub{@i{x@}@} is the value of @[GL_ZOOM_X] and
 *! zoom@sub{@i{y@}@} is the value of @[GL_ZOOM_Y].
 *! 
 *! @param x
 *! @param y
 *! 
 *! Specify the window coordinates of the lower left corner of the
 *! rectangular region of pixels to be copied.
 *! 
 *! @param width
 *! @param height
 *! 
 *! Specify the dimensions of the rectangular region of pixels to be
 *! copied. Both must be nonnegative.
 *! 
 *! @param type
 *! 
 *! Specifies whether color values, depth values, or stencil values are to
 *! be copied. Symbolic constants @[GL_COLOR], @[GL_DEPTH], and
 *! @[GL_STENCIL] are accepted.
 *! 
 *! @example
 *! 
 *! To copy the color pixel in the lower left corner of the window to the
 *! current raster position, use
 *! 
 *! @code
 *! 
 *! glCopyPixels(0, 0, 1, 1, @[GL_COLOR]);
 *! @endcode
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@}
 *! is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is @[GL_DEPTH] and
 *! there is no depth buffer.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is @[GL_STENCIL] and
 *! there is no stencil buffer.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glCopyPixels] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Modes specified by @[glPixelStore] have no effect on the operation of
 *! @[glCopyPixels].
 *! 
 *! @seealso
 *! 
 *! @[glColorTable], @[glConvolutionFilter1D], @[glConvolutionFilter2D],
 *! @[glDepthFunc], @[glDrawBuffer], @[glDrawPixels], @[glMatrixMode],
 *! @[glPixelMap], @[glPixelTransfer], @[glPixelZoom], @[glRasterPos],
 *! @[glReadBuffer], @[glReadPixels], @[glSeparableFilter2D],
 *! @[glStencilFunc], @[glWindowPos]
 *! 
 */


/*! @decl void glCopyTexImage1D(int target, int level, int internalformat, @
 *! int x, int y, int width, int border)
 *! 
 *! @[glCopyTexImage1D] defines a one-dimensional texture image with
 *! pixels from the current @[GL_READ_BUFFER].
 *! 
 *! The screen-aligned pixel row with left corner at (@i{x@}, @i{y@}) and
 *! with a length of
 *! width+2(border)
 *! defines the texture array at the mipmap level specified by @i{level@}.
 *! @i{internalformat@} specifies the internal format of the texture
 *! array.
 *! 
 *! The pixels in the row are processed exactly as if @[glCopyPixels] had
 *! been called, but the process stops just before final conversion. At
 *! this point all pixel component values are clamped to the range [0, 1]
 *! and then converted to the texture's internal format for storage in the
 *! texel array.
 *! 
 *! Pixel ordering is such that lower @i{x@} screen coordinates correspond
 *! to lower texture coordinates.
 *! 
 *! If any of the pixels within the specified row of the current
 *! @[GL_READ_BUFFER] are outside the window associated with the current
 *! rendering context, then the values obtained for those pixels are
 *! undefined.
 *! 
 *! @[glCopyTexImage1D] defines a one-dimensional texture image with
 *! pixels from the current @[GL_READ_BUFFER].
 *! 
 *! When @i{internalformat@} is one of the sRGB types, the GL does not
 *! automatically convert the source pixels to the sRGB color space. In
 *! this case, the @[glPixelMap] function can be used to accomplish the
 *! conversion.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture. Must be @[GL_TEXTURE_1D].
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number. Level 0 is the base image level.
 *! Level @i{n@} is the @i{n@}th mipmap reduction image.
 *! 
 *! @param internalformat
 *! 
 *! Specifies the internal format of the texture. Must be one of the
 *! following symbolic constants: @[GL_ALPHA], @[GL_ALPHA4], @[GL_ALPHA8],
 *! @[GL_ALPHA12], @[GL_ALPHA16], @[GL_COMPRESSED_ALPHA],
 *! @[GL_COMPRESSED_LUMINANCE], @[GL_COMPRESSED_LUMINANCE_ALPHA],
 *! @[GL_COMPRESSED_INTENSITY], @[GL_COMPRESSED_RGB],
 *! @[GL_COMPRESSED_RGBA], @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], @[GL_DEPTH_COMPONENT32], @[GL_LUMINANCE],
 *! @[GL_LUMINANCE4], @[GL_LUMINANCE8], @[GL_LUMINANCE12],
 *! @[GL_LUMINANCE16], @[GL_LUMINANCE_ALPHA], @[GL_LUMINANCE4_ALPHA4],
 *! @[GL_LUMINANCE6_ALPHA2], @[GL_LUMINANCE8_ALPHA8],
 *! @[GL_LUMINANCE12_ALPHA4], @[GL_LUMINANCE12_ALPHA12],
 *! @[GL_LUMINANCE16_ALPHA16], @[GL_INTENSITY], @[GL_INTENSITY4],
 *! @[GL_INTENSITY8], @[GL_INTENSITY12], @[GL_INTENSITY16], @[GL_RGB],
 *! @[GL_R3_G3_B2], @[GL_RGB4], @[GL_RGB5], @[GL_RGB8], @[GL_RGB10],
 *! @[GL_RGB12], @[GL_RGB16], @[GL_RGBA], @[GL_RGBA2], @[GL_RGBA4],
 *! @[GL_RGB5_A1], @[GL_RGBA8], @[GL_RGB10_A2], @[GL_RGBA12],
 *! @[GL_RGBA16], @[GL_SLUMINANCE], @[GL_SLUMINANCE8],
 *! @[GL_SLUMINANCE_ALPHA], @[GL_SLUMINANCE8_ALPHA8], @[GL_SRGB],
 *! @[GL_SRGB8], @[GL_SRGB_ALPHA], or @[GL_SRGB8_ALPHA8].
 *! 
 *! @param x
 *! @param y
 *! 
 *! Specify the window coordinates of the left corner of the row of pixels
 *! to be copied.
 *! 
 *! @param width
 *! 
 *! Specifies the width of the texture image. Must be 0 or
 *! 2@sup{@i{n@}@}+2(border)
 *! for some integer @i{n@}. The height of the texture image is 1.
 *! 
 *! @param border
 *! 
 *! Specifies the width of the border. Must be either 0 or 1.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is not one of the
 *! allowable values.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if @i{level@} is greater than
 *! log@sub{2@}max
 *! , where max is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{internalformat@} is not an
 *! allowable value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{width@} is less than 0 or
 *! greater than 2 + @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if non-power-of-two textures are not
 *! supported and the @i{width@} cannot be represented as
 *! 2@sup{@i{n@}@}+2(border)
 *! for some integer value of @i{n@}.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{border@} is not 0 or 1.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glCopyTexImage1D] is
 *! executed between the execution of @[glBegin] and the corresponding
 *! execution of @[glEnd].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{internalformat@} is
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], or @[GL_DEPTH_COMPONENT32] and there is no
 *! depth buffer.
 *! 
 *! @note
 *! 
 *! @[glCopyTexImage1D] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Texturing has no effect in color index mode.
 *! 
 *! 1, 2, 3, and 4 are not accepted values for @i{internalformat@}.
 *! 
 *! An image with 0 width indicates a NULL texture.
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, the RGBA components
 *! copied from the framebuffer may be processed by the imaging pipeline.
 *! See @[glTexImage1D] for specific details.
 *! 
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], and @[GL_DEPTH_COMPONENT32] are available
 *! only if the GL version is 1.4 or greater.
 *! 
 *! Non-power-of-two textures are supported if the GL version is 2.0 or
 *! greater, or if the implementation exports the
 *! @[GL_ARB_texture_non_power_of_two] extension.
 *! 
 *! The @[GL_SRGB], @[GL_SRGB8], @[GL_SRGB_ALPHA], @[GL_SRGB8_ALPHA8],
 *! @[GL_SLUMINANCE], @[GL_SLUMINANCE8], @[GL_SLUMINANCE_ALPHA], and
 *! @[GL_SLUMINANCE8_ALPHA8] internal formats are only available if the GL
 *! version is 2.1 or greater. See @[glTexImage1D] for specific details
 *! about sRGB conversion.
 *! 
 *! @seealso
 *! 
 *! @[glCopyPixels], @[glCopyTexImage2D], @[glCopyTexSubImage1D],
 *! @[glCopyTexSubImage2D], @[glPixelStore], @[glPixelTransfer],
 *! @[glTexEnv], @[glTexGen], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexSubImage1D], @[glTexSubImage2D], @[glTexParameter]
 *! 
 */


/*! @decl void glCopyTexImage2D(int target, int level, int internalformat, @
 *! int x, int y, int width, int height, int border)
 *! 
 *! @[glCopyTexImage2D] defines a two-dimensional texture image, or
 *! cube-map texture image with pixels from the current @[GL_READ_BUFFER].
 *! 
 *! The screen-aligned pixel rectangle with lower left corner at (@i{x@},
 *! @i{y@}) and with a width of
 *! width+2(border)
 *! and a height of
 *! height+2(border)
 *! defines the texture array at the mipmap level specified by @i{level@}.
 *! @i{internalformat@} specifies the internal format of the texture
 *! array.
 *! 
 *! The pixels in the rectangle are processed exactly as if
 *! @[glCopyPixels] had been called, but the process stops just before
 *! final conversion. At this point all pixel component values are clamped
 *! to the range [0, 1] and then converted to the texture's internal
 *! format for storage in the texel array.
 *! 
 *! Pixel ordering is such that lower @i{x@} and @i{y@} screen coordinates
 *! correspond to lower @i{s@} and @i{t@} texture coordinates.
 *! 
 *! If any of the pixels within the specified rectangle of the current
 *! @[GL_READ_BUFFER] are outside the window associated with the current
 *! rendering context, then the values obtained for those pixels are
 *! undefined.
 *! 
 *! When @i{internalformat@} is one of the sRGB types, the GL does not
 *! automatically convert the source pixels to the sRGB color space. In
 *! this case, the @[glPixelMap] function can be used to accomplish the
 *! conversion.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture. Must be @[GL_TEXTURE_2D],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_X], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z], or
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z].
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number. Level 0 is the base image level.
 *! Level @i{n@} is the @i{n@}th mipmap reduction image.
 *! 
 *! @param internalformat
 *! 
 *! Specifies the internal format of the texture. Must be one of the
 *! following symbolic constants: @[GL_ALPHA], @[GL_ALPHA4], @[GL_ALPHA8],
 *! @[GL_ALPHA12], @[GL_ALPHA16], @[GL_COMPRESSED_ALPHA],
 *! @[GL_COMPRESSED_LUMINANCE], @[GL_COMPRESSED_LUMINANCE_ALPHA],
 *! @[GL_COMPRESSED_INTENSITY], @[GL_COMPRESSED_RGB],
 *! @[GL_COMPRESSED_RGBA], @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], @[GL_DEPTH_COMPONENT32], @[GL_LUMINANCE],
 *! @[GL_LUMINANCE4], @[GL_LUMINANCE8], @[GL_LUMINANCE12],
 *! @[GL_LUMINANCE16], @[GL_LUMINANCE_ALPHA], @[GL_LUMINANCE4_ALPHA4],
 *! @[GL_LUMINANCE6_ALPHA2], @[GL_LUMINANCE8_ALPHA8],
 *! @[GL_LUMINANCE12_ALPHA4], @[GL_LUMINANCE12_ALPHA12],
 *! @[GL_LUMINANCE16_ALPHA16], @[GL_INTENSITY], @[GL_INTENSITY4],
 *! @[GL_INTENSITY8], @[GL_INTENSITY12], @[GL_INTENSITY16], @[GL_RGB],
 *! @[GL_R3_G3_B2], @[GL_RGB4], @[GL_RGB5], @[GL_RGB8], @[GL_RGB10],
 *! @[GL_RGB12], @[GL_RGB16], @[GL_RGBA], @[GL_RGBA2], @[GL_RGBA4],
 *! @[GL_RGB5_A1], @[GL_RGBA8], @[GL_RGB10_A2], @[GL_RGBA12],
 *! @[GL_RGBA16], @[GL_SLUMINANCE], @[GL_SLUMINANCE8],
 *! @[GL_SLUMINANCE_ALPHA], @[GL_SLUMINANCE8_ALPHA8], @[GL_SRGB],
 *! @[GL_SRGB8], @[GL_SRGB_ALPHA], or @[GL_SRGB8_ALPHA8].
 *! 
 *! @param x
 *! @param y
 *! 
 *! Specify the window coordinates of the lower left corner of the
 *! rectangular region of pixels to be copied.
 *! 
 *! @param width
 *! 
 *! Specifies the width of the texture image. Must be 0 or
 *! 2@sup{@i{n@}@}+2(border)
 *! for some integer @i{n@}.
 *! 
 *! @param height
 *! 
 *! Specifies the height of the texture image. Must be 0 or
 *! 2@sup{@i{m@}@}+2(border)
 *! for some integer @i{m@}.
 *! 
 *! @param border
 *! 
 *! Specifies the width of the border. Must be either 0 or 1.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is not
 *! @[GL_TEXTURE_2D], @[GL_TEXTURE_CUBE_MAP_POSITIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z],
 *! or @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if @i{level@} is greater than
 *! log@sub{2@}max
 *! , where max is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{width@} is less than 0 or
 *! greater than 2 + @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if non-power-of-two textures are not
 *! supported and the @i{width@} or @i{depth@} cannot be represented as
 *! 2@sup{@i{k@}@}+2(border)
 *! for some integer @i{k@}.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{border@} is not 0 or 1.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{internalformat@} is not an
 *! accepted format.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glCopyTexImage2D] is
 *! executed between the execution of @[glBegin] and the corresponding
 *! execution of @[glEnd].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{internalformat@} is
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], or @[GL_DEPTH_COMPONENT32] and there is no
 *! depth buffer.
 *! 
 *! @note
 *! 
 *! @[glCopyTexImage2D] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Texturing has no effect in color index mode.
 *! 
 *! 1, 2, 3, and 4 are not accepted values for @i{internalformat@}.
 *! 
 *! An image with height or width of 0 indicates a NULL texture.
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, the RGBA components
 *! read from the framebuffer may be processed by the imaging pipeline.
 *! See @[glTexImage1D] for specific details.
 *! 
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_X], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z],
 *! or @[GL_PROXY_TEXTURE_CUBE_MAP] are available only if the GL version
 *! is 1.3 or greater.
 *! 
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], and @[GL_DEPTH_COMPONENT32] are available
 *! only if the GL version is 1.4 or greater.
 *! 
 *! The @[GL_SRGB], @[GL_SRGB8], @[GL_SRGB_ALPHA], @[GL_SRGB8_ALPHA8],
 *! @[GL_SLUMINANCE], @[GL_SLUMINANCE8], @[GL_SLUMINANCE_ALPHA], and
 *! @[GL_SLUMINANCE8_ALPHA8] internal formats are only available if the GL
 *! version is 2.1 or greater. See @[glTexImage2D] for specific details
 *! about sRGB conversion.
 *! 
 *! @seealso
 *! 
 *! @[glCopyPixels], @[glCopyTexImage1D], @[glCopyTexSubImage1D],
 *! @[glCopyTexSubImage2D], @[glPixelStore], @[glPixelTransfer],
 *! @[glTexEnv], @[glTexGen], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexSubImage1D], @[glTexSubImage2D], @[glTexParameter]
 *! 
 */


/*! @decl void glCopyTexSubImage1D(int target, int level, int xoffset, @
 *! int x, int y, int width)
 *! 
 *! @[glCopyTexSubImage1D] replaces a portion of a one-dimensional texture
 *! image with pixels from the current @[GL_READ_BUFFER] (rather than from
 *! main memory, as is the case for @[glTexSubImage1D]).
 *! 
 *! The screen-aligned pixel row with left corner at (@i{x@},\ @i{y@}),
 *! and with length @i{width@} replaces the portion of the texture array
 *! with x indices @i{xoffset@} through
 *! xoffset+width-1
 *! , inclusive. The destination in the texture array may not include any
 *! texels outside the texture array as it was originally specified.
 *! 
 *! The pixels in the row are processed exactly as if @[glCopyPixels] had
 *! been called, but the process stops just before final conversion. At
 *! this point, all pixel component values are clamped to the range [0, 1]
 *! and then converted to the texture's internal format for storage in the
 *! texel array.
 *! 
 *! It is not an error to specify a subtexture with zero width, but such a
 *! specification has no effect. If any of the pixels within the specified
 *! row of the current @[GL_READ_BUFFER] are outside the read window
 *! associated with the current rendering context, then the values
 *! obtained for those pixels are undefined.
 *! 
 *! No change is made to the @i{internalformat@}, @i{width@}, or
 *! @i{border@} parameters of the specified texture array or to texel
 *! values outside the specified subregion.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture. Must be @[GL_TEXTURE_1D].
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number. Level 0 is the base image level.
 *! Level @i{n@} is the @i{n@}th mipmap reduction image.
 *! 
 *! @param xoffset
 *! 
 *! Specifies the texel offset within the texture array.
 *! 
 *! @param x
 *! @param y
 *! 
 *! Specify the window coordinates of the left corner of the row of pixels
 *! to be copied.
 *! 
 *! @param width
 *! 
 *! Specifies the width of the texture subimage.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if /@i{target@} is not
 *! @[GL_TEXTURE_1D].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if the texture array has not been
 *! defined by a previous @[glTexImage1D] or @[glCopyTexImage1D]
 *! operation.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if
 *! level>log@sub{2@}(max)
 *! , where @i{max@} is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if
 *! xoffset<-@i{b@}
 *! , or
 *! (xoffset+width)>(@i{w@}-@i{b@})
 *! , where @i{w@} is the @[GL_TEXTURE_WIDTH] and @i{b@} is the
 *! @[GL_TEXTURE_BORDER] of the texture image being modified. Note that
 *! @i{w@} includes twice the border width.
 *! 
 *! @note
 *! 
 *! @[glCopyTexSubImage1D] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Texturing has no effect in color index mode.
 *! 
 *! @[glPixelStore] and @[glPixelTransfer] modes affect texture images in
 *! exactly the way they affect @[glDrawPixels].
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, the RGBA components
 *! copied from the framebuffer may be processed by the imaging pipeline.
 *! See @[glTexImage1D] for specific details.
 *! 
 *! @seealso
 *! 
 *! @[glCopyPixels], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage2D], @[glCopyTexSubImage3D], @[glPixelStore],
 *! @[glPixelTransfer], @[glReadBuffer], @[glTexEnv], @[glTexGen],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexImage3D], @[glTexParameter],
 *! @[glTexSubImage1D], @[glTexSubImage2D], @[glTexSubImage3D]
 *! 
 */


/*! @decl void glCopyTexSubImage2D(int target, int level, int xoffset, @
 *! int yoffset, int x, int y, int width, int height)
 *! 
 *! @[glCopyTexSubImage2D] replaces a rectangular portion of a
 *! two-dimensional texture image or cube-map texture image with pixels
 *! from the current @[GL_READ_BUFFER] (rather than from main memory, as
 *! is the case for @[glTexSubImage2D]).
 *! 
 *! The screen-aligned pixel rectangle with lower left corner at (@i{x@},
 *! @i{y@}) and with width @i{width@} and height @i{height@} replaces the
 *! portion of the texture array with x indices @i{xoffset@} through
 *! xoffset+width-1
 *! , inclusive, and y indices @i{yoffset@} through
 *! yoffset+height-1
 *! , inclusive, at the mipmap level specified by @i{level@}.
 *! 
 *! The pixels in the rectangle are processed exactly as if
 *! @[glCopyPixels] had been called, but the process stops just before
 *! final conversion. At this point, all pixel component values are
 *! clamped to the range [0, 1] and then converted to the texture's
 *! internal format for storage in the texel array.
 *! 
 *! The destination rectangle in the texture array may not include any
 *! texels outside the texture array as it was originally specified. It is
 *! not an error to specify a subtexture with zero width or height, but
 *! such a specification has no effect.
 *! 
 *! If any of the pixels within the specified rectangle of the current
 *! @[GL_READ_BUFFER] are outside the read window associated with the
 *! current rendering context, then the values obtained for those pixels
 *! are undefined.
 *! 
 *! No change is made to the @i{internalformat@}, @i{width@}, @i{height@},
 *! or @i{border@} parameters of the specified texture array or to texel
 *! values outside the specified subregion.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture. Must be @[GL_TEXTURE_2D],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_X], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z], or
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z].
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number. Level 0 is the base image level.
 *! Level @i{n@} is the @i{n@}th mipmap reduction image.
 *! 
 *! @param xoffset
 *! 
 *! Specifies a texel offset in the x direction within the texture array.
 *! 
 *! @param yoffset
 *! 
 *! Specifies a texel offset in the y direction within the texture array.
 *! 
 *! @param x
 *! @param y
 *! 
 *! Specify the window coordinates of the lower left corner of the
 *! rectangular region of pixels to be copied.
 *! 
 *! @param width
 *! 
 *! Specifies the width of the texture subimage.
 *! 
 *! @param height
 *! 
 *! Specifies the height of the texture subimage.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is not
 *! @[GL_TEXTURE_2D], @[GL_TEXTURE_CUBE_MAP_POSITIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z],
 *! or @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if the texture array has not been
 *! defined by a previous @[glTexImage2D] or @[glCopyTexImage2D]
 *! operation.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if
 *! level>log@sub{2@}(max)
 *! , where max is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if
 *! xoffset<-@i{b@}
 *! ,
 *! (xoffset+width)>(@i{w@}-@i{b@})
 *! ,
 *! yoffset<-@i{b@}
 *! , or
 *! (yoffset+height)>(@i{h@}-@i{b@})
 *! , where @i{w@} is the @[GL_TEXTURE_WIDTH], @i{h@} is the
 *! @[GL_TEXTURE_HEIGHT], and @i{b@} is the @[GL_TEXTURE_BORDER] of the
 *! texture image being modified. Note that @i{w@} and @i{h@} include
 *! twice the border width.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glCopyTexSubImage2D] is
 *! executed between the execution of @[glBegin] and the corresponding
 *! execution of @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glCopyTexSubImage2D] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_X], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z],
 *! or @[GL_PROXY_TEXTURE_CUBE_MAP] are available only if the GL version
 *! is 1.3 or greater.
 *! 
 *! Texturing has no effect in color index mode.
 *! 
 *! @[glPixelStore] and @[glPixelTransfer] modes affect texture images in
 *! exactly the way they affect @[glDrawPixels].
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, the RGBA components
 *! read from the framebuffer may be processed by the imaging pipeline.
 *! See @[glTexImage1D] for specific details.
 *! 
 *! @seealso
 *! 
 *! @[glCopyPixels], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage3D], @[glPixelStore],
 *! @[glPixelTransfer], @[glReadBuffer], @[glTexEnv], @[glTexGen],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexImage3D], @[glTexParameter],
 *! @[glTexSubImage1D], @[glTexSubImage2D], @[glTexSubImage3D]
 *! 
 */


/*! @decl void glDeleteLists(int list, int range)
 *! 
 *! @[glDeleteLists] causes a contiguous group of display lists to be
 *! deleted. @i{list@} is the name of the first display list to be
 *! deleted, and @i{range@} is the number of display lists to delete. All
 *! display lists @i{d@} with
 *! list<=@i{d@}<=list+range-1
 *! are deleted.
 *! 
 *! All storage locations allocated to the specified display lists are
 *! freed, and the names are available for reuse at a later time. Names
 *! within the range that do not have an associated display list are
 *! ignored. If @i{range@} is 0, nothing happens.
 *! 
 *! @param list
 *! 
 *! Specifies the integer name of the first display list to delete.
 *! 
 *! @param range
 *! 
 *! Specifies the number of display lists to delete.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{range@} is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDeleteLists] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glCallList], @[glCallLists], @[glGenLists], @[glIsList],
 *! @[glNewList]
 *! 
 */


/*! @decl void glDepthMask(int flag)
 *! 
 *! @[glDepthMask] specifies whether the depth buffer is enabled for
 *! writing. If @i{flag@} is @[GL_FALSE], depth buffer writing is
 *! disabled. Otherwise, it is enabled. Initially, depth buffer writing is
 *! enabled.
 *! 
 *! @param flag
 *! 
 *! Specifies whether the depth buffer is enabled for writing. If
 *! @i{flag@} is @[GL_FALSE], depth buffer writing is disabled. Otherwise,
 *! it is enabled. Initially, depth buffer writing is enabled.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDepthMask] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glColorMask], @[glDepthFunc], @[glDepthRange], @[glIndexMask],
 *! @[glStencilMask]
 *! 
 */


/*! @decl void glDepthRange(float nearVal, float farVal)
 *! 
 *! After clipping and division by @i{w@}, depth coordinates range from -1
 *! to 1, corresponding to the near and far clipping planes.
 *! @[glDepthRange] specifies a linear mapping of the normalized depth
 *! coordinates in this range to window depth coordinates. Regardless of
 *! the actual depth buffer implementation, window coordinate depth values
 *! are treated as though they range from 0 through 1 (like color
 *! components). Thus, the values accepted by @[glDepthRange] are both
 *! clamped to this range before they are accepted.
 *! 
 *! The setting of (0,1) maps the near plane to 0 and the far plane to 1.
 *! With this mapping, the depth buffer range is fully utilized.
 *! 
 *! @param nearVal
 *! 
 *! Specifies the mapping of the near clipping plane to window
 *! coordinates. The initial value is 0.
 *! 
 *! @param farVal
 *! 
 *! Specifies the mapping of the far clipping plane to window coordinates.
 *! The initial value is 1.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDepthRange] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! It is not necessary that @i{nearVal@} be less than @i{farVal@}.
 *! Reverse mappings such as
 *! nearVal=1
 *! , and
 *! farVal=0
 *! are acceptable.
 *! 
 *! @seealso
 *! 
 *! @[glDepthFunc], @[glPolygonOffset], @[glViewport]
 *! 
 */


/*! @decl void glDrawArrays(int mode, int first, int count)
 *! 
 *! @[glDrawArrays] specifies multiple geometric primitives with very few
 *! subroutine calls. Instead of calling a GL procedure to pass each
 *! individual vertex, normal, texture coordinate, edge flag, or color,
 *! you can prespecify separate arrays of vertices, normals, and colors
 *! and use them to construct a sequence of primitives with a single call
 *! to @[glDrawArrays].
 *! 
 *! When @[glDrawArrays] is called, it uses @i{count@} sequential elements
 *! from each enabled array to construct a sequence of geometric
 *! primitives, beginning with element @i{first@}. @i{mode@} specifies
 *! what kind of primitives are constructed and how the array elements
 *! construct those primitives. If @[GL_VERTEX_ARRAY] is not enabled, no
 *! geometric primitives are generated.
 *! 
 *! Vertex attributes that are modified by @[glDrawArrays] have an
 *! unspecified value after @[glDrawArrays] returns. For example, if
 *! @[GL_COLOR_ARRAY] is enabled, the value of the current color is
 *! undefined after @[glDrawArrays] executes. Attributes that aren't
 *! modified remain well defined.
 *! 
 *! @param mode
 *! 
 *! Specifies what kind of primitives to render. Symbolic constants
 *! @[GL_POINTS], @[GL_LINE_STRIP], @[GL_LINE_LOOP], @[GL_LINES],
 *! @[GL_TRIANGLE_STRIP], @[GL_TRIANGLE_FAN], @[GL_TRIANGLES],
 *! @[GL_QUAD_STRIP], @[GL_QUADS], and @[GL_POLYGON] are accepted.
 *! 
 *! @param first
 *! 
 *! Specifies the starting index in the enabled arrays.
 *! 
 *! @param count
 *! 
 *! Specifies the number of indices to be rendered.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{count@} is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to an enabled array and the buffer object's data store is
 *! currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDrawArrays] is executed
 *! between the execution of @[glBegin] and the corresponding @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glDrawArrays] is available only if the GL version is 1.1 or greater.
 *! 
 *! @[glDrawArrays] is included in display lists. If @[glDrawArrays] is
 *! entered into a display list, the necessary array data (determined by
 *! the array pointers and enables) is also entered into the display list.
 *! Because the array pointers and enables are client-side state, their
 *! values affect display lists when the lists are created, not when the
 *! lists are executed.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glColorPointer], @[glDrawElements],
 *! @[glDrawRangeElements], @[glEdgeFlagPointer], @[glFogCoordPointer],
 *! @[glGetPointerv], @[glIndexPointer], @[glInterleavedArrays],
 *! @[glNormalPointer], @[glSecondaryColorPointer], @[glTexCoordPointer],
 *! @[glVertexPointer]
 *! 
 */


/*! @decl void glDrawPixels(object|mapping(string:object) width, @
 *! object|mapping(string:object) height, @
 *! object|mapping(string:object) format, @
 *! object|mapping(string:object) type, @
 *! object|mapping(string:object) data)
 *! 
 *! @[glDrawPixels] reads pixel data from memory and writes it into the
 *! frame buffer relative to the current raster position, provided that
 *! the raster position is valid. Use @[glRasterPos] or @[glWindowPos] to
 *! set the current raster position; use @[glGet] with argument
 *! @[GL_CURRENT_RASTER_POSITION_VALID] to determine if the specified
 *! raster position is valid, and @[glGet] with argument
 *! @[GL_CURRENT_RASTER_POSITION] to query the raster position.
 *! 
 *! Several parameters define the encoding of pixel data in memory and
 *! control the processing of the pixel data before it is placed in the
 *! frame buffer. These parameters are set with four commands:
 *! @[glPixelStore], @[glPixelTransfer], @[glPixelMap], and
 *! @[glPixelZoom]. This reference page describes the effects on
 *! @[glDrawPixels] of many, but not all, of the parameters specified by
 *! these four commands.
 *! 
 *! Data is read from @i{data@} as a sequence of signed or unsigned bytes,
 *! signed or unsigned shorts, signed or unsigned integers, or
 *! single-precision floating-point values, depending on @i{type@}. When
 *! @i{type@} is one of @[GL_UNSIGNED_BYTE], @[GL_BYTE],
 *! @[GL_UNSIGNED_SHORT], @[GL_SHORT], @[GL_UNSIGNED_INT], @[GL_INT], or
 *! @[GL_FLOAT] each of these bytes, shorts, integers, or floating-point
 *! values is interpreted as one color or depth component, or one index,
 *! depending on @i{format@}. When @i{type@} is one of
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_SHORT_5_6_5],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_5_5_5_1],
 *! @[GL_UNSIGNED_INT_8_8_8_8], or @[GL_UNSIGNED_INT_10_10_10_2], each
 *! unsigned value is interpreted as containing all the components for a
 *! single pixel, with the color components arranged according to
 *! @i{format@}. When @i{type@} is one of @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5_REV], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_1_5_5_5_REV], @[GL_UNSIGNED_INT_8_8_8_8_REV], or
 *! @[GL_UNSIGNED_INT_2_10_10_10_REV], each unsigned value is interpreted
 *! as containing all color components, specified by @i{format@}, for a
 *! single pixel in a reversed order. Indices are always treated
 *! individually. Color components are treated as groups of one, two,
 *! three, or four values, again based on @i{format@}. Both individual
 *! indices and groups of components are referred to as pixels. If
 *! @i{type@} is @[GL_BITMAP], the data must be unsigned bytes, and
 *! @i{format@} must be either @[GL_COLOR_INDEX] or @[GL_STENCIL_INDEX].
 *! Each unsigned byte is treated as eight 1-bit pixels, with bit ordering
 *! determined by @[GL_UNPACK_LSB_FIRST] (see @[glPixelStore]).
 *! 
 *! width@xml{&#215;@}height
 *! pixels are read from memory, starting at location @i{data@}. By
 *! default, these pixels are taken from adjacent memory locations, except
 *! that after all @i{width@} pixels are read, the read pointer is
 *! advanced to the next four-byte boundary. The four-byte row alignment
 *! is specified by @[glPixelStore] with argument @[GL_UNPACK_ALIGNMENT],
 *! and it can be set to one, two, four, or eight bytes. Other pixel store
 *! parameters specify different read pointer advancements, both before
 *! the first pixel is read and after all @i{width@} pixels are read. See
 *! the @[glPixelStore] reference page for details on these options.
 *! 
 *! If a non-zero named buffer object is bound to the
 *! @[GL_PIXEL_UNPACK_BUFFER] target (see @[glBindBuffer]) while a block
 *! of pixels is specified, @i{data@} is treated as a byte offset into the
 *! buffer object's data store.
 *! 
 *! The
 *! width@xml{&#215;@}height
 *! pixels that are read from memory are each operated on in the same way,
 *! based on the values of several parameters specified by
 *! @[glPixelTransfer] and @[glPixelMap]. The details of these operations,
 *! as well as the target buffer into which the pixels are drawn, are
 *! specific to the format of the pixels, as specified by @i{format@}.
 *! @i{format@} can assume one of 13 symbolic values:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_COLOR_INDEX</ref></c>
 *! <c><p>Each pixel is a single value, a color index. It is converted to
 *! fixed-point format, with an unspecified number of bits to the right of
 *! the binary point, regardless of the memory data type. Floating-point
 *! values convert to true fixed-point values. Signed and unsigned integer
 *! data is converted with all fraction bits set to 0. Bitmap data convert
 *! to either 0 or 1. </p><p>Each fixed-point index is then shifted left
 *! by <ref>GL_INDEX_SHIFT</ref> bits and added to <ref
 *! >GL_INDEX_OFFSET</ref>. If <ref>GL_INDEX_SHIFT</ref> is negative, the
 *! shift is to the right. In either case, zero bits fill otherwise
 *! unspecified bit locations in the result. </p><p>If the GL is in RGBA
 *! mode, the resulting index is converted to an RGBA pixel with the help
 *! of the <ref>GL_PIXEL_MAP_I_TO_R</ref>, <ref>GL_PIXEL_MAP_I_TO_G</ref>,
 *! <ref>GL_PIXEL_MAP_I_TO_B</ref>, and <ref>GL_PIXEL_MAP_I_TO_A</ref>
 *! tables. If the GL is in color index mode, and if <ref
 *! >GL_MAP_COLOR</ref> is true, the index is replaced with the value that
 *! it references in lookup table <ref>GL_PIXEL_MAP_I_TO_I</ref>. Whether
 *! the lookup replacement of the index is done or not, the integer part
 *! of the index is then ANDed with 2<sup><i>b</i></sup>-1, where <i>b</i>
 *! is the number of bits in a color index buffer. </p><p>The GL then
 *! converts the resulting indices or RGBA colors to fragments by
 *! attaching the current raster position <i>z</i> coordinate and texture
 *! coordinates to each pixel, then assigning <i>x</i> and <i>y</i> window
 *! coordinates to the <i>n</i>th fragment such that <i>x</i><sub><i>n</i
 *! ></sub>=<i>x</i><sub><i>r</i></sub>+<i>n</i>%width<p> <i>y</i><sub><i
 *! >n</i></sub>=<i>y</i><sub><i>r</i></sub>+&#8970;<i>n</i
 *! >/width&#8971;</p><p></p></p><p>where (<i>x</i><sub><i>r</i></sub>, <i
 *! >y</i><sub><i>r</i></sub>) is the current raster position. These pixel
 *! fragments are then treated just like the fragments generated by
 *! rasterizing points, lines, or polygons. Texture mapping, fog, and all
 *! the fragment operations are applied before the fragments are written
 *! to the frame buffer. </p></c></r>
 *! <r><c><ref>GL_STENCIL_INDEX</ref></c>
 *! <c><p>Each pixel is a single value, a stencil index. It is converted
 *! to fixed-point format, with an unspecified number of bits to the right
 *! of the binary point, regardless of the memory data type.
 *! Floating-point values convert to true fixed-point values. Signed and
 *! unsigned integer data is converted with all fraction bits set to 0.
 *! Bitmap data convert to either 0 or 1. </p><p>Each fixed-point index is
 *! then shifted left by <ref>GL_INDEX_SHIFT</ref> bits, and added to <ref
 *! >GL_INDEX_OFFSET</ref>. If <ref>GL_INDEX_SHIFT</ref> is negative, the
 *! shift is to the right. In either case, zero bits fill otherwise
 *! unspecified bit locations in the result. If <ref>GL_MAP_STENCIL</ref>
 *! is true, the index is replaced with the value that it references in
 *! lookup table <ref>GL_PIXEL_MAP_S_TO_S</ref>. Whether the lookup
 *! replacement of the index is done or not, the integer part of the index
 *! is then ANDed with 2<sup><i>b</i></sup>-1, where <i>b</i> is the
 *! number of bits in the stencil buffer. The resulting stencil indices
 *! are then written to the stencil buffer such that the <i>n</i>th index
 *! is written to location </p><p><i>x</i><sub><i>n</i></sub>=<i>x</i><sub
 *! ><i>r</i></sub>+<i>n</i>%width<p> <i>y</i><sub><i>n</i></sub>=<i>y</i
 *! ><sub><i>r</i></sub>+&#8970;<i>n</i>/width&#8971;</p></p><p>where (<i
 *! >x</i><sub><i>r</i></sub>, <i>y</i><sub><i>r</i></sub>) is the current
 *! raster position. Only the pixel ownership test, the scissor test, and
 *! the stencil writemask affect these write operations. </p></c></r>
 *! <r><c><ref>GL_DEPTH_COMPONENT</ref></c>
 *! <c><p>Each pixel is a single-depth component. Floating-point data is
 *! converted directly to an internal floating-point format with
 *! unspecified precision. Signed integer data is mapped linearly to the
 *! internal floating-point format such that the most positive
 *! representable integer value maps to 1.0, and the most negative
 *! representable value maps to -1.0. Unsigned integer data is mapped
 *! similarly: the largest integer value maps to 1.0, and 0 maps to 0.0.
 *! The resulting floating-point depth value is then multiplied by <ref
 *! >GL_DEPTH_SCALE</ref> and added to <ref>GL_DEPTH_BIAS</ref>. The
 *! result is clamped to the range [0, 1]. </p><p>The GL then converts the
 *! resulting depth components to fragments by attaching the current
 *! raster position color or color index and texture coordinates to each
 *! pixel, then assigning <i>x</i> and <i>y</i> window coordinates to the
 *! <i>n</i>th fragment such that </p><p><i>x</i><sub><i>n</i></sub>=<i
 *! >x</i><sub><i>r</i></sub>+<i>n</i>%width<p> <i>y</i><sub><i>n</i></sub
 *! >=<i>y</i><sub><i>r</i></sub>+&#8970;<i>n</i>/width&#8971;</p></p><p
 *! >where (<i>x</i><sub><i>r</i></sub>, <i>y</i><sub><i>r</i></sub>) is
 *! the current raster position. These pixel fragments are then treated
 *! just like the fragments generated by rasterizing points, lines, or
 *! polygons. Texture mapping, fog, and all the fragment operations are
 *! applied before the fragments are written to the frame buffer. </p
 *! ></c></r>
 *! <r><c><ref>GL_RGBA</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BGRA</ref></c>
 *! <c><p>Each pixel is a four-component group: For <ref>GL_RGBA</ref>,
 *! the red component is first, followed by green, followed by blue,
 *! followed by alpha; for <ref>GL_BGRA</ref> the order is blue, green,
 *! red and then alpha. Floating-point values are converted directly to an
 *! internal floating-point format with unspecified precision. Signed
 *! integer values are mapped linearly to the internal floating-point
 *! format such that the most positive representable integer value maps to
 *! 1.0, and the most negative representable value maps to -1.0. (Note
 *! that this mapping does not convert 0 precisely to 0.0.) Unsigned
 *! integer data is mapped similarly: The largest integer value maps to
 *! 1.0, and 0 maps to 0.0. The resulting floating-point color values are
 *! then multiplied by <tt>GL_<i>c</i>_SCALE</tt> and added to <tt>GL_<i
 *! >c</i>_BIAS</tt>, where <i>c</i> is RED, GREEN, BLUE, and ALPHA for
 *! the respective color components. The results are clamped to the range
 *! [0, 1]. </p><p>If <ref>GL_MAP_COLOR</ref> is true, each color
 *! component is scaled by the size of lookup table <tt>GL_PIXEL_MAP_<i
 *! >c</i>_TO_<i>c</i></tt>, then replaced by the value that it references
 *! in that table. <i>c</i> is R, G, B, or A respectively. </p><p>The GL
 *! then converts the resulting RGBA colors to fragments by attaching the
 *! current raster position <i>z</i> coordinate and texture coordinates to
 *! each pixel, then assigning <i>x</i> and <i>y</i> window coordinates to
 *! the <i>n</i>th fragment such that </p><p><i>x</i><sub><i>n</i></sub
 *! >=<i>x</i><sub><i>r</i></sub>+<i>n</i>%width<p> <i>y</i><sub><i>n</i
 *! ></sub>=<i>y</i><sub><i>r</i></sub>+&#8970;<i>n</i>/width&#8971;</p
 *! ></p><p>where (<i>x</i><sub><i>r</i></sub>, <i>y</i><sub><i>r</i></sub
 *! >) is the current raster position. These pixel fragments are then
 *! treated just like the fragments generated by rasterizing points,
 *! lines, or polygons. Texture mapping, fog, and all the fragment
 *! operations are applied before the fragments are written to the frame
 *! buffer. </p></c></r>
 *! <r><c><ref>GL_RED</ref></c>
 *! <c><p>Each pixel is a single red component. This component is
 *! converted to the internal floating-point format in the same way the
 *! red component of an RGBA pixel is. It is then converted to an RGBA
 *! pixel with green and blue set to 0, and alpha set to 1. After this
 *! conversion, the pixel is treated as if it had been read as an RGBA
 *! pixel. </p></c></r>
 *! <r><c><ref>GL_GREEN</ref></c>
 *! <c><p>Each pixel is a single green component. This component is
 *! converted to the internal floating-point format in the same way the
 *! green component of an RGBA pixel is. It is then converted to an RGBA
 *! pixel with red and blue set to 0, and alpha set to 1. After this
 *! conversion, the pixel is treated as if it had been read as an RGBA
 *! pixel. </p></c></r>
 *! <r><c><ref>GL_BLUE</ref></c>
 *! <c><p>Each pixel is a single blue component. This component is
 *! converted to the internal floating-point format in the same way the
 *! blue component of an RGBA pixel is. It is then converted to an RGBA
 *! pixel with red and green set to 0, and alpha set to 1. After this
 *! conversion, the pixel is treated as if it had been read as an RGBA
 *! pixel. </p></c></r>
 *! <r><c><ref>GL_ALPHA</ref></c>
 *! <c><p>Each pixel is a single alpha component. This component is
 *! converted to the internal floating-point format in the same way the
 *! alpha component of an RGBA pixel is. It is then converted to an RGBA
 *! pixel with red, green, and blue set to 0. After this conversion, the
 *! pixel is treated as if it had been read as an RGBA pixel. </p></c></r>
 *! <r><c><ref>GL_RGB</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BGR</ref></c>
 *! <c><p>Each pixel is a three-component group: red first, followed by
 *! green, followed by blue; for <ref>GL_BGR</ref>, the first component is
 *! blue, followed by green and then red. Each component is converted to
 *! the internal floating-point format in the same way the red, green, and
 *! blue components of an RGBA pixel are. The color triple is converted to
 *! an RGBA pixel with alpha set to 1. After this conversion, the pixel is
 *! treated as if it had been read as an RGBA pixel. </p></c></r>
 *! <r><c><ref>GL_LUMINANCE</ref></c>
 *! <c><p>Each pixel is a single luminance component. This component is
 *! converted to the internal floating-point format in the same way the
 *! red component of an RGBA pixel is. It is then converted to an RGBA
 *! pixel with red, green, and blue set to the converted luminance value,
 *! and alpha set to 1. After this conversion, the pixel is treated as if
 *! it had been read as an RGBA pixel. </p></c></r>
 *! <r><c><ref>GL_LUMINANCE_ALPHA</ref></c>
 *! <c><p>Each pixel is a two-component group: luminance first, followed
 *! by alpha. The two components are converted to the internal
 *! floating-point format in the same way the red component of an RGBA
 *! pixel is. They are then converted to an RGBA pixel with red, green,
 *! and blue set to the converted luminance value, and alpha set to the
 *! converted alpha value. After this conversion, the pixel is treated as
 *! if it had been read as an RGBA pixel. </p></c></r>
 *! </matrix>@}The following table summarizes the meaning of the valid
 *! constants for the @i{type@} parameter:
 *! 
 *! @xml{<matrix><r><c><b>Type </b></c><c><b>Corresponding Type </b></c
 *! ></r><r><c><ref>GL_UNSIGNED_BYTE</ref></c><c> unsigned 8-bit integer
 *! </c></r><r><c><ref>GL_BYTE</ref></c><c> signed 8-bit integer </c></r
 *! ><r><c><ref>GL_BITMAP</ref></c><c> single bits in unsigned 8-bit
 *! integers </c></r><r><c><ref>GL_UNSIGNED_SHORT</ref></c><c> unsigned
 *! 16-bit integer </c></r><r><c><ref>GL_SHORT</ref></c><c> signed 16-bit
 *! integer </c></r><r><c><ref>GL_UNSIGNED_INT</ref></c><c> unsigned
 *! 32-bit integer </c></r><r><c><ref>GL_INT</ref></c><c> 32-bit integer
 *! </c></r><r><c><ref>GL_FLOAT</ref></c><c> single-precision
 *! floating-point </c></r><r><c><ref>GL_UNSIGNED_BYTE_3_3_2</ref></c><c>
 *! unsigned 8-bit integer </c></r><r><c><ref
 *! >GL_UNSIGNED_BYTE_2_3_3_REV</ref></c><c> unsigned 8-bit integer with
 *! reversed component ordering </c></r><r><c><ref
 *! >GL_UNSIGNED_SHORT_5_6_5</ref></c><c> unsigned 16-bit integer </c></r
 *! ><r><c><ref>GL_UNSIGNED_SHORT_5_6_5_REV</ref></c><c> unsigned 16-bit
 *! integer with reversed component ordering </c></r><r><c><ref
 *! >GL_UNSIGNED_SHORT_4_4_4_4</ref></c><c> unsigned 16-bit integer </c
 *! ></r><r><c><ref>GL_UNSIGNED_SHORT_4_4_4_4_REV</ref></c><c> unsigned
 *! 16-bit integer with reversed component ordering </c></r><r><c><ref
 *! >GL_UNSIGNED_SHORT_5_5_5_1</ref></c><c> unsigned 16-bit integer </c
 *! ></r><r><c><ref>GL_UNSIGNED_SHORT_1_5_5_5_REV</ref></c><c> unsigned
 *! 16-bit integer with reversed component ordering </c></r><r><c><ref
 *! >GL_UNSIGNED_INT_8_8_8_8</ref></c><c> unsigned 32-bit integer </c></r
 *! ><r><c><ref>GL_UNSIGNED_INT_8_8_8_8_REV</ref></c><c> unsigned 32-bit
 *! integer with reversed component ordering </c></r><r><c><ref
 *! >GL_UNSIGNED_INT_10_10_10_2</ref></c><c> unsigned 32-bit integer </c
 *! ></r><r><c><ref>GL_UNSIGNED_INT_2_10_10_10_REV</ref></c><c> unsigned
 *! 32-bit integer with reversed component ordering </c></r></matrix>@}
 *! 
 *! The rasterization described so far assumes pixel zoom factors of 1. If
 *! @[glPixelZoom] is used to change the @i{x@} and @i{y@} pixel zoom
 *! factors, pixels are converted to fragments as follows. If
 *! (@i{x@}@sub{@i{r@}@}, @i{y@}@sub{@i{r@}@}) is the current raster
 *! position, and a given pixel is in the @i{n@}th column and @i{m@}th row
 *! of the pixel rectangle, then fragments are generated for pixels whose
 *! centers are in the rectangle with corners at
 *! 
 *! (@i{x@}@sub{@i{r@}@}+zoom@sub{@i{x@}@}@i{n@},
 *! @i{y@}@sub{@i{r@}@}+zoom@sub{@i{y@}@}@i{m@})
 *! (@i{x@}@sub{@i{r@}@}+zoom@sub{@i{x@}@}(@i{n@}+1),
 *! @i{y@}@sub{@i{r@}@}+zoom@sub{@i{y@}@}(@i{m@}+1))
 *! 
 *! where zoom@sub{@i{x@}@} is the value of @[GL_ZOOM_X] and
 *! zoom@sub{@i{y@}@} is the value of @[GL_ZOOM_Y].
 *! 
 *! @param width
 *! @param height
 *! 
 *! Specify the dimensions of the pixel rectangle to be written into the
 *! frame buffer.
 *! 
 *! @param format
 *! 
 *! Specifies the format of the pixel data. Symbolic constants
 *! @[GL_COLOR_INDEX], @[GL_STENCIL_INDEX], @[GL_DEPTH_COMPONENT],
 *! @[GL_RGB], @[GL_BGR], @[GL_RGBA], @[GL_BGRA], @[GL_RED], @[GL_GREEN],
 *! @[GL_BLUE], @[GL_ALPHA], @[GL_LUMINANCE], and @[GL_LUMINANCE_ALPHA]
 *! are accepted.
 *! 
 *! @param type
 *! 
 *! Specifies the data type for @i{data@}. Symbolic constants
 *! @[GL_UNSIGNED_BYTE], @[GL_BYTE], @[GL_BITMAP], @[GL_UNSIGNED_SHORT],
 *! @[GL_SHORT], @[GL_UNSIGNED_INT], @[GL_INT], @[GL_FLOAT],
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], and @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! are accepted.
 *! 
 *! @param data
 *! 
 *! Specifies a pointer to the pixel data.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{format@} or @i{type@} is not one
 *! of the accepted values.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *! @i{format@} is not either @[GL_COLOR_INDEX] or @[GL_STENCIL_INDEX].
 *! 
 *! @[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@}
 *! is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is
 *! @[GL_STENCIL_INDEX] and there is no stencil buffer.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is @[GL_RED],
 *! @[GL_GREEN], @[GL_BLUE], @[GL_ALPHA], @[GL_RGB], @[GL_RGBA],
 *! @[GL_BGR], @[GL_BGRA], @[GL_LUMINANCE], or @[GL_LUMINANCE_ALPHA], and
 *! the GL is in color index mode.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is one of
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], or @[GL_UNSIGNED_SHORT_5_6_5_REV] and
 *! @i{format@} is not @[GL_RGB].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is one of
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], or @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! and @i{format@} is neither @[GL_RGBA] nor @[GL_BGRA].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the buffer
 *! object's data store is currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the data would be
 *! unpacked from the buffer object such that the memory reads required
 *! would exceed the data store size.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and @i{data@} is not
 *! evenly divisible into the number of bytes needed to store in memory a
 *! datum indicated by @i{type@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDrawPixels] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[GL_BGR] and @[GL_BGRA] are only valid for @i{format@} if the GL
 *! version is 1.2 or greater.
 *! 
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], and @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! are only valid for @i{type@} if the GL version is 1.2 or greater.
 *! 
 *! @seealso
 *! 
 *! @[glAlphaFunc], @[glBlendFunc], @[glCopyPixels], @[glDepthFunc],
 *! @[glLogicOp], @[glPixelMap], @[glPixelStore], @[glPixelTransfer],
 *! @[glPixelZoom], @[glRasterPos], @[glReadPixels], @[glScissor],
 *! @[glStencilFunc], @[glWindowPos]
 *! 
 */


/*! @decl void glEdgeFlag(int flag)
 *! 
 *! Each vertex of a polygon, separate triangle, or separate quadrilateral
 *! specified between a @[glBegin]/@[glEnd] pair is marked as the start of
 *! either a boundary or nonboundary edge. If the current edge flag is
 *! true when the vertex is specified, the vertex is marked as the start
 *! of a boundary edge. Otherwise, the vertex is marked as the start of a
 *! nonboundary edge. @[glEdgeFlag] sets the edge flag bit to @[GL_TRUE]
 *! if @i{flag@} is @[GL_TRUE] and to @[GL_FALSE] otherwise.
 *! 
 *! The vertices of connected triangles and connected quadrilaterals are
 *! always marked as boundary, regardless of the value of the edge flag.
 *! 
 *! Boundary and nonboundary edge flags on vertices are significant only
 *! if @[GL_POLYGON_MODE] is set to @[GL_POINT] or @[GL_LINE]. See
 *! @[glPolygonMode].
 *! 
 *! @param flag
 *! 
 *! Specifies the current edge flag value, either @[GL_TRUE] or
 *! @[GL_FALSE]. The initial value is @[GL_TRUE].
 *! 
 *! @param flag
 *! 
 *! Specifies an array that contains a single boolean element, which
 *! replaces the current edge flag value.
 *! 
 *! @note
 *! 
 *! The current edge flag can be updated at any time. In particular,
 *! @[glEdgeFlag] can be called between a call to @[glBegin] and the
 *! corresponding call to @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glBegin], @[glEdgeFlagPointer], @[glPolygonMode]
 *! 
 */


/*! @decl void glEvalCoord(float|int u, float|int|void v)
 *! @decl void glEvalCoord(array(float|int) u)
 *! 
 *! @[glEvalCoord] evaluates enabled one-dimensional maps at argument
 *! @i{u@}. @[glEvalCoord] does the same for two-dimensional maps using
 *! two domain values, @i{u@} and @i{v@}. To define a map, call @[glMap1]
 *! and @[glMap2]; to enable and disable it, call @[glEnable] and
 *! @[glDisable].
 *! 
 *! When one of the @[glEvalCoord] commands is issued, all currently
 *! enabled maps of the indicated dimension are evaluated. Then, for each
 *! enabled map, it is as if the corresponding GL command had been issued
 *! with the computed value. That is, if @[GL_MAP1_INDEX] or
 *! @[GL_MAP2_INDEX] is enabled, a @[glIndex] command is simulated. If
 *! @[GL_MAP1_COLOR_4] or @[GL_MAP2_COLOR_4] is enabled, a @[glColor]
 *! command is simulated. If @[GL_MAP1_NORMAL] or @[GL_MAP2_NORMAL] is
 *! enabled, a normal vector is produced, and if any of
 *! @[GL_MAP1_TEXTURE_COORD_1], @[GL_MAP1_TEXTURE_COORD_2],
 *! @[GL_MAP1_TEXTURE_COORD_3], @[GL_MAP1_TEXTURE_COORD_4],
 *! @[GL_MAP2_TEXTURE_COORD_1], @[GL_MAP2_TEXTURE_COORD_2],
 *! @[GL_MAP2_TEXTURE_COORD_3], or @[GL_MAP2_TEXTURE_COORD_4] is enabled,
 *! then an appropriate @[glTexCoord] command is simulated.
 *! 
 *! For color, color index, normal, and texture coordinates the GL uses
 *! evaluated values instead of current values for those evaluations that
 *! are enabled, and current values otherwise, However, the evaluated
 *! values do not update the current values. Thus, if @[glVertex] commands
 *! are interspersed with @[glEvalCoord] commands, the color, normal, and
 *! texture coordinates associated with the @[glVertex] commands are not
 *! affected by the values generated by the @[glEvalCoord] commands, but
 *! only by the most recent @[glColor], @[glIndex], @[glNormal], and
 *! @[glTexCoord] commands.
 *! 
 *! No commands are issued for maps that are not enabled. If more than one
 *! texture evaluation is enabled for a particular dimension (for example,
 *! @[GL_MAP2_TEXTURE_COORD_1] and @[GL_MAP2_TEXTURE_COORD_2]), then only
 *! the evaluation of the map that produces the larger number of
 *! coordinates (in this case, @[GL_MAP2_TEXTURE_COORD_2]) is carried out.
 *! @[GL_MAP1_VERTEX_4] overrides @[GL_MAP1_VERTEX_3], and
 *! @[GL_MAP2_VERTEX_4] overrides @[GL_MAP2_VERTEX_3], in the same manner.
 *! If neither a three- nor a four-component vertex map is enabled for the
 *! specified dimension, the @[glEvalCoord] command is ignored.
 *! 
 *! If you have enabled automatic normal generation, by calling
 *! @[glEnable] with argument @[GL_AUTO_NORMAL], @[glEvalCoord] generates
 *! surface normals analytically, regardless of the contents or enabling
 *! of the @[GL_MAP2_NORMAL] map. Let
 *! 
 *! @i{m@}=@xml{&#8706;@}@i{p@}/@xml{&#8706;@}@i{u@}@xml{&#215;@}@xml{&#8706;@}@i{p@}/@xml{&#8706;@}@i{v@}
 *! 
 *! Then the generated normal @i{n@} is
 *! @i{n@}=@i{m@}/@xml{&#8741;@}@i{m@}@xml{&#8741;@}
 *! 
 *! If automatic normal generation is disabled, the corresponding normal
 *! map @[GL_MAP2_NORMAL], if enabled, is used to produce a normal. If
 *! neither automatic normal generation nor a normal map is enabled, no
 *! normal is generated for @[glEvalCoord] commands.
 *! 
 *! @param u
 *! 
 *! Specifies a value that is the domain coordinate @i{u@} to the basis
 *! function defined in a previous @[glMap1] or @[glMap2] command.
 *! 
 *! @param v
 *! 
 *! Specifies a value that is the domain coordinate @i{v@} to the basis
 *! function defined in a previous @[glMap2] command. This argument is not
 *! present in a @[glEvalCoord] command.
 *! 
 *! @param u
 *! 
 *! Specifies an array containing either one or two domain coordinates.
 *! The first coordinate is @i{u@}. The second coordinate is @i{v@}, which
 *! is present only in @[glEvalCoord] versions.
 *! 
 *! @seealso
 *! 
 *! @[glBegin], @[glColor], @[glEnable], @[glEvalMesh1] and
 *! @[glEvalMesh2], @[glEvalPoint], @[glIndex], @[glMap1], @[glMap2],
 *! @[glMapGrid], @[glNormal], @[glTexCoord], @[glVertex]
 *! 
 */


/*! @decl void glEvalPoint(int|array(int) i, int|void j)
 *! 
 *! @[glMapGrid] and @[glEvalMesh1] and @[glEvalMesh2] are used in tandem
 *! to efficiently generate and evaluate a series of evenly spaced map
 *! domain values. @[glEvalPoint] can be used to evaluate a single grid
 *! point in the same gridspace that is traversed by @[glEvalMesh1] and
 *! @[glEvalMesh2]. Calling @[glEvalPoint] is equivalent to calling
 *! @code
 *! 
 *! glEvalCoord1( i@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@} );
 *! @endcode
 *! where
 *! @xml{&#916;@}@i{u@}=(@i{u@}@sub{2@}-@i{u@}@sub{1@})/@i{n@}
 *! 
 *! and @i{n@}, @i{u@}@sub{1@}, and @i{u@}@sub{2@} are the arguments to
 *! the most recent @[glMapGrid] command. The one absolute numeric
 *! requirement is that if
 *! @i{i@}=@i{n@}
 *! , then the value computed from
 *! @i{i@}@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@}
 *! is exactly @i{u@}@sub{2@}.
 *! 
 *! In the two-dimensional case, @[glEvalPoint], let
 *! 
 *! @xml{&#916;@}@i{u@}=(@i{u@}@sub{2@}-@i{u@}@sub{1@})/@i{n@}
 *! @xml{&#916;@}@i{v@}=(@i{v@}@sub{2@}-@i{v@}@sub{1@})/@i{m@}
 *! 
 *! where @i{n@}, @i{u@}@sub{1@}, @i{u@}@sub{2@}, @i{m@}, @i{v@}@sub{1@},
 *! and @i{v@}@sub{2@} are the arguments to the most recent @[glMapGrid]
 *! command. Then the @[glEvalPoint] command is equivalent to calling
 *! @code
 *! 
 *! glEvalCoord2( i@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@},j@xml{&#183;@}@xml{&#916;@}@i{v@}+@i{v@}@sub{1@} );
 *! @endcode
 *! The only absolute numeric requirements are that if
 *! @i{i@}=@i{n@}
 *! , then the value computed from
 *! @i{i@}@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@}
 *! is exactly @i{u@}@sub{2@}, and if
 *! @i{j@}=@i{m@}
 *! , then the value computed from
 *! @i{j@}@xml{&#183;@}@xml{&#916;@}@i{v@}+@i{v@}@sub{1@}
 *! is exactly @i{v@}@sub{2@}.
 *! 
 *! @param i
 *! 
 *! Specifies the integer value for grid domain variable @i{i@}.
 *! 
 *! @param j
 *! 
 *! Specifies the integer value for grid domain variable @i{j@}
 *! (@[glEvalPoint] only).
 *! 
 *! @seealso
 *! 
 *! @[glEvalCoord], @[glEvalMesh1] and @[glEvalMesh2], @[glMap1],
 *! @[glMap2], @[glMapGrid]
 *! 
 */


/*! @decl void glFog(int pname, float|int|array(float|int) param)
 *! 
 *! Fog is initially disabled. While enabled, fog affects rasterized
 *! geometry, bitmaps, and pixel blocks, but not buffer clear operations.
 *! To enable and disable fog, call @[glEnable] and @[glDisable] with
 *! argument @[GL_FOG].
 *! 
 *! @[glFog] assigns the value or values in @i{params@} to the fog
 *! parameter specified by @i{pname@}. The following values are accepted
 *! for @i{pname@}:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_FOG_MODE</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies the equation to be used to compute the fog blend factor, <i
 *! >f</i>. Three symbolic constants are accepted: <ref>GL_LINEAR</ref>,
 *! <ref>GL_EXP</ref>, and <ref>GL_EXP2</ref>. The equations corresponding
 *! to these symbolic constants are defined below. The initial fog mode is
 *! <ref>GL_EXP</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_DENSITY</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies density, the fog density used in both exponential fog
 *! equations. Only nonnegative densities are accepted. The initial fog
 *! density is 1. </p></c></r>
 *! <r><c><ref>GL_FOG_START</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies start, the near distance used in the linear fog equation.
 *! The initial near distance is 0. </p></c></r>
 *! <r><c><ref>GL_FOG_END</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies end, the far distance used in the linear fog equation. The
 *! initial far distance is 1. </p></c></r>
 *! <r><c><ref>GL_FOG_INDEX</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies <i>i</i><sub><i>f</i></sub>, the fog color index. The
 *! initial fog index is 0. </p></c></r>
 *! <r><c><ref>GL_FOG_COLOR</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify <i>C</i><sub><i>f</i></sub>, the fog color. Integer
 *! values are mapped linearly such that the most positive representable
 *! value maps to 1.0, and the most negative representable value maps to
 *! -1.0. Floating-point values are mapped directly. After conversion, all
 *! color components are clamped to the range [0, 1]. The initial fog
 *! color is (0, 0, 0, 0). </p></c></r>
 *! <r><c><ref>GL_FOG_COORD_SRC</ref></c>
 *! <c><p><i>params</i> contains either of the following symbolic
 *! constants: <ref>GL_FOG_COORD</ref> or <ref>GL_FRAGMENT_DEPTH</ref>.
 *! <ref>GL_FOG_COORD</ref> specifies that the current fog coordinate
 *! should be used as distance value in the fog color computation. <ref
 *! >GL_FRAGMENT_DEPTH</ref> specifies that the current fragment depth
 *! should be used as distance value in the fog computation. </p></c></r>
 *! </matrix>@}Fog blends a fog color with each rasterized pixel
 *! fragment's post-texturing color using a blending factor @i{f@}. Factor
 *! @i{f@} is computed in one of three ways, depending on the fog mode.
 *! Let @i{c@} be either the distance in eye coordinate from the origin
 *! (in the case that the @[GL_FOG_COORD_SRC] is @[GL_FRAGMENT_DEPTH]) or
 *! the current fog coordinate (in the case that @[GL_FOG_COORD_SRC] is
 *! @[GL_FOG_COORD]). The equation for @[GL_LINEAR] fog is
 *! @i{f@}=end-@i{c@}/end-start
 *! 
 *! The equation for @[GL_EXP] fog is
 *! @i{f@}=@i{e@}@sup{-(density@xml{&#183;@}@i{c@})@}
 *! 
 *! The equation for @[GL_EXP2] fog is
 *! @i{f@}=@i{e@}@sup{-(density@xml{&#183;@}@i{c@})@sup{2@}@}
 *! 
 *! Regardless of the fog mode, @i{f@} is clamped to the range [0, 1]
 *! after it is computed. Then, if the GL is in RGBA color mode, the
 *! fragment's red, green, and blue colors, represented by
 *! @i{C@}@sub{@i{r@}@}, are replaced by
 *! 
 *! @i{C@}@sub{@i{r@}@}@sup{@xml{&#8243;@}@}=@i{f@}@xml{&#215;@}@i{C@}@sub{@i{r@}@}+(1-@i{f@})@xml{&#215;@}@i{C@}@sub{@i{f@}@}
 *! 
 *! Fog does not affect a fragment's alpha component.
 *! 
 *! In color index mode, the fragment's color index @i{i@}@sub{@i{r@}@} is
 *! replaced by
 *! 
 *! @i{i@}@sub{@i{r@}@}@sup{@xml{&#8243;@}@}=@i{i@}@sub{@i{r@}@}+(1-@i{f@})@xml{&#215;@}@i{i@}@sub{@i{f@}@}
 *! 
 *! @param pname
 *! 
 *! Specifies a single-valued fog parameter. @[GL_FOG_MODE],
 *! @[GL_FOG_DENSITY], @[GL_FOG_START], @[GL_FOG_END], @[GL_FOG_INDEX],
 *! and @[GL_FOG_COORD_SRC] are accepted.
 *! 
 *! @param param
 *! 
 *! Specifies the value that @i{pname@} will be set to.
 *! 
 *! @param pname
 *! 
 *! Specifies a fog parameter. @[GL_FOG_MODE], @[GL_FOG_DENSITY],
 *! @[GL_FOG_START], @[GL_FOG_END], @[GL_FOG_INDEX], @[GL_FOG_COLOR], and
 *! @[GL_FOG_COORD_SRC] are accepted.
 *! 
 *! @param params
 *! 
 *! Specifies the value or values to be assigned to @i{pname@}.
 *! @[GL_FOG_COLOR] requires an array of four values. All other parameters
 *! accept an array containing only a single value.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{pname@} is not an accepted
 *! value, or if @i{pname@} is @[GL_FOG_MODE] and @i{params@} is not an
 *! accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{pname@} is @[GL_FOG_DENSITY]
 *! and @i{params@} is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glFog] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[GL_FOG_COORD_SRC] is available only if the GL version is 1.4 or
 *! greater.
 *! 
 *! @seealso
 *! 
 *! @[glEnable]
 *! 
 */


/*! @decl int glGenLists(int range)
 *! 
 *! @[glGenLists] has one argument, @i{range@}. It returns an integer
 *! @i{n@} such that @i{range@} contiguous empty display lists, named
 *! @i{n@},
 *! @i{n@}+1
 *! , ...,
 *! @i{n@}+range-1
 *! , are created. If @i{range@} is 0, if there is no group of @i{range@}
 *! contiguous names available, or if any error is generated, no display
 *! lists are generated, and 0 is returned.
 *! 
 *! @param range
 *! 
 *! Specifies the number of contiguous empty display lists to be
 *! generated.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{range@} is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glGenLists] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glCallList], @[glCallLists], @[glDeleteLists], @[glNewList]
 *! 
 */


/*! @decl int glGetError()
 *! 
 *! @[glGetError] returns the value of the error flag. Each detectable
 *! error is assigned a numeric code and symbolic name. When an error
 *! occurs, the error flag is set to the appropriate error code value. No
 *! other errors are recorded until @[glGetError] is called, the error
 *! code is returned, and the flag is reset to @[GL_NO_ERROR]. If a call
 *! to @[glGetError] returns @[GL_NO_ERROR], there has been no detectable
 *! error since the last call to @[glGetError], or since the GL was
 *! initialized.
 *! 
 *! To allow for distributed implementations, there may be several error
 *! flags. If any single error flag has recorded an error, the value of
 *! that flag is returned and that flag is reset to @[GL_NO_ERROR] when
 *! @[glGetError] is called. If more than one flag has recorded an error,
 *! @[glGetError] returns and clears an arbitrary error flag value. Thus,
 *! @[glGetError] should always be called in a loop, until it returns
 *! @[GL_NO_ERROR], if all error flags are to be reset.
 *! 
 *! Initially, all error flags are set to @[GL_NO_ERROR].
 *! 
 *! The following errors are currently defined:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_NO_ERROR</ref></c>
 *! <c><p>No error has been recorded. The value of this symbolic constant
 *! is guaranteed to be 0. </p></c></r>
 *! <r><c><ref>GL_INVALID_ENUM</ref></c>
 *! <c><p>An unacceptable value is specified for an enumerated argument.
 *! The offending command is ignored and has no other side effect than to
 *! set the error flag. </p></c></r>
 *! <r><c><ref>GL_INVALID_VALUE</ref></c>
 *! <c><p>A numeric argument is out of range. The offending command is
 *! ignored and has no other side effect than to set the error flag. </p
 *! ></c></r>
 *! <r><c><ref>GL_INVALID_OPERATION</ref></c>
 *! <c><p>The specified operation is not allowed in the current state. The
 *! offending command is ignored and has no other side effect than to set
 *! the error flag. </p></c></r>
 *! <r><c><ref>GL_STACK_OVERFLOW</ref></c>
 *! <c><p>This command would cause a stack overflow. The offending command
 *! is ignored and has no other side effect than to set the error flag.
 *! </p></c></r>
 *! <r><c><ref>GL_STACK_UNDERFLOW</ref></c>
 *! <c><p>This command would cause a stack underflow. The offending
 *! command is ignored and has no other side effect than to set the error
 *! flag. </p></c></r>
 *! <r><c><ref>GL_OUT_OF_MEMORY</ref></c>
 *! <c><p>There is not enough memory left to execute the command. The
 *! state of the GL is undefined, except for the state of the error flags,
 *! after this error is recorded. </p></c></r>
 *! <r><c><ref>GL_TABLE_TOO_LARGE</ref></c>
 *! <c><p>The specified table exceeds the implementation's maximum
 *! supported table size. The offending command is ignored and has no
 *! other side effect than to set the error flag. </p></c></r>
 *! </matrix>@}When an error flag is set, results of a GL operation are
 *! undefined only if @[GL_OUT_OF_MEMORY] has occurred. In all other
 *! cases, the command generating the error is ignored and has no effect
 *! on the GL state or frame buffer contents. If the generating command
 *! returns a value, it returns 0. If @[glGetError] itself generates an
 *! error, it returns 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glGetError] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd]. In this case, @[glGetError] returns 0.
 *! 
 *! @note
 *! 
 *! @[GL_TABLE_TOO_LARGE] was introduced in GL version 1.2.
 *! 
 */


/*! @decl string glGetString(int name)
 *! 
 *! @[glGetString] returns a pointer to a static string describing some
 *! aspect of the current GL connection. @i{name@} can be one of the
 *! following:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_VENDOR</ref></c>
 *! <c><p></p><p>Returns the company responsible for this GL
 *! implementation. This name does not change from release to release. </p
 *! ></c></r>
 *! <r><c><ref>GL_RENDERER</ref></c>
 *! <c><p></p><p>Returns the name of the renderer. This name is typically
 *! specific to a particular configuration of a hardware platform. It does
 *! not change from release to release. </p></c></r>
 *! <r><c><ref>GL_VERSION</ref></c>
 *! <c><p></p><p>Returns a version or release number. </p></c></r>
 *! <r><c><ref>GL_SHADING_LANGUAGE_VERSION</ref></c>
 *! <c><p></p><p>Returns a version or release number for the shading
 *! language. </p></c></r>
 *! <r><c><ref>GL_EXTENSIONS</ref></c>
 *! <c><p></p><p>Returns a space-separated list of supported extensions to
 *! GL. </p></c></r>
 *! </matrix>@}Because the GL does not include queries for the performance
 *! characteristics of an implementation, some applications are written to
 *! recognize known platforms and modify their GL usage based on known
 *! performance characteristics of these platforms. Strings @[GL_VENDOR]
 *! and @[GL_RENDERER] together uniquely specify a platform. They do not
 *! change from release to release and should be used by
 *! platform-recognition algorithms.
 *! 
 *! Some applications want to make use of features that are not part of
 *! the standard GL. These features may be implemented as extensions to
 *! the standard GL. The @[GL_EXTENSIONS] string is a space-separated list
 *! of supported GL extensions. (Extension names never contain a space
 *! character.)
 *! 
 *! The @[GL_VERSION] and @[GL_SHADING_LANGUAGE_VERSION] strings begin
 *! with a version number. The version number uses one of these forms:
 *! 
 *! @i{major_number.minor_number@}@i{major_number.minor_number.release_number@}
 *! 
 *! Vendor-specific information may follow the version number. Its format
 *! depends on the implementation, but a space always separates the
 *! version number and the vendor-specific information.
 *! 
 *! All strings are null-terminated.
 *! 
 *! @param name
 *! 
 *! Specifies a symbolic constant, one of @[GL_VENDOR], @[GL_RENDERER],
 *! @[GL_VERSION], @[GL_SHADING_LANGUAGE_VERSION], or @[GL_EXTENSIONS].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{name@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glGetString] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! If an error is generated, @[glGetString] returns 0.
 *! 
 *! The client and server may support different versions or extensions.
 *! @[glGetString] always returns a compatible version number or list of
 *! extensions. The release number always describes the server.
 *! 
 *! @[GL_SHADING_LANGUAGE_VERSION] is available only if the GL version is
 *! 2.0 or greater.
 *! 
 */


/*! @decl void glGetTexImage(int target, int level, int format, int type, @
 *! System.Memory img)
 *! 
 *! @[glGetTexImage] returns a texture image into @i{img@}. @i{target@}
 *! specifies whether the desired texture image is one specified by
 *! @[glTexImage1D] (@[GL_TEXTURE_1D]), @[glTexImage2D] (@[GL_TEXTURE_2D]
 *! or any of @[GL_TEXTURE_CUBE_MAP_*]), or @[glTexImage3D]
 *! (@[GL_TEXTURE_3D]). @i{level@} specifies the level-of-detail number of
 *! the desired image. @i{format@} and @i{type@} specify the format and
 *! type of the desired image array. See the reference pages
 *! @[glTexImage1D] and @[glDrawPixels] for a description of the
 *! acceptable values for the @i{format@} and @i{type@} parameters,
 *! respectively.
 *! 
 *! If a non-zero named buffer object is bound to the
 *! @[GL_PIXEL_PACK_BUFFER] target (see @[glBindBuffer]) while a texture
 *! image is requested, @i{img@} is treated as a byte offset into the
 *! buffer object's data store.
 *! 
 *! To understand the operation of @[glGetTexImage], consider the selected
 *! internal four-component texture image to be an RGBA color buffer the
 *! size of the image. The semantics of @[glGetTexImage] are then
 *! identical to those of @[glReadPixels], with the exception that no
 *! pixel transfer operations are performed, when called with the same
 *! @i{format@} and @i{type@}, with @i{x@} and @i{y@} set to 0, @i{width@}
 *! set to the width of the texture image (including border if one was
 *! specified), and @i{height@} set to 1 for 1D images, or to the height
 *! of the texture image (including border if one was specified) for 2D
 *! images. Because the internal texture image is an RGBA image, pixel
 *! formats @[GL_COLOR_INDEX], @[GL_STENCIL_INDEX], and
 *! @[GL_DEPTH_COMPONENT] are not accepted, and pixel type @[GL_BITMAP] is
 *! not accepted.
 *! 
 *! If the selected texture image does not contain four components, the
 *! following mappings are applied. Single-component textures are treated
 *! as RGBA buffers with red set to the single-component value, green set
 *! to 0, blue set to 0, and alpha set to 1. Two-component textures are
 *! treated as RGBA buffers with red set to the value of component zero,
 *! alpha set to the value of component one, and green and blue set to 0.
 *! Finally, three-component textures are treated as RGBA buffers with red
 *! set to component zero, green set to component one, blue set to
 *! component two, and alpha set to 1.
 *! 
 *! To determine the required size of @i{img@}, use
 *! @[glGetTexLevelParameter] to determine the dimensions of the internal
 *! texture image, then scale the required number of pixels by the storage
 *! required for each pixel, based on @i{format@} and @i{type@}. Be sure
 *! to take the pixel storage parameters into account, especially
 *! @[GL_PACK_ALIGNMENT].
 *! 
 *! @param target
 *! 
 *! Specifies which texture is to be obtained. @[GL_TEXTURE_1D],
 *! @[GL_TEXTURE_2D], @[GL_TEXTURE_3D], @[GL_TEXTURE_CUBE_MAP_POSITIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z],
 *! and @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z] are accepted.
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number of the desired image. Level 0 is
 *! the base image level. Level @i{n@} is the @i{n@}th mipmap reduction
 *! image.
 *! 
 *! @param format
 *! 
 *! Specifies a pixel format for the returned data. The supported formats
 *! are @[GL_RED], @[GL_GREEN], @[GL_BLUE], @[GL_ALPHA], @[GL_RGB],
 *! @[GL_BGR], @[GL_RGBA], @[GL_BGRA], @[GL_LUMINANCE], and
 *! @[GL_LUMINANCE_ALPHA].
 *! 
 *! @param type
 *! 
 *! Specifies a pixel type for the returned data. The supported types are
 *! @[GL_UNSIGNED_BYTE], @[GL_BYTE], @[GL_UNSIGNED_SHORT], @[GL_SHORT],
 *! @[GL_UNSIGNED_INT], @[GL_INT], @[GL_FLOAT], @[GL_UNSIGNED_BYTE_3_3_2],
 *! @[GL_UNSIGNED_BYTE_2_3_3_REV], @[GL_UNSIGNED_SHORT_5_6_5],
 *! @[GL_UNSIGNED_SHORT_5_6_5_REV], @[GL_UNSIGNED_SHORT_4_4_4_4],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4_REV], @[GL_UNSIGNED_SHORT_5_5_5_1],
 *! @[GL_UNSIGNED_SHORT_1_5_5_5_REV], @[GL_UNSIGNED_INT_8_8_8_8],
 *! @[GL_UNSIGNED_INT_8_8_8_8_REV], @[GL_UNSIGNED_INT_10_10_10_2], and
 *! @[GL_UNSIGNED_INT_2_10_10_10_REV].
 *! 
 *! @param img
 *! 
 *! Returns the texture image. Should be an array of the type specified by
 *! @i{type@}.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@}, @i{format@}, or
 *! @i{type@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if @i{level@} is greater than
 *! log@sub{2@}(max)
 *! , where max is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_OPERATION] is returned if @i{type@} is one of
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], or @[GL_UNSIGNED_SHORT_5_6_5_REV] and
 *! @i{format@} is not @[GL_RGB].
 *! 
 *! @[GL_INVALID_OPERATION] is returned if @i{type@} is one of
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], or @[GL_UNSIGNED_INT_2_10_10_10_REV],
 *! and @i{format@} is neither @[GL_RGBA] or @[GL_BGRA].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_PACK_BUFFER] target and the buffer object's
 *! data store is currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_PACK_BUFFER] target and the data would be
 *! packed to the buffer object such that the memory writes required would
 *! exceed the data store size.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_PACK_BUFFER] target and @i{img@} is not
 *! evenly divisible into the number of bytes needed to store in memory a
 *! datum indicated by @i{type@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glGetTexImage] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! If an error is generated, no change is made to the contents of
 *! @i{img@}.
 *! 
 *! The types @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], @[GL_UNSIGNED_INT_2_10_10_10_REV], and
 *! the formats @[GL_BGR], and @[GL_BGRA] are available only if the GL
 *! version is 1.2 or greater.
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, @[glGetTexImage]
 *! returns the texture image for the active texture unit.
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glDrawPixels], @[glReadPixels], @[glTexEnv],
 *! @[glTexGen], @[glTexImage1D], @[glTexImage2D], @[glTexImage3D],
 *! @[glTexSubImage1D], @[glTexSubImage2D], @[glTexSubImage3D],
 *! @[glTexParameter]
 *! 
 */


/*! @decl void glReadPixels(int x, int y, int width, int height, @
 *! int format, int type, System.Memory data)
 *! 
 *! @[glReadPixels] returns pixel data from the frame buffer, starting
 *! with the pixel whose lower left corner is at location (@i{x@},
 *! @i{y@}), into client memory starting at location @i{data@}. Several
 *! parameters control the processing of the pixel data before it is
 *! placed into client memory. These parameters are set with three
 *! commands: @[glPixelStore], @[glPixelTransfer], and @[glPixelMap]. This
 *! reference page describes the effects on @[glReadPixels] of most, but
 *! not all of the parameters specified by these three commands.
 *! 
 *! If a non-zero named buffer object is bound to the
 *! @[GL_PIXEL_PACK_BUFFER] target (see @[glBindBuffer]) while a block of
 *! pixels is requested, @i{data@} is treated as a byte offset into the
 *! buffer object's data store rather than a pointer to client memory.
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, the pixel data may
 *! be processed by additional operations including color table lookup,
 *! color matrix transformations, convolutions, histograms, and minimum
 *! and maximum pixel value computations.
 *! 
 *! @[glReadPixels] returns values from each pixel with lower left corner
 *! at
 *! (@i{x@}+@i{i@}, @i{y@}+@i{j@})
 *! for
 *! 0<=@i{i@}<width
 *! and
 *! 0<=@i{j@}<height
 *! . This pixel is said to be the @i{i@}th pixel in the @i{j@}th row.
 *! Pixels are returned in row order from the lowest to the highest row,
 *! left to right in each row.
 *! 
 *! @i{format@} specifies the format for the returned pixel values;
 *! accepted values are:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_COLOR_INDEX</ref></c>
 *! <c><p>Color indices are read from the color buffer selected by <ref
 *! >glReadBuffer</ref>. Each index is converted to fixed point, shifted
 *! left or right depending on the value and sign of <ref
 *! >GL_INDEX_SHIFT</ref>, and added to <ref>GL_INDEX_OFFSET</ref>. If
 *! <ref>GL_MAP_COLOR</ref> is <ref>GL_TRUE</ref>, indices are replaced by
 *! their mappings in the table <ref>GL_PIXEL_MAP_I_TO_I</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_STENCIL_INDEX</ref></c>
 *! <c><p>Stencil values are read from the stencil buffer. Each index is
 *! converted to fixed point, shifted left or right depending on the value
 *! and sign of <ref>GL_INDEX_SHIFT</ref>, and added to <ref
 *! >GL_INDEX_OFFSET</ref>. If <ref>GL_MAP_STENCIL</ref> is <ref
 *! >GL_TRUE</ref>, indices are replaced by their mappings in the table
 *! <ref>GL_PIXEL_MAP_S_TO_S</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_COMPONENT</ref></c>
 *! <c><p>Depth values are read from the depth buffer. Each component is
 *! converted to floating point such that the minimum depth value maps to
 *! 0 and the maximum value maps to 1. Each component is then multiplied
 *! by <ref>GL_DEPTH_SCALE</ref>, added to <ref>GL_DEPTH_BIAS</ref>, and
 *! finally clamped to the range [0, 1]. </p></c></r>
 *! <r><c><ref>GL_RED</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_GREEN</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BLUE</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_ALPHA</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_RGB</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BGR</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_RGBA</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BGRA</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_LUMINANCE</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_LUMINANCE_ALPHA</ref></c>
 *! <c><p>Processing differs depending on whether color buffers store
 *! color indices or RGBA color components. If color indices are stored,
 *! they are read from the color buffer selected by <ref>glReadBuffer</ref
 *! >. Each index is converted to fixed point, shifted left or right
 *! depending on the value and sign of <ref>GL_INDEX_SHIFT</ref>, and
 *! added to <ref>GL_INDEX_OFFSET</ref>. Indices are then replaced by the
 *! red, green, blue, and alpha values obtained by indexing the tables
 *! <ref>GL_PIXEL_MAP_I_TO_R</ref>, <ref>GL_PIXEL_MAP_I_TO_G</ref>, <ref
 *! >GL_PIXEL_MAP_I_TO_B</ref>, and <ref>GL_PIXEL_MAP_I_TO_A</ref>. Each
 *! table must be of size 2<sup><i>n</i></sup>, but <i>n</i> may be
 *! different for different tables. Before an index is used to look up a
 *! value in a table of size 2<sup><i>n</i></sup>, it must be masked
 *! against 2<sup><i>n</i></sup>-1. </p><p>If RGBA color components are
 *! stored in the color buffers, they are read from the color buffer
 *! selected by <ref>glReadBuffer</ref>. Each color component is converted
 *! to floating point such that zero intensity maps to 0.0 and full
 *! intensity maps to 1.0. Each component is then multiplied by <tt>GL_<i
 *! >c</i>_SCALE</tt> and added to <tt>GL_<i>c</i>_BIAS</tt>, where <i
 *! >c</i> is RED, GREEN, BLUE, or ALPHA. Finally, if <ref
 *! >GL_MAP_COLOR</ref> is <ref>GL_TRUE</ref>, each component is clamped
 *! to the range [0, 1], scaled to the size of its corresponding table,
 *! and is then replaced by its mapping in the table <tt>GL_PIXEL_MAP_<i
 *! >c</i>_TO_<i>c</i></tt>, where <i>c</i> is R, G, B, or A. </p><p
 *! >Unneeded data is then discarded. For example, <ref>GL_RED</ref>
 *! discards the green, blue, and alpha components, while <ref>GL_RGB</ref
 *! > discards only the alpha component. <ref>GL_LUMINANCE</ref> computes
 *! a single-component value as the sum of the red, green, and blue
 *! components, and <ref>GL_LUMINANCE_ALPHA</ref> does the same, while
 *! keeping alpha as a second value. The final values are clamped to the
 *! range [0, 1]. </p></c></r>
 *! </matrix>@}The shift, scale, bias, and lookup factors just described
 *! are all specified by @[glPixelTransfer]. The lookup table contents
 *! themselves are specified by @[glPixelMap].
 *! 
 *! Finally, the indices or components are converted to the proper format,
 *! as specified by @i{type@}. If @i{format@} is @[GL_COLOR_INDEX] or
 *! @[GL_STENCIL_INDEX] and @i{type@} is not @[GL_FLOAT], each index is
 *! masked with the mask value given in the following table. If @i{type@}
 *! is @[GL_FLOAT], then each integer index is converted to
 *! single-precision floating-point format.
 *! 
 *! If @i{format@} is @[GL_RED], @[GL_GREEN], @[GL_BLUE], @[GL_ALPHA],
 *! @[GL_RGB], @[GL_BGR], @[GL_RGBA], @[GL_BGRA], @[GL_LUMINANCE], or
 *! @[GL_LUMINANCE_ALPHA] and @i{type@} is not @[GL_FLOAT], each component
 *! is multiplied by the multiplier shown in the following table. If type
 *! is @[GL_FLOAT], then each component is passed as is (or converted to
 *! the client's single-precision floating-point format if it is different
 *! from the one used by the GL).
 *! 
 *! @xml{<matrix><r><c><i>type</i></c><c><b> Index Mask </b></c><c><b
 *! >Component Conversion </b></c></r><r><c><ref>GL_UNSIGNED_BYTE</ref></c
 *! ><c> 2<sup>8</sup>-1</c><c> (2<sup>8</sup>-1)<i>c</i></c></r><r><c
 *! ><ref>GL_BYTE</ref></c><c> 2<sup>7</sup>-1</c><c> (2<sup>8</sup>-1)<i
 *! >c</i>-1/2</c></r><r><c><ref>GL_BITMAP</ref></c><c>1</c><c>1</c></r><r
 *! ><c><ref>GL_UNSIGNED_SHORT</ref></c><c> 2<sup>16</sup>-1</c><c> (2<sup
 *! >16</sup>-1)<i>c</i></c></r><r><c><ref>GL_SHORT</ref></c><c> 2<sup
 *! >15</sup>-1</c><c> (2<sup>16</sup>-1)<i>c</i>-1/2</c></r><r><c><ref
 *! >GL_UNSIGNED_INT</ref></c><c> 2<sup>32</sup>-1</c><c> (2<sup>32</sup
 *! >-1)<i>c</i></c></r><r><c><ref>GL_INT</ref></c><c> 2<sup>31</sup>-1</c
 *! ><c> (2<sup>32</sup>-1)<i>c</i>-1/2</c></r><r><c><ref>GL_FLOAT</ref
 *! ></c><c> none </c><c><i>c</i></c></r></matrix>@} Return values are
 *! placed in memory as follows. If @i{format@} is @[GL_COLOR_INDEX],
 *! @[GL_STENCIL_INDEX], @[GL_DEPTH_COMPONENT], @[GL_RED], @[GL_GREEN],
 *! @[GL_BLUE], @[GL_ALPHA], or @[GL_LUMINANCE], a single value is
 *! returned and the data for the @i{i@}th pixel in the @i{j@}th row is
 *! placed in location
 *! (@i{j@})width+@i{i@}
 *! . @[GL_RGB] and @[GL_BGR] return three values, @[GL_RGBA] and
 *! @[GL_BGRA] return four values, and @[GL_LUMINANCE_ALPHA] returns two
 *! values for each pixel, with all values corresponding to a single pixel
 *! occupying contiguous space in @i{data@}. Storage parameters set by
 *! @[glPixelStore], such as @[GL_PACK_LSB_FIRST] and
 *! @[GL_PACK_SWAP_BYTES], affect the way that data is written into
 *! memory. See @[glPixelStore] for a description.
 *! 
 *! @param x
 *! @param y
 *! 
 *! Specify the window coordinates of the first pixel that is read from
 *! the frame buffer. This location is the lower left corner of a
 *! rectangular block of pixels.
 *! 
 *! @param width
 *! @param height
 *! 
 *! Specify the dimensions of the pixel rectangle. @i{width@} and
 *! @i{height@} of one correspond to a single pixel.
 *! 
 *! @param format
 *! 
 *! Specifies the format of the pixel data. The following symbolic values
 *! are accepted: @[GL_COLOR_INDEX], @[GL_STENCIL_INDEX],
 *! @[GL_DEPTH_COMPONENT], @[GL_RED], @[GL_GREEN], @[GL_BLUE],
 *! @[GL_ALPHA], @[GL_RGB], @[GL_BGR], @[GL_RGBA], @[GL_BGRA],
 *! @[GL_LUMINANCE], and @[GL_LUMINANCE_ALPHA].
 *! 
 *! @param type
 *! 
 *! Specifies the data type of the pixel data. Must be one of
 *! @[GL_UNSIGNED_BYTE], @[GL_BYTE], @[GL_BITMAP], @[GL_UNSIGNED_SHORT],
 *! @[GL_SHORT], @[GL_UNSIGNED_INT], @[GL_INT], @[GL_FLOAT],
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], or @[GL_UNSIGNED_INT_2_10_10_10_REV].
 *! 
 *! @param data
 *! 
 *! Returns the pixel data.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{format@} or @i{type@} is not an
 *! accepted value.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *! @i{format@} is not @[GL_COLOR_INDEX] or @[GL_STENCIL_INDEX].
 *! 
 *! @[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@}
 *! is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is
 *! @[GL_COLOR_INDEX] and the color buffers store RGBA color components.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is
 *! @[GL_STENCIL_INDEX] and there is no stencil buffer.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is
 *! @[GL_DEPTH_COMPONENT] and there is no depth buffer.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], or @[GL_UNSIGNED_SHORT_5_6_5_REV] and
 *! @i{format@} is not @[GL_RGB].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], or @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! and @i{format@} is neither @[GL_RGBA] nor @[GL_BGRA].
 *! 
 *! The formats @[GL_BGR], and @[GL_BGRA] and types
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], and @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! are available only if the GL version is 1.2 or greater.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_PACK_BUFFER] target and the buffer object's
 *! data store is currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_PACK_BUFFER] target and the data would be
 *! packed to the buffer object such that the memory writes required would
 *! exceed the data store size.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_PACK_BUFFER] target and @i{data@} is not
 *! evenly divisible into the number of bytes needed to store in memory a
 *! datum indicated by @i{type@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glReadPixels] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Values for pixels that lie outside the window connected to the current
 *! GL context are undefined.
 *! 
 *! If an error is generated, no change is made to the contents of
 *! @i{data@}.
 *! 
 *! @seealso
 *! 
 *! @[glCopyPixels], @[glDrawPixels], @[glPixelMap], @[glPixelStore],
 *! @[glPixelTransfer], @[glReadBuffer]
 *! 
 */


/*! @decl void glSelectBuffer(int size, System.Memory buffer)
 *! 
 *! @[glSelectBuffer] has two arguments: @i{buffer@} is an array of
 *! unsigned integers, and @i{size@} indicates the size of the array.
 *! @i{buffer@} returns values from the name stack (see @[glInitNames],
 *! @[glLoadName], @[glPushName]) when the rendering mode is @[GL_SELECT]
 *! (see @[glRenderMode]). @[glSelectBuffer] must be issued before
 *! selection mode is enabled, and it must not be issued while the
 *! rendering mode is @[GL_SELECT].
 *! 
 *! A programmer can use selection to determine which primitives are drawn
 *! into some region of a window. The region is defined by the current
 *! modelview and perspective matrices.
 *! 
 *! In selection mode, no pixel fragments are produced from rasterization.
 *! Instead, if a primitive or a raster position intersects the clipping
 *! volume defined by the viewing frustum and the user-defined clipping
 *! planes, this primitive causes a selection hit. (With polygons, no hit
 *! occurs if the polygon is culled.) When a change is made to the name
 *! stack, or when @[glRenderMode] is called, a hit record is copied to
 *! @i{buffer@} if any hits have occurred since the last such event (name
 *! stack change or @[glRenderMode] call). The hit record consists of the
 *! number of names in the name stack at the time of the event, followed
 *! by the minimum and maximum depth values of all vertices that hit since
 *! the previous event, followed by the name stack contents, bottom name
 *! first.
 *! 
 *! Depth values (which are in the range [0,1]) are multiplied by
 *! 2@sup{32@}-1
 *! , before being placed in the hit record.
 *! 
 *! An internal index into @i{buffer@} is reset to 0 whenever selection
 *! mode is entered. Each time a hit record is copied into @i{buffer@},
 *! the index is incremented to point to the cell just past the end of the
 *! block of names\(emthat is, to the next available cell If the hit
 *! record is larger than the number of remaining locations in
 *! @i{buffer@}, as much data as can fit is copied, and the overflow flag
 *! is set. If the name stack is empty when a hit record is copied, that
 *! record consists of 0 followed by the minimum and maximum depth values.
 *! 
 *! To exit selection mode, call @[glRenderMode] with an argument other
 *! than @[GL_SELECT]. Whenever @[glRenderMode] is called while the render
 *! mode is @[GL_SELECT], it returns the number of hit records copied to
 *! @i{buffer@}, resets the overflow flag and the selection buffer
 *! pointer, and initializes the name stack to be empty. If the overflow
 *! bit was set when @[glRenderMode] was called, a negative hit record
 *! count is returned.
 *! 
 *! @param size
 *! 
 *! Specifies the size of @i{buffer@}.
 *! 
 *! @param buffer
 *! 
 *! Returns the selection data.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{size@} is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glSelectBuffer] is called
 *! while the render mode is @[GL_SELECT], or if @[glRenderMode] is called
 *! with argument @[GL_SELECT] before @[glSelectBuffer] is called at least
 *! once.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glSelectBuffer] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! The contents of @i{buffer@} is undefined until @[glRenderMode] is
 *! called with an argument other than @[GL_SELECT].
 *! 
 *! @[glBegin]/@[glEnd] primitives and calls to @[glRasterPos] can result
 *! in hits. @[glWindowPos] will always generate a selection hit.
 *! 
 *! @seealso
 *! 
 *! @[glFeedbackBuffer], @[glInitNames], @[glLoadName], @[glPushName],
 *! @[glRenderMode]
 *! 
 */


/*! @decl void glFeedbackBuffer(int size, int type, System.Memory buffer)
 *! 
 *! The @[glFeedbackBuffer] function controls feedback. Feedback, like
 *! selection, is a GL mode. The mode is selected by calling
 *! @[glRenderMode] with @[GL_FEEDBACK]. When the GL is in feedback mode,
 *! no pixels are produced by rasterization. Instead, information about
 *! primitives that would have been rasterized is fed back to the
 *! application using the GL.
 *! 
 *! @[glFeedbackBuffer] has three arguments: @i{buffer@} is an array of
 *! floating-point values into which feedback information is placed.
 *! @i{size@} indicates the size of the array. @i{type@} is a symbolic
 *! constant describing the information that is fed back for each vertex.
 *! @[glFeedbackBuffer] must be issued before feedback mode is enabled (by
 *! calling @[glRenderMode] with argument @[GL_FEEDBACK]). Setting
 *! @[GL_FEEDBACK] without establishing the feedback buffer, or calling
 *! @[glFeedbackBuffer] while the GL is in feedback mode, is an error.
 *! 
 *! When @[glRenderMode] is called while in feedback mode, it returns the
 *! number of entries placed in the feedback array and resets the feedback
 *! array pointer to the base of the feedback buffer. The returned value
 *! never exceeds @i{size@}. If the feedback data required more room than
 *! was available in @i{buffer@}, @[glRenderMode] returns a negative
 *! value. To take the GL out of feedback mode, call @[glRenderMode] with
 *! a parameter value other than @[GL_FEEDBACK].
 *! 
 *! While in feedback mode, each primitive, bitmap, or pixel rectangle
 *! that would be rasterized generates a block of values that are copied
 *! into the feedback array. If doing so would cause the number of entries
 *! to exceed the maximum, the block is partially written so as to fill
 *! the array (if there is any room left at all), and an overflow flag is
 *! set. Each block begins with a code indicating the primitive type,
 *! followed by values that describe the primitive's vertices and
 *! associated data. Entries are also written for bitmaps and pixel
 *! rectangles. Feedback occurs after polygon culling and @[glPolygonMode]
 *! interpretation of polygons has taken place, so polygons that are
 *! culled are not returned in the feedback buffer. It can also occur
 *! after polygons with more than three edges are broken up into
 *! triangles, if the GL implementation renders polygons by performing
 *! this decomposition.
 *! 
 *! The @[glPassThrough] command can be used to insert a marker into the
 *! feedback buffer. See @[glPassThrough].
 *! 
 *! Following is the grammar for the blocks of values written into the
 *! feedback buffer. Each primitive is indicated with a unique identifying
 *! value followed by some number of vertices. Polygon entries include an
 *! integer value indicating how many vertices follow. A vertex is fed
 *! back as some number of floating-point values, as determined by
 *! @i{type@}. Colors are fed back as four values in RGBA mode and one
 *! value in color index mode.
 *! 
 *! feedbackList
 *! @xml{&#8592;@}
 *! feedbackItem feedbackList | feedbackItem
 *! 
 *! feedbackItem
 *! @xml{&#8592;@}
 *! point | lineSegment | polygon | bitmap | pixelRectangle | passThru
 *! 
 *! point
 *! @xml{&#8592;@}
 *! @[GL_POINT_TOKEN] vertex
 *! 
 *! lineSegment
 *! @xml{&#8592;@}
 *! @[GL_LINE_TOKEN] vertex vertex | @[GL_LINE_RESET_TOKEN] vertex vertex
 *! 
 *! polygon
 *! @xml{&#8592;@}
 *! @[GL_POLYGON_TOKEN] n polySpec
 *! 
 *! polySpec
 *! @xml{&#8592;@}
 *! polySpec vertex | vertex vertex vertex
 *! 
 *! bitmap
 *! @xml{&#8592;@}
 *! @[GL_BITMAP_TOKEN] vertex
 *! 
 *! pixelRectangle
 *! @xml{&#8592;@}
 *! @[GL_DRAW_PIXEL_TOKEN] vertex | @[GL_COPY_PIXEL_TOKEN] vertex
 *! 
 *! passThru
 *! @xml{&#8592;@}
 *! @[GL_PASS_THROUGH_TOKEN] value
 *! 
 *! vertex
 *! @xml{&#8592;@}
 *! 2d | 3d | 3dColor | 3dColorTexture | 4dColorTexture
 *! 
 *! 2d
 *! @xml{&#8592;@}
 *! value value
 *! 
 *! 3d
 *! @xml{&#8592;@}
 *! value value value
 *! 
 *! 3dColor
 *! @xml{&#8592;@}
 *! value value value color
 *! 
 *! 3dColorTexture
 *! @xml{&#8592;@}
 *! value value value color tex
 *! 
 *! 4dColorTexture
 *! @xml{&#8592;@}
 *! value value value value color tex
 *! 
 *! color
 *! @xml{&#8592;@}
 *! rgba | index
 *! 
 *! rgba
 *! @xml{&#8592;@}
 *! value value value value
 *! 
 *! index
 *! @xml{&#8592;@}
 *! value
 *! 
 *! tex
 *! @xml{&#8592;@}
 *! value value value value
 *! 
 *! @i{value@} is a floating-point number, and @i{n@} is a floating-point
 *! integer giving the number of vertices in the polygon.
 *! @[GL_POINT_TOKEN], @[GL_LINE_TOKEN], @[GL_LINE_RESET_TOKEN],
 *! @[GL_POLYGON_TOKEN], @[GL_BITMAP_TOKEN], @[GL_DRAW_PIXEL_TOKEN],
 *! @[GL_COPY_PIXEL_TOKEN] and @[GL_PASS_THROUGH_TOKEN] are symbolic
 *! floating-point constants. @[GL_LINE_RESET_TOKEN] is returned whenever
 *! the line stipple pattern is reset. The data returned as a vertex
 *! depends on the feedback @i{type@}.
 *! 
 *! The following table gives the correspondence between @i{type@} and the
 *! number of values per vertex. @i{k@} is 1 in color index mode and 4 in
 *! RGBA mode.
 *! 
 *! @xml{<matrix><r><c><b>Type </b></c><c><b>Coordinates </b></c><c><b
 *! >Color </b></c><c><b>Texture </b></c><c><b>Total Number of Values </b
 *! ></c></r><r><c><ref>GL_2D</ref></c><c><i>x</i>, <i>y</i></c><c></c><c
 *! ></c><c> 2 </c></r><r><c><ref>GL_3D</ref></c><c><i>x</i>, <i>y</i>, <i
 *! >z</i></c><c></c><c></c><c> 3 </c></r><r><c><ref>GL_3D_COLOR</ref></c
 *! ><c><i>x</i>, <i>y</i>, <i>z</i></c><c><i>k</i></c><c></c><c> 3+<i
 *! >k</i></c></r><r><c><ref>GL_3D_COLOR_TEXTURE</ref></c><c><i>x</i>, <i
 *! >y</i>, <i>z</i></c><c><i>k</i></c><c> 4 </c><c>7+<i>k</i></c></r><r
 *! ><c><ref>GL_4D_COLOR_TEXTURE</ref></c><c><i>x</i>, <i>y</i>, <i>z</i>,
 *! <i>w</i></c><c><i>k</i></c><c> 4 </c><c>8+<i>k</i></c></r></matrix>@}
 *! Feedback vertex coordinates are in window coordinates, except @i{w@},
 *! which is in clip coordinates. Feedback colors are lighted, if lighting
 *! is enabled. Feedback texture coordinates are generated, if texture
 *! coordinate generation is enabled. They are always transformed by the
 *! texture matrix.
 *! 
 *! @param size
 *! 
 *! Specifies the maximum number of values that can be written into
 *! @i{buffer@}.
 *! 
 *! @param type
 *! 
 *! Specifies a symbolic constant that describes the information that will
 *! be returned for each vertex. @[GL_2D], @[GL_3D], @[GL_3D_COLOR],
 *! @[GL_3D_COLOR_TEXTURE], and @[GL_4D_COLOR_TEXTURE] are accepted.
 *! 
 *! @param buffer
 *! 
 *! Returns the feedback data.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{size@} is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glFeedbackBuffer] is called
 *! while the render mode is @[GL_FEEDBACK], or if @[glRenderMode] is
 *! called with argument @[GL_FEEDBACK] before @[glFeedbackBuffer] is
 *! called at least once.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glFeedbackBuffer] is
 *! executed between the execution of @[glBegin] and the corresponding
 *! execution of @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glFeedbackBuffer], when used in a display list, is not compiled into
 *! the display list but is executed immediately.
 *! 
 *! @[glFeedbackBuffer] returns only the texture coordinate of texture
 *! unit @[GL_TEXTURE0].
 *! 
 *! @seealso
 *! 
 *! @[glBegin], @[glLineStipple], @[glPassThrough], @[glPolygonMode],
 *! @[glRenderMode], @[glSelectBuffer]
 *! 
 */


/*! @decl void glVertexPointer(int size, int type, int stride, @
 *! System.Memory pointer)
 *! 
 *! @[glVertexPointer] specifies the location and data format of an array
 *! of vertex coordinates to use when rendering. @i{size@} specifies the
 *! number of coordinates per vertex, and must be 2, 3, or 4. @i{type@}
 *! specifies the data type of each coordinate, and @i{stride@} specifies
 *! the byte stride from one vertex to the next, allowing vertices and
 *! attributes to be packed into a single array or stored in separate
 *! arrays. (Single-array storage may be more efficient on some
 *! implementations; see @[glInterleavedArrays].)
 *! 
 *! If a non-zero named buffer object is bound to the @[GL_ARRAY_BUFFER]
 *! target (see @[glBindBuffer]) while a vertex array is specified,
 *! @i{pointer@} is treated as a byte offset into the buffer object's data
 *! store. Also, the buffer object binding (@[GL_ARRAY_BUFFER_BINDING]) is
 *! saved as vertex array client-side state
 *! (@[GL_VERTEX_ARRAY_BUFFER_BINDING]).
 *! 
 *! When a vertex array is specified, @i{size@}, @i{type@}, @i{stride@},
 *! and @i{pointer@} are saved as client-side state, in addition to the
 *! current vertex array buffer object binding.
 *! 
 *! To enable and disable the vertex array, call @[glEnableClientState]
 *! and @[glDisableClientState] with the argument @[GL_VERTEX_ARRAY]. If
 *! enabled, the vertex array is used when @[glArrayElement],
 *! @[glDrawArrays], @[glMultiDrawArrays], @[glDrawElements],
 *! @[glMultiDrawElements], or @[glDrawRangeElements] is called.
 *! 
 *! @param size
 *! 
 *! Specifies the number of coordinates per vertex. Must be 2, 3, or 4.
 *! The initial value is 4.
 *! 
 *! @param type
 *! 
 *! Specifies the data type of each coordinate in the array. Symbolic
 *! constants @[GL_SHORT], @[GL_INT], @[GL_FLOAT], or @[GL_DOUBLE] are
 *! accepted. The initial value is @[GL_FLOAT].
 *! 
 *! @param stride
 *! 
 *! Specifies the byte offset between consecutive vertices. If @i{stride@}
 *! is 0, the vertices are understood to be tightly packed in the array.
 *! The initial value is 0.
 *! 
 *! @param pointer
 *! 
 *! Specifies a pointer to the first coordinate of the first vertex in the
 *! array. The initial value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{size@} is not 2, 3, or 4.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{stride@} is negative.
 *! 
 *! @note
 *! 
 *! @[glVertexPointer] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! The vertex array is initially disabled and isn't accessed when
 *! @[glArrayElement], @[glDrawElements], @[glDrawRangeElements],
 *! @[glDrawArrays], @[glMultiDrawArrays], or @[glMultiDrawElements] is
 *! called.
 *! 
 *! Execution of @[glVertexPointer] is not allowed between the execution
 *! of @[glBegin] and the corresponding execution of @[glEnd], but an
 *! error may or may not be generated. If no error is generated, the
 *! operation is undefined.
 *! 
 *! @[glVertexPointer] is typically implemented on the client side.
 *! 
 *! Vertex array parameters are client-side state and are therefore not
 *! saved or restored by @[glPushAttrib] and @[glPopAttrib]. Use
 *! @[glPushClientAttrib] and @[glPopClientAttrib] instead.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glBindBuffer], @[glColorPointer],
 *! @[glDisableClientState], @[glDrawArrays], @[glDrawElements],
 *! @[glDrawRangeElements], @[glEdgeFlagPointer], @[glEnableClientState],
 *! @[glFogCoordPointer], @[glIndexPointer], @[glInterleavedArrays],
 *! @[glMultiDrawArrays], @[glMultiDrawElements], @[glNormalPointer],
 *! @[glPopClientAttrib], @[glPushClientAttrib],
 *! @[glSecondaryColorPointer], @[glTexCoordPointer], @[glVertex],
 *! @[glVertexAttribPointer]
 *! 
 */


/*! @decl void glInterleavedArrays(int format, int stride, @
 *! System.Memory pointer)
 *! 
 *! @[glInterleavedArrays] lets you specify and enable individual color,
 *! normal, texture and vertex arrays whose elements are part of a larger
 *! aggregate array element. For some implementations, this is more
 *! efficient than specifying the arrays separately.
 *! 
 *! If @i{stride@} is 0, the aggregate elements are stored consecutively.
 *! Otherwise, @i{stride@} bytes occur between the beginning of one
 *! aggregate array element and the beginning of the next aggregate array
 *! element.
 *! 
 *! @i{format@} serves as a ``key'' describing the extraction of
 *! individual arrays from the aggregate array. If @i{format@} contains a
 *! T, then texture coordinates are extracted from the interleaved array.
 *! If C is present, color values are extracted. If N is present, normal
 *! coordinates are extracted. Vertex coordinates are always extracted.
 *! 
 *! The digits 2, 3, and 4 denote how many values are extracted. F
 *! indicates that values are extracted as floating-point values. Colors
 *! may also be extracted as 4 unsigned bytes if 4UB follows the C. If a
 *! color is extracted as 4 unsigned bytes, the vertex array element which
 *! follows is located at the first possible floating-point aligned
 *! address.
 *! 
 *! @param format
 *! 
 *! Specifies the type of array to enable. Symbolic constants @[GL_V2F],
 *! @[GL_V3F], @[GL_C4UB_V2F], @[GL_C4UB_V3F], @[GL_C3F_V3F],
 *! @[GL_N3F_V3F], @[GL_C4F_N3F_V3F], @[GL_T2F_V3F], @[GL_T4F_V4F],
 *! @[GL_T2F_C4UB_V3F], @[GL_T2F_C3F_V3F], @[GL_T2F_N3F_V3F],
 *! @[GL_T2F_C4F_N3F_V3F], and @[GL_T4F_C4F_N3F_V4F] are accepted.
 *! 
 *! @param stride
 *! 
 *! Specifies the offset in bytes between each aggregate array element.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{format@} is not an accepted
 *! value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{stride@} is negative.
 *! 
 *! @note
 *! 
 *! @[glInterleavedArrays] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! If @[glInterleavedArrays] is called while compiling a display list, it
 *! is not compiled into the list, and it is executed immediately.
 *! 
 *! Execution of @[glInterleavedArrays] is not allowed between the
 *! execution of @[glBegin] and the corresponding execution of @[glEnd],
 *! but an error may or may not be generated. If no error is generated,
 *! the operation is undefined.
 *! 
 *! @[glInterleavedArrays] is typically implemented on the client side.
 *! 
 *! Vertex array parameters are client-side state and are therefore not
 *! saved or restored by @[glPushAttrib] and @[glPopAttrib]. Use
 *! @[glPushClientAttrib] and @[glPopClientAttrib] instead.
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, @[glInterleavedArrays]
 *! only updates the texture coordinate array for the client active
 *! texture unit. The texture coordinate state for other client texture
 *! units is not updated, regardless of whether the client texture unit is
 *! enabled or not.
 *! 
 *! Secondary color values are not supported in interleaved vertex array
 *! formats.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glClientActiveTexture], @[glColorPointer],
 *! @[glDrawArrays], @[glDrawElements], @[glEdgeFlagPointer],
 *! @[glEnableClientState], @[glGetPointerv], @[glIndexPointer],
 *! @[glNormalPointer], @[glSecondaryColorPointer], @[glTexCoordPointer],
 *! @[glVertexPointer]
 *! 
 */


/*! @decl void glTexCoordPointer(int size, int type, int stride, @
 *! System.Memory pointer)
 *! 
 *! @[glTexCoordPointer] specifies the location and data format of an
 *! array of texture coordinates to use when rendering. @i{size@}
 *! specifies the number of coordinates per texture coordinate set, and
 *! must be 1, 2, 3, or 4. @i{type@} specifies the data type of each
 *! texture coordinate, and @i{stride@} specifies the byte stride from one
 *! texture coordinate set to the next, allowing vertices and attributes
 *! to be packed into a single array or stored in separate arrays.
 *! (Single-array storage may be more efficient on some implementations;
 *! see @[glInterleavedArrays].)
 *! 
 *! If a non-zero named buffer object is bound to the @[GL_ARRAY_BUFFER]
 *! target (see @[glBindBuffer]) while a texture coordinate array is
 *! specified, @i{pointer@} is treated as a byte offset into the buffer
 *! object's data store. Also, the buffer object binding
 *! (@[GL_ARRAY_BUFFER_BINDING]) is saved as texture coordinate vertex
 *! array client-side state (@[GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING]).
 *! 
 *! When a texture coordinate array is specified, @i{size@}, @i{type@},
 *! @i{stride@}, and @i{pointer@} are saved as client-side state, in
 *! addition to the current vertex array buffer object binding.
 *! 
 *! To enable and disable a texture coordinate array, call
 *! @[glEnableClientState] and @[glDisableClientState] with the argument
 *! @[GL_TEXTURE_COORD_ARRAY]. If enabled, the texture coordinate array is
 *! used when @[glArrayElement], @[glDrawArrays], @[glMultiDrawArrays],
 *! @[glDrawElements], @[glMultiDrawElements], or @[glDrawRangeElements]
 *! is called.
 *! 
 *! @param size
 *! 
 *! Specifies the number of coordinates per array element. Must be 1, 2,
 *! 3, or 4. The initial value is 4.
 *! 
 *! @param type
 *! 
 *! Specifies the data type of each texture coordinate. Symbolic constants
 *! @[GL_SHORT], @[GL_INT], @[GL_FLOAT], or @[GL_DOUBLE] are accepted. The
 *! initial value is @[GL_FLOAT].
 *! 
 *! @param stride
 *! 
 *! Specifies the byte offset between consecutive texture coordinate sets.
 *! If @i{stride@} is 0, the array elements are understood to be tightly
 *! packed. The initial value is 0.
 *! 
 *! @param pointer
 *! 
 *! Specifies a pointer to the first coordinate of the first texture
 *! coordinate set in the array. The initial value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{size@} is not 1, 2, 3, or 4.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{stride@} is negative.
 *! 
 *! @note
 *! 
 *! @[glTexCoordPointer] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, @[glTexCoordPointer]
 *! updates the texture coordinate array state of the active client
 *! texture unit, specified with @[glClientActiveTexture].
 *! 
 *! Each texture coordinate array is initially disabled and isn't accessed
 *! when @[glArrayElement], @[glDrawElements], @[glDrawRangeElements],
 *! @[glDrawArrays], @[glMultiDrawArrays], or @[glMultiDrawElements] is
 *! called.
 *! 
 *! Execution of @[glTexCoordPointer] is not allowed between the execution
 *! of @[glBegin] and the corresponding execution of @[glEnd], but an
 *! error may or may not be generated. If no error is generated, the
 *! operation is undefined.
 *! 
 *! @[glTexCoordPointer] is typically implemented on the client side.
 *! 
 *! Texture coordinate array parameters are client-side state and are
 *! therefore not saved or restored by @[glPushAttrib] and @[glPopAttrib].
 *! Use @[glPushClientAttrib] and @[glPopClientAttrib] instead.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glBindBuffer], @[glClientActiveTexture],
 *! @[glColorPointer], @[glDisableClientState], @[glDrawArrays],
 *! @[glDrawElements], @[glDrawRangeElements], @[glEdgeFlagPointer],
 *! @[glEnableClientState], @[glFogCoordPointer], @[glIndexPointer],
 *! @[glInterleavedArrays], @[glMultiDrawArrays], @[glMultiDrawElements],
 *! @[glMultiTexCoord], @[glNormalPointer], @[glPopClientAttrib],
 *! @[glPushClientAttrib], @[glSecondaryColorPointer], @[glTexCoord],
 *! @[glVertexAttribPointer], @[glVertexPointer]
 *! 
 */


/*! @decl void glIndexPointer(int type, int stride, System.Memory pointer)
 *! 
 *! @[glIndexPointer] specifies the location and data format of an array
 *! of color indexes to use when rendering. @i{type@} specifies the data
 *! type of each color index and @i{stride@} specifies the byte stride
 *! from one color index to the next, allowing vertices and attributes to
 *! be packed into a single array or stored in separate arrays.
 *! 
 *! If a non-zero named buffer object is bound to the @[GL_ARRAY_BUFFER]
 *! target (see @[glBindBuffer]) while a color index array is specified,
 *! @i{pointer@} is treated as a byte offset into the buffer object's data
 *! store. Also, the buffer object binding (@[GL_ARRAY_BUFFER_BINDING]) is
 *! saved as color index vertex array client-side state
 *! (@[GL_INDEX_ARRAY_BUFFER_BINDING]).
 *! 
 *! When a color index array is specified, @i{type@}, @i{stride@}, and
 *! @i{pointer@} are saved as client-side state, in addition to the
 *! current vertex array buffer object binding.
 *! 
 *! To enable and disable the color index array, call
 *! @[glEnableClientState] and @[glDisableClientState] with the argument
 *! @[GL_INDEX_ARRAY]. If enabled, the color index array is used when
 *! @[glDrawArrays], @[glMultiDrawArrays], @[glDrawElements],
 *! @[glMultiDrawElements], @[glDrawRangeElements], or @[glArrayElement]
 *! is called.
 *! 
 *! @param type
 *! 
 *! Specifies the data type of each color index in the array. Symbolic
 *! constants @[GL_UNSIGNED_BYTE], @[GL_SHORT], @[GL_INT], @[GL_FLOAT],
 *! and @[GL_DOUBLE] are accepted. The initial value is @[GL_FLOAT].
 *! 
 *! @param stride
 *! 
 *! Specifies the byte offset between consecutive color indexes. If
 *! @i{stride@} is 0, the color indexes are understood to be tightly
 *! packed in the array. The initial value is 0.
 *! 
 *! @param pointer
 *! 
 *! Specifies a pointer to the first index in the array. The initial value
 *! is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{stride@} is negative.
 *! 
 *! @note
 *! 
 *! @[glIndexPointer] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Color indexes are not supported for interleaved vertex array formats
 *! (see @[glInterleavedArrays]).
 *! 
 *! The color index array is initially disabled and isn't accessed when
 *! @[glArrayElement], @[glDrawElements], @[glDrawRangeElements],
 *! @[glDrawArrays], @[glMultiDrawArrays], or @[glMultiDrawElements] is
 *! called.
 *! 
 *! Execution of @[glIndexPointer] is not allowed between @[glBegin] and
 *! the corresponding @[glEnd], but an error may or may not be generated.
 *! If an error is not generated, the operation is undefined.
 *! 
 *! @[glIndexPointer] is typically implemented on the client side.
 *! 
 *! Color index array parameters are client-side state and are therefore
 *! not saved or restored by @[glPushAttrib] and @[glPopAttrib]. Use
 *! @[glPushClientAttrib] and @[glPopClientAttrib] instead.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glBindBuffer], @[glColorPointer],
 *! @[glDisableClientState], @[glDrawArrays], @[glDrawElements],
 *! @[glDrawRangeElements], @[glEdgeFlagPointer], @[glEnableClientState],
 *! @[glFogCoordPointer], @[glIndex], @[glInterleavedArrays],
 *! @[glMultiDrawArrays], @[glMultiDrawElements], @[glNormalPointer],
 *! @[glPopClientAttrib], @[glPushClientAttrib],
 *! @[glSecondaryColorPointer], @[glTexCoordPointer],
 *! @[glVertexAttribPointer], @[glVertexPointer]
 *! 
 */


/*! @decl void glNormalPointer(int type, int stride, @
 *! System.Memory pointer)
 *! 
 *! @[glNormalPointer] specifies the location and data format of an array
 *! of normals to use when rendering. @i{type@} specifies the data type of
 *! each normal coordinate, and @i{stride@} specifies the byte stride from
 *! one normal to the next, allowing vertices and attributes to be packed
 *! into a single array or stored in separate arrays. (Single-array
 *! storage may be more efficient on some implementations; see
 *! @[glInterleavedArrays].)
 *! 
 *! If a non-zero named buffer object is bound to the @[GL_ARRAY_BUFFER]
 *! target (see @[glBindBuffer]) while a normal array is specified,
 *! @i{pointer@} is treated as a byte offset into the buffer object's data
 *! store. Also, the buffer object binding (@[GL_ARRAY_BUFFER_BINDING]) is
 *! saved as normal vertex array client-side state
 *! (@[GL_NORMAL_ARRAY_BUFFER_BINDING]).
 *! 
 *! When a normal array is specified, @i{type@}, @i{stride@}, and
 *! @i{pointer@} are saved as client-side state, in addition to the
 *! current vertex array buffer object binding.
 *! 
 *! To enable and disable the normal array, call @[glEnableClientState]
 *! and @[glDisableClientState] with the argument @[GL_NORMAL_ARRAY]. If
 *! enabled, the normal array is used when @[glDrawArrays],
 *! @[glMultiDrawArrays], @[glDrawElements], @[glMultiDrawElements],
 *! @[glDrawRangeElements], or @[glArrayElement] is called.
 *! 
 *! @param type
 *! 
 *! Specifies the data type of each coordinate in the array. Symbolic
 *! constants @[GL_BYTE], @[GL_SHORT], @[GL_INT], @[GL_FLOAT], and
 *! @[GL_DOUBLE] are accepted. The initial value is @[GL_FLOAT].
 *! 
 *! @param stride
 *! 
 *! Specifies the byte offset between consecutive normals. If @i{stride@}
 *! is 0, the normals are understood to be tightly packed in the array.
 *! The initial value is 0.
 *! 
 *! @param pointer
 *! 
 *! Specifies a pointer to the first coordinate of the first normal in the
 *! array. The initial value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{stride@} is negative.
 *! 
 *! @note
 *! 
 *! @[glNormalPointer] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! The normal array is initially disabled and isn't accessed when
 *! @[glArrayElement], @[glDrawElements], @[glDrawRangeElements],
 *! @[glDrawArrays], @[glMultiDrawArrays], or @[glMultiDrawElements] is
 *! called.
 *! 
 *! Execution of @[glNormalPointer] is not allowed between @[glBegin] and
 *! the corresponding @[glEnd], but an error may or may not be generated.
 *! If an error is not generated, the operation is undefined.
 *! 
 *! @[glNormalPointer] is typically implemented on the client side.
 *! 
 *! Normal array parameters are client-side state and are therefore not
 *! saved or restored by @[glPushAttrib] and @[glPopAttrib]. Use
 *! @[glPushClientAttrib] and @[glPopClientAttrib] instead.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glBindBuffer], @[glColorPointer],
 *! @[glDisableClientState], @[glDrawArrays], @[glDrawElements],
 *! @[glDrawRangeElements], @[glEdgeFlagPointer], @[glEnableClientState],
 *! @[glFogCoordPointer], @[glIndexPointer], @[glInterleavedArrays],
 *! @[glMultiDrawArrays], @[glMultiDrawElements], @[glNormal],
 *! @[glPopClientAttrib], @[glPushClientAttrib],
 *! @[glSecondaryColorPointer], @[glTexCoordPointer],
 *! @[glVertexAttribPointer], @[glVertexPointer]
 *! 
 */


/*! @decl void glColorPointer(int size, int type, int stride, @
 *! System.Memory pointer)
 *! 
 *! @[glColorPointer] specifies the location and data format of an array
 *! of color components to use when rendering. @i{size@} specifies the
 *! number of components per color, and must be 3 or 4. @i{type@}
 *! specifies the data type of each color component, and @i{stride@}
 *! specifies the byte stride from one color to the next, allowing
 *! vertices and attributes to be packed into a single array or stored in
 *! separate arrays. (Single-array storage may be more efficient on some
 *! implementations; see @[glInterleavedArrays].)
 *! 
 *! If a non-zero named buffer object is bound to the @[GL_ARRAY_BUFFER]
 *! target (see @[glBindBuffer]) while a color array is specified,
 *! @i{pointer@} is treated as a byte offset into the buffer object's data
 *! store. Also, the buffer object binding (@[GL_ARRAY_BUFFER_BINDING]) is
 *! saved as color vertex array client-side state
 *! (@[GL_COLOR_ARRAY_BUFFER_BINDING]).
 *! 
 *! When a color array is specified, @i{size@}, @i{type@}, @i{stride@},
 *! and @i{pointer@} are saved as client-side state, in addition to the
 *! current vertex array buffer object binding.
 *! 
 *! To enable and disable the color array, call @[glEnableClientState] and
 *! @[glDisableClientState] with the argument @[GL_COLOR_ARRAY]. If
 *! enabled, the color array is used when @[glDrawArrays],
 *! @[glMultiDrawArrays], @[glDrawElements], @[glMultiDrawElements],
 *! @[glDrawRangeElements], or @[glArrayElement] is called.
 *! 
 *! @param size
 *! 
 *! Specifies the number of components per color. Must be 3 or 4. The
 *! initial value is 4.
 *! 
 *! @param type
 *! 
 *! Specifies the data type of each color component in the array. Symbolic
 *! constants @[GL_BYTE], @[GL_UNSIGNED_BYTE], @[GL_SHORT],
 *! @[GL_UNSIGNED_SHORT], @[GL_INT], @[GL_UNSIGNED_INT], @[GL_FLOAT], and
 *! @[GL_DOUBLE] are accepted. The initial value is @[GL_FLOAT].
 *! 
 *! @param stride
 *! 
 *! Specifies the byte offset between consecutive colors. If @i{stride@}
 *! is 0, the colors are understood to be tightly packed in the array. The
 *! initial value is 0.
 *! 
 *! @param pointer
 *! 
 *! Specifies a pointer to the first component of the first color element
 *! in the array. The initial value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{size@} is not 3 or 4.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{stride@} is negative.
 *! 
 *! @note
 *! 
 *! @[glColorPointer] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! The color array is initially disabled and isn't accessed when
 *! @[glArrayElement], @[glDrawElements], @[glDrawRangeElements],
 *! @[glDrawArrays], @[glMultiDrawArrays], or @[glMultiDrawElements] is
 *! called.
 *! 
 *! Execution of @[glColorPointer] is not allowed between the execution of
 *! @[glBegin] and the corresponding execution of @[glEnd], but an error
 *! may or may not be generated. If no error is generated, the operation
 *! is undefined.
 *! 
 *! @[glColorPointer] is typically implemented on the client side.
 *! 
 *! Color array parameters are client-side state and are therefore not
 *! saved or restored by @[glPushAttrib] and @[glPopAttrib]. Use
 *! @[glPushClientAttrib] and @[glPopClientAttrib] instead.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glBindBuffer], @[glColor],
 *! @[glDisableClientState], @[glDrawArrays], @[glDrawElements],
 *! @[glDrawRangeElements], @[glEdgeFlagPointer], @[glEnableClientState],
 *! @[glFogCoordPointer], @[glIndexPointer], @[glInterleavedArrays],
 *! @[glMultiDrawArrays], @[glMultiDrawElements], @[glNormalPointer],
 *! @[glPopClientAttrib], @[glPushClientAttrib],
 *! @[glSecondaryColorPointer], @[glTexCoordPointer],
 *! @[glVertexAttribPointer], @[glVertexPointer]
 *! 
 */


/*! @decl void glEdgeFlagPointer(int stride, System.Memory pointer)
 *! 
 *! @[glEdgeFlagPointer] specifies the location and data format of an
 *! array of boolean edge flags to use when rendering. @i{stride@}
 *! specifies the byte stride from one edge flag to the next, allowing
 *! vertices and attributes to be packed into a single array or stored in
 *! separate arrays.
 *! 
 *! If a non-zero named buffer object is bound to the @[GL_ARRAY_BUFFER]
 *! target (see @[glBindBuffer]) while an edge flag array is specified,
 *! @i{pointer@} is treated as a byte offset into the buffer object's data
 *! store. Also, the buffer object binding (@[GL_ARRAY_BUFFER_BINDING]) is
 *! saved as edge flag vertex array client-side state
 *! (@[GL_EDGE_FLAG_ARRAY_BUFFER_BINDING]).
 *! 
 *! When an edge flag array is specified, @i{stride@} and @i{pointer@} are
 *! saved as client-side state, in addition to the current vertex array
 *! buffer object binding.
 *! 
 *! To enable and disable the edge flag array, call @[glEnableClientState]
 *! and @[glDisableClientState] with the argument @[GL_EDGE_FLAG_ARRAY].
 *! If enabled, the edge flag array is used when @[glDrawArrays],
 *! @[glMultiDrawArrays], @[glDrawElements], @[glMultiDrawElements],
 *! @[glDrawRangeElements], or @[glArrayElement] is called.
 *! 
 *! @param stride
 *! 
 *! Specifies the byte offset between consecutive edge flags. If
 *! @i{stride@} is 0, the edge flags are understood to be tightly packed
 *! in the array. The initial value is 0.
 *! 
 *! @param pointer
 *! 
 *! Specifies a pointer to the first edge flag in the array. The initial
 *! value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{stride@} is negative.
 *! 
 *! @note
 *! 
 *! @[glEdgeFlagPointer] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Edge flags are not supported for interleaved vertex array formats (see
 *! @[glInterleavedArrays]).
 *! 
 *! The edge flag array is initially disabled and isn't accessed when
 *! @[glArrayElement], @[glDrawElements], @[glDrawRangeElements],
 *! @[glDrawArrays], @[glMultiDrawArrays], or @[glMultiDrawElements] is
 *! called.
 *! 
 *! Execution of @[glEdgeFlagPointer] is not allowed between the execution
 *! of @[glBegin] and the corresponding execution of @[glEnd], but an
 *! error may or may not be generated. If no error is generated, the
 *! operation is undefined.
 *! 
 *! @[glEdgeFlagPointer] is typically implemented on the client side.
 *! 
 *! Edge flag array parameters are client-side state and are therefore not
 *! saved or restored by @[glPushAttrib] and @[glPopAttrib]. Use
 *! @[glPushClientAttrib] and @[glPopClientAttrib] instead.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glBindBuffer], @[glColorPointer],
 *! @[glDisableClientState], @[glDrawArrays], @[glDrawElements],
 *! @[glDrawRangeElements], @[glEdgeFlag], @[glEnableClientState],
 *! @[glFogCoordPointer], @[glIndexPointer], @[glInterleavedArrays],
 *! @[glMultiDrawArrays], @[glMultiDrawElements], @[glNormalPointer],
 *! @[glPopClientAttrib], @[glPushClientAttrib],
 *! @[glSecondaryColorPointer], @[glTexCoordPointer],
 *! @[glVertexAttribPointer], @[glVertexPointer]
 *! 
 */


/*! @decl void glHint(int target, int mode)
 *! 
 *! Certain aspects of GL behavior, when there is room for interpretation,
 *! can be controlled with hints. A hint is specified with two arguments.
 *! @i{target@} is a symbolic constant indicating the behavior to be
 *! controlled, and @i{mode@} is another symbolic constant indicating the
 *! desired behavior. The initial value for each @i{target@} is
 *! @[GL_DONT_CARE]. @i{mode@} can be one of the following:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_FASTEST</ref></c>
 *! <c><p></p><p>The most efficient option should be chosen. </p></c></r>
 *! <r><c><ref>GL_NICEST</ref></c>
 *! <c><p></p><p>The most correct, or highest quality, option should be
 *! chosen. </p></c></r>
 *! <r><c><ref>GL_DONT_CARE</ref></c>
 *! <c><p></p><p>No preference. </p></c></r>
 *! </matrix>@}Though the implementation aspects that can be hinted are
 *! well defined, the interpretation of the hints depends on the
 *! implementation. The hint aspects that can be specified with
 *! @i{target@}, along with suggested semantics, are as follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_FOG_HINT</ref></c>
 *! <c><p></p><p>Indicates the accuracy of fog calculation. If per-pixel
 *! fog calculation is not efficiently supported by the GL implementation,
 *! hinting <ref>GL_DONT_CARE</ref> or <ref>GL_FASTEST</ref> can result in
 *! per-vertex calculation of fog effects. </p></c></r>
 *! <r><c><ref>GL_FRAGMENT_SHADER_DERIVATIVE_HINT</ref></c>
 *! <c><p></p><p>Indicates the accuracy of the derivative calculation for
 *! the GL shading language fragment processing built-in functions: <ref
 *! >dFdx</ref>, <ref>dFdy</ref>, and <ref>fwidth</ref>. </p></c></r>
 *! <r><c><ref>GL_GENERATE_MIPMAP_HINT</ref></c>
 *! <c><p></p><p>Indicates the quality of filtering when generating mipmap
 *! images. </p></c></r>
 *! <r><c><ref>GL_LINE_SMOOTH_HINT</ref></c>
 *! <c><p></p><p>Indicates the sampling quality of antialiased lines. If a
 *! larger filter function is applied, hinting <ref>GL_NICEST</ref> can
 *! result in more pixel fragments being generated during rasterization.
 *! </p></c></r>
 *! <r><c><ref>GL_PERSPECTIVE_CORRECTION_HINT</ref></c>
 *! <c><p></p><p>Indicates the quality of color, texture coordinate, and
 *! fog coordinate interpolation. If perspective-corrected parameter
 *! interpolation is not efficiently supported by the GL implementation,
 *! hinting <ref>GL_DONT_CARE</ref> or <ref>GL_FASTEST</ref> can result in
 *! simple linear interpolation of colors and/or texture coordinates. </p
 *! ></c></r>
 *! <r><c><ref>GL_POINT_SMOOTH_HINT</ref></c>
 *! <c><p></p><p>Indicates the sampling quality of antialiased points. If
 *! a larger filter function is applied, hinting <ref>GL_NICEST</ref> can
 *! result in more pixel fragments being generated during rasterization.
 *! </p></c></r>
 *! <r><c><ref>GL_POLYGON_SMOOTH_HINT</ref></c>
 *! <c><p></p><p>Indicates the sampling quality of antialiased polygons.
 *! Hinting <ref>GL_NICEST</ref> can result in more pixel fragments being
 *! generated during rasterization, if a larger filter function is
 *! applied. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COMPRESSION_HINT</ref></c>
 *! <c><p></p><p>Indicates the quality and performance of the compressing
 *! texture images. Hinting <ref>GL_FASTEST</ref> indicates that texture
 *! images should be compressed as quickly as possible, while <ref
 *! >GL_NICEST</ref> indicates that texture images should be compressed
 *! with as little image quality loss as possible. <ref>GL_NICEST</ref>
 *! should be selected if the texture is to be retrieved by <ref
 *! >glGetCompressedTexImage</ref> for reuse. </p></c></r>
 *! </matrix>@}
 *! 
 *! @param target
 *! 
 *! Specifies a symbolic constant indicating the behavior to be
 *! controlled. @[GL_FOG_HINT], @[GL_GENERATE_MIPMAP_HINT],
 *! @[GL_LINE_SMOOTH_HINT], @[GL_PERSPECTIVE_CORRECTION_HINT],
 *! @[GL_POINT_SMOOTH_HINT], @[GL_POLYGON_SMOOTH_HINT],
 *! @[GL_TEXTURE_COMPRESSION_HINT], and
 *! @[GL_FRAGMENT_SHADER_DERIVATIVE_HINT] are accepted.
 *! 
 *! @param mode
 *! 
 *! Specifies a symbolic constant indicating the desired behavior.
 *! @[GL_FASTEST], @[GL_NICEST], and @[GL_DONT_CARE] are accepted.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if either @i{target@} or @i{mode@} is
 *! not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glHint] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! The interpretation of hints depends on the implementation. Some
 *! implementations ignore @[glHint] settings.
 *! 
 *! @[GL_TEXTURE_COMPRESSION_HINT] is available only if the GL version is
 *! 1.3 or greater.
 *! 
 *! @[GL_GENERATE_MIPMAP_HINT] is available only if the GL version is 1.4
 *! or greater.
 *! 
 *! @[GL_FRAGMENT_SHADER_DERIVATIVE_HINT] is available only if the GL
 *! version is 2.0 or greater.
 *! 
 */


/*! @decl void glIndex(float|int c)
 *! 
 *! @[glIndex] updates the current (single-valued) color index. It takes
 *! one argument, the new value for the current color index.
 *! 
 *! The current index is stored as a floating-point value. Integer values
 *! are converted directly to floating-point values, with no special
 *! mapping. The initial value is 1.
 *! 
 *! Index values outside the representable range of the color index buffer
 *! are not clamped. However, before an index is dithered (if enabled) and
 *! written to the frame buffer, it is converted to fixed-point format.
 *! Any bits in the integer portion of the resulting fixed-point value
 *! that do not correspond to bits in the frame buffer are masked out.
 *! 
 *! @param c
 *! 
 *! Specifies the new value for the current color index.
 *! 
 *! @param c
 *! 
 *! Specifies a pointer to a one-element array that contains the new value
 *! for the current color index.
 *! 
 *! @note
 *! 
 *! @[glIndex] and @[glIndex] are available only if the GL version is 1.1
 *! or greater.
 *! 
 *! The current index can be updated at any time. In particular,
 *! @[glIndex] can be called between a call to @[glBegin] and the
 *! corresponding call to @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glColor], @[glIndexPointer]
 *! 
 */


/*! @decl void glIndexMask(int mask)
 *! 
 *! @[glIndexMask] controls the writing of individual bits in the color
 *! index buffers. The least significant @i{n@} bits of @i{mask@}, where
 *! @i{n@} is the number of bits in a color index buffer, specify a mask.
 *! Where a 1 (one) appears in the mask, it's possible to write to the
 *! corresponding bit in the color index buffer (or buffers). Where a 0
 *! (zero) appears, the corresponding bit is write-protected.
 *! 
 *! This mask is used only in color index mode, and it affects only the
 *! buffers currently selected for writing (see @[glDrawBuffer]).
 *! Initially, all bits are enabled for writing.
 *! 
 *! @param mask
 *! 
 *! Specifies a bit mask to enable and disable the writing of individual
 *! bits in the color index buffers. Initially, the mask is all 1's.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glIndexMask] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glColorMask], @[glDepthMask], @[glDrawBuffer], @[glIndex],
 *! @[glIndexPointer], @[glStencilMask]
 *! 
 */


/*! @decl int glIsEnabled(int cap)
 *! 
 *! @[glIsEnabled] returns @[GL_TRUE] if @i{cap@} is an enabled capability
 *! and returns @[GL_FALSE] otherwise. Initially all capabilities except
 *! @[GL_DITHER] are disabled; @[GL_DITHER] is initially enabled.
 *! 
 *! The following capabilities are accepted for @i{cap@}:
 *! 
 *! @xml{<matrix><r><c><b>Constant </b></c><c><b>See </b></c></r><r><c
 *! ><ref>GL_ALPHA_TEST</ref></c><c><ref>glAlphaFunc</ref></c></r><r><c
 *! ><ref>GL_AUTO_NORMAL</ref></c><c><ref>glEvalCoord</ref></c></r><r><c
 *! ><ref>GL_BLEND</ref></c><c><ref>glBlendFunc</ref>, <ref>glLogicOp</ref
 *! ></c></r><r><c><tt>GL_CLIP_PLANE</tt><i>i</i></c><c><ref
 *! >glClipPlane</ref></c></r><r><c><ref>GL_COLOR_ARRAY</ref></c><c><ref
 *! >glColorPointer</ref></c></r><r><c><ref>GL_COLOR_LOGIC_OP</ref></c><c
 *! ><ref>glLogicOp</ref></c></r><r><c><ref>GL_COLOR_MATERIAL</ref></c><c
 *! ><ref>glColorMaterial</ref></c></r><r><c><ref>GL_COLOR_SUM</ref></c><c
 *! ><ref>glSecondaryColor</ref></c></r><r><c><ref>GL_COLOR_TABLE</ref></c
 *! ><c><ref>glColorTable</ref></c></r><r><c><ref>GL_CONVOLUTION_1D</ref
 *! ></c><c><ref>glConvolutionFilter1D</ref></c></r><r><c><ref
 *! >GL_CONVOLUTION_2D</ref></c><c><ref>glConvolutionFilter2D</ref></c></r
 *! ><r><c><ref>GL_CULL_FACE</ref></c><c><ref>glCullFace</ref></c></r><r
 *! ><c><ref>GL_DEPTH_TEST</ref></c><c><ref>glDepthFunc</ref>, <ref
 *! >glDepthRange</ref></c></r><r><c><ref>GL_DITHER</ref></c><c><ref
 *! >glEnable</ref></c></r><r><c><ref>GL_EDGE_FLAG_ARRAY</ref></c><c><ref
 *! >glEdgeFlagPointer</ref></c></r><r><c><ref>GL_FOG</ref></c><c><ref
 *! >glFog</ref></c></r><r><c><ref>GL_FOG_COORD_ARRAY</ref></c><c><ref
 *! >glFogCoordPointer</ref></c></r><r><c><ref>GL_HISTOGRAM</ref></c><c
 *! ><ref>glHistogram</ref></c></r><r><c><ref>GL_INDEX_ARRAY</ref></c><c
 *! ><ref>glIndexPointer</ref></c></r><r><c><ref>GL_INDEX_LOGIC_OP</ref
 *! ></c><c><ref>glLogicOp</ref></c></r><r><c><tt>GL_LIGHT</tt><i>i</i></c
 *! ><c><ref>glLightModel</ref>, <ref>glLight</ref></c></r><r><c><ref
 *! >GL_LIGHTING</ref></c><c><ref>glMaterial</ref>, <ref>glLightModel</ref
 *! >, <ref>glLight</ref></c></r><r><c><ref>GL_LINE_SMOOTH</ref></c><c
 *! ><ref>glLineWidth</ref></c></r><r><c><ref>GL_LINE_STIPPLE</ref></c><c
 *! ><ref>glLineStipple</ref></c></r><r><c><ref>GL_MAP1_COLOR_4</ref></c
 *! ><c><ref>glMap1</ref></c></r><r><c><ref>GL_MAP1_INDEX</ref></c><c><ref
 *! >glMap1</ref></c></r><r><c><ref>GL_MAP1_NORMAL</ref></c><c><ref
 *! >glMap1</ref></c></r><r><c><ref>GL_MAP1_TEXTURE_COORD_1</ref></c><c
 *! ><ref>glMap1</ref></c></r><r><c><ref>GL_MAP1_TEXTURE_COORD_2</ref></c
 *! ><c><ref>glMap1</ref></c></r><r><c><ref>GL_MAP1_TEXTURE_COORD_3</ref
 *! ></c><c><ref>glMap1</ref></c></r><r><c><ref
 *! >GL_MAP1_TEXTURE_COORD_4</ref></c><c><ref>glMap1</ref></c></r><r><c
 *! ><ref>GL_MAP2_COLOR_4</ref></c><c><ref>glMap2</ref></c></r><r><c><ref
 *! >GL_MAP2_INDEX</ref></c><c><ref>glMap2</ref></c></r><r><c><ref
 *! >GL_MAP2_NORMAL</ref></c><c><ref>glMap2</ref></c></r><r><c><ref
 *! >GL_MAP2_TEXTURE_COORD_1</ref></c><c><ref>glMap2</ref></c></r><r><c
 *! ><ref>GL_MAP2_TEXTURE_COORD_2</ref></c><c><ref>glMap2</ref></c></r><r
 *! ><c><ref>GL_MAP2_TEXTURE_COORD_3</ref></c><c><ref>glMap2</ref></c></r
 *! ><r><c><ref>GL_MAP2_TEXTURE_COORD_4</ref></c><c><ref>glMap2</ref></c
 *! ></r><r><c><ref>GL_MAP2_VERTEX_3</ref></c><c><ref>glMap2</ref></c></r
 *! ><r><c><ref>GL_MAP2_VERTEX_4</ref></c><c><ref>glMap2</ref></c></r><r
 *! ><c><ref>GL_MINMAX</ref></c><c><ref>glMinmax</ref></c></r><r><c><ref
 *! >GL_MULTISAMPLE</ref></c><c><ref>glSampleCoverage</ref></c></r><r><c
 *! ><ref>GL_NORMAL_ARRAY</ref></c><c><ref>glNormalPointer</ref></c></r><r
 *! ><c><ref>GL_NORMALIZE</ref></c><c><ref>glNormal</ref></c></r><r><c
 *! ><ref>GL_POINT_SMOOTH</ref></c><c><ref>glPointSize</ref></c></r><r><c
 *! ><ref>GL_POINT_SPRITE</ref></c><c><ref>glEnable</ref></c></r><r><c
 *! ><ref>GL_POLYGON_SMOOTH</ref></c><c><ref>glPolygonMode</ref></c></r><r
 *! ><c><ref>GL_POLYGON_OFFSET_FILL</ref></c><c><ref>glPolygonOffset</ref
 *! ></c></r><r><c><ref>GL_POLYGON_OFFSET_LINE</ref></c><c><ref
 *! >glPolygonOffset</ref></c></r><r><c><ref>GL_POLYGON_OFFSET_POINT</ref
 *! ></c><c><ref>glPolygonOffset</ref></c></r><r><c><ref
 *! >GL_POLYGON_STIPPLE</ref></c><c><ref>glPolygonStipple</ref></c></r><r
 *! ><c><ref>GL_POST_COLOR_MATRIX_COLOR_TABLE</ref></c><c><ref
 *! >glColorTable</ref></c></r><r><c><ref
 *! >GL_POST_CONVOLUTION_COLOR_TABLE</ref></c><c><ref>glColorTable</ref
 *! ></c></r><r><c><ref>GL_RESCALE_NORMAL</ref></c><c><ref>glNormal</ref
 *! ></c></r><r><c><ref>GL_SAMPLE_ALPHA_TO_COVERAGE</ref></c><c><ref
 *! >glSampleCoverage</ref></c></r><r><c><ref>GL_SAMPLE_ALPHA_TO_ONE</ref
 *! ></c><c><ref>glSampleCoverage</ref></c></r><r><c><ref
 *! >GL_SAMPLE_COVERAGE</ref></c><c><ref>glSampleCoverage</ref></c></r><r
 *! ><c><ref>GL_SCISSOR_TEST</ref></c><c><ref>glScissor</ref></c></r><r><c
 *! ><ref>GL_SECONDARY_COLOR_ARRAY</ref></c><c><ref
 *! >glSecondaryColorPointer</ref></c></r><r><c><ref>GL_SEPARABLE_2D</ref
 *! ></c><c><ref>glSeparableFilter2D</ref></c></r><r><c><ref
 *! >GL_STENCIL_TEST</ref></c><c><ref>glStencilFunc</ref>, <ref
 *! >glStencilOp</ref></c></r><r><c><ref>GL_TEXTURE_1D</ref></c><c><ref
 *! >glTexImage1D</ref></c></r><r><c><ref>GL_TEXTURE_2D</ref></c><c><ref
 *! >glTexImage2D</ref></c></r><r><c><ref>GL_TEXTURE_3D</ref></c><c><ref
 *! >glTexImage3D</ref></c></r><r><c><ref>GL_TEXTURE_COORD_ARRAY</ref></c
 *! ><c><ref>glTexCoordPointer</ref></c></r><r><c><ref
 *! >GL_TEXTURE_CUBE_MAP</ref></c><c><ref>glTexImage2D</ref></c></r><r><c
 *! ><ref>GL_TEXTURE_GEN_Q</ref></c><c><ref>glTexGen</ref></c></r><r><c
 *! ><ref>GL_TEXTURE_GEN_R</ref></c><c><ref>glTexGen</ref></c></r><r><c
 *! ><ref>GL_TEXTURE_GEN_S</ref></c><c><ref>glTexGen</ref></c></r><r><c
 *! ><ref>GL_TEXTURE_GEN_T</ref></c><c><ref>glTexGen</ref></c></r><r><c
 *! ><ref>GL_VERTEX_ARRAY</ref></c><c><ref>glVertexPointer</ref></c></r><r
 *! ><c><ref>GL_VERTEX_PROGRAM_POINT_SIZE</ref></c><c><ref>glEnable</ref
 *! ></c></r><r><c><ref>GL_VERTEX_PROGRAM_TWO_SIDE</ref></c><c><ref
 *! >glEnable</ref></c></r></matrix>@}
 *! 
 *! @param cap
 *! 
 *! Specifies a symbolic constant indicating a GL capability.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{cap@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glIsEnabled] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! If an error is generated, @[glIsEnabled] returns 0.
 *! 
 *! @[GL_COLOR_LOGIC_OP], @[GL_COLOR_ARRAY], @[GL_EDGE_FLAG_ARRAY],
 *! @[GL_INDEX_ARRAY], @[GL_INDEX_LOGIC_OP], @[GL_NORMAL_ARRAY],
 *! @[GL_POLYGON_OFFSET_FILL], @[GL_POLYGON_OFFSET_LINE],
 *! @[GL_POLYGON_OFFSET_POINT], @[GL_TEXTURE_COORD_ARRAY], and
 *! @[GL_VERTEX_ARRAY] are available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @[GL_RESCALE_NORMAL], and @[GL_TEXTURE_3D] are available only if the
 *! GL version is 1.2 or greater.
 *! 
 *! @[GL_MULTISAMPLE], @[GL_SAMPLE_ALPHA_TO_COVERAGE],
 *! @[GL_SAMPLE_ALPHA_TO_ONE], @[GL_SAMPLE_COVERAGE],
 *! @[GL_TEXTURE_CUBE_MAP] are available only if the GL version is 1.3 or
 *! greater.
 *! 
 *! @[GL_FOG_COORD_ARRAY] and @[GL_SECONDARY_COLOR_ARRAY] are available
 *! only if the GL version is 1.4 or greater.
 *! 
 *! @[GL_POINT_SPRITE], @[GL_VERTEX_PROGRAM_POINT_SIZE], and
 *! @[GL_VERTEX_PROGRAM_TWO_SIDE] are available only if the GL version is
 *! 2.0 or greater.
 *! 
 *! @[GL_COLOR_TABLE], @[GL_CONVOLUTION_1D], @[GL_CONVOLUTION_2D],
 *! @[GL_HISTOGRAM], @[GL_MINMAX], @[GL_POST_COLOR_MATRIX_COLOR_TABLE],
 *! @[GL_POST_CONVOLUTION_COLOR_TABLE], and @[GL_SEPARABLE_2D] are
 *! available only if @tt{ARB_imaging@} is returned when @[glGet] is
 *! called with @[GL_EXTENSIONS].
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, the following
 *! parameters return the associated value for the active texture unit:
 *! @[GL_TEXTURE_1D], @[GL_TEXTURE_2D], @[GL_TEXTURE_3D],
 *! @[GL_TEXTURE_CUBE_MAP], @[GL_TEXTURE_GEN_S], @[GL_TEXTURE_GEN_T],
 *! @[GL_TEXTURE_GEN_R], @[GL_TEXTURE_GEN_Q], @[GL_TEXTURE_MATRIX], and
 *! @[GL_TEXTURE_STACK_DEPTH]. Likewise, the following parameters return
 *! the associated value for the active client texture unit:
 *! @[GL_TEXTURE_COORD_ARRAY], @[GL_TEXTURE_COORD_ARRAY_SIZE],
 *! @[GL_TEXTURE_COORD_ARRAY_STRIDE], @[GL_TEXTURE_COORD_ARRAY_TYPE].
 *! 
 *! @seealso
 *! 
 *! @[glEnable], @[glEnableClientState], @[glGet]
 *! 
 */


/*! @decl int glIsList(int list)
 *! 
 *! @[glIsList] returns @[GL_TRUE] if @i{list@} is the name of a display
 *! list and returns @[GL_FALSE] if it is not, or if an error occurs.
 *! 
 *! A name returned by @[glGenLists], but not yet associated with a
 *! display list by calling @[glNewList], is not the name of a display
 *! list.
 *! 
 *! @param list
 *! 
 *! Specifies a potential display list name.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glIsList] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glCallList], @[glCallLists], @[glDeleteLists], @[glGenLists],
 *! @[glNewList]
 *! 
 */


/*! @decl int glIsTexture(int texture)
 *! 
 *! @[glIsTexture] returns @[GL_TRUE] if @i{texture@} is currently the
 *! name of a texture. If @i{texture@} is zero, or is a non-zero value
 *! that is not currently the name of a texture, or if an error occurs,
 *! @[glIsTexture] returns @[GL_FALSE].
 *! 
 *! A name returned by @[glGenTextures], but not yet associated with a
 *! texture by calling @[glBindTexture], is not the name of a texture.
 *! 
 *! @param texture
 *! 
 *! Specifies a value that may be the name of a texture.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glIsTexture] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glIsTexture] is available only if the GL version is 1.1 or greater.
 *! 
 *! @seealso
 *! 
 *! @[glBindTexture], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glDeleteTextures], @[glGenTextures], @[glGet], @[glGetTexParameter],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexImage3D], @[glTexParameter]
 *! 
 */


/*! @decl void glLight(int light, int pname, @
 *! float|int|array(float|int) param)
 *! 
 *! @[glLight] sets the values of individual light source parameters.
 *! @i{light@} names the light and is a symbolic name of the form
 *! @tt{GL_LIGHT@}@i{i@}, where i ranges from 0 to the value of
 *! @[GL_MAX_LIGHTS] - 1. @i{pname@} specifies one of ten light source
 *! parameters, again by symbolic name. @i{params@} is either a single
 *! value or an array that contains the new values.
 *! 
 *! To enable and disable lighting calculation, call @[glEnable] and
 *! @[glDisable] with argument @[GL_LIGHTING]. Lighting is initially
 *! disabled. When it is enabled, light sources that are enabled
 *! contribute to the lighting calculation. Light source @i{i@} is enabled
 *! and disabled using @[glEnable] and @[glDisable] with argument
 *! @tt{GL_LIGHT@}@i{i@}.
 *! 
 *! The ten light parameters are as follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_AMBIENT</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify the ambient RGBA intensity of the light. Integer values
 *! are mapped linearly such that the most positive representable value
 *! maps to 1.0, and the most negative representable value maps to -1.0.
 *! Floating-point values are mapped directly. Neither integer nor
 *! floating-point values are clamped. The initial ambient light intensity
 *! is (0, 0, 0, 1). </p></c></r>
 *! <r><c><ref>GL_DIFFUSE</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify the diffuse RGBA intensity of the light. Integer values
 *! are mapped linearly such that the most positive representable value
 *! maps to 1.0, and the most negative representable value maps to -1.0.
 *! Floating-point values are mapped directly. Neither integer nor
 *! floating-point values are clamped. The initial value for <ref
 *! >GL_LIGHT0</ref> is (1, 1, 1, 1); for other lights, the initial value
 *! is (0, 0, 0, 1). </p></c></r>
 *! <r><c><ref>GL_SPECULAR</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify the specular RGBA intensity of the light. Integer values
 *! are mapped linearly such that the most positive representable value
 *! maps to 1.0, and the most negative representable value maps to -1.0.
 *! Floating-point values are mapped directly. Neither integer nor
 *! floating-point values are clamped. The initial value for <ref
 *! >GL_LIGHT0</ref> is (1, 1, 1, 1); for other lights, the initial value
 *! is (0, 0, 0, 1). </p></c></r>
 *! <r><c><ref>GL_POSITION</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify the position of the light in homogeneous object
 *! coordinates. Both integer and floating-point values are mapped
 *! directly. Neither integer nor floating-point values are clamped. </p
 *! ><p>The position is transformed by the modelview matrix when <ref
 *! >glLight</ref> is called (just as if it were a point), and it is
 *! stored in eye coordinates. If the <i>w</i> component of the position
 *! is 0, the light is treated as a directional source. Diffuse and
 *! specular lighting calculations take the light's direction, but not its
 *! actual position, into account, and attenuation is disabled. Otherwise,
 *! diffuse and specular lighting calculations are based on the actual
 *! location of the light in eye coordinates, and attenuation is enabled.
 *! The initial position is (0, 0, 1, 0); thus, the initial light source
 *! is directional, parallel to, and in the direction of the -<i>z</i>
 *! axis. </p></c></r>
 *! <r><c><ref>GL_SPOT_DIRECTION</ref></c>
 *! <c><p><i>params</i> contains three integer or floating-point values
 *! that specify the direction of the light in homogeneous object
 *! coordinates. Both integer and floating-point values are mapped
 *! directly. Neither integer nor floating-point values are clamped. </p
 *! ><p>The spot direction is transformed by the upper 3x3 of the
 *! modelview matrix when <ref>glLight</ref> is called, and it is stored
 *! in eye coordinates. It is significant only when <ref
 *! >GL_SPOT_CUTOFF</ref> is not 180, which it is initially. The initial
 *! direction is (0, 0, -1). </p></c></r>
 *! <r><c><ref>GL_SPOT_EXPONENT</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies the intensity distribution of the light. Integer and
 *! floating-point values are mapped directly. Only values in the range
 *! [0, 128] are accepted. </p><p>Effective light intensity is attenuated
 *! by the cosine of the angle between the direction of the light and the
 *! direction from the light to the vertex being lighted, raised to the
 *! power of the spot exponent. Thus, higher spot exponents result in a
 *! more focused light source, regardless of the spot cutoff angle (see
 *! <ref>GL_SPOT_CUTOFF</ref>, next paragraph). The initial spot exponent
 *! is 0, resulting in uniform light distribution. </p></c></r>
 *! <r><c><ref>GL_SPOT_CUTOFF</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies the maximum spread angle of a light source. Integer and
 *! floating-point values are mapped directly. Only values in the range
 *! [0, 90] and the special value 180 are accepted. If the angle between
 *! the direction of the light and the direction from the light to the
 *! vertex being lighted is greater than the spot cutoff angle, the light
 *! is completely masked. Otherwise, its intensity is controlled by the
 *! spot exponent and the attenuation factors. The initial spot cutoff is
 *! 180, resulting in uniform light distribution. </p></c></r>
 *! <r><c><ref>GL_CONSTANT_ATTENUATION</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_LINEAR_ATTENUATION</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_QUADRATIC_ATTENUATION</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies one of the three light attenuation factors. Integer and
 *! floating-point values are mapped directly. Only nonnegative values are
 *! accepted. If the light is positional, rather than directional, its
 *! intensity is attenuated by the reciprocal of the sum of the constant
 *! factor, the linear factor times the distance between the light and the
 *! vertex being lighted, and the quadratic factor times the square of the
 *! same distance. The initial attenuation factors are (1, 0, 0),
 *! resulting in no attenuation. </p></c></r>
 *! </matrix>@}
 *! 
 *! @param light
 *! 
 *! Specifies a light. The number of lights depends on the implementation,
 *! but at least eight lights are supported. They are identified by
 *! symbolic names of the form @tt{GL_LIGHT@}@i{i@}, where i ranges from 0
 *! to the value of @[GL_MAX_LIGHTS] - 1.
 *! 
 *! @param pname
 *! 
 *! Specifies a single-valued light source parameter for @i{light@}.
 *! @[GL_SPOT_EXPONENT], @[GL_SPOT_CUTOFF], @[GL_CONSTANT_ATTENUATION],
 *! @[GL_LINEAR_ATTENUATION], and @[GL_QUADRATIC_ATTENUATION] are
 *! accepted.
 *! 
 *! @param param
 *! 
 *! Specifies the value that parameter @i{pname@} of light source
 *! @i{light@} will be set to.
 *! 
 *! @param light
 *! 
 *! Specifies a light. The number of lights depends on the implementation,
 *! but at least eight lights are supported. They are identified by
 *! symbolic names of the form @tt{GL_LIGHT@}@i{i@}, where i ranges from 0
 *! to the value of @[GL_MAX_LIGHTS] - 1.
 *! 
 *! @param pname
 *! 
 *! Specifies a light source parameter for @i{light@}. @[GL_AMBIENT],
 *! @[GL_DIFFUSE], @[GL_SPECULAR], @[GL_POSITION], @[GL_SPOT_CUTOFF],
 *! @[GL_SPOT_DIRECTION], @[GL_SPOT_EXPONENT], @[GL_CONSTANT_ATTENUATION],
 *! @[GL_LINEAR_ATTENUATION], and @[GL_QUADRATIC_ATTENUATION] are
 *! accepted.
 *! 
 *! @param params
 *! 
 *! Specifies a pointer to the value or values that parameter @i{pname@}
 *! of light source @i{light@} will be set to.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if either @i{light@} or @i{pname@} is
 *! not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if a spot exponent value is specified
 *! outside the range [0, 128], or if spot cutoff is specified outside the
 *! range [0, 90] (except for the special value 180), or if a negative
 *! attenuation factor is specified.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLight] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! It is always the case that @tt{GL_LIGHT@}@i{i@} = @[GL_LIGHT0] +
 *! @i{i@}.
 *! 
 *! @seealso
 *! 
 *! @[glColorMaterial], @[glLightModel], @[glMaterial]
 *! 
 */


/*! @decl void glLightModel(int pname, float|int|array(float|int) param)
 *! 
 *! @[glLightModel] sets the lighting model parameter. @i{pname@} names a
 *! parameter and @i{params@} gives the new value. There are three
 *! lighting model parameters:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_LIGHT_MODEL_AMBIENT</ref></c>
 *! <c><p></p><p><i>params</i> contains four integer or floating-point
 *! values that specify the ambient RGBA intensity of the entire scene.
 *! Integer values are mapped linearly such that the most positive
 *! representable value maps to 1.0, and the most negative representable
 *! value maps to -1.0. Floating-point values are mapped directly. Neither
 *! integer nor floating-point values are clamped. The initial ambient
 *! scene intensity is (0.2, 0.2, 0.2, 1.0). </p></c></r>
 *! <r><c><ref>GL_LIGHT_MODEL_COLOR_CONTROL</ref></c>
 *! <c><p></p><p><i>params</i> must be either <ref
 *! >GL_SEPARATE_SPECULAR_COLOR</ref> or <ref>GL_SINGLE_COLOR</ref>. <ref
 *! >GL_SINGLE_COLOR</ref> specifies that a single color is generated from
 *! the lighting computation for a vertex. <ref
 *! >GL_SEPARATE_SPECULAR_COLOR</ref> specifies that the specular color
 *! computation of lighting be stored separately from the remainder of the
 *! lighting computation. The specular color is summed into the generated
 *! fragment's color after the application of texture mapping (if
 *! enabled). The initial value is <ref>GL_SINGLE_COLOR</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_LIGHT_MODEL_LOCAL_VIEWER</ref></c>
 *! <c><p></p><p><i>params</i> is a single integer or floating-point value
 *! that specifies how specular reflection angles are computed. If <i
 *! >params</i> is 0 (or 0.0), specular reflection angles take the view
 *! direction to be parallel to and in the direction of the -<i>z</i>
 *! axis, regardless of the location of the vertex in eye coordinates.
 *! Otherwise, specular reflections are computed from the origin of the
 *! eye coordinate system. The initial value is 0. </p></c></r>
 *! <r><c><ref>GL_LIGHT_MODEL_TWO_SIDE</ref></c>
 *! <c><p></p><p><i>params</i> is a single integer or floating-point value
 *! that specifies whether one- or two-sided lighting calculations are
 *! done for polygons. It has no effect on the lighting calculations for
 *! points, lines, or bitmaps. If <i>params</i> is 0 (or 0.0), one-sided
 *! lighting is specified, and only the <i>front</i> material parameters
 *! are used in the lighting equation. Otherwise, two-sided lighting is
 *! specified. In this case, vertices of back-facing polygons are lighted
 *! using the <i>back</i> material parameters and have their normals
 *! reversed before the lighting equation is evaluated. Vertices of
 *! front-facing polygons are always lighted using the <i>front</i>
 *! material parameters, with no change to their normals. The initial
 *! value is 0. </p></c></r>
 *! </matrix>@}In RGBA mode, the lighted color of a vertex is the sum of
 *! the material emission intensity, the product of the material ambient
 *! reflectance and the lighting model full-scene ambient intensity, and
 *! the contribution of each enabled light source. Each light source
 *! contributes the sum of three terms: ambient, diffuse, and specular.
 *! The ambient light source contribution is the product of the material
 *! ambient reflectance and the light's ambient intensity. The diffuse
 *! light source contribution is the product of the material diffuse
 *! reflectance, the light's diffuse intensity, and the dot product of the
 *! vertex's normal with the normalized vector from the vertex to the
 *! light source. The specular light source contribution is the product of
 *! the material specular reflectance, the light's specular intensity, and
 *! the dot product of the normalized vertex-to-eye and vertex-to-light
 *! vectors, raised to the power of the shininess of the material. All
 *! three light source contributions are attenuated equally based on the
 *! distance from the vertex to the light source and on light source
 *! direction, spread exponent, and spread cutoff angle. All dot products
 *! are replaced with 0 if they evaluate to a negative value.
 *! 
 *! The alpha component of the resulting lighted color is set to the alpha
 *! value of the material diffuse reflectance.
 *! 
 *! In color index mode, the value of the lighted index of a vertex ranges
 *! from the ambient to the specular values passed to @[glMaterial] using
 *! @[GL_COLOR_INDEXES]. Diffuse and specular coefficients, computed with
 *! a (.30, .59, .11) weighting of the lights' colors, the shininess of
 *! the material, and the same reflection and attenuation equations as in
 *! the RGBA case, determine how much above ambient the resulting index
 *! is.
 *! 
 *! @param pname
 *! 
 *! Specifies a single-valued lighting model parameter.
 *! @[GL_LIGHT_MODEL_LOCAL_VIEWER], @[GL_LIGHT_MODEL_COLOR_CONTROL], and
 *! @[GL_LIGHT_MODEL_TWO_SIDE] are accepted.
 *! 
 *! @param param
 *! 
 *! Specifies the value that @i{param@} will be set to.
 *! 
 *! @param pname
 *! 
 *! Specifies a lighting model parameter. @[GL_LIGHT_MODEL_AMBIENT],
 *! @[GL_LIGHT_MODEL_COLOR_CONTROL], @[GL_LIGHT_MODEL_LOCAL_VIEWER], and
 *! @[GL_LIGHT_MODEL_TWO_SIDE] are accepted.
 *! 
 *! @param params
 *! 
 *! Specifies a pointer to the value or values that @i{params@} will be
 *! set to.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{pname@} is not an accepted
 *! value.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{pname@} is
 *! @[GL_LIGHT_MODEL_COLOR_CONTROL] and @i{params@} is not one of
 *! @[GL_SINGLE_COLOR] or @[GL_SEPARATE_SPECULAR_COLOR].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLightModel] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[GL_LIGHT_MODEL_COLOR_CONTROL] is available only if the GL version is
 *! 1.2 or greater.
 *! 
 *! @seealso
 *! 
 *! @[glLight], @[glMaterial]
 *! 
 */


/*! @decl void glLineStipple(int factor, int pattern)
 *! 
 *! Line stippling masks out certain fragments produced by rasterization;
 *! those fragments will not be drawn. The masking is achieved by using
 *! three parameters: the 16-bit line stipple pattern @i{pattern@}, the
 *! repeat count @i{factor@}, and an integer stipple counter @i{s@}.
 *! 
 *! Counter @i{s@} is reset to 0 whenever @[glBegin] is called and before
 *! each line segment of a @[glBegin](@[GL_LINES])/@[glEnd] sequence is
 *! generated. It is incremented after each fragment of a unit width
 *! aliased line segment is generated or after each @i{i@} fragments of an
 *! @i{i@} width line segment are generated. The @i{i@} fragments
 *! associated with count @i{s@} are masked out if
 *! 
 *! @i{pattern@} bit
 *! (@i{s@}/factor)%16
 *! 
 *! is 0, otherwise these fragments are sent to the frame buffer. Bit zero
 *! of @i{pattern@} is the least significant bit.
 *! 
 *! Antialiased lines are treated as a sequence of
 *! 1@xml{&#215;@}width
 *! rectangles for purposes of stippling. Whether rectangle @i{s@} is
 *! rasterized or not depends on the fragment rule described for aliased
 *! lines, counting rectangles rather than groups of fragments.
 *! 
 *! To enable and disable line stippling, call @[glEnable] and
 *! @[glDisable] with argument @[GL_LINE_STIPPLE]. When enabled, the line
 *! stipple pattern is applied as described above. When disabled, it is as
 *! if the pattern were all 1's. Initially, line stippling is disabled.
 *! 
 *! @param factor
 *! 
 *! Specifies a multiplier for each bit in the line stipple pattern. If
 *! @i{factor@} is 3, for example, each bit in the pattern is used three
 *! times before the next bit in the pattern is used. @i{factor@} is
 *! clamped to the range [1, 256] and defaults to 1.
 *! 
 *! @param pattern
 *! 
 *! Specifies a 16-bit integer whose bit pattern determines which
 *! fragments of a line will be drawn when the line is rasterized. Bit
 *! zero is used first; the default pattern is all 1's.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLineStipple] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glLineWidth], @[glPolygonStipple]
 *! 
 */


/*! @decl void glLineWidth(float width)
 *! 
 *! @[glLineWidth] specifies the rasterized width of both aliased and
 *! antialiased lines. Using a line width other than 1 has different
 *! effects, depending on whether line antialiasing is enabled. To enable
 *! and disable line antialiasing, call @[glEnable] and @[glDisable] with
 *! argument @[GL_LINE_SMOOTH]. Line antialiasing is initially disabled.
 *! 
 *! If line antialiasing is disabled, the actual width is determined by
 *! rounding the supplied width to the nearest integer. (If the rounding
 *! results in the value 0, it is as if the line width were 1.) If
 *! @xml{&#8739;@}@xml{&#916;@}@i{x@}@xml{&#8739;@}>=@xml{&#8739;@}@xml{&#916;@}@i{y@}@xml{&#8739;@}
 *! , @i{i@} pixels are filled in each column that is rasterized, where
 *! @i{i@} is the rounded value of @i{width@}. Otherwise, @i{i@} pixels
 *! are filled in each row that is rasterized.
 *! 
 *! If antialiasing is enabled, line rasterization produces a fragment for
 *! each pixel square that intersects the region lying within the
 *! rectangle having width equal to the current line width, length equal
 *! to the actual length of the line, and centered on the mathematical
 *! line segment. The coverage value for each fragment is the window
 *! coordinate area of the intersection of the rectangular region with the
 *! corresponding pixel square. This value is saved and used in the final
 *! rasterization step.
 *! 
 *! Not all widths can be supported when line antialiasing is enabled. If
 *! an unsupported width is requested, the nearest supported width is
 *! used. Only width 1 is guaranteed to be supported; others depend on the
 *! implementation. Likewise, there is a range for aliased line widths as
 *! well. To query the range of supported widths and the size difference
 *! between supported widths within the range, call @[glGet] with
 *! arguments @[GL_ALIASED_LINE_WIDTH_RANGE],
 *! @[GL_SMOOTH_LINE_WIDTH_RANGE], and
 *! @[GL_SMOOTH_LINE_WIDTH_GRANULARITY].
 *! 
 *! @param width
 *! 
 *! Specifies the width of rasterized lines. The initial value is 1.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{width@} is less than or equal
 *! to 0.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLineWidth] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! The line width specified by @[glLineWidth] is always returned when
 *! @[GL_LINE_WIDTH] is queried. Clamping and rounding for aliased and
 *! antialiased lines have no effect on the specified value.
 *! 
 *! Nonantialiased line width may be clamped to an
 *! implementation-dependent maximum. Call @[glGet] with
 *! @[GL_ALIASED_LINE_WIDTH_RANGE] to determine the maximum width.
 *! 
 *! In OpenGL 1.2, the tokens @[GL_LINE_WIDTH_RANGE] and
 *! @[GL_LINE_WIDTH_GRANULARITY] were replaced by
 *! @[GL_ALIASED_LINE_WIDTH_RANGE], @[GL_SMOOTH_LINE_WIDTH_RANGE], and
 *! @[GL_SMOOTH_LINE_WIDTH_GRANULARITY]. The old names are retained for
 *! backward compatibility, but should not be used in new code.
 *! 
 *! @seealso
 *! 
 *! @[glEnable]
 *! 
 */


/*! @decl void glListBase(int base)
 *! 
 *! @[glCallLists] specifies an array of offsets. Display-list names are
 *! generated by adding @i{base@} to each offset. Names that reference
 *! valid display lists are executed; the others are ignored.
 *! 
 *! @param base
 *! 
 *! Specifies an integer offset that will be added to @[glCallLists]
 *! offsets to generate display-list names. The initial value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glListBase] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glCallLists]
 *! 
 */


/*! @decl void glLoadMatrix(array(float|int) m)
 *! 
 *! @[glLoadMatrix] replaces the current matrix with the one whose
 *! elements are specified by @i{m@}. The current matrix is the projection
 *! matrix, modelview matrix, or texture matrix, depending on the current
 *! matrix mode (see @[glMatrixMode]).
 *! 
 *! The current matrix, M, defines a transformation of coordinates. For
 *! instance, assume M refers to the modelview matrix. If
 *! @i{v@}=(@i{v@}[0], @i{v@}[1], @i{v@}[2], @i{v@}[3])
 *! is the set of object coordinates of a vertex, and @i{m@} points to an
 *! array of 16 single- or double-precision floating-point values
 *! @i{m@}={@i{m@}[0], @i{m@}[1], ..., @i{m@}[15]}
 *! , then the modelview transformation
 *! @i{M@}(@i{v@})
 *! does the following:
 *! 
 *! @i{M@}(@i{v@})=(@xml{<matrix><r><c><i>m</i>[0]</c><c><i>m</i>[4]</c><c
 *! ><i>m</i>[8]</c><c><i>m</i>[12]</c></r><r><c><i>m</i>[1]</c><c><i>m</i
 *! >[5]</c><c><i>m</i>[9]</c><c><i>m</i>[13]</c></r><r><c><i>m</i>[2]</c
 *! ><c><i>m</i>[6]</c><c><i>m</i>[10]</c><c><i>m</i>[14]</c></r><r><c><i
 *! >m</i>[3]</c><c><i>m</i>[7]</c><c><i>m</i>[11]</c><c><i>m</i>[15]</c
 *! ></r></matrix>@})@xml{&#215;@}(@xml{<matrix><r><c><i>v</i>[0]</c></r
 *! ><r><c><i>v</i>[1]</c></r><r><c><i>v</i>[2]</c></r><r><c><i>v</i
 *! >[3]</c></r></matrix>@})
 *! 
 *! Projection and texture transformations are similarly defined.
 *! 
 *! @param m
 *! 
 *! Specifies a pointer to 16 consecutive values, which are used as the
 *! elements of a
 *! 4@xml{&#215;@}4
 *! column-major matrix.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLoadMatrix] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! While the elements of the matrix may be specified with single or
 *! double precision, the GL implementation may store or operate on these
 *! values in less than single precision.
 *! 
 *! @seealso
 *! 
 *! @[glLoadIdentity], @[glMatrixMode], @[glMultMatrix],
 *! @[glMultTransposeMatrix], @[glPushMatrix]
 *! 
 */


/*! @decl void glLoadName(int name)
 *! 
 *! The name stack is used during selection mode to allow sets of
 *! rendering commands to be uniquely identified. It consists of an
 *! ordered set of unsigned integers and is initially empty.
 *! 
 *! @[glLoadName] causes @i{name@} to replace the value on the top of the
 *! name stack.
 *! 
 *! The name stack is always empty while the render mode is not
 *! @[GL_SELECT]. Calls to @[glLoadName] while the render mode is not
 *! @[GL_SELECT] are ignored.
 *! 
 *! @param name
 *! 
 *! Specifies a name that will replace the top value on the name stack.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLoadName] is called while
 *! the name stack is empty.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLoadName] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glInitNames], @[glPushName], @[glRenderMode], @[glSelectBuffer]
 *! 
 */


/*! @decl void glMaterial(int face, int pname, @
 *! float|int|array(float|int) param)
 *! 
 *! @[glMaterial] assigns values to material parameters. There are two
 *! matched sets of material parameters. One, the @i{front-facing@} set,
 *! is used to shade points, lines, bitmaps, and all polygons (when
 *! two-sided lighting is disabled), or just front-facing polygons (when
 *! two-sided lighting is enabled). The other set, @i{back-facing@}, is
 *! used to shade back-facing polygons only when two-sided lighting is
 *! enabled. Refer to the @[glLightModel] reference page for details
 *! concerning one- and two-sided lighting calculations.
 *! 
 *! @[glMaterial] takes three arguments. The first, @i{face@}, specifies
 *! whether the @[GL_FRONT] materials, the @[GL_BACK] materials, or both
 *! @[GL_FRONT_AND_BACK] materials will be modified. The second,
 *! @i{pname@}, specifies which of several parameters in one or both sets
 *! will be modified. The third, @i{params@}, specifies what value or
 *! values will be assigned to the specified parameter.
 *! 
 *! Material parameters are used in the lighting equation that is
 *! optionally applied to each vertex. The equation is discussed in the
 *! @[glLightModel] reference page. The parameters that can be specified
 *! using @[glMaterial], and their interpretations by the lighting
 *! equation, are as follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_AMBIENT</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify the ambient RGBA reflectance of the material. Integer
 *! values are mapped linearly such that the most positive representable
 *! value maps to 1.0, and the most negative representable value maps to
 *! -1.0. Floating-point values are mapped directly. Neither integer nor
 *! floating-point values are clamped. The initial ambient reflectance for
 *! both front- and back-facing materials is (0.2, 0.2, 0.2, 1.0). </p
 *! ></c></r>
 *! <r><c><ref>GL_DIFFUSE</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify the diffuse RGBA reflectance of the material. Integer
 *! values are mapped linearly such that the most positive representable
 *! value maps to 1.0, and the most negative representable value maps to
 *! -1.0. Floating-point values are mapped directly. Neither integer nor
 *! floating-point values are clamped. The initial diffuse reflectance for
 *! both front- and back-facing materials is (0.8, 0.8, 0.8, 1.0). </p
 *! ></c></r>
 *! <r><c><ref>GL_SPECULAR</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify the specular RGBA reflectance of the material. Integer
 *! values are mapped linearly such that the most positive representable
 *! value maps to 1.0, and the most negative representable value maps to
 *! -1.0. Floating-point values are mapped directly. Neither integer nor
 *! floating-point values are clamped. The initial specular reflectance
 *! for both front- and back-facing materials is (0, 0, 0, 1). </p
 *! ></c></r>
 *! <r><c><ref>GL_EMISSION</ref></c>
 *! <c><p><i>params</i> contains four integer or floating-point values
 *! that specify the RGBA emitted light intensity of the material. Integer
 *! values are mapped linearly such that the most positive representable
 *! value maps to 1.0, and the most negative representable value maps to
 *! -1.0. Floating-point values are mapped directly. Neither integer nor
 *! floating-point values are clamped. The initial emission intensity for
 *! both front- and back-facing materials is (0, 0, 0, 1). </p></c></r>
 *! <r><c><ref>GL_SHININESS</ref></c>
 *! <c><p><i>params</i> is a single integer or floating-point value that
 *! specifies the RGBA specular exponent of the material. Integer and
 *! floating-point values are mapped directly. Only values in the range
 *! [0, 128] are accepted. The initial specular exponent for both front-
 *! and back-facing materials is 0. </p></c></r>
 *! <r><c><ref>GL_AMBIENT_AND_DIFFUSE</ref></c>
 *! <c><p>Equivalent to calling <ref>glMaterial</ref> twice with the same
 *! parameter values, once with <ref>GL_AMBIENT</ref> and once with <ref
 *! >GL_DIFFUSE</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_INDEXES</ref></c>
 *! <c><p><i>params</i> contains three integer or floating-point values
 *! specifying the color indices for ambient, diffuse, and specular
 *! lighting. These three values, and <ref>GL_SHININESS</ref>, are the
 *! only material values used by the color index mode lighting equation.
 *! Refer to the <ref>glLightModel</ref> reference page for a discussion
 *! of color index lighting. </p></c></r>
 *! </matrix>@}
 *! 
 *! @param face
 *! 
 *! Specifies which face or faces are being updated. Must be one of
 *! @[GL_FRONT], @[GL_BACK], or @[GL_FRONT_AND_BACK].
 *! 
 *! @param pname
 *! 
 *! Specifies the single-valued material parameter of the face or faces
 *! that is being updated. Must be @[GL_SHININESS].
 *! 
 *! @param param
 *! 
 *! Specifies the value that parameter @[GL_SHININESS] will be set to.
 *! 
 *! @param face
 *! 
 *! Specifies which face or faces are being updated. Must be one of
 *! @[GL_FRONT], @[GL_BACK], or @[GL_FRONT_AND_BACK].
 *! 
 *! @param pname
 *! 
 *! Specifies the material parameter of the face or faces that is being
 *! updated. Must be one of @[GL_AMBIENT], @[GL_DIFFUSE], @[GL_SPECULAR],
 *! @[GL_EMISSION], @[GL_SHININESS], @[GL_AMBIENT_AND_DIFFUSE], or
 *! @[GL_COLOR_INDEXES].
 *! 
 *! @param params
 *! 
 *! Specifies a pointer to the value or values that @i{pname@} will be set
 *! to.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if either @i{face@} or @i{pname@} is
 *! not an accepted value.
 *! 
 *! @[GL_INVALID_VALUE] is generated if a specular exponent outside the
 *! range [0, 128] is specified.
 *! 
 *! @note
 *! 
 *! The material parameters can be updated at any time. In particular,
 *! @[glMaterial] can be called between a call to @[glBegin] and the
 *! corresponding call to @[glEnd]. If only a single material parameter is
 *! to be changed per vertex, however, @[glColorMaterial] is preferred
 *! over @[glMaterial] (see @[glColorMaterial]).
 *! 
 *! While the ambient, diffuse, specular and emission material parameters
 *! all have alpha components, only the diffuse alpha component is used in
 *! the lighting computation.
 *! 
 *! @seealso
 *! 
 *! @[glColorMaterial], @[glLight], @[glLightModel]
 *! 
 */


/*! @decl void glMultMatrix(array(float|int) m)
 *! 
 *! @[glMultMatrix] multiplies the current matrix with the one specified
 *! using @i{m@}, and replaces the current matrix with the product.
 *! 
 *! The current matrix is determined by the current matrix mode (see
 *! @[glMatrixMode]). It is either the projection matrix, modelview
 *! matrix, or the texture matrix.
 *! 
 *! @param m
 *! 
 *! Points to 16 consecutive values that are used as the elements of a
 *! 4@xml{&#215;@}4
 *! column-major matrix.
 *! 
 *! @example
 *! 
 *! If the current matrix is @i{C@} and the coordinates to be transformed
 *! are
 *! @i{v@}=(@i{v@}[0], @i{v@}[1], @i{v@}[2], @i{v@}[3])
 *! , then the current transformation is
 *! @i{C@}@xml{&#215;@}@i{v@}
 *! , or
 *! 
 *! (@xml{<matrix><r><c><i>c</i>[0]</c><c><i>c</i>[4]</c><c><i>c</i>[8]</c
 *! ><c><i>c</i>[12]</c></r><r><c><i>c</i>[1]</c><c><i>c</i>[5]</c><c><i
 *! >c</i>[9]</c><c><i>c</i>[13]</c></r><r><c><i>c</i>[2]</c><c><i>c</i
 *! >[6]</c><c><i>c</i>[10]</c><c><i>c</i>[14]</c></r><r><c><i>c</i>[3]</c
 *! ><c><i>c</i>[7]</c><c><i>c</i>[11]</c><c><i>c</i>[15]</c></r></matrix
 *! >@})@xml{&#215;@}(@xml{<matrix><r><c><i>v</i>[0]</c></r><r><c><i>v</i
 *! >[1]</c></r><r><c><i>v</i>[2]</c></r><r><c><i>v</i>[3]</c></r></matrix
 *! >@})
 *! 
 *! Calling @[glMultMatrix] with an argument of
 *! @i{m@}={@i{m@}[0], @i{m@}[1], ..., @i{m@}[15]}
 *! replaces the current transformation with
 *! (@i{C@}@xml{&#215;@}@i{M@})@xml{&#215;@}@i{v@}
 *! , or
 *! 
 *! (@xml{<matrix><r><c><i>c</i>[0]</c><c><i>c</i>[4]</c><c><i>c</i>[8]</c
 *! ><c><i>c</i>[12]</c></r><r><c><i>c</i>[1]</c><c><i>c</i>[5]</c><c><i
 *! >c</i>[9]</c><c><i>c</i>[13]</c></r><r><c><i>c</i>[2]</c><c><i>c</i
 *! >[6]</c><c><i>c</i>[10]</c><c><i>c</i>[14]</c></r><r><c><i>c</i>[3]</c
 *! ><c><i>c</i>[7]</c><c><i>c</i>[11]</c><c><i>c</i>[15]</c></r></matrix
 *! >@})@xml{&#215;@}(@xml{<matrix><r><c><i>m</i>[0]</c><c><i>m</i>[4]</c
 *! ><c><i>m</i>[8]</c><c><i>m</i>[12]</c></r><r><c><i>m</i>[1]</c><c><i
 *! >m</i>[5]</c><c><i>m</i>[9]</c><c><i>m</i>[13]</c></r><r><c><i>m</i
 *! >[2]</c><c><i>m</i>[6]</c><c><i>m</i>[10]</c><c><i>m</i>[14]</c></r><r
 *! ><c><i>m</i>[3]</c><c><i>m</i>[7]</c><c><i>m</i>[11]</c><c><i>m</i
 *! >[15]</c></r></matrix>@})@xml{&#215;@}(@xml{<matrix><r><c><i>v</i
 *! >[0]</c></r><r><c><i>v</i>[1]</c></r><r><c><i>v</i>[2]</c></r><r><c><i
 *! >v</i>[3]</c></r></matrix>@})
 *! 
 *! Where @i{v@} is represented as a
 *! 4@xml{&#215;@}1
 *! matrix.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glMultMatrix] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! While the elements of the matrix may be specified with single or
 *! double precision, the GL may store or operate on these values in
 *! less-than-single precision.
 *! 
 *! In many computer languages,
 *! 4@xml{&#215;@}4
 *! arrays are represented in row-major order. The transformations just
 *! described represent these matrices in column-major order. The order of
 *! the multiplication is important. For example, if the current
 *! transformation is a rotation, and @[glMultMatrix] is called with a
 *! translation matrix, the translation is done directly on the
 *! coordinates to be transformed, while the rotation is done on the
 *! results of that translation.
 *! 
 *! @seealso
 *! 
 *! @[glLoadIdentity], @[glLoadMatrix], @[glLoadTransposeMatrix],
 *! @[glMatrixMode], @[glMultTransposeMatrix], @[glPushMatrix]
 *! 
 */


/*! @decl void glNewList(int list, int mode)
 *! @decl void glEndList()
 *! 
 *! Display lists are groups of GL commands that have been stored for
 *! subsequent execution. Display lists are created with @[glNewList]. All
 *! subsequent commands are placed in the display list, in the order
 *! issued, until @[glEndList] is called.
 *! 
 *! @[glNewList] has two arguments. The first argument, @i{list@}, is a
 *! positive integer that becomes the unique name for the display list.
 *! Names can be created and reserved with @[glGenLists] and tested for
 *! uniqueness with @[glIsList]. The second argument, @i{mode@}, is a
 *! symbolic constant that can assume one of two values:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_COMPILE</ref></c>
 *! <c><p>Commands are merely compiled. </p></c></r>
 *! <r><c><ref>GL_COMPILE_AND_EXECUTE</ref></c>
 *! <c><p>Commands are executed as they are compiled into the display
 *! list. </p></c></r>
 *! </matrix>@}Certain commands are not compiled into the display list but
 *! are executed immediately, regardless of the display-list mode. These
 *! commands are @[glAreTexturesResident], @[glColorPointer],
 *! @[glDeleteLists], @[glDeleteTextures], @[glDisableClientState],
 *! @[glEdgeFlagPointer], @[glEnableClientState], @[glFeedbackBuffer],
 *! @[glFinish], @[glFlush], @[glGenLists], @[glGenTextures],
 *! @[glIndexPointer], @[glInterleavedArrays], @[glIsEnabled],
 *! @[glIsList], @[glIsTexture], @[glNormalPointer], @[glPopClientAttrib],
 *! @[glPixelStore], @[glPushClientAttrib], @[glReadPixels],
 *! @[glRenderMode], @[glSelectBuffer], @[glTexCoordPointer],
 *! @[glVertexPointer], and all of the @[glGet] commands.
 *! 
 *! Similarly, @[glTexImage1D], @[glTexImage2D], and @[glTexImage3D] are
 *! executed immediately and not compiled into the display list when their
 *! first argument is @[GL_PROXY_TEXTURE_1D], @[GL_PROXY_TEXTURE_1D], or
 *! @[GL_PROXY_TEXTURE_3D], respectively.
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, @[glHistogram]
 *! executes immediately when its argument is @[GL_PROXY_HISTOGRAM].
 *! Similarly, @[glColorTable] executes immediately when its first
 *! argument is @[GL_PROXY_COLOR_TABLE],
 *! @[GL_PROXY_POST_CONVOLUTION_COLOR_TABLE], or
 *! @[GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE].
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported,
 *! @[glClientActiveTexture] is not compiled into display lists, but
 *! executed immediately.
 *! 
 *! When @[glEndList] is encountered, the display-list definition is
 *! completed by associating the list with the unique name @i{list@}
 *! (specified in the @[glNewList] command). If a display list with name
 *! @i{list@} already exists, it is replaced only when @[glEndList] is
 *! called.
 *! 
 *! @param list
 *! 
 *! Specifies the display-list name.
 *! 
 *! @param mode
 *! 
 *! Specifies the compilation mode, which can be @[GL_COMPILE] or
 *! @[GL_COMPILE_AND_EXECUTE].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{list@} is 0.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glEndList] is called without
 *! a preceding @[glNewList], or if @[glNewList] is called while a display
 *! list is being defined.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glNewList] or @[glEndList]
 *! is executed between the execution of @[glBegin] and the corresponding
 *! execution of @[glEnd].
 *! 
 *! @[GL_OUT_OF_MEMORY] is generated if there is insufficient memory to
 *! compile the display list. If the GL version is 1.1 or greater, no
 *! change is made to the previous contents of the display list, if any,
 *! and no other change is made to the GL state. (It is as if no attempt
 *! had been made to create the new display list.)
 *! 
 *! @note
 *! 
 *! @[glCallList] and @[glCallLists] can be entered into display lists.
 *! Commands in the display list or lists executed by @[glCallList] or
 *! @[glCallLists] are not included in the display list being created,
 *! even if the list creation mode is @[GL_COMPILE_AND_EXECUTE].
 *! 
 *! A display list is just a group of commands and arguments, so errors
 *! generated by commands in a display list must be generated when the
 *! list is executed. If the list is created in @[GL_COMPILE] mode, errors
 *! are not generated until the list is executed.
 *! 
 *! @seealso
 *! 
 *! @[glCallList], @[glCallLists], @[glDeleteLists], @[glGenLists]
 *! 
 */


/*! @decl void glNormal(float|int nx, float|int ny, float|int nz)
 *! @decl void glNormal(array(float|int) v)
 *! 
 *! The current normal is set to the given coordinates whenever
 *! @[glNormal] is issued. Byte, short, or integer arguments are converted
 *! to floating-point format with a linear mapping that maps the most
 *! positive representable integer value to 1.0 and the most negative
 *! representable integer value to -1.0.
 *! 
 *! Normals specified with @[glNormal] need not have unit length. If
 *! @[GL_NORMALIZE] is enabled, then normals of any length specified with
 *! @[glNormal] are normalized after transformation. If
 *! @[GL_RESCALE_NORMAL] is enabled, normals are scaled by a scaling
 *! factor derived from the modelview matrix. @[GL_RESCALE_NORMAL]
 *! requires that the originally specified normals were of unit length,
 *! and that the modelview matrix contain only uniform scales for proper
 *! results. To enable and disable normalization, call @[glEnable] and
 *! @[glDisable] with either @[GL_NORMALIZE] or @[GL_RESCALE_NORMAL].
 *! Normalization is initially disabled.
 *! 
 *! @param nx
 *! @param ny
 *! @param nz
 *! 
 *! Specify the @i{x@}, @i{y@}, and @i{z@} coordinates of the new current
 *! normal. The initial value of the current normal is the unit vector,
 *! (0, 0, 1).
 *! 
 *! @param v
 *! 
 *! Specifies an array of three elements: the @i{x@}, @i{y@}, and @i{z@}
 *! coordinates of the new current normal.
 *! 
 *! @note
 *! 
 *! The current normal can be updated at any time. In particular,
 *! @[glNormal] can be called between a call to @[glBegin] and the
 *! corresponding call to @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glBegin], @[glColor], @[glIndex], @[glMultiTexCoord],
 *! @[glNormalPointer], @[glTexCoord], @[glVertex]
 *! 
 */


/*! @decl void glOrtho(float left, float right, float bottom, float top, @
 *! float nearVal, float farVal)
 *! 
 *! @[glOrtho] describes a transformation that produces a parallel
 *! projection. The current matrix (see @[glMatrixMode]) is multiplied by
 *! this matrix and the result replaces the current matrix, as if
 *! @[glMultMatrix] were called with the following matrix as its argument:
 *! 
 *! (@xml{<matrix><r><c>2/right-left</c><c>0</c><c>0</c><c><i>t</i><sub><i
 *! >x</i></sub></c></r><r><c>0</c><c>2/top-bottom</c><c>0</c><c><i>t</i
 *! ><sub><i>y</i></sub></c></r><r><c>0</c><c>0</c><c>-2/farVal-nearVal</c
 *! ><c><i>t</i><sub><i>z</i></sub></c></r><r><c>0</c><c>0</c><c>0</c><c
 *! >1</c></r></matrix>@})
 *! 
 *! where
 *! @i{t@}@sub{@i{x@}@}=-right+left/right-left
 *! @i{t@}@sub{@i{y@}@}=-top+bottom/top-bottom
 *! @i{t@}@sub{@i{z@}@}=-farVal+nearVal/farVal-nearVal
 *! 
 *! Typically, the matrix mode is @[GL_PROJECTION], and
 *! (left, bottom, -nearVal)
 *! and
 *! (right, top, -nearVal)
 *! specify the points on the near clipping plane that are mapped to the
 *! lower left and upper right corners of the window, respectively,
 *! assuming that the eye is located at (0, 0, 0).
 *! -farVal
 *! specifies the location of the far clipping plane. Both @i{nearVal@}
 *! and @i{farVal@} can be either positive or negative.
 *! 
 *! Use @[glPushMatrix] and @[glPopMatrix] to save and restore the current
 *! matrix stack.
 *! 
 *! @param left
 *! @param right
 *! 
 *! Specify the coordinates for the left and right vertical clipping
 *! planes.
 *! 
 *! @param bottom
 *! @param top
 *! 
 *! Specify the coordinates for the bottom and top horizontal clipping
 *! planes.
 *! 
 *! @param nearVal
 *! @param farVal
 *! 
 *! Specify the distances to the nearer and farther depth clipping planes.
 *! These values are negative if the plane is to be behind the viewer.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{left@} = @i{right@}, or
 *! @i{bottom@} = @i{top@}, or @i{near@} = @i{far@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glOrtho] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glFrustum], @[glMatrixMode], @[glMultMatrix], @[glPushMatrix],
 *! @[glViewport]
 *! 
 */


/*! @decl void glPassThrough(float token)
 *! 
 *! Feedback is a GL render mode. The mode is selected by calling
 *! @[glRenderMode] with @[GL_FEEDBACK]. When the GL is in feedback mode,
 *! no pixels are produced by rasterization. Instead, information about
 *! primitives that would have been rasterized is fed back to the
 *! application using the GL. See the @[glFeedbackBuffer] reference page
 *! for a description of the feedback buffer and the values in it.
 *! 
 *! @[glPassThrough] inserts a user-defined marker in the feedback buffer
 *! when it is executed in feedback mode. @i{token@} is returned as if it
 *! were a primitive; it is indicated with its own unique identifying
 *! value: @[GL_PASS_THROUGH_TOKEN]. The order of @[glPassThrough]
 *! commands with respect to the specification of graphics primitives is
 *! maintained.
 *! 
 *! @param token
 *! 
 *! Specifies a marker value to be placed in the feedback buffer following
 *! a @[GL_PASS_THROUGH_TOKEN].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glPassThrough] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glPassThrough] is ignored if the GL is not in feedback mode.
 *! 
 *! @seealso
 *! 
 *! @[glFeedbackBuffer], @[glRenderMode]
 *! 
 */


/*! @decl void glPixelZoom(float xfactor, float yfactor)
 *! 
 *! @[glPixelZoom] specifies values for the @i{x@} and @i{y@} zoom
 *! factors. During the execution of @[glDrawPixels] or @[glCopyPixels],
 *! if (xr, yr) is the current raster position, and a given element is in
 *! the @i{m@}th row and @i{n@}th column of the pixel rectangle, then
 *! pixels whose centers are in the rectangle with corners at
 *! 
 *! (
 *! xr+@i{n@}@xml{&#183;@}xfactor
 *! ,
 *! yr+@i{m@}@xml{&#183;@}yfactor
 *! )
 *! 
 *! (
 *! xr+(@i{n@}+1)@xml{&#183;@}xfactor
 *! ,
 *! yr+(@i{m@}+1)@xml{&#183;@}yfactor
 *! )
 *! 
 *! are candidates for replacement. Any pixel whose center lies on the
 *! bottom or left edge of this rectangular region is also modified.
 *! 
 *! Pixel zoom factors are not limited to positive values. Negative zoom
 *! factors reflect the resulting image about the current raster position.
 *! 
 *! @param xfactor
 *! @param yfactor
 *! 
 *! Specify the @i{x@} and @i{y@} zoom factors for pixel write operations.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glPixelZoom] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glCopyPixels], @[glDrawPixels]
 *! 
 */


/*! @decl void glPointSize(float size)
 *! 
 *! @[glPointSize] specifies the rasterized diameter of both aliased and
 *! antialiased points. Using a point size other than 1 has different
 *! effects, depending on whether point antialiasing is enabled. To enable
 *! and disable point antialiasing, call @[glEnable] and @[glDisable] with
 *! argument @[GL_POINT_SMOOTH]. Point antialiasing is initially disabled.
 *! 
 *! The specified point size is multiplied with a distance attenuation
 *! factor and clamped to the specified point size range, and further
 *! clamped to the implementation-dependent point size range to produce
 *! the derived point size using
 *! 
 *! pointSize=clamp(size@xml{&#215;@}@xml{&#8730;@}(1/@i{a@}+@i{b@}@xml{&#215;@}@i{d@}+@i{c@}@xml{&#215;@}@i{d@}@sup{2@}))
 *! 
 *! where @i{d@} is the eye-coordinate distance from the eye to the
 *! vertex, and @i{a@}, @i{b@}, and @i{c@} are the distance attenuation
 *! coefficients (see @[glPointParameter]).
 *! 
 *! If multisampling is disabled, the computed point size is used as the
 *! point's width.
 *! 
 *! If multisampling is enabled, the point may be faded by modifying the
 *! point alpha value (see @[glSampleCoverage]) instead of allowing the
 *! point width to go below a given threshold (see @[glPointParameter]).
 *! In this case, the width is further modified in the following manner:
 *! 
 *! pointWidth={@xml{<matrix><r><c>pointSize</c></r><r><c>threshold</c></r
 *! ></matrix>@}@xml{<matrix><r><c>pointSize&gt;=threshold</c></r><r><c
 *! >otherwise</c></r></matrix>@}
 *! 
 *! The point alpha value is modified by computing:
 *! 
 *! pointAlpha={@xml{<matrix><r><c>1</c></r><r><c
 *! >(pointSize/threshold)<sup>2</sup></c></r></matrix>@}@xml{<matrix><r
 *! ><c>pointSize&gt;=threshold</c></r><r><c>otherwise</c></r></matrix>@}
 *! 
 *! If point antialiasing is disabled, the actual size is determined by
 *! rounding the supplied size to the nearest integer. (If the rounding
 *! results in the value 0, it is as if the point size were 1.) If the
 *! rounded size is odd, then the center point (@i{x@}, @i{y@}) of the
 *! pixel fragment that represents the point is computed as
 *! 
 *! (@xml{&#8970;@}@i{x@}@sub{@i{w@}@}@xml{&#8971;@}+.5,
 *! @xml{&#8970;@}@i{y@}@sub{@i{w@}@}@xml{&#8971;@}+.5)
 *! 
 *! where @i{w@} subscripts indicate window coordinates. All pixels that
 *! lie within the square grid of the rounded size centered at (@i{x@},
 *! @i{y@}) make up the fragment. If the size is even, the center point is
 *! 
 *! (@xml{&#8970;@}@i{x@}@sub{@i{w@}@}+.5@xml{&#8971;@},
 *! @xml{&#8970;@}@i{y@}@sub{@i{w@}@}+.5@xml{&#8971;@})
 *! 
 *! and the rasterized fragment's centers are the half-integer window
 *! coordinates within the square of the rounded size centered at (@i{x@},
 *! @i{y@}). All pixel fragments produced in rasterizing a nonantialiased
 *! point are assigned the same associated data, that of the vertex
 *! corresponding to the point.
 *! 
 *! If antialiasing is enabled, then point rasterization produces a
 *! fragment for each pixel square that intersects the region lying within
 *! the circle having diameter equal to the current point size and
 *! centered at the point's (@i{x@}@sub{@i{w@}@}, @i{y@}@sub{@i{w@}@}).
 *! The coverage value for each fragment is the window coordinate area of
 *! the intersection of the circular region with the corresponding pixel
 *! square. This value is saved and used in the final rasterization step.
 *! The data associated with each fragment is the data associated with the
 *! point being rasterized.
 *! 
 *! Not all sizes are supported when point antialiasing is enabled. If an
 *! unsupported size is requested, the nearest supported size is used.
 *! Only size 1 is guaranteed to be supported; others depend on the
 *! implementation. To query the range of supported sizes and the size
 *! difference between supported sizes within the range, call @[glGet]
 *! with arguments @[GL_SMOOTH_POINT_SIZE_RANGE] and
 *! @[GL_SMOOTH_POINT_SIZE_GRANULARITY]. For aliased points, query the
 *! supported ranges and granularity with @[glGet] with arguments
 *! @[GL_ALIASED_POINT_SIZE_RANGE].
 *! 
 *! @param size
 *! 
 *! Specifies the diameter of rasterized points. The initial value is 1.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{size@} is less than or equal to
 *! 0.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glPointSize] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! The point size specified by @[glPointSize] is always returned when
 *! @[GL_POINT_SIZE] is queried. Clamping and rounding for aliased and
 *! antialiased points have no effect on the specified value.
 *! 
 *! A non-antialiased point size may be clamped to an
 *! implementation-dependent maximum. Although this maximum cannot be
 *! queried, it must be no less than the maximum value for antialiased
 *! points, rounded to the nearest integer value.
 *! 
 *! @[GL_POINT_SIZE_RANGE] and @[GL_POINT_SIZE_GRANULARITY] are deprecated
 *! in GL versions 1.2 and greater. Their functionality has been replaced
 *! by @[GL_SMOOTH_POINT_SIZE_RANGE] and
 *! @[GL_SMOOTH_POINT_SIZE_GRANULARITY].
 *! 
 *! @seealso
 *! 
 *! @[glEnable], @[glPointParameter]
 *! 
 */


/*! @decl void glPolygonMode(int face, int mode)
 *! 
 *! @[glPolygonMode] controls the interpretation of polygons for
 *! rasterization. @i{face@} describes which polygons @i{mode@} applies
 *! to: front-facing polygons (@[GL_FRONT]), back-facing polygons
 *! (@[GL_BACK]), or both (@[GL_FRONT_AND_BACK]). The polygon mode affects
 *! only the final rasterization of polygons. In particular, a polygon's
 *! vertices are lit and the polygon is clipped and possibly culled before
 *! these modes are applied.
 *! 
 *! Three modes are defined and can be specified in @i{mode@}:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_POINT</ref></c>
 *! <c><p>Polygon vertices that are marked as the start of a boundary edge
 *! are drawn as points. Point attributes such as <ref>GL_POINT_SIZE</ref>
 *! and <ref>GL_POINT_SMOOTH</ref> control the rasterization of the
 *! points. Polygon rasterization attributes other than <ref
 *! >GL_POLYGON_MODE</ref> have no effect. </p></c></r>
 *! <r><c><ref>GL_LINE</ref></c>
 *! <c><p>Boundary edges of the polygon are drawn as line segments. They
 *! are treated as connected line segments for line stippling; the line
 *! stipple counter and pattern are not reset between segments (see <ref
 *! >glLineStipple</ref>). Line attributes such as <ref>GL_LINE_WIDTH</ref
 *! > and <ref>GL_LINE_SMOOTH</ref> control the rasterization of the
 *! lines. Polygon rasterization attributes other than <ref
 *! >GL_POLYGON_MODE</ref> have no effect. </p></c></r>
 *! <r><c><ref>GL_FILL</ref></c>
 *! <c><p>The interior of the polygon is filled. Polygon attributes such
 *! as <ref>GL_POLYGON_STIPPLE</ref> and <ref>GL_POLYGON_SMOOTH</ref>
 *! control the rasterization of the polygon. </p></c></r>
 *! </matrix>@}
 *! 
 *! @param face
 *! 
 *! Specifies the polygons that @i{mode@} applies to. Must be @[GL_FRONT]
 *! for front-facing polygons, @[GL_BACK] for back-facing polygons, or
 *! @[GL_FRONT_AND_BACK] for front- and back-facing polygons.
 *! 
 *! @param mode
 *! 
 *! Specifies how polygons will be rasterized. Accepted values are
 *! @[GL_POINT], @[GL_LINE], and @[GL_FILL]. The initial value is
 *! @[GL_FILL] for both front- and back-facing polygons.
 *! 
 *! @example
 *! 
 *! To draw a surface with filled back-facing polygons and outlined
 *! front-facing polygons, call
 *! @code
 *! 
 *! glPolygonMode(@[GL_FRONT], @[GL_LINE]);
 *! @endcode
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if either @i{face@} or @i{mode@} is
 *! not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glPolygonMode] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Vertices are marked as boundary or nonboundary with an edge flag. Edge
 *! flags are generated internally by the GL when it decomposes polygons;
 *! they can be set explicitly using @[glEdgeFlag].
 *! 
 *! @seealso
 *! 
 *! @[glBegin], @[glEdgeFlag], @[glLineStipple], @[glLineWidth],
 *! @[glPointSize], @[glPolygonStipple]
 *! 
 */


/*! @decl void glPolygonOffset(float factor, float units)
 *! 
 *! When @[GL_POLYGON_OFFSET_FILL], @[GL_POLYGON_OFFSET_LINE], or
 *! @[GL_POLYGON_OFFSET_POINT] is enabled, each fragment's @i{depth@}
 *! value will be offset after it is interpolated from the @i{depth@}
 *! values of the appropriate vertices. The value of the offset is
 *! factor@xml{&#215;@}DZ+@i{r@}@xml{&#215;@}units
 *! , where DZ is a measurement of the change in depth relative to the
 *! screen area of the polygon, and @i{r@} is the smallest value that is
 *! guaranteed to produce a resolvable offset for a given implementation.
 *! The offset is added before the depth test is performed and before the
 *! value is written into the depth buffer.
 *! 
 *! @[glPolygonOffset] is useful for rendering hidden-line images, for
 *! applying decals to surfaces, and for rendering solids with highlighted
 *! edges.
 *! 
 *! @param factor
 *! 
 *! Specifies a scale factor that is used to create a variable depth
 *! offset for each polygon. The initial value is 0.
 *! 
 *! @param units
 *! 
 *! Is multiplied by an implementation-specific value to create a constant
 *! depth offset. The initial value is 0.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glPolygonOffset] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glPolygonOffset] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @[glPolygonOffset] has no effect on depth coordinates placed in the
 *! feedback buffer.
 *! 
 *! @[glPolygonOffset] has no effect on selection.
 *! 
 *! @seealso
 *! 
 *! @[glDepthFunc], @[glEnable], @[glGet], @[glIsEnabled]
 *! 
 */


/*! @decl void glPushAttrib(int mask)
 *! @decl void glPopAttrib()
 *! 
 *! @[glPushAttrib] takes one argument, a mask that indicates which groups
 *! of state variables to save on the attribute stack. Symbolic constants
 *! are used to set bits in the mask. @i{mask@} is typically constructed
 *! by specifying the bitwise-or of several of these constants together.
 *! The special mask @[GL_ALL_ATTRIB_BITS] can be used to save all
 *! stackable states.
 *! 
 *! The symbolic mask constants and their associated GL state are as
 *! follows (the second column lists which attributes are saved):
 *! 
 *! @xml{<matrix><r><c><ref>GL_ACCUM_BUFFER_BIT</ref></c><c> Accumulation
 *! buffer clear value </c></r><r><c><ref>GL_COLOR_BUFFER_BIT</ref></c><c
 *! ><ref>GL_ALPHA_TEST</ref> enable bit </c></r><r><c></c><c>Alpha test
 *! function and reference value </c></r><r><c></c><c><ref>GL_BLEND</ref>
 *! enable bit </c></r><r><c></c><c>Blending source and destination
 *! functions </c></r><r><c></c><c>Constant blend color </c></r><r><c></c
 *! ><c>Blending equation </c></r><r><c></c><c><ref>GL_DITHER</ref> enable
 *! bit </c></r><r><c></c><c><ref>GL_DRAW_BUFFER</ref> setting </c></r><r
 *! ><c></c><c><ref>GL_COLOR_LOGIC_OP</ref> enable bit </c></r><r><c></c
 *! ><c><ref>GL_INDEX_LOGIC_OP</ref> enable bit </c></r><r><c></c><c>Logic
 *! op function </c></r><r><c></c><c>Color mode and index mode clear
 *! values </c></r><r><c></c><c>Color mode and index mode writemasks </c
 *! ></r><r><c><ref>GL_CURRENT_BIT</ref></c><c> Current RGBA color </c></r
 *! ><r><c></c><c>Current color index </c></r><r><c></c><c>Current normal
 *! vector </c></r><r><c></c><c>Current texture coordinates </c></r><r><c
 *! ></c><c>Current raster position </c></r><r><c></c><c><ref
 *! >GL_CURRENT_RASTER_POSITION_VALID</ref> flag </c></r><r><c></c><c>RGBA
 *! color associated with current raster position </c></r><r><c></c><c
 *! >Color index associated with current raster position </c></r><r><c></c
 *! ><c>Texture coordinates associated with current raster position </c
 *! ></r><r><c></c><c><ref>GL_EDGE_FLAG</ref> flag </c></r><r><c><ref
 *! >GL_DEPTH_BUFFER_BIT</ref></c><c><ref>GL_DEPTH_TEST</ref> enable bit
 *! </c></r><r><c></c><c>Depth buffer test function </c></r><r><c></c><c
 *! >Depth buffer clear value </c></r><r><c></c><c><ref
 *! >GL_DEPTH_WRITEMASK</ref> enable bit </c></r><r><c><ref
 *! >GL_ENABLE_BIT</ref></c><c><ref>GL_ALPHA_TEST</ref> flag </c></r><r><c
 *! ></c><c><ref>GL_AUTO_NORMAL</ref> flag </c></r><r><c></c><c><ref
 *! >GL_BLEND</ref> flag </c></r><r><c></c><c>Enable bits for the
 *! user-definable clipping planes </c></r><r><c></c><c><ref
 *! >GL_COLOR_MATERIAL</ref></c></r><r><c></c><c><ref>GL_CULL_FACE</ref>
 *! flag </c></r><r><c></c><c><ref>GL_DEPTH_TEST</ref> flag </c></r><r><c
 *! ></c><c><ref>GL_DITHER</ref> flag </c></r><r><c></c><c><ref
 *! >GL_FOG</ref> flag </c></r><r><c></c><c><tt>GL_LIGHT</tt><i>i</i>
 *! where <ref>0</ref> &lt;= <i>i</i> &lt; <ref>GL_MAX_LIGHTS</ref></c></r
 *! ><r><c></c><c><ref>GL_LIGHTING</ref> flag </c></r><r><c></c><c><ref
 *! >GL_LINE_SMOOTH</ref> flag </c></r><r><c></c><c><ref
 *! >GL_LINE_STIPPLE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_COLOR_LOGIC_OP</ref> flag </c></r><r><c></c><c><ref
 *! >GL_INDEX_LOGIC_OP</ref> flag </c></r><r><c></c><c><ref>GL_MAP1_</ref
 *! ><i>x</i> where <i>x</i> is a map type </c></r><r><c></c><c><ref
 *! >GL_MAP2_</ref><i>x</i> where <i>x</i> is a map type </c></r><r><c></c
 *! ><c><ref>GL_MULTISAMPLE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_NORMALIZE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POINT_SMOOTH</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_OFFSET_LINE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_OFFSET_FILL</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_OFFSET_POINT</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_SMOOTH</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_STIPPLE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_SAMPLE_ALPHA_TO_COVERAGE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_SAMPLE_ALPHA_TO_ONE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_SAMPLE_COVERAGE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_SCISSOR_TEST</ref> flag </c></r><r><c></c><c><ref
 *! >GL_STENCIL_TEST</ref> flag </c></r><r><c></c><c><ref
 *! >GL_TEXTURE_1D</ref> flag </c></r><r><c></c><c><ref>GL_TEXTURE_2D</ref
 *! > flag </c></r><r><c></c><c><ref>GL_TEXTURE_3D</ref> flag </c></r><r
 *! ><c></c><c>Flags <tt>GL_TEXTURE_GEN_</tt><i>x</i> where <i>x</i> is S,
 *! T, R, or Q </c></r><r><c><ref>GL_EVAL_BIT</ref></c><c><ref
 *! >GL_MAP1_</ref><i>x</i> enable bits, where <i>x</i> is a map type </c
 *! ></r><r><c></c><c><ref>GL_MAP2_</ref><i>x</i> enable bits, where <i
 *! >x</i> is a map type </c></r><r><c></c><c>1D grid endpoints and
 *! divisions </c></r><r><c></c><c>2D grid endpoints and divisions </c></r
 *! ><r><c></c><c><ref>GL_AUTO_NORMAL</ref> enable bit </c></r><r><c><ref
 *! >GL_FOG_BIT</ref></c><c><ref>GL_FOG</ref> enable bit </c></r><r><c></c
 *! ><c>Fog color </c></r><r><c></c><c>Fog density </c></r><r><c></c><c
 *! >Linear fog start </c></r><r><c></c><c>Linear fog end </c></r><r><c
 *! ></c><c>Fog index </c></r><r><c></c><c><ref>GL_FOG_MODE</ref> value
 *! </c></r><r><c><ref>GL_HINT_BIT</ref></c><c><ref
 *! >GL_PERSPECTIVE_CORRECTION_HINT</ref> setting </c></r><r><c></c><c
 *! ><ref>GL_POINT_SMOOTH_HINT</ref> setting </c></r><r><c></c><c><ref
 *! >GL_LINE_SMOOTH_HINT</ref> setting </c></r><r><c></c><c><ref
 *! >GL_POLYGON_SMOOTH_HINT</ref> setting </c></r><r><c></c><c><ref
 *! >GL_FOG_HINT</ref> setting </c></r><r><c></c><c><ref
 *! >GL_GENERATE_MIPMAP_HINT</ref> setting </c></r><r><c></c><c><ref
 *! >GL_TEXTURE_COMPRESSION_HINT</ref> setting </c></r><r><c><ref
 *! >GL_LIGHTING_BIT</ref></c><c><ref>GL_COLOR_MATERIAL</ref> enable bit
 *! </c></r><r><c></c><c><ref>GL_COLOR_MATERIAL_FACE</ref> value </c></r
 *! ><r><c></c><c>Color material parameters that are tracking the current
 *! color </c></r><r><c></c><c>Ambient scene color </c></r><r><c></c><c
 *! ><ref>GL_LIGHT_MODEL_LOCAL_VIEWER</ref> value </c></r><r><c></c><c
 *! ><ref>GL_LIGHT_MODEL_TWO_SIDE</ref> setting </c></r><r><c></c><c><ref
 *! >GL_LIGHTING</ref> enable bit </c></r><r><c></c><c>Enable bit for each
 *! light </c></r><r><c></c><c>Ambient, diffuse, and specular intensity
 *! for each light </c></r><r><c></c><c>Direction, position, exponent, and
 *! cutoff angle for each light </c></r><r><c></c><c>Constant, linear, and
 *! quadratic attenuation factors for each light </c></r><r><c></c><c
 *! >Ambient, diffuse, specular, and emissive color for each material </c
 *! ></r><r><c></c><c>Ambient, diffuse, and specular color indices for
 *! each material </c></r><r><c></c><c>Specular exponent for each material
 *! </c></r><r><c></c><c><ref>GL_SHADE_MODEL</ref> setting </c></r><r><c
 *! ><ref>GL_LINE_BIT</ref></c><c><ref>GL_LINE_SMOOTH</ref> flag </c></r
 *! ><r><c></c><c><ref>GL_LINE_STIPPLE</ref> enable bit </c></r><r><c></c
 *! ><c>Line stipple pattern and repeat counter </c></r><r><c></c><c>Line
 *! width </c></r><r><c><ref>GL_LIST_BIT</ref></c><c><ref
 *! >GL_LIST_BASE</ref> setting </c></r><r><c><ref>GL_MULTISAMPLE_BIT</ref
 *! ></c><c><ref>GL_MULTISAMPLE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_SAMPLE_ALPHA_TO_COVERAGE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_SAMPLE_ALPHA_TO_ONE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_SAMPLE_COVERAGE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_SAMPLE_COVERAGE_VALUE</ref> value </c></r><r><c></c><c><ref
 *! >GL_SAMPLE_COVERAGE_INVERT</ref> value </c></r><r><c><ref
 *! >GL_PIXEL_MODE_BIT</ref></c><c><ref>GL_RED_BIAS</ref> and <ref
 *! >GL_RED_SCALE</ref> settings </c></r><r><c></c><c><ref
 *! >GL_GREEN_BIAS</ref> and <ref>GL_GREEN_SCALE</ref> values </c></r><r
 *! ><c></c><c><ref>GL_BLUE_BIAS</ref> and <ref>GL_BLUE_SCALE</ref></c></r
 *! ><r><c></c><c><ref>GL_ALPHA_BIAS</ref> and <ref>GL_ALPHA_SCALE</ref
 *! ></c></r><r><c></c><c><ref>GL_DEPTH_BIAS</ref> and <ref
 *! >GL_DEPTH_SCALE</ref></c></r><r><c></c><c><ref>GL_INDEX_OFFSET</ref>
 *! and <ref>GL_INDEX_SHIFT</ref> values </c></r><r><c></c><c><ref
 *! >GL_MAP_COLOR</ref> and <ref>GL_MAP_STENCIL</ref> flags </c></r><r><c
 *! ></c><c><ref>GL_ZOOM_X</ref> and <ref>GL_ZOOM_Y</ref> factors </c></r
 *! ><r><c></c><c><ref>GL_READ_BUFFER</ref> setting </c></r><r><c><ref
 *! >GL_POINT_BIT</ref></c><c><ref>GL_POINT_SMOOTH</ref> flag </c></r><r
 *! ><c></c><c>Point size </c></r><r><c><ref>GL_POLYGON_BIT</ref></c><c
 *! ><ref>GL_CULL_FACE</ref> enable bit </c></r><r><c></c><c><ref
 *! >GL_CULL_FACE_MODE</ref> value </c></r><r><c></c><c><ref
 *! >GL_FRONT_FACE</ref> indicator </c></r><r><c></c><c><ref
 *! >GL_POLYGON_MODE</ref> setting </c></r><r><c></c><c><ref
 *! >GL_POLYGON_SMOOTH</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_STIPPLE</ref> enable bit </c></r><r><c></c><c><ref
 *! >GL_POLYGON_OFFSET_FILL</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_OFFSET_LINE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_OFFSET_POINT</ref> flag </c></r><r><c></c><c><ref
 *! >GL_POLYGON_OFFSET_FACTOR</ref></c></r><r><c></c><c><ref
 *! >GL_POLYGON_OFFSET_UNITS</ref></c></r><r><c><ref
 *! >GL_POLYGON_STIPPLE_BIT</ref></c><c> Polygon stipple image </c></r><r
 *! ><c><ref>GL_SCISSOR_BIT</ref></c><c><ref>GL_SCISSOR_TEST</ref> flag
 *! </c></r><r><c></c><c>Scissor box </c></r><r><c><ref
 *! >GL_STENCIL_BUFFER_BIT</ref></c><c><ref>GL_STENCIL_TEST</ref> enable
 *! bit </c></r><r><c></c><c>Stencil function and reference value </c></r
 *! ><r><c></c><c>Stencil value mask </c></r><r><c></c><c>Stencil fail,
 *! pass, and depth buffer pass actions </c></r><r><c></c><c>Stencil
 *! buffer clear value </c></r><r><c></c><c>Stencil buffer writemask </c
 *! ></r><r><c><ref>GL_TEXTURE_BIT</ref></c><c> Enable bits for the four
 *! texture coordinates </c></r><r><c></c><c>Border color for each texture
 *! image </c></r><r><c></c><c>Minification function for each texture
 *! image </c></r><r><c></c><c>Magnification function for each texture
 *! image </c></r><r><c></c><c>Texture coordinates and wrap mode for each
 *! texture image </c></r><r><c></c><c>Color and mode for each texture
 *! environment </c></r><r><c></c><c>Enable bits <tt>GL_TEXTURE_GEN_</tt
 *! ><i>x</i>, <i>x</i> is S, T, R, and Q </c></r><r><c></c><c><ref
 *! >GL_TEXTURE_GEN_MODE</ref> setting for S, T, R, and Q </c></r><r><c
 *! ></c><c><ref>glTexGen</ref> plane equations for S, T, R, and Q </c></r
 *! ><r><c></c><c>Current texture bindings (for example, <ref
 *! >GL_TEXTURE_BINDING_2D</ref>) </c></r><r><c><ref>GL_TRANSFORM_BIT</ref
 *! ></c><c> Coefficients of the six clipping planes </c></r><r><c></c><c
 *! >Enable bits for the user-definable clipping planes </c></r><r><c></c
 *! ><c><ref>GL_MATRIX_MODE</ref> value </c></r><r><c></c><c><ref
 *! >GL_NORMALIZE</ref> flag </c></r><r><c></c><c><ref
 *! >GL_RESCALE_NORMAL</ref> flag </c></r><r><c><ref>GL_VIEWPORT_BIT</ref
 *! ></c><c> Depth range (near and far) </c></r><r><c></c><c>Viewport
 *! origin and extent </c></r></matrix>@}@[glPopAttrib] restores the
 *! values of the state variables saved with the last @[glPushAttrib]
 *! command. Those not saved are left unchanged.
 *! 
 *! It is an error to push attributes onto a full stack or to pop
 *! attributes off an empty stack. In either case, the error flag is set
 *! and no other change is made to GL state.
 *! 
 *! Initially, the attribute stack is empty.
 *! 
 *! @param mask
 *! 
 *! Specifies a mask that indicates which attributes to save. Values for
 *! @i{mask@} are listed below.
 *! 
 *! @throws
 *! 
 *! @[GL_STACK_OVERFLOW] is generated if @[glPushAttrib] is called while
 *! the attribute stack is full.
 *! 
 *! @[GL_STACK_UNDERFLOW] is generated if @[glPopAttrib] is called while
 *! the attribute stack is empty.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glPushAttrib] or
 *! @[glPopAttrib] is executed between the execution of @[glBegin] and the
 *! corresponding execution of @[glEnd].
 *! 
 *! @note
 *! 
 *! Not all values for GL state can be saved on the attribute stack. For
 *! example, render mode state, and select and feedback state cannot be
 *! saved. Client state must be saved with @[glPushClientAttrib].
 *! 
 *! The depth of the attribute stack depends on the implementation, but it
 *! must be at least 16.
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, pushing and popping
 *! texture state applies to all supported texture units.
 *! 
 *! @seealso
 *! 
 *! @[glGet], @[glGetClipPlane], @[glGetError], @[glGetLight],
 *! @[glGetMap], @[glGetMaterial], @[glGetPixelMap],
 *! @[glGetPolygonStipple], @[glGetString], @[glGetTexEnv],
 *! @[glGetTexGen], @[glGetTexImage], @[glGetTexLevelParameter],
 *! @[glGetTexParameter], @[glIsEnabled], @[glPushClientAttrib]
 *! 
 */


/*! @decl void glPushClientAttrib(int mask)
 *! @decl void glPopClientAttrib()
 *! 
 *! @[glPushClientAttrib] takes one argument, a mask that indicates which
 *! groups of client-state variables to save on the client attribute
 *! stack. Symbolic constants are used to set bits in the mask. @i{mask@}
 *! is typically constructed by specifying the bitwise-or of several of
 *! these constants together. The special mask
 *! @[GL_CLIENT_ALL_ATTRIB_BITS] can be used to save all stackable client
 *! state.
 *! 
 *! The symbolic mask constants and their associated GL client state are
 *! as follows (the second column lists which attributes are saved):
 *! 
 *! @[GL_CLIENT_PIXEL_STORE_BIT] Pixel storage modes
 *! @[GL_CLIENT_VERTEX_ARRAY_BIT] Vertex arrays (and enables)
 *! 
 *! @[glPopClientAttrib] restores the values of the client-state variables
 *! saved with the last @[glPushClientAttrib]. Those not saved are left
 *! unchanged.
 *! 
 *! It is an error to push attributes onto a full client attribute stack
 *! or to pop attributes off an empty stack. In either case, the error
 *! flag is set, and no other change is made to GL state.
 *! 
 *! Initially, the client attribute stack is empty.
 *! 
 *! @param mask
 *! 
 *! Specifies a mask that indicates which attributes to save. Values for
 *! @i{mask@} are listed below.
 *! 
 *! @throws
 *! 
 *! @[GL_STACK_OVERFLOW] is generated if @[glPushClientAttrib] is called
 *! while the attribute stack is full.
 *! 
 *! @[GL_STACK_UNDERFLOW] is generated if @[glPopClientAttrib] is called
 *! while the attribute stack is empty.
 *! 
 *! @note
 *! 
 *! @[glPushClientAttrib] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Not all values for GL client state can be saved on the attribute
 *! stack. For example, select and feedback state cannot be saved.
 *! 
 *! The depth of the attribute stack depends on the implementation, but it
 *! must be at least 16.
 *! 
 *! Use @[glPushAttrib] and @[glPopAttrib] to push and restore state that
 *! is kept on the server. Only pixel storage modes and vertex array state
 *! may be pushed and popped with @[glPushClientAttrib] and
 *! @[glPopClientAttrib].
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, pushing and popping
 *! client vertex array state applies to all supported texture units, and
 *! the active client texture state.
 *! 
 *! @seealso
 *! 
 *! @[glColorPointer], @[glDisableClientState], @[glEdgeFlagPointer],
 *! @[glEnableClientState], @[glFogCoordPointer], @[glGet], @[glGetError],
 *! @[glIndexPointer], @[glNormalPointer], @[glNewList], @[glPixelStore],
 *! @[glPushAttrib], @[glTexCoordPointer], @[glVertexPointer]
 *! 
 */


/*! @decl void glPushName(int name)
 *! @decl void glPopName()
 *! 
 *! The name stack is used during selection mode to allow sets of
 *! rendering commands to be uniquely identified. It consists of an
 *! ordered set of unsigned integers and is initially empty.
 *! 
 *! @[glPushName] causes @i{name@} to be pushed onto the name stack.
 *! @[glPopName] pops one name off the top of the stack.
 *! 
 *! The maximum name stack depth is implementation-dependent; call
 *! @[GL_MAX_NAME_STACK_DEPTH] to find out the value for a particular
 *! implementation. It is an error to push a name onto a full stack or to
 *! pop a name off an empty stack. It is also an error to manipulate the
 *! name stack between the execution of @[glBegin] and the corresponding
 *! execution of @[glEnd]. In any of these cases, the error flag is set
 *! and no other change is made to GL state.
 *! 
 *! The name stack is always empty while the render mode is not
 *! @[GL_SELECT]. Calls to @[glPushName] or @[glPopName] while the render
 *! mode is not @[GL_SELECT] are ignored.
 *! 
 *! @param name
 *! 
 *! Specifies a name that will be pushed onto the name stack.
 *! 
 *! @throws
 *! 
 *! @[GL_STACK_OVERFLOW] is generated if @[glPushName] is called while the
 *! name stack is full.
 *! 
 *! @[GL_STACK_UNDERFLOW] is generated if @[glPopName] is called while the
 *! name stack is empty.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glPushName] or @[glPopName]
 *! is executed between a call to @[glBegin] and the corresponding call to
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glInitNames], @[glLoadName], @[glRenderMode], @[glSelectBuffer]
 *! 
 */


/*! @decl void glRasterPos(float|int x, float|int y, float|int|void z, @
 *! float|int|void w)
 *! @decl void glRasterPos(array(float|int) v)
 *! 
 *! The GL maintains a 3D position in window coordinates. This position,
 *! called the raster position, is used to position pixel and bitmap write
 *! operations. It is maintained with subpixel accuracy. See @[glBitmap],
 *! @[glDrawPixels], and @[glCopyPixels].
 *! 
 *! The current raster position consists of three window coordinates
 *! (@i{x@}, @i{y@}, @i{z@}), a clip coordinate value (@i{w@}), an eye
 *! coordinate distance, a valid bit, and associated color data and
 *! texture coordinates. The @i{w@} coordinate is a clip coordinate,
 *! because @i{w@} is not projected to window coordinates. The variable
 *! @i{z@} defaults to 0 and @i{w@} defaults to 1.
 *! 
 *! The object coordinates presented by @[glRasterPos] are treated just
 *! like those of a @[glVertex] command: They are transformed by the
 *! current modelview and projection matrices and passed to the clipping
 *! stage. If the vertex is not culled, then it is projected and scaled to
 *! window coordinates, which become the new current raster position, and
 *! the @[GL_CURRENT_RASTER_POSITION_VALID] flag is set. If the vertex
 *! @i{is@} culled, then the valid bit is cleared and the current raster
 *! position and associated color and texture coordinates are undefined.
 *! 
 *! The current raster position also includes some associated color data
 *! and texture coordinates. If lighting is enabled, then
 *! @[GL_CURRENT_RASTER_COLOR] (in RGBA mode) or
 *! @[GL_CURRENT_RASTER_INDEX] (in color index mode) is set to the color
 *! produced by the lighting calculation (see @[glLight], @[glLightModel],
 *! and @[glShadeModel]). If lighting is disabled, current color (in RGBA
 *! mode, state variable @[GL_CURRENT_COLOR]) or color index (in color
 *! index mode, state variable @[GL_CURRENT_INDEX]) is used to update the
 *! current raster color. @[GL_CURRENT_RASTER_SECONDARY_COLOR] (in RGBA
 *! mode) is likewise updated.
 *! 
 *! Likewise, @[GL_CURRENT_RASTER_TEXTURE_COORDS] is updated as a function
 *! of @[GL_CURRENT_TEXTURE_COORDS], based on the texture matrix and the
 *! texture generation functions (see @[glTexGen]). Finally, the distance
 *! from the origin of the eye coordinate system to the vertex as
 *! transformed by only the modelview matrix replaces
 *! @[GL_CURRENT_RASTER_DISTANCE].
 *! 
 *! Initially, the current raster position is (0, 0, 0, 1), the current
 *! raster distance is 0, the valid bit is set, the associated RGBA color
 *! is (1, 1, 1, 1), the associated color index is 1, and the associated
 *! texture coordinates are (0, 0, 0, 1). In RGBA mode,
 *! @[GL_CURRENT_RASTER_INDEX] is always 1; in color index mode, the
 *! current raster RGBA color always maintains its initial value.
 *! 
 *! @param x
 *! @param y
 *! @param z
 *! @param w
 *! 
 *! Specify the @i{x@}, @i{y@}, @i{z@}, and @i{w@} object coordinates (if
 *! present) for the raster position.
 *! 
 *! @param v
 *! 
 *! Specifies an array of two, three, or four elements, specifying @i{x@},
 *! @i{y@}, @i{z@}, and @i{w@} coordinates, respectively.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glRasterPos] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! The raster position is modified by @[glRasterPos], @[glBitmap], and
 *! @[glWindowPos].
 *! 
 *! When the raster position coordinates are invalid, drawing commands
 *! that are based on the raster position are ignored (that is, they do
 *! not result in changes to GL state).
 *! 
 *! Calling @[glDrawElements] or @[glDrawRangeElements] may leave the
 *! current color or index indeterminate. If @[glRasterPos] is executed
 *! while the current color or index is indeterminate, the current raster
 *! color or current raster index remains indeterminate.
 *! 
 *! To set a valid raster position outside the viewport, first set a valid
 *! raster position, then call @[glBitmap] with NULL as the @i{bitmap@}
 *! parameter.
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, there are distinct
 *! raster texture coordinates for each texture unit. Each texture unit's
 *! current raster texture coordinates are updated by @[glRasterPos].
 *! 
 *! @seealso
 *! 
 *! @[glBitmap], @[glCopyPixels], @[glDrawArrays], @[glDrawElements],
 *! @[glDrawRangeElements], @[glDrawPixels], @[glMultiTexCoord],
 *! @[glTexCoord], @[glTexGen], @[glVertex], @[glWindowPos]
 *! 
 */


/*! @decl void glRotate(float|int|array(float|int) angle, @
 *! float|int|void x, float|int|void y, float|int|void z)
 *! 
 *! @[glRotate] produces a rotation of @i{angle@} degrees around the
 *! vector (@i{x@}, @i{y@}, @i{z@}). The current matrix (see
 *! @[glMatrixMode]) is multiplied by a rotation matrix with the product
 *! replacing the current matrix, as if @[glMultMatrix] were called with
 *! the following matrix as its argument:
 *! 
 *! (@xml{<matrix><r><c><i>x</i><sup>2</sup>(1-<i>c</i>)+<i>c</i></c><c><i
 *! >x</i><i>y</i>(1-<i>c</i>)-<i>z</i><i>s</i></c><c><i>x</i><i>z</i
 *! >(1-<i>c</i>)+<i>y</i><i>s</i></c><c>0</c></r><r><c><i>y</i><i>x</i
 *! >(1-<i>c</i>)+<i>z</i><i>s</i></c><c><i>y</i><sup>2</sup>(1-<i>c</i
 *! >)+<i>c</i></c><c><i>y</i><i>z</i>(1-<i>c</i>)-<i>x</i><i>s</i></c><c
 *! >0</c></r><r><c><i>x</i><i>z</i>(1-<i>c</i>)-<i>y</i><i>s</i></c><c><i
 *! >y</i><i>z</i>(1-<i>c</i>)+<i>x</i><i>s</i></c><c><i>z</i><sup>2</sup
 *! >(1-<i>c</i>)+<i>c</i></c><c>0</c></r><r><c>0</c><c>0</c><c>0</c><c
 *! >1</c></r></matrix>@})
 *! 
 *! Where
 *! @i{c@}=cos(angle)
 *! ,
 *! @i{s@}=sin(angle)
 *! , and
 *! @xml{&#8741;@}(@i{x@}, @i{y@}, @i{z@})@xml{&#8741;@}=1
 *! (if not, the GL will normalize this vector).
 *! 
 *! If the matrix mode is either @[GL_MODELVIEW] or @[GL_PROJECTION], all
 *! objects drawn after @[glRotate] is called are rotated. Use
 *! @[glPushMatrix] and @[glPopMatrix] to save and restore the unrotated
 *! coordinate system.
 *! 
 *! @param angle
 *! 
 *! Specifies the angle of rotation, in degrees.
 *! 
 *! @param x
 *! @param y
 *! @param z
 *! 
 *! Specify the @i{x@}, @i{y@}, and @i{z@} coordinates of a vector,
 *! respectively.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glRotate] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! This rotation follows the right-hand rule, so if the vector (@i{x@},
 *! @i{y@}, @i{z@}) points toward the user, the rotation will be
 *! counterclockwise.
 *! 
 *! @seealso
 *! 
 *! @[glMatrixMode], @[glMultMatrix], @[glPushMatrix], @[glScale],
 *! @[glTranslate]
 *! 
 */


/*! @decl void glScale(float|int|array(float|int) x, float|int|void y, @
 *! float|int|void z)
 *! 
 *! @[glScale] produces a nonuniform scaling along the @i{x@}, @i{y@}, and
 *! @i{z@} axes. The three parameters indicate the desired scale factor
 *! along each of the three axes.
 *! 
 *! The current matrix (see @[glMatrixMode]) is multiplied by this scale
 *! matrix, and the product replaces the current matrix as if
 *! @[glMultMatrix] were called with the following matrix as its argument:
 *! 
 *! (@xml{<matrix><r><c><i>x</i></c><c>0</c><c>0</c><c>0</c></r><r><c>0</c
 *! ><c><i>y</i></c><c>0</c><c>0</c></r><r><c>0</c><c>0</c><c><i>z</i></c
 *! ><c>0</c></r><r><c>0</c><c>0</c><c>0</c><c>1</c></r></matrix>@})
 *! 
 *! If the matrix mode is either @[GL_MODELVIEW] or @[GL_PROJECTION], all
 *! objects drawn after @[glScale] is called are scaled.
 *! 
 *! Use @[glPushMatrix] and @[glPopMatrix] to save and restore the
 *! unscaled coordinate system.
 *! 
 *! @param x
 *! @param y
 *! @param z
 *! 
 *! Specify scale factors along the @i{x@}, @i{y@}, and @i{z@} axes,
 *! respectively.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glScale] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! If scale factors other than 1 are applied to the modelview matrix and
 *! lighting is enabled, lighting often appears wrong. In that case,
 *! enable automatic normalization of normals by calling @[glEnable] with
 *! the argument @[GL_NORMALIZE].
 *! 
 *! @seealso
 *! 
 *! @[glMatrixMode], @[glMultMatrix], @[glPushMatrix], @[glRotate],
 *! @[glTranslate]
 *! 
 */


/*! @decl void glScissor(int x, int y, int width, int height)
 *! 
 *! @[glScissor] defines a rectangle, called the scissor box, in window
 *! coordinates. The first two arguments, @i{x@} and @i{y@}, specify the
 *! lower left corner of the box. @i{width@} and @i{height@} specify the
 *! width and height of the box.
 *! 
 *! To enable and disable the scissor test, call @[glEnable] and
 *! @[glDisable] with argument @[GL_SCISSOR_TEST]. The test is initially
 *! disabled. While the test is enabled, only pixels that lie within the
 *! scissor box can be modified by drawing commands. Window coordinates
 *! have integer values at the shared corners of frame buffer pixels.
 *! @tt{glScissor(0,0,1,1)@} allows modification of only the lower left
 *! pixel in the window, and @tt{glScissor(0,0,0,0)@} doesn't allow
 *! modification of any pixels in the window.
 *! 
 *! When the scissor test is disabled, it is as though the scissor box
 *! includes the entire window.
 *! 
 *! @param x
 *! @param y
 *! 
 *! Specify the lower left corner of the scissor box. Initially (0, 0).
 *! 
 *! @param width
 *! @param height
 *! 
 *! Specify the width and height of the scissor box. When a GL context is
 *! first attached to a window, @i{width@} and @i{height@} are set to the
 *! dimensions of that window.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@}
 *! is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glScissor] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glEnable], @[glViewport]
 *! 
 */


/*! @decl void glStencilFunc(int func, int ref, int mask)
 *! 
 *! Stenciling, like depth-buffering, enables and disables drawing on a
 *! per-pixel basis. Stencil planes are first drawn into using GL drawing
 *! primitives, then geometry and images are rendered using the stencil
 *! planes to mask out portions of the screen. Stenciling is typically
 *! used in multipass rendering algorithms to achieve special effects,
 *! such as decals, outlining, and constructive solid geometry rendering.
 *! 
 *! The stencil test conditionally eliminates a pixel based on the outcome
 *! of a comparison between the reference value and the value in the
 *! stencil buffer. To enable and disable the test, call @[glEnable] and
 *! @[glDisable] with argument @[GL_STENCIL_TEST]. To specify actions
 *! based on the outcome of the stencil test, call @[glStencilOp] or
 *! @[glStencilOpSeparate].
 *! 
 *! There can be two separate sets of @i{func@}, @i{ref@}, and @i{mask@}
 *! parameters; one affects back-facing polygons, and the other affects
 *! front-facing polygons as well as other non-polygon primitives.
 *! @[glStencilFunc] sets both front and back stencil state to the same
 *! values. Use @[glStencilFuncSeparate] to set front and back stencil
 *! state to different values.
 *! 
 *! @i{func@} is a symbolic constant that determines the stencil
 *! comparison function. It accepts one of eight values, shown in the
 *! following list. @i{ref@} is an integer reference value that is used in
 *! the stencil comparison. It is clamped to the range
 *! [0, 2@sup{@i{n@}@}-1]
 *! , where @i{n@} is the number of bitplanes in the stencil buffer.
 *! @i{mask@} is bitwise ANDed with both the reference value and the
 *! stored stencil value, with the ANDed values participating in the
 *! comparison.
 *! 
 *! If @i{stencil@} represents the value stored in the corresponding
 *! stencil buffer location, the following list shows the effect of each
 *! comparison function that can be specified by @i{func@}. Only if the
 *! comparison succeeds is the pixel passed through to the next stage in
 *! the rasterization process (see @[glStencilOp]). All tests treat
 *! @i{stencil@} values as unsigned integers in the range
 *! [0, 2@sup{@i{n@}@}-1]
 *! , where @i{n@} is the number of bitplanes in the stencil buffer.
 *! 
 *! The following values are accepted by @i{func@}:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_NEVER</ref></c>
 *! <c><p>Always fails. </p></c></r>
 *! <r><c><ref>GL_LESS</ref></c>
 *! <c><p>Passes if ( <i>ref</i> &amp; <i>mask</i> ) &lt; ( <i>stencil</i>
 *! &amp; <i>mask</i> ). </p></c></r>
 *! <r><c><ref>GL_LEQUAL</ref></c>
 *! <c><p>Passes if ( <i>ref</i> &amp; <i>mask</i> ) &lt;= ( <i>stencil</i
 *! > &amp; <i>mask</i> ). </p></c></r>
 *! <r><c><ref>GL_GREATER</ref></c>
 *! <c><p>Passes if ( <i>ref</i> &amp; <i>mask</i> ) &gt; ( <i>stencil</i>
 *! &amp; <i>mask</i> ). </p></c></r>
 *! <r><c><ref>GL_GEQUAL</ref></c>
 *! <c><p>Passes if ( <i>ref</i> &amp; <i>mask</i> ) &gt;= ( <i>stencil</i
 *! > &amp; <i>mask</i> ). </p></c></r>
 *! <r><c><ref>GL_EQUAL</ref></c>
 *! <c><p>Passes if ( <i>ref</i> &amp; <i>mask</i> ) = ( <i>stencil</i>
 *! &amp; <i>mask</i> ). </p></c></r>
 *! <r><c><ref>GL_NOTEQUAL</ref></c>
 *! <c><p>Passes if ( <i>ref</i> &amp; <i>mask</i> ) != ( <i>stencil</i>
 *! &amp; <i>mask</i> ). </p></c></r>
 *! <r><c><ref>GL_ALWAYS</ref></c>
 *! <c><p>Always passes. </p></c></r>
 *! </matrix>@}
 *! 
 *! @param func
 *! 
 *! Specifies the test function. Eight symbolic constants are valid:
 *! @[GL_NEVER], @[GL_LESS], @[GL_LEQUAL], @[GL_GREATER], @[GL_GEQUAL],
 *! @[GL_EQUAL], @[GL_NOTEQUAL], and @[GL_ALWAYS]. The initial value is
 *! @[GL_ALWAYS].
 *! 
 *! @param ref
 *! 
 *! Specifies the reference value for the stencil test. @i{ref@} is
 *! clamped to the range
 *! [0, 2@sup{@i{n@}@}-1]
 *! , where @i{n@} is the number of bitplanes in the stencil buffer. The
 *! initial value is 0.
 *! 
 *! @param mask
 *! 
 *! Specifies a mask that is ANDed with both the reference value and the
 *! stored stencil value when the test is done. The initial value is all
 *! 1's.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{func@} is not one of the eight
 *! accepted values.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glStencilFunc] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Initially, the stencil test is disabled. If there is no stencil
 *! buffer, no stencil modification can occur and it is as if the stencil
 *! test always passes.
 *! 
 *! @[glStencilFunc] is the same as calling @[glStencilFuncSeparate] with
 *! @i{face@} set to @[GL_FRONT_AND_BACK].
 *! 
 *! @seealso
 *! 
 *! @[glAlphaFunc], @[glBlendFunc], @[glDepthFunc], @[glEnable],
 *! @[glLogicOp], @[glStencilFuncSeparate], @[glStencilMask],
 *! @[glStencilMaskSeparate], @[glStencilOp], @[glStencilOpSeparate]
 *! 
 */


/*! @decl void glStencilMask(int mask)
 *! 
 *! @[glStencilMask] controls the writing of individual bits in the
 *! stencil planes. The least significant @i{n@} bits of @i{mask@}, where
 *! @i{n@} is the number of bits in the stencil buffer, specify a mask.
 *! Where a 1 appears in the mask, it's possible to write to the
 *! corresponding bit in the stencil buffer. Where a 0 appears, the
 *! corresponding bit is write-protected. Initially, all bits are enabled
 *! for writing.
 *! 
 *! There can be two separate @i{mask@} writemasks; one affects
 *! back-facing polygons, and the other affects front-facing polygons as
 *! well as other non-polygon primitives. @[glStencilMask] sets both front
 *! and back stencil writemasks to the same values. Use
 *! @[glStencilMaskSeparate] to set front and back stencil writemasks to
 *! different values.
 *! 
 *! @param mask
 *! 
 *! Specifies a bit mask to enable and disable writing of individual bits
 *! in the stencil planes. Initially, the mask is all 1's.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glStencilMask] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glStencilMask] is the same as calling @[glStencilMaskSeparate] with
 *! @i{face@} set to @[GL_FRONT_AND_BACK].
 *! 
 *! @seealso
 *! 
 *! @[glColorMask], @[glDepthMask], @[glIndexMask], @[glStencilFunc],
 *! @[glStencilFuncSeparate], @[glStencilMaskSeparate], @[glStencilOp],
 *! @[glStencilOpSeparate]
 *! 
 */


/*! @decl void glStencilOp(int sfail, int dpfail, int dppass)
 *! 
 *! Stenciling, like depth-buffering, enables and disables drawing on a
 *! per-pixel basis. You draw into the stencil planes using GL drawing
 *! primitives, then render geometry and images, using the stencil planes
 *! to mask out portions of the screen. Stenciling is typically used in
 *! multipass rendering algorithms to achieve special effects, such as
 *! decals, outlining, and constructive solid geometry rendering.
 *! 
 *! The stencil test conditionally eliminates a pixel based on the outcome
 *! of a comparison between the value in the stencil buffer and a
 *! reference value. To enable and disable the test, call @[glEnable] and
 *! @[glDisable] with argument @[GL_STENCIL_TEST]; to control it, call
 *! @[glStencilFunc] or @[glStencilFuncSeparate].
 *! 
 *! There can be two separate sets of @i{sfail@}, @i{dpfail@}, and
 *! @i{dppass@} parameters; one affects back-facing polygons, and the
 *! other affects front-facing polygons as well as other non-polygon
 *! primitives. @[glStencilOp] sets both front and back stencil state to
 *! the same values. Use @[glStencilOpSeparate] to set front and back
 *! stencil state to different values.
 *! 
 *! @[glStencilOp] takes three arguments that indicate what happens to the
 *! stored stencil value while stenciling is enabled. If the stencil test
 *! fails, no change is made to the pixel's color or depth buffers, and
 *! @i{sfail@} specifies what happens to the stencil buffer contents. The
 *! following eight actions are possible.
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_KEEP</ref></c>
 *! <c><p>Keeps the current value. </p></c></r>
 *! <r><c><ref>GL_ZERO</ref></c>
 *! <c><p>Sets the stencil buffer value to 0. </p></c></r>
 *! <r><c><ref>GL_REPLACE</ref></c>
 *! <c><p>Sets the stencil buffer value to <i>ref</i>, as specified by
 *! <ref>glStencilFunc</ref>. </p></c></r>
 *! <r><c><ref>GL_INCR</ref></c>
 *! <c><p>Increments the current stencil buffer value. Clamps to the
 *! maximum representable unsigned value. </p></c></r>
 *! <r><c><ref>GL_INCR_WRAP</ref></c>
 *! <c><p>Increments the current stencil buffer value. Wraps stencil
 *! buffer value to zero when incrementing the maximum representable
 *! unsigned value. </p></c></r>
 *! <r><c><ref>GL_DECR</ref></c>
 *! <c><p>Decrements the current stencil buffer value. Clamps to 0. </p
 *! ></c></r>
 *! <r><c><ref>GL_DECR_WRAP</ref></c>
 *! <c><p>Decrements the current stencil buffer value. Wraps stencil
 *! buffer value to the maximum representable unsigned value when
 *! decrementing a stencil buffer value of zero. </p></c></r>
 *! <r><c><ref>GL_INVERT</ref></c>
 *! <c><p>Bitwise inverts the current stencil buffer value. </p></c></r>
 *! </matrix>@}Stencil buffer values are treated as unsigned integers.
 *! When incremented and decremented, values are clamped to 0 and
 *! 2@sup{@i{n@}@}-1
 *! , where @i{n@} is the value returned by querying @[GL_STENCIL_BITS].
 *! 
 *! The other two arguments to @[glStencilOp] specify stencil buffer
 *! actions that depend on whether subsequent depth buffer tests succeed
 *! (@i{dppass@}) or fail (@i{dpfail@}) (see @[glDepthFunc]). The actions
 *! are specified using the same eight symbolic constants as @i{sfail@}.
 *! Note that @i{dpfail@} is ignored when there is no depth buffer, or
 *! when the depth buffer is not enabled. In these cases, @i{sfail@} and
 *! @i{dppass@} specify stencil action when the stencil test fails and
 *! passes, respectively.
 *! 
 *! @param sfail
 *! 
 *! Specifies the action to take when the stencil test fails. Eight
 *! symbolic constants are accepted: @[GL_KEEP], @[GL_ZERO],
 *! @[GL_REPLACE], @[GL_INCR], @[GL_INCR_WRAP], @[GL_DECR],
 *! @[GL_DECR_WRAP], and @[GL_INVERT]. The initial value is @[GL_KEEP].
 *! 
 *! @param dpfail
 *! 
 *! Specifies the stencil action when the stencil test passes, but the
 *! depth test fails. @i{dpfail@} accepts the same symbolic constants as
 *! @i{sfail@}. The initial value is @[GL_KEEP].
 *! 
 *! @param dppass
 *! 
 *! Specifies the stencil action when both the stencil test and the depth
 *! test pass, or when the stencil test passes and either there is no
 *! depth buffer or depth testing is not enabled. @i{dppass@} accepts the
 *! same symbolic constants as @i{sfail@}. The initial value is
 *! @[GL_KEEP].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{sfail@}, @i{dpfail@}, or
 *! @i{dppass@} is any value other than the eight defined constant values.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glStencilOp] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[GL_DECR_WRAP] and @[GL_INCR_WRAP] are available only if the GL
 *! version is 1.4 or greater.
 *! 
 *! Initially the stencil test is disabled. If there is no stencil buffer,
 *! no stencil modification can occur and it is as if the stencil tests
 *! always pass, regardless of any call to @[glStencilOp].
 *! 
 *! @[glStencilOp] is the same as calling @[glStencilOpSeparate] with
 *! @i{face@} set to @[GL_FRONT_AND_BACK].
 *! 
 *! @seealso
 *! 
 *! @[glAlphaFunc], @[glBlendFunc], @[glDepthFunc], @[glEnable],
 *! @[glLogicOp], @[glStencilFunc], @[glStencilFuncSeparate],
 *! @[glStencilMask], @[glStencilMaskSeparate], @[glStencilOpSeparate]
 *! 
 */


/*! @decl void glTexCoord(float|int s, float|int|void t, float|int|void r, @
 *! float|int|void q)
 *! @decl void glTexCoord(array(float|int) v)
 *! 
 *! @[glTexCoord] specifies texture coordinates in one, two, three, or
 *! four dimensions. @[glTexCoord] sets the current texture coordinates to
 *! (@i{s@}, 0, 0, 1); a call to @[glTexCoord] sets them to (@i{s@},
 *! @i{t@}, 0, 1). Similarly, @[glTexCoord] specifies the texture
 *! coordinates as (@i{s@}, @i{t@}, @i{r@}, 1), and @[glTexCoord] defines
 *! all four components explicitly as (@i{s@}, @i{t@}, @i{r@}, @i{q@}).
 *! 
 *! The current texture coordinates are part of the data that is
 *! associated with each vertex and with the current raster position.
 *! Initially, the values for @i{s@}, @i{t@}, @i{r@}, and @i{q@} are (0,
 *! 0, 0, 1).
 *! 
 *! @param s
 *! @param t
 *! @param r
 *! @param q
 *! 
 *! Specify @i{s@}, @i{t@}, @i{r@}, and @i{q@} texture coordinates. Not
 *! all parameters are present in all forms of the command.
 *! 
 *! @param v
 *! 
 *! Specifies an array of one, two, three, or four elements, which in turn
 *! specify the @i{s@}, @i{t@}, @i{r@}, and @i{q@} texture coordinates.
 *! 
 *! @note
 *! 
 *! The current texture coordinates can be updated at any time. In
 *! particular, @[glTexCoord] can be called between a call to @[glBegin]
 *! and the corresponding call to @[glEnd].
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, @[glTexCoord]
 *! always updates texture unit @[GL_TEXTURE0].
 *! 
 *! @seealso
 *! 
 *! @[glMultiTexCoord], @[glTexCoordPointer], @[glVertex]
 *! 
 */


/*! @decl void glTexEnv(int target, int pname, @
 *! float|int|array(float|int) param)
 *! 
 *! A texture environment specifies how texture values are interpreted
 *! when a fragment is textured. When @i{target@} is
 *! @[GL_TEXTURE_FILTER_CONTROL], @i{pname@} must be
 *! @[GL_TEXTURE_LOD_BIAS]. When @i{target@} is @[GL_TEXTURE_ENV],
 *! @i{pname@} can be @[GL_TEXTURE_ENV_MODE], @[GL_TEXTURE_ENV_COLOR],
 *! @[GL_COMBINE_RGB], @[GL_COMBINE_ALPHA], @[GL_RGB_SCALE],
 *! @[GL_ALPHA_SCALE], @[GL_SRC0_RGB], @[GL_SRC1_RGB], @[GL_SRC2_RGB],
 *! @[GL_SRC0_ALPHA], @[GL_SRC1_ALPHA], or @[GL_SRC2_ALPHA].
 *! 
 *! If @i{pname@} is @[GL_TEXTURE_ENV_MODE], then @i{params@} is (or
 *! points to) the symbolic name of a texture function. Six texture
 *! functions may be specified: @[GL_ADD], @[GL_MODULATE], @[GL_DECAL],
 *! @[GL_BLEND], @[GL_REPLACE], or @[GL_COMBINE].
 *! 
 *! The following table shows the correspondence of filtered texture
 *! values @i{R@}@sub{@i{t@}@}, @i{G@}@sub{@i{t@}@}, @i{B@}@sub{@i{t@}@},
 *! @i{A@}@sub{@i{t@}@}, @i{L@}@sub{@i{t@}@}, @i{I@}@sub{@i{t@}@} to
 *! texture source components. @i{C@}@sub{@i{s@}@} and @i{A@}@sub{@i{s@}@}
 *! are used by the texture functions described below.
 *! 
 *! @xml{<matrix><r><c>Texture Base Internal Format </c><c><i>C</i><sub><i
 *! >s</i></sub></c><c><i>A</i><sub><i>s</i></sub></c></r><r><c><ref
 *! >GL_ALPHA</ref></c><c> (0, 0, 0) </c><c><i>A</i><sub><i>t</i></sub></c
 *! ></r><r><c><ref>GL_LUMINANCE</ref></c><c> ( <i>L</i><sub><i>t</i></sub
 *! >, <i>L</i><sub><i>t</i></sub>, <i>L</i><sub><i>t</i></sub> ) </c><c>1
 *! </c></r><r><c><ref>GL_LUMINANCE_ALPHA</ref></c><c> ( <i>L</i><sub><i
 *! >t</i></sub>, <i>L</i><sub><i>t</i></sub>, <i>L</i><sub><i>t</i></sub>
 *! ) </c><c><i>A</i><sub><i>t</i></sub></c></r><r><c><ref
 *! >GL_INTENSITY</ref></c><c> ( <i>I</i><sub><i>t</i></sub>, <i>I</i><sub
 *! ><i>t</i></sub>, <i>I</i><sub><i>t</i></sub> ) </c><c><i>I</i><sub><i
 *! >t</i></sub></c></r><r><c><ref>GL_RGB</ref></c><c> ( <i>R</i><sub><i
 *! >t</i></sub>, <i>G</i><sub><i>t</i></sub>, <i>B</i><sub><i>t</i></sub>
 *! ) </c><c>1 </c></r><r><c><ref>GL_RGBA</ref></c><c> ( <i>R</i><sub><i
 *! >t</i></sub>, <i>G</i><sub><i>t</i></sub>, <i>B</i><sub><i>t</i></sub>
 *! ) </c><c><i>A</i><sub><i>t</i></sub></c></r></matrix>@} A texture
 *! function acts on the fragment to be textured using the texture image
 *! value that applies to the fragment (see @[glTexParameter]) and
 *! produces an RGBA color for that fragment. The following table shows
 *! how the RGBA color is produced for each of the first five texture
 *! functions that can be chosen. @i{C@} is a triple of color values (RGB)
 *! and @i{A@} is the associated alpha value. RGBA values extracted from a
 *! texture image are in the range [0,1]. The subscript @i{p@} refers to
 *! the color computed from the previous texture stage (or the incoming
 *! fragment if processing texture stage 0), the subscript @i{s@} to the
 *! texture source color, the subscript @i{c@} to the texture environment
 *! color, and the subscript @i{v@} indicates a value produced by the
 *! texture function.
 *! 
 *! @xml{<matrix><r><c>Texture Base Internal Format </c><c><ref>Value</ref
 *! ></c><c><ref>GL_REPLACE</ref> Function </c><c><ref>GL_MODULATE</ref>
 *! Function </c><c><ref>GL_DECAL</ref> Function </c><c><ref>GL_BLEND</ref
 *! > Function </c><c><ref>GL_ADD</ref> Function </c></r><r><c><ref
 *! >GL_ALPHA</ref></c><c> <i>C</i><sub><i>v</i></sub>=</c><c> <i>C</i
 *! ><sub><i>p</i></sub></c><c> <i>C</i><sub><i>p</i></sub></c><c>
 *! undefined </c><c><i>C</i><sub><i>p</i></sub></c><c> <i>C</i><sub><i
 *! >p</i></sub></c></r><r><c></c><c> <i>A</i><sub><i>v</i></sub>=</c><c>
 *! <i>A</i><sub><i>s</i></sub></c><c> <i>A</i><sub><i>p</i></sub><i>A</i
 *! ><sub><i>s</i></sub></c><c></c><c> <i>A</i><sub><i>v</i></sub>=<i>A</i
 *! ><sub><i>p</i></sub><i>A</i><sub><i>s</i></sub></c><c> <i>A</i><sub><i
 *! >p</i></sub><i>A</i><sub><i>s</i></sub></c></r><r><c><ref
 *! >GL_LUMINANCE</ref></c><c> <i>C</i><sub><i>v</i></sub>=</c><c> <i>C</i
 *! ><sub><i>s</i></sub></c><c> <i>C</i><sub><i>p</i></sub><i>C</i><sub><i
 *! >s</i></sub></c><c> undefined </c><c><i>C</i><sub><i>p</i></sub>(1-<i
 *! >C</i><sub><i>s</i></sub>)+<i>C</i><sub><i>c</i></sub><i>C</i><sub><i
 *! >s</i></sub></c><c> <i>C</i><sub><i>p</i></sub>+<i>C</i><sub><i>s</i
 *! ></sub></c></r><r><c> (or 1) </c><c><i>A</i><sub><i>v</i></sub>=</c><c
 *! > <i>A</i><sub><i>p</i></sub></c><c> <i>A</i><sub><i>p</i></sub></c><c
 *! ></c><c> <i>A</i><sub><i>p</i></sub></c><c> <i>A</i><sub><i>p</i></sub
 *! ></c></r><r><c><ref>GL_LUMINANCE_ALPHA</ref></c><c> <i>C</i><sub><i
 *! >v</i></sub>=</c><c> <i>C</i><sub><i>s</i></sub></c><c> <i>C</i><sub
 *! ><i>p</i></sub><i>C</i><sub><i>s</i></sub></c><c> undefined </c><c><i
 *! >C</i><sub><i>p</i></sub>(1-<i>C</i><sub><i>s</i></sub>)+<i>C</i><sub
 *! ><i>c</i></sub><i>C</i><sub><i>s</i></sub></c><c> <i>C</i><sub><i>p</i
 *! ></sub>+<i>C</i><sub><i>s</i></sub></c></r><r><c> (or 2) </c><c><i
 *! >A</i><sub><i>v</i></sub>=</c><c> <i>A</i><sub><i>s</i></sub></c><c>
 *! <i>A</i><sub><i>p</i></sub><i>A</i><sub><i>s</i></sub></c><c></c><c>
 *! <i>A</i><sub><i>p</i></sub><i>A</i><sub><i>s</i></sub></c><c> <i>A</i
 *! ><sub><i>p</i></sub><i>A</i><sub><i>s</i></sub></c></r><r><c><ref
 *! >GL_INTENSITY</ref></c><c> <i>C</i><sub><i>v</i></sub>=</c><c> <i>C</i
 *! ><sub><i>s</i></sub></c><c> <i>C</i><sub><i>p</i></sub><i>C</i><sub><i
 *! >s</i></sub></c><c> undefined </c><c><i>C</i><sub><i>p</i></sub>(1-<i
 *! >C</i><sub><i>s</i></sub>)+<i>C</i><sub><i>c</i></sub><i>C</i><sub><i
 *! >s</i></sub></c><c> <i>C</i><sub><i>p</i></sub>+<i>C</i><sub><i>s</i
 *! ></sub></c></r><r><c></c><c> <i>A</i><sub><i>v</i></sub>=</c><c> <i
 *! >A</i><sub><i>s</i></sub></c><c> <i>A</i><sub><i>p</i></sub><i>A</i
 *! ><sub><i>s</i></sub></c><c></c><c> <i>A</i><sub><i>p</i></sub>(1-<i
 *! >A</i><sub><i>s</i></sub>)+<i>A</i><sub><i>c</i></sub><i>A</i><sub><i
 *! >s</i></sub></c><c> <i>A</i><sub><i>p</i></sub>+<i>A</i><sub><i>s</i
 *! ></sub></c></r><r><c><ref>GL_RGB</ref></c><c> <i>C</i><sub><i>v</i
 *! ></sub>=</c><c> <i>C</i><sub><i>s</i></sub></c><c> <i>C</i><sub><i
 *! >p</i></sub><i>C</i><sub><i>s</i></sub></c><c> <i>C</i><sub><i>s</i
 *! ></sub></c><c> <i>C</i><sub><i>p</i></sub>(1-<i>C</i><sub><i>s</i
 *! ></sub>)+<i>C</i><sub><i>c</i></sub><i>C</i><sub><i>s</i></sub></c><c>
 *! <i>C</i><sub><i>p</i></sub>+<i>C</i><sub><i>s</i></sub></c></r><r><c>
 *! (or 3) </c><c><i>A</i><sub><i>v</i></sub>=</c><c> <i>A</i><sub><i>p</i
 *! ></sub></c><c> <i>A</i><sub><i>p</i></sub></c><c> <i>A</i><sub><i>p</i
 *! ></sub></c><c> <i>A</i><sub><i>p</i></sub></c><c> <i>A</i><sub><i>p</i
 *! ></sub></c></r><r><c><ref>GL_RGBA</ref></c><c> <i>C</i><sub><i>v</i
 *! ></sub>=</c><c> <i>C</i><sub><i>s</i></sub></c><c> <i>C</i><sub><i
 *! >p</i></sub><i>C</i><sub><i>s</i></sub></c><c> <i>C</i><sub><i>p</i
 *! ></sub>(1-<i>A</i><sub><i>s</i></sub>)+<i>C</i><sub><i>s</i></sub><i
 *! >A</i><sub><i>s</i></sub></c><c> <i>C</i><sub><i>p</i></sub>(1-<i>C</i
 *! ><sub><i>s</i></sub>)+<i>C</i><sub><i>c</i></sub><i>C</i><sub><i>s</i
 *! ></sub></c><c> <i>C</i><sub><i>p</i></sub>+<i>C</i><sub><i>s</i></sub
 *! ></c></r><r><c> (or 4) </c><c><i>A</i><sub><i>v</i></sub>=</c><c> <i
 *! >A</i><sub><i>s</i></sub></c><c> <i>A</i><sub><i>p</i></sub><i>A</i
 *! ><sub><i>s</i></sub></c><c> <i>A</i><sub><i>p</i></sub></c><c> <i>A</i
 *! ><sub><i>p</i></sub><i>A</i><sub><i>s</i></sub></c><c> <i>A</i><sub><i
 *! >p</i></sub><i>A</i><sub><i>s</i></sub></c></r></matrix>@} If
 *! @i{pname@} is @[GL_TEXTURE_ENV_MODE], and @i{params@} is
 *! @[GL_COMBINE], the form of the texture function depends on the values
 *! of @[GL_COMBINE_RGB] and @[GL_COMBINE_ALPHA].
 *! 
 *! The following describes how the texture sources, as specified by
 *! @[GL_SRC0_RGB], @[GL_SRC1_RGB], @[GL_SRC2_RGB], @[GL_SRC0_ALPHA],
 *! @[GL_SRC1_ALPHA], and @[GL_SRC2_ALPHA], are combined to produce a
 *! final texture color. In the following tables, @[GL_SRC0_c] is
 *! represented by Arg0, @[GL_SRC1_c] is represented by Arg1, and
 *! @[GL_SRC2_c] is represented by Arg2.
 *! 
 *! @[GL_COMBINE_RGB] accepts any of @[GL_REPLACE], @[GL_MODULATE],
 *! @[GL_ADD], @[GL_ADD_SIGNED], @[GL_INTERPOLATE], @[GL_SUBTRACT],
 *! @[GL_DOT3_RGB], or @[GL_DOT3_RGBA].
 *! 
 *! @xml{<matrix><r><c><b><ref>GL_COMBINE_RGB</ref></b></c><c><b> Texture
 *! Function </b></c></r><r><c><ref>GL_REPLACE</ref></c><c>Arg0</c></r><r
 *! ><c><ref>GL_MODULATE</ref></c><c> Arg0&#215;Arg1</c></r><r><c><ref
 *! >GL_ADD</ref></c><c> Arg0+Arg1</c></r><r><c><ref>GL_ADD_SIGNED</ref
 *! ></c><c> Arg0+Arg1-0.5</c></r><r><c><ref>GL_INTERPOLATE</ref></c><c>
 *! Arg0&#215;Arg2+Arg1&#215;(1-Arg2)</c></r><r><c><ref>GL_SUBTRACT</ref
 *! ></c><c> Arg0-Arg1</c></r><r><c><ref>GL_DOT3_RGB</ref> or <ref
 *! >GL_DOT3_RGBA</ref></c><c> 4&#215;(((Arg0<sub><i>r</i></sub
 *! >-0.5)&#215;(Arg1<sub><i>r</i></sub>-0.5))+((Arg0<sub><i>g</i></sub
 *! >-0.5)&#215;(Arg1<sub><i>g</i></sub>-0.5))+((Arg0<sub><i>b</i></sub
 *! >-0.5)&#215;(Arg1<sub><i>b</i></sub>-0.5)))</c></r></matrix>@} The
 *! scalar results for @[GL_DOT3_RGB] and @[GL_DOT3_RGBA] are placed into
 *! each of the 3 (RGB) or 4 (RGBA) components on output.
 *! 
 *! Likewise, @[GL_COMBINE_ALPHA] accepts any of @[GL_REPLACE],
 *! @[GL_MODULATE], @[GL_ADD], @[GL_ADD_SIGNED], @[GL_INTERPOLATE], or
 *! @[GL_SUBTRACT]. The following table describes how alpha values are
 *! combined:
 *! 
 *! @xml{<matrix><r><c><b><ref>GL_COMBINE_ALPHA</ref></b></c><c><b>
 *! Texture Function </b></c></r><r><c><ref>GL_REPLACE</ref></c><c>Arg0</c
 *! ></r><r><c><ref>GL_MODULATE</ref></c><c> Arg0&#215;Arg1</c></r><r><c
 *! ><ref>GL_ADD</ref></c><c> Arg0+Arg1</c></r><r><c><ref
 *! >GL_ADD_SIGNED</ref></c><c> Arg0+Arg1-0.5</c></r><r><c><ref
 *! >GL_INTERPOLATE</ref></c><c> Arg0&#215;Arg2+Arg1&#215;(1-Arg2)</c></r
 *! ><r><c><ref>GL_SUBTRACT</ref></c><c> Arg0-Arg1</c></r></matrix>@} In
 *! the following tables, the value @i{C@}@sub{@i{s@}@} represents the
 *! color sampled from the currently bound texture, @i{C@}@sub{@i{c@}@}
 *! represents the constant texture-environment color, @i{C@}@sub{@i{f@}@}
 *! represents the primary color of the incoming fragment, and
 *! @i{C@}@sub{@i{p@}@} represents the color computed from the previous
 *! texture stage or @i{C@}@sub{@i{f@}@} if processing texture stage 0.
 *! Likewise, @i{A@}@sub{@i{s@}@}, @i{A@}@sub{@i{c@}@},
 *! @i{A@}@sub{@i{f@}@}, and @i{A@}@sub{@i{p@}@} represent the respective
 *! alpha values.
 *! 
 *! The following table describes the values assigned to Arg0, Arg1, and
 *! Arg2 based upon the RGB sources and operands:
 *! 
 *! @xml{<matrix><r><c><b><ref>GL_SRCn_RGB</ref></b></c><c><b><ref
 *! >GL_OPERANDn_RGB</ref></b></c><c><b> Argument Value </b></c></r><r><c
 *! ><ref>GL_TEXTURE</ref></c><c><ref>GL_SRC_COLOR</ref></c><c><i>C</i
 *! ><sub><i>s</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_COLOR</ref></c><c> 1-<i>C</i><sub><i>s</i></sub></c
 *! ></r><r><c></c><c><ref>GL_SRC_ALPHA</ref></c><c><i>A</i><sub><i>s</i
 *! ></sub></c></r><r><c></c><c><ref>GL_ONE_MINUS_SRC_ALPHA</ref></c><c>
 *! 1-<i>A</i><sub><i>s</i></sub></c></r><r><c><ref>GL_TEXTUREn</ref></c
 *! ><c><ref>GL_SRC_COLOR</ref></c><c><i>C</i><sub><i>s</i></sub></c></r
 *! ><r><c></c><c><ref>GL_ONE_MINUS_SRC_COLOR</ref></c><c> 1-<i>C</i><sub
 *! ><i>s</i></sub></c></r><r><c></c><c><ref>GL_SRC_ALPHA</ref></c><c><i
 *! >A</i><sub><i>s</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_ALPHA</ref></c><c> 1-<i>A</i><sub><i>s</i></sub></c
 *! ></r><r><c><ref>GL_CONSTANT</ref></c><c><ref>GL_SRC_COLOR</ref></c><c
 *! ><i>C</i><sub><i>c</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_COLOR</ref></c><c> 1-<i>C</i><sub><i>c</i></sub></c
 *! ></r><r><c></c><c><ref>GL_SRC_ALPHA</ref></c><c><i>A</i><sub><i>c</i
 *! ></sub></c></r><r><c></c><c><ref>GL_ONE_MINUS_SRC_ALPHA</ref></c><c>
 *! 1-<i>A</i><sub><i>c</i></sub></c></r><r><c><ref>GL_PRIMARY_COLOR</ref
 *! ></c><c><ref>GL_SRC_COLOR</ref></c><c><i>C</i><sub><i>f</i></sub></c
 *! ></r><r><c></c><c><ref>GL_ONE_MINUS_SRC_COLOR</ref></c><c> 1-<i>C</i
 *! ><sub><i>f</i></sub></c></r><r><c></c><c><ref>GL_SRC_ALPHA</ref></c><c
 *! ><i>A</i><sub><i>f</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_ALPHA</ref></c><c> 1-<i>A</i><sub><i>f</i></sub></c
 *! ></r><r><c><ref>GL_PREVIOUS</ref></c><c><ref>GL_SRC_COLOR</ref></c><c
 *! ><i>C</i><sub><i>p</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_COLOR</ref></c><c> 1-<i>C</i><sub><i>p</i></sub></c
 *! ></r><r><c></c><c><ref>GL_SRC_ALPHA</ref></c><c><i>A</i><sub><i>p</i
 *! ></sub></c></r><r><c></c><c><ref>GL_ONE_MINUS_SRC_ALPHA</ref></c><c>
 *! 1-<i>A</i><sub><i>p</i></sub></c></r></matrix>@} For @[GL_TEXTUREn]
 *! sources, @i{C@}@sub{@i{s@}@} and @i{A@}@sub{@i{s@}@} represent the
 *! color and alpha, respectively, produced from texture stage @i{n@}.
 *! 
 *! The follow table describes the values assigned to Arg0, Arg1, and Arg2
 *! based upon the alpha sources and operands:
 *! 
 *! @xml{<matrix><r><c><b><ref>GL_SRCn_ALPHA</ref></b></c><c><b><ref
 *! >GL_OPERANDn_ALPHA</ref></b></c><c><b> Argument Value </b></c></r><r
 *! ><c><ref>GL_TEXTURE</ref></c><c><ref>GL_SRC_ALPHA</ref></c><c><i>A</i
 *! ><sub><i>s</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_ALPHA</ref></c><c> 1-<i>A</i><sub><i>s</i></sub></c
 *! ></r><r><c><ref>GL_TEXTUREn</ref></c><c><ref>GL_SRC_ALPHA</ref></c><c
 *! ><i>A</i><sub><i>s</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_ALPHA</ref></c><c> 1-<i>A</i><sub><i>s</i></sub></c
 *! ></r><r><c><ref>GL_CONSTANT</ref></c><c><ref>GL_SRC_ALPHA</ref></c><c
 *! ><i>A</i><sub><i>c</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_ALPHA</ref></c><c> 1-<i>A</i><sub><i>c</i></sub></c
 *! ></r><r><c><ref>GL_PRIMARY_COLOR</ref></c><c><ref>GL_SRC_ALPHA</ref
 *! ></c><c><i>A</i><sub><i>f</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_ALPHA</ref></c><c> 1-<i>A</i><sub><i>f</i></sub></c
 *! ></r><r><c><ref>GL_PREVIOUS</ref></c><c><ref>GL_SRC_ALPHA</ref></c><c
 *! ><i>A</i><sub><i>p</i></sub></c></r><r><c></c><c><ref
 *! >GL_ONE_MINUS_SRC_ALPHA</ref></c><c> 1-<i>A</i><sub><i>p</i></sub></c
 *! ></r></matrix>@} The RGB and alpha results of the texture function are
 *! multiplied by the values of @[GL_RGB_SCALE] and @[GL_ALPHA_SCALE],
 *! respectively, and clamped to the range [0, 1].
 *! 
 *! If @i{pname@} is @[GL_TEXTURE_ENV_COLOR], @i{params@} is an array that
 *! holds an RGBA color consisting of four values. Integer color
 *! components are interpreted linearly such that the most positive
 *! integer maps to 1.0, and the most negative integer maps to -1.0. The
 *! values are clamped to the range [0,1] when they are specified.
 *! @i{C@}@sub{@i{c@}@} takes these four values.
 *! 
 *! If @i{pname@} is @[GL_TEXTURE_LOD_BIAS], the value specified is added
 *! to the texture level-of-detail parameter, that selects which mipmap,
 *! or mipmaps depending upon the selected @[GL_TEXTURE_MIN_FILTER], will
 *! be sampled.
 *! 
 *! @[GL_TEXTURE_ENV_MODE] defaults to @[GL_MODULATE] and
 *! @[GL_TEXTURE_ENV_COLOR] defaults to (0, 0, 0, 0).
 *! 
 *! If @i{target@} is @[GL_POINT_SPRITE] and @i{pname@} is
 *! @[GL_COORD_REPLACE], the boolean value specified is used to either
 *! enable or disable point sprite texture coordinate replacement. The
 *! default value is @[GL_FALSE].
 *! 
 *! @param target
 *! 
 *! Specifies a texture environment. May be @[GL_TEXTURE_ENV],
 *! @[GL_TEXTURE_FILTER_CONTROL] or @[GL_POINT_SPRITE].
 *! 
 *! @param pname
 *! 
 *! Specifies the symbolic name of a single-valued texture environment
 *! parameter. May be either @[GL_TEXTURE_ENV_MODE],
 *! @[GL_TEXTURE_LOD_BIAS], @[GL_COMBINE_RGB], @[GL_COMBINE_ALPHA],
 *! @[GL_SRC0_RGB], @[GL_SRC1_RGB], @[GL_SRC2_RGB], @[GL_SRC0_ALPHA],
 *! @[GL_SRC1_ALPHA], @[GL_SRC2_ALPHA], @[GL_OPERAND0_RGB],
 *! @[GL_OPERAND1_RGB], @[GL_OPERAND2_RGB], @[GL_OPERAND0_ALPHA],
 *! @[GL_OPERAND1_ALPHA], @[GL_OPERAND2_ALPHA], @[GL_RGB_SCALE],
 *! @[GL_ALPHA_SCALE], or @[GL_COORD_REPLACE].
 *! 
 *! @param param
 *! 
 *! Specifies a single symbolic constant, one of @[GL_ADD],
 *! @[GL_ADD_SIGNED], @[GL_INTERPOLATE], @[GL_MODULATE], @[GL_DECAL],
 *! @[GL_BLEND], @[GL_REPLACE], @[GL_SUBTRACT], @[GL_COMBINE],
 *! @[GL_TEXTURE], @[GL_CONSTANT], @[GL_PRIMARY_COLOR], @[GL_PREVIOUS],
 *! @[GL_SRC_COLOR], @[GL_ONE_MINUS_SRC_COLOR], @[GL_SRC_ALPHA],
 *! @[GL_ONE_MINUS_SRC_ALPHA], a single boolean value for the point sprite
 *! texture coordinate replacement, a single floating-point value for the
 *! texture level-of-detail bias, or 1.0, 2.0, or 4.0 when specifying the
 *! @[GL_RGB_SCALE] or @[GL_ALPHA_SCALE].
 *! 
 *! @param target
 *! 
 *! Specifies a texture environment. May be either @[GL_TEXTURE_ENV], or
 *! @[GL_TEXTURE_FILTER_CONTROL].
 *! 
 *! @param pname
 *! 
 *! Specifies the symbolic name of a texture environment parameter.
 *! Accepted values are @[GL_TEXTURE_ENV_MODE], @[GL_TEXTURE_ENV_COLOR],
 *! or @[GL_TEXTURE_LOD_BIAS].
 *! 
 *! @param params
 *! 
 *! Specifies a pointer to a parameter array that contains either a single
 *! symbolic constant, single floating-point number, or an RGBA color.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated when @i{target@} or @i{pname@} is not
 *! one of the accepted defined values, or when @i{params@} should have a
 *! defined constant value (based on the value of @i{pname@}) and does
 *! not.
 *! 
 *! @[GL_INVALID_VALUE] is generated if the @i{params@} value for
 *! @[GL_RGB_SCALE] or @[GL_ALPHA_SCALE] are not one of 1.0, 2.0, or 4.0.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glTexEnv] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[GL_REPLACE] may only be used if the GL version is 1.1 or greater.
 *! 
 *! @[GL_TEXTURE_FILTER_CONTROL] and @[GL_TEXTURE_LOD_BIAS] may only be
 *! used if the GL version is 1.4 or greater.
 *! 
 *! @[GL_COMBINE] mode and its associated constants may only be used if
 *! the GL version is 1.3 or greater.
 *! 
 *! @[GL_TEXTUREn] may only be used if the GL version is 1.4 or greater.
 *! 
 *! Internal formats other than 1, 2, 3, or 4 may only be used if the GL
 *! version is 1.1 or greater.
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, @[glTexEnv] controls
 *! the texture environment for the current active texture unit, selected
 *! by @[glActiveTexture].
 *! 
 *! @[GL_POINT_SPRITE] and @[GL_COORD_REPLACE] are available only if the
 *! GL version is 2.0 or greater.
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glCopyPixels], @[glCopyTexImage1D],
 *! @[glCopyTexImage2D], @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glCopyTexSubImage3D], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexImage3D], @[glTexParameter], @[glTexSubImage1D],
 *! @[glTexSubImage2D], @[glTexSubImage3D]
 *! 
 */


/*! @decl void glTexGen(int coord, int pname, @
 *! float|int|array(float|int) param)
 *! 
 *! @[glTexGen] selects a texture-coordinate generation function or
 *! supplies coefficients for one of the functions. @i{coord@} names one
 *! of the (@i{s@}, @i{t@}, @i{r@}, @i{q@}) texture coordinates; it must
 *! be one of the symbols @[GL_S], @[GL_T], @[GL_R], or @[GL_Q].
 *! @i{pname@} must be one of three symbolic constants:
 *! @[GL_TEXTURE_GEN_MODE], @[GL_OBJECT_PLANE], or @[GL_EYE_PLANE]. If
 *! @i{pname@} is @[GL_TEXTURE_GEN_MODE], then @i{params@} chooses a mode,
 *! one of @[GL_OBJECT_LINEAR], @[GL_EYE_LINEAR], @[GL_SPHERE_MAP],
 *! @[GL_NORMAL_MAP], or @[GL_REFLECTION_MAP]. If @i{pname@} is either
 *! @[GL_OBJECT_PLANE] or @[GL_EYE_PLANE], @i{params@} contains
 *! coefficients for the corresponding texture generation function.
 *! 
 *! If the texture generation function is @[GL_OBJECT_LINEAR], the
 *! function
 *! 
 *! @i{g@}=@i{p@}@sub{1@}@xml{&#215;@}@i{x@}@sub{@i{o@}@}+@i{p@}@sub{2@}@xml{&#215;@}@i{y@}@sub{@i{o@}@}+@i{p@}@sub{3@}@xml{&#215;@}@i{z@}@sub{@i{o@}@}+@i{p@}@sub{4@}@xml{&#215;@}@i{w@}@sub{@i{o@}@}
 *! 
 *! is used, where @i{g@} is the value computed for the coordinate named
 *! in @i{coord@}, @i{p@}@sub{1@}, @i{p@}@sub{2@}, @i{p@}@sub{3@}, and
 *! @i{p@}@sub{4@} are the four values supplied in @i{params@}, and
 *! @i{x@}@sub{@i{o@}@}, @i{y@}@sub{@i{o@}@}, @i{z@}@sub{@i{o@}@}, and
 *! @i{w@}@sub{@i{o@}@} are the object coordinates of the vertex. This
 *! function can be used, for example, to texture-map terrain using sea
 *! level as a reference plane (defined by @i{p@}@sub{1@}, @i{p@}@sub{2@},
 *! @i{p@}@sub{3@}, and @i{p@}@sub{4@}). The altitude of a terrain vertex
 *! is computed by the @[GL_OBJECT_LINEAR] coordinate generation function
 *! as its distance from sea level; that altitude can then be used to
 *! index the texture image to map white snow onto peaks and green grass
 *! onto foothills.
 *! 
 *! If the texture generation function is @[GL_EYE_LINEAR], the function
 *! 
 *! @i{g@}=@i{p@}@sub{1@}@sup{@xml{&#8243;@}@}@xml{&#215;@}@i{x@}@sub{@i{e@}@}+@i{p@}@sub{2@}@sup{@xml{&#8243;@}@}@xml{&#215;@}@i{y@}@sub{@i{e@}@}+@i{p@}@sub{3@}@sup{@xml{&#8243;@}@}@xml{&#215;@}@i{z@}@sub{@i{e@}@}+@i{p@}@sub{4@}@sup{@xml{&#8243;@}@}@xml{&#215;@}@i{w@}@sub{@i{e@}@}
 *! 
 *! is used, where
 *! 
 *! (@i{p@}@sub{1@}@sup{@xml{&#8243;@}@}@i{p@}@sub{2@}@sup{@xml{&#8243;@}@}@i{p@}@sub{3@}@sup{@xml{&#8243;@}@}@i{p@}@sub{4@}@sup{@xml{&#8243;@}@})=(@i{p@}@sub{1@}@i{p@}@sub{2@}@i{p@}@sub{3@}@i{p@}@sub{4@})@i{M@}@sup{-1@}
 *! 
 *! and @i{x@}@sub{@i{e@}@}, @i{y@}@sub{@i{e@}@}, @i{z@}@sub{@i{e@}@}, and
 *! @i{w@}@sub{@i{e@}@} are the eye coordinates of the vertex,
 *! @i{p@}@sub{1@}, @i{p@}@sub{2@}, @i{p@}@sub{3@}, and @i{p@}@sub{4@} are
 *! the values supplied in @i{params@}, and @i{M@} is the modelview matrix
 *! when @[glTexGen] is invoked. If @i{M@} is poorly conditioned or
 *! singular, texture coordinates generated by the resulting function may
 *! be inaccurate or undefined.
 *! 
 *! Note that the values in @i{params@} define a reference plane in eye
 *! coordinates. The modelview matrix that is applied to them may not be
 *! the same one in effect when the polygon vertices are transformed. This
 *! function establishes a field of texture coordinates that can produce
 *! dynamic contour lines on moving objects.
 *! 
 *! If the texture generation function is @[GL_SPHERE_MAP] and @i{coord@}
 *! is either @[GL_S] or @[GL_T], @i{s@} and @i{t@} texture coordinates
 *! are generated as follows. Let @i{u@} be the unit vector pointing from
 *! the origin to the polygon vertex (in eye coordinates). Let @i{n@} sup
 *! prime be the current normal, after transformation to eye coordinates.
 *! Let
 *! 
 *! @i{f@}=(@i{f@}@sub{@i{x@}@}@i{f@}@sub{@i{y@}@}@i{f@}@sub{@i{z@}@})@sup{@i{T@}@}
 *! be the reflection vector such that
 *! 
 *! @i{f@}=@i{u@}-2@i{n@}@sup{@xml{&#8243;@}@}@i{n@}@sup{@xml{&#8243;@}@}@sup{@i{T@}@}@i{u@}
 *! 
 *! Finally, let
 *! @i{m@}=2@xml{&#8730;@}(@i{f@}@sub{@i{x@}@}@sup{2@}+@i{f@}@sub{@i{y@}@}@sup{2@}+(@i{f@}@sub{@i{z@}@}+1)@sup{2@})
 *! . Then the values assigned to the @i{s@} and @i{t@} texture
 *! coordinates are
 *! 
 *! @i{s@}=@i{f@}@sub{@i{x@}@}/@i{m@}+1/2
 *! 
 *! @i{t@}=@i{f@}@sub{@i{y@}@}/@i{m@}+1/2
 *! 
 *! To enable or disable a texture-coordinate generation function, call
 *! @[glEnable] or @[glDisable] with one of the symbolic
 *! texture-coordinate names (@[GL_TEXTURE_GEN_S], @[GL_TEXTURE_GEN_T],
 *! @[GL_TEXTURE_GEN_R], or @[GL_TEXTURE_GEN_Q]) as the argument. When
 *! enabled, the specified texture coordinate is computed according to the
 *! generating function associated with that coordinate. When disabled,
 *! subsequent vertices take the specified texture coordinate from the
 *! current set of texture coordinates. Initially, all texture generation
 *! functions are set to @[GL_EYE_LINEAR] and are disabled. Both @i{s@}
 *! plane equations are (1, 0, 0, 0), both @i{t@} plane equations are (0,
 *! 1, 0, 0), and all @i{r@} and @i{q@} plane equations are (0, 0, 0, 0).
 *! 
 *! When the @tt{ARB_multitexture@} extension is supported, @[glTexGen]
 *! sets the texture generation parameters for the currently active
 *! texture unit, selected with @[glActiveTexture].
 *! 
 *! @param coord
 *! 
 *! Specifies a texture coordinate. Must be one of @[GL_S], @[GL_T],
 *! @[GL_R], or @[GL_Q].
 *! 
 *! @param pname
 *! 
 *! Specifies the symbolic name of the texture-coordinate generation
 *! function. Must be @[GL_TEXTURE_GEN_MODE].
 *! 
 *! @param param
 *! 
 *! Specifies a single-valued texture generation parameter, one of
 *! @[GL_OBJECT_LINEAR], @[GL_EYE_LINEAR], @[GL_SPHERE_MAP],
 *! @[GL_NORMAL_MAP], or @[GL_REFLECTION_MAP].
 *! 
 *! @param coord
 *! 
 *! Specifies a texture coordinate. Must be one of @[GL_S], @[GL_T],
 *! @[GL_R], or @[GL_Q].
 *! 
 *! @param pname
 *! 
 *! Specifies the symbolic name of the texture-coordinate generation
 *! function or function parameters. Must be @[GL_TEXTURE_GEN_MODE],
 *! @[GL_OBJECT_PLANE], or @[GL_EYE_PLANE].
 *! 
 *! @param params
 *! 
 *! Specifies an array of texture generation parameters. If @i{pname@} is
 *! @[GL_TEXTURE_GEN_MODE], then the array must contain a single symbolic
 *! constant, one of @[GL_OBJECT_LINEAR], @[GL_EYE_LINEAR],
 *! @[GL_SPHERE_MAP], @[GL_NORMAL_MAP], or @[GL_REFLECTION_MAP].
 *! Otherwise, @i{params@} holds the coefficients for the
 *! texture-coordinate generation function specified by @i{pname@}.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated when @i{coord@} or @i{pname@} is not
 *! an accepted defined value, or when @i{pname@} is
 *! @[GL_TEXTURE_GEN_MODE] and @i{params@} is not an accepted defined
 *! value.
 *! 
 *! @[GL_INVALID_ENUM] is generated when @i{pname@} is
 *! @[GL_TEXTURE_GEN_MODE], @i{params@} is @[GL_SPHERE_MAP], and
 *! @i{coord@} is either @[GL_R] or @[GL_Q].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glTexGen] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glCopyPixels], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glCopyTexSubImage3D], @[glTexEnv], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexImage3D], @[glTexParameter], @[glTexSubImage1D],
 *! @[glTexSubImage2D], @[glTexSubImage3D]
 *! 
 */




/*! @decl void glEvalMesh1(int mode, int i1, int i2)
 *! @decl void glEvalMesh2(int mode, int i1, int i2, int j1, int j2)
 *! 
 *! @[glMapGrid] and @[glEvalMesh1] and @[glEvalMesh2] are used in tandem
 *! to efficiently generate and evaluate a series of evenly-spaced map
 *! domain values. @[glEvalMesh1] and @[glEvalMesh2] steps through the
 *! integer domain of a one- or two-dimensional grid, whose range is the
 *! domain of the evaluation maps specified by @[glMap1] and @[glMap2].
 *! @i{mode@} determines whether the resulting vertices are connected as
 *! points, lines, or filled polygons.
 *! 
 *! In the one-dimensional case, @[glEvalMesh1], the mesh is generated as
 *! if the following code fragment were executed:
 *! 
 *! @code
 *! 
 *! glBegin( @i{type@} );
 *! for ( i = @i{i1@}; i <= @i{i2@}; i += 1 )
 *!    glEvalCoord1( i@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@} );
 *! glEnd();
 *! @endcode
 *! where
 *! 
 *! @xml{&#916;@}@i{u@}=(@i{u@}@sub{2@}-@i{u@}@sub{1@})/@i{n@}
 *! 
 *! and @i{n@}, @i{u@}@sub{1@}, and @i{u@}@sub{2@} are the arguments to
 *! the most recent @[glMapGrid] command. @i{type@} is @[GL_POINTS] if
 *! @i{mode@} is @[GL_POINT], or @[GL_LINES] if @i{mode@} is @[GL_LINE].
 *! 
 *! The one absolute numeric requirement is that if
 *! @i{i@}=@i{n@}
 *! , then the value computed from
 *! @i{i@}@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@}
 *! is exactly @i{u@}@sub{2@}.
 *! 
 *! In the two-dimensional case, @[glEvalMesh2], let .cp
 *! @xml{&#916;@}@i{u@}=(@i{u@}@sub{2@}-@i{u@}@sub{1@})/@i{n@}
 *! 
 *! @xml{&#916;@}@i{v@}=(@i{v@}@sub{2@}-@i{v@}@sub{1@})/@i{m@}
 *! 
 *! where @i{n@}, @i{u@}@sub{1@}, @i{u@}@sub{2@}, @i{m@}, @i{v@}@sub{1@},
 *! and @i{v@}@sub{2@} are the arguments to the most recent @[glMapGrid]
 *! command. Then, if @i{mode@} is @[GL_FILL], the @[glEvalMesh2] command
 *! is equivalent to:
 *! 
 *! @code
 *! 
 *! for ( j = @i{j1@}; j < @i{j2@}; j += 1 ) {
 *!    glBegin( GL_QUAD_STRIP );
 *!    for ( i = @i{i1@}; i <= @i{i2@}; i += 1 ) {
 *!       glEvalCoord2( i@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@},j@xml{&#183;@}@xml{&#916;@}@i{v@}+@i{v@}@sub{1@} );
 *!       glEvalCoord2( i@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@},(j+1)@xml{&#183;@}@xml{&#916;@}@i{v@}+@i{v@}@sub{1@} );
 *!    }
 *!    glEnd();
 *! }
 *! @endcode
 *! 
 *! If @i{mode@} is @[GL_LINE], then a call to @[glEvalMesh2] is
 *! equivalent to:
 *! 
 *! @code
 *! 
 *! for ( j = @i{j1@}; j <= @i{j2@}; j += 1 ) {
 *!    glBegin( GL_LINE_STRIP );
 *!    for ( i = @i{i1@}; i <= @i{i2@}; i += 1 )
 *!       glEvalCoord2( i@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@},j@xml{&#183;@}@xml{&#916;@}@i{v@}+@i{v@}@sub{1@} );
 *!    glEnd();
 *! }
 *! 
 *! for ( i = @i{i1@};  i <= @i{i2@}; i += 1 ) {
 *!    glBegin( GL_LINE_STRIP );
 *!    for ( j = @i{j1@}; j <= @i{j1@}; j += 1 )
 *!       glEvalCoord2( i@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@},j@xml{&#183;@}@xml{&#916;@}@i{v@}+@i{v@}@sub{1@} );
 *!    glEnd();
 *! }
 *! @endcode
 *! 
 *! And finally, if @i{mode@} is @[GL_POINT], then a call to
 *! @[glEvalMesh2] is equivalent to:
 *! 
 *! @code
 *! 
 *! glBegin( GL_POINTS );
 *! for ( j = @i{j1@}; j <= @i{j2@}; j += 1 )
 *!    for ( i = @i{i1@}; i <= @i{i2@}; i += 1 )
 *!       glEvalCoord2( i@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@},j@xml{&#183;@}@xml{&#916;@}@i{v@}+@i{v@}@sub{1@} );
 *! glEnd();
 *! @endcode
 *! 
 *! In all three cases, the only absolute numeric requirements are that if
 *! @i{i@}=@i{n@}
 *! , then the value computed from
 *! @i{i@}@xml{&#183;@}@xml{&#916;@}@i{u@}+@i{u@}@sub{1@}
 *! is exactly @i{u@}@sub{2@}, and if
 *! @i{j@}=@i{m@}
 *! , then the value computed from
 *! @i{j@}@xml{&#183;@}@xml{&#916;@}@i{v@}+@i{v@}@sub{1@}
 *! is exactly @i{v@}@sub{2@}.
 *! 
 *! @param mode
 *! 
 *! In @[glEvalMesh1], specifies whether to compute a one-dimensional mesh
 *! of points or lines. Symbolic constants @[GL_POINT] and @[GL_LINE] are
 *! accepted.
 *! 
 *! @param i1
 *! @param i2
 *! 
 *! Specify the first and last integer values for grid domain variable
 *! @i{i@}.
 *! 
 *! @param mode
 *! 
 *! In @[glEvalMesh2], specifies whether to compute a two-dimensional mesh
 *! of points, lines, or polygons. Symbolic constants @[GL_POINT],
 *! @[GL_LINE], and @[GL_FILL] are accepted.
 *! 
 *! @param i1
 *! @param i2
 *! 
 *! Specify the first and last integer values for grid domain variable
 *! @i{i@}.
 *! 
 *! @param j1
 *! @param j2
 *! 
 *! Specify the first and last integer values for grid domain variable
 *! @i{j@}.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glEvalMesh1] and
 *! @[glEvalMesh2] is executed between the execution of @[glBegin] and the
 *! corresponding execution of @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glBegin], @[glEvalCoord], @[glEvalPoint], @[glMap1], @[glMap2],
 *! @[glMapGrid]
 *! 
 */


/*! @decl void glTexImage2D(int target, int level, int internalFormat, @
 *! object|mapping(string:object) width, @
 *! object|mapping(string:object) height, int border, @
 *! object|mapping(string:object) format, @
 *! object|mapping(string:object) type, @
 *! object|mapping(string:object) data)
 *! 
 *! Texturing maps a portion of a specified texture image onto each
 *! graphical primitive for which texturing is enabled. To enable and
 *! disable two-dimensional texturing, call @[glEnable] and @[glDisable]
 *! with argument @[GL_TEXTURE_2D]. To enable and disable texturing using
 *! cube-mapped texture, call @[glEnable] and @[glDisable] with argument
 *! @[GL_TEXTURE_CUBE_MAP].
 *! 
 *! To define texture images, call @[glTexImage2D]. The arguments describe
 *! the parameters of the texture image, such as height, width, width of
 *! the border, level-of-detail number (see @[glTexParameter]), and number
 *! of color components provided. The last three arguments describe how
 *! the image is represented in memory; they are identical to the pixel
 *! formats used for @[glDrawPixels].
 *! 
 *! If @i{target@} is @[GL_PROXY_TEXTURE_2D] or
 *! @[GL_PROXY_TEXTURE_CUBE_MAP], no data is read from @i{data@}, but all
 *! of the texture image state is recalculated, checked for consistency,
 *! and checked against the implementation's capabilities. If the
 *! implementation cannot handle a texture of the requested texture size,
 *! it sets all of the image state to 0, but does not generate an error
 *! (see @[glGetError]). To query for an entire mipmap array, use an image
 *! array level greater than or equal to 1.
 *! 
 *! If @i{target@} is @[GL_TEXTURE_2D], or one of the
 *! @[GL_TEXTURE_CUBE_MAP] targets, data is read from @i{data@} as a
 *! sequence of signed or unsigned bytes, shorts, or longs, or
 *! single-precision floating-point values, depending on @i{type@}. These
 *! values are grouped into sets of one, two, three, or four values,
 *! depending on @i{format@}, to form elements. If @i{type@} is
 *! @[GL_BITMAP], the data is considered as a string of unsigned bytes
 *! (and @i{format@} must be @[GL_COLOR_INDEX]). Each data byte is treated
 *! as eight 1-bit elements, with bit ordering determined by
 *! @[GL_UNPACK_LSB_FIRST] (see @[glPixelStore]).
 *! 
 *! If a non-zero named buffer object is bound to the
 *! @[GL_PIXEL_UNPACK_BUFFER] target (see @[glBindBuffer]) while a texture
 *! image is specified, @i{data@} is treated as a byte offset into the
 *! buffer object's data store.
 *! 
 *! The first element corresponds to the lower left corner of the texture
 *! image. Subsequent elements progress left-to-right through the
 *! remaining texels in the lowest row of the texture image, and then in
 *! successively higher rows of the texture image. The final element
 *! corresponds to the upper right corner of the texture image.
 *! 
 *! @i{format@} determines the composition of each element in @i{data@}.
 *! It can assume one of these symbolic values:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_COLOR_INDEX</ref></c>
 *! <c><p>Each element is a single value, a color index. The GL converts
 *! it to fixed point (with an unspecified number of zero bits to the
 *! right of the binary point), shifted left or right depending on the
 *! value and sign of <ref>GL_INDEX_SHIFT</ref>, and added to <ref
 *! >GL_INDEX_OFFSET</ref> (see <ref>glPixelTransfer</ref>). The resulting
 *! index is converted to a set of color components using the <ref
 *! >GL_PIXEL_MAP_I_TO_R</ref>, <ref>GL_PIXEL_MAP_I_TO_G</ref>, <ref
 *! >GL_PIXEL_MAP_I_TO_B</ref>, and <ref>GL_PIXEL_MAP_I_TO_A</ref> tables,
 *! and clamped to the range [0,1]. </p></c></r>
 *! <r><c><ref>GL_RED</ref></c>
 *! <c><p>Each element is a single red component. The GL converts it to
 *! floating point and assembles it into an RGBA element by attaching 0
 *! for green and blue, and 1 for alpha. Each component is then multiplied
 *! by the signed scale factor <tt>GL_<i>c</i>_SCALE</tt>, added to the
 *! signed bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the range [0,1]
 *! (see <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_GREEN</ref></c>
 *! <c><p>Each element is a single green component. The GL converts it to
 *! floating point and assembles it into an RGBA element by attaching 0
 *! for red and blue, and 1 for alpha. Each component is then multiplied
 *! by the signed scale factor <tt>GL_<i>c</i>_SCALE</tt>, added to the
 *! signed bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the range [0,1]
 *! (see <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_BLUE</ref></c>
 *! <c><p>Each element is a single blue component. The GL converts it to
 *! floating point and assembles it into an RGBA element by attaching 0
 *! for red and green, and 1 for alpha. Each component is then multiplied
 *! by the signed scale factor <tt>GL_<i>c</i>_SCALE</tt>, added to the
 *! signed bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the range [0,1]
 *! (see <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_ALPHA</ref></c>
 *! <c><p>Each element is a single alpha component. The GL converts it to
 *! floating point and assembles it into an RGBA element by attaching 0
 *! for red, green, and blue. Each component is then multiplied by the
 *! signed scale factor <tt>GL_<i>c</i>_SCALE</tt>, added to the signed
 *! bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the range [0,1] (see
 *! <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_INTENSITY</ref></c>
 *! <c><p>Each element is a single intensity value. The GL converts it to
 *! floating point, then assembles it into an RGBA element by replicating
 *! the intensity value three times for red, green, blue, and alpha. Each
 *! component is then multiplied by the signed scale factor <tt>GL_<i>c</i
 *! >_SCALE</tt>, added to the signed bias <tt>GL_<i>c</i>_BIAS</tt>, and
 *! clamped to the range [0,1] (see <ref>glPixelTransfer</ref>). </p
 *! ></c></r>
 *! <r><c><ref>GL_RGB</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BGR</ref></c>
 *! <c><p>Each element is an RGB triple. The GL converts it to floating
 *! point and assembles it into an RGBA element by attaching 1 for alpha.
 *! Each component is then multiplied by the signed scale factor <tt>GL_<i
 *! >c</i>_SCALE</tt>, added to the signed bias <tt>GL_<i>c</i>_BIAS</tt>,
 *! and clamped to the range [0,1] (see <ref>glPixelTransfer</ref>). </p
 *! ></c></r>
 *! <r><c><ref>GL_RGBA</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BGRA</ref></c>
 *! <c><p>Each element contains all four components. Each component is
 *! multiplied by the signed scale factor <tt>GL_<i>c</i>_SCALE</tt>,
 *! added to the signed bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the
 *! range [0,1] (see <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_LUMINANCE</ref></c>
 *! <c><p>Each element is a single luminance value. The GL converts it to
 *! floating point, then assembles it into an RGBA element by replicating
 *! the luminance value three times for red, green, and blue and attaching
 *! 1 for alpha. Each component is then multiplied by the signed scale
 *! factor <tt>GL_<i>c</i>_SCALE</tt>, added to the signed bias <tt>GL_<i
 *! >c</i>_BIAS</tt>, and clamped to the range [0,1] (see <ref
 *! >glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_LUMINANCE_ALPHA</ref></c>
 *! <c><p>Each element is a luminance/alpha pair. The GL converts it to
 *! floating point, then assembles it into an RGBA element by replicating
 *! the luminance value three times for red, green, and blue. Each
 *! component is then multiplied by the signed scale factor <tt>GL_<i>c</i
 *! >_SCALE</tt>, added to the signed bias <tt>GL_<i>c</i>_BIAS</tt>, and
 *! clamped to the range [0,1] (see <ref>glPixelTransfer</ref>). </p
 *! ></c></r>
 *! <r><c><ref>GL_DEPTH_COMPONENT</ref></c>
 *! <c><p>Each element is a single depth value. The GL converts it to
 *! floating point, multiplies by the signed scale factor <ref
 *! >GL_DEPTH_SCALE</ref>, adds the signed bias <ref>GL_DEPTH_BIAS</ref>,
 *! and clamps to the range [0,1] (see <ref>glPixelTransfer</ref>). </p
 *! ></c></r>
 *! </matrix>@}Refer to the @[glDrawPixels] reference page for a
 *! description of the acceptable values for the @i{type@} parameter.
 *! 
 *! If an application wants to store the texture at a certain resolution
 *! or in a certain format, it can request the resolution and format with
 *! @i{internalFormat@}. The GL will choose an internal representation
 *! that closely approximates that requested by @i{internalFormat@}, but
 *! it may not match exactly. (The representations specified by
 *! @[GL_LUMINANCE], @[GL_LUMINANCE_ALPHA], @[GL_RGB], and @[GL_RGBA] must
 *! match exactly. The numeric values 1, 2, 3, and 4 may also be used to
 *! specify the above representations.)
 *! 
 *! If the @i{internalFormat@} parameter is one of the generic compressed
 *! formats, @[GL_COMPRESSED_ALPHA], @[GL_COMPRESSED_INTENSITY],
 *! @[GL_COMPRESSED_LUMINANCE], @[GL_COMPRESSED_LUMINANCE_ALPHA],
 *! @[GL_COMPRESSED_RGB], or @[GL_COMPRESSED_RGBA], the GL will replace
 *! the internal format with the symbolic constant for a specific internal
 *! format and compress the texture before storage. If no corresponding
 *! internal format is available, or the GL can not compress that image
 *! for any reason, the internal format is instead replaced with a
 *! corresponding base internal format.
 *! 
 *! If the @i{internalFormat@} parameter is @[GL_SRGB], @[GL_SRGB8],
 *! @[GL_SRGB_ALPHA], @[GL_SRGB8_ALPHA8], @[GL_SLUMINANCE],
 *! @[GL_SLUMINANCE8], @[GL_SLUMINANCE_ALPHA], or
 *! @[GL_SLUMINANCE8_ALPHA8], the texture is treated as if the red, green,
 *! blue, or luminance components are encoded in the sRGB color space. Any
 *! alpha component is left unchanged. The conversion from the sRGB
 *! encoded component @i{c@}@sub{@i{s@}@} to a linear component
 *! @i{c@}@sub{@i{l@}@} is:
 *! 
 *! @i{c@}@sub{@i{l@}@}={@xml{<matrix><r><c><i>c</i><sub><i>s</i></sub
 *! >/12.92</c><c> if <i>c</i><sub><i>s</i></sub>&#8804;0.04045</c></r><r
 *! ><c>(<i>c</i><sub><i>s</i></sub>+0.055/1.055)<sup>2.4</sup></c><c> if
 *! <i>c</i><sub><i>s</i></sub>&gt;0.04045</c></r></matrix>@}
 *! 
 *! Assume @i{c@}@sub{@i{s@}@} is the sRGB component in the range [0,1].
 *! 
 *! Use the @[GL_PROXY_TEXTURE_2D] or @[GL_PROXY_TEXTURE_CUBE_MAP] target
 *! to try out a resolution and format. The implementation will update and
 *! recompute its best match for the requested storage resolution and
 *! format. To then query this state, call @[glGetTexLevelParameter]. If
 *! the texture cannot be accommodated, texture state is set to 0.
 *! 
 *! A one-component texture image uses only the red component of the RGBA
 *! color extracted from @i{data@}. A two-component image uses the R and A
 *! values. A three-component image uses the R, G, and B values. A
 *! four-component image uses all of the RGBA components.
 *! 
 *! Depth textures can be treated as LUMINANCE, INTENSITY or ALPHA
 *! textures during texture filtering and application. Image-based
 *! shadowing can be enabled by comparing texture r coordinates to depth
 *! texture values to generate a boolean result. See @[glTexParameter] for
 *! details on texture comparison.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture. Must be @[GL_TEXTURE_2D],
 *! @[GL_PROXY_TEXTURE_2D], @[GL_TEXTURE_CUBE_MAP_POSITIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z], or @[GL_PROXY_TEXTURE_CUBE_MAP].
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number. Level 0 is the base image level.
 *! Level @i{n@} is the @i{n@}th mipmap reduction image.
 *! 
 *! @param internalFormat
 *! 
 *! Specifies the number of color components in the texture. Must be 1, 2,
 *! 3, or 4, or one of the following symbolic constants: @[GL_ALPHA],
 *! @[GL_ALPHA4], @[GL_ALPHA8], @[GL_ALPHA12], @[GL_ALPHA16],
 *! @[GL_COMPRESSED_ALPHA], @[GL_COMPRESSED_LUMINANCE],
 *! @[GL_COMPRESSED_LUMINANCE_ALPHA], @[GL_COMPRESSED_INTENSITY],
 *! @[GL_COMPRESSED_RGB], @[GL_COMPRESSED_RGBA], @[GL_DEPTH_COMPONENT],
 *! @[GL_DEPTH_COMPONENT16], @[GL_DEPTH_COMPONENT24],
 *! @[GL_DEPTH_COMPONENT32], @[GL_LUMINANCE], @[GL_LUMINANCE4],
 *! @[GL_LUMINANCE8], @[GL_LUMINANCE12], @[GL_LUMINANCE16],
 *! @[GL_LUMINANCE_ALPHA], @[GL_LUMINANCE4_ALPHA4],
 *! @[GL_LUMINANCE6_ALPHA2], @[GL_LUMINANCE8_ALPHA8],
 *! @[GL_LUMINANCE12_ALPHA4], @[GL_LUMINANCE12_ALPHA12],
 *! @[GL_LUMINANCE16_ALPHA16], @[GL_INTENSITY], @[GL_INTENSITY4],
 *! @[GL_INTENSITY8], @[GL_INTENSITY12], @[GL_INTENSITY16],
 *! @[GL_R3_G3_B2], @[GL_RGB], @[GL_RGB4], @[GL_RGB5], @[GL_RGB8],
 *! @[GL_RGB10], @[GL_RGB12], @[GL_RGB16], @[GL_RGBA], @[GL_RGBA2],
 *! @[GL_RGBA4], @[GL_RGB5_A1], @[GL_RGBA8], @[GL_RGB10_A2], @[GL_RGBA12],
 *! @[GL_RGBA16], @[GL_SLUMINANCE], @[GL_SLUMINANCE8],
 *! @[GL_SLUMINANCE_ALPHA], @[GL_SLUMINANCE8_ALPHA8], @[GL_SRGB],
 *! @[GL_SRGB8], @[GL_SRGB_ALPHA], or @[GL_SRGB8_ALPHA8].
 *! 
 *! @param width
 *! 
 *! Specifies the width of the texture image including the border if any.
 *! If the GL version does not support non-power-of-two sizes, this value
 *! must be
 *! 2@sup{@i{n@}@}+2(border)
 *! for some integer @i{n@}. All implementations support texture images
 *! that are at least 64 texels wide.
 *! 
 *! @param height
 *! 
 *! Specifies the height of the texture image including the border if any.
 *! If the GL version does not support non-power-of-two sizes, this value
 *! must be
 *! 2@sup{@i{m@}@}+2(border)
 *! for some integer @i{m@}. All implementations support texture images
 *! that are at least 64 texels high.
 *! 
 *! @param border
 *! 
 *! Specifies the width of the border. Must be either 0 or 1.
 *! 
 *! @param format
 *! 
 *! Specifies the format of the pixel data. The following symbolic values
 *! are accepted: @[GL_COLOR_INDEX], @[GL_RED], @[GL_GREEN], @[GL_BLUE],
 *! @[GL_ALPHA], @[GL_RGB], @[GL_BGR], @[GL_RGBA], @[GL_BGRA],
 *! @[GL_LUMINANCE], and @[GL_LUMINANCE_ALPHA].
 *! 
 *! @param type
 *! 
 *! Specifies the data type of the pixel data. The following symbolic
 *! values are accepted: @[GL_UNSIGNED_BYTE], @[GL_BYTE], @[GL_BITMAP],
 *! @[GL_UNSIGNED_SHORT], @[GL_SHORT], @[GL_UNSIGNED_INT], @[GL_INT],
 *! @[GL_FLOAT], @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], and @[GL_UNSIGNED_INT_2_10_10_10_REV].
 *! 
 *! @param data
 *! 
 *! Specifies a pointer to the image data in memory.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is not
 *! @[GL_TEXTURE_2D], @[GL_PROXY_TEXTURE_2D],
 *! @[GL_PROXY_TEXTURE_CUBE_MAP], @[GL_TEXTURE_CUBE_MAP_POSITIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z],
 *! or @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z].
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is one of the six cube
 *! map 2D image targets and the width and height parameters are not
 *! equal.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not a type constant.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *! @i{format@} is not @[GL_COLOR_INDEX].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{width@} or @i{height@} is less
 *! than 0 or greater than 2 + @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if @i{level@} is greater than
 *! log@sub{2@}(max)
 *! , where @i{max@} is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{internalFormat@} is not 1, 2,
 *! 3, 4, or one of the accepted resolution and format symbolic constants.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{width@} or @i{height@} is less
 *! than 0 or greater than 2 + @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if non-power-of-two textures are not
 *! supported and the @i{width@} or @i{height@} cannot be represented as
 *! 2@sup{@i{k@}@}+2(border)
 *! for some integer value of @i{k@}.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{border@} is not 0 or 1.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], or @[GL_UNSIGNED_SHORT_5_6_5_REV] and
 *! @i{format@} is not @[GL_RGB].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], or @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! and @i{format@} is neither @[GL_RGBA] nor @[GL_BGRA].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{target@} is not
 *! @[GL_TEXTURE_2D] or @[GL_PROXY_TEXTURE_2D] and @i{internalFormat@} is
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], or @[GL_DEPTH_COMPONENT32].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is
 *! @[GL_DEPTH_COMPONENT] and @i{internalFormat@} is not
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], or @[GL_DEPTH_COMPONENT32].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{internalFormat@} is
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], or @[GL_DEPTH_COMPONENT32], and @i{format@}
 *! is not @[GL_DEPTH_COMPONENT].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the buffer
 *! object's data store is currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the data would be
 *! unpacked from the buffer object such that the memory reads required
 *! would exceed the data store size.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and @i{data@} is not
 *! evenly divisible into the number of bytes needed to store in memory a
 *! datum indicated by @i{type@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glTexImage2D] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Texturing has no effect in color index mode.
 *! 
 *! If the @tt{ARB_imaging@} extension is supported, RGBA elements may
 *! also be processed by the imaging pipeline. The following stages may be
 *! applied to an RGBA color before color component clamping to the range
 *! [0, 1]:
 *! 
 *! @xml{<matrix>
 *! <r><c>1. Color component replacement by the color table specified
 *! for</c>
 *! <c><p><ref>GL_COLOR_TABLE</ref>, if enabled. See <ref
 *! >glColorTable</ref>. </p></c></r>
 *! <r><c>2. Two-dimensional Convolution filtering, if enabled.</c>
 *! <c><p>See <ref>glConvolutionFilter1D</ref>. </p><p>If a convolution
 *! filter changes the <i>width</i> of the texture (by processing with a
 *! <ref>GL_CONVOLUTION_BORDER_MODE</ref> of <ref>GL_REDUCE</ref>, for
 *! example), and the GL does not support non-power-of-two textures, the
 *! <i>width</i> must 2<sup><i>n</i></sup>+2(border), for some integer <i
 *! >n</i>, and <i>height</i> must be 2<sup><i>m</i></sup>+2(border), for
 *! some integer <i>m</i>, after filtering. </p></c></r>
 *! <r><c>3. RGBA components may be multiplied by <ref
 *! >GL_POST_CONVOLUTION_c_SCALE</ref>,</c>
 *! <c><p>and added to <ref>GL_POST_CONVOLUTION_c_BIAS</ref>, if enabled.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c>4. Color component replacement by the color table specified
 *! for</c>
 *! <c><p><ref>GL_POST_CONVOLUTION_COLOR_TABLE</ref>, if enabled. See <ref
 *! >glColorTable</ref>. </p></c></r>
 *! <r><c>5. Transformation by the color matrix.</c>
 *! <c><p>See <ref>glMatrixMode</ref>. </p></c></r>
 *! <r><c>6. RGBA components may be multiplied by <ref
 *! >GL_POST_COLOR_MATRIX_c_SCALE</ref>,</c>
 *! <c><p>and added to <ref>GL_POST_COLOR_MATRIX_c_BIAS</ref>, if enabled.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c>7. Color component replacement by the color table specified
 *! for</c>
 *! <c><p><ref>GL_POST_COLOR_MATRIX_COLOR_TABLE</ref>, if enabled. See
 *! <ref>glColorTable</ref>. </p></c></r>
 *! </matrix>@}The texture image can be represented by the same data
 *! formats as the pixels in a @[glDrawPixels] command, except that
 *! @[GL_STENCIL_INDEX] cannot be used. @[glPixelStore] and
 *! @[glPixelTransfer] modes affect texture images in exactly the way they
 *! affect @[glDrawPixels].
 *! 
 *! @[glTexImage2D] and @[GL_PROXY_TEXTURE_2D] are available only if the
 *! GL version is 1.1 or greater.
 *! 
 *! Internal formats other than 1, 2, 3, or 4 may be used only if the GL
 *! version is 1.1 or greater.
 *! 
 *! In GL version 1.1 or greater, @i{data@} may be a null pointer. In this
 *! case, texture memory is allocated to accommodate a texture of width
 *! @i{width@} and height @i{height@}. You can then download subtextures
 *! to initialize this texture memory. The image is undefined if the user
 *! tries to apply an uninitialized portion of the texture image to a
 *! primitive.
 *! 
 *! Formats @[GL_BGR], and @[GL_BGRA] and types @[GL_UNSIGNED_BYTE_3_3_2],
 *! @[GL_UNSIGNED_BYTE_2_3_3_REV], @[GL_UNSIGNED_SHORT_5_6_5],
 *! @[GL_UNSIGNED_SHORT_5_6_5_REV], @[GL_UNSIGNED_SHORT_4_4_4_4],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4_REV], @[GL_UNSIGNED_SHORT_5_5_5_1],
 *! @[GL_UNSIGNED_SHORT_1_5_5_5_REV], @[GL_UNSIGNED_INT_8_8_8_8],
 *! @[GL_UNSIGNED_INT_8_8_8_8_REV], @[GL_UNSIGNED_INT_10_10_10_2], and
 *! @[GL_UNSIGNED_INT_2_10_10_10_REV] are available only if the GL version
 *! is 1.2 or greater.
 *! 
 *! When the @tt{ARB_multitexture@} extension is supported or the GL
 *! version is 1.3 or greater, @[glTexImage2D] specifies the
 *! two-dimensional texture for the current texture unit, specified with
 *! @[glActiveTexture].
 *! 
 *! @[GL_TEXTURE_CUBE_MAP] and @[GL_PROXY_TEXTURE_CUBE_MAP] are available
 *! only if the GL version is 1.3 or greater.
 *! 
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], and @[GL_DEPTH_COMPONENT32] are available
 *! only if the GL version is 1.4 or greater.
 *! 
 *! Non-power-of-two textures are supported if the GL version is 2.0 or
 *! greater, or if the implementation exports the
 *! @[GL_ARB_texture_non_power_of_two] extension.
 *! 
 *! The @[GL_SRGB], @[GL_SRGB8], @[GL_SRGB_ALPHA], @[GL_SRGB8_ALPHA8],
 *! @[GL_SLUMINANCE], @[GL_SLUMINANCE8], @[GL_SLUMINANCE_ALPHA], and
 *! @[GL_SLUMINANCE8_ALPHA8] internal formats are only available if the GL
 *! version is 2.1 or greater.
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glColorTable], @[glConvolutionFilter2D],
 *! @[glCopyPixels], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glCopyTexSubImage3D], @[glDrawPixels], @[glMatrixMode],
 *! @[glPixelStore], @[glPixelTransfer], @[glSeparableFilter2D],
 *! @[glTexEnv], @[glTexGen], @[glTexImage1D], @[glTexImage3D],
 *! @[glTexSubImage1D], @[glTexSubImage2D], @[glTexSubImage3D],
 *! @[glTexParameter]
 *! 
 */


/*! @decl void glTexImage1D(int target, int level, int internalFormat, @
 *! object|mapping(string:object) width, int border, @
 *! object|mapping(string:object) format, @
 *! object|mapping(string:object) type, @
 *! object|mapping(string:object) data)
 *! 
 *! Texturing maps a portion of a specified texture image onto each
 *! graphical primitive for which texturing is enabled. To enable and
 *! disable one-dimensional texturing, call @[glEnable] and @[glDisable]
 *! with argument @[GL_TEXTURE_1D].
 *! 
 *! Texture images are defined with @[glTexImage1D]. The arguments
 *! describe the parameters of the texture image, such as width, width of
 *! the border, level-of-detail number (see @[glTexParameter]), and the
 *! internal resolution and format used to store the image. The last three
 *! arguments describe how the image is represented in memory; they are
 *! identical to the pixel formats used for @[glDrawPixels].
 *! 
 *! If @i{target@} is @[GL_PROXY_TEXTURE_1D], no data is read from
 *! @i{data@}, but all of the texture image state is recalculated, checked
 *! for consistency, and checked against the implementation's
 *! capabilities. If the implementation cannot handle a texture of the
 *! requested texture size, it sets all of the image state to 0, but does
 *! not generate an error (see @[glGetError]). To query for an entire
 *! mipmap array, use an image array level greater than or equal to 1.
 *! 
 *! If @i{target@} is @[GL_TEXTURE_1D], data is read from @i{data@} as a
 *! sequence of signed or unsigned bytes, shorts, or longs, or
 *! single-precision floating-point values, depending on @i{type@}. These
 *! values are grouped into sets of one, two, three, or four values,
 *! depending on @i{format@}, to form elements. If @i{type@} is
 *! @[GL_BITMAP], the data is considered as a string of unsigned bytes
 *! (and @i{format@} must be @[GL_COLOR_INDEX]). Each data byte is treated
 *! as eight 1-bit elements, with bit ordering determined by
 *! @[GL_UNPACK_LSB_FIRST] (see @[glPixelStore]).
 *! 
 *! If a non-zero named buffer object is bound to the
 *! @[GL_PIXEL_UNPACK_BUFFER] target (see @[glBindBuffer]) while a texture
 *! image is specified, @i{data@} is treated as a byte offset into the
 *! buffer object's data store.
 *! 
 *! The first element corresponds to the left end of the texture array.
 *! Subsequent elements progress left-to-right through the remaining
 *! texels in the texture array. The final element corresponds to the
 *! right end of the texture array.
 *! 
 *! @i{format@} determines the composition of each element in @i{data@}.
 *! It can assume one of these symbolic values:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_COLOR_INDEX</ref></c>
 *! <c><p>Each element is a single value, a color index. The GL converts
 *! it to fixed point (with an unspecified number of zero bits to the
 *! right of the binary point), shifted left or right depending on the
 *! value and sign of <ref>GL_INDEX_SHIFT</ref>, and added to <ref
 *! >GL_INDEX_OFFSET</ref> (see <ref>glPixelTransfer</ref>). The resulting
 *! index is converted to a set of color components using the <ref
 *! >GL_PIXEL_MAP_I_TO_R</ref>, <ref>GL_PIXEL_MAP_I_TO_G</ref>, <ref
 *! >GL_PIXEL_MAP_I_TO_B</ref>, and <ref>GL_PIXEL_MAP_I_TO_A</ref> tables,
 *! and clamped to the range [0,1]. </p></c></r>
 *! <r><c><ref>GL_RED</ref></c>
 *! <c><p>Each element is a single red component. The GL converts it to
 *! floating point and assembles it into an RGBA element by attaching 0
 *! for green and blue, and 1 for alpha. Each component is then multiplied
 *! by the signed scale factor <tt>GL_<i>c</i>_SCALE</tt>, added to the
 *! signed bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the range [0,1]
 *! (see <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_GREEN</ref></c>
 *! <c><p>Each element is a single green component. The GL converts it to
 *! floating point and assembles it into an RGBA element by attaching 0
 *! for red and blue, and 1 for alpha. Each component is then multiplied
 *! by the signed scale factor <tt>GL_<i>c</i>_SCALE</tt>, added to the
 *! signed bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the range [0,1]
 *! (see <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_BLUE</ref></c>
 *! <c><p>Each element is a single blue component. The GL converts it to
 *! floating point and assembles it into an RGBA element by attaching 0
 *! for red and green, and 1 for alpha. Each component is then multiplied
 *! by the signed scale factor <tt>GL_<i>c</i>_SCALE</tt>, added to the
 *! signed bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the range [0,1]
 *! (see <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_ALPHA</ref></c>
 *! <c><p>Each element is a single alpha component. The GL converts it to
 *! floating point and assembles it into an RGBA element by attaching 0
 *! for red, green, and blue. Each component is then multiplied by the
 *! signed scale factor <tt>GL_<i>c</i>_SCALE</tt>, added to the signed
 *! bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the range [0,1] (see
 *! <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_INTENSITY</ref></c>
 *! <c><p>Each element is a single intensity value. The GL converts it to
 *! floating point, then assembles it into an RGBA element by replicating
 *! the intensity value three times for red, green, blue, and alpha. Each
 *! component is then multiplied by the signed scale factor <tt>GL_<i>c</i
 *! >_SCALE</tt>, added to the signed bias <tt>GL_<i>c</i>_BIAS</tt>, and
 *! clamped to the range [0,1] (see <ref>glPixelTransfer</ref>). </p
 *! ></c></r>
 *! <r><c><ref>GL_RGB</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BGR</ref></c>
 *! <c><p>Each element is an RGB triple. The GL converts it to floating
 *! point and assembles it into an RGBA element by attaching 1 for alpha.
 *! Each component is then multiplied by the signed scale factor <tt>GL_<i
 *! >c</i>_SCALE</tt>, added to the signed bias <tt>GL_<i>c</i>_BIAS</tt>,
 *! and clamped to the range [0,1] (see <ref>glPixelTransfer</ref>). </p
 *! ></c></r>
 *! <r><c><ref>GL_RGBA</ref></c>
 *! <c></c></r>
 *! <r><c><ref>GL_BGRA</ref></c>
 *! <c><p>Each element contains all four components. Each component is
 *! multiplied by the signed scale factor <tt>GL_<i>c</i>_SCALE</tt>,
 *! added to the signed bias <tt>GL_<i>c</i>_BIAS</tt>, and clamped to the
 *! range [0,1] (see <ref>glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_LUMINANCE</ref></c>
 *! <c><p>Each element is a single luminance value. The GL converts it to
 *! floating point, then assembles it into an RGBA element by replicating
 *! the luminance value three times for red, green, and blue and attaching
 *! 1 for alpha. Each component is then multiplied by the signed scale
 *! factor <tt>GL_<i>c</i>_SCALE</tt>, added to the signed bias <tt>GL_<i
 *! >c</i>_BIAS</tt>, and clamped to the range [0,1] (see <ref
 *! >glPixelTransfer</ref>). </p></c></r>
 *! <r><c><ref>GL_LUMINANCE_ALPHA</ref></c>
 *! <c><p>Each element is a luminance/alpha pair. The GL converts it to
 *! floating point, then assembles it into an RGBA element by replicating
 *! the luminance value three times for red, green, and blue. Each
 *! component is then multiplied by the signed scale factor <tt>GL_<i>c</i
 *! >_SCALE</tt>, added to the signed bias <tt>GL_<i>c</i>_BIAS</tt>, and
 *! clamped to the range [0,1] (see <ref>glPixelTransfer</ref>). </p
 *! ></c></r>
 *! <r><c><ref>GL_DEPTH_COMPONENT</ref></c>
 *! <c><p>Each element is a single depth value. The GL converts it to
 *! floating point, multiplies by the signed scale factor <ref
 *! >GL_DEPTH_SCALE</ref>, adds the signed bias <ref>GL_DEPTH_BIAS</ref>,
 *! and clamps to the range [0,1] (see <ref>glPixelTransfer</ref>). </p
 *! ></c></r>
 *! </matrix>@}Refer to the @[glDrawPixels] reference page for a
 *! description of the acceptable values for the @i{type@} parameter.
 *! 
 *! If an application wants to store the texture at a certain resolution
 *! or in a certain format, it can request the resolution and format with
 *! @i{internalFormat@}. The GL will choose an internal representation
 *! that closely approximates that requested by @i{internalFormat@}, but
 *! it may not match exactly. (The representations specified by
 *! @[GL_LUMINANCE], @[GL_LUMINANCE_ALPHA], @[GL_RGB], and @[GL_RGBA] must
 *! match exactly. The numeric values 1, 2, 3, and 4 may also be used to
 *! specify the above representations.)
 *! 
 *! If the @i{internalFormat@} parameter is one of the generic compressed
 *! formats, @[GL_COMPRESSED_ALPHA], @[GL_COMPRESSED_INTENSITY],
 *! @[GL_COMPRESSED_LUMINANCE], @[GL_COMPRESSED_LUMINANCE_ALPHA],
 *! @[GL_COMPRESSED_RGB], or @[GL_COMPRESSED_RGBA], the GL will replace
 *! the internal format with the symbolic constant for a specific internal
 *! format and compress the texture before storage. If no corresponding
 *! internal format is available, or the GL can not compress that image
 *! for any reason, the internal format is instead replaced with a
 *! corresponding base internal format.
 *! 
 *! If the @i{internalFormat@} parameter is @[GL_SRGB], @[GL_SRGB8],
 *! @[GL_SRGB_ALPHA], @[GL_SRGB8_ALPHA8], @[GL_SLUMINANCE],
 *! @[GL_SLUMINANCE8], @[GL_SLUMINANCE_ALPHA], or
 *! @[GL_SLUMINANCE8_ALPHA8], the texture is treated as if the red, green,
 *! blue, or luminance components are encoded in the sRGB color space. Any
 *! alpha component is left unchanged. The conversion from the sRGB
 *! encoded component @i{c@}@sub{@i{s@}@} to a linear component
 *! @i{c@}@sub{@i{l@}@} is:
 *! 
 *! @i{c@}@sub{@i{l@}@}={@xml{<matrix><r><c><i>c</i><sub><i>s</i></sub
 *! >/12.92</c><c> if <i>c</i><sub><i>s</i></sub>&#8804;0.04045</c></r><r
 *! ><c>(<i>c</i><sub><i>s</i></sub>+0.055/1.055)<sup>2.4</sup></c><c> if
 *! <i>c</i><sub><i>s</i></sub>&gt;0.04045</c></r></matrix>@}
 *! 
 *! Assume @i{c@}@sub{@i{s@}@} is the sRGB component in the range [0,1].
 *! 
 *! Use the @[GL_PROXY_TEXTURE_1D] target to try out a resolution and
 *! format. The implementation will update and recompute its best match
 *! for the requested storage resolution and format. To then query this
 *! state, call @[glGetTexLevelParameter]. If the texture cannot be
 *! accommodated, texture state is set to 0.
 *! 
 *! A one-component texture image uses only the red component of the RGBA
 *! color from @i{data@}. A two-component image uses the R and A values. A
 *! three-component image uses the R, G, and B values. A four-component
 *! image uses all of the RGBA components.
 *! 
 *! Depth textures can be treated as LUMINANCE, INTENSITY or ALPHA
 *! textures during texture filtering and application. Image-based
 *! shadowing can be enabled by comparing texture r coordinates to depth
 *! texture values to generate a boolean result. See @[glTexParameter] for
 *! details on texture comparison.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture. Must be @[GL_TEXTURE_1D] or
 *! @[GL_PROXY_TEXTURE_1D].
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number. Level 0 is the base image level.
 *! Level @i{n@} is the @i{n@}th mipmap reduction image.
 *! 
 *! @param internalFormat
 *! 
 *! Specifies the number of color components in the texture. Must be 1, 2,
 *! 3, or 4, or one of the following symbolic constants: @[GL_ALPHA],
 *! @[GL_ALPHA4], @[GL_ALPHA8], @[GL_ALPHA12], @[GL_ALPHA16],
 *! @[GL_COMPRESSED_ALPHA], @[GL_COMPRESSED_LUMINANCE],
 *! @[GL_COMPRESSED_LUMINANCE_ALPHA], @[GL_COMPRESSED_INTENSITY],
 *! @[GL_COMPRESSED_RGB], @[GL_COMPRESSED_RGBA], @[GL_DEPTH_COMPONENT],
 *! @[GL_DEPTH_COMPONENT16], @[GL_DEPTH_COMPONENT24],
 *! @[GL_DEPTH_COMPONENT32], @[GL_LUMINANCE], @[GL_LUMINANCE4],
 *! @[GL_LUMINANCE8], @[GL_LUMINANCE12], @[GL_LUMINANCE16],
 *! @[GL_LUMINANCE_ALPHA], @[GL_LUMINANCE4_ALPHA4],
 *! @[GL_LUMINANCE6_ALPHA2], @[GL_LUMINANCE8_ALPHA8],
 *! @[GL_LUMINANCE12_ALPHA4], @[GL_LUMINANCE12_ALPHA12],
 *! @[GL_LUMINANCE16_ALPHA16], @[GL_INTENSITY], @[GL_INTENSITY4],
 *! @[GL_INTENSITY8], @[GL_INTENSITY12], @[GL_INTENSITY16],
 *! @[GL_R3_G3_B2], @[GL_RGB], @[GL_RGB4], @[GL_RGB5], @[GL_RGB8],
 *! @[GL_RGB10], @[GL_RGB12], @[GL_RGB16], @[GL_RGBA], @[GL_RGBA2],
 *! @[GL_RGBA4], @[GL_RGB5_A1], @[GL_RGBA8], @[GL_RGB10_A2], @[GL_RGBA12],
 *! @[GL_RGBA16], @[GL_SLUMINANCE], @[GL_SLUMINANCE8],
 *! @[GL_SLUMINANCE_ALPHA], @[GL_SLUMINANCE8_ALPHA8], @[GL_SRGB],
 *! @[GL_SRGB8], @[GL_SRGB_ALPHA], or @[GL_SRGB8_ALPHA8].
 *! 
 *! @param width
 *! 
 *! Specifies the width of the texture image including the border if any.
 *! If the GL version does not support non-power-of-two sizes, this value
 *! must be
 *! 2@sup{@i{n@}@}+2(border)
 *! for some integer @i{n@}. All implementations support texture images
 *! that are at least 64 texels wide. The height of the 1D texture image
 *! is 1.
 *! 
 *! @param border
 *! 
 *! Specifies the width of the border. Must be either 0 or 1.
 *! 
 *! @param format
 *! 
 *! Specifies the format of the pixel data. The following symbolic values
 *! are accepted: @[GL_COLOR_INDEX], @[GL_RED], @[GL_GREEN], @[GL_BLUE],
 *! @[GL_ALPHA], @[GL_RGB], @[GL_BGR], @[GL_RGBA], @[GL_BGRA],
 *! @[GL_LUMINANCE], and @[GL_LUMINANCE_ALPHA].
 *! 
 *! @param type
 *! 
 *! Specifies the data type of the pixel data. The following symbolic
 *! values are accepted: @[GL_UNSIGNED_BYTE], @[GL_BYTE], @[GL_BITMAP],
 *! @[GL_UNSIGNED_SHORT], @[GL_SHORT], @[GL_UNSIGNED_INT], @[GL_INT],
 *! @[GL_FLOAT], @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], and @[GL_UNSIGNED_INT_2_10_10_10_REV].
 *! 
 *! @param data
 *! 
 *! Specifies a pointer to the image data in memory.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is not @[GL_TEXTURE_1D]
 *! or @[GL_PROXY_TEXTURE_1D].
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{format@} is not an accepted
 *! format constant. Format constants other than @[GL_STENCIL_INDEX] are
 *! accepted.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not a type constant.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *! @i{format@} is not @[GL_COLOR_INDEX].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if @i{level@} is greater than
 *! log@sub{2@}(max)
 *! , where @i{max@} is the returned value of @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{internalFormat@} is not 1, 2,
 *! 3, 4, or one of the accepted resolution and format symbolic constants.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{width@} is less than 0 or
 *! greater than 2 + @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if non-power-of-two textures are not
 *! supported and the @i{width@} cannot be represented as
 *! 2@sup{@i{n@}@}+2(border)
 *! for some integer value of @i{n@}.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{border@} is not 0 or 1.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], or @[GL_UNSIGNED_SHORT_5_6_5_REV] and
 *! @i{format@} is not @[GL_RGB].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], or @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! and @i{format@} is neither @[GL_RGBA] nor @[GL_BGRA].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{format@} is
 *! @[GL_DEPTH_COMPONENT] and @i{internalFormat@} is not
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], or @[GL_DEPTH_COMPONENT32].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{internalFormat@} is
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], or @[GL_DEPTH_COMPONENT32], and @i{format@}
 *! is not @[GL_DEPTH_COMPONENT].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the buffer
 *! object's data store is currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the data would be
 *! unpacked from the buffer object such that the memory reads required
 *! would exceed the data store size.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and @i{data@} is not
 *! evenly divisible into the number of bytes needed to store in memory a
 *! datum indicated by @i{type@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glTexImage1D] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Texturing has no effect in color index mode.
 *! 
 *! If the @tt{ARB_imaging@} extension is supported, RGBA elements may
 *! also be processed by the imaging pipeline. The following stages may be
 *! applied to an RGBA color before color component clamping to the range
 *! [0, 1]:
 *! 
 *! @xml{<matrix>
 *! <r><c>1. Color component replacement by the color table specified
 *! for</c>
 *! <c><p><ref>GL_COLOR_TABLE</ref>, if enabled. See <ref
 *! >glColorTable</ref>. </p></c></r>
 *! <r><c>2. One-dimensional convolution filtering, if enabled. See</c>
 *! <c><p><ref>glConvolutionFilter1D</ref>. </p><p>If a convolution filter
 *! changes the <i>width</i> of the texture (by processing with a <ref
 *! >GL_CONVOLUTION_BORDER_MODE</ref> of <ref>GL_REDUCE</ref>, for
 *! example), the <i>width</i> must 2<sup><i>n</i></sup>+2(border), for
 *! some integer <i>n</i>, after filtering. </p></c></r>
 *! <r><c>3. RGBA components may be multiplied by <ref
 *! >GL_POST_CONVOLUTION_c_SCALE</ref>,</c>
 *! <c><p>and added to <ref>GL_POST_CONVOLUTION_c_BIAS</ref>, if enabled.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c>4. Color component replacement by the color table specified
 *! for</c>
 *! <c><p><ref>GL_POST_CONVOLUTION_COLOR_TABLE</ref>, if enabled. See <ref
 *! >glColorTable</ref>. </p></c></r>
 *! <r><c>5. Transformation by the color matrix.</c>
 *! <c><p>See <ref>glMatrixMode</ref>. </p></c></r>
 *! <r><c>6. RGBA components may be multiplied by <ref
 *! >GL_POST_COLOR_MATRIX_c_SCALE</ref>,</c>
 *! <c><p>and added to <ref>GL_POST_COLOR_MATRIX_c_BIAS</ref>, if enabled.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c>7. Color component replacement by the color table specified
 *! for</c>
 *! <c><p><ref>GL_POST_COLOR_MATRIX_COLOR_TABLE</ref>, if enabled. See
 *! <ref>glColorTable</ref>. </p></c></r>
 *! </matrix>@}The texture image can be represented by the same data
 *! formats as the pixels in a @[glDrawPixels] command, except that
 *! @[GL_STENCIL_INDEX] cannot be used. @[glPixelStore] and
 *! @[glPixelTransfer] modes affect texture images in exactly the way they
 *! affect @[glDrawPixels].
 *! 
 *! @[GL_PROXY_TEXTURE_1D] may be used only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Internal formats other than 1, 2, 3, or 4 may be used only if the GL
 *! version is 1.1 or greater.
 *! 
 *! In GL version 1.1 or greater, @i{data@} may be a null pointer. In this
 *! case texture memory is allocated to accommodate a texture of width
 *! @i{width@}. You can then download subtextures to initialize the
 *! texture memory. The image is undefined if the program tries to apply
 *! an uninitialized portion of the texture image to a primitive.
 *! 
 *! Formats @[GL_BGR], and @[GL_BGRA] and types @[GL_UNSIGNED_BYTE_3_3_2],
 *! @[GL_UNSIGNED_BYTE_2_3_3_REV], @[GL_UNSIGNED_SHORT_5_6_5],
 *! @[GL_UNSIGNED_SHORT_5_6_5_REV], @[GL_UNSIGNED_SHORT_4_4_4_4],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4_REV], @[GL_UNSIGNED_SHORT_5_5_5_1],
 *! @[GL_UNSIGNED_SHORT_1_5_5_5_REV], @[GL_UNSIGNED_INT_8_8_8_8],
 *! @[GL_UNSIGNED_INT_8_8_8_8_REV], @[GL_UNSIGNED_INT_10_10_10_2], and
 *! @[GL_UNSIGNED_INT_2_10_10_10_REV] are available only if the GL version
 *! is 1.2 or greater.
 *! 
 *! When the @tt{ARB_multitexture@} extension is supported, or the GL
 *! version is 1.3 or greater, @[glTexImage1D] specifies the
 *! one-dimensional texture for the current texture unit, specified with
 *! @[glActiveTexture].
 *! 
 *! @[GL_DEPTH_COMPONENT], @[GL_DEPTH_COMPONENT16],
 *! @[GL_DEPTH_COMPONENT24], and @[GL_DEPTH_COMPONENT32] are available
 *! only if the GL version is 1.4 or greater.
 *! 
 *! Non-power-of-two textures are supported if the GL version is 2.0 or
 *! greater, or if the implementation exports the
 *! @[GL_ARB_texture_non_power_of_two] extension.
 *! 
 *! The @[GL_SRGB], @[GL_SRGB8], @[GL_SRGB_ALPHA], @[GL_SRGB8_ALPHA8],
 *! @[GL_SLUMINANCE], @[GL_SLUMINANCE8], @[GL_SLUMINANCE_ALPHA], and
 *! @[GL_SLUMINANCE8_ALPHA8] internal formats are only available if the GL
 *! version is 2.1 or greater.
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glColorTable], @[glCompressedTexImage1D],
 *! @[glCompressedTexSubImage1D], @[glConvolutionFilter1D],
 *! @[glCopyPixels], @[glCopyTexImage1D], @[glCopyTexSubImage1D],
 *! @[glDrawPixels], @[glGetCompressedTexImage], @[glMatrixMode],
 *! @[glPixelStore], @[glPixelTransfer], @[glTexEnv], @[glTexGen],
 *! @[glTexImage2D], @[glTexImage3D], @[glTexSubImage1D],
 *! @[glTexSubImage2D], @[glTexSubImage3D], @[glTexParameter]
 *! 
 */


/*! @decl void glTexParameter(int target, int pname, @
 *! float|int|array(float|int) param)
 *! 
 *! Texture mapping is a technique that applies an image onto an object's
 *! surface as if the image were a decal or cellophane shrink-wrap. The
 *! image is created in texture space, with an (@i{s@}, @i{t@}) coordinate
 *! system. A texture is a one- or two-dimensional image and a set of
 *! parameters that determine how samples are derived from the image.
 *! 
 *! @[glTexParameter] assigns the value or values in @i{params@} to the
 *! texture parameter specified as @i{pname@}. @i{target@} defines the
 *! target texture, either @[GL_TEXTURE_1D], @[GL_TEXTURE_2D], or
 *! @[GL_TEXTURE_3D]. The following symbols are accepted in @i{pname@}:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_TEXTURE_MIN_FILTER</ref></c>
 *! <c><p>The texture minifying function is used whenever the pixel being
 *! textured maps to an area greater than one texture element. There are
 *! six defined minifying functions. Two of them use the nearest one or
 *! nearest four texture elements to compute the texture value. The other
 *! four use mipmaps. </p><p>A mipmap is an ordered set of arrays
 *! representing the same image at progressively lower resolutions. If the
 *! texture has dimensions 2<sup><i>n</i></sup>&#215;2<sup><i>m</i></sup>,
 *! there are max(<i>n</i>, <i>m</i>)+1 mipmaps. The first mipmap is the
 *! original texture, with dimensions 2<sup><i>n</i></sup>&#215;2<sup><i
 *! >m</i></sup>. Each subsequent mipmap has dimensions 2<sup><i>k</i
 *! >-1</sup>&#215;2<sup><i>l</i>-1</sup>, where 2<sup><i>k</i></sup
 *! >&#215;2<sup><i>l</i></sup> are the dimensions of the previous mipmap,
 *! until either <i>k</i>=0 or <i>l</i>=0. At that point, subsequent
 *! mipmaps have dimension 1&#215;2<sup><i>l</i>-1</sup> or 2<sup><i>k</i
 *! >-1</sup>&#215;1 until the final mipmap, which has dimension 1&#215;1.
 *! To define the mipmaps, call <ref>glTexImage1D</ref>, <ref
 *! >glTexImage2D</ref>, <ref>glTexImage3D</ref>, <ref
 *! >glCopyTexImage1D</ref>, or <ref>glCopyTexImage2D</ref> with the <i
 *! >level</i> argument indicating the order of the mipmaps. Level 0 is
 *! the original texture; level max(<i>n</i>, <i>m</i>) is the final
 *! 1&#215;1 mipmap. </p><p><i>params</i> supplies a function for
 *! minifying the texture as one of the following: <matrix>
 *! <r><c><ref>GL_NEAREST</ref></c>
 *! <c><p>Returns the value of the texture element that is nearest (in
 *! Manhattan distance) to the center of the pixel being textured. </p
 *! ></c></r>
 *! <r><c><ref>GL_LINEAR</ref></c>
 *! <c><p>Returns the weighted average of the four texture elements that
 *! are closest to the center of the pixel being textured. These can
 *! include border texture elements, depending on the values of <ref
 *! >GL_TEXTURE_WRAP_S</ref> and <ref>GL_TEXTURE_WRAP_T</ref>, and on the
 *! exact mapping. </p></c></r>
 *! <r><c><ref>GL_NEAREST_MIPMAP_NEAREST</ref></c>
 *! <c><p>Chooses the mipmap that most closely matches the size of the
 *! pixel being textured and uses the <ref>GL_NEAREST</ref> criterion (the
 *! texture element nearest to the center of the pixel) to produce a
 *! texture value. </p></c></r>
 *! <r><c><ref>GL_LINEAR_MIPMAP_NEAREST</ref></c>
 *! <c><p>Chooses the mipmap that most closely matches the size of the
 *! pixel being textured and uses the <ref>GL_LINEAR</ref> criterion (a
 *! weighted average of the four texture elements that are closest to the
 *! center of the pixel) to produce a texture value. </p></c></r>
 *! <r><c><ref>GL_NEAREST_MIPMAP_LINEAR</ref></c>
 *! <c><p>Chooses the two mipmaps that most closely match the size of the
 *! pixel being textured and uses the <ref>GL_NEAREST</ref> criterion (the
 *! texture element nearest to the center of the pixel) to produce a
 *! texture value from each mipmap. The final texture value is a weighted
 *! average of those two values. </p></c></r>
 *! <r><c><ref>GL_LINEAR_MIPMAP_LINEAR</ref></c>
 *! <c><p>Chooses the two mipmaps that most closely match the size of the
 *! pixel being textured and uses the <ref>GL_LINEAR</ref> criterion (a
 *! weighted average of the four texture elements that are closest to the
 *! center of the pixel) to produce a texture value from each mipmap. The
 *! final texture value is a weighted average of those two values. </p
 *! ></c></r>
 *! </matrix></p><p>As more texture elements are sampled in the
 *! minification process, fewer aliasing artifacts will be apparent. While
 *! the <ref>GL_NEAREST</ref> and <ref>GL_LINEAR</ref> minification
 *! functions can be faster than the other four, they sample only one or
 *! four texture elements to determine the texture value of the pixel
 *! being rendered and can produce moire patterns or ragged transitions.
 *! The initial value of <ref>GL_TEXTURE_MIN_FILTER</ref> is <ref
 *! >GL_NEAREST_MIPMAP_LINEAR</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_MAG_FILTER</ref></c>
 *! <c><p>The texture magnification function is used when the pixel being
 *! textured maps to an area less than or equal to one texture element. It
 *! sets the texture magnification function to either <ref>GL_NEAREST</ref
 *! > or <ref>GL_LINEAR</ref> (see below). <ref>GL_NEAREST</ref> is
 *! generally faster than <ref>GL_LINEAR</ref>, but it can produce
 *! textured images with sharper edges because the transition between
 *! texture elements is not as smooth. The initial value of <ref
 *! >GL_TEXTURE_MAG_FILTER</ref> is <ref>GL_LINEAR</ref>. <matrix>
 *! <r><c><ref>GL_NEAREST</ref></c>
 *! <c><p>Returns the value of the texture element that is nearest (in
 *! Manhattan distance) to the center of the pixel being textured. </p
 *! ></c></r>
 *! <r><c><ref>GL_LINEAR</ref></c>
 *! <c><p>Returns the weighted average of the four texture elements that
 *! are closest to the center of the pixel being textured. These can
 *! include border texture elements, depending on the values of <ref
 *! >GL_TEXTURE_WRAP_S</ref> and <ref>GL_TEXTURE_WRAP_T</ref>, and on the
 *! exact mapping. </p></c></r>
 *! </matrix><p></p></p></c></r>
 *! </matrix>@}
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_TEXTURE_MIN_LOD</ref></c>
 *! <c><p>Sets the minimum level-of-detail parameter. This floating-point
 *! value limits the selection of highest resolution mipmap (lowest mipmap
 *! level). The initial value is -1000. </p></c></r>
 *! </matrix>@}
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_TEXTURE_MAX_LOD</ref></c>
 *! <c><p>Sets the maximum level-of-detail parameter. This floating-point
 *! value limits the selection of the lowest resolution mipmap (highest
 *! mipmap level). The initial value is 1000. </p></c></r>
 *! </matrix>@}
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_TEXTURE_BASE_LEVEL</ref></c>
 *! <c><p>Specifies the index of the lowest defined mipmap level. This is
 *! an integer value. The initial value is 0. </p></c></r>
 *! </matrix>@}
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_TEXTURE_MAX_LEVEL</ref></c>
 *! <c><p>Sets the index of the highest defined mipmap level. This is an
 *! integer value. The initial value is 1000. </p></c></r>
 *! </matrix>@}
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_TEXTURE_WRAP_S</ref></c>
 *! <c><p>Sets the wrap parameter for texture coordinate <i>s</i> to
 *! either <ref>GL_CLAMP</ref>, <ref>GL_CLAMP_TO_BORDER</ref>, <ref
 *! >GL_CLAMP_TO_EDGE</ref>, <ref>GL_MIRRORED_REPEAT</ref>, or <ref
 *! >GL_REPEAT</ref>. <ref>GL_CLAMP</ref> causes <i>s</i> coordinates to
 *! be clamped to the range [0,1] and is useful for preventing wrapping
 *! artifacts when mapping a single image onto an object. <ref
 *! >GL_CLAMP_TO_BORDER</ref> causes the <i>s</i> coordinate to be clamped
 *! to the range [-1/2<i>N</i>, 1+1/2<i>N</i>], where <i>N</i> is the size
 *! of the texture in the direction of clamping.<ref>GL_CLAMP_TO_EDGE</ref
 *! > causes <i>s</i> coordinates to be clamped to the range [1/2<i>N</i>,
 *! 1-1/2<i>N</i>], where <i>N</i> is the size of the texture in the
 *! direction of clamping. <ref>GL_REPEAT</ref> causes the integer part of
 *! the <i>s</i> coordinate to be ignored; the GL uses only the fractional
 *! part, thereby creating a repeating pattern. <ref
 *! >GL_MIRRORED_REPEAT</ref> causes the <i>s</i> coordinate to be set to
 *! the fractional part of the texture coordinate if the integer part of
 *! <i>s</i> is even; if the integer part of <i>s</i> is odd, then the <i
 *! >s</i> texture coordinate is set to 1-frac(<i>s</i>), where frac(<i
 *! >s</i>) represents the fractional part of <i>s</i>. Border texture
 *! elements are accessed only if wrapping is set to <ref>GL_CLAMP</ref>
 *! or <ref>GL_CLAMP_TO_BORDER</ref>. Initially, <ref
 *! >GL_TEXTURE_WRAP_S</ref> is set to <ref>GL_REPEAT</ref>. </p></c></r>
 *! </matrix>@}
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_TEXTURE_WRAP_T</ref></c>
 *! <c><p>Sets the wrap parameter for texture coordinate <i>t</i> to
 *! either <ref>GL_CLAMP</ref>, <ref>GL_CLAMP_TO_BORDER</ref>, <ref
 *! >GL_CLAMP_TO_EDGE</ref>, <ref>GL_MIRRORED_REPEAT</ref>, or <ref
 *! >GL_REPEAT</ref>. See the discussion under <ref>GL_TEXTURE_WRAP_S</ref
 *! >. Initially, <ref>GL_TEXTURE_WRAP_T</ref> is set to <ref
 *! >GL_REPEAT</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_WRAP_R</ref></c>
 *! <c><p>Sets the wrap parameter for texture coordinate <i>r</i> to
 *! either <ref>GL_CLAMP</ref>, <ref>GL_CLAMP_TO_BORDER</ref>, <ref
 *! >GL_CLAMP_TO_EDGE</ref>, <ref>GL_MIRRORED_REPEAT</ref>, or <ref
 *! >GL_REPEAT</ref>. See the discussion under <ref>GL_TEXTURE_WRAP_S</ref
 *! >. Initially, <ref>GL_TEXTURE_WRAP_R</ref> is set to <ref
 *! >GL_REPEAT</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_BORDER_COLOR</ref></c>
 *! <c><p>Sets a border color. <i>params</i> contains four values that
 *! comprise the RGBA color of the texture border. Integer color
 *! components are interpreted linearly such that the most positive
 *! integer maps to 1.0, and the most negative integer maps to -1.0. The
 *! values are clamped to the range [0,1] when they are specified.
 *! Initially, the border color is (0, 0, 0, 0). </p></c></r>
 *! <r><c><ref>GL_TEXTURE_PRIORITY</ref></c>
 *! <c><p>Specifies the texture residence priority of the currently bound
 *! texture. Permissible values are in the range [0, 1]. See <ref
 *! >glPrioritizeTextures</ref> and <ref>glBindTexture</ref> for more
 *! information. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COMPARE_MODE</ref></c>
 *! <c><p>Specifies the texture comparison mode for currently bound depth
 *! textures. That is, a texture whose internal format is <ref
 *! >GL_DEPTH_COMPONENT_*</ref>; see <ref>glTexImage2D</ref>) Permissible
 *! values are: <matrix>
 *! <r><c><ref>GL_COMPARE_R_TO_TEXTURE</ref></c>
 *! <c><p>Specifies that the interpolated and clamped <i>r</i> texture
 *! coordinate should be compared to the value in the currently bound
 *! depth texture. See the discussion of <ref>GL_TEXTURE_COMPARE_FUNC</ref
 *! > for details of how the comparison is evaluated. The result of the
 *! comparison is assigned to luminance, intensity, or alpha (as specified
 *! by <ref>GL_DEPTH_TEXTURE_MODE</ref>). </p></c></r>
 *! <r><c><ref>GL_NONE</ref></c>
 *! <c><p>Specifies that the luminance, intensity, or alpha (as specified
 *! by <ref>GL_DEPTH_TEXTURE_MODE</ref>) should be assigned the
 *! appropriate value from the currently bound depth texture. </p></c></r>
 *! </matrix></p></c></r>
 *! <r><c><ref>GL_TEXTURE_COMPARE_FUNC</ref></c>
 *! <c><p>Specifies the comparison operator used when <ref
 *! >GL_TEXTURE_COMPARE_MODE</ref> is set to <ref
 *! >GL_COMPARE_R_TO_TEXTURE</ref>. Permissible values are: <matrix><r><c
 *! ><b>Texture Comparison Function </b></c><c><b>Computed result </b></c
 *! ></r><r><c><ref>GL_LEQUAL</ref></c><c> result={<matrix><r><c>1.0</c
 *! ></r><r><c>0.0</c></r></matrix> <matrix><r><c><i>r</i>&lt;=<i>D</i
 *! ><sub><i>t</i></sub></c></r><r><c><i>r</i>&gt;<i>D</i><sub><i>t</i
 *! ></sub></c></r></matrix></c></r><r><c><ref>GL_GEQUAL</ref></c><c>
 *! result={<matrix><r><c>1.0</c></r><r><c>0.0</c></r></matrix> <matrix><r
 *! ><c><i>r</i>&gt;=<i>D</i><sub><i>t</i></sub></c></r><r><c><i>r</i
 *! >&lt;<i>D</i><sub><i>t</i></sub></c></r></matrix></c></r><r><c><ref
 *! >GL_LESS</ref></c><c> result={<matrix><r><c>1.0</c></r><r><c>0.0</c
 *! ></r></matrix> <matrix><r><c><i>r</i>&lt;<i>D</i><sub><i>t</i></sub
 *! ></c></r><r><c><i>r</i>&gt;=<i>D</i><sub><i>t</i></sub></c></r
 *! ></matrix></c></r><r><c><ref>GL_GREATER</ref></c><c> result={<matrix
 *! ><r><c>1.0</c></r><r><c>0.0</c></r></matrix> <matrix><r><c><i>r</i
 *! >&gt;<i>D</i><sub><i>t</i></sub></c></r><r><c><i>r</i>&lt;=<i>D</i
 *! ><sub><i>t</i></sub></c></r></matrix></c></r><r><c><ref>GL_EQUAL</ref
 *! ></c><c> result={<matrix><r><c>1.0</c></r><r><c>0.0</c></r></matrix>
 *! <matrix><r><c><i>r</i>=<i>D</i><sub><i>t</i></sub></c></r><r><c><i
 *! >r</i>&#8800;<i>D</i><sub><i>t</i></sub></c></r></matrix></c></r><r><c
 *! ><ref>GL_NOTEQUAL</ref></c><c> result={<matrix><r><c>1.0</c></r><r><c
 *! >0.0</c></r></matrix> <matrix><r><c><i>r</i>&#8800;<i>D</i><sub><i
 *! >t</i></sub></c></r><r><c><i>r</i>=<i>D</i><sub><i>t</i></sub></c></r
 *! ></matrix></c></r><r><c><ref>GL_ALWAYS</ref></c><c> result=1.0</c></r
 *! ><r><c><ref>GL_NEVER</ref></c><c> result=0.0</c></r></matrix> where <i
 *! >r</i> is the current interpolated texture coordinate, and <i>D</i
 *! ><sub><i>t</i></sub> is the depth texture value sampled from the
 *! currently bound depth texture. result is assigned to the either the
 *! luminance, intensity, or alpha (as specified by <ref
 *! >GL_DEPTH_TEXTURE_MODE</ref>.) </p></c></r>
 *! <r><c><ref>GL_DEPTH_TEXTURE_MODE</ref></c>
 *! <c><p>Specifies a single symbolic constant indicating how depth values
 *! should be treated during filtering and texture application. Accepted
 *! values are <ref>GL_LUMINANCE</ref>, <ref>GL_INTENSITY</ref>, and <ref
 *! >GL_ALPHA</ref>. The initial value is <ref>GL_LUMINANCE</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_GENERATE_MIPMAP</ref></c>
 *! <c><p>Specifies a boolean value that indicates if all levels of a
 *! mipmap array should be automatically updated when any modification to
 *! the base level mipmap is done. The initial value is <ref>GL_FALSE</ref
 *! >. </p></c></r>
 *! </matrix>@}
 *! 
 *! @param target
 *! 
 *! Specifies the target texture, which must be either @[GL_TEXTURE_1D],
 *! @[GL_TEXTURE_2D], @[GL_TEXTURE_3D], or @[GL_TEXTURE_CUBE_MAP].
 *! 
 *! @param pname
 *! 
 *! Specifies the symbolic name of a single-valued texture parameter.
 *! @i{pname@} can be one of the following: @[GL_TEXTURE_MIN_FILTER],
 *! @[GL_TEXTURE_MAG_FILTER], @[GL_TEXTURE_MIN_LOD],
 *! @[GL_TEXTURE_MAX_LOD], @[GL_TEXTURE_BASE_LEVEL],
 *! @[GL_TEXTURE_MAX_LEVEL], @[GL_TEXTURE_WRAP_S], @[GL_TEXTURE_WRAP_T],
 *! @[GL_TEXTURE_WRAP_R], @[GL_TEXTURE_PRIORITY],
 *! @[GL_TEXTURE_COMPARE_MODE], @[GL_TEXTURE_COMPARE_FUNC],
 *! @[GL_DEPTH_TEXTURE_MODE], or @[GL_GENERATE_MIPMAP].
 *! 
 *! @param param
 *! 
 *! Specifies the value of @i{pname@}.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture, which must be either @[GL_TEXTURE_1D],
 *! @[GL_TEXTURE_2D] or @[GL_TEXTURE_3D].
 *! 
 *! @param pname
 *! 
 *! Specifies the symbolic name of a texture parameter. @i{pname@} can be
 *! one of the following: @[GL_TEXTURE_MIN_FILTER],
 *! @[GL_TEXTURE_MAG_FILTER], @[GL_TEXTURE_MIN_LOD],
 *! @[GL_TEXTURE_MAX_LOD], @[GL_TEXTURE_BASE_LEVEL],
 *! @[GL_TEXTURE_MAX_LEVEL], @[GL_TEXTURE_WRAP_S], @[GL_TEXTURE_WRAP_T],
 *! @[GL_TEXTURE_WRAP_R], @[GL_TEXTURE_BORDER_COLOR],
 *! @[GL_TEXTURE_PRIORITY], @[GL_TEXTURE_COMPARE_MODE],
 *! @[GL_TEXTURE_COMPARE_FUNC], @[GL_DEPTH_TEXTURE_MODE], or
 *! @[GL_GENERATE_MIPMAP].
 *! 
 *! @param params
 *! 
 *! Specifies an array where the value or values of @i{pname@} are stored.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} or @i{pname@} is not
 *! one of the accepted defined values.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{params@} should have a defined
 *! constant value (based on the value of @i{pname@}) and does not.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glTexParameter] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[GL_TEXTURE_3D], @[GL_TEXTURE_MIN_LOD], @[GL_TEXTURE_MAX_LOD],
 *! @[GL_CLAMP_TO_EDGE], @[GL_TEXTURE_BASE_LEVEL], and
 *! @[GL_TEXTURE_MAX_LEVEL] are available only if the GL version is 1.2 or
 *! greater.
 *! 
 *! @[GL_CLAMP_TO_BORDER] is available only if the GL version is 1.3 or
 *! greater.
 *! 
 *! @[GL_MIRRORED_REPEAT], @[GL_TEXTURE_COMPARE_MODE],
 *! @[GL_TEXTURE_COMPARE_FUNC], @[GL_DEPTH_TEXTURE_MODE], and
 *! @[GL_GENERATE_MIPMAP] are available only if the GL version is 1.4 or
 *! greater.
 *! 
 *! @[GL_TEXTURE_COMPARE_FUNC] allows the following additional comparison
 *! modes only if the GL version is 1.5 or greater: @[GL_LESS],
 *! @[GL_GREATER], @[GL_EQUAL], @[GL_NOTEQUAL], @[GL_ALWAYS], and
 *! @[GL_NEVER].
 *! 
 *! Suppose that a program has enabled texturing (by calling @[glEnable]
 *! with argument @[GL_TEXTURE_1D], @[GL_TEXTURE_2D], or @[GL_TEXTURE_3D])
 *! and has set @[GL_TEXTURE_MIN_FILTER] to one of the functions that
 *! requires a mipmap. If either the dimensions of the texture images
 *! currently defined (with previous calls to @[glTexImage1D],
 *! @[glTexImage2D], @[glTexImage3D], @[glCopyTexImage1D], or
 *! @[glCopyTexImage2D]) do not follow the proper sequence for mipmaps
 *! (described above), or there are fewer texture images defined than are
 *! needed, or the set of texture images have differing numbers of texture
 *! components, then it is as if texture mapping were disabled.
 *! 
 *! Linear filtering accesses the four nearest texture elements only in 2D
 *! textures. In 1D textures, linear filtering accesses the two nearest
 *! texture elements.
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, @[glTexParameter]
 *! specifies the texture parameters for the active texture unit,
 *! specified by calling @[glActiveTexture].
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glBindTexture], @[glCopyPixels],
 *! @[glCopyTexImage1D], @[glCopyTexImage2D], @[glCopyTexSubImage1D],
 *! @[glCopyTexSubImage2D], @[glCopyTexSubImage3D], @[glDrawPixels],
 *! @[glPixelStore], @[glPixelTransfer], @[glPrioritizeTextures],
 *! @[glTexEnv], @[glTexGen], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexImage3D], @[glTexSubImage1D], @[glTexSubImage2D],
 *! @[glTexSubImage3D]
 *! 
 */


/*! @decl void glTexSubImage2D(int target, int level, int xoffset, @
 *! int yoffset, object|mapping(string:object) width, @
 *! object|mapping(string:object) height, @
 *! object|mapping(string:object) format, @
 *! object|mapping(string:object) type, @
 *! object|mapping(string:object) data)
 *! 
 *! Texturing maps a portion of a specified texture image onto each
 *! graphical primitive for which texturing is enabled. To enable and
 *! disable two-dimensional texturing, call @[glEnable] and @[glDisable]
 *! with argument @[GL_TEXTURE_2D].
 *! 
 *! @[glTexSubImage2D] redefines a contiguous subregion of an existing
 *! two-dimensional texture image. The texels referenced by @i{data@}
 *! replace the portion of the existing texture array with x indices
 *! @i{xoffset@} and
 *! xoffset+width-1
 *! , inclusive, and y indices @i{yoffset@} and
 *! yoffset+height-1
 *! , inclusive. This region may not include any texels outside the range
 *! of the texture array as it was originally specified. It is not an
 *! error to specify a subtexture with zero width or height, but such a
 *! specification has no effect.
 *! 
 *! If a non-zero named buffer object is bound to the
 *! @[GL_PIXEL_UNPACK_BUFFER] target (see @[glBindBuffer]) while a texture
 *! image is specified, @i{data@} is treated as a byte offset into the
 *! buffer object's data store.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture. Must be @[GL_TEXTURE_2D],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_X], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y], @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z], or
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z].
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number. Level 0 is the base image level.
 *! Level @i{n@} is the @i{n@}th mipmap reduction image.
 *! 
 *! @param xoffset
 *! 
 *! Specifies a texel offset in the x direction within the texture array.
 *! 
 *! @param yoffset
 *! 
 *! Specifies a texel offset in the y direction within the texture array.
 *! 
 *! @param width
 *! 
 *! Specifies the width of the texture subimage.
 *! 
 *! @param height
 *! 
 *! Specifies the height of the texture subimage.
 *! 
 *! @param format
 *! 
 *! Specifies the format of the pixel data. The following symbolic values
 *! are accepted: @[GL_COLOR_INDEX], @[GL_RED], @[GL_GREEN], @[GL_BLUE],
 *! @[GL_ALPHA], @[GL_RGB], @[GL_BGR], @[GL_RGBA], @[GL_BGRA],
 *! @[GL_LUMINANCE], and @[GL_LUMINANCE_ALPHA].
 *! 
 *! @param type
 *! 
 *! Specifies the data type of the pixel data. The following symbolic
 *! values are accepted: @[GL_UNSIGNED_BYTE], @[GL_BYTE], @[GL_BITMAP],
 *! @[GL_UNSIGNED_SHORT], @[GL_SHORT], @[GL_UNSIGNED_INT], @[GL_INT],
 *! @[GL_FLOAT], @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], and @[GL_UNSIGNED_INT_2_10_10_10_REV].
 *! 
 *! @param data
 *! 
 *! Specifies a pointer to the image data in memory.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is not
 *! @[GL_TEXTURE_2D], @[GL_TEXTURE_CUBE_MAP_POSITIVE_X],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_X], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Y],
 *! @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y], @[GL_TEXTURE_CUBE_MAP_POSITIVE_Z],
 *! or @[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z].
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{format@} is not an accepted
 *! format constant.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not a type constant.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *! @i{format@} is not @[GL_COLOR_INDEX].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if @i{level@} is greater than
 *! log@sub{2@}@i{max@}, where @i{max@} is the returned value of
 *! @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if
 *! xoffset<-@i{b@}
 *! ,
 *! (xoffset+width)>(@i{w@}-@i{b@})
 *! ,
 *! yoffset<-@i{b@}
 *! , or
 *! (yoffset+height)>(@i{h@}-@i{b@})
 *! , where @i{w@} is the @[GL_TEXTURE_WIDTH], @i{h@} is the
 *! @[GL_TEXTURE_HEIGHT], and @i{b@} is the border width of the texture
 *! image being modified. Note that @i{w@} and @i{h@} include twice the
 *! border width.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{width@} or @i{height@} is less
 *! than 0.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if the texture array has not been
 *! defined by a previous @[glTexImage2D] operation.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], or @[GL_UNSIGNED_SHORT_5_6_5_REV] and
 *! @i{format@} is not @[GL_RGB].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], or @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! and @i{format@} is neither @[GL_RGBA] nor @[GL_BGRA].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the buffer
 *! object's data store is currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the data would be
 *! unpacked from the buffer object such that the memory reads required
 *! would exceed the data store size.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and @i{data@} is not
 *! evenly divisible into the number of bytes needed to store in memory a
 *! datum indicated by @i{type@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glTexSubImage2D] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glTexSubImage2D] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Texturing has no effect in color index mode.
 *! 
 *! @[glPixelStore] and @[glPixelTransfer] modes affect texture images in
 *! exactly the way they affect @[glDrawPixels].
 *! 
 *! Formats @[GL_BGR], and @[GL_BGRA] and types @[GL_UNSIGNED_BYTE_3_3_2],
 *! @[GL_UNSIGNED_BYTE_2_3_3_REV], @[GL_UNSIGNED_SHORT_5_6_5],
 *! @[GL_UNSIGNED_SHORT_5_6_5_REV], @[GL_UNSIGNED_SHORT_4_4_4_4],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4_REV], @[GL_UNSIGNED_SHORT_5_5_5_1],
 *! @[GL_UNSIGNED_SHORT_1_5_5_5_REV], @[GL_UNSIGNED_INT_8_8_8_8],
 *! @[GL_UNSIGNED_INT_8_8_8_8_REV], @[GL_UNSIGNED_INT_10_10_10_2], and
 *! @[GL_UNSIGNED_INT_2_10_10_10_REV] are available only if the GL version
 *! is 1.2 or greater.
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, @[glTexSubImage2D]
 *! specifies a two-dimensional subtexture for the current texture unit,
 *! specified with @[glActiveTexture].
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, the RGBA components
 *! specified in @i{data@} may be processed by the imaging pipeline. See
 *! @[glTexImage1D] for specific details.
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glCopyTexSubImage3D], @[glDrawPixels], @[glPixelStore],
 *! @[glPixelTransfer], @[glTexEnv], @[glTexGen], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexImage3D], @[glTexSubImage1D],
 *! @[glTexSubImage3D], @[glTexParameter]
 *! 
 */


/*! @decl void glTexSubImage1D(int target, int level, int xoffset, @
 *! object|mapping(string:object) width, @
 *! object|mapping(string:object) format, @
 *! object|mapping(string:object) type, @
 *! object|mapping(string:object) data)
 *! 
 *! Texturing maps a portion of a specified texture image onto each
 *! graphical primitive for which texturing is enabled. To enable or
 *! disable one-dimensional texturing, call @[glEnable] and @[glDisable]
 *! with argument @[GL_TEXTURE_1D].
 *! 
 *! @[glTexSubImage1D] redefines a contiguous subregion of an existing
 *! one-dimensional texture image. The texels referenced by @i{data@}
 *! replace the portion of the existing texture array with x indices
 *! @i{xoffset@} and
 *! xoffset+width-1
 *! , inclusive. This region may not include any texels outside the range
 *! of the texture array as it was originally specified. It is not an
 *! error to specify a subtexture with width of 0, but such a
 *! specification has no effect.
 *! 
 *! If a non-zero named buffer object is bound to the
 *! @[GL_PIXEL_UNPACK_BUFFER] target (see @[glBindBuffer]) while a texture
 *! image is specified, @i{data@} is treated as a byte offset into the
 *! buffer object's data store.
 *! 
 *! @param target
 *! 
 *! Specifies the target texture. Must be @[GL_TEXTURE_1D].
 *! 
 *! @param level
 *! 
 *! Specifies the level-of-detail number. Level 0 is the base image level.
 *! Level @i{n@} is the @i{n@}th mipmap reduction image.
 *! 
 *! @param xoffset
 *! 
 *! Specifies a texel offset in the x direction within the texture array.
 *! 
 *! @param width
 *! 
 *! Specifies the width of the texture subimage.
 *! 
 *! @param format
 *! 
 *! Specifies the format of the pixel data. The following symbolic values
 *! are accepted: @[GL_COLOR_INDEX], @[GL_RED], @[GL_GREEN], @[GL_BLUE],
 *! @[GL_ALPHA], @[GL_RGB], @[GL_BGR], @[GL_RGBA], @[GL_BGRA],
 *! @[GL_LUMINANCE], and @[GL_LUMINANCE_ALPHA].
 *! 
 *! @param type
 *! 
 *! Specifies the data type of the pixel data. The following symbolic
 *! values are accepted: @[GL_UNSIGNED_BYTE], @[GL_BYTE], @[GL_BITMAP],
 *! @[GL_UNSIGNED_SHORT], @[GL_SHORT], @[GL_UNSIGNED_INT], @[GL_INT],
 *! @[GL_FLOAT], @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], @[GL_UNSIGNED_SHORT_5_6_5_REV],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], and @[GL_UNSIGNED_INT_2_10_10_10_REV].
 *! 
 *! @param data
 *! 
 *! Specifies a pointer to the image data in memory.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{target@} is not one of the
 *! allowable values.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{format@} is not an accepted
 *! format constant.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is not a type constant.
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{type@} is @[GL_BITMAP] and
 *! @i{format@} is not @[GL_COLOR_INDEX].
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{level@} is less than 0.
 *! 
 *! @[GL_INVALID_VALUE] may be generated if @i{level@} is greater than
 *! log@sub{2@}@i{max@}, where @i{max@} is the returned value of
 *! @[GL_MAX_TEXTURE_SIZE].
 *! 
 *! @[GL_INVALID_VALUE] is generated if
 *! xoffset<-@i{b@}
 *! , or if
 *! (xoffset+width)>(@i{w@}-@i{b@})
 *! , where @i{w@} is the @[GL_TEXTURE_WIDTH], and @i{b@} is the width of
 *! the @[GL_TEXTURE_BORDER] of the texture image being modified. Note
 *! that @i{w@} includes twice the border width.
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{width@} is less than 0.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if the texture array has not been
 *! defined by a previous @[glTexImage1D] operation.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_BYTE_3_3_2], @[GL_UNSIGNED_BYTE_2_3_3_REV],
 *! @[GL_UNSIGNED_SHORT_5_6_5], or @[GL_UNSIGNED_SHORT_5_6_5_REV] and
 *! @i{format@} is not @[GL_RGB].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{type@} is one of
 *! @[GL_UNSIGNED_SHORT_4_4_4_4], @[GL_UNSIGNED_SHORT_4_4_4_4_REV],
 *! @[GL_UNSIGNED_SHORT_5_5_5_1], @[GL_UNSIGNED_SHORT_1_5_5_5_REV],
 *! @[GL_UNSIGNED_INT_8_8_8_8], @[GL_UNSIGNED_INT_8_8_8_8_REV],
 *! @[GL_UNSIGNED_INT_10_10_10_2], or @[GL_UNSIGNED_INT_2_10_10_10_REV]
 *! and @i{format@} is neither @[GL_RGBA] nor @[GL_BGRA].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the buffer
 *! object's data store is currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and the data would be
 *! unpacked from the buffer object such that the memory reads required
 *! would exceed the data store size.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to the @[GL_PIXEL_UNPACK_BUFFER] target and @i{data@} is not
 *! evenly divisible into the number of bytes needed to store in memory a
 *! datum indicated by @i{type@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glTexSubImage1D] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glTexSubImage1D] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! Texturing has no effect in color index mode.
 *! 
 *! @[glPixelStore] and @[glPixelTransfer] modes affect texture images in
 *! exactly the way they affect @[glDrawPixels].
 *! 
 *! Formats @[GL_BGR], and @[GL_BGRA] and types @[GL_UNSIGNED_BYTE_3_3_2],
 *! @[GL_UNSIGNED_BYTE_2_3_3_REV], @[GL_UNSIGNED_SHORT_5_6_5],
 *! @[GL_UNSIGNED_SHORT_5_6_5_REV], @[GL_UNSIGNED_SHORT_4_4_4_4],
 *! @[GL_UNSIGNED_SHORT_4_4_4_4_REV], @[GL_UNSIGNED_SHORT_5_5_5_1],
 *! @[GL_UNSIGNED_SHORT_1_5_5_5_REV], @[GL_UNSIGNED_INT_8_8_8_8],
 *! @[GL_UNSIGNED_INT_8_8_8_8_REV], @[GL_UNSIGNED_INT_10_10_10_2], and
 *! @[GL_UNSIGNED_INT_2_10_10_10_REV] are available only if the GL version
 *! is 1.2 or greater.
 *! 
 *! For OpenGL versions 1.3 and greater, or when the
 *! @tt{ARB_multitexture@} extension is supported, @[glTexSubImage1D]
 *! specifies a one-dimensional subtexture for the current texture unit,
 *! specified with @[glActiveTexture].
 *! 
 *! When the @tt{ARB_imaging@} extension is supported, the RGBA components
 *! specified in @i{data@} may be processed by the imaging pipeline. See
 *! @[glTexImage1D] for specific details.
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glCopyTexSubImage3D], @[glDrawPixels], @[glPixelStore],
 *! @[glPixelTransfer], @[glTexEnv], @[glTexGen], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexImage3D], @[glTexParameter],
 *! @[glTexSubImage2D], @[glTexSubImage3D]
 *! 
 */


/*! @decl void glTranslate(float|int|array(float|int) x, float|int|void y, @
 *! float|int|void z)
 *! 
 *! @[glTranslate] produces a translation by (@i{x@}, @i{y@}, @i{z@}). The
 *! current matrix (see @[glMatrixMode]) is multiplied by this translation
 *! matrix, with the product replacing the current matrix, as if
 *! @[glMultMatrix] were called with the following matrix for its
 *! argument:
 *! 
 *! (@xml{<matrix><r><c>1</c><c>0</c><c>0</c><c><i>x</i></c></r><r><c>0</c
 *! ><c>1</c><c>0</c><c><i>y</i></c></r><r><c>0</c><c>0</c><c>1</c><c><i
 *! >z</i></c></r><r><c>0</c><c>0</c><c>0</c><c>1</c></r></matrix>@})
 *! 
 *! If the matrix mode is either @[GL_MODELVIEW] or @[GL_PROJECTION], all
 *! objects drawn after a call to @[glTranslate] are translated.
 *! 
 *! Use @[glPushMatrix] and @[glPopMatrix] to save and restore the
 *! untranslated coordinate system.
 *! 
 *! @param x
 *! @param y
 *! @param z
 *! 
 *! Specify the @i{x@}, @i{y@}, and @i{z@} coordinates of a translation
 *! vector.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glTranslate] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glMatrixMode], @[glMultMatrix], @[glPushMatrix], @[glRotate],
 *! @[glScale]
 *! 
 */


/*! @decl void glVertex(float|int x, float|int y, float|int|void z, @
 *! float|int|void w)
 *! @decl void glVertex(array(float|int) v)
 *! 
 *! @[glVertex] commands are used within @[glBegin]/@[glEnd] pairs to
 *! specify point, line, and polygon vertices. The current color, normal,
 *! texture coordinates, and fog coordinate are associated with the vertex
 *! when @[glVertex] is called.
 *! 
 *! When only @i{x@} and @i{y@} are specified, @i{z@} defaults to 0 and
 *! @i{w@} defaults to 1. When @i{x@}, @i{y@}, and @i{z@} are specified,
 *! @i{w@} defaults to 1.
 *! 
 *! @param x
 *! @param y
 *! @param z
 *! @param w
 *! 
 *! Specify @i{x@}, @i{y@}, @i{z@}, and @i{w@} coordinates of a vertex.
 *! Not all parameters are present in all forms of the command.
 *! 
 *! @param v
 *! 
 *! Specifies an array of two, three, or four elements. The elements of a
 *! two-element array are @i{x@} and @i{y@}; of a three-element array,
 *! @i{x@}, @i{y@}, and @i{z@}; and of a four-element array, @i{x@},
 *! @i{y@}, @i{z@}, and @i{w@}.
 *! 
 *! @note
 *! 
 *! Invoking @[glVertex] outside of a @[glBegin]/@[glEnd] pair results in
 *! undefined behavior.
 *! 
 *! @seealso
 *! 
 *! @[glBegin], @[glCallList], @[glColor], @[glEdgeFlag], @[glEvalCoord],
 *! @[glFogCoord], @[glIndex], @[glMaterial], @[glMultiTexCoord],
 *! @[glNormal], @[glRect], @[glTexCoord], @[glVertexPointer]
 *! 
 */


/*! @decl void glViewport(int x, int y, int width, int height)
 *! 
 *! @[glViewport] specifies the affine transformation of @i{x@} and @i{y@}
 *! from normalized device coordinates to window coordinates. Let
 *! (@i{x@}@sub{nd@}, @i{y@}@sub{nd@}) be normalized device coordinates.
 *! Then the window coordinates (@i{x@}@sub{@i{w@}@}, @i{y@}@sub{@i{w@}@})
 *! are computed as follows:
 *! 
 *! @i{x@}@sub{@i{w@}@}=(@i{x@}@sub{nd@}+1)(width/2)+@i{x@}
 *! 
 *! @i{y@}@sub{@i{w@}@}=(@i{y@}@sub{nd@}+1)(height/2)+@i{y@}
 *! 
 *! Viewport width and height are silently clamped to a range that depends
 *! on the implementation. To query this range, call @[glGet] with
 *! argument @[GL_MAX_VIEWPORT_DIMS].
 *! 
 *! @param x
 *! @param y
 *! 
 *! Specify the lower left corner of the viewport rectangle, in pixels.
 *! The initial value is (0,0).
 *! 
 *! @param width
 *! @param height
 *! 
 *! Specify the width and height of the viewport. When a GL context is
 *! first attached to a window, @i{width@} and @i{height@} are set to the
 *! dimensions of that window.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if either @i{width@} or @i{height@}
 *! is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glViewport] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glDepthRange]
 *! 
 */


/*! @decl void glCallLists(int ... lists)
 *! 
 *! @[glCallLists] causes each display list in the list of names passed as
 *! @i{lists@} to be executed. As a result, the commands saved in each
 *! display list are executed in order, just as if they were called
 *! without using a display list. Names of display lists that have not
 *! been defined are ignored.
 *! 
 *! @[glCallLists] provides an efficient means for executing more than one
 *! display list.
 *! 
 *! An additional level of indirection is made available with the
 *! @[glListBase] command, which specifies an unsigned offset that is
 *! added to each display-list name specified in @i{lists@} before that
 *! display list is executed.
 *! 
 *! @[glCallLists] can appear inside a display list. To avoid the
 *! possibility of infinite recursion resulting from display lists calling
 *! one another, a limit is placed on the nesting level of display lists
 *! during display-list execution. This limit must be at least 64, and it
 *! depends on the implementation.
 *! 
 *! GL state is not saved and restored across a call to @[glCallLists].
 *! Thus, changes made to GL state during the execution of the display
 *! lists remain after execution is completed. Use @[glPushAttrib],
 *! @[glPopAttrib], @[glPushMatrix], and @[glPopMatrix] to preserve GL
 *! state across @[glCallLists] calls.
 *! 
 *! @param lists
 *! 
 *! Specifies the address of an array of name offsets in the display list.
 *! 
 *! @note
 *! 
 *! Display lists can be executed between a call to @[glBegin] and the
 *! corresponding call to @[glEnd], as long as the display list includes
 *! only commands that are allowed in this interval.
 *! 
 *! @seealso
 *! 
 *! @[glCallList], @[glDeleteLists], @[glGenLists], @[glListBase],
 *! @[glNewList], @[glPushAttrib], @[glPushMatrix]
 *! 
 */


/*! @decl void glDeleteTextures(int ... textures)
 *! 
 *! @[glDeleteTextures] deletes @i{n@} textures named by the elements of
 *! the array @i{textures@}. After a texture is deleted, it has no
 *! contents or dimensionality, and its name is free for reuse (for
 *! example by @[glGenTextures]). If a texture that is currently bound is
 *! deleted, the binding reverts to 0 (the default texture).
 *! 
 *! @[glDeleteTextures] silently ignores 0's and names that do not
 *! correspond to existing textures.
 *! 
 *! @param textures
 *! 
 *! Specifies the textures to be deleted.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDeleteTextures] is
 *! executed between the execution of @[glBegin] and the corresponding
 *! execution of @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glDeleteTextures] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @seealso
 *! 
 *! @[glAreTexturesResident], @[glBindTexture], @[glCopyTexImage1D],
 *! @[glCopyTexImage2D], @[glGenTextures], @[glGet], @[glGetTexParameter],
 *! @[glPrioritizeTextures], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexParameter]
 *! 
 */


/*! @decl void glFrustum(float left, float right, float bottom, float top, @
 *! float nearVal, float farVal)
 *! 
 *! @[glFrustum] describes a perspective matrix that produces a
 *! perspective projection. The current matrix (see @[glMatrixMode]) is
 *! multiplied by this matrix and the result replaces the current matrix,
 *! as if @[glMultMatrix] were called with the following matrix as its
 *! argument:
 *! 
 *! [@xml{<matrix><r><c>2nearVal/right-left</c><c>0</c><c><i>A</i></c><c
 *! >0</c></r><r><c>0</c><c>2nearVal/top-bottom</c><c><i>B</i></c><c>0</c
 *! ></r><r><c>0</c><c>0</c><c><i>C</i></c><c><i>D</i></c></r><r><c>0</c
 *! ><c>0</c><c>-1</c><c>0</c></r></matrix>@}]
 *! 
 *! @i{A@}=right+left/right-left
 *! 
 *! @i{B@}=top+bottom/top-bottom
 *! 
 *! @i{C@}=-farVal+nearVal/farVal-nearVal
 *! 
 *! @i{D@}=-2farValnearVal/farVal-nearVal
 *! 
 *! Typically, the matrix mode is @[GL_PROJECTION], and
 *! (left, bottom, -nearVal)
 *! and
 *! (right, top, -nearVal)
 *! specify the points on the near clipping plane that are mapped to the
 *! lower left and upper right corners of the window, assuming that the
 *! eye is located at (0, 0, 0).
 *! -farVal
 *! specifies the location of the far clipping plane. Both @i{nearVal@}
 *! and @i{farVal@} must be positive.
 *! 
 *! Use @[glPushMatrix] and @[glPopMatrix] to save and restore the current
 *! matrix stack.
 *! 
 *! @param left
 *! @param right
 *! 
 *! Specify the coordinates for the left and right vertical clipping
 *! planes.
 *! 
 *! @param bottom
 *! @param top
 *! 
 *! Specify the coordinates for the bottom and top horizontal clipping
 *! planes.
 *! 
 *! @param nearVal
 *! @param farVal
 *! 
 *! Specify the distances to the near and far depth clipping planes. Both
 *! distances must be positive.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{nearVal@} or @i{farVal@} is not
 *! positive, or if @i{left@} = @i{right@}, or @i{bottom@} = @i{top@}, or
 *! @i{near@} = @i{far@}.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glFrustum] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Depth buffer precision is affected by the values specified for
 *! @i{nearVal@} and @i{farVal@}. The greater the ratio of @i{farVal@} to
 *! @i{nearVal@} is, the less effective the depth buffer will be at
 *! distinguishing between surfaces that are near each other. If
 *! 
 *! @i{r@}=farVal/nearVal
 *! 
 *! roughly
 *! log@sub{2@}(@i{r@})
 *! bits of depth buffer precision are lost. Because @i{r@} approaches
 *! infinity as @i{nearVal@} approaches 0, @i{nearVal@} must never be set
 *! to 0.
 *! 
 *! @seealso
 *! 
 *! @[glOrtho], @[glMatrixMode], @[glMultMatrix], @[glPushMatrix],
 *! @[glViewport]
 *! 
 */


/*! @decl array(int) glGenTextures(int n)
 *! 
 *! @[glGenTextures] returns @i{n@} texture names in an array. There is no
 *! guarantee that the names form a contiguous set of integers; however,
 *! it is guaranteed that none of the returned names was in use
 *! immediately before the call to @[glGenTextures].
 *! 
 *! The generated textures have no dimensionality; they assume the
 *! dimensionality of the texture target to which they are first bound
 *! (see @[glBindTexture]).
 *! 
 *! Texture names returned by a call to @[glGenTextures] are not returned
 *! by subsequent calls, unless they are first deleted with
 *! @[glDeleteTextures].
 *! 
 *! @param n
 *! 
 *! Specifies the number of texture names to be generated.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if @i{n@} is negative.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glGenTextures] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glGenTextures] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @seealso
 *! 
 *! @[glBindTexture], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glDeleteTextures], @[glGet], @[glGetTexParameter], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexImage3D], @[glTexParameter]
 *! 
 */


/*! @decl void glDrawElements(int mode, array(int) indices)
 *! 
 *! @[glDrawElements] specifies multiple geometric primitives with very
 *! few subroutine calls. Instead of calling a GL function to pass each
 *! individual vertex, normal, texture coordinate, edge flag, or color,
 *! you can prespecify separate arrays of vertices, normals, and so on,
 *! and use them to construct a sequence of primitives with a single call
 *! to @[glDrawElements].
 *! 
 *! When @[glDrawElements] is called, it uses sequential elements from an
 *! enabled array @i{indices@} to construct a sequence of geometric
 *! primitives. @i{mode@} specifies what kind of primitives are
 *! constructed and how the array elements construct these primitives. If
 *! more than one array is enabled, each is used. If @[GL_VERTEX_ARRAY] is
 *! not enabled, no geometric primitives are constructed.
 *! 
 *! Vertex attributes that are modified by @[glDrawElements] have an
 *! unspecified value after @[glDrawElements] returns. For example, if
 *! @[GL_COLOR_ARRAY] is enabled, the value of the current color is
 *! undefined after @[glDrawElements] executes. Attributes that aren't
 *! modified maintain their previous values.
 *! 
 *! @param mode
 *! 
 *! Specifies what kind of primitives to render. Symbolic constants
 *! @[GL_POINTS], @[GL_LINE_STRIP], @[GL_LINE_LOOP], @[GL_LINES],
 *! @[GL_TRIANGLE_STRIP], @[GL_TRIANGLE_FAN], @[GL_TRIANGLES],
 *! @[GL_QUAD_STRIP], @[GL_QUADS], and @[GL_POLYGON] are accepted.
 *! 
 *! @param indices
 *! 
 *! Specifies an array of indices.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a non-zero buffer object name
 *! is bound to an enabled array or the element array and the buffer
 *! object's data store is currently mapped.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDrawElements] is executed
 *! between the execution of @[glBegin] and the corresponding @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glDrawElements] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @[glDrawElements] is included in display lists. If @[glDrawElements]
 *! is entered into a display list, the necessary array data (determined
 *! by the array pointers and enables) is also entered into the display
 *! list. Because the array pointers and enables are client-side state,
 *! their values affect display lists when the lists are created, not when
 *! the lists are executed.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glColorPointer], @[glDrawArrays],
 *! @[glDrawRangeElements], @[glEdgeFlagPointer], @[glFogCoordPointer],
 *! @[glGetPointerv], @[glIndexPointer], @[glInterleavedArrays],
 *! @[glNormalPointer], @[glSecondaryColorPointer], @[glTexCoordPointer],
 *! @[glVertexPointer]
 *! 
 */


/*! @decl void glMapGrid(int un, float u1, float u2)
 *! @decl void glMapGrid(int un, float u1, float u2, int vn, float v1, @
 *! float v2)
 *! 
 *! @[glMapGrid] and @[glEvalMesh1] and @[glEvalMesh2] are used together
 *! to efficiently generate and evaluate a series of evenly-spaced map
 *! domain values. @[glEvalMesh1] and @[glEvalMesh2] steps through the
 *! integer domain of a one- or two-dimensional grid, whose range is the
 *! domain of the evaluation maps specified by @[glMap1] and @[glMap2].
 *! 
 *! @[glMapGrid] specifies the linear grid mappings between the @i{i@} (or
 *! @i{i@} and @i{j@}) integer grid coordinates, to the @i{u@} (or @i{u@}
 *! and @i{v@}) floating-point evaluation map coordinates. See @[glMap1]
 *! and @[glMap2] for details of how @i{u@} and @i{v@} coordinates are
 *! evaluated.
 *! 
 *! @[glMapGrid] with three arguments specifies a single linear mapping
 *! such that integer grid coordinate 0 maps exactly to @i{u1@}, and
 *! integer grid coordinate @i{un@} maps exactly to @i{u2@}. All other
 *! integer grid coordinates @i{i@} are mapped so that
 *! 
 *! @i{u@}=@i{i@}(u2-u1)/un+u1
 *! 
 *! @[glMapGrid] with six arguments specifies two such linear mappings.
 *! One maps integer grid coordinate
 *! @i{i@}=0
 *! exactly to @i{u1@}, and integer grid coordinate
 *! @i{i@}=un
 *! exactly to @i{u2@}. The other maps integer grid coordinate
 *! @i{j@}=0
 *! exactly to @i{v1@}, and integer grid coordinate
 *! @i{j@}=vn
 *! exactly to @i{v2@}. Other integer grid coordinates @i{i@} and @i{j@}
 *! are mapped such that
 *! 
 *! @i{u@}=@i{i@}(u2-u1)/un+u1
 *! 
 *! @i{v@}=@i{j@}(v2-v1)/vn+v1
 *! 
 *! The mappings specified by @[glMapGrid] are used identically by
 *! @[glEvalMesh1] and @[glEvalMesh2] and @[glEvalPoint].
 *! 
 *! @param un
 *! 
 *! Specifies the number of partitions in the grid range interval
 *! [@i{u1@}, @i{u2@}]. Must be positive.
 *! 
 *! @param u1
 *! @param u2
 *! 
 *! Specify the mappings for integer grid domain values
 *! @i{i@}=0
 *! and
 *! @i{i@}=un
 *! .
 *! 
 *! @param vn
 *! 
 *! Specifies the number of partitions in the grid range interval
 *! [@i{v1@}, @i{v2@}] (With six arguments only).
 *! 
 *! @param v1
 *! @param v2
 *! 
 *! Specify the mappings for integer grid domain values
 *! @i{j@}=0
 *! and
 *! @i{j@}=vn
 *! (With six arguments only).
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_VALUE] is generated if either @i{un@} or @i{vn@} is not
 *! positive.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glMapGrid] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glEvalCoord], @[glEvalMesh1] and @[glEvalMesh2], @[glEvalPoint],
 *! @[glMap1], @[glMap2]
 *! 
 */


/*! @decl int|float|array(int)|array(float) glGet(int pname)
 *! 
 *! This command returns values for simple state variables in GL.
 *! @i{pname@} is a symbolic constant indicating the state variable to be
 *! returned.
 *! 
 *! Type conversion is performed if @i{params@} has a different type than
 *! the state variable value being requested. If @[glGet] is called, a
 *! floating-point (or integer) value is converted to @[GL_FALSE] if and
 *! only if it is 0.0 (or 0). Otherwise, it is converted to @[GL_TRUE]. If
 *! @[glGet] is called, boolean values are returned as @[GL_TRUE] or
 *! @[GL_FALSE], and most floating-point values are rounded to the nearest
 *! integer value. Floating-point colors and normals, however, are
 *! returned with a linear mapping that maps 1.0 to the most positive
 *! representable integer value and -1.0 to the most negative
 *! representable integer value. If @[glGet] or @[glGet] is called,
 *! boolean values are returned as @[GL_TRUE] or @[GL_FALSE], and integer
 *! values are converted to floating-point values.
 *! 
 *! The following symbolic constants are accepted by @i{pname@}:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_ACCUM_ALPHA_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of alpha
 *! bitplanes in the accumulation buffer. </p></c></r>
 *! <r><c><ref>GL_ACCUM_BLUE_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of blue
 *! bitplanes in the accumulation buffer. </p></c></r>
 *! <r><c><ref>GL_ACCUM_CLEAR_VALUE</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the red, green, blue,
 *! and alpha values used to clear the accumulation buffer. Integer
 *! values, if requested, are linearly mapped from the internal
 *! floating-point representation such that 1.0 returns the most positive
 *! representable integer value, and -1.0 returns the most negative
 *! representable integer value. The initial value is (0, 0, 0, 0). See
 *! <ref>glClearAccum</ref>. </p></c></r>
 *! <r><c><ref>GL_ACCUM_GREEN_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of green
 *! bitplanes in the accumulation buffer. </p></c></r>
 *! <r><c><ref>GL_ACCUM_RED_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of red
 *! bitplanes in the accumulation buffer. </p></c></r>
 *! <r><c><ref>GL_ACTIVE_TEXTURE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value indicating the
 *! active multitexture unit. The initial value is <ref>GL_TEXTURE0</ref>.
 *! See <ref>glActiveTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_ALIASED_POINT_SIZE_RANGE</ref></c>
 *! <c><p></p><p><i>params</i> returns two values, the smallest and
 *! largest supported sizes for aliased points. </p></c></r>
 *! <r><c><ref>GL_ALIASED_LINE_WIDTH_RANGE</ref></c>
 *! <c><p></p><p><i>params</i> returns two values, the smallest and
 *! largest supported widths for aliased lines. </p></c></r>
 *! <r><c><ref>GL_ALPHA_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the alpha bias factor
 *! used during pixel transfers. The initial value is 0. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_ALPHA_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of alpha
 *! bitplanes in each color buffer. </p></c></r>
 *! <r><c><ref>GL_ALPHA_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the alpha scale factor
 *! used during pixel transfers. The initial value is 1. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_ALPHA_TEST</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether alpha testing of fragments is enabled. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glAlphaFunc</ref>. </p></c></r>
 *! <r><c><ref>GL_ALPHA_TEST_FUNC</ref><i>params</i> returns one
 *! value,</c>
 *! <c><p></p><p>the symbolic name of the alpha test function. The initial
 *! value is <ref>GL_ALWAYS</ref>. See <ref>glAlphaFunc</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_ALPHA_TEST_REF</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the reference value for
 *! the alpha test. The initial value is 0. See <ref>glAlphaFunc</ref>. An
 *! integer value, if requested, is linearly mapped from the internal
 *! floating-point representation such that 1.0 returns the most positive
 *! representable integer value, and -1.0 returns the most negative
 *! representable integer value. </p></c></r>
 *! <r><c><ref>GL_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object currently bound to the target <ref>GL_ARRAY_BUFFER</ref
 *! >. If no buffer object is bound to this target, 0 is returned. The
 *! initial value is 0. See <ref>glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_ATTRIB_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the depth of the
 *! attribute stack. If the stack is empty, 0 is returned. The initial
 *! value is 0. See <ref>glPushAttrib</ref>. </p></c></r>
 *! <r><c><ref>GL_AUTO_NORMAL</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D map evaluation automatically generates surface normals. The
 *! initial value is <ref>GL_FALSE</ref>. See <ref>glMap2</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_AUX_BUFFERS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of auxiliary
 *! color buffers available. </p></c></r>
 *! <r><c><ref>GL_BLEND</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether blending is enabled. The initial value is <ref>GL_FALSE</ref>.
 *! See <ref>glBlendFunc</ref>. </p></c></r>
 *! <r><c><ref>GL_BLEND_COLOR</ref></c>
 *! <c><p></p><p><i>params</i> returns four values, the red, green, blue,
 *! and alpha values which are the components of the blend color. See <ref
 *! >glBlendColor</ref>. </p></c></r>
 *! <r><c><ref>GL_BLEND_DST_ALPHA</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the symbolic constant
 *! identifying the alpha destination blend function. The initial value is
 *! <ref>GL_ZERO</ref>. See <ref>glBlendFunc</ref> and <ref
 *! >glBlendFuncSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_BLEND_DST_RGB</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the symbolic constant
 *! identifying the RGB destination blend function. The initial value is
 *! <ref>GL_ZERO</ref>. See <ref>glBlendFunc</ref> and <ref
 *! >glBlendFuncSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_BLEND_EQUATION_RGB</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating whether the RGB blend equation is <ref>GL_FUNC_ADD</ref>,
 *! <ref>GL_FUNC_SUBTRACT</ref>, <ref>GL_FUNC_REVERSE_SUBTRACT</ref>, <ref
 *! >GL_MIN</ref> or <ref>GL_MAX</ref>. See <ref
 *! >glBlendEquationSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_BLEND_EQUATION_ALPHA</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating whether the Alpha blend equation is <ref>GL_FUNC_ADD</ref>,
 *! <ref>GL_FUNC_SUBTRACT</ref>, <ref>GL_FUNC_REVERSE_SUBTRACT</ref>, <ref
 *! >GL_MIN</ref> or <ref>GL_MAX</ref>. See <ref
 *! >glBlendEquationSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_BLEND_SRC_ALPHA</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the symbolic constant
 *! identifying the alpha source blend function. The initial value is <ref
 *! >GL_ONE</ref>. See <ref>glBlendFunc</ref> and <ref
 *! >glBlendFuncSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_BLEND_SRC_RGB</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the symbolic constant
 *! identifying the RGB source blend function. The initial value is <ref
 *! >GL_ONE</ref>. See <ref>glBlendFunc</ref> and <ref
 *! >glBlendFuncSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_BLUE_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the blue bias factor
 *! used during pixel transfers. The initial value is 0. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_BLUE_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of blue
 *! bitplanes in each color buffer. </p></c></r>
 *! <r><c><ref>GL_BLUE_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the blue scale factor
 *! used during pixel transfers. The initial value is 1. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_CLIENT_ACTIVE_TEXTURE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single integer value indicating
 *! the current client active multitexture unit. The initial value is <ref
 *! >GL_TEXTURE0</ref>. See <ref>glClientActiveTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_CLIENT_ATTRIB_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value indicating the depth of
 *! the attribute stack. The initial value is 0. See <ref
 *! >glPushClientAttrib</ref>. </p></c></r>
 *! <r><c><tt>GL_CLIP_PLANE</tt><i>i</i></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the specified clipping plane is enabled. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glClipPlane</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_ARRAY</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the color array is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glColorPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object associated with the color array. This buffer object
 *! would have been bound to the target <ref>GL_ARRAY_BUFFER</ref> at the
 *! time of the most recent call to <ref>glColorPointer</ref>. If no
 *! buffer object was bound to this target, 0 is returned. The initial
 *! value is 0. See <ref>glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_ARRAY_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of components
 *! per color in the color array. The initial value is 4. See <ref
 *! >glColorPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_ARRAY_STRIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte offset between
 *! consecutive colors in the color array. The initial value is 0. See
 *! <ref>glColorPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_ARRAY_TYPE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the data type of each
 *! component in the color array. The initial value is <ref>GL_FLOAT</ref
 *! >. See <ref>glColorPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_CLEAR_VALUE</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the red, green, blue,
 *! and alpha values used to clear the color buffers. Integer values, if
 *! requested, are linearly mapped from the internal floating-point
 *! representation such that 1.0 returns the most positive representable
 *! integer value, and -1.0 returns the most negative representable
 *! integer value. The initial value is (0, 0, 0, 0). See <ref
 *! >glClearColor</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_LOGIC_OP</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether a fragment's RGBA color values are merged into the framebuffer
 *! using a logical operation. The initial value is <ref>GL_FALSE</ref>.
 *! See <ref>glLogicOp</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_MATERIAL</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether one or more material parameters are tracking the current
 *! color. The initial value is <ref>GL_FALSE</ref>. See <ref
 *! >glColorMaterial</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_MATERIAL_FACE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating which materials have a parameter that is tracking the
 *! current color. The initial value is <ref>GL_FRONT_AND_BACK</ref>. See
 *! <ref>glColorMaterial</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_MATERIAL_PARAMETER</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating which material parameters are tracking the current color.
 *! The initial value is <ref>GL_AMBIENT_AND_DIFFUSE</ref>. See <ref
 *! >glColorMaterial</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_MATRIX</ref></c>
 *! <c><p></p><p><i>params</i> returns sixteen values: the color matrix on
 *! the top of the color matrix stack. Initially this matrix is the
 *! identity matrix. See <ref>glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_MATRIX_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! depth of the projection matrix stack. The value must be at least 2.
 *! See <ref>glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_SUM</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether primary and secondary color sum is enabled. See <ref
 *! >glSecondaryColor</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_TABLE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the color table lookup is enabled. See <ref>glColorTable</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_COLOR_WRITEMASK</ref></c>
 *! <c><p></p><p><i>params</i> returns four boolean values: the red,
 *! green, blue, and alpha write enables for the color buffers. The
 *! initial value is (<ref>GL_TRUE</ref>, <ref>GL_TRUE</ref>, <ref
 *! >GL_TRUE</ref>, <ref>GL_TRUE</ref>). See <ref>glColorMask</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_COMPRESSED_TEXTURE_FORMATS</ref></c>
 *! <c><p></p><p><i>params</i> returns a list of symbolic constants of
 *! length <ref>GL_NUM_COMPRESSED_TEXTURE_FORMATS</ref> indicating which
 *! compressed texture formats are available. See <ref
 *! >glCompressedTexImage2D</ref>. </p></c></r>
 *! <r><c><ref>GL_CONVOLUTION_1D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D convolution is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glConvolutionFilter1D</ref>. </p></c></r>
 *! <r><c><ref>GL_CONVOLUTION_2D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D convolution is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glConvolutionFilter2D</ref>. </p></c></r>
 *! <r><c><ref>GL_CULL_FACE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether polygon culling is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glCullFace</ref>. </p></c></r>
 *! <r><c><ref>GL_CULL_FACE_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating which polygon faces are to be culled. The initial value is
 *! <ref>GL_BACK</ref>. See <ref>glCullFace</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_COLOR</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the red, green, blue,
 *! and alpha values of the current color. Integer values, if requested,
 *! are linearly mapped from the internal floating-point representation
 *! such that 1.0 returns the most positive representable integer value,
 *! and -1.0 returns the most negative representable integer value. The
 *! initial value is (1, 1, 1, 1). See <ref>glColor</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_FOG_COORD</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the current fog
 *! coordinate. The initial value is 0. See <ref>glFogCoord</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_CURRENT_INDEX</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the current color index.
 *! The initial value is 1. See <ref>glIndex</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_NORMAL</ref></c>
 *! <c><p></p><p><i>params</i> returns three values: the <i>x</i>, <i>y</i
 *! >, and <i>z</i> values of the current normal. Integer values, if
 *! requested, are linearly mapped from the internal floating-point
 *! representation such that 1.0 returns the most positive representable
 *! integer value, and -1.0 returns the most negative representable
 *! integer value. The initial value is (0, 0, 1). See <ref>glNormal</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_CURRENT_PROGRAM</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the name of the program
 *! object that is currently active, or 0 if no program object is active.
 *! See <ref>glUseProgram</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_RASTER_COLOR</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the red, green, blue,
 *! and alpha color values of the current raster position. Integer values,
 *! if requested, are linearly mapped from the internal floating-point
 *! representation such that 1.0 returns the most positive representable
 *! integer value, and -1.0 returns the most negative representable
 *! integer value. The initial value is (1, 1, 1, 1). See <ref
 *! >glRasterPos</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_RASTER_DISTANCE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the distance from the
 *! eye to the current raster position. The initial value is 0. See <ref
 *! >glRasterPos</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_RASTER_INDEX</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the color index of the
 *! current raster position. The initial value is 1. See <ref
 *! >glRasterPos</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_RASTER_POSITION</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the <i>x</i>, <i>y</i
 *! >, <i>z</i>, and <i>w</i> components of the current raster position.
 *! <i>x</i>, <i>y</i>, and <i>z</i> are in window coordinates, and <i
 *! >w</i> is in clip coordinates. The initial value is (0, 0, 0, 1). See
 *! <ref>glRasterPos</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_RASTER_POSITION_VALID</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the current raster position is valid. The initial value is
 *! <ref>GL_TRUE</ref>. See <ref>glRasterPos</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_RASTER_SECONDARY_COLOR</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the red, green, blue,
 *! and alpha secondary color values of the current raster position.
 *! Integer values, if requested, are linearly mapped from the internal
 *! floating-point representation such that 1.0 returns the most positive
 *! representable integer value, and -1.0 returns the most negative
 *! representable integer value. The initial value is (1, 1, 1, 1). See
 *! <ref>glRasterPos</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_RASTER_TEXTURE_COORDS</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the <i>s</i>, <i>t</i
 *! >, <i>r</i>, and <i>q</i> texture coordinates of the current raster
 *! position. The initial value is (0, 0, 0, 1). See <ref>glRasterPos</ref
 *! > and <ref>glMultiTexCoord</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_SECONDARY_COLOR</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the red, green, blue,
 *! and alpha values of the current secondary color. Integer values, if
 *! requested, are linearly mapped from the internal floating-point
 *! representation such that 1.0 returns the most positive representable
 *! integer value, and -1.0 returns the most negative representable
 *! integer value. The initial value is (0, 0, 0, 0). See <ref
 *! >glSecondaryColor</ref>. </p></c></r>
 *! <r><c><ref>GL_CURRENT_TEXTURE_COORDS</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the <i>s</i>, <i>t</i
 *! >, <i>r</i>, and <i>q</i> current texture coordinates. The initial
 *! value is (0, 0, 0, 1). See <ref>glMultiTexCoord</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the depth bias factor
 *! used during pixel transfers. The initial value is 0. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of bitplanes
 *! in the depth buffer. </p></c></r>
 *! <r><c><ref>GL_DEPTH_CLEAR_VALUE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the value that is used
 *! to clear the depth buffer. Integer values, if requested, are linearly
 *! mapped from the internal floating-point representation such that 1.0
 *! returns the most positive representable integer value, and -1.0
 *! returns the most negative representable integer value. The initial
 *! value is 1. See <ref>glClearDepth</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_FUNC</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the symbolic constant
 *! that indicates the depth comparison function. The initial value is
 *! <ref>GL_LESS</ref>. See <ref>glDepthFunc</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_RANGE</ref></c>
 *! <c><p></p><p><i>params</i> returns two values: the near and far
 *! mapping limits for the depth buffer. Integer values, if requested, are
 *! linearly mapped from the internal floating-point representation such
 *! that 1.0 returns the most positive representable integer value, and
 *! -1.0 returns the most negative representable integer value. The
 *! initial value is (0, 1). See <ref>glDepthRange</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the depth scale factor
 *! used during pixel transfers. The initial value is 1. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_TEST</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether depth testing of fragments is enabled. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glDepthFunc</ref> and <ref
 *! >glDepthRange</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_WRITEMASK</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! if the depth buffer is enabled for writing. The initial value is <ref
 *! >GL_TRUE</ref>. See <ref>glDepthMask</ref>. </p></c></r>
 *! <r><c><ref>GL_DITHER</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether dithering of fragment colors and indices is enabled. The
 *! initial value is <ref>GL_TRUE</ref>. </p></c></r>
 *! <r><c><ref>GL_DOUBLEBUFFER</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether double buffering is supported. </p></c></r>
 *! <r><c><ref>GL_DRAW_BUFFER</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating which buffers are being drawn to. See <ref
 *! >glDrawBuffer</ref>. The initial value is <ref>GL_BACK</ref> if there
 *! are back buffers, otherwise it is <ref>GL_FRONT</ref>. </p></c></r>
 *! <r><c><ref>GL_DRAW_BUFFER</ref><i>i</i></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating which buffers are being drawn to by the corresponding
 *! output color. See <ref>glDrawBuffers</ref>. The initial value of <ref
 *! >GL_DRAW_BUFFER0</ref> is <ref>GL_BACK</ref> if there are back
 *! buffers, otherwise it is <ref>GL_FRONT</ref>. The initial values of
 *! draw buffers for all other output colors is <ref>GL_NONE</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_EDGE_FLAG</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the current edge flag is <ref>GL_TRUE</ref> or <ref
 *! >GL_FALSE</ref>. The initial value is <ref>GL_TRUE</ref>. See <ref
 *! >glEdgeFlag</ref>. </p></c></r>
 *! <r><c><ref>GL_EDGE_FLAG_ARRAY</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the edge flag array is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glEdgeFlagPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_EDGE_FLAG_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object associated with the edge flag array. This buffer object
 *! would have been bound to the target <ref>GL_ARRAY_BUFFER</ref> at the
 *! time of the most recent call to <ref>glEdgeFlagPointer</ref>. If no
 *! buffer object was bound to this target, 0 is returned. The initial
 *! value is 0. See <ref>glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_EDGE_FLAG_ARRAY_STRIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte offset between
 *! consecutive edge flags in the edge flag array. The initial value is 0.
 *! See <ref>glEdgeFlagPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_ELEMENT_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object currently bound to the target <ref
 *! >GL_ELEMENT_ARRAY_BUFFER</ref>. If no buffer object is bound to this
 *! target, 0 is returned. The initial value is 0. See <ref
 *! >glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_FEEDBACK_BUFFER_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the feedback
 *! buffer. See <ref>glFeedbackBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_FEEDBACK_BUFFER_TYPE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the type of the feedback
 *! buffer. See <ref>glFeedbackBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether fogging is enabled. The initial value is <ref>GL_FALSE</ref>.
 *! See <ref>glFog</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_COORD_ARRAY</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the fog coordinate array is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glFogCoordPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_COORD_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object associated with the fog coordinate array. This buffer
 *! object would have been bound to the target <ref>GL_ARRAY_BUFFER</ref>
 *! at the time of the most recent call to <ref>glFogCoordPointer</ref>.
 *! If no buffer object was bound to this target, 0 is returned. The
 *! initial value is 0. See <ref>glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_COORD_ARRAY_STRIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte offset between
 *! consecutive fog coordinates in the fog coordinate array. The initial
 *! value is 0. See <ref>glFogCoordPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_COORD_ARRAY_TYPE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the type of the fog
 *! coordinate array. The initial value is <ref>GL_FLOAT</ref>. See <ref
 *! >glFogCoordPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_COORD_SRC</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the source of the fog coordinate. The initial value is <ref
 *! >GL_FRAGMENT_DEPTH</ref>. See <ref>glFog</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_COLOR</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the red, green, blue,
 *! and alpha components of the fog color. Integer values, if requested,
 *! are linearly mapped from the internal floating-point representation
 *! such that 1.0 returns the most positive representable integer value,
 *! and -1.0 returns the most negative representable integer value. The
 *! initial value is (0, 0, 0, 0). See <ref>glFog</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_DENSITY</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the fog density
 *! parameter. The initial value is 1. See <ref>glFog</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_END</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the end factor for the
 *! linear fog equation. The initial value is 1. See <ref>glFog</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_FOG_HINT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the mode of the fog hint. The initial value is <ref
 *! >GL_DONT_CARE</ref>. See <ref>glHint</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_INDEX</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the fog color index. The
 *! initial value is 0. See <ref>glFog</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating which fog equation is selected. The initial value is <ref
 *! >GL_EXP</ref>. See <ref>glFog</ref>. </p></c></r>
 *! <r><c><ref>GL_FOG_START</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the start factor for the
 *! linear fog equation. The initial value is 0. See <ref>glFog</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_FRAGMENT_SHADER_DERIVATIVE_HINT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the mode of the derivative accuracy hint for fragment
 *! shaders. The initial value is <ref>GL_DONT_CARE</ref>. See <ref
 *! >glHint</ref>. </p></c></r>
 *! <r><c><ref>GL_FRONT_FACE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating whether clockwise or counterclockwise polygon winding is
 *! treated as front-facing. The initial value is <ref>GL_CCW</ref>. See
 *! <ref>glFrontFace</ref>. </p></c></r>
 *! <r><c><ref>GL_GENERATE_MIPMAP_HINT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the mode of the mipmap generation filtering hint. The
 *! initial value is <ref>GL_DONT_CARE</ref>. See <ref>glHint</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_GREEN_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the green bias factor
 *! used during pixel transfers. The initial value is 0. </p></c></r>
 *! <r><c><ref>GL_GREEN_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of green
 *! bitplanes in each color buffer. </p></c></r>
 *! <r><c><ref>GL_GREEN_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the green scale factor
 *! used during pixel transfers. The initial value is 1. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_HISTOGRAM</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether histogram is enabled. The initial value is <ref>GL_FALSE</ref
 *! >. See <ref>glHistogram</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_ARRAY</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the color index array is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glIndexPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object associated with the color index array. This buffer
 *! object would have been bound to the target <ref>GL_ARRAY_BUFFER</ref>
 *! at the time of the most recent call to <ref>glIndexPointer</ref>. If
 *! no buffer object was bound to this target, 0 is returned. The initial
 *! value is 0. See <ref>glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_ARRAY_STRIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte offset between
 *! consecutive color indexes in the color index array. The initial value
 *! is 0. See <ref>glIndexPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_ARRAY_TYPE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the data type of indexes
 *! in the color index array. The initial value is <ref>GL_FLOAT</ref>.
 *! See <ref>glIndexPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of bitplanes
 *! in each color index buffer. </p></c></r>
 *! <r><c><ref>GL_INDEX_CLEAR_VALUE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the color index used to
 *! clear the color index buffers. The initial value is 0. See <ref
 *! >glClearIndex</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_LOGIC_OP</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether a fragment's index values are merged into the framebuffer
 *! using a logical operation. The initial value is <ref>GL_FALSE</ref>.
 *! See <ref>glLogicOp</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the GL is in color index mode (<ref>GL_TRUE</ref>) or RGBA
 *! mode (<ref>GL_FALSE</ref>). </p></c></r>
 *! <r><c><ref>GL_INDEX_OFFSET</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the offset added to
 *! color and stencil indices during pixel transfers. The initial value is
 *! 0. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_SHIFT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the amount that color
 *! and stencil indices are shifted during pixel transfers. The initial
 *! value is 0. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_WRITEMASK</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a mask indicating which
 *! bitplanes of each color index buffer can be written. The initial value
 *! is all 1's. See <ref>glIndexMask</ref>. </p></c></r>
 *! <r><c><tt>GL_LIGHT</tt><i>i</i></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the specified light is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glLight</ref> and <ref>glLightModel</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_LIGHTING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether lighting is enabled. The initial value is <ref>GL_FALSE</ref>.
 *! See <ref>glLightModel</ref>. </p></c></r>
 *! <r><c><ref>GL_LIGHT_MODEL_AMBIENT</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the red, green, blue,
 *! and alpha components of the ambient intensity of the entire scene.
 *! Integer values, if requested, are linearly mapped from the internal
 *! floating-point representation such that 1.0 returns the most positive
 *! representable integer value, and -1.0 returns the most negative
 *! representable integer value. The initial value is (0.2, 0.2, 0.2,
 *! 1.0). See <ref>glLightModel</ref>. </p></c></r>
 *! <r><c><ref>GL_LIGHT_MODEL_COLOR_CONTROL</ref></c>
 *! <c><p></p><p><i>params</i> returns single enumerated value indicating
 *! whether specular reflection calculations are separated from normal
 *! lighting computations. The initial value is <ref>GL_SINGLE_COLOR</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_LIGHT_MODEL_LOCAL_VIEWER</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether specular reflection calculations treat the viewer as being
 *! local to the scene. The initial value is <ref>GL_FALSE</ref>. See <ref
 *! >glLightModel</ref>. </p></c></r>
 *! <r><c><ref>GL_LIGHT_MODEL_TWO_SIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether separate materials are used to compute lighting for front- and
 *! back-facing polygons. The initial value is <ref>GL_FALSE</ref>. See
 *! <ref>glLightModel</ref>. </p></c></r>
 *! <r><c><ref>GL_LINE_SMOOTH</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether antialiasing of lines is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glLineWidth</ref>. </p></c></r>
 *! <r><c><ref>GL_LINE_SMOOTH_HINT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the mode of the line antialiasing hint. The initial value
 *! is <ref>GL_DONT_CARE</ref>. See <ref>glHint</ref>. </p></c></r>
 *! <r><c><ref>GL_LINE_STIPPLE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether stippling of lines is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glLineStipple</ref>. </p></c></r>
 *! <r><c><ref>GL_LINE_STIPPLE_PATTERN</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the 16-bit line stipple
 *! pattern. The initial value is all 1's. See <ref>glLineStipple</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_LINE_STIPPLE_REPEAT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the line stipple repeat
 *! factor. The initial value is 1. See <ref>glLineStipple</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_LINE_WIDTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the line width as
 *! specified with <ref>glLineWidth</ref>. The initial value is 1. </p
 *! ></c></r>
 *! <r><c><ref>GL_LINE_WIDTH_GRANULARITY</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the width difference
 *! between adjacent supported widths for antialiased lines. See <ref
 *! >glLineWidth</ref>. </p></c></r>
 *! <r><c><ref>GL_LINE_WIDTH_RANGE</ref></c>
 *! <c><p></p><p><i>params</i> returns two values: the smallest and
 *! largest supported widths for antialiased lines. See <ref
 *! >glLineWidth</ref>. </p></c></r>
 *! <r><c><ref>GL_LIST_BASE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the base offset added to
 *! all names in arrays presented to <ref>glCallLists</ref>. The initial
 *! value is 0. See <ref>glListBase</ref>. </p></c></r>
 *! <r><c><ref>GL_LIST_INDEX</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the name of the display
 *! list currently under construction. 0 is returned if no display list is
 *! currently under construction. The initial value is 0. See <ref
 *! >glNewList</ref>. </p></c></r>
 *! <r><c><ref>GL_LIST_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the construction mode of the display list currently under
 *! construction. The initial value is 0. See <ref>glNewList</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_LOGIC_OP_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the selected logic operation mode. The initial value is
 *! <ref>GL_COPY</ref>. See <ref>glLogicOp</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_COLOR_4</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates colors. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_GRID_DOMAIN</ref></c>
 *! <c><p></p><p><i>params</i> returns two values: the endpoints of the 1D
 *! map's grid domain. The initial value is (0, 1). See <ref
 *! >glMapGrid</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_GRID_SEGMENTS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of partitions
 *! in the 1D map's grid domain. The initial value is 1. See <ref
 *! >glMapGrid</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_INDEX</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates color indices. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_NORMAL</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates normals. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_TEXTURE_COORD_1</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates 1D texture coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_TEXTURE_COORD_2</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates 2D texture coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_TEXTURE_COORD_3</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates 3D texture coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_TEXTURE_COORD_4</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates 4D texture coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_VERTEX_3</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates 3D vertex coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_VERTEX_4</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D evaluation generates 4D vertex coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_COLOR_4</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates colors. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_GRID_DOMAIN</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the endpoints of the
 *! 2D map's <i>i</i> and <i>j</i> grid domains. The initial value is
 *! (0,1; 0,1). See <ref>glMapGrid</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_GRID_SEGMENTS</ref></c>
 *! <c><p></p><p><i>params</i> returns two values: the number of
 *! partitions in the 2D map's <i>i</i> and <i>j</i> grid domains. The
 *! initial value is (1,1). See <ref>glMapGrid</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_INDEX</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates color indices. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_NORMAL</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates normals. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_TEXTURE_COORD_1</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates 1D texture coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_TEXTURE_COORD_2</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates 2D texture coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_TEXTURE_COORD_3</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates 3D texture coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_TEXTURE_COORD_4</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates 4D texture coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_VERTEX_3</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates 3D vertex coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_VERTEX_4</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D evaluation generates 4D vertex coordinates. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP_COLOR</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! if colors and color indices are to be replaced by table lookup during
 *! pixel transfers. The initial value is <ref>GL_FALSE</ref>. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP_STENCIL</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! if stencil indices are to be replaced by table lookup during pixel
 *! transfers. The initial value is <ref>GL_FALSE</ref>. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_MATRIX_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating which matrix stack is currently the target of all matrix
 *! operations. The initial value is <ref>GL_MODELVIEW</ref>. See <ref
 *! >glMatrixMode</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_3D_TEXTURE_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a rough estimate of the
 *! largest 3D texture that the GL can handle. The value must be at least
 *! 16. If the GL version is 1.2 or greater, use <ref
 *! >GL_PROXY_TEXTURE_3D</ref> to determine if a texture is too large. See
 *! <ref>glTexImage3D</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_CLIENT_ATTRIB_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value indicating the maximum
 *! supported depth of the client attribute stack. See <ref
 *! >glPushClientAttrib</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_ATTRIB_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! depth of the attribute stack. The value must be at least 16. See <ref
 *! >glPushAttrib</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_CLIP_PLANES</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum number of
 *! application-defined clipping planes. The value must be at least 6. See
 *! <ref>glClipPlane</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_COLOR_MATRIX_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! depth of the color matrix stack. The value must be at least 2. See
 *! <ref>glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! texture image units that can be used to access texture maps from the
 *! vertex shader and the fragment processor combined. If both the vertex
 *! shader and the fragment processing stage access the same texture image
 *! unit, then that counts as using two texture image units against this
 *! limit. The value must be at least 2. See <ref>glActiveTexture</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_MAX_CUBE_MAP_TEXTURE_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value. The value gives a rough
 *! estimate of the largest cube-map texture that the GL can handle. The
 *! value must be at least 16. If the GL version is 1.3 or greater, use
 *! <ref>GL_PROXY_TEXTURE_CUBE_MAP</ref> to determine if a texture is too
 *! large. See <ref>glTexImage2D</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_DRAW_BUFFERS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum number of
 *! simultaneous output colors allowed from a fragment shader using the
 *! <tt>gl_FragData</tt> built-in array. The value must be at least 1. See
 *! <ref>glDrawBuffers</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_ELEMENTS_INDICES</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the recommended maximum
 *! number of vertex array indices. See <ref>glDrawRangeElements</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_MAX_ELEMENTS_VERTICES</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the recommended maximum
 *! number of vertex array vertices. See <ref>glDrawRangeElements</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_MAX_EVAL_ORDER</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum equation
 *! order supported by 1D and 2D evaluators. The value must be at least 8.
 *! See <ref>glMap1</ref> and <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_FRAGMENT_UNIFORM_COMPONENTS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum number of
 *! individual floating-point, integer, or boolean values that can be held
 *! in uniform variable storage for a fragment shader. The value must be
 *! at least 64. See <ref>glUniform</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_LIGHTS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum number of
 *! lights. The value must be at least 8. See <ref>glLight</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_MAX_LIST_NESTING</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum recursion
 *! depth allowed during display-list traversal. The value must be at
 *! least 64. See <ref>glCallList</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_MODELVIEW_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! depth of the modelview matrix stack. The value must be at least 32.
 *! See <ref>glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_NAME_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! depth of the selection name stack. The value must be at least 64. See
 *! <ref>glPushName</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_PIXEL_MAP_TABLE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! size of a <ref>glPixelMap</ref> lookup table. The value must be at
 *! least 32. See <ref>glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_PROJECTION_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! depth of the projection matrix stack. The value must be at least 2.
 *! See <ref>glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_TEXTURE_COORDS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum number of
 *! texture coordinate sets available to vertex and fragment shaders. The
 *! value must be at least 2. See <ref>glActiveTexture</ref> and <ref
 *! >glClientActiveTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_TEXTURE_IMAGE_UNITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! texture image units that can be used to access texture maps from the
 *! fragment shader. The value must be at least 2. See <ref
 *! >glActiveTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_TEXTURE_LOD_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum, absolute
 *! value of the texture level-of-detail bias. The value must be at least
 *! 4. </p></c></r>
 *! <r><c><ref>GL_MAX_TEXTURE_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value. The value gives a rough
 *! estimate of the largest texture that the GL can handle. The value must
 *! be at least 64. If the GL version is 1.1 or greater, use <ref
 *! >GL_PROXY_TEXTURE_1D</ref> or <ref>GL_PROXY_TEXTURE_2D</ref> to
 *! determine if a texture is too large. See <ref>glTexImage1D</ref> and
 *! <ref>glTexImage2D</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_TEXTURE_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! depth of the texture matrix stack. The value must be at least 2. See
 *! <ref>glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_TEXTURE_UNITS</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value indicating the
 *! number of conventional texture units supported. Each conventional
 *! texture unit includes both a texture coordinate set and a texture
 *! image unit. Conventional texture units may be used for fixed-function
 *! (non-shader) rendering. The value must be at least 2. Additional
 *! texture coordinate sets and texture image units may be accessed from
 *! vertex and fragment shaders. See <ref>glActiveTexture</ref> and <ref
 *! >glClientActiveTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_VARYING_FLOATS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum number of
 *! interpolators available for processing varying variables used by
 *! vertex and fragment shaders. This value represents the number of
 *! individual floating-point values that can be interpolated; varying
 *! variables declared as vectors, matrices, and arrays will all consume
 *! multiple interpolators. The value must be at least 32. </p></c></r>
 *! <r><c><ref>GL_MAX_VERTEX_ATTRIBS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum number of
 *! 4-component generic vertex attributes accessible to a vertex shader.
 *! The value must be at least 16. See <ref>glVertexAttrib</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum supported
 *! texture image units that can be used to access texture maps from the
 *! vertex shader. The value may be 0. See <ref>glActiveTexture</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_MAX_VERTEX_UNIFORM_COMPONENTS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the maximum number of
 *! individual floating-point, integer, or boolean values that can be held
 *! in uniform variable storage for a vertex shader. The value must be at
 *! least 512. See <ref>glUniform</ref>. </p></c></r>
 *! <r><c><ref>GL_MAX_VIEWPORT_DIMS</ref></c>
 *! <c><p></p><p><i>params</i> returns two values: the maximum supported
 *! width and height of the viewport. These must be at least as large as
 *! the visible dimensions of the display being rendered to. See <ref
 *! >glViewport</ref>. </p></c></r>
 *! <r><c><ref>GL_MINMAX</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether pixel minmax values are computed. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glMinmax</ref>. </p></c></r>
 *! <r><c><ref>GL_MODELVIEW_MATRIX</ref></c>
 *! <c><p></p><p><i>params</i> returns sixteen values: the modelview
 *! matrix on the top of the modelview matrix stack. Initially this matrix
 *! is the identity matrix. See <ref>glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_MODELVIEW_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of matrices
 *! on the modelview matrix stack. The initial value is 1. See <ref
 *! >glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_NAME_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of names on
 *! the selection name stack. The initial value is 0. See <ref
 *! >glPushName</ref>. </p></c></r>
 *! <r><c><ref>GL_NORMAL_ARRAY</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value, indicating
 *! whether the normal array is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glNormalPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_NORMAL_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object associated with the normal array. This buffer object
 *! would have been bound to the target <ref>GL_ARRAY_BUFFER</ref> at the
 *! time of the most recent call to <ref>glNormalPointer</ref>. If no
 *! buffer object was bound to this target, 0 is returned. The initial
 *! value is 0. See <ref>glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_NORMAL_ARRAY_STRIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte offset between
 *! consecutive normals in the normal array. The initial value is 0. See
 *! <ref>glNormalPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_NORMAL_ARRAY_TYPE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the data type of each
 *! coordinate in the normal array. The initial value is <ref
 *! >GL_FLOAT</ref>. See <ref>glNormalPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_NORMALIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether normals are automatically scaled to unit length after they
 *! have been transformed to eye coordinates. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glNormal</ref>. </p></c></r>
 *! <r><c><ref>GL_NUM_COMPRESSED_TEXTURE_FORMATS</ref></c>
 *! <c><p></p><p><i>params</i> returns a single integer value indicating
 *! the number of available compressed texture formats. The minimum value
 *! is 0. See <ref>glCompressedTexImage2D</ref>. </p></c></r>
 *! <r><c><ref>GL_PACK_ALIGNMENT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte alignment used
 *! for writing pixel data to memory. The initial value is 4. See <ref
 *! >glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_PACK_IMAGE_HEIGHT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the image height used
 *! for writing pixel data to memory. The initial value is 0. See <ref
 *! >glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_PACK_LSB_FIRST</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether single-bit pixels being written to memory are written first to
 *! the least significant bit of each unsigned byte. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_PACK_ROW_LENGTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the row length used for
 *! writing pixel data to memory. The initial value is 0. See <ref
 *! >glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_PACK_SKIP_IMAGES</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of pixel
 *! images skipped before the first pixel is written into memory. The
 *! initial value is 0. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_PACK_SKIP_PIXELS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of pixel
 *! locations skipped before the first pixel is written into memory. The
 *! initial value is 0. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_PACK_SKIP_ROWS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of rows of
 *! pixel locations skipped before the first pixel is written into memory.
 *! The initial value is 0. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_PACK_SWAP_BYTES</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the bytes of two-byte and four-byte pixel indices and
 *! components are swapped before being written to memory. The initial
 *! value is <ref>GL_FALSE</ref>. See <ref>glPixelStore</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_PERSPECTIVE_CORRECTION_HINT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the mode of the perspective correction hint. The initial
 *! value is <ref>GL_DONT_CARE</ref>. See <ref>glHint</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_A_TO_A_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! alpha-to-alpha pixel translation table. The initial value is 1. See
 *! <ref>glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_B_TO_B_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! blue-to-blue pixel translation table. The initial value is 1. See <ref
 *! >glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_G_TO_G_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! green-to-green pixel translation table. The initial value is 1. See
 *! <ref>glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_I_TO_A_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! index-to-alpha pixel translation table. The initial value is 1. See
 *! <ref>glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_I_TO_B_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! index-to-blue pixel translation table. The initial value is 1. See
 *! <ref>glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_I_TO_G_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! index-to-green pixel translation table. The initial value is 1. See
 *! <ref>glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_I_TO_I_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! index-to-index pixel translation table. The initial value is 1. See
 *! <ref>glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_I_TO_R_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! index-to-red pixel translation table. The initial value is 1. See <ref
 *! >glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_R_TO_R_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! red-to-red pixel translation table. The initial value is 1. See <ref
 *! >glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_MAP_S_TO_S_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size of the
 *! stencil-to-stencil pixel translation table. The initial value is 1.
 *! See <ref>glPixelMap</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_PACK_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object currently bound to the target <ref
 *! >GL_PIXEL_PACK_BUFFER</ref>. If no buffer object is bound to this
 *! target, 0 is returned. The initial value is 0. See <ref
 *! >glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_PIXEL_UNPACK_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object currently bound to the target <ref
 *! >GL_PIXEL_UNPACK_BUFFER</ref>. If no buffer object is bound to this
 *! target, 0 is returned. The initial value is 0. See <ref
 *! >glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_DISTANCE_ATTENUATION</ref></c>
 *! <c><p></p><p><i>params</i> returns three values, the coefficients for
 *! computing the attenuation value for points. See <ref
 *! >glPointParameter</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_FADE_THRESHOLD_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the point size threshold
 *! for determining the point size. See <ref>glPointParameter</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_POINT_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the point size as
 *! specified by <ref>glPointSize</ref>. The initial value is 1. </p
 *! ></c></r>
 *! <r><c><ref>GL_POINT_SIZE_GRANULARITY</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the size difference
 *! between adjacent supported sizes for antialiased points. See <ref
 *! >glPointSize</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_SIZE_MAX</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the upper bound for the
 *! attenuated point sizes. The initial value is 0.0. See <ref
 *! >glPointParameter</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_SIZE_MIN</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the lower bound for the
 *! attenuated point sizes. The initial value is 1.0. See <ref
 *! >glPointParameter</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_SIZE_RANGE</ref></c>
 *! <c><p></p><p><i>params</i> returns two values: the smallest and
 *! largest supported sizes for antialiased points. The smallest size must
 *! be at most 1, and the largest size must be at least 1. See <ref
 *! >glPointSize</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_SMOOTH</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether antialiasing of points is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glPointSize</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_SMOOTH_HINT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the mode of the point antialiasing hint. The initial value
 *! is <ref>GL_DONT_CARE</ref>. See <ref>glHint</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_SPRITE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether point sprite is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns two values: symbolic constants
 *! indicating whether front-facing and back-facing polygons are
 *! rasterized as points, lines, or filled polygons. The initial value is
 *! <ref>GL_FILL</ref>. See <ref>glPolygonMode</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_OFFSET_FACTOR</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the scaling factor used
 *! to determine the variable offset that is added to the depth value of
 *! each fragment generated when a polygon is rasterized. The initial
 *! value is 0. See <ref>glPolygonOffset</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_OFFSET_UNITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value. This value is multiplied
 *! by an implementation-specific value and then added to the depth value
 *! of each fragment generated when a polygon is rasterized. The initial
 *! value is 0. See <ref>glPolygonOffset</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_OFFSET_FILL</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether polygon offset is enabled for polygons in fill mode. The
 *! initial value is <ref>GL_FALSE</ref>. See <ref>glPolygonOffset</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_POLYGON_OFFSET_LINE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether polygon offset is enabled for polygons in line mode. The
 *! initial value is <ref>GL_FALSE</ref>. See <ref>glPolygonOffset</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_POLYGON_OFFSET_POINT</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether polygon offset is enabled for polygons in point mode. The
 *! initial value is <ref>GL_FALSE</ref>. See <ref>glPolygonOffset</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_POLYGON_SMOOTH</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether antialiasing of polygons is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glPolygonMode</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_SMOOTH_HINT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating the mode of the polygon antialiasing hint. The initial
 *! value is <ref>GL_DONT_CARE</ref>. See <ref>glHint</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_STIPPLE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether polygon stippling is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glPolygonStipple</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_COLOR_TABLE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether post color matrix transformation lookup is enabled. The
 *! initial value is <ref>GL_FALSE</ref>. See <ref>glColorTable</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_RED_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the red bias factor
 *! applied to RGBA fragments after color matrix transformations. The
 *! initial value is 0. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_GREEN_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the green bias factor
 *! applied to RGBA fragments after color matrix transformations. The
 *! initial value is 0. See <ref>glPixelTransfer</ref></p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_BLUE_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the blue bias factor
 *! applied to RGBA fragments after color matrix transformations. The
 *! initial value is 0. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_ALPHA_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the alpha bias factor
 *! applied to RGBA fragments after color matrix transformations. The
 *! initial value is 0. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_RED_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the red scale factor
 *! applied to RGBA fragments after color matrix transformations. The
 *! initial value is 1. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_GREEN_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the green scale factor
 *! applied to RGBA fragments after color matrix transformations. The
 *! initial value is 1. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_BLUE_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the blue scale factor
 *! applied to RGBA fragments after color matrix transformations. The
 *! initial value is 1. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_ALPHA_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the alpha scale factor
 *! applied to RGBA fragments after color matrix transformations. The
 *! initial value is 1. See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_COLOR_TABLE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether post convolution lookup is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glColorTable</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_RED_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the red bias factor
 *! applied to RGBA fragments after convolution. The initial value is 0.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_GREEN_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the green bias factor
 *! applied to RGBA fragments after convolution. The initial value is 0.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_BLUE_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the blue bias factor
 *! applied to RGBA fragments after convolution. The initial value is 0.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_ALPHA_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the alpha bias factor
 *! applied to RGBA fragments after convolution. The initial value is 0.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_RED_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the red scale factor
 *! applied to RGBA fragments after convolution. The initial value is 1.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_GREEN_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the green scale factor
 *! applied to RGBA fragments after convolution. The initial value is 1.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_BLUE_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the blue scale factor
 *! applied to RGBA fragments after convolution. The initial value is 1.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_ALPHA_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the alpha scale factor
 *! applied to RGBA fragments after convolution. The initial value is 1.
 *! See <ref>glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_PROJECTION_MATRIX</ref></c>
 *! <c><p></p><p><i>params</i> returns sixteen values: the projection
 *! matrix on the top of the projection matrix stack. Initially this
 *! matrix is the identity matrix. See <ref>glPushMatrix</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_PROJECTION_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of matrices
 *! on the projection matrix stack. The initial value is 1. See <ref
 *! >glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_READ_BUFFER</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating which color buffer is selected for reading. The initial
 *! value is <ref>GL_BACK</ref> if there is a back buffer, otherwise it is
 *! <ref>GL_FRONT</ref>. See <ref>glReadPixels</ref> and <ref>glAccum</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_RED_BIAS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the red bias factor used
 *! during pixel transfers. The initial value is 0. </p></c></r>
 *! <r><c><ref>GL_RED_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of red
 *! bitplanes in each color buffer. </p></c></r>
 *! <r><c><ref>GL_RED_SCALE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the red scale factor
 *! used during pixel transfers. The initial value is 1. See <ref
 *! >glPixelTransfer</ref>. </p></c></r>
 *! <r><c><ref>GL_RENDER_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating whether the GL is in render, select, or feedback mode. The
 *! initial value is <ref>GL_RENDER</ref>. See <ref>glRenderMode</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_RESCALE_NORMAL</ref></c>
 *! <c><p></p><p><i>params</i> returns single boolean value indicating
 *! whether normal rescaling is enabled. See <ref>glEnable</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_RGBA_MODE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the GL is in RGBA mode (true) or color index mode (false). See
 *! <ref>glColor</ref>. </p></c></r>
 *! <r><c><ref>GL_SAMPLE_BUFFERS</ref></c>
 *! <c><p></p><p><i>params</i> returns a single integer value indicating
 *! the number of sample buffers associated with the framebuffer. See <ref
 *! >glSampleCoverage</ref>. </p></c></r>
 *! <r><c><ref>GL_SAMPLE_COVERAGE_VALUE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single positive floating-point
 *! value indicating the current sample coverage value. See <ref
 *! >glSampleCoverage</ref>. </p></c></r>
 *! <r><c><ref>GL_SAMPLE_COVERAGE_INVERT</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! if the temporary coverage value should be inverted. See <ref
 *! >glSampleCoverage</ref>. </p></c></r>
 *! <r><c><ref>GL_SAMPLES</ref></c>
 *! <c><p></p><p><i>params</i> returns a single integer value indicating
 *! the coverage mask size. See <ref>glSampleCoverage</ref>. </p></c></r>
 *! <r><c><ref>GL_SCISSOR_BOX</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the <i>x</i> and <i
 *! >y</i> window coordinates of the scissor box, followed by its width
 *! and height. Initially the <i>x</i> and <i>y</i> window coordinates are
 *! both 0 and the width and height are set to the size of the window. See
 *! <ref>glScissor</ref>. </p></c></r>
 *! <r><c><ref>GL_SCISSOR_TEST</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether scissoring is enabled. The initial value is <ref>GL_FALSE</ref
 *! >. See <ref>glScissor</ref>. </p></c></r>
 *! <r><c><ref>GL_SECONDARY_COLOR_ARRAY</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the secondary color array is enabled. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glSecondaryColorPointer</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object associated with the secondary color array. This buffer
 *! object would have been bound to the target <ref>GL_ARRAY_BUFFER</ref>
 *! at the time of the most recent call to <ref
 *! >glSecondaryColorPointer</ref>. If no buffer object was bound to this
 *! target, 0 is returned. The initial value is 0. See <ref
 *! >glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_SECONDARY_COLOR_ARRAY_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of components
 *! per color in the secondary color array. The initial value is 3. See
 *! <ref>glSecondaryColorPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_SECONDARY_COLOR_ARRAY_STRIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte offset between
 *! consecutive colors in the secondary color array. The initial value is
 *! 0. See <ref>glSecondaryColorPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_SECONDARY_COLOR_ARRAY_TYPE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the data type of each
 *! component in the secondary color array. The initial value is <ref
 *! >GL_FLOAT</ref>. See <ref>glSecondaryColorPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_SELECTION_BUFFER_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> return one value, the size of the selection
 *! buffer. See <ref>glSelectBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_SEPARABLE_2D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D separable convolution is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glSeparableFilter2D</ref>. </p></c></r>
 *! <r><c><ref>GL_SHADE_MODEL</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating whether the shading mode is flat or smooth. The initial
 *! value is <ref>GL_SMOOTH</ref>. See <ref>glShadeModel</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_SMOOTH_LINE_WIDTH_RANGE</ref></c>
 *! <c><p></p><p><i>params</i> returns two values, the smallest and
 *! largest supported widths for antialiased lines. See <ref
 *! >glLineWidth</ref>. </p></c></r>
 *! <r><c><ref>GL_SMOOTH_LINE_WIDTH_GRANULARITY</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the granularity of
 *! widths for antialiased lines. See <ref>glLineWidth</ref>. </p></c></r>
 *! <r><c><ref>GL_SMOOTH_POINT_SIZE_RANGE</ref></c>
 *! <c><p></p><p><i>params</i> returns two values, the smallest and
 *! largest supported widths for antialiased points. See <ref
 *! >glPointSize</ref>. </p></c></r>
 *! <r><c><ref>GL_SMOOTH_POINT_SIZE_GRANULARITY</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the granularity of sizes
 *! for antialiased points. See <ref>glPointSize</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_BACK_FAIL</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating what action is taken for back-facing polygons when the
 *! stencil test fails. The initial value is <ref>GL_KEEP</ref>. See <ref
 *! >glStencilOpSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_BACK_FUNC</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating what function is used for back-facing polygons to compare
 *! the stencil reference value with the stencil buffer value. The initial
 *! value is <ref>GL_ALWAYS</ref>. See <ref>glStencilFuncSeparate</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_STENCIL_BACK_PASS_DEPTH_FAIL</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating what action is taken for back-facing polygons when the
 *! stencil test passes, but the depth test fails. The initial value is
 *! <ref>GL_KEEP</ref>. See <ref>glStencilOpSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_BACK_PASS_DEPTH_PASS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating what action is taken for back-facing polygons when the
 *! stencil test passes and the depth test passes. The initial value is
 *! <ref>GL_KEEP</ref>. See <ref>glStencilOpSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_BACK_REF</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the reference value that
 *! is compared with the contents of the stencil buffer for back-facing
 *! polygons. The initial value is 0. See <ref>glStencilFuncSeparate</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_STENCIL_BACK_VALUE_MASK</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the mask that is used
 *! for back-facing polygons to mask both the stencil reference value and
 *! the stencil buffer value before they are compared. The initial value
 *! is all 1's. See <ref>glStencilFuncSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_BACK_WRITEMASK</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the mask that controls
 *! writing of the stencil bitplanes for back-facing polygons. The initial
 *! value is all 1's. See <ref>glStencilMaskSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of bitplanes
 *! in the stencil buffer. </p></c></r>
 *! <r><c><ref>GL_STENCIL_CLEAR_VALUE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the index to which the
 *! stencil bitplanes are cleared. The initial value is 0. See <ref
 *! >glClearStencil</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_FAIL</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating what action is taken when the stencil test fails. The
 *! initial value is <ref>GL_KEEP</ref>. See <ref>glStencilOp</ref>. If
 *! the GL version is 2.0 or greater, this stencil state only affects
 *! non-polygons and front-facing polygons. Back-facing polygons use
 *! separate stencil state. See <ref>glStencilOpSeparate</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_STENCIL_FUNC</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating what function is used to compare the stencil reference
 *! value with the stencil buffer value. The initial value is <ref
 *! >GL_ALWAYS</ref>. See <ref>glStencilFunc</ref>. If the GL version is
 *! 2.0 or greater, this stencil state only affects non-polygons and
 *! front-facing polygons. Back-facing polygons use separate stencil
 *! state. See <ref>glStencilFuncSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_PASS_DEPTH_FAIL</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating what action is taken when the stencil test passes, but the
 *! depth test fails. The initial value is <ref>GL_KEEP</ref>. See <ref
 *! >glStencilOp</ref>. If the GL version is 2.0 or greater, this stencil
 *! state only affects non-polygons and front-facing polygons. Back-facing
 *! polygons use separate stencil state. See <ref>glStencilOpSeparate</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_STENCIL_PASS_DEPTH_PASS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, a symbolic constant
 *! indicating what action is taken when the stencil test passes and the
 *! depth test passes. The initial value is <ref>GL_KEEP</ref>. See <ref
 *! >glStencilOp</ref>. If the GL version is 2.0 or greater, this stencil
 *! state only affects non-polygons and front-facing polygons. Back-facing
 *! polygons use separate stencil state. See <ref>glStencilOpSeparate</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_STENCIL_REF</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the reference value that
 *! is compared with the contents of the stencil buffer. The initial value
 *! is 0. See <ref>glStencilFunc</ref>. If the GL version is 2.0 or
 *! greater, this stencil state only affects non-polygons and front-facing
 *! polygons. Back-facing polygons use separate stencil state. See <ref
 *! >glStencilFuncSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_TEST</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether stencil testing of fragments is enabled. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glStencilFunc</ref> and <ref
 *! >glStencilOp</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_VALUE_MASK</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the mask that is used to
 *! mask both the stencil reference value and the stencil buffer value
 *! before they are compared. The initial value is all 1's. See <ref
 *! >glStencilFunc</ref>. If the GL version is 2.0 or greater, this
 *! stencil state only affects non-polygons and front-facing polygons.
 *! Back-facing polygons use separate stencil state. See <ref
 *! >glStencilFuncSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_WRITEMASK</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the mask that controls
 *! writing of the stencil bitplanes. The initial value is all 1's. See
 *! <ref>glStencilMask</ref>. If the GL version is 2.0 or greater, this
 *! stencil state only affects non-polygons and front-facing polygons.
 *! Back-facing polygons use separate stencil state. See <ref
 *! >glStencilMaskSeparate</ref>. </p></c></r>
 *! <r><c><ref>GL_STEREO</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether stereo buffers (left and right) are supported. </p></c></r>
 *! <r><c><ref>GL_SUBPIXEL_BITS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, an estimate of the
 *! number of bits of subpixel resolution that are used to position
 *! rasterized geometry in window coordinates. The value must be at least
 *! 4. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_1D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 1D texture mapping is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glTexImage1D</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_BINDING_1D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! texture currently bound to the target <ref>GL_TEXTURE_1D</ref>. The
 *! initial value is 0. See <ref>glBindTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_2D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 2D texture mapping is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glTexImage2D</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_BINDING_2D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! texture currently bound to the target <ref>GL_TEXTURE_2D</ref>. The
 *! initial value is 0. See <ref>glBindTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_3D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether 3D texture mapping is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glTexImage3D</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_BINDING_3D</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! texture currently bound to the target <ref>GL_TEXTURE_3D</ref>. The
 *! initial value is 0. See <ref>glBindTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_BINDING_CUBE_MAP</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! texture currently bound to the target <ref>GL_TEXTURE_CUBE_MAP</ref>.
 *! The initial value is 0. See <ref>glBindTexture</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COMPRESSION_HINT</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value indicating the mode
 *! of the texture compression hint. The initial value is <ref
 *! >GL_DONT_CARE</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COORD_ARRAY</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the texture coordinate array is enabled. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glTexCoordPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object associated with the texture coordinate array. This
 *! buffer object would have been bound to the target <ref
 *! >GL_ARRAY_BUFFER</ref> at the time of the most recent call to <ref
 *! >glTexCoordPointer</ref>. If no buffer object was bound to this
 *! target, 0 is returned. The initial value is 0. See <ref
 *! >glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COORD_ARRAY_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of
 *! coordinates per element in the texture coordinate array. The initial
 *! value is 4. See <ref>glTexCoordPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COORD_ARRAY_STRIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte offset between
 *! consecutive elements in the texture coordinate array. The initial
 *! value is 0. See <ref>glTexCoordPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COORD_ARRAY_TYPE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the data type of the
 *! coordinates in the texture coordinate array. The initial value is <ref
 *! >GL_FLOAT</ref>. See <ref>glTexCoordPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_CUBE_MAP</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether cube-mapped texture mapping is enabled. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glTexImage2D</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_GEN_Q</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether automatic generation of the <i>q</i> texture coordinate is
 *! enabled. The initial value is <ref>GL_FALSE</ref>. See <ref
 *! >glTexGen</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_GEN_R</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether automatic generation of the <i>r</i> texture coordinate is
 *! enabled. The initial value is <ref>GL_FALSE</ref>. See <ref
 *! >glTexGen</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_GEN_S</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether automatic generation of the <i>S</i> texture coordinate is
 *! enabled. The initial value is <ref>GL_FALSE</ref>. See <ref
 *! >glTexGen</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_GEN_T</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether automatic generation of the T texture coordinate is enabled.
 *! The initial value is <ref>GL_FALSE</ref>. See <ref>glTexGen</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_TEXTURE_MATRIX</ref></c>
 *! <c><p></p><p><i>params</i> returns sixteen values: the texture matrix
 *! on the top of the texture matrix stack. Initially this matrix is the
 *! identity matrix. See <ref>glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_STACK_DEPTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of matrices
 *! on the texture matrix stack. The initial value is 1. See <ref
 *! >glPushMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_TRANSPOSE_COLOR_MATRIX</ref></c>
 *! <c><p></p><p><i>params</i> returns 16 values, the elements of the
 *! color matrix in row-major order. See <ref>glLoadTransposeMatrix</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_TRANSPOSE_MODELVIEW_MATRIX</ref></c>
 *! <c><p></p><p><i>params</i> returns 16 values, the elements of the
 *! modelview matrix in row-major order. See <ref
 *! >glLoadTransposeMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_TRANSPOSE_PROJECTION_MATRIX</ref></c>
 *! <c><p></p><p><i>params</i> returns 16 values, the elements of the
 *! projection matrix in row-major order. See <ref
 *! >glLoadTransposeMatrix</ref>. </p></c></r>
 *! <r><c><ref>GL_TRANSPOSE_TEXTURE_MATRIX</ref></c>
 *! <c><p></p><p><i>params</i> returns 16 values, the elements of the
 *! texture matrix in row-major order. See <ref>glLoadTransposeMatrix</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_UNPACK_ALIGNMENT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte alignment used
 *! for reading pixel data from memory. The initial value is 4. See <ref
 *! >glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_UNPACK_IMAGE_HEIGHT</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the image height used
 *! for reading pixel data from memory. The initial is 0. See <ref
 *! >glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_UNPACK_LSB_FIRST</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether single-bit pixels being read from memory are read first from
 *! the least significant bit of each unsigned byte. The initial value is
 *! <ref>GL_FALSE</ref>. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_UNPACK_ROW_LENGTH</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the row length used for
 *! reading pixel data from memory. The initial value is 0. See <ref
 *! >glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_UNPACK_SKIP_IMAGES</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of pixel
 *! images skipped before the first pixel is read from memory. The initial
 *! value is 0. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_UNPACK_SKIP_PIXELS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of pixel
 *! locations skipped before the first pixel is read from memory. The
 *! initial value is 0. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_UNPACK_SKIP_ROWS</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of rows of
 *! pixel locations skipped before the first pixel is read from memory.
 *! The initial value is 0. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_UNPACK_SWAP_BYTES</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the bytes of two-byte and four-byte pixel indices and
 *! components are swapped after being read from memory. The initial value
 *! is <ref>GL_FALSE</ref>. See <ref>glPixelStore</ref>. </p></c></r>
 *! <r><c><ref>GL_VERTEX_ARRAY</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether the vertex array is enabled. The initial value is <ref
 *! >GL_FALSE</ref>. See <ref>glVertexPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_VERTEX_ARRAY_BUFFER_BINDING</ref></c>
 *! <c><p></p><p><i>params</i> returns a single value, the name of the
 *! buffer object associated with the vertex array. This buffer object
 *! would have been bound to the target <ref>GL_ARRAY_BUFFER</ref> at the
 *! time of the most recent call to <ref>glVertexPointer</ref>. If no
 *! buffer object was bound to this target, 0 is returned. The initial
 *! value is 0. See <ref>glBindBuffer</ref>. </p></c></r>
 *! <r><c><ref>GL_VERTEX_ARRAY_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the number of
 *! coordinates per vertex in the vertex array. The initial value is 4.
 *! See <ref>glVertexPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_VERTEX_ARRAY_STRIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the byte offset between
 *! consecutive vertices in the vertex array. The initial value is 0. See
 *! <ref>glVertexPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_VERTEX_ARRAY_TYPE</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the data type of each
 *! coordinate in the vertex array. The initial value is <ref
 *! >GL_FLOAT</ref>. See <ref>glVertexPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_VERTEX_PROGRAM_POINT_SIZE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether vertex program point size mode is enabled. If enabled, and a
 *! vertex shader is active, then the point size is taken from the shader
 *! built-in <tt>gl_PointSize</tt>. If disabled, and a vertex shader is
 *! active, then the point size is taken from the point state as specified
 *! by <ref>glPointSize</ref>. The initial value is <ref>GL_FALSE</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_VERTEX_PROGRAM_TWO_SIDE</ref></c>
 *! <c><p></p><p><i>params</i> returns a single boolean value indicating
 *! whether vertex program two-sided color mode is enabled. If enabled,
 *! and a vertex shader is active, then the GL chooses the back color
 *! output for back-facing polygons, and the front color output for
 *! non-polygons and front-facing polygons. If disabled, and a vertex
 *! shader is active, then the front color output is always selected. The
 *! initial value is <ref>GL_FALSE</ref>. </p></c></r>
 *! <r><c><ref>GL_VIEWPORT</ref></c>
 *! <c><p></p><p><i>params</i> returns four values: the <i>x</i> and <i
 *! >y</i> window coordinates of the viewport, followed by its width and
 *! height. Initially the <i>x</i> and <i>y</i> window coordinates are
 *! both set to 0, and the width and height are set to the width and
 *! height of the window into which the GL will do its rendering. See <ref
 *! >glViewport</ref>. </p></c></r>
 *! <r><c><ref>GL_ZOOM_X</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the <i>x</i> pixel zoom
 *! factor. The initial value is 1. See <ref>glPixelZoom</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_ZOOM_Y</ref></c>
 *! <c><p></p><p><i>params</i> returns one value, the <i>y</i> pixel zoom
 *! factor. The initial value is 1. See <ref>glPixelZoom</ref>. </p
 *! ></c></r>
 *! </matrix>@}Many of the boolean parameters can also be queried more
 *! easily using @[glIsEnabled].
 *! 
 *! @param pname
 *! 
 *! Specifies the parameter value to be returned. The symbolic constants
 *! in the list below are accepted.
 *! 
 *! @param params
 *! 
 *! Returns the value or values of the specified parameter.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{pname@} is not an accepted
 *! value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glGet] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[GL_COLOR_LOGIC_OP], @[GL_COLOR_ARRAY], @[GL_COLOR_ARRAY_SIZE],
 *! @[GL_COLOR_ARRAY_STRIDE], @[GL_COLOR_ARRAY_TYPE],
 *! @[GL_EDGE_FLAG_ARRAY], @[GL_EDGE_FLAG_ARRAY_STRIDE],
 *! @[GL_INDEX_ARRAY], @[GL_INDEX_ARRAY_STRIDE], @[GL_INDEX_ARRAY_TYPE],
 *! @[GL_INDEX_LOGIC_OP], @[GL_NORMAL_ARRAY], @[GL_NORMAL_ARRAY_STRIDE],
 *! @[GL_NORMAL_ARRAY_TYPE], @[GL_POLYGON_OFFSET_UNITS],
 *! @[GL_POLYGON_OFFSET_FACTOR], @[GL_POLYGON_OFFSET_FILL],
 *! @[GL_POLYGON_OFFSET_LINE], @[GL_POLYGON_OFFSET_POINT],
 *! @[GL_TEXTURE_COORD_ARRAY], @[GL_TEXTURE_COORD_ARRAY_SIZE],
 *! @[GL_TEXTURE_COORD_ARRAY_STRIDE], @[GL_TEXTURE_COORD_ARRAY_TYPE],
 *! @[GL_VERTEX_ARRAY], @[GL_VERTEX_ARRAY_SIZE],
 *! @[GL_VERTEX_ARRAY_STRIDE], and @[GL_VERTEX_ARRAY_TYPE] are available
 *! only if the GL version is 1.1 or greater.
 *! 
 *! @[GL_ALIASED_POINT_SIZE_RANGE], @[GL_FEEDBACK_BUFFER_SIZE],
 *! @[GL_FEEDBACK_BUFFER_TYPE], @[GL_LIGHT_MODEL_AMBIENT],
 *! @[GL_LIGHT_MODEL_COLOR_CONTROL], @[GL_MAX_3D_TEXTURE_SIZE],
 *! @[GL_MAX_ELEMENTS_INDICES], @[GL_MAX_ELEMENTS_VERTICES],
 *! @[GL_PACK_IMAGE_HEIGHT], @[GL_PACK_SKIP_IMAGES], @[GL_RESCALE_NORMAL],
 *! @[GL_SELECTION_BUFFER_SIZE], @[GL_SMOOTH_LINE_WIDTH_GRANULARITY],
 *! @[GL_SMOOTH_LINE_WIDTH_RANGE], @[GL_SMOOTH_POINT_SIZE_GRANULARITY],
 *! @[GL_SMOOTH_POINT_SIZE_RANGE], @[GL_TEXTURE_3D],
 *! @[GL_TEXTURE_BINDING_3D], @[GL_UNPACK_IMAGE_HEIGHT], and
 *! @[GL_UNPACK_SKIP_IMAGES] are available only if the GL version is 1.2
 *! or greater.
 *! 
 *! @[GL_COMPRESSED_TEXTURE_FORMATS],
 *! @[GL_NUM_COMPRESSED_TEXTURE_FORMATS], @[GL_TEXTURE_BINDING_CUBE_MAP],
 *! and @[GL_TEXTURE_COMPRESSION_HINT] are available only if the GL
 *! version is 1.3 or greater.
 *! 
 *! @[GL_BLEND_DST_ALPHA], @[GL_BLEND_DST_RGB], @[GL_BLEND_SRC_ALPHA],
 *! @[GL_BLEND_SRC_RGB], @[GL_CURRENT_FOG_COORD],
 *! @[GL_CURRENT_SECONDARY_COLOR], @[GL_FOG_COORD_ARRAY_STRIDE],
 *! @[GL_FOG_COORD_ARRAY_TYPE], @[GL_FOG_COORD_SRC],
 *! @[GL_MAX_TEXTURE_LOD_BIAS], @[GL_POINT_SIZE_MIN],
 *! @[GL_POINT_SIZE_MAX], @[GL_POINT_FADE_THRESHOLD_SIZE],
 *! @[GL_POINT_DISTANCE_ATTENUATION], @[GL_SECONDARY_COLOR_ARRAY_SIZE],
 *! @[GL_SECONDARY_COLOR_ARRAY_STRIDE], and
 *! @[GL_SECONDARY_COLOR_ARRAY_TYPE] are available only if the GL version
 *! is 1.4 or greater.
 *! 
 *! @[GL_ARRAY_BUFFER_BINDING], @[GL_COLOR_ARRAY_BUFFER_BINDING],
 *! @[GL_EDGE_FLAG_ARRAY_BUFFER_BINDING],
 *! @[GL_ELEMENT_ARRAY_BUFFER_BINDING],
 *! @[GL_FOG_COORD_ARRAY_BUFFER_BINDING],
 *! @[GL_INDEX_ARRAY_BUFFER_BINDING], @[GL_NORMAL_ARRAY_BUFFER_BINDING],
 *! @[GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING],
 *! @[GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING], and
 *! @[GL_VERTEX_ARRAY_BUFFER_BINDING] are available only if the GL version
 *! is 1.5 or greater.
 *! 
 *! @[GL_BLEND_EQUATION_ALPHA], @[GL_BLEND_EQUATION_RGB],
 *! @[GL_DRAW_BUFFER]@i{i@}, @[GL_FRAGMENT_SHADER_DERIVATIVE_HINT],
 *! @[GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS], @[GL_MAX_DRAW_BUFFERS],
 *! @[GL_MAX_FRAGMENT_UNIFORM_COMPONENTS], @[GL_MAX_TEXTURE_COORDS],
 *! @[GL_MAX_TEXTURE_IMAGE_UNITS], @[GL_MAX_VARYING_FLOATS],
 *! @[GL_MAX_VERTEX_ATTRIBS], @[GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS],
 *! @[GL_MAX_VERTEX_UNIFORM_COMPONENTS], @[GL_POINT_SPRITE],
 *! @[GL_STENCIL_BACK_FAIL], @[GL_STENCIL_BACK_FUNC],
 *! @[GL_STENCIL_BACK_PASS_DEPTH_FAIL],
 *! @[GL_STENCIL_BACK_PASS_DEPTH_PASS], @[GL_STENCIL_BACK_REF],
 *! @[GL_STENCIL_BACK_VALUE_MASK], @[GL_STENCIL_BACK_WRITEMASK],
 *! @[GL_VERTEX_PROGRAM_POINT_SIZE], and @[GL_VERTEX_PROGRAM_TWO_SIDE] are
 *! available only if the GL version is 2.0 or greater.
 *! 
 *! @[GL_CURRENT_RASTER_SECONDARY_COLOR], @[GL_PIXEL_PACK_BUFFER_BINDING]
 *! and @[GL_PIXEL_UNPACK_BUFFER_BINDING] are available only if the GL
 *! version is 2.1 or greater.
 *! 
 *! @[GL_LINE_WIDTH_GRANULARITY] was deprecated in GL version 1.2. Its
 *! functionality was replaced by @[GL_SMOOTH_LINE_WIDTH_GRANULARITY].
 *! 
 *! @[GL_LINE_WIDTH_RANGE] was deprecated in GL version 1.2. Its
 *! functionality was replaced by @[GL_SMOOTH_LINE_WIDTH_RANGE].
 *! 
 *! @[GL_POINT_SIZE_GRANULARITY] was deprecated in GL version 1.2. Its
 *! functionality was replaced by @[GL_SMOOTH_POINT_SIZE_GRANULARITY].
 *! 
 *! @[GL_POINT_SIZE_RANGE] was deprecated in GL version 1.2. Its
 *! functionality was replaced by @[GL_SMOOTH_POINT_SIZE_RANGE].
 *! 
 *! @[GL_BLEND_EQUATION] was deprecated in GL version 2.0. Its
 *! functionality was replaced by @[GL_BLEND_EQUATION_RGB] and
 *! @[GL_BLEND_EQUATION_ALPHA].
 *! 
 *! @[GL_COLOR_MATRIX], @[GL_COLOR_MATRIX_STACK_DEPTH], @[GL_COLOR_TABLE],
 *! @[GL_CONVOLUTION_1D], @[GL_CONVOLUTION_2D], @[GL_HISTOGRAM],
 *! @[GL_MAX_COLOR_MATRIX_STACK_DEPTH], @[GL_MINMAX],
 *! @[GL_POST_COLOR_MATRIX_COLOR_TABLE], @[GL_POST_COLOR_MATRIX_RED_BIAS],
 *! @[GL_POST_COLOR_MATRIX_GREEN_BIAS], @[GL_POST_COLOR_MATRIX_BLUE_BIAS],
 *! @[GL_POST_COLOR_MATRIX_ALPHA_BIAS], @[GL_POST_COLOR_MATRIX_RED_SCALE],
 *! @[GL_POST_COLOR_MATRIX_GREEN_SCALE],
 *! @[GL_POST_COLOR_MATRIX_BLUE_SCALE],
 *! @[GL_POST_COLOR_MATRIX_ALPHA_SCALE],
 *! @[GL_POST_CONVOLUTION_COLOR_TABLE], @[GL_POST_CONVOLUTION_RED_BIAS],
 *! @[GL_POST_CONVOLUTION_GREEN_BIAS], @[GL_POST_CONVOLUTION_BLUE_BIAS],
 *! @[GL_POST_CONVOLUTION_ALPHA_BIAS], @[GL_POST_CONVOLUTION_RED_SCALE],
 *! @[GL_POST_CONVOLUTION_GREEN_SCALE], @[GL_POST_CONVOLUTION_BLUE_SCALE],
 *! @[GL_POST_CONVOLUTION_ALPHA_SCALE], and @[GL_SEPARABLE_2D] are
 *! available only if @tt{ARB_imaging@} is returned from @[glGet] when
 *! called with the argument @[GL_EXTENSIONS].
 *! 
 *! When the @tt{ARB_multitexture@} extension is supported, or the GL
 *! version is 1.3 or greater, the following parameters return the
 *! associated value for the active texture unit:
 *! @[GL_CURRENT_RASTER_TEXTURE_COORDS], @[GL_TEXTURE_1D],
 *! @[GL_TEXTURE_BINDING_1D], @[GL_TEXTURE_2D], @[GL_TEXTURE_BINDING_2D],
 *! @[GL_TEXTURE_3D], @[GL_TEXTURE_BINDING_3D], @[GL_TEXTURE_GEN_S],
 *! @[GL_TEXTURE_GEN_T], @[GL_TEXTURE_GEN_R], @[GL_TEXTURE_GEN_Q],
 *! @[GL_TEXTURE_MATRIX], and @[GL_TEXTURE_STACK_DEPTH]. Likewise, the
 *! following parameters return the associated value for the active client
 *! texture unit: @[GL_TEXTURE_COORD_ARRAY],
 *! @[GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING],
 *! @[GL_TEXTURE_COORD_ARRAY_SIZE], @[GL_TEXTURE_COORD_ARRAY_STRIDE],
 *! @[GL_TEXTURE_COORD_ARRAY_TYPE].
 *! 
 *! @seealso
 *! 
 *! @[glGetActiveAttrib], @[glGetActiveUniform], @[glGetAttachedShaders],
 *! @[glGetAttribLocation], @[glGetBufferParameteriv],
 *! @[glGetBufferPointerv], @[glGetBufferSubData], @[glGetClipPlane],
 *! @[glGetColorTable], @[glGetColorTableParameter],
 *! @[glGetCompressedTexImage], @[glGetConvolutionFilter],
 *! @[glGetConvolutionParameter], @[glGetError], @[glGetHistogram],
 *! @[glGetHistogramParameter], @[glGetLight], @[glGetMap],
 *! @[glGetMaterial], @[glGetMinmax], @[glGetMinmaxParameter],
 *! @[glGetPixelMap], @[glGetPointerv], @[glGetPolygonStipple],
 *! @[glGetProgram], @[glGetProgramInfoLog], @[glGetQueryiv],
 *! @[glGetQueryObject], @[glGetSeparableFilter], @[glGetShader],
 *! @[glGetShaderInfoLog], @[glGetShaderSource], @[glGetString],
 *! @[glGetTexEnv], @[glGetTexGen], @[glGetTexImage],
 *! @[glGetTexLevelParameter], @[glGetTexParameter], @[glGetUniform],
 *! @[glGetUniformLocation], @[glGetVertexAttrib],
 *! @[glGetVertexAttribPointerv], @[glIsEnabled]
 *! 
 */




/*! @decl void glFinish()
 *! 
 *! @[glFinish] does not return until the effects of all previously called
 *! GL commands are complete. Such effects include all changes to GL
 *! state, all changes to connection state, and all changes to the frame
 *! buffer contents.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glFinish] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glFinish] requires a round trip to the server.
 *! 
 *! @seealso
 *! 
 *! @[glFlush]
 *! 
 */

/*! @decl void glFlush()
 *! 
 *! Different GL implementations buffer commands in several different
 *! locations, including network buffers and the graphics accelerator
 *! itself. @[glFlush] empties all of these buffers, causing all issued
 *! commands to be executed as quickly as they are accepted by the actual
 *! rendering engine. Though this execution may not be completed in any
 *! particular time period, it does complete in finite time.
 *! 
 *! Because any GL program might be executed over a network, or on an
 *! accelerator that buffers commands, all programs should call @[glFlush]
 *! whenever they count on having all of their previously issued commands
 *! completed. For example, call @[glFlush] before waiting for user input
 *! that depends on the generated image.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glFlush] is executed between
 *! the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! @[glFlush] can return at any time. It does not wait until the
 *! execution of all previously issued GL commands is complete.
 *! 
 *! @seealso
 *! 
 *! @[glFinish]
 *! 
 */

/*! @decl void glInitNames()
 *! 
 *! The name stack is used during selection mode to allow sets of
 *! rendering commands to be uniquely identified. It consists of an
 *! ordered set of unsigned integers. @[glInitNames] causes the name stack
 *! to be initialized to its default empty state.
 *! 
 *! The name stack is always empty while the render mode is not
 *! @[GL_SELECT]. Calls to @[glInitNames] while the render mode is not
 *! @[GL_SELECT] are ignored.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glInitNames] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glLoadName], @[glPushName], @[glRenderMode], @[glSelectBuffer]
 *! 
 */

/*! @decl void glLoadIdentity()
 *! 
 *! @[glLoadIdentity] replaces the current matrix with the identity
 *! matrix. It is semantically equivalent to calling @[glLoadMatrix] with
 *! the identity matrix
 *! 
 *! (@xml{<matrix><r><c>1</c><c>0</c><c>0</c><c>0</c></r><r><c>0</c><c
 *! >1</c><c>0</c><c>0</c></r><r><c>0</c><c>0</c><c>1</c><c>0</c></r><r><c
 *! >0</c><c>0</c><c>0</c><c>1</c></r></matrix>@})
 *! 
 *! but in some cases it is more efficient.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLoadIdentity] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glLoadMatrix], @[glLoadTransposeMatrix], @[glMatrixMode],
 *! @[glMultMatrix], @[glMultTransposeMatrix], @[glPushMatrix]
 *! 
 */





/*! @decl void glPushMatrix()
 *! @decl void glPopMatrix()
 *! 
 *! There is a stack of matrices for each of the matrix modes. In
 *! @[GL_MODELVIEW] mode, the stack depth is at least 32. In the other
 *! modes, @[GL_COLOR], @[GL_PROJECTION], and @[GL_TEXTURE], the depth is
 *! at least 2. The current matrix in any mode is the matrix on the top of
 *! the stack for that mode.
 *! 
 *! @[glPushMatrix] pushes the current matrix stack down by one,
 *! duplicating the current matrix. That is, after a @[glPushMatrix] call,
 *! the matrix on top of the stack is identical to the one below it.
 *! 
 *! @[glPopMatrix] pops the current matrix stack, replacing the current
 *! matrix with the one below it on the stack.
 *! 
 *! Initially, each of the stacks contains one matrix, an identity matrix.
 *! 
 *! It is an error to push a full matrix stack or to pop a matrix stack
 *! that contains only a single matrix. In either case, the error flag is
 *! set and no other change is made to GL state.
 *! 
 *! @throws
 *! 
 *! @[GL_STACK_OVERFLOW] is generated if @[glPushMatrix] is called while
 *! the current matrix stack is full.
 *! 
 *! @[GL_STACK_UNDERFLOW] is generated if @[glPopMatrix] is called while
 *! the current matrix stack contains only a single matrix.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glPushMatrix] or
 *! @[glPopMatrix] is executed between the execution of @[glBegin] and the
 *! corresponding execution of @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glFrustum], @[glLoadIdentity], @[glLoadMatrix],
 *! @[glLoadTransposeMatrix], @[glMatrixMode], @[glMultMatrix],
 *! @[glMultTransposeMatrix], @[glOrtho], @[glRotate], @[glScale],
 *! @[glTranslate], @[glViewport]
 *! 
 */

/*! @decl void glBegin(int mode)
 *! @decl void glEnd()
 *! 
 *! @[glBegin] and @[glEnd] delimit the vertices that define a primitive
 *! or a group of like primitives. @[glBegin] accepts a single argument
 *! that specifies in which of ten ways the vertices are interpreted.
 *! Taking @i{n@} as an integer count starting at one, and @i{N@} as the
 *! total number of vertices specified, the interpretations are as
 *! follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_POINTS</ref></c>
 *! <c><p>Treats each vertex as a single point. Vertex <i>n</i> defines
 *! point <i>n</i>. <i>N</i> points are drawn. </p></c></r>
 *! <r><c><ref>GL_LINES</ref></c>
 *! <c><p>Treats each pair of vertices as an independent line segment.
 *! Vertices 2<i>n</i>-1 and 2<i>n</i> define line <i>n</i>. <i>N</i>/2
 *! lines are drawn. </p></c></r>
 *! <r><c><ref>GL_LINE_STRIP</ref></c>
 *! <c><p>Draws a connected group of line segments from the first vertex
 *! to the last. Vertices <i>n</i> and <i>n</i>+1 define line <i>n</i>. <i
 *! >N</i>-1 lines are drawn. </p></c></r>
 *! <r><c><ref>GL_LINE_LOOP</ref></c>
 *! <c><p>Draws a connected group of line segments from the first vertex
 *! to the last, then back to the first. Vertices <i>n</i> and <i>n</i>+1
 *! define line <i>n</i>. The last line, however, is defined by vertices
 *! <i>N</i> and 1. <i>N</i> lines are drawn. </p></c></r>
 *! <r><c><ref>GL_TRIANGLES</ref></c>
 *! <c><p>Treats each triplet of vertices as an independent triangle.
 *! Vertices 3<i>n</i>-2, 3<i>n</i>-1, and 3<i>n</i> define triangle <i
 *! >n</i>. <i>N</i>/3 triangles are drawn. </p></c></r>
 *! <r><c><ref>GL_TRIANGLE_STRIP</ref></c>
 *! <c><p>Draws a connected group of triangles. One triangle is defined
 *! for each vertex presented after the first two vertices. For odd <i
 *! >n</i>, vertices <i>n</i>, <i>n</i>+1, and <i>n</i>+2 define triangle
 *! <i>n</i>. For even <i>n</i>, vertices <i>n</i>+1, <i>n</i>, and <i
 *! >n</i>+2 define triangle <i>n</i>. <i>N</i>-2 triangles are drawn. </p
 *! ></c></r>
 *! <r><c><ref>GL_TRIANGLE_FAN</ref></c>
 *! <c><p>Draws a connected group of triangles. One triangle is defined
 *! for each vertex presented after the first two vertices. Vertices 1, <i
 *! >n</i>+1, and <i>n</i>+2 define triangle <i>n</i>. <i>N</i>-2
 *! triangles are drawn. </p></c></r>
 *! <r><c><ref>GL_QUADS</ref></c>
 *! <c><p>Treats each group of four vertices as an independent
 *! quadrilateral. Vertices 4<i>n</i>-3, 4<i>n</i>-2, 4<i>n</i>-1, and 4<i
 *! >n</i> define quadrilateral <i>n</i>. <i>N</i>/4 quadrilaterals are
 *! drawn. </p></c></r>
 *! <r><c><ref>GL_QUAD_STRIP</ref></c>
 *! <c><p>Draws a connected group of quadrilaterals. One quadrilateral is
 *! defined for each pair of vertices presented after the first pair.
 *! Vertices 2<i>n</i>-1, 2<i>n</i>, 2<i>n</i>+2, and 2<i>n</i>+1 define
 *! quadrilateral <i>n</i>. <i>N</i>/2-1 quadrilaterals are drawn. Note
 *! that the order in which vertices are used to construct a quadrilateral
 *! from strip data is different from that used with independent data. </p
 *! ></c></r>
 *! <r><c><ref>GL_POLYGON</ref></c>
 *! <c><p>Draws a single, convex polygon. Vertices 1 through <i>N</i>
 *! define this polygon. </p></c></r>
 *! </matrix>@}Only a subset of GL commands can be used between @[glBegin]
 *! and @[glEnd]. The commands are @[glVertex], @[glColor],
 *! @[glSecondaryColor], @[glIndex], @[glNormal], @[glFogCoord],
 *! @[glTexCoord], @[glMultiTexCoord], @[glVertexAttrib], @[glEvalCoord],
 *! @[glEvalPoint], @[glArrayElement], @[glMaterial], and @[glEdgeFlag].
 *! Also, it is acceptable to use @[glCallList] or @[glCallLists] to
 *! execute display lists that include only the preceding commands. If any
 *! other GL command is executed between @[glBegin] and @[glEnd], the
 *! error flag is set and the command is ignored.
 *! 
 *! Regardless of the value chosen for @i{mode@}, there is no limit to the
 *! number of vertices that can be defined between @[glBegin] and
 *! @[glEnd]. Lines, triangles, quadrilaterals, and polygons that are
 *! incompletely specified are not drawn. Incomplete specification results
 *! when either too few vertices are provided to specify even a single
 *! primitive or when an incorrect multiple of vertices is specified. The
 *! incomplete primitive is ignored; the rest are drawn.
 *! 
 *! The minimum specification of vertices for each primitive is as
 *! follows: 1 for a point, 2 for a line, 3 for a triangle, 4 for a
 *! quadrilateral, and 3 for a polygon. Modes that require a certain
 *! multiple of vertices are @[GL_LINES] (2), @[GL_TRIANGLES] (3),
 *! @[GL_QUADS] (4), and @[GL_QUAD_STRIP] (2).
 *! 
 *! @param mode
 *! 
 *! Specifies the primitive or primitives that will be created from
 *! vertices presented between @[glBegin] and the subsequent @[glEnd]. Ten
 *! symbolic constants are accepted: @[GL_POINTS], @[GL_LINES],
 *! @[GL_LINE_STRIP], @[GL_LINE_LOOP], @[GL_TRIANGLES],
 *! @[GL_TRIANGLE_STRIP], @[GL_TRIANGLE_FAN], @[GL_QUADS],
 *! @[GL_QUAD_STRIP], and @[GL_POLYGON].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is set to an unaccepted
 *! value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glBegin] is executed between
 *! a @[glBegin] and the corresponding execution of @[glEnd].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glEnd] is executed without
 *! being preceded by a @[glBegin].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if a command other than
 *! @[glVertex], @[glColor], @[glSecondaryColor], @[glIndex], @[glNormal],
 *! @[glFogCoord], @[glTexCoord], @[glMultiTexCoord], @[glVertexAttrib],
 *! @[glEvalCoord], @[glEvalPoint], @[glArrayElement], @[glMaterial],
 *! @[glEdgeFlag], @[glCallList], or @[glCallLists] is executed between
 *! the execution of @[glBegin] and the corresponding execution @[glEnd].
 *! 
 *! Execution of @[glEnableClientState], @[glDisableClientState],
 *! @[glEdgeFlagPointer], @[glFogCoordPointer], @[glTexCoordPointer],
 *! @[glColorPointer], @[glSecondaryColorPointer], @[glIndexPointer],
 *! @[glNormalPointer], @[glVertexPointer], @[glVertexAttribPointer],
 *! @[glInterleavedArrays], or @[glPixelStore] is not allowed after a call
 *! to @[glBegin] and before the corresponding call to @[glEnd], but an
 *! error may or may not be generated.
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glCallList], @[glCallLists], @[glColor],
 *! @[glEdgeFlag], @[glEvalCoord], @[glEvalPoint], @[glFogCoord],
 *! @[glIndex], @[glMaterial], @[glMultiTexCoord], @[glNormal],
 *! @[glSecondaryColor], @[glTexCoord], @[glVertex], @[glVertexAttrib]
 *! 
 */

/*! @decl void glCullFace(int mode)
 *! 
 *! @[glCullFace] specifies whether front- or back-facing facets are
 *! culled (as specified by @i{mode@}) when facet culling is enabled.
 *! Facet culling is initially disabled. To enable and disable facet
 *! culling, call the @[glEnable] and @[glDisable] commands with the
 *! argument @[GL_CULL_FACE]. Facets include triangles, quadrilaterals,
 *! polygons, and rectangles.
 *! 
 *! @[glFrontFace] specifies which of the clockwise and counterclockwise
 *! facets are front-facing and back-facing. See @[glFrontFace].
 *! 
 *! @param mode
 *! 
 *! Specifies whether front- or back-facing facets are candidates for
 *! culling. Symbolic constants @[GL_FRONT], @[GL_BACK], and
 *! @[GL_FRONT_AND_BACK] are accepted. The initial value is @[GL_BACK].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glCullFace] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! If @i{mode@} is @[GL_FRONT_AND_BACK], no facets are drawn, but other
 *! primitives such as points and lines are drawn.
 *! 
 *! @seealso
 *! 
 *! @[glEnable], @[glFrontFace]
 *! 
 */

/*! @decl void glDepthFunc(int func)
 *! 
 *! @[glDepthFunc] specifies the function used to compare each incoming
 *! pixel depth value with the depth value present in the depth buffer.
 *! The comparison is performed only if depth testing is enabled. (See
 *! @[glEnable] and @[glDisable] of @[GL_DEPTH_TEST].)
 *! 
 *! @i{func@} specifies the conditions under which the pixel will be
 *! drawn. The comparison functions are as follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_NEVER</ref></c>
 *! <c><p>Never passes. </p></c></r>
 *! <r><c><ref>GL_LESS</ref></c>
 *! <c><p>Passes if the incoming depth value is less than the stored depth
 *! value. </p></c></r>
 *! <r><c><ref>GL_EQUAL</ref></c>
 *! <c><p>Passes if the incoming depth value is equal to the stored depth
 *! value. </p></c></r>
 *! <r><c><ref>GL_LEQUAL</ref></c>
 *! <c><p>Passes if the incoming depth value is less than or equal to the
 *! stored depth value. </p></c></r>
 *! <r><c><ref>GL_GREATER</ref></c>
 *! <c><p>Passes if the incoming depth value is greater than the stored
 *! depth value. </p></c></r>
 *! <r><c><ref>GL_NOTEQUAL</ref></c>
 *! <c><p>Passes if the incoming depth value is not equal to the stored
 *! depth value. </p></c></r>
 *! <r><c><ref>GL_GEQUAL</ref></c>
 *! <c><p>Passes if the incoming depth value is greater than or equal to
 *! the stored depth value. </p></c></r>
 *! <r><c><ref>GL_ALWAYS</ref></c>
 *! <c><p>Always passes. </p></c></r>
 *! </matrix>@}The initial value of @i{func@} is @[GL_LESS]. Initially,
 *! depth testing is disabled. If depth testing is disabled or if no depth
 *! buffer exists, it is as if the depth test always passes.
 *! 
 *! @param func
 *! 
 *! Specifies the depth comparison function. Symbolic constants
 *! @[GL_NEVER], @[GL_LESS], @[GL_EQUAL], @[GL_LEQUAL], @[GL_GREATER],
 *! @[GL_NOTEQUAL], @[GL_GEQUAL], and @[GL_ALWAYS] are accepted. The
 *! initial value is @[GL_LESS].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{func@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDepthFunc] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Even if the depth buffer exists and the depth mask is non-zero, the
 *! depth buffer is not updated if the depth test is disabled.
 *! 
 *! @seealso
 *! 
 *! @[glDepthRange], @[glEnable], @[glPolygonOffset]
 *! 
 */



/*! @decl void glDrawBuffer(int mode)
 *! 
 *! When colors are written to the frame buffer, they are written into the
 *! color buffers specified by @[glDrawBuffer]. The specifications are as
 *! follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_NONE</ref></c>
 *! <c><p>No color buffers are written. </p></c></r>
 *! <r><c><ref>GL_FRONT_LEFT</ref></c>
 *! <c><p>Only the front left color buffer is written. </p></c></r>
 *! <r><c><ref>GL_FRONT_RIGHT</ref></c>
 *! <c><p>Only the front right color buffer is written. </p></c></r>
 *! <r><c><ref>GL_BACK_LEFT</ref></c>
 *! <c><p>Only the back left color buffer is written. </p></c></r>
 *! <r><c><ref>GL_BACK_RIGHT</ref></c>
 *! <c><p>Only the back right color buffer is written. </p></c></r>
 *! <r><c><ref>GL_FRONT</ref></c>
 *! <c><p>Only the front left and front right color buffers are written.
 *! If there is no front right color buffer, only the front left color
 *! buffer is written. </p></c></r>
 *! <r><c><ref>GL_BACK</ref></c>
 *! <c><p>Only the back left and back right color buffers are written. If
 *! there is no back right color buffer, only the back left color buffer
 *! is written. </p></c></r>
 *! <r><c><ref>GL_LEFT</ref></c>
 *! <c><p>Only the front left and back left color buffers are written. If
 *! there is no back left color buffer, only the front left color buffer
 *! is written. </p></c></r>
 *! <r><c><ref>GL_RIGHT</ref></c>
 *! <c><p>Only the front right and back right color buffers are written.
 *! If there is no back right color buffer, only the front right color
 *! buffer is written. </p></c></r>
 *! <r><c><ref>GL_FRONT_AND_BACK</ref></c>
 *! <c><p>All the front and back color buffers (front left, front right,
 *! back left, back right) are written. If there are no back color
 *! buffers, only the front left and front right color buffers are
 *! written. If there are no right color buffers, only the front left and
 *! back left color buffers are written. If there are no right or back
 *! color buffers, only the front left color buffer is written. </p
 *! ></c></r>
 *! <r><c><ref>GL_AUX0</ref> through <ref>GL_AUX3</ref><i>i</i></c>
 *! <c><p>Only auxiliary color buffer <i>i</i> is written. </p></c></r>
 *! </matrix>@}If more than one color buffer is selected for drawing, then
 *! blending or logical operations are computed and applied independently
 *! for each color buffer and can produce different results in each
 *! buffer.
 *! 
 *! Monoscopic contexts include only @i{left@} buffers, and stereoscopic
 *! contexts include both @i{left@} and @i{right@} buffers. Likewise,
 *! single-buffered contexts include only @i{front@} buffers, and
 *! double-buffered contexts include both @i{front@} and @i{back@}
 *! buffers. The context is selected at GL initialization.
 *! 
 *! @param mode
 *! 
 *! Specifies up to four color buffers to be drawn into. Symbolic
 *! constants @[GL_NONE], @[GL_FRONT_LEFT], @[GL_FRONT_RIGHT],
 *! @[GL_BACK_LEFT], @[GL_BACK_RIGHT], @[GL_FRONT], @[GL_BACK],
 *! @[GL_LEFT], @[GL_RIGHT], @[GL_FRONT_AND_BACK], and @[GL_AUX0] through
 *! @[GL_AUX3]@i{i@}, where @i{i@} is between 0 and the value of
 *! @[GL_AUX_BUFFERS] minus 1, are accepted. (@[GL_AUX_BUFFERS] is not the
 *! upper limit; use @[glGet] to query the number of available aux
 *! buffers.) The initial value is @[GL_FRONT] for single-buffered
 *! contexts, and @[GL_BACK] for double-buffered contexts.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if none of the buffers indicated
 *! by @i{mode@} exists.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glDrawBuffer] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! It is always the case that @[GL_AUX0] through @[GL_AUX3]@i{i@} =
 *! @[GL_AUX0] + @i{i@}.
 *! 
 *! @seealso
 *! 
 *! @[glBlendFunc], @[glColorMask], @[glIndexMask], @[glLogicOp],
 *! @[glReadBuffer]
 *! 
 */

/*! @decl void glEnable(int cap)
 *! @decl void glDisable(int cap)
 *! 
 *! @[glEnable] and @[glDisable] enable and disable various capabilities.
 *! Use @[glIsEnabled] or @[glGet] to determine the current setting of any
 *! capability. The initial value for each capability with the exception
 *! of @[GL_DITHER] and @[GL_MULTISAMPLE] is @[GL_FALSE]. The initial
 *! value for @[GL_DITHER] and @[GL_MULTISAMPLE] is @[GL_TRUE].
 *! 
 *! Both @[glEnable] and @[glDisable] take a single argument, @i{cap@},
 *! which can assume one of the following values:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_ALPHA_TEST</ref></c>
 *! <c><p></p><p>If enabled, do alpha testing. See <ref>glAlphaFunc</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_AUTO_NORMAL</ref></c>
 *! <c><p></p><p>If enabled, generate normal vectors when either <ref
 *! >GL_MAP2_VERTEX_3</ref> or <ref>GL_MAP2_VERTEX_4</ref> is used to
 *! generate vertices. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_BLEND</ref></c>
 *! <c><p></p><p>If enabled, blend the computed fragment color values with
 *! the values in the color buffers. See <ref>glBlendFunc</ref>. </p
 *! ></c></r>
 *! <r><c><tt>GL_CLIP_PLANE</tt><i>i</i></c>
 *! <c><p></p><p>If enabled, clip geometry against user-defined clipping
 *! plane <i>i</i>. See <ref>glClipPlane</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_LOGIC_OP</ref></c>
 *! <c><p></p><p>If enabled, apply the currently selected logical
 *! operation to the computed fragment color and color buffer values. See
 *! <ref>glLogicOp</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_MATERIAL</ref></c>
 *! <c><p></p><p>If enabled, have one or more material parameters track
 *! the current color. See <ref>glColorMaterial</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_SUM</ref></c>
 *! <c><p></p><p>If enabled and no fragment shader is active, add the
 *! secondary color value to the computed fragment color. See <ref
 *! >glSecondaryColor</ref>. </p></c></r>
 *! <r><c><ref>GL_COLOR_TABLE</ref></c>
 *! <c><p></p><p>If enabled, perform a color table lookup on the incoming
 *! RGBA color values. See <ref>glColorTable</ref>. </p></c></r>
 *! <r><c><ref>GL_CONVOLUTION_1D</ref></c>
 *! <c><p></p><p>If enabled, perform a 1D convolution operation on
 *! incoming RGBA color values. See <ref>glConvolutionFilter1D</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_CONVOLUTION_2D</ref></c>
 *! <c><p></p><p>If enabled, perform a 2D convolution operation on
 *! incoming RGBA color values. See <ref>glConvolutionFilter2D</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_CULL_FACE</ref></c>
 *! <c><p></p><p>If enabled, cull polygons based on their winding in
 *! window coordinates. See <ref>glCullFace</ref>. </p></c></r>
 *! <r><c><ref>GL_DEPTH_TEST</ref></c>
 *! <c><p></p><p>If enabled, do depth comparisons and update the depth
 *! buffer. Note that even if the depth buffer exists and the depth mask
 *! is non-zero, the depth buffer is not updated if the depth test is
 *! disabled. See <ref>glDepthFunc</ref> and <ref>glDepthRange</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_DITHER</ref></c>
 *! <c><p></p><p>If enabled, dither color components or indices before
 *! they are written to the color buffer. </p></c></r>
 *! <r><c><ref>GL_FOG</ref></c>
 *! <c><p></p><p>If enabled and no fragment shader is active, blend a fog
 *! color into the post-texturing color. See <ref>glFog</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_HISTOGRAM</ref></c>
 *! <c><p></p><p>If enabled, histogram incoming RGBA color values. See
 *! <ref>glHistogram</ref>. </p></c></r>
 *! <r><c><ref>GL_INDEX_LOGIC_OP</ref></c>
 *! <c><p></p><p>If enabled, apply the currently selected logical
 *! operation to the incoming index and color buffer indices. See <ref
 *! >glLogicOp</ref>. </p></c></r>
 *! <r><c><tt>GL_LIGHT</tt><i>i</i></c>
 *! <c><p></p><p>If enabled, include light <i>i</i> in the evaluation of
 *! the lighting equation. See <ref>glLightModel</ref> and <ref
 *! >glLight</ref>. </p></c></r>
 *! <r><c><ref>GL_LIGHTING</ref></c>
 *! <c><p></p><p>If enabled and no vertex shader is active, use the
 *! current lighting parameters to compute the vertex color or index.
 *! Otherwise, simply associate the current color or index with each
 *! vertex. See <ref>glMaterial</ref>, <ref>glLightModel</ref>, and <ref
 *! >glLight</ref>. </p></c></r>
 *! <r><c><ref>GL_LINE_SMOOTH</ref></c>
 *! <c><p></p><p>If enabled, draw lines with correct filtering. Otherwise,
 *! draw aliased lines. See <ref>glLineWidth</ref>. </p></c></r>
 *! <r><c><ref>GL_LINE_STIPPLE</ref></c>
 *! <c><p></p><p>If enabled, use the current line stipple pattern when
 *! drawing lines. See <ref>glLineStipple</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_COLOR_4</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate RGBA values.
 *! See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_INDEX</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate color indices.
 *! See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_NORMAL</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate normals. See
 *! <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_TEXTURE_COORD_1</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate <i>s</i>
 *! texture coordinates. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_TEXTURE_COORD_2</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate <i>s</i> and
 *! <i>t</i> texture coordinates. See <ref>glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_TEXTURE_COORD_3</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate <i>s</i>, <i
 *! >t</i>, and <i>r</i> texture coordinates. See <ref>glMap1</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_MAP1_TEXTURE_COORD_4</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate <i>s</i>, <i
 *! >t</i>, <i>r</i>, and <i>q</i> texture coordinates. See <ref
 *! >glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP1_VERTEX_3</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate <i>x</i>, <i
 *! >y</i>, and <i>z</i> vertex coordinates. See <ref>glMap1</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_MAP1_VERTEX_4</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh1</ref>, and <ref>glEvalPoint</ref> generate homogeneous <i
 *! >x</i>, <i>y</i>, <i>z</i>, and <i>w</i> vertex coordinates. See <ref
 *! >glMap1</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_COLOR_4</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate RGBA values.
 *! See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_INDEX</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate color indices.
 *! See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_NORMAL</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate normals. See
 *! <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_TEXTURE_COORD_1</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate <i>s</i>
 *! texture coordinates. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_TEXTURE_COORD_2</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate <i>s</i> and
 *! <i>t</i> texture coordinates. See <ref>glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_TEXTURE_COORD_3</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate <i>s</i>, <i
 *! >t</i>, and <i>r</i> texture coordinates. See <ref>glMap2</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_MAP2_TEXTURE_COORD_4</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate <i>s</i>, <i
 *! >t</i>, <i>r</i>, and <i>q</i> texture coordinates. See <ref
 *! >glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MAP2_VERTEX_3</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate <i>x</i>, <i
 *! >y</i>, and <i>z</i> vertex coordinates. See <ref>glMap2</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_MAP2_VERTEX_4</ref></c>
 *! <c><p></p><p>If enabled, calls to <ref>glEvalCoord</ref>, <ref
 *! >glEvalMesh2</ref>, and <ref>glEvalPoint</ref> generate homogeneous <i
 *! >x</i>, <i>y</i>, <i>z</i>, and <i>w</i> vertex coordinates. See <ref
 *! >glMap2</ref>. </p></c></r>
 *! <r><c><ref>GL_MINMAX</ref></c>
 *! <c><p></p><p>If enabled, compute the minimum and maximum values of
 *! incoming RGBA color values. See <ref>glMinmax</ref>. </p></c></r>
 *! <r><c><ref>GL_MULTISAMPLE</ref></c>
 *! <c><p></p><p>If enabled, use multiple fragment samples in computing
 *! the final color of a pixel. See <ref>glSampleCoverage</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_NORMALIZE</ref></c>
 *! <c><p></p><p>If enabled and no vertex shader is active, normal vectors
 *! are normalized to unit length after transformation and before
 *! lighting. This method is generally less efficient than <ref
 *! >GL_RESCALE_NORMAL</ref>. See <ref>glNormal</ref> and <ref
 *! >glNormalPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_SMOOTH</ref></c>
 *! <c><p></p><p>If enabled, draw points with proper filtering. Otherwise,
 *! draw aliased points. See <ref>glPointSize</ref>. </p></c></r>
 *! <r><c><ref>GL_POINT_SPRITE</ref></c>
 *! <c><p></p><p>If enabled, calculate texture coordinates for points
 *! based on texture environment and point parameter settings. Otherwise
 *! texture coordinates are constant across points. </p></c></r>
 *! <r><c><ref>GL_POLYGON_OFFSET_FILL</ref></c>
 *! <c><p></p><p>If enabled, and if the polygon is rendered in <ref
 *! >GL_FILL</ref> mode, an offset is added to depth values of a polygon's
 *! fragments before the depth comparison is performed. See <ref
 *! >glPolygonOffset</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_OFFSET_LINE</ref></c>
 *! <c><p></p><p>If enabled, and if the polygon is rendered in <ref
 *! >GL_LINE</ref> mode, an offset is added to depth values of a polygon's
 *! fragments before the depth comparison is performed. See <ref
 *! >glPolygonOffset</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_OFFSET_POINT</ref></c>
 *! <c><p></p><p>If enabled, an offset is added to depth values of a
 *! polygon's fragments before the depth comparison is performed, if the
 *! polygon is rendered in <ref>GL_POINT</ref> mode. See <ref
 *! >glPolygonOffset</ref>. </p></c></r>
 *! <r><c><ref>GL_POLYGON_SMOOTH</ref></c>
 *! <c><p></p><p>If enabled, draw polygons with proper filtering.
 *! Otherwise, draw aliased polygons. For correct antialiased polygons, an
 *! alpha buffer is needed and the polygons must be sorted front to back.
 *! </p></c></r>
 *! <r><c><ref>GL_POLYGON_STIPPLE</ref></c>
 *! <c><p></p><p>If enabled, use the current polygon stipple pattern when
 *! rendering polygons. See <ref>glPolygonStipple</ref>. </p></c></r>
 *! <r><c><ref>GL_POST_COLOR_MATRIX_COLOR_TABLE</ref></c>
 *! <c><p></p><p>If enabled, perform a color table lookup on RGBA color
 *! values after color matrix transformation. See <ref>glColorTable</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_POST_CONVOLUTION_COLOR_TABLE</ref></c>
 *! <c><p></p><p>If enabled, perform a color table lookup on RGBA color
 *! values after convolution. See <ref>glColorTable</ref>. </p></c></r>
 *! <r><c><ref>GL_RESCALE_NORMAL</ref></c>
 *! <c><p></p><p>If enabled and no vertex shader is active, normal vectors
 *! are scaled after transformation and before lighting by a factor
 *! computed from the modelview matrix. If the modelview matrix scales
 *! space uniformly, this has the effect of restoring the transformed
 *! normal to unit length. This method is generally more efficient than
 *! <ref>GL_NORMALIZE</ref>. See <ref>glNormal</ref> and <ref
 *! >glNormalPointer</ref>. </p></c></r>
 *! <r><c><ref>GL_SAMPLE_ALPHA_TO_COVERAGE</ref></c>
 *! <c><p></p><p>If enabled, compute a temporary coverage value where each
 *! bit is determined by the alpha value at the corresponding sample
 *! location. The temporary coverage value is then ANDed with the fragment
 *! coverage value. </p></c></r>
 *! <r><c><ref>GL_SAMPLE_ALPHA_TO_ONE</ref></c>
 *! <c><p></p><p>If enabled, each sample alpha value is replaced by the
 *! maximum representable alpha value. </p></c></r>
 *! <r><c><ref>GL_SAMPLE_COVERAGE</ref></c>
 *! <c><p></p><p>If enabled, the fragment's coverage is ANDed with the
 *! temporary coverage value. If <ref>GL_SAMPLE_COVERAGE_INVERT</ref> is
 *! set to <ref>GL_TRUE</ref>, invert the coverage value. See <ref
 *! >glSampleCoverage</ref>. </p></c></r>
 *! <r><c><ref>GL_SEPARABLE_2D</ref></c>
 *! <c><p></p><p>If enabled, perform a two-dimensional convolution
 *! operation using a separable convolution filter on incoming RGBA color
 *! values. See <ref>glSeparableFilter2D</ref>. </p></c></r>
 *! <r><c><ref>GL_SCISSOR_TEST</ref></c>
 *! <c><p></p><p>If enabled, discard fragments that are outside the
 *! scissor rectangle. See <ref>glScissor</ref>. </p></c></r>
 *! <r><c><ref>GL_STENCIL_TEST</ref></c>
 *! <c><p></p><p>If enabled, do stencil testing and update the stencil
 *! buffer. See <ref>glStencilFunc</ref> and <ref>glStencilOp</ref>. </p
 *! ></c></r>
 *! <r><c><ref>GL_TEXTURE_1D</ref></c>
 *! <c><p></p><p>If enabled and no fragment shader is active,
 *! one-dimensional texturing is performed (unless two- or
 *! three-dimensional or cube-mapped texturing is also enabled). See <ref
 *! >glTexImage1D</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_2D</ref></c>
 *! <c><p></p><p>If enabled and no fragment shader is active,
 *! two-dimensional texturing is performed (unless three-dimensional or
 *! cube-mapped texturing is also enabled). See <ref>glTexImage2D</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_TEXTURE_3D</ref></c>
 *! <c><p></p><p>If enabled and no fragment shader is active,
 *! three-dimensional texturing is performed (unless cube-mapped texturing
 *! is also enabled). See <ref>glTexImage3D</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_CUBE_MAP</ref></c>
 *! <c><p></p><p>If enabled and no fragment shader is active, cube-mapped
 *! texturing is performed. See <ref>glTexImage2D</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_GEN_Q</ref></c>
 *! <c><p></p><p>If enabled and no vertex shader is active, the <i>q</i>
 *! texture coordinate is computed using the texture generation function
 *! defined with <ref>glTexGen</ref>. Otherwise, the current <i>q</i>
 *! texture coordinate is used. See <ref>glTexGen</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_GEN_R</ref></c>
 *! <c><p></p><p>If enabled and no vertex shader is active, the <i>r</i>
 *! texture coordinate is computed using the texture generation function
 *! defined with <ref>glTexGen</ref>. Otherwise, the current <i>r</i>
 *! texture coordinate is used. See <ref>glTexGen</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_GEN_S</ref></c>
 *! <c><p></p><p>If enabled and no vertex shader is active, the <i>s</i>
 *! texture coordinate is computed using the texture generation function
 *! defined with <ref>glTexGen</ref>. Otherwise, the current <i>s</i>
 *! texture coordinate is used. See <ref>glTexGen</ref>. </p></c></r>
 *! <r><c><ref>GL_TEXTURE_GEN_T</ref></c>
 *! <c><p></p><p>If enabled and no vertex shader is active, the <i>t</i>
 *! texture coordinate is computed using the texture generation function
 *! defined with <ref>glTexGen</ref>. Otherwise, the current <i>t</i>
 *! texture coordinate is used. See <ref>glTexGen</ref>. </p></c></r>
 *! <r><c><ref>GL_VERTEX_PROGRAM_POINT_SIZE</ref></c>
 *! <c><p></p><p>If enabled and a vertex shader is active, then the
 *! derived point size is taken from the (potentially clipped) shader
 *! builtin <ref>gl_PointSize</ref> and clamped to the
 *! implementation-dependent point size range. </p></c></r>
 *! <r><c><ref>GL_VERTEX_PROGRAM_TWO_SIDE</ref></c>
 *! <c><p></p><p>If enabled and a vertex shader is active, it specifies
 *! that the GL will choose between front and back colors based on the
 *! polygon's face direction of which the vertex being shaded is a part.
 *! It has no effect on points or lines. </p></c></r>
 *! </matrix>@}
 *! 
 *! @param cap
 *! 
 *! Specifies a symbolic constant indicating a GL capability.
 *! 
 *! @param cap
 *! 
 *! Specifies a symbolic constant indicating a GL capability.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{cap@} is not one of the values
 *! listed previously.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glEnable] or @[glDisable] is
 *! executed between the execution of @[glBegin] and the corresponding
 *! execution of @[glEnd].
 *! 
 *! @note
 *! 
 *! @[GL_POLYGON_OFFSET_FILL], @[GL_POLYGON_OFFSET_LINE],
 *! @[GL_POLYGON_OFFSET_POINT], @[GL_COLOR_LOGIC_OP], and
 *! @[GL_INDEX_LOGIC_OP] are available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @[GL_RESCALE_NORMAL], and @[GL_TEXTURE_3D] are available only if the
 *! GL version is 1.2 or greater.
 *! 
 *! @[GL_MULTISAMPLE], @[GL_SAMPLE_ALPHA_TO_COVERAGE],
 *! @[GL_SAMPLE_ALPHA_TO_ONE], @[GL_SAMPLE_COVERAGE],
 *! @[GL_TEXTURE_CUBE_MAP] are available only if the GL version is 1.3 or
 *! greater.
 *! 
 *! @[GL_POINT_SPRITE], @[GL_VERTEX_PROGRAM_POINT_SIZE], and
 *! @[GL_VERTEX_PROGRAM_TWO_SIDE] is available only if the GL version is
 *! 2.0 or greater.
 *! 
 *! @[GL_COLOR_TABLE], @[GL_CONVOLUTION_1D], @[GL_CONVOLUTION_2D],
 *! @[GL_HISTOGRAM], @[GL_MINMAX], @[GL_POST_COLOR_MATRIX_COLOR_TABLE],
 *! @[GL_POST_CONVOLUTION_COLOR_TABLE], and @[GL_SEPARABLE_2D] are
 *! available only if @tt{ARB_imaging@} is returned from @[glGet] with an
 *! argument of @[GL_EXTENSIONS].
 *! 
 *! For OpenGL versions 1.3 and greater, or when @tt{ARB_multitexture@} is
 *! supported, @[GL_TEXTURE_1D], @[GL_TEXTURE_2D], @[GL_TEXTURE_3D],
 *! @[GL_TEXTURE_GEN_S], @[GL_TEXTURE_GEN_T], @[GL_TEXTURE_GEN_R], and
 *! @[GL_TEXTURE_GEN_Q] enable or disable the respective state for the
 *! active texture unit specified with @[glActiveTexture].
 *! 
 *! @seealso
 *! 
 *! @[glActiveTexture], @[glAlphaFunc], @[glBlendFunc], @[glClipPlane],
 *! @[glColorMaterial], @[glCullFace], @[glDepthFunc], @[glDepthRange],
 *! @[glEnableClientState], @[glFog], @[glGet], @[glIsEnabled],
 *! @[glLight], @[glLightModel], @[glLineWidth], @[glLineStipple],
 *! @[glLogicOp], @[glMap1], @[glMap2], @[glMaterial], @[glNormal],
 *! @[glNormalPointer], @[glPointSize], @[glPolygonMode],
 *! @[glPolygonOffset], @[glPolygonStipple], @[glSampleCoverage],
 *! @[glScissor], @[glStencilFunc], @[glStencilOp], @[glTexGen],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexImage3D]
 *! 
 */

/*! @decl void glEnableClientState(int cap)
 *! @decl void glDisableClientState(int cap)
 *! 
 *! @[glEnableClientState] and @[glDisableClientState] enable or disable
 *! individual client-side capabilities. By default, all client-side
 *! capabilities are disabled. Both @[glEnableClientState] and
 *! @[glDisableClientState] take a single argument, @i{cap@}, which can
 *! assume one of the following values:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_COLOR_ARRAY</ref></c>
 *! <c><p>If enabled, the color array is enabled for writing and used
 *! during rendering when <ref>glArrayElement</ref>, <ref
 *! >glDrawArrays</ref>, <ref>glDrawElements</ref>, <ref
 *! >glDrawRangeElements</ref><ref>glMultiDrawArrays</ref>, or <ref
 *! >glMultiDrawElements</ref> is called. See <ref>glColorPointer</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_EDGE_FLAG_ARRAY</ref></c>
 *! <c><p>If enabled, the edge flag array is enabled for writing and used
 *! during rendering when <ref>glArrayElement</ref>, <ref
 *! >glDrawArrays</ref>, <ref>glDrawElements</ref>, <ref
 *! >glDrawRangeElements</ref><ref>glMultiDrawArrays</ref>, or <ref
 *! >glMultiDrawElements</ref> is called. See <ref>glEdgeFlagPointer</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_FOG_COORD_ARRAY</ref></c>
 *! <c><p>If enabled, the fog coordinate array is enabled for writing and
 *! used during rendering when <ref>glArrayElement</ref>, <ref
 *! >glDrawArrays</ref>, <ref>glDrawElements</ref>, <ref
 *! >glDrawRangeElements</ref><ref>glMultiDrawArrays</ref>, or <ref
 *! >glMultiDrawElements</ref> is called. See <ref>glFogCoordPointer</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_INDEX_ARRAY</ref></c>
 *! <c><p>If enabled, the index array is enabled for writing and used
 *! during rendering when <ref>glArrayElement</ref>, <ref
 *! >glDrawArrays</ref>, <ref>glDrawElements</ref>, <ref
 *! >glDrawRangeElements</ref><ref>glMultiDrawArrays</ref>, or <ref
 *! >glMultiDrawElements</ref> is called. See <ref>glIndexPointer</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_NORMAL_ARRAY</ref></c>
 *! <c><p>If enabled, the normal array is enabled for writing and used
 *! during rendering when <ref>glArrayElement</ref>, <ref
 *! >glDrawArrays</ref>, <ref>glDrawElements</ref>, <ref
 *! >glDrawRangeElements</ref><ref>glMultiDrawArrays</ref>, or <ref
 *! >glMultiDrawElements</ref> is called. See <ref>glNormalPointer</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_SECONDARY_COLOR_ARRAY</ref></c>
 *! <c><p>If enabled, the secondary color array is enabled for writing and
 *! used during rendering when <ref>glArrayElement</ref>, <ref
 *! >glDrawArrays</ref>, <ref>glDrawElements</ref>, <ref
 *! >glDrawRangeElements</ref><ref>glMultiDrawArrays</ref>, or <ref
 *! >glMultiDrawElements</ref> is called. See <ref>glColorPointer</ref>.
 *! </p></c></r>
 *! <r><c><ref>GL_TEXTURE_COORD_ARRAY</ref></c>
 *! <c><p>If enabled, the texture coordinate array is enabled for writing
 *! and used during rendering when <ref>glArrayElement</ref>, <ref
 *! >glDrawArrays</ref>, <ref>glDrawElements</ref>, <ref
 *! >glDrawRangeElements</ref><ref>glMultiDrawArrays</ref>, or <ref
 *! >glMultiDrawElements</ref> is called. See <ref>glTexCoordPointer</ref
 *! >. </p></c></r>
 *! <r><c><ref>GL_VERTEX_ARRAY</ref></c>
 *! <c><p>If enabled, the vertex array is enabled for writing and used
 *! during rendering when <ref>glArrayElement</ref>, <ref
 *! >glDrawArrays</ref>, <ref>glDrawElements</ref>, <ref
 *! >glDrawRangeElements</ref><ref>glMultiDrawArrays</ref>, or <ref
 *! >glMultiDrawElements</ref> is called. See <ref>glVertexPointer</ref>.
 *! </p></c></r>
 *! </matrix>@}
 *! 
 *! @param cap
 *! 
 *! Specifies the capability to enable. Symbolic constants
 *! @[GL_COLOR_ARRAY], @[GL_EDGE_FLAG_ARRAY], @[GL_FOG_COORD_ARRAY],
 *! @[GL_INDEX_ARRAY], @[GL_NORMAL_ARRAY], @[GL_SECONDARY_COLOR_ARRAY],
 *! @[GL_TEXTURE_COORD_ARRAY], and @[GL_VERTEX_ARRAY] are accepted.
 *! 
 *! @param cap
 *! 
 *! Specifies the capability to disable.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{cap@} is not an accepted value.
 *! 
 *! @[glEnableClientState] is not allowed between the execution of
 *! @[glBegin] and the corresponding @[glEnd], but an error may or may not
 *! be generated. If no error is generated, the behavior is undefined.
 *! 
 *! @note
 *! 
 *! @[glEnableClientState] is available only if the GL version is 1.1 or
 *! greater.
 *! 
 *! @[GL_FOG_COORD_ARRAY] and @[GL_SECONDARY_COLOR_ARRAY] are available
 *! only if the GL version is 1.4 or greater.
 *! 
 *! For OpenGL versions 1.3 and greater, or when @tt{ARB_multitexture@} is
 *! supported, enabling and disabling @[GL_TEXTURE_COORD_ARRAY] affects
 *! the active client texture unit. The active client texture unit is
 *! controlled with @[glClientActiveTexture].
 *! 
 *! @seealso
 *! 
 *! @[glArrayElement], @[glClientActiveTexture], @[glColorPointer],
 *! @[glDrawArrays], @[glDrawElements], @[glEdgeFlagPointer],
 *! @[glFogCoordPointer], @[glEnable], @[glGetPointerv],
 *! @[glIndexPointer], @[glInterleavedArrays], @[glNormalPointer],
 *! @[glSecondaryColorPointer], @[glTexCoordPointer], @[glVertexPointer]
 *! 
 */

/*! @decl void glFrontFace(int mode)
 *! 
 *! In a scene composed entirely of opaque closed surfaces, back-facing
 *! polygons are never visible. Eliminating these invisible polygons has
 *! the obvious benefit of speeding up the rendering of the image. To
 *! enable and disable elimination of back-facing polygons, call
 *! @[glEnable] and @[glDisable] with argument @[GL_CULL_FACE].
 *! 
 *! The projection of a polygon to window coordinates is said to have
 *! clockwise winding if an imaginary object following the path from its
 *! first vertex, its second vertex, and so on, to its last vertex, and
 *! finally back to its first vertex, moves in a clockwise direction about
 *! the interior of the polygon. The polygon's winding is said to be
 *! counterclockwise if the imaginary object following the same path moves
 *! in a counterclockwise direction about the interior of the polygon.
 *! @[glFrontFace] specifies whether polygons with clockwise winding in
 *! window coordinates, or counterclockwise winding in window coordinates,
 *! are taken to be front-facing. Passing @[GL_CCW] to @i{mode@} selects
 *! counterclockwise polygons as front-facing; @[GL_CW] selects clockwise
 *! polygons as front-facing. By default, counterclockwise polygons are
 *! taken to be front-facing.
 *! 
 *! @param mode
 *! 
 *! Specifies the orientation of front-facing polygons. @[GL_CW] and
 *! @[GL_CCW] are accepted. The initial value is @[GL_CCW].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glFrontFace] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glCullFace], @[glLightModel]
 *! 
 */

/*! @decl void glLogicOp(int opcode)
 *! 
 *! @[glLogicOp] specifies a logical operation that, when enabled, is
 *! applied between the incoming color index or RGBA color and the color
 *! index or RGBA color at the corresponding location in the frame buffer.
 *! To enable or disable the logical operation, call @[glEnable] and
 *! @[glDisable] using the symbolic constant @[GL_COLOR_LOGIC_OP] for RGBA
 *! mode or @[GL_INDEX_LOGIC_OP] for color index mode. The initial value
 *! is disabled for both operations.
 *! 
 *! @xml{<matrix><r><c><b>Opcode </b></c><c><b>Resulting Operation </b></c
 *! ></r><r><c><ref>GL_CLEAR</ref></c><c> 0 </c></r><r><c><ref>GL_SET</ref
 *! ></c><c> 1 </c></r><r><c><ref>GL_COPY</ref></c><c> s </c></r><r><c
 *! ><ref>GL_COPY_INVERTED</ref></c><c> ~s </c></r><r><c><ref>GL_NOOP</ref
 *! ></c><c> d </c></r><r><c><ref>GL_INVERT</ref></c><c> ~d </c></r><r><c
 *! ><ref>GL_AND</ref></c><c> s &amp; d </c></r><r><c><ref>GL_NAND</ref
 *! ></c><c> ~(s &amp; d) </c></r><r><c><ref>GL_OR</ref></c><c> s | d </c
 *! ></r><r><c><ref>GL_NOR</ref></c><c> ~(s | d) </c></r><r><c><ref
 *! >GL_XOR</ref></c><c> s ^ d </c></r><r><c><ref>GL_EQUIV</ref></c><c>
 *! ~(s ^ d) </c></r><r><c><ref>GL_AND_REVERSE</ref></c><c> s &amp; ~d </c
 *! ></r><r><c><ref>GL_AND_INVERTED</ref></c><c> ~s &amp; d </c></r><r><c
 *! ><ref>GL_OR_REVERSE</ref></c><c> s | ~d </c></r><r><c><ref
 *! >GL_OR_INVERTED</ref></c><c> ~s | d </c></r></matrix>@}@i{opcode@} is
 *! a symbolic constant chosen from the list above. In the explanation of
 *! the logical operations, @i{s@} represents the incoming color index and
 *! @i{d@} represents the index in the frame buffer. Standard C-language
 *! operators are used. As these bitwise operators suggest, the logical
 *! operation is applied independently to each bit pair of the source and
 *! destination indices or colors.
 *! 
 *! @param opcode
 *! 
 *! Specifies a symbolic constant that selects a logical operation. The
 *! following symbols are accepted: @[GL_CLEAR], @[GL_SET], @[GL_COPY],
 *! @[GL_COPY_INVERTED], @[GL_NOOP], @[GL_INVERT], @[GL_AND], @[GL_NAND],
 *! @[GL_OR], @[GL_NOR], @[GL_XOR], @[GL_EQUIV], @[GL_AND_REVERSE],
 *! @[GL_AND_INVERTED], @[GL_OR_REVERSE], and @[GL_OR_INVERTED]. The
 *! initial value is @[GL_COPY].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{opcode@} is not an accepted
 *! value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glLogicOp] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! Color index logical operations are always supported. RGBA logical
 *! operations are supported only if the GL version is 1.1 or greater.
 *! 
 *! When more than one RGBA color or index buffer is enabled for drawing,
 *! logical operations are performed separately for each enabled buffer,
 *! using for the destination value the contents of that buffer (see
 *! @[glDrawBuffer]).
 *! 
 *! @seealso
 *! 
 *! @[glAlphaFunc], @[glBlendFunc], @[glDrawBuffer], @[glEnable],
 *! @[glStencilOp]
 *! 
 */

/*! @decl void glMatrixMode(int mode)
 *! 
 *! @[glMatrixMode] sets the current matrix mode. @i{mode@} can assume one
 *! of four values:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_MODELVIEW</ref></c>
 *! <c><p>Applies subsequent matrix operations to the modelview matrix
 *! stack. </p></c></r>
 *! <r><c><ref>GL_PROJECTION</ref></c>
 *! <c><p>Applies subsequent matrix operations to the projection matrix
 *! stack. </p></c></r>
 *! <r><c><ref>GL_TEXTURE</ref></c>
 *! <c><p>Applies subsequent matrix operations to the texture matrix
 *! stack. </p></c></r>
 *! <r><c><ref>GL_COLOR</ref></c>
 *! <c><p>Applies subsequent matrix operations to the color matrix stack.
 *! </p></c></r>
 *! </matrix>@}To find out which matrix stack is currently the target of
 *! all matrix operations, call @[glGet] with argument @[GL_MATRIX_MODE].
 *! The initial value is @[GL_MODELVIEW].
 *! 
 *! @param mode
 *! 
 *! Specifies which matrix stack is the target for subsequent matrix
 *! operations. Three values are accepted: @[GL_MODELVIEW],
 *! @[GL_PROJECTION], and @[GL_TEXTURE]. The initial value is
 *! @[GL_MODELVIEW]. Additionally, if the @tt{ARB_imaging@} extension is
 *! supported, @[GL_COLOR] is also accepted.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not an accepted value.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glMatrixMode] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glLoadMatrix], @[glLoadTransposeMatrix], @[glMultMatrix],
 *! @[glMultTransposeMatrix], @[glPopMatrix], @[glPushMatrix]
 *! 
 */

/*! @decl void glReadBuffer(int mode)
 *! 
 *! @[glReadBuffer] specifies a color buffer as the source for subsequent
 *! @[glReadPixels], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glCopyTexSubImage3D], and @[glCopyPixels] commands. @i{mode@}
 *! accepts one of twelve or more predefined values. (@[GL_AUX0] through
 *! @[GL_AUX3] are always defined.) In a fully configured system,
 *! @[GL_FRONT], @[GL_LEFT], and @[GL_FRONT_LEFT] all name the front left
 *! buffer, @[GL_FRONT_RIGHT] and @[GL_RIGHT] name the front right buffer,
 *! and @[GL_BACK_LEFT] and @[GL_BACK] name the back left buffer.
 *! 
 *! Nonstereo double-buffered configurations have only a front left and a
 *! back left buffer. Single-buffered configurations have a front left and
 *! a front right buffer if stereo, and only a front left buffer if
 *! nonstereo. It is an error to specify a nonexistent buffer to
 *! @[glReadBuffer].
 *! 
 *! @i{mode@} is initially @[GL_FRONT] in single-buffered configurations
 *! and @[GL_BACK] in double-buffered configurations.
 *! 
 *! @param mode
 *! 
 *! Specifies a color buffer. Accepted values are @[GL_FRONT_LEFT],
 *! @[GL_FRONT_RIGHT], @[GL_BACK_LEFT], @[GL_BACK_RIGHT], @[GL_FRONT],
 *! @[GL_BACK], @[GL_LEFT], @[GL_RIGHT], and @[GL_AUX0] through
 *! @[GL_AUX3]@i{i@}, where @i{i@} is between 0 and the value of
 *! @[GL_AUX_BUFFERS] minus 1.
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not one of the twelve
 *! (or more) accepted values.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @i{mode@} specifies a buffer
 *! that does not exist.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glReadBuffer] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glCopyPixels], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glCopyTexSubImage3D], @[glDrawBuffer], @[glReadPixels]
 *! 
 */

/*! @decl void glRenderMode(int mode)
 *! 
 *! @[glRenderMode] sets the rasterization mode. It takes one argument,
 *! @i{mode@}, which can assume one of three predefined values:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_RENDER</ref></c>
 *! <c><p>Render mode. Primitives are rasterized, producing pixel
 *! fragments, which are written into the frame buffer. This is the normal
 *! mode and also the default mode. </p></c></r>
 *! <r><c><ref>GL_SELECT</ref></c>
 *! <c><p>Selection mode. No pixel fragments are produced, and no change
 *! to the frame buffer contents is made. Instead, a record of the names
 *! of primitives that would have been drawn if the render mode had been
 *! <ref>GL_RENDER</ref> is returned in a select buffer, which must be
 *! created (see <ref>glSelectBuffer</ref>) before selection mode is
 *! entered. </p></c></r>
 *! <r><c><ref>GL_FEEDBACK</ref></c>
 *! <c><p>Feedback mode. No pixel fragments are produced, and no change to
 *! the frame buffer contents is made. Instead, the coordinates and
 *! attributes of vertices that would have been drawn if the render mode
 *! had been <ref>GL_RENDER</ref> is returned in a feedback buffer, which
 *! must be created (see <ref>glFeedbackBuffer</ref>) before feedback mode
 *! is entered. </p></c></r>
 *! </matrix>@}The return value of @[glRenderMode] is determined by the
 *! render mode at the time @[glRenderMode] is called, rather than by
 *! @i{mode@}. The values returned for the three render modes are as
 *! follows:
 *! 
 *! @xml{<matrix>
 *! <r><c><ref>GL_RENDER</ref></c>
 *! <c><p>0. </p></c></r>
 *! <r><c><ref>GL_SELECT</ref></c>
 *! <c><p>The number of hit records transferred to the select buffer. </p
 *! ></c></r>
 *! <r><c><ref>GL_FEEDBACK</ref></c>
 *! <c><p>The number of values (not vertices) transferred to the feedback
 *! buffer. </p></c></r>
 *! </matrix>@}See the @[glSelectBuffer] and @[glFeedbackBuffer] reference
 *! pages for more details concerning selection and feedback operation.
 *! 
 *! @param mode
 *! 
 *! Specifies the rasterization mode. Three values are accepted:
 *! @[GL_RENDER], @[GL_SELECT], and @[GL_FEEDBACK]. The initial value is
 *! @[GL_RENDER].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is not one of the three
 *! accepted values.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glSelectBuffer] is called
 *! while the render mode is @[GL_SELECT], or if @[glRenderMode] is called
 *! with argument @[GL_SELECT] before @[glSelectBuffer] is called at least
 *! once.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glFeedbackBuffer] is called
 *! while the render mode is @[GL_FEEDBACK], or if @[glRenderMode] is
 *! called with argument @[GL_FEEDBACK] before @[glFeedbackBuffer] is
 *! called at least once.
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glRenderMode] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @note
 *! 
 *! If an error is generated, @[glRenderMode] returns 0 regardless of the
 *! current render mode.
 *! 
 *! @seealso
 *! 
 *! @[glFeedbackBuffer], @[glInitNames], @[glLoadName], @[glPassThrough],
 *! @[glPushName], @[glSelectBuffer]
 *! 
 */

/*! @decl void glShadeModel(int mode)
 *! 
 *! GL primitives can have either flat or smooth shading. Smooth shading,
 *! the default, causes the computed colors of vertices to be interpolated
 *! as the primitive is rasterized, typically assigning different colors
 *! to each resulting pixel fragment. Flat shading selects the computed
 *! color of just one vertex and assigns it to all the pixel fragments
 *! generated by rasterizing a single primitive. In either case, the
 *! computed color of a vertex is the result of lighting if lighting is
 *! enabled, or it is the current color at the time the vertex was
 *! specified if lighting is disabled.
 *! 
 *! Flat and smooth shading are indistinguishable for points. Starting
 *! when @[glBegin] is issued and counting vertices and primitives from 1,
 *! the GL gives each flat-shaded line segment @i{i@} the computed color
 *! of vertex
 *! @i{i@}+1
 *! , its second vertex. Counting similarly from 1, the GL gives each
 *! flat-shaded polygon the computed color of the vertex listed in the
 *! following table. This is the last vertex to specify the polygon in all
 *! cases except single polygons, where the first vertex specifies the
 *! flat-shaded color.
 *! 
 *! @xml{<matrix><r><c><b>Primitive Type of Polygon <i>i</i></b></c><c><b>
 *! Vertex </b></c></r><r><c>Single polygon ( <i>i</i>==1) </c><c>1 </c
 *! ></r><r><c>Triangle strip </c><c><i>i</i>+2</c></r><r><c> Triangle fan
 *! </c><c><i>i</i>+2</c></r><r><c> Independent triangle </c><c>3<i>i</i
 *! ></c></r><r><c> Quad strip </c><c>2<i>i</i>+2</c></r><r><c>
 *! Independent quad </c><c>4<i>i</i></c></r></matrix>@} Flat and smooth
 *! shading are specified by @[glShadeModel] with @i{mode@} set to
 *! @[GL_FLAT] and @[GL_SMOOTH], respectively.
 *! 
 *! @param mode
 *! 
 *! Specifies a symbolic value representing a shading technique. Accepted
 *! values are @[GL_FLAT] and @[GL_SMOOTH]. The initial value is
 *! @[GL_SMOOTH].
 *! 
 *! @throws
 *! 
 *! @[GL_INVALID_ENUM] is generated if @i{mode@} is any value other than
 *! @[GL_FLAT] or @[GL_SMOOTH].
 *! 
 *! @[GL_INVALID_OPERATION] is generated if @[glShadeModel] is executed
 *! between the execution of @[glBegin] and the corresponding execution of
 *! @[glEnd].
 *! 
 *! @seealso
 *! 
 *! @[glBegin], @[glColor], @[glLight], @[glLightModel]
 *! 
 */

/*! @decl constant GL_2D = 1536
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_2_BYTES = 5127
 */
/*! @decl constant GL_3D = 1537
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_3D_COLOR = 1538
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_3D_COLOR_TEXTURE = 1539
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_3_BYTES = 5128
 */
/*! @decl constant GL_4D_COLOR_TEXTURE = 1540
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_4_BYTES = 5129
 */
/*! @decl constant GL_ABGR_EXT = 32768
 */
/*! @decl constant GL_ACCUM = 256
 *! Used in @[glAccum]
 */
/*! @decl constant GL_ACCUM_ALPHA_BITS = 3419
 *! Used in @[glAccum] and @[glGet]
 */
/*! @decl constant GL_ACCUM_BLUE_BITS = 3418
 *! Used in @[glAccum] and @[glGet]
 */
/*! @decl constant GL_ACCUM_BUFFER_BIT = 512
 *! Used in @[glClear] and @[glPushAttrib]
 */
/*! @decl constant GL_ACCUM_CLEAR_VALUE = 2944
 *! Used in @[glClear], @[glClearAccum] and @[glGet]
 */
/*! @decl constant GL_ACCUM_GREEN_BITS = 3417
 *! Used in @[glAccum] and @[glGet]
 */
/*! @decl constant GL_ACCUM_RED_BITS = 3416
 *! Used in @[glAccum] and @[glGet]
 */
/*! @decl constant GL_ACTIVE_TEXTURE = 34016
 *! Used in @[glGet]
 */
/*! @decl constant GL_ACTIVE_TEXTURE_ARB = 34016
 */
/*! @decl constant GL_ADD = 260
 *! Used in @[glAccum] and @[glTexEnv]
 */
/*! @decl constant GL_ADD_SIGNED = 34164
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_ALIASED_LINE_WIDTH_RANGE = 33902
 *! Used in @[glGet] and @[glLineWidth]
 */
/*! @decl constant GL_ALIASED_POINT_SIZE_RANGE = 33901
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_ALL_ATTRIB_BITS = 1048575
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_ALL_CLIENT_ATTRIB_BITS = -1
 */
/*! @decl constant GL_ALPHA = 6406
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glDrawPixels],
 *! @[glGetTexImage], @[glReadPixels], @[glTexEnv], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexParameter], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_ALPHA12 = 32829
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_ALPHA16 = 32830
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_ALPHA4 = 32827
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_ALPHA8 = 32828
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_ALPHA_BIAS = 3357
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_ALPHA_BITS = 3413
 *! Used in @[glGet]
 */
/*! @decl constant GL_ALPHA_BLEND_EQUATION_ATI = 34877
 */
/*! @decl constant GL_ALPHA_SCALE = 3356
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexEnv], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_ALPHA_TEST = 3008
 *! Used in @[glAlphaFunc], @[glEnable], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_ALPHA_TEST_FUNC = 3009
 *! Used in @[glAlphaFunc] and @[glGet]
 */
/*! @decl constant GL_ALPHA_TEST_REF = 3010
 *! Used in @[glAlphaFunc] and @[glGet]
 */
/*! @decl constant GL_ALWAYS = 519
 *! Used in @[glAlphaFunc], @[glDepthFunc], @[glGet], @[glStencilFunc] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_AMBIENT = 4608
 *! Used in @[glColorMaterial], @[glLight] and @[glMaterial]
 */
/*! @decl constant GL_AMBIENT_AND_DIFFUSE = 5634
 *! Used in @[glColorMaterial], @[glGet] and @[glMaterial]
 */
/*! @decl constant GL_AND = 5377
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_AND_INVERTED = 5380
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_AND_REVERSE = 5378
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_ARB_imaging = 1
 */
/*! @decl constant GL_ARB_multitexture = 1
 */
/*! @decl constant GL_ARB_texture_non_power_of_two = 1
 *! Used in @[glCopyTexImage1D], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_ARRAY_BUFFER = 34962
 *! Used in @[glColorPointer], @[glEdgeFlagPointer], @[glGet],
 *! @[glIndexPointer], @[glNormalPointer], @[glTexCoordPointer] and
 *! @[glVertexPointer]
 */
/*! @decl constant GL_ARRAY_BUFFER_BINDING = 34964
 *! Used in @[glColorPointer], @[glEdgeFlagPointer], @[glGet],
 *! @[glIndexPointer], @[glNormalPointer], @[glTexCoordPointer] and
 *! @[glVertexPointer]
 */
/*! @decl constant GL_ATI_blend_equation_separate = 1
 */
/*! @decl constant GL_ATTRIB_STACK_DEPTH = 2992
 *! Used in @[glGet], @[glPushAttrib] and @[glPushClientAttrib]
 */
/*! @decl constant GL_AUTO_NORMAL = 3456
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_AUX0 = 1033
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_AUX1 = 1034
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_AUX2 = 1035
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_AUX3 = 1036
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_AUX_BUFFERS = 3072
 *! Used in @[glDrawBuffer], @[glGet] and @[glReadBuffer]
 */
/*! @decl constant GL_AVERAGE_EXT = 33589
 */
/*! @decl constant GL_BACK = 1029
 *! Used in @[glColorMaterial], @[glCullFace], @[glDrawBuffer], @[glGet],
 *! @[glMaterial], @[glPolygonMode] and @[glReadBuffer]
 */
/*! @decl constant GL_BACK_LEFT = 1026
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_BACK_RIGHT = 1027
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_BGR = 32992
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_BGRA = 32993
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_BITMAP = 6656
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_BITMAP_TOKEN = 1796
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_BLEND = 3042
 *! Used in @[glBlendFunc], @[glEnable], @[glGet], @[glIsEnabled],
 *! @[glPushAttrib] and @[glTexEnv]
 */
/*! @decl constant GL_BLEND_COLOR = 32773
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLEND_COLOR_EXT = 32773
 */
/*! @decl constant GL_BLEND_DST = 3040
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_BLEND_DST_ALPHA = 32970
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLEND_DST_RGB = 32968
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLEND_EQUATION = 32777
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLEND_EQUATION_ALPHA = 34877
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLEND_EQUATION_EXT = 32777
 */
/*! @decl constant GL_BLEND_EQUATION_RGB = 32777
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLEND_SRC = 3041
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_BLEND_SRC_ALPHA = 32971
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLEND_SRC_RGB = 32969
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLUE = 6405
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_BLUE_BIAS = 3355
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_BLUE_BITS = 3412
 *! Used in @[glGet]
 */
/*! @decl constant GL_BLUE_SCALE = 3354
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_BYTE = 5120
 *! Used in @[glColorPointer], @[glDrawPixels], @[glGetTexImage],
 *! @[glNormalPointer], @[glReadPixels], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexSubImage1D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_C3F_V3F = 10788
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_C4F_N3F_V3F = 10790
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_C4UB_V2F = 10786
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_C4UB_V3F = 10787
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_CCW = 2305
 *! Used in @[glFrontFace] and @[glGet]
 */
/*! @decl constant GL_CLAMP = 10496
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_CLAMP_TO_BORDER = 33069
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_CLAMP_TO_EDGE = 33071
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_CLEAR = 5376
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_CLIENT_ACTIVE_TEXTURE = 34017
 *! Used in @[glGet]
 */
/*! @decl constant GL_CLIENT_ACTIVE_TEXTURE_ARB = 34017
 */
/*! @decl constant GL_CLIENT_ALL_ATTRIB_BITS = -1
 *! Used in @[glPushClientAttrib]
 */
/*! @decl constant GL_CLIENT_ATTRIB_STACK_DEPTH = 2993
 *! Used in @[glGet]
 */
/*! @decl constant GL_CLIENT_PIXEL_STORE_BIT = 1
 *! Used in @[glPushClientAttrib]
 */
/*! @decl constant GL_CLIENT_VERTEX_ARRAY_BIT = 2
 *! Used in @[glPushClientAttrib]
 */
/*! @decl constant GL_CLIP_PLANE0 = 12288
 *! Used in @[glClipPlane], @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_CLIP_PLANE1 = 12289
 *! Used in @[glClipPlane], @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_CLIP_PLANE2 = 12290
 *! Used in @[glClipPlane], @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_CLIP_PLANE3 = 12291
 *! Used in @[glClipPlane], @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_CLIP_PLANE4 = 12292
 *! Used in @[glClipPlane], @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_CLIP_PLANE5 = 12293
 *! Used in @[glClipPlane], @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_COEFF = 2560
 */
/*! @decl constant GL_COLOR = 6144
 *! Used in @[glCopyPixels], @[glMatrixMode] and @[glPushMatrix]
 */
/*! @decl constant GL_COLOR_ARRAY = 32886
 *! Used in @[glColorPointer], @[glDrawArrays], @[glDrawElements],
 *! @[glEnableClientState], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_COLOR_ARRAY_BUFFER_BINDING = 34968
 *! Used in @[glColorPointer] and @[glGet]
 */
/*! @decl constant GL_COLOR_ARRAY_POINTER = 32912
 *! Used in @[glColorPointer]
 */
/*! @decl constant GL_COLOR_ARRAY_SIZE = 32897
 *! Used in @[glColorPointer] and @[glGet]
 */
/*! @decl constant GL_COLOR_ARRAY_STRIDE = 32899
 *! Used in @[glColorPointer] and @[glGet]
 */
/*! @decl constant GL_COLOR_ARRAY_TYPE = 32898
 *! Used in @[glColorPointer] and @[glGet]
 */
/*! @decl constant GL_COLOR_BUFFER_BIT = 16384
 *! Used in @[glClear] and @[glPushAttrib]
 */
/*! @decl constant GL_COLOR_CLEAR_VALUE = 3106
 *! Used in @[glClear], @[glClearColor] and @[glGet]
 */
/*! @decl constant GL_COLOR_INDEX = 6400
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_COLOR_INDEXES = 5635
 *! Used in @[glLightModel] and @[glMaterial]
 */
/*! @decl constant GL_COLOR_LOGIC_OP = 3058
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLogicOp] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_COLOR_MATERIAL = 2903
 *! Used in @[glColorMaterial], @[glEnable], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_COLOR_MATERIAL_FACE = 2901
 *! Used in @[glColorMaterial], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_COLOR_MATERIAL_PARAMETER = 2902
 *! Used in @[glColorMaterial] and @[glGet]
 */
/*! @decl constant GL_COLOR_MATRIX = 32945
 *! Used in @[glFrustum], @[glGet], @[glLoadIdentity], @[glLoadMatrix],
 *! @[glMultMatrix], @[glOrtho], @[glPushMatrix], @[glRotate], @[glScale]
 *! and @[glTranslate]
 */
/*! @decl constant GL_COLOR_MATRIX_STACK_DEPTH = 32946
 *! Used in @[glGet] and @[glPushMatrix]
 */
/*! @decl constant GL_COLOR_SUM = 33880
 *! Used in @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_COLOR_TABLE = 32976
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_COLOR_TABLE_ALPHA_SIZE = 32989
 */
/*! @decl constant GL_COLOR_TABLE_ALPHA_SIZE_SGI = 32989
 */
/*! @decl constant GL_COLOR_TABLE_BIAS = 32983
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_COLOR_TABLE_BIAS_SGI = 32983
 */
/*! @decl constant GL_COLOR_TABLE_BLUE_SIZE = 32988
 */
/*! @decl constant GL_COLOR_TABLE_BLUE_SIZE_SGI = 32988
 */
/*! @decl constant GL_COLOR_TABLE_FORMAT = 32984
 */
/*! @decl constant GL_COLOR_TABLE_FORMAT_SGI = 32984
 */
/*! @decl constant GL_COLOR_TABLE_GREEN_SIZE = 32987
 */
/*! @decl constant GL_COLOR_TABLE_GREEN_SIZE_SGI = 32987
 */
/*! @decl constant GL_COLOR_TABLE_INTENSITY_SIZE = 32991
 */
/*! @decl constant GL_COLOR_TABLE_INTENSITY_SIZE_SGI = 32991
 */
/*! @decl constant GL_COLOR_TABLE_LUMINANCE_SIZE = 32990
 */
/*! @decl constant GL_COLOR_TABLE_LUMINANCE_SIZE_SGI = 32990
 */
/*! @decl constant GL_COLOR_TABLE_RED_SIZE = 32986
 */
/*! @decl constant GL_COLOR_TABLE_RED_SIZE_SGI = 32986
 */
/*! @decl constant GL_COLOR_TABLE_SCALE = 32982
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_COLOR_TABLE_SCALE_SGI = 32982
 */
/*! @decl constant GL_COLOR_TABLE_WIDTH = 32985
 */
/*! @decl constant GL_COLOR_TABLE_WIDTH_SGI = 32985
 */
/*! @decl constant GL_COLOR_WRITEMASK = 3107
 *! Used in @[glColorMask] and @[glGet]
 */
/*! @decl constant GL_COMBINE = 34160
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_COMBINE_ALPHA = 34162
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_COMBINE_RGB = 34161
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_COMPARE_R_TO_TEXTURE = 34894
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_COMPILE = 4864
 *! Used in @[glNewList]
 */
/*! @decl constant GL_COMPILE_AND_EXECUTE = 4865
 *! Used in @[glNewList]
 */
/*! @decl constant GL_COMPRESSED_ALPHA = 34025
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_COMPRESSED_GEOM_ACCELERATED_SUNX = 33232
 */
/*! @decl constant GL_COMPRESSED_GEOM_VERSION_SUNX = 33233
 */
/*! @decl constant GL_COMPRESSED_INTENSITY = 34028
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_COMPRESSED_LUMINANCE = 34026
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_COMPRESSED_LUMINANCE_ALPHA = 34027
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_COMPRESSED_RGB = 34029
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_COMPRESSED_RGBA = 34030
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_COMPRESSED_TEXTURE_FORMATS = 34467
 *! Used in @[glGet]
 */
/*! @decl constant GL_CONSTANT = 34166
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_CONSTANT_ALPHA = 32771
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_CONSTANT_ALPHA_EXT = 32771
 */
/*! @decl constant GL_CONSTANT_ATTENUATION = 4615
 *! Used in @[glLight]
 */
/*! @decl constant GL_CONSTANT_BORDER = 33105
 */
/*! @decl constant GL_CONSTANT_BORDER_HP = 33105
 */
/*! @decl constant GL_CONSTANT_COLOR = 32769
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_CONSTANT_COLOR_EXT = 32769
 */
/*! @decl constant GL_CONVOLUTION_1D = 32784
 *! Used in @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_CONVOLUTION_1D_EXT = 32784
 */
/*! @decl constant GL_CONVOLUTION_2D = 32785
 *! Used in @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_CONVOLUTION_2D_EXT = 32785
 */
/*! @decl constant GL_CONVOLUTION_BORDER_COLOR = 33108
 */
/*! @decl constant GL_CONVOLUTION_BORDER_COLOR_HP = 33108
 */
/*! @decl constant GL_CONVOLUTION_BORDER_MODE = 32787
 *! Used in @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_CONVOLUTION_BORDER_MODE_EXT = 32787
 */
/*! @decl constant GL_CONVOLUTION_FILTER_BIAS = 32789
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_CONVOLUTION_FILTER_BIAS_EXT = 32789
 */
/*! @decl constant GL_CONVOLUTION_FILTER_SCALE = 32788
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_CONVOLUTION_FILTER_SCALE_EXT = 32788
 */
/*! @decl constant GL_CONVOLUTION_FORMAT = 32791
 */
/*! @decl constant GL_CONVOLUTION_FORMAT_EXT = 32791
 */
/*! @decl constant GL_CONVOLUTION_HEIGHT = 32793
 */
/*! @decl constant GL_CONVOLUTION_HEIGHT_EXT = 32793
 */
/*! @decl constant GL_CONVOLUTION_WIDTH = 32792
 */
/*! @decl constant GL_CONVOLUTION_WIDTH_EXT = 32792
 */
/*! @decl constant GL_COORD_REPLACE = 34914
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_COPY = 5379
 *! Used in @[glGet] and @[glLogicOp]
 */
/*! @decl constant GL_COPY_INVERTED = 5388
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_COPY_PIXEL_TOKEN = 1798
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_CUBIC_EXT = 33588
 */
/*! @decl constant GL_CULL_FACE = 2884
 *! Used in @[glCullFace], @[glEnable], @[glFrontFace], @[glGet],
 *! @[glIsEnabled] and @[glPushAttrib]
 */
/*! @decl constant GL_CULL_FACE_MODE = 2885
 *! Used in @[glCullFace], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_CURRENT_BIT = 1
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_CURRENT_COLOR = 2816
 *! Used in @[glColor], @[glGet] and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_FOG_COORD = 33875
 *! Used in @[glGet]
 */
/*! @decl constant GL_CURRENT_INDEX = 2817
 *! Used in @[glGet], @[glIndex] and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_NORMAL = 2818
 *! Used in @[glGet] and @[glNormal]
 */
/*! @decl constant GL_CURRENT_PROGRAM = 35725
 *! Used in @[glGet]
 */
/*! @decl constant GL_CURRENT_RASTER_COLOR = 2820
 *! Used in @[glGet] and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_RASTER_DISTANCE = 2825
 *! Used in @[glGet] and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_RASTER_INDEX = 2821
 *! Used in @[glGet] and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_RASTER_POSITION = 2823
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_RASTER_POSITION_VALID = 2824
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib]
 *! and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_RASTER_SECONDARY_COLOR = 33887
 *! Used in @[glGet] and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_RASTER_TEXTURE_COORDS = 2822
 *! Used in @[glGet] and @[glRasterPos]
 */
/*! @decl constant GL_CURRENT_SECONDARY_COLOR = 33881
 *! Used in @[glGet]
 */
/*! @decl constant GL_CURRENT_TEXTURE_COORDS = 2819
 *! Used in @[glGet], @[glRasterPos] and @[glTexCoord]
 */
/*! @decl constant GL_CW = 2304
 *! Used in @[glFrontFace]
 */
/*! @decl constant GL_DECAL = 8449
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_DECR = 7683
 *! Used in @[glStencilOp]
 */
/*! @decl constant GL_DECR_WRAP = 34056
 *! Used in @[glStencilOp]
 */
/*! @decl constant GL_DEPTH = 6145
 *! Used in @[glCopyPixels]
 */
/*! @decl constant GL_DEPTH_BIAS = 3359
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_DEPTH_BITS = 3414
 *! Used in @[glGet]
 */
/*! @decl constant GL_DEPTH_BUFFER_BIT = 256
 *! Used in @[glClear] and @[glPushAttrib]
 */
/*! @decl constant GL_DEPTH_CLEAR_VALUE = 2931
 *! Used in @[glClear], @[glClearDepth] and @[glGet]
 */
/*! @decl constant GL_DEPTH_COMPONENT = 6402
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glDrawPixels],
 *! @[glGetTexImage], @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_DEPTH_COMPONENT16 = 33189
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_DEPTH_COMPONENT24 = 33190
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_DEPTH_COMPONENT32 = 33191
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_DEPTH_FUNC = 2932
 *! Used in @[glDepthFunc] and @[glGet]
 */
/*! @decl constant GL_DEPTH_RANGE = 2928
 *! Used in @[glDepthRange] and @[glGet]
 */
/*! @decl constant GL_DEPTH_SCALE = 3358
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_DEPTH_TEST = 2929
 *! Used in @[glDepthFunc], @[glEnable], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_DEPTH_TEXTURE_MODE = 34891
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_DEPTH_WRITEMASK = 2930
 *! Used in @[glDepthMask], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_DIFFUSE = 4609
 *! Used in @[glColorMaterial], @[glLight] and @[glMaterial]
 */
/*! @decl constant GL_DITHER = 3024
 *! Used in @[glEnable], @[glGet], @[glIsEnabled] and @[glPushAttrib]
 */
/*! @decl constant GL_DOMAIN = 2562
 */
/*! @decl constant GL_DONT_CARE = 4352
 *! Used in @[glGet] and @[glHint]
 */
/*! @decl constant GL_DOT3_RGB = 34478
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_DOT3_RGBA = 34479
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_DOUBLE = 5130
 *! Used in @[glColorPointer], @[glIndexPointer], @[glNormalPointer],
 *! @[glTexCoordPointer] and @[glVertexPointer]
 */
/*! @decl constant GL_DOUBLEBUFFER = 3122
 *! Used in @[glGet]
 */
/*! @decl constant GL_DRAW_BUFFER = 3073
 *! Used in @[glDrawBuffer], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_DRAW_BUFFER0 = 34853
 *! Used in @[glGet]
 */
/*! @decl constant GL_DRAW_PIXEL_TOKEN = 1797
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_DST_ALPHA = 772
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_DST_COLOR = 774
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_EDGE_FLAG = 2883
 *! Used in @[glEdgeFlag], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_EDGE_FLAG_ARRAY = 32889
 *! Used in @[glEdgeFlagPointer], @[glEnableClientState], @[glGet] and
 *! @[glIsEnabled]
 */
/*! @decl constant GL_EDGE_FLAG_ARRAY_BUFFER_BINDING = 34971
 *! Used in @[glEdgeFlagPointer] and @[glGet]
 */
/*! @decl constant GL_EDGE_FLAG_ARRAY_POINTER = 32915
 *! Used in @[glEdgeFlagPointer]
 */
/*! @decl constant GL_EDGE_FLAG_ARRAY_STRIDE = 32908
 *! Used in @[glEdgeFlagPointer] and @[glGet]
 */
/*! @decl constant GL_ELEMENT_ARRAY_BUFFER = 34963
 *! Used in @[glGet]
 */
/*! @decl constant GL_ELEMENT_ARRAY_BUFFER_BINDING = 34965
 *! Used in @[glGet]
 */
/*! @decl constant GL_EMISSION = 5632
 *! Used in @[glColorMaterial] and @[glMaterial]
 */
/*! @decl constant GL_ENABLE_BIT = 8192
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_EQUAL = 514
 *! Used in @[glAlphaFunc], @[glDepthFunc], @[glStencilFunc] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_EQUIV = 5385
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_EVAL_BIT = 65536
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_EXP = 2048
 *! Used in @[glFog] and @[glGet]
 */
/*! @decl constant GL_EXP2 = 2049
 *! Used in @[glFog]
 */
/*! @decl constant GL_EXTENSIONS = 7939
 *! Used in @[glEnable], @[glGet], @[glGetString] and @[glIsEnabled]
 */
/*! @decl constant GL_EXT_abgr = 1
 */
/*! @decl constant GL_EXT_blend_color = 1
 */
/*! @decl constant GL_EXT_blend_minmax = 1
 */
/*! @decl constant GL_EXT_blend_subtract = 1
 */
/*! @decl constant GL_EXT_convolution = 1
 */
/*! @decl constant GL_EXT_histogram = 1
 */
/*! @decl constant GL_EXT_pixel_transform = 1
 */
/*! @decl constant GL_EXT_rescale_normal = 1
 */
/*! @decl constant GL_EXT_texture3D = 1
 */
/*! @decl constant GL_EYE_LINEAR = 9216
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_EYE_PLANE = 9474
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_FALSE = 0
 *! Used in @[glColorMask], @[glDepthMask], @[glEdgeFlag], @[glEnable],
 *! @[glGet], @[glIsEnabled], @[glIsList], @[glIsTexture], @[glTexEnv] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_FASTEST = 4353
 *! Used in @[glHint]
 */
/*! @decl constant GL_FEEDBACK = 7169
 *! Used in @[glFeedbackBuffer], @[glPassThrough] and @[glRenderMode]
 */
/*! @decl constant GL_FEEDBACK_BUFFER_POINTER = 3568
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_FEEDBACK_BUFFER_SIZE = 3569
 *! Used in @[glFeedbackBuffer] and @[glGet]
 */
/*! @decl constant GL_FEEDBACK_BUFFER_TYPE = 3570
 *! Used in @[glFeedbackBuffer] and @[glGet]
 */
/*! @decl constant GL_FILL = 6914
 *! Used in @[glEnable], @[glEvalMesh2], @[glGet] and @[glPolygonMode]
 */
/*! @decl constant GL_FLAT = 7424
 *! Used in @[glShadeModel]
 */
/*! @decl constant GL_FLOAT = 5126
 *! Used in @[glColorPointer], @[glDrawPixels], @[glGet],
 *! @[glGetTexImage], @[glIndexPointer], @[glNormalPointer],
 *! @[glReadPixels], @[glTexCoordPointer], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexSubImage1D], @[glTexSubImage2D] and
 *! @[glVertexPointer]
 */
/*! @decl constant GL_FOG = 2912
 *! Used in @[glEnable], @[glFog], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_FOG_BIT = 128
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_FOG_COLOR = 2918
 *! Used in @[glFog] and @[glGet]
 */
/*! @decl constant GL_FOG_COORD = 33873
 *! Used in @[glFog]
 */
/*! @decl constant GL_FOG_COORD_ARRAY = 33879
 *! Used in @[glEnableClientState], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_FOG_COORD_ARRAY_BUFFER_BINDING = 34973
 *! Used in @[glGet]
 */
/*! @decl constant GL_FOG_COORD_ARRAY_STRIDE = 33877
 *! Used in @[glGet]
 */
/*! @decl constant GL_FOG_COORD_ARRAY_TYPE = 33876
 *! Used in @[glGet]
 */
/*! @decl constant GL_FOG_COORD_SRC = 33872
 *! Used in @[glFog] and @[glGet]
 */
/*! @decl constant GL_FOG_DENSITY = 2914
 *! Used in @[glFog] and @[glGet]
 */
/*! @decl constant GL_FOG_END = 2916
 *! Used in @[glFog] and @[glGet]
 */
/*! @decl constant GL_FOG_HINT = 3156
 *! Used in @[glGet], @[glHint] and @[glPushAttrib]
 */
/*! @decl constant GL_FOG_INDEX = 2913
 *! Used in @[glFog] and @[glGet]
 */
/*! @decl constant GL_FOG_MODE = 2917
 *! Used in @[glFog], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_FOG_START = 2915
 *! Used in @[glFog] and @[glGet]
 */
/*! @decl constant GL_FRAGMENT_DEPTH = 33874
 *! Used in @[glFog] and @[glGet]
 */
/*! @decl constant GL_FRAGMENT_SHADER_DERIVATIVE_HINT = 35723
 *! Used in @[glGet] and @[glHint]
 */
/*! @decl constant GL_FRONT = 1028
 *! Used in @[glColorMaterial], @[glCullFace], @[glDrawBuffer], @[glGet],
 *! @[glMaterial], @[glPolygonMode] and @[glReadBuffer]
 */
/*! @decl constant GL_FRONT_AND_BACK = 1032
 *! Used in @[glColorMaterial], @[glCullFace], @[glDrawBuffer], @[glGet],
 *! @[glMaterial], @[glPolygonMode], @[glStencilFunc], @[glStencilMask]
 *! and @[glStencilOp]
 */
/*! @decl constant GL_FRONT_FACE = 2886
 *! Used in @[glFrontFace], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_FRONT_LEFT = 1024
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_FRONT_RIGHT = 1025
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_FUNC_ADD = 32774
 *! Used in @[glGet]
 */
/*! @decl constant GL_FUNC_ADD_EXT = 32774
 */
/*! @decl constant GL_FUNC_REVERSE_SUBTRACT = 32779
 *! Used in @[glGet]
 */
/*! @decl constant GL_FUNC_REVERSE_SUBTRACT_EXT = 32779
 */
/*! @decl constant GL_FUNC_SUBTRACT = 32778
 *! Used in @[glGet]
 */
/*! @decl constant GL_FUNC_SUBTRACT_EXT = 32778
 */
/*! @decl constant GL_GENERATE_MIPMAP = 33169
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_GENERATE_MIPMAP_HINT = 33170
 *! Used in @[glGet], @[glHint] and @[glPushAttrib]
 */
/*! @decl constant GL_GEQUAL = 518
 *! Used in @[glAlphaFunc], @[glDepthFunc], @[glStencilFunc] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_GREATER = 516
 *! Used in @[glAlphaFunc], @[glDepthFunc], @[glStencilFunc] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_GREEN = 6404
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_GREEN_BIAS = 3353
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_GREEN_BITS = 3411
 *! Used in @[glGet]
 */
/*! @decl constant GL_GREEN_SCALE = 3352
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_HINT_BIT = 32768
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_HISTOGRAM = 32804
 *! Used in @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_HISTOGRAM_ALPHA_SIZE = 32811
 */
/*! @decl constant GL_HISTOGRAM_ALPHA_SIZE_EXT = 32811
 */
/*! @decl constant GL_HISTOGRAM_BLUE_SIZE = 32810
 */
/*! @decl constant GL_HISTOGRAM_BLUE_SIZE_EXT = 32810
 */
/*! @decl constant GL_HISTOGRAM_EXT = 32804
 */
/*! @decl constant GL_HISTOGRAM_FORMAT = 32807
 */
/*! @decl constant GL_HISTOGRAM_FORMAT_EXT = 32807
 */
/*! @decl constant GL_HISTOGRAM_GREEN_SIZE = 32809
 */
/*! @decl constant GL_HISTOGRAM_GREEN_SIZE_EXT = 32809
 */
/*! @decl constant GL_HISTOGRAM_LUMINANCE_SIZE = 32812
 */
/*! @decl constant GL_HISTOGRAM_LUMINANCE_SIZE_EXT = 32812
 */
/*! @decl constant GL_HISTOGRAM_RED_SIZE = 32808
 */
/*! @decl constant GL_HISTOGRAM_RED_SIZE_EXT = 32808
 */
/*! @decl constant GL_HISTOGRAM_SINK = 32813
 */
/*! @decl constant GL_HISTOGRAM_SINK_EXT = 32813
 */
/*! @decl constant GL_HISTOGRAM_WIDTH = 32806
 */
/*! @decl constant GL_HISTOGRAM_WIDTH_EXT = 32806
 */
/*! @decl constant GL_HP_convolution_border_modes = 1
 */
/*! @decl constant GL_HP_occlusion_test = 1
 */
/*! @decl constant GL_IGNORE_BORDER_HP = 33104
 */
/*! @decl constant GL_INCR = 7682
 *! Used in @[glStencilOp]
 */
/*! @decl constant GL_INCR_WRAP = 34055
 *! Used in @[glStencilOp]
 */
/*! @decl constant GL_INDEX_ARRAY = 32887
 *! Used in @[glEnableClientState], @[glGet], @[glIndexPointer] and
 *! @[glIsEnabled]
 */
/*! @decl constant GL_INDEX_ARRAY_BUFFER_BINDING = 34969
 *! Used in @[glGet] and @[glIndexPointer]
 */
/*! @decl constant GL_INDEX_ARRAY_POINTER = 32913
 *! Used in @[glIndexPointer]
 */
/*! @decl constant GL_INDEX_ARRAY_STRIDE = 32902
 *! Used in @[glGet] and @[glIndexPointer]
 */
/*! @decl constant GL_INDEX_ARRAY_TYPE = 32901
 *! Used in @[glGet] and @[glIndexPointer]
 */
/*! @decl constant GL_INDEX_BITS = 3409
 *! Used in @[glClearIndex] and @[glGet]
 */
/*! @decl constant GL_INDEX_CLEAR_VALUE = 3104
 *! Used in @[glClear], @[glClearIndex] and @[glGet]
 */
/*! @decl constant GL_INDEX_LOGIC_OP = 3057
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLogicOp] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_INDEX_MODE = 3120
 *! Used in @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_INDEX_OFFSET = 3347
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_INDEX_SHIFT = 3346
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_INDEX_WRITEMASK = 3105
 *! Used in @[glGet] and @[glIndexMask]
 */
/*! @decl constant GL_INT = 5124
 *! Used in @[glColorPointer], @[glDrawPixels], @[glGetTexImage],
 *! @[glIndexPointer], @[glNormalPointer], @[glReadPixels],
 *! @[glTexCoordPointer], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexSubImage1D], @[glTexSubImage2D] and @[glVertexPointer]
 */
/*! @decl constant GL_INTENSITY = 32841
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexEnv],
 *! @[glTexImage1D], @[glTexImage2D] and @[glTexParameter]
 */
/*! @decl constant GL_INTENSITY12 = 32844
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_INTENSITY16 = 32845
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_INTENSITY4 = 32842
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_INTENSITY8 = 32843
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_INTERPOLATE = 34165
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_INVALID_ENUM = 1280
 *! Used in @[glAccum], @[glAlphaFunc], @[glBegin], @[glBindTexture],
 *! @[glBlendFunc], @[glClipPlane], @[glColorMaterial], @[glColorPointer],
 *! @[glCopyPixels], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D], @[glCullFace],
 *! @[glDepthFunc], @[glDrawArrays], @[glDrawBuffer], @[glDrawElements],
 *! @[glDrawPixels], @[glEdgeFlagPointer], @[glEnable],
 *! @[glEnableClientState], @[glEvalMesh2], @[glFeedbackBuffer], @[glFog],
 *! @[glFrontFace], @[glGet], @[glGetError], @[glGetString],
 *! @[glGetTexImage], @[glHint], @[glIndexPointer],
 *! @[glInterleavedArrays], @[glIsEnabled], @[glLight], @[glLightModel],
 *! @[glLogicOp], @[glMaterial], @[glMatrixMode], @[glNewList],
 *! @[glNormalPointer], @[glPolygonMode], @[glReadBuffer],
 *! @[glReadPixels], @[glRenderMode], @[glShadeModel], @[glStencilFunc],
 *! @[glStencilOp], @[glTexCoordPointer], @[glTexEnv], @[glTexGen],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexParameter],
 *! @[glTexSubImage1D], @[glTexSubImage2D] and @[glVertexPointer]
 */
/*! @decl constant GL_INVALID_OPERATION = 1282
 *! Used in @[glAccum], @[glAlphaFunc], @[glArrayElement], @[glBegin],
 *! @[glBindTexture], @[glBlendFunc], @[glClear], @[glClearAccum],
 *! @[glClearColor], @[glClearDepth], @[glClearIndex], @[glClearStencil],
 *! @[glClipPlane], @[glColorMask], @[glColorMaterial], @[glCopyPixels],
 *! @[glCopyTexImage1D], @[glCopyTexImage2D], @[glCopyTexSubImage1D],
 *! @[glCopyTexSubImage2D], @[glCullFace], @[glDeleteLists],
 *! @[glDeleteTextures], @[glDepthFunc], @[glDepthMask], @[glDepthRange],
 *! @[glDrawArrays], @[glDrawBuffer], @[glDrawElements], @[glDrawPixels],
 *! @[glEnable], @[glEvalMesh2], @[glFeedbackBuffer], @[glFinish],
 *! @[glFlush], @[glFog], @[glFrontFace], @[glFrustum], @[glGenLists],
 *! @[glGenTextures], @[glGet], @[glGetError], @[glGetString],
 *! @[glGetTexImage], @[glHint], @[glIndexMask], @[glInitNames],
 *! @[glIsEnabled], @[glIsList], @[glIsTexture], @[glLight],
 *! @[glLightModel], @[glLineStipple], @[glLineWidth], @[glListBase],
 *! @[glLoadIdentity], @[glLoadMatrix], @[glLoadName], @[glLogicOp],
 *! @[glMapGrid], @[glMatrixMode], @[glMultMatrix], @[glNewList],
 *! @[glOrtho], @[glPassThrough], @[glPixelZoom], @[glPointSize],
 *! @[glPolygonMode], @[glPolygonOffset], @[glPushAttrib],
 *! @[glPushMatrix], @[glPushName], @[glRasterPos], @[glReadBuffer],
 *! @[glReadPixels], @[glRenderMode], @[glRotate], @[glScale],
 *! @[glScissor], @[glSelectBuffer], @[glShadeModel], @[glStencilFunc],
 *! @[glStencilMask], @[glStencilOp], @[glTexEnv], @[glTexGen],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexParameter],
 *! @[glTexSubImage1D], @[glTexSubImage2D], @[glTranslate] and
 *! @[glViewport]
 */
/*! @decl constant GL_INVALID_VALUE = 1281
 *! Used in @[glArrayElement], @[glClear], @[glColorPointer],
 *! @[glCopyPixels], @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D], @[glDeleteLists],
 *! @[glDrawArrays], @[glDrawPixels], @[glFeedbackBuffer], @[glFog],
 *! @[glFrustum], @[glGenLists], @[glGenTextures], @[glGetError],
 *! @[glGetTexImage], @[glIndexPointer], @[glInterleavedArrays],
 *! @[glLight], @[glLineWidth], @[glMapGrid], @[glMaterial], @[glNewList],
 *! @[glNormalPointer], @[glOrtho], @[glPointSize], @[glReadPixels],
 *! @[glScissor], @[glSelectBuffer], @[glTexCoordPointer], @[glTexEnv],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D],
 *! @[glTexSubImage2D], @[glVertexPointer] and @[glViewport]
 */
/*! @decl constant GL_INVERT = 5386
 *! Used in @[glLogicOp] and @[glStencilOp]
 */
/*! @decl constant GL_KEEP = 7680
 *! Used in @[glGet] and @[glStencilOp]
 */
/*! @decl constant GL_LARGE_SUNX = 33235
 */
/*! @decl constant GL_LEFT = 1030
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_LEQUAL = 515
 *! Used in @[glAlphaFunc], @[glDepthFunc], @[glStencilFunc] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_LESS = 513
 *! Used in @[glAlphaFunc], @[glDepthFunc], @[glGet], @[glStencilFunc] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_LIGHT0 = 16384
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT1 = 16385
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT2 = 16386
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT3 = 16387
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT4 = 16388
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT5 = 16389
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT6 = 16390
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT7 = 16391
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHTING = 2896
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight],
 *! @[glLightModel] and @[glPushAttrib]
 */
/*! @decl constant GL_LIGHTING_BIT = 64
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT_MODEL_AMBIENT = 2899
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight],
 *! @[glLightModel] and @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT_MODEL_COLOR_CONTROL = 33272
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight],
 *! @[glLightModel] and @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT_MODEL_LOCAL_VIEWER = 2897
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight],
 *! @[glLightModel] and @[glPushAttrib]
 */
/*! @decl constant GL_LIGHT_MODEL_TWO_SIDE = 2898
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLight],
 *! @[glLightModel] and @[glPushAttrib]
 */
/*! @decl constant GL_LINE = 6913
 *! Used in @[glEdgeFlag], @[glEnable], @[glEvalMesh2] and
 *! @[glPolygonMode]
 */
/*! @decl constant GL_LINEAR = 9729
 *! Used in @[glFog] and @[glTexParameter]
 */
/*! @decl constant GL_LINEAR_ATTENUATION = 4616
 *! Used in @[glLight]
 */
/*! @decl constant GL_LINEAR_MIPMAP_LINEAR = 9987
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_LINEAR_MIPMAP_NEAREST = 9985
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_LINES = 1
 *! Used in @[glBegin], @[glDrawArrays], @[glDrawElements], @[glEvalMesh2]
 *! and @[glLineStipple]
 */
/*! @decl constant GL_LINE_BIT = 4
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_LINE_LOOP = 2
 *! Used in @[glBegin], @[glDrawArrays] and @[glDrawElements]
 */
/*! @decl constant GL_LINE_RESET_TOKEN = 1799
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_LINE_SMOOTH = 2848
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLineWidth],
 *! @[glPolygonMode] and @[glPushAttrib]
 */
/*! @decl constant GL_LINE_SMOOTH_HINT = 3154
 *! Used in @[glGet], @[glHint] and @[glPushAttrib]
 */
/*! @decl constant GL_LINE_STIPPLE = 2852
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glLineStipple] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_LINE_STIPPLE_PATTERN = 2853
 *! Used in @[glGet] and @[glLineStipple]
 */
/*! @decl constant GL_LINE_STIPPLE_REPEAT = 2854
 *! Used in @[glGet] and @[glLineStipple]
 */
/*! @decl constant GL_LINE_STRIP = 3
 *! Used in @[glBegin], @[glDrawArrays] and @[glDrawElements]
 */
/*! @decl constant GL_LINE_TOKEN = 1794
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_LINE_WIDTH = 2849
 *! Used in @[glGet], @[glLineWidth] and @[glPolygonMode]
 */
/*! @decl constant GL_LINE_WIDTH_GRANULARITY = 2851
 *! Used in @[glGet] and @[glLineWidth]
 */
/*! @decl constant GL_LINE_WIDTH_RANGE = 2850
 *! Used in @[glGet] and @[glLineWidth]
 */
/*! @decl constant GL_LIST_BASE = 2866
 *! Used in @[glCallLists], @[glGet], @[glListBase] and @[glPushAttrib]
 */
/*! @decl constant GL_LIST_BIT = 131072
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_LIST_INDEX = 2867
 *! Used in @[glGet] and @[glNewList]
 */
/*! @decl constant GL_LIST_MODE = 2864
 *! Used in @[glGet] and @[glNewList]
 */
/*! @decl constant GL_LOAD = 257
 *! Used in @[glAccum]
 */
/*! @decl constant GL_LOGIC_OP = 3057
 */
/*! @decl constant GL_LOGIC_OP_MODE = 3056
 *! Used in @[glGet] and @[glLogicOp]
 */
/*! @decl constant GL_LUMINANCE = 6409
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glDrawPixels],
 *! @[glGetTexImage], @[glReadPixels], @[glTexEnv], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexParameter], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_LUMINANCE12 = 32833
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE12_ALPHA12 = 32839
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE12_ALPHA4 = 32838
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE16 = 32834
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE16_ALPHA16 = 32840
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE4 = 32831
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE4_ALPHA4 = 32835
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE6_ALPHA2 = 32836
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE8 = 32832
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE8_ALPHA8 = 32837
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_LUMINANCE_ALPHA = 6410
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glDrawPixels],
 *! @[glGetTexImage], @[glReadPixels], @[glTexEnv], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexSubImage1D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_MAP1_COLOR_4 = 3472
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_GRID_DOMAIN = 3536
 *! Used in @[glEvalMesh2], @[glEvalPoint], @[glGet], @[glMapGrid] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_GRID_SEGMENTS = 3537
 *! Used in @[glEvalMesh2], @[glEvalPoint], @[glGet], @[glMapGrid] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_INDEX = 3473
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_NORMAL = 3474
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_TEXTURE_COORD_1 = 3475
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_TEXTURE_COORD_2 = 3476
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_TEXTURE_COORD_3 = 3477
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_TEXTURE_COORD_4 = 3478
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_VERTEX_3 = 3479
 *! Used in @[glEnable], @[glEvalCoord], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_MAP1_VERTEX_4 = 3480
 *! Used in @[glEnable], @[glEvalCoord], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_COLOR_4 = 3504
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_GRID_DOMAIN = 3538
 *! Used in @[glEvalMesh2], @[glEvalPoint], @[glGet], @[glMapGrid] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_GRID_SEGMENTS = 3539
 *! Used in @[glEvalMesh2], @[glEvalPoint], @[glGet], @[glMapGrid] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_INDEX = 3505
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_NORMAL = 3506
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_TEXTURE_COORD_1 = 3507
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_TEXTURE_COORD_2 = 3508
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_TEXTURE_COORD_3 = 3509
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_TEXTURE_COORD_4 = 3510
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_VERTEX_3 = 3511
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP2_VERTEX_4 = 3512
 *! Used in @[glEnable], @[glEvalCoord], @[glGet], @[glIsEnabled] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_MAP_COLOR = 3344
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib]
 *! and @[glReadPixels]
 */
/*! @decl constant GL_MAP_STENCIL = 3345
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib]
 *! and @[glReadPixels]
 */
/*! @decl constant GL_MATRIX_MODE = 2976
 *! Used in @[glFrustum], @[glGet], @[glLoadIdentity], @[glLoadMatrix],
 *! @[glMatrixMode], @[glMultMatrix], @[glOrtho], @[glPushAttrib],
 *! @[glPushMatrix], @[glRotate], @[glScale] and @[glTranslate]
 */
/*! @decl constant GL_MAX = 32776
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_3D_TEXTURE_SIZE = 32883
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_3D_TEXTURE_SIZE_EXT = 32883
 */
/*! @decl constant GL_MAX_ATTRIB_STACK_DEPTH = 3381
 *! Used in @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_MAX_CLIENT_ATTRIB_STACK_DEPTH = 3387
 *! Used in @[glGet] and @[glPushClientAttrib]
 */
/*! @decl constant GL_MAX_CLIP_PLANES = 3378
 *! Used in @[glClipPlane] and @[glGet]
 */
/*! @decl constant GL_MAX_COLOR_MATRIX_STACK_DEPTH = 32947
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = 35661
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_CONVOLUTION_HEIGHT = 32795
 */
/*! @decl constant GL_MAX_CONVOLUTION_HEIGHT_EXT = 32795
 */
/*! @decl constant GL_MAX_CONVOLUTION_WIDTH = 32794
 */
/*! @decl constant GL_MAX_CONVOLUTION_WIDTH_EXT = 32794
 */
/*! @decl constant GL_MAX_CUBE_MAP_TEXTURE_SIZE = 34076
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_DRAW_BUFFERS = 34852
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_ELEMENTS_INDICES = 33001
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_ELEMENTS_VERTICES = 33000
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_EVAL_ORDER = 3376
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_EXT = 32776
 */
/*! @decl constant GL_MAX_FRAGMENT_UNIFORM_COMPONENTS = 35657
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_LIGHTS = 3377
 *! Used in @[glGet], @[glLight] and @[glPushAttrib]
 */
/*! @decl constant GL_MAX_LIST_NESTING = 2865
 *! Used in @[glCallList], @[glCallLists] and @[glGet]
 */
/*! @decl constant GL_MAX_MODELVIEW_STACK_DEPTH = 3382
 *! Used in @[glGet] and @[glPushMatrix]
 */
/*! @decl constant GL_MAX_NAME_STACK_DEPTH = 3383
 *! Used in @[glGet], @[glInitNames], @[glLoadName] and @[glPushName]
 */
/*! @decl constant GL_MAX_PIXEL_MAP_TABLE = 3380
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT = 33591
 */
/*! @decl constant GL_MAX_PROJECTION_STACK_DEPTH = 3384
 *! Used in @[glGet] and @[glPushMatrix]
 */
/*! @decl constant GL_MAX_TEXTURE_COORDS = 34929
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_TEXTURE_IMAGE_UNITS = 34930
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_TEXTURE_LOD_BIAS = 34045
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_MAX_TEXTURE_SIZE = 3379
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D], @[glGet],
 *! @[glGetTexImage], @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D]
 *! and @[glTexSubImage2D]
 */
/*! @decl constant GL_MAX_TEXTURE_STACK_DEPTH = 3385
 *! Used in @[glGet] and @[glPushMatrix]
 */
/*! @decl constant GL_MAX_TEXTURE_UNITS = 34018
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_TEXTURE_UNITS_ARB = 34018
 */
/*! @decl constant GL_MAX_VARYING_FLOATS = 35659
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_VERTEX_ATTRIBS = 34921
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS = 35660
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_VERTEX_UNIFORM_COMPONENTS = 35658
 *! Used in @[glGet]
 */
/*! @decl constant GL_MAX_VIEWPORT_DIMS = 3386
 *! Used in @[glGet] and @[glViewport]
 */
/*! @decl constant GL_MIN = 32775
 *! Used in @[glGet]
 */
/*! @decl constant GL_MINMAX = 32814
 *! Used in @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_MINMAX_EXT = 32814
 */
/*! @decl constant GL_MINMAX_FORMAT = 32815
 */
/*! @decl constant GL_MINMAX_FORMAT_EXT = 32815
 */
/*! @decl constant GL_MINMAX_SINK = 32816
 */
/*! @decl constant GL_MINMAX_SINK_EXT = 32816
 */
/*! @decl constant GL_MIN_EXT = 32775
 */
/*! @decl constant GL_MIRRORED_REPEAT = 33648
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_MODELVIEW = 5888
 *! Used in @[glGet], @[glMatrixMode], @[glPushMatrix], @[glRotate],
 *! @[glScale] and @[glTranslate]
 */
/*! @decl constant GL_MODELVIEW_MATRIX = 2982
 *! Used in @[glFrustum], @[glGet], @[glLoadIdentity], @[glLoadMatrix],
 *! @[glMultMatrix], @[glOrtho], @[glPushMatrix], @[glRotate], @[glScale]
 *! and @[glTranslate]
 */
/*! @decl constant GL_MODELVIEW_STACK_DEPTH = 2979
 *! Used in @[glGet] and @[glPushMatrix]
 */
/*! @decl constant GL_MODULATE = 8448
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_MULT = 259
 *! Used in @[glAccum]
 */
/*! @decl constant GL_MULTISAMPLE = 32925
 *! Used in @[glEnable], @[glIsEnabled] and @[glPushAttrib]
 */
/*! @decl constant GL_MULTISAMPLE_BIT = 536870912
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_N3F_V3F = 10789
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_NAME_STACK_DEPTH = 3440
 *! Used in @[glGet], @[glInitNames], @[glLoadName], @[glPushName] and
 *! @[glSelectBuffer]
 */
/*! @decl constant GL_NAND = 5390
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_NEAREST = 9728
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_NEAREST_MIPMAP_LINEAR = 9986
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_NEAREST_MIPMAP_NEAREST = 9984
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_NEVER = 512
 *! Used in @[glAlphaFunc], @[glDepthFunc], @[glStencilFunc] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_NICEST = 4354
 *! Used in @[glHint]
 */
/*! @decl constant GL_NONE = 0
 *! Used in @[glDrawBuffer], @[glGet] and @[glTexParameter]
 */
/*! @decl constant GL_NOOP = 5381
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_NOR = 5384
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_NORMALIZE = 2977
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glNormal],
 *! @[glPushAttrib] and @[glScale]
 */
/*! @decl constant GL_NORMAL_ARRAY = 32885
 *! Used in @[glEnableClientState], @[glGet], @[glIsEnabled] and
 *! @[glNormalPointer]
 */
/*! @decl constant GL_NORMAL_ARRAY_BUFFER_BINDING = 34967
 *! Used in @[glGet] and @[glNormalPointer]
 */
/*! @decl constant GL_NORMAL_ARRAY_POINTER = 32911
 *! Used in @[glNormalPointer]
 */
/*! @decl constant GL_NORMAL_ARRAY_STRIDE = 32895
 *! Used in @[glGet] and @[glNormalPointer]
 */
/*! @decl constant GL_NORMAL_ARRAY_TYPE = 32894
 *! Used in @[glGet] and @[glNormalPointer]
 */
/*! @decl constant GL_NORMAL_MAP = 34065
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_NOTEQUAL = 517
 *! Used in @[glAlphaFunc], @[glDepthFunc], @[glStencilFunc] and
 *! @[glTexParameter]
 */
/*! @decl constant GL_NO_ERROR = 0
 *! Used in @[glGetError]
 */
/*! @decl constant GL_NUM_COMPRESSED_TEXTURE_FORMATS = 34466
 *! Used in @[glGet]
 */
/*! @decl constant GL_OBJECT_LINEAR = 9217
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_OBJECT_PLANE = 9473
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_OCCLUSION_RESULT_HP = 33126
 */
/*! @decl constant GL_OCCLUSION_TEST_HP = 33125
 */
/*! @decl constant GL_ONE = 1
 *! Used in @[glBlendFunc] and @[glGet]
 */
/*! @decl constant GL_ONE_MINUS_CONSTANT_ALPHA = 32772
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_ONE_MINUS_CONSTANT_ALPHA_EXT = 32772
 */
/*! @decl constant GL_ONE_MINUS_CONSTANT_COLOR = 32770
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_ONE_MINUS_CONSTANT_COLOR_EXT = 32770
 */
/*! @decl constant GL_ONE_MINUS_DST_ALPHA = 773
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_ONE_MINUS_DST_COLOR = 775
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_ONE_MINUS_SRC_ALPHA = 771
 *! Used in @[glBlendFunc] and @[glTexEnv]
 */
/*! @decl constant GL_ONE_MINUS_SRC_COLOR = 769
 *! Used in @[glBlendFunc] and @[glTexEnv]
 */
/*! @decl constant GL_OPERAND0_ALPHA = 34200
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_OPERAND0_RGB = 34192
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_OPERAND1_ALPHA = 34201
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_OPERAND1_RGB = 34193
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_OPERAND2_ALPHA = 34202
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_OPERAND2_RGB = 34194
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_OR = 5383
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_ORDER = 2561
 */
/*! @decl constant GL_OR_INVERTED = 5389
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_OR_REVERSE = 5387
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_OUT_OF_MEMORY = 1285
 *! Used in @[glGetError] and @[glNewList]
 */
/*! @decl constant GL_PACK_ALIGNMENT = 3333
 *! Used in @[glGet] and @[glGetTexImage]
 */
/*! @decl constant GL_PACK_IMAGE_HEIGHT = 32876
 *! Used in @[glGet]
 */
/*! @decl constant GL_PACK_IMAGE_HEIGHT_EXT = 32876
 */
/*! @decl constant GL_PACK_LSB_FIRST = 3329
 *! Used in @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PACK_ROW_LENGTH = 3330
 *! Used in @[glGet]
 */
/*! @decl constant GL_PACK_SKIP_IMAGES = 32875
 *! Used in @[glGet]
 */
/*! @decl constant GL_PACK_SKIP_IMAGES_EXT = 32875
 */
/*! @decl constant GL_PACK_SKIP_PIXELS = 3332
 *! Used in @[glGet]
 */
/*! @decl constant GL_PACK_SKIP_ROWS = 3331
 *! Used in @[glGet]
 */
/*! @decl constant GL_PACK_SWAP_BYTES = 3328
 *! Used in @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PASS_THROUGH_TOKEN = 1792
 *! Used in @[glFeedbackBuffer] and @[glPassThrough]
 */
/*! @decl constant GL_PERSPECTIVE_CORRECTION_HINT = 3152
 *! Used in @[glGet], @[glHint] and @[glPushAttrib]
 */
/*! @decl constant GL_PIXEL_CUBIC_WEIGHT_EXT = 33587
 */
/*! @decl constant GL_PIXEL_MAG_FILTER_EXT = 33585
 */
/*! @decl constant GL_PIXEL_MAP_A_TO_A = 3193
 *! Used in @[glCopyPixels], @[glDrawPixels] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_A_TO_A_SIZE = 3257
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_B_TO_B = 3192
 *! Used in @[glCopyPixels], @[glDrawPixels] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_B_TO_B_SIZE = 3256
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_G_TO_G = 3191
 *! Used in @[glCopyPixels], @[glDrawPixels] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_G_TO_G_SIZE = 3255
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_A = 3189
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_A_SIZE = 3253
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_B = 3188
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_B_SIZE = 3252
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_G = 3187
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_G_SIZE = 3251
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_I = 3184
 *! Used in @[glCopyPixels], @[glDrawPixels] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_I_SIZE = 3248
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_R = 3186
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_PIXEL_MAP_I_TO_R_SIZE = 3250
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_R_TO_R = 3190
 *! Used in @[glCopyPixels], @[glDrawPixels] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_R_TO_R_SIZE = 3254
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_S_TO_S = 3185
 *! Used in @[glCopyPixels], @[glDrawPixels] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MAP_S_TO_S_SIZE = 3249
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_MIN_FILTER_EXT = 33586
 */
/*! @decl constant GL_PIXEL_MODE_BIT = 32
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_PIXEL_PACK_BUFFER = 35051
 *! Used in @[glGet], @[glGetTexImage] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_PACK_BUFFER_BINDING = 35053
 *! Used in @[glGet], @[glGetTexImage] and @[glReadPixels]
 */
/*! @decl constant GL_PIXEL_TRANSFORM_2D_EXT = 33584
 */
/*! @decl constant GL_PIXEL_TRANSFORM_2D_MATRIX_EXT = 33592
 */
/*! @decl constant GL_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT = 33590
 */
/*! @decl constant GL_PIXEL_TRANSFORM_COLOR_TABLE_EXT = 33593
 */
/*! @decl constant GL_PIXEL_UNPACK_BUFFER = 35052
 *! Used in @[glDrawPixels], @[glGet], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexSubImage1D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_PIXEL_UNPACK_BUFFER_BINDING = 35055
 *! Used in @[glDrawPixels], @[glGet], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexSubImage1D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_POINT = 6912
 *! Used in @[glEdgeFlag], @[glEnable], @[glEvalMesh2] and
 *! @[glPolygonMode]
 */
/*! @decl constant GL_POINTS = 0
 *! Used in @[glBegin], @[glDrawArrays], @[glDrawElements] and
 *! @[glEvalMesh2]
 */
/*! @decl constant GL_POINT_BIT = 2
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_POINT_DISTANCE_ATTENUATION = 33065
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_POINT_FADE_THRESHOLD_SIZE = 33064
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_POINT_SIZE = 2833
 *! Used in @[glGet], @[glPointSize] and @[glPolygonMode]
 */
/*! @decl constant GL_POINT_SIZE_GRANULARITY = 2835
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_POINT_SIZE_MAX = 33063
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_POINT_SIZE_MIN = 33062
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_POINT_SIZE_RANGE = 2834
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_POINT_SMOOTH = 2832
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPointSize],
 *! @[glPolygonMode] and @[glPushAttrib]
 */
/*! @decl constant GL_POINT_SMOOTH_HINT = 3153
 *! Used in @[glGet], @[glHint] and @[glPushAttrib]
 */
/*! @decl constant GL_POINT_SPRITE = 34913
 *! Used in @[glEnable], @[glGet], @[glIsEnabled] and @[glTexEnv]
 */
/*! @decl constant GL_POINT_TOKEN = 1793
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_POLYGON = 9
 *! Used in @[glBegin], @[glDrawArrays] and @[glDrawElements]
 */
/*! @decl constant GL_POLYGON_BIT = 8
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_MODE = 2880
 *! Used in @[glEdgeFlag], @[glGet], @[glPolygonMode] and @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_OFFSET_FACTOR = 32824
 *! Used in @[glGet], @[glPolygonOffset] and @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_OFFSET_FILL = 32823
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPolygonOffset] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_OFFSET_LINE = 10754
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPolygonOffset] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_OFFSET_POINT = 10753
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPolygonOffset] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_OFFSET_UNITS = 10752
 *! Used in @[glGet], @[glPolygonOffset] and @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_SMOOTH = 2881
 *! Used in @[glBlendFunc], @[glEnable], @[glGet], @[glIsEnabled],
 *! @[glPolygonMode] and @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_SMOOTH_HINT = 3155
 *! Used in @[glGet], @[glHint] and @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_STIPPLE = 2882
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPolygonMode] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_STIPPLE_BIT = 16
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_POLYGON_TOKEN = 1795
 *! Used in @[glFeedbackBuffer]
 */
/*! @decl constant GL_POSITION = 4611
 *! Used in @[glLight]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_ALPHA_BIAS = 32955
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_ALPHA_SCALE = 32951
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_BLUE_BIAS = 32954
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_BLUE_SCALE = 32950
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_COLOR_TABLE = 32978
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_GREEN_BIAS = 32953
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_GREEN_SCALE = 32949
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_RED_BIAS = 32952
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_COLOR_MATRIX_RED_SCALE = 32948
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_ALPHA_BIAS = 32803
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_ALPHA_BIAS_EXT = 32803
 */
/*! @decl constant GL_POST_CONVOLUTION_ALPHA_SCALE = 32799
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_ALPHA_SCALE_EXT = 32799
 */
/*! @decl constant GL_POST_CONVOLUTION_BLUE_BIAS = 32802
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_BLUE_BIAS_EXT = 32802
 */
/*! @decl constant GL_POST_CONVOLUTION_BLUE_SCALE = 32798
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_BLUE_SCALE_EXT = 32798
 */
/*! @decl constant GL_POST_CONVOLUTION_COLOR_TABLE = 32977
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_COLOR_TABLE_SGI = 32977
 */
/*! @decl constant GL_POST_CONVOLUTION_GREEN_BIAS = 32801
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_GREEN_BIAS_EXT = 32801
 */
/*! @decl constant GL_POST_CONVOLUTION_GREEN_SCALE = 32797
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_GREEN_SCALE_EXT = 32797
 */
/*! @decl constant GL_POST_CONVOLUTION_RED_BIAS = 32800
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_RED_BIAS_EXT = 32800
 */
/*! @decl constant GL_POST_CONVOLUTION_RED_SCALE = 32796
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_POST_CONVOLUTION_RED_SCALE_EXT = 32796
 */
/*! @decl constant GL_PREVIOUS = 34168
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_PRIMARY_COLOR = 34167
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_PROJECTION = 5889
 *! Used in @[glFrustum], @[glMatrixMode], @[glOrtho], @[glPushMatrix],
 *! @[glRotate], @[glScale] and @[glTranslate]
 */
/*! @decl constant GL_PROJECTION_MATRIX = 2983
 *! Used in @[glFrustum], @[glGet], @[glLoadIdentity], @[glLoadMatrix],
 *! @[glMultMatrix], @[glOrtho], @[glPushMatrix], @[glRotate], @[glScale]
 *! and @[glTranslate]
 */
/*! @decl constant GL_PROJECTION_STACK_DEPTH = 2980
 *! Used in @[glGet] and @[glPushMatrix]
 */
/*! @decl constant GL_PROXY_COLOR_TABLE = 32979
 *! Used in @[glNewList]
 */
/*! @decl constant GL_PROXY_COLOR_TABLE_SGI = 32979
 */
/*! @decl constant GL_PROXY_HISTOGRAM = 32805
 *! Used in @[glNewList]
 */
/*! @decl constant GL_PROXY_HISTOGRAM_EXT = 32805
 */
/*! @decl constant GL_PROXY_PIXEL_TRANSFORM_COLOR_TABLE_EXT = 33594
 */
/*! @decl constant GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE = 32981
 *! Used in @[glNewList]
 */
/*! @decl constant GL_PROXY_POST_CONVOLUTION_COLOR_TABLE = 32980
 *! Used in @[glNewList]
 */
/*! @decl constant GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI = 32980
 */
/*! @decl constant GL_PROXY_TEXTURE_1D = 32867
 *! Used in @[glGet], @[glNewList] and @[glTexImage1D]
 */
/*! @decl constant GL_PROXY_TEXTURE_2D = 32868
 *! Used in @[glGet] and @[glTexImage2D]
 */
/*! @decl constant GL_PROXY_TEXTURE_3D = 32880
 *! Used in @[glGet] and @[glNewList]
 */
/*! @decl constant GL_PROXY_TEXTURE_3D_EXT = 32880
 */
/*! @decl constant GL_PROXY_TEXTURE_COLOR_TABLE_SGI = 32957
 */
/*! @decl constant GL_PROXY_TEXTURE_CUBE_MAP = 34075
 *! Used in @[glCopyTexImage2D], @[glCopyTexSubImage2D], @[glGet] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_Q = 8195
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_QUADRATIC_ATTENUATION = 4617
 *! Used in @[glLight]
 */
/*! @decl constant GL_QUADS = 7
 *! Used in @[glBegin], @[glDrawArrays] and @[glDrawElements]
 */
/*! @decl constant GL_QUAD_STRIP = 8
 *! Used in @[glBegin], @[glDrawArrays] and @[glDrawElements]
 */
/*! @decl constant GL_R = 8194
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_R3_G3_B2 = 10768
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_READ_BUFFER = 3074
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D],
 *! @[glCopyTexSubImage1D], @[glCopyTexSubImage2D], @[glGet],
 *! @[glPushAttrib] and @[glReadBuffer]
 */
/*! @decl constant GL_RED = 6403
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_REDUCE = 32790
 *! Used in @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_REDUCE_EXT = 32790
 */
/*! @decl constant GL_RED_BIAS = 3349
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_RED_BITS = 3410
 *! Used in @[glGet]
 */
/*! @decl constant GL_RED_SCALE = 3348
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPushAttrib],
 *! @[glReadPixels], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_REFLECTION_MAP = 34066
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_RENDER = 7168
 *! Used in @[glGet] and @[glRenderMode]
 */
/*! @decl constant GL_RENDERER = 7937
 *! Used in @[glGetString]
 */
/*! @decl constant GL_RENDER_MODE = 3136
 *! Used in @[glFeedbackBuffer], @[glGet], @[glPassThrough] and
 *! @[glRenderMode]
 */
/*! @decl constant GL_REPEAT = 10497
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_REPLACE = 7681
 *! Used in @[glStencilOp] and @[glTexEnv]
 */
/*! @decl constant GL_REPLICATE_BORDER = 33107
 */
/*! @decl constant GL_REPLICATE_BORDER_HP = 33107
 */
/*! @decl constant GL_RESCALE_NORMAL = 32826
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glNormal] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_RESCALE_NORMAL_EXT = 32826
 */
/*! @decl constant GL_RETURN = 258
 *! Used in @[glAccum]
 */
/*! @decl constant GL_RGB = 6407
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glDrawPixels],
 *! @[glGetTexImage], @[glReadPixels], @[glTexEnv], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexSubImage1D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_RGB10 = 32850
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGB10_A2 = 32857
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGB12 = 32851
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGB16 = 32852
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGB4 = 32847
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGB5 = 32848
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGB5_A1 = 32855
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGB8 = 32849
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGBA = 6408
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glDrawPixels],
 *! @[glGetTexImage], @[glReadPixels], @[glTexEnv], @[glTexImage1D],
 *! @[glTexImage2D], @[glTexSubImage1D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_RGBA12 = 32858
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGBA16 = 32859
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGBA2 = 32853
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGBA4 = 32854
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGBA8 = 32856
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_RGBA_MODE = 3121
 *! Used in @[glColor], @[glColorMask] and @[glGet]
 */
/*! @decl constant GL_RGB_SCALE = 34163
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexEnv], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_RIGHT = 1031
 *! Used in @[glDrawBuffer] and @[glReadBuffer]
 */
/*! @decl constant GL_S = 8192
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_SAMPLES = 32937
 *! Used in @[glGet]
 */
/*! @decl constant GL_SAMPLE_ALPHA_TO_COVERAGE = 32926
 *! Used in @[glEnable], @[glIsEnabled] and @[glPushAttrib]
 */
/*! @decl constant GL_SAMPLE_ALPHA_TO_ONE = 32927
 *! Used in @[glEnable], @[glIsEnabled] and @[glPushAttrib]
 */
/*! @decl constant GL_SAMPLE_BUFFERS = 32936
 *! Used in @[glGet]
 */
/*! @decl constant GL_SAMPLE_COVERAGE = 32928
 *! Used in @[glEnable], @[glIsEnabled] and @[glPushAttrib]
 */
/*! @decl constant GL_SAMPLE_COVERAGE_INVERT = 32939
 *! Used in @[glEnable], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_SAMPLE_COVERAGE_VALUE = 32938
 *! Used in @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_SCISSOR_BIT = 524288
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_SCISSOR_BOX = 3088
 *! Used in @[glGet] and @[glScissor]
 */
/*! @decl constant GL_SCISSOR_TEST = 3089
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPushAttrib] and
 *! @[glScissor]
 */
/*! @decl constant GL_SECONDARY_COLOR_ARRAY = 33886
 *! Used in @[glEnableClientState], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING = 34972
 *! Used in @[glGet]
 */
/*! @decl constant GL_SECONDARY_COLOR_ARRAY_SIZE = 33882
 *! Used in @[glGet]
 */
/*! @decl constant GL_SECONDARY_COLOR_ARRAY_STRIDE = 33884
 *! Used in @[glGet]
 */
/*! @decl constant GL_SECONDARY_COLOR_ARRAY_TYPE = 33883
 *! Used in @[glGet]
 */
/*! @decl constant GL_SELECT = 7170
 *! Used in @[glInitNames], @[glLoadName], @[glPushName], @[glRenderMode]
 *! and @[glSelectBuffer]
 */
/*! @decl constant GL_SELECTION_BUFFER_POINTER = 3571
 *! Used in @[glSelectBuffer]
 */
/*! @decl constant GL_SELECTION_BUFFER_SIZE = 3572
 *! Used in @[glGet] and @[glSelectBuffer]
 */
/*! @decl constant GL_SEPARABLE_2D = 32786
 *! Used in @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_SEPARABLE_2D_EXT = 32786
 */
/*! @decl constant GL_SEPARATE_SPECULAR_COLOR = 33274
 *! Used in @[glLightModel]
 */
/*! @decl constant GL_SET = 5391
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_SGI_color_table = 1
 */
/*! @decl constant GL_SGI_texture_color_table = 1
 */
/*! @decl constant GL_SHADE_MODEL = 2900
 *! Used in @[glGet], @[glPushAttrib] and @[glShadeModel]
 */
/*! @decl constant GL_SHADING_LANGUAGE_VERSION = 35724
 *! Used in @[glGetString]
 */
/*! @decl constant GL_SHININESS = 5633
 *! Used in @[glMaterial]
 */
/*! @decl constant GL_SHORT = 5122
 *! Used in @[glColorPointer], @[glDrawPixels], @[glGetTexImage],
 *! @[glIndexPointer], @[glNormalPointer], @[glReadPixels],
 *! @[glTexCoordPointer], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexSubImage1D], @[glTexSubImage2D] and @[glVertexPointer]
 */
/*! @decl constant GL_SINGLE_COLOR = 33273
 *! Used in @[glGet] and @[glLightModel]
 */
/*! @decl constant GL_SLUMINANCE = 35910
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_SLUMINANCE8 = 35911
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_SLUMINANCE8_ALPHA8 = 35909
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_SLUMINANCE_ALPHA = 35908
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_SMOOTH = 7425
 *! Used in @[glGet] and @[glShadeModel]
 */
/*! @decl constant GL_SMOOTH_LINE_WIDTH_GRANULARITY = 2851
 *! Used in @[glGet] and @[glLineWidth]
 */
/*! @decl constant GL_SMOOTH_LINE_WIDTH_RANGE = 2850
 *! Used in @[glGet] and @[glLineWidth]
 */
/*! @decl constant GL_SMOOTH_POINT_SIZE_GRANULARITY = 2835
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_SMOOTH_POINT_SIZE_RANGE = 2834
 *! Used in @[glGet] and @[glPointSize]
 */
/*! @decl constant GL_SOURCE0_ALPHA = 34184
 */
/*! @decl constant GL_SOURCE0_RGB = 34176
 */
/*! @decl constant GL_SOURCE1_ALPHA = 34185
 */
/*! @decl constant GL_SOURCE1_RGB = 34177
 */
/*! @decl constant GL_SOURCE2_ALPHA = 34186
 */
/*! @decl constant GL_SOURCE2_RGB = 34178
 */
/*! @decl constant GL_SPECULAR = 4610
 *! Used in @[glColorMaterial], @[glLight] and @[glMaterial]
 */
/*! @decl constant GL_SPHERE_MAP = 9218
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_SPOT_CUTOFF = 4614
 *! Used in @[glLight]
 */
/*! @decl constant GL_SPOT_DIRECTION = 4612
 *! Used in @[glLight]
 */
/*! @decl constant GL_SPOT_EXPONENT = 4613
 *! Used in @[glLight]
 */
/*! @decl constant GL_SRC0_ALPHA = 34184
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_SRC0_RGB = 34176
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_SRC1_ALPHA = 34185
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_SRC1_RGB = 34177
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_SRC2_ALPHA = 34186
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_SRC2_RGB = 34178
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_SRC_ALPHA = 770
 *! Used in @[glBlendFunc] and @[glTexEnv]
 */
/*! @decl constant GL_SRC_ALPHA_SATURATE = 776
 *! Used in @[glBlendFunc]
 */
/*! @decl constant GL_SRC_COLOR = 768
 *! Used in @[glBlendFunc] and @[glTexEnv]
 */
/*! @decl constant GL_SRGB = 35904
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_SRGB8 = 35905
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_SRGB8_ALPHA8 = 35907
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_SRGB_ALPHA = 35906
 *! Used in @[glCopyTexImage1D], @[glCopyTexImage2D], @[glTexImage1D] and
 *! @[glTexImage2D]
 */
/*! @decl constant GL_STACK_OVERFLOW = 1283
 *! Used in @[glGetError], @[glPushAttrib], @[glPushClientAttrib],
 *! @[glPushMatrix] and @[glPushName]
 */
/*! @decl constant GL_STACK_UNDERFLOW = 1284
 *! Used in @[glGetError], @[glPushAttrib], @[glPushClientAttrib],
 *! @[glPushMatrix] and @[glPushName]
 */
/*! @decl constant GL_STENCIL = 6146
 *! Used in @[glCopyPixels]
 */
/*! @decl constant GL_STENCIL_BACK_FAIL = 34817
 *! Used in @[glGet] and @[glStencilOp]
 */
/*! @decl constant GL_STENCIL_BACK_FUNC = 34816
 *! Used in @[glGet] and @[glStencilFunc]
 */
/*! @decl constant GL_STENCIL_BACK_PASS_DEPTH_FAIL = 34818
 *! Used in @[glGet] and @[glStencilOp]
 */
/*! @decl constant GL_STENCIL_BACK_PASS_DEPTH_PASS = 34819
 *! Used in @[glGet] and @[glStencilOp]
 */
/*! @decl constant GL_STENCIL_BACK_REF = 36003
 *! Used in @[glGet] and @[glStencilFunc]
 */
/*! @decl constant GL_STENCIL_BACK_VALUE_MASK = 36004
 *! Used in @[glGet] and @[glStencilFunc]
 */
/*! @decl constant GL_STENCIL_BACK_WRITEMASK = 36005
 *! Used in @[glGet] and @[glStencilMask]
 */
/*! @decl constant GL_STENCIL_BITS = 3415
 *! Used in @[glClearStencil], @[glGet], @[glStencilFunc],
 *! @[glStencilMask] and @[glStencilOp]
 */
/*! @decl constant GL_STENCIL_BUFFER_BIT = 1024
 *! Used in @[glClear] and @[glPushAttrib]
 */
/*! @decl constant GL_STENCIL_CLEAR_VALUE = 2961
 *! Used in @[glClear], @[glClearStencil] and @[glGet]
 */
/*! @decl constant GL_STENCIL_FAIL = 2964
 *! Used in @[glGet] and @[glStencilOp]
 */
/*! @decl constant GL_STENCIL_FUNC = 2962
 *! Used in @[glGet] and @[glStencilFunc]
 */
/*! @decl constant GL_STENCIL_INDEX = 6401
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_STENCIL_PASS_DEPTH_FAIL = 2965
 *! Used in @[glGet] and @[glStencilOp]
 */
/*! @decl constant GL_STENCIL_PASS_DEPTH_PASS = 2966
 *! Used in @[glGet] and @[glStencilOp]
 */
/*! @decl constant GL_STENCIL_REF = 2967
 *! Used in @[glGet] and @[glStencilFunc]
 */
/*! @decl constant GL_STENCIL_TEST = 2960
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPushAttrib],
 *! @[glStencilFunc] and @[glStencilOp]
 */
/*! @decl constant GL_STENCIL_VALUE_MASK = 2963
 *! Used in @[glGet] and @[glStencilFunc]
 */
/*! @decl constant GL_STENCIL_WRITEMASK = 2968
 *! Used in @[glGet] and @[glStencilMask]
 */
/*! @decl constant GL_STEREO = 3123
 *! Used in @[glGet]
 */
/*! @decl constant GL_SUBPIXEL_BITS = 3408
 *! Used in @[glGet]
 */
/*! @decl constant GL_SUBTRACT = 34023
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_SUNX_geometry_compression = 1
 */
/*! @decl constant GL_SUNX_surface_hint = 1
 */
/*! @decl constant GL_SUN_convolution_border_modes = 1
 */
/*! @decl constant GL_SUN_multi_draw_arrays = 1
 */
/*! @decl constant GL_SURFACE_SIZE_HINT_SUNX = 33234
 */
/*! @decl constant GL_T = 8193
 *! Used in @[glTexGen]
 */
/*! @decl constant GL_T2F_C3F_V3F = 10794
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_T2F_C4F_N3F_V3F = 10796
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_T2F_C4UB_V3F = 10793
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_T2F_N3F_V3F = 10795
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_T2F_V3F = 10791
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_T4F_C4F_N3F_V4F = 10797
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_T4F_V4F = 10792
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_TABLE_TOO_LARGE = 32817
 *! Used in @[glGetError]
 */
/*! @decl constant GL_TABLE_TOO_LARGE_EXT = 32817
 */
/*! @decl constant GL_TEXTURE = 5890
 *! Used in @[glMatrixMode], @[glPushMatrix] and @[glTexEnv]
 */
/*! @decl constant GL_TEXTURE0 = 33984
 *! Used in @[glFeedbackBuffer], @[glGet] and @[glTexCoord]
 */
/*! @decl constant GL_TEXTURE0_ARB = 33984
 */
/*! @decl constant GL_TEXTURE1 = 33985
 */
/*! @decl constant GL_TEXTURE10 = 33994
 */
/*! @decl constant GL_TEXTURE10_ARB = 33994
 */
/*! @decl constant GL_TEXTURE11 = 33995
 */
/*! @decl constant GL_TEXTURE11_ARB = 33995
 */
/*! @decl constant GL_TEXTURE12 = 33996
 */
/*! @decl constant GL_TEXTURE12_ARB = 33996
 */
/*! @decl constant GL_TEXTURE13 = 33997
 */
/*! @decl constant GL_TEXTURE13_ARB = 33997
 */
/*! @decl constant GL_TEXTURE14 = 33998
 */
/*! @decl constant GL_TEXTURE14_ARB = 33998
 */
/*! @decl constant GL_TEXTURE15 = 33999
 */
/*! @decl constant GL_TEXTURE15_ARB = 33999
 */
/*! @decl constant GL_TEXTURE16 = 34000
 */
/*! @decl constant GL_TEXTURE16_ARB = 34000
 */
/*! @decl constant GL_TEXTURE17 = 34001
 */
/*! @decl constant GL_TEXTURE17_ARB = 34001
 */
/*! @decl constant GL_TEXTURE18 = 34002
 */
/*! @decl constant GL_TEXTURE18_ARB = 34002
 */
/*! @decl constant GL_TEXTURE19 = 34003
 */
/*! @decl constant GL_TEXTURE19_ARB = 34003
 */
/*! @decl constant GL_TEXTURE1_ARB = 33985
 */
/*! @decl constant GL_TEXTURE2 = 33986
 */
/*! @decl constant GL_TEXTURE20 = 34004
 */
/*! @decl constant GL_TEXTURE20_ARB = 34004
 */
/*! @decl constant GL_TEXTURE21 = 34005
 */
/*! @decl constant GL_TEXTURE21_ARB = 34005
 */
/*! @decl constant GL_TEXTURE22 = 34006
 */
/*! @decl constant GL_TEXTURE22_ARB = 34006
 */
/*! @decl constant GL_TEXTURE23 = 34007
 */
/*! @decl constant GL_TEXTURE23_ARB = 34007
 */
/*! @decl constant GL_TEXTURE24 = 34008
 */
/*! @decl constant GL_TEXTURE24_ARB = 34008
 */
/*! @decl constant GL_TEXTURE25 = 34009
 */
/*! @decl constant GL_TEXTURE25_ARB = 34009
 */
/*! @decl constant GL_TEXTURE26 = 34010
 */
/*! @decl constant GL_TEXTURE26_ARB = 34010
 */
/*! @decl constant GL_TEXTURE27 = 34011
 */
/*! @decl constant GL_TEXTURE27_ARB = 34011
 */
/*! @decl constant GL_TEXTURE28 = 34012
 */
/*! @decl constant GL_TEXTURE28_ARB = 34012
 */
/*! @decl constant GL_TEXTURE29 = 34013
 */
/*! @decl constant GL_TEXTURE29_ARB = 34013
 */
/*! @decl constant GL_TEXTURE2_ARB = 33986
 */
/*! @decl constant GL_TEXTURE3 = 33987
 */
/*! @decl constant GL_TEXTURE30 = 34014
 */
/*! @decl constant GL_TEXTURE30_ARB = 34014
 */
/*! @decl constant GL_TEXTURE31 = 34015
 */
/*! @decl constant GL_TEXTURE31_ARB = 34015
 */
/*! @decl constant GL_TEXTURE3_ARB = 33987
 */
/*! @decl constant GL_TEXTURE4 = 33988
 */
/*! @decl constant GL_TEXTURE4_ARB = 33988
 */
/*! @decl constant GL_TEXTURE5 = 33989
 */
/*! @decl constant GL_TEXTURE5_ARB = 33989
 */
/*! @decl constant GL_TEXTURE6 = 33990
 */
/*! @decl constant GL_TEXTURE6_ARB = 33990
 */
/*! @decl constant GL_TEXTURE7 = 33991
 */
/*! @decl constant GL_TEXTURE7_ARB = 33991
 */
/*! @decl constant GL_TEXTURE8 = 33992
 */
/*! @decl constant GL_TEXTURE8_ARB = 33992
 */
/*! @decl constant GL_TEXTURE9 = 33993
 */
/*! @decl constant GL_TEXTURE9_ARB = 33993
 */
/*! @decl constant GL_TEXTURE_1D = 3552
 *! Used in @[glBindTexture], @[glCopyTexImage1D], @[glCopyTexSubImage1D],
 *! @[glEnable], @[glGet], @[glGetTexImage], @[glIsEnabled],
 *! @[glPushAttrib], @[glTexImage1D], @[glTexParameter] and
 *! @[glTexSubImage1D]
 */
/*! @decl constant GL_TEXTURE_1D_BINDING = 32872
 */
/*! @decl constant GL_TEXTURE_2D = 3553
 *! Used in @[glBindTexture], @[glCopyTexImage2D], @[glCopyTexSubImage2D],
 *! @[glEnable], @[glGet], @[glGetTexImage], @[glIsEnabled],
 *! @[glPushAttrib], @[glTexImage2D], @[glTexParameter] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_2D_BINDING = 32873
 */
/*! @decl constant GL_TEXTURE_3D = 32879
 *! Used in @[glBindTexture], @[glEnable], @[glGet], @[glGetTexImage],
 *! @[glIsEnabled], @[glPushAttrib] and @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_3D_BINDING = 32874
 */
/*! @decl constant GL_TEXTURE_3D_EXT = 32879
 */
/*! @decl constant GL_TEXTURE_ALPHA_SIZE = 32863
 */
/*! @decl constant GL_TEXTURE_BASE_LEVEL = 33084
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_BINDING_1D = 32872
 *! Used in @[glBindTexture] and @[glGet]
 */
/*! @decl constant GL_TEXTURE_BINDING_2D = 32873
 *! Used in @[glBindTexture], @[glGet] and @[glPushAttrib]
 */
/*! @decl constant GL_TEXTURE_BINDING_3D = 32874
 *! Used in @[glBindTexture] and @[glGet]
 */
/*! @decl constant GL_TEXTURE_BINDING_CUBE_MAP = 34068
 *! Used in @[glGet]
 */
/*! @decl constant GL_TEXTURE_BIT = 262144
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_TEXTURE_BLUE_SIZE = 32862
 */
/*! @decl constant GL_TEXTURE_BORDER = 4101
 *! Used in @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glGetTexImage] and @[glTexSubImage1D]
 */
/*! @decl constant GL_TEXTURE_BORDER_COLOR = 4100
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_COLOR_TABLE_SGI = 32956
 */
/*! @decl constant GL_TEXTURE_COMPARE_FUNC = 34893
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_COMPARE_MODE = 34892
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_COMPONENTS = 4099
 */
/*! @decl constant GL_TEXTURE_COMPRESSED = 34465
 */
/*! @decl constant GL_TEXTURE_COMPRESSED_IMAGE_SIZE = 34464
 */
/*! @decl constant GL_TEXTURE_COMPRESSION_HINT = 34031
 *! Used in @[glGet], @[glHint] and @[glPushAttrib]
 */
/*! @decl constant GL_TEXTURE_COORD_ARRAY = 32888
 *! Used in @[glEnableClientState], @[glGet], @[glIsEnabled] and
 *! @[glTexCoordPointer]
 */
/*! @decl constant GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING = 34970
 *! Used in @[glGet] and @[glTexCoordPointer]
 */
/*! @decl constant GL_TEXTURE_COORD_ARRAY_POINTER = 32914
 *! Used in @[glTexCoordPointer]
 */
/*! @decl constant GL_TEXTURE_COORD_ARRAY_SIZE = 32904
 *! Used in @[glGet], @[glIsEnabled] and @[glTexCoordPointer]
 */
/*! @decl constant GL_TEXTURE_COORD_ARRAY_STRIDE = 32906
 *! Used in @[glGet], @[glIsEnabled] and @[glTexCoordPointer]
 */
/*! @decl constant GL_TEXTURE_COORD_ARRAY_TYPE = 32905
 *! Used in @[glGet], @[glIsEnabled] and @[glTexCoordPointer]
 */
/*! @decl constant GL_TEXTURE_CUBE_MAP = 34067
 *! Used in @[glBindTexture], @[glCopyTexImage2D], @[glEnable], @[glGet],
 *! @[glIsEnabled], @[glTexImage2D] and @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_CUBE_MAP_NEGATIVE_X = 34070
 *! Used in @[glCopyTexImage2D], @[glCopyTexSubImage2D], @[glGetTexImage],
 *! @[glTexImage2D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_CUBE_MAP_NEGATIVE_Y = 34072
 *! Used in @[glCopyTexImage2D], @[glCopyTexSubImage2D], @[glGetTexImage],
 *! @[glTexImage2D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_CUBE_MAP_NEGATIVE_Z = 34074
 *! Used in @[glCopyTexImage2D], @[glCopyTexSubImage2D], @[glGetTexImage],
 *! @[glTexImage2D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_CUBE_MAP_POSITIVE_X = 34069
 *! Used in @[glCopyTexImage2D], @[glCopyTexSubImage2D], @[glGetTexImage],
 *! @[glTexImage2D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_CUBE_MAP_POSITIVE_Y = 34071
 *! Used in @[glCopyTexImage2D], @[glCopyTexSubImage2D], @[glGetTexImage],
 *! @[glTexImage2D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_CUBE_MAP_POSITIVE_Z = 34073
 *! Used in @[glCopyTexImage2D], @[glCopyTexSubImage2D], @[glGetTexImage],
 *! @[glTexImage2D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_DEPTH = 32881
 */
/*! @decl constant GL_TEXTURE_DEPTH_EXT = 32881
 */
/*! @decl constant GL_TEXTURE_ENV = 8960
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_TEXTURE_ENV_COLOR = 8705
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_TEXTURE_ENV_MODE = 8704
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_TEXTURE_FILTER_CONTROL = 34048
 *! Used in @[glTexEnv]
 */
/*! @decl constant GL_TEXTURE_GEN_MODE = 9472
 *! Used in @[glPushAttrib] and @[glTexGen]
 */
/*! @decl constant GL_TEXTURE_GEN_Q = 3171
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPushAttrib] and
 *! @[glTexGen]
 */
/*! @decl constant GL_TEXTURE_GEN_R = 3170
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPushAttrib] and
 *! @[glTexGen]
 */
/*! @decl constant GL_TEXTURE_GEN_S = 3168
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPushAttrib] and
 *! @[glTexGen]
 */
/*! @decl constant GL_TEXTURE_GEN_T = 3169
 *! Used in @[glEnable], @[glGet], @[glIsEnabled], @[glPushAttrib] and
 *! @[glTexGen]
 */
/*! @decl constant GL_TEXTURE_GREEN_SIZE = 32861
 */
/*! @decl constant GL_TEXTURE_HEIGHT = 4097
 *! Used in @[glCopyTexSubImage2D], @[glGetTexImage] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_INTENSITY_SIZE = 32865
 */
/*! @decl constant GL_TEXTURE_INTERNAL_FORMAT = 4099
 *! Used in @[glGetTexImage]
 */
/*! @decl constant GL_TEXTURE_LOD_BIAS = 34049
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glReadPixels],
 *! @[glTexEnv], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_TEXTURE_LUMINANCE_SIZE = 32864
 */
/*! @decl constant GL_TEXTURE_MAG_FILTER = 10240
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_MATRIX = 2984
 *! Used in @[glFrustum], @[glGet], @[glIsEnabled], @[glLoadIdentity],
 *! @[glLoadMatrix], @[glMultMatrix], @[glOrtho], @[glPushMatrix],
 *! @[glRotate], @[glScale] and @[glTranslate]
 */
/*! @decl constant GL_TEXTURE_MAX_LEVEL = 33085
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_MAX_LOD = 33083
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_MIN_FILTER = 10241
 *! Used in @[glTexEnv] and @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_MIN_LOD = 33082
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_PRIORITY = 32870
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_RED_SIZE = 32860
 */
/*! @decl constant GL_TEXTURE_RESIDENT = 32871
 */
/*! @decl constant GL_TEXTURE_STACK_DEPTH = 2981
 *! Used in @[glGet], @[glIsEnabled] and @[glPushMatrix]
 */
/*! @decl constant GL_TEXTURE_WIDTH = 4096
 *! Used in @[glCopyTexSubImage1D], @[glCopyTexSubImage2D],
 *! @[glGetTexImage], @[glTexSubImage1D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_TEXTURE_WRAP_R = 32882
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_WRAP_R_EXT = 32882
 */
/*! @decl constant GL_TEXTURE_WRAP_S = 10242
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TEXTURE_WRAP_T = 10243
 *! Used in @[glTexParameter]
 */
/*! @decl constant GL_TRANSFORM_BIT = 4096
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_TRANSPOSE_COLOR_MATRIX = 34022
 *! Used in @[glGet]
 */
/*! @decl constant GL_TRANSPOSE_MODELVIEW_MATRIX = 34019
 *! Used in @[glGet]
 */
/*! @decl constant GL_TRANSPOSE_PROJECTION_MATRIX = 34020
 *! Used in @[glGet]
 */
/*! @decl constant GL_TRANSPOSE_TEXTURE_MATRIX = 34021
 *! Used in @[glGet]
 */
/*! @decl constant GL_TRIANGLES = 4
 *! Used in @[glBegin], @[glDrawArrays] and @[glDrawElements]
 */
/*! @decl constant GL_TRIANGLE_FAN = 6
 *! Used in @[glBegin], @[glDrawArrays] and @[glDrawElements]
 */
/*! @decl constant GL_TRIANGLE_STRIP = 5
 *! Used in @[glBegin], @[glDrawArrays] and @[glDrawElements]
 */
/*! @decl constant GL_TRUE = 1
 *! Used in @[glColorMask], @[glEdgeFlag], @[glEnable], @[glGet],
 *! @[glIsEnabled], @[glIsList], @[glIsTexture] and @[glReadPixels]
 */
/*! @decl constant GL_UNPACK_ALIGNMENT = 3317
 *! Used in @[glDrawPixels] and @[glGet]
 */
/*! @decl constant GL_UNPACK_IMAGE_HEIGHT = 32878
 *! Used in @[glGet]
 */
/*! @decl constant GL_UNPACK_IMAGE_HEIGHT_EXT = 32878
 */
/*! @decl constant GL_UNPACK_LSB_FIRST = 3313
 *! Used in @[glDrawPixels], @[glGet], @[glTexImage1D] and @[glTexImage2D]
 */
/*! @decl constant GL_UNPACK_ROW_LENGTH = 3314
 *! Used in @[glGet]
 */
/*! @decl constant GL_UNPACK_SKIP_IMAGES = 32877
 *! Used in @[glGet]
 */
/*! @decl constant GL_UNPACK_SKIP_IMAGES_EXT = 32877
 */
/*! @decl constant GL_UNPACK_SKIP_PIXELS = 3316
 *! Used in @[glGet]
 */
/*! @decl constant GL_UNPACK_SKIP_ROWS = 3315
 *! Used in @[glGet]
 */
/*! @decl constant GL_UNPACK_SWAP_BYTES = 3312
 *! Used in @[glGet]
 */
/*! @decl constant GL_UNSIGNED_BYTE = 5121
 *! Used in @[glColorPointer], @[glDrawPixels], @[glGetTexImage],
 *! @[glIndexPointer], @[glReadPixels], @[glTexImage1D], @[glTexImage2D],
 *! @[glTexSubImage1D] and @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_BYTE_2_3_3_REV = 33634
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_BYTE_3_3_2 = 32818
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_INT = 5125
 *! Used in @[glColorPointer], @[glDrawPixels], @[glGetTexImage],
 *! @[glReadPixels], @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D]
 *! and @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_INT_10_10_10_2 = 32822
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_INT_2_10_10_10_REV = 33640
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_INT_8_8_8_8 = 32821
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_INT_8_8_8_8_REV = 33639
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_SHORT = 5123
 *! Used in @[glColorPointer], @[glDrawPixels], @[glGetTexImage],
 *! @[glReadPixels], @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D]
 *! and @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_SHORT_1_5_5_5_REV = 33638
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_SHORT_4_4_4_4 = 32819
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_SHORT_4_4_4_4_REV = 33637
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_SHORT_5_5_5_1 = 32820
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_SHORT_5_6_5 = 33635
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_UNSIGNED_SHORT_5_6_5_REV = 33636
 *! Used in @[glDrawPixels], @[glGetTexImage], @[glReadPixels],
 *! @[glTexImage1D], @[glTexImage2D], @[glTexSubImage1D] and
 *! @[glTexSubImage2D]
 */
/*! @decl constant GL_V2F = 10784
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_V3F = 10785
 *! Used in @[glInterleavedArrays]
 */
/*! @decl constant GL_VENDOR = 7936
 *! Used in @[glGetString]
 */
/*! @decl constant GL_VERSION = 7938
 *! Used in @[glGetString]
 */
/*! @decl constant GL_VERSION_1_1 = 1
 */
/*! @decl constant GL_VERSION_1_2 = 1
 */
/*! @decl constant GL_VERSION_1_3 = 1
 */
/*! @decl constant GL_VERTEX_ARRAY = 32884
 *! Used in @[glArrayElement], @[glDrawArrays], @[glDrawElements],
 *! @[glEnableClientState], @[glGet], @[glIsEnabled] and
 *! @[glVertexPointer]
 */
/*! @decl constant GL_VERTEX_ARRAY_BUFFER_BINDING = 34966
 *! Used in @[glGet] and @[glVertexPointer]
 */
/*! @decl constant GL_VERTEX_ARRAY_POINTER = 32910
 *! Used in @[glVertexPointer]
 */
/*! @decl constant GL_VERTEX_ARRAY_SIZE = 32890
 *! Used in @[glGet] and @[glVertexPointer]
 */
/*! @decl constant GL_VERTEX_ARRAY_STRIDE = 32892
 *! Used in @[glGet] and @[glVertexPointer]
 */
/*! @decl constant GL_VERTEX_ARRAY_TYPE = 32891
 *! Used in @[glGet] and @[glVertexPointer]
 */
/*! @decl constant GL_VERTEX_PROGRAM_POINT_SIZE = 34370
 *! Used in @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_VERTEX_PROGRAM_TWO_SIDE = 34371
 *! Used in @[glEnable], @[glGet] and @[glIsEnabled]
 */
/*! @decl constant GL_VIEWPORT = 2978
 *! Used in @[glGet] and @[glViewport]
 */
/*! @decl constant GL_VIEWPORT_BIT = 2048
 *! Used in @[glPushAttrib]
 */
/*! @decl constant GL_WRAP_BORDER_SUN = 33236
 */
/*! @decl constant GL_XOR = 5382
 *! Used in @[glLogicOp]
 */
/*! @decl constant GL_ZERO = 0
 *! Used in @[glBlendFunc], @[glGet] and @[glStencilOp]
 */
/*! @decl constant GL_ZOOM_X = 3350
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPixelZoom] and
 *! @[glPushAttrib]
 */
/*! @decl constant GL_ZOOM_Y = 3351
 *! Used in @[glCopyPixels], @[glDrawPixels], @[glGet], @[glPixelZoom] and
 *! @[glPushAttrib]
 */

/*! @endmodule
 */

