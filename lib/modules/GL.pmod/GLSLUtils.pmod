#pike __REAL_VERSION__

//! This module contains various utilities that greatly simplifies
//! working with GLSL and some other extensions such as vertex buffers
//! and render buffers.

#require constant(GL.GLSL)
import GL;
import GLSL;

#define CHECK_ERROR(X) do{while(int err = glGetError()) { werror(#X  ": glerror #%x\n", err);}}while(0)

private array(string) shader_path = ({});
private string shader_defines = "";
private string shader_ext = "";

//! Set the extension added to all shader filerequests.
//! The default is to never add an extension, which means that the
//! full filename has to be specified.
void set_shader_ext( string x )
{
    shader_ext = x;
}

//! Adds a directory to the list of directories where shaders can be
//! found.
void add_shader_path( string x )
{
    shader_path += ({x});
}

//! Remove all preprocessing defines.
void clear_shader_defines()
{
    shader_defines = "";
}

//! Add a preprocessing define that will be used for all shader
//! compilations from now forward.
void add_shader_define( string x )
{
    shader_defines += x;
}

private mapping(string:GLSLProgram) programs = ([]);

GLSLProgram get_program( string filename )
//! Convenice function to compile and cache a GLSLProgram
{
    if( programs[filename] )
        return programs[filename];
    return programs[filename] = GLSLProgram( filename );
}

class VertexBuffer
//! This class defines a vertex buffer. This is a chunk of memory
//! defining vertex attributes for future drawing. Any number of
//! vertex buffers (up to a hardware specific limit) can be added to
//! any GLSLProgram.
//!
//! Vertex buffers are generally speaking stored on the graphic card
//! for fast access, making it possible to draw huge amounts of
//! primitives without using all available CPU->GPU memory bandwidth.
//!
//! Normal usage of this class is to create it, then call add_[type]
//! for each vertex attribute (@[add_float], @[add_vec2] etc) defined
//! in your program, then call @[set_size] or @[set_data] to set the
//! size of the buffer, add the VertexBuffer to any relevant
//! GLSLProgram instances, and then optionally call @[pwrite],
//! @[vertex] or @[stream_vertex]() to update the data.
//!
//! The add_* functions can not be called once the VertexBuffer has
//! been added to a @[GLSLProgram]. @[set_size], @[set_data] or @[recalc]
//! must be called before the VertexBuffer is added to a
//! @[GLSLProgram].
{
    int offset;
    //! The index of the last vertex that was written using @[stream]

    //! The maximum offset ever written using @[stream]
    int current_size; // for streaming writing

    //! The total size of the VertexBuffer, counted in vertices
    int size;

    //! The size of a single vertex, in bytes
    int stride;

    //! The size of a single vertex, in floats
    int vertex_size;

    //! The vertexbuffer id (the object id passed to @[glBindBuffer]).
    int id;

    int draw_mode;
    array(Attribute) attribute_list = ({});
    mapping(string:Attribute) attributes_by_name = ([]);

    void stream( array(float) data )
    //! Append the @[data] to the current object. Once the end of the
    //! buffer has been reached, start over from the beginning. This
    //! is useful for particle systems of various kinds, where each
    //! vertex (or every few vertices) indicate a separate particle
    //! that evolves over time using a GLSLProgram, and there is no
    //! need to send the whole list to the graphic card each frame.
    //!
    //! When calling glDrawArrays using this kind of VertexBuffer, use
    //! the current_size member variable, it indicates the last fully
    //! written vertice.
    {
        int num = sizeof(data) / vertex_size;
        vertex( offset, data );
        offset += num;
        while( offset > size )
        {
            current_size = size;
            offset -= size;
        }
        if( offset > current_size )
            current_size = offset;
    }

    void vertex( int vertex, array(float) data )
    //! Give data starting at the given vertex. Any number of vertices
    //! up to the full size of the VertexBuffer can be written from
    //! this point onwards.
    {
        int num = (sizeof(data)+vertex_size-1) / vertex_size;
        if( num + vertex > size )
        {
            pwrite( vertex * stride, data[..(size-vertex)*vertex_size-1]);
            pwrite( 0, data[(size-vertex)*vertex_size..]);
        }
        else
            pwrite( vertex * stride, data );
    }

    void pwrite( int offset, array(float) data )
    //! Write @[data] at the byteoffset @[offset]. The size of a
    //! vertex is given by the @[stride] member variable in this
    //! class.
    //!
    //! It's usually more convenient to use the @[vertex] or @[stream]
    //! methods when updating data.
    {
        glBindBuffer( GL_ARRAY_BUFFER, id );
        glBufferSubData( GL_ARRAY_BUFFER, offset, data );
    }

    class Attribute( string name, int type, bool normalize, int size, int offset)
    {
        Attribute linked_to;

        array arr()
        {
            return ({name,size,type,normalize,offset});
        }
    }

    //! Add a float attribute named @[name].
    void add_float( string name ) { add_attribute( name, GL_FLOAT, GL_FALSE, 1 ); }

    //! Add a vec2 attribute named @[name].
    void add_vec2( string name )  { add_attribute( name, GL_FLOAT, GL_FALSE, 2 ); }

    //! Add a vec3 attribute named @[name].
    void add_vec3( string name )  { add_attribute( name, GL_FLOAT, GL_FALSE, 3 ); }

    //! Add a vec4 attribute named @[name].
    void add_vec4( string name )  { add_attribute( name, GL_FLOAT, GL_FALSE, 4 ); }

    //! Add a generic attribute named @[name], of the type @[type]
    //! (eg, GL_FLOAT), @[normalized] or not of size @[size].
    //!
    //! As an example add_vec2(@[name]) is an alias for add_attribute(
    //! @[name], GL_FLOAT, GL_FALSE, 2 )
    void add_attribute( string name, int type, bool normalize, int size )
    {
        if( attributes_by_name[name] )
            error("There is already an attribute named %O\n", name );
        attributes_by_name[name] = Attribute( name, type, normalize, size, 0 );
        attribute_list += ({ attributes_by_name[name] });
    }

    //! Add @[name] as an alias for @[name2]. As an example
    //! add_alias( "normal", "pos") will make the vertex attribute
    //! normal have the same value as pos without using additional
    //! storage.
    //!
    //! The size can be smaller than the size for the original
    //! attribute, but never larger. If no size is given they will
    //! have the same size.
    void add_alias( string name, string name2, int|void size )
    {
        Attribute link = attributes_by_name[name2];
        if( !link )
            error("The attribute %O must be defined before add_alias is called\n", name2 );
        if( size > link->size )
            error("The attribute %O is smaller than %d\n", name2, size );

        if( attributes_by_name[name] )
            error("There is already an attribute named %O\n", name );
        attributes_by_name[name] =
            Attribute( name, link->type, link->normalize, size, 0 );
        attributes_by_name[name]->linked_to = link;
        attribute_list += ({ attributes_by_name[name] });
    }

    //! Set the size of the VertexBuffer. The size is given in
    //! @[stride] increments. That is, it defines the number of
    //! complete vertexes that can be generated from this buffer, not
    //! the number of floats or bytes in it.
    //!
    //! @[set_size] will remove all the data that was previously
    //! present in the buffer.
    void set_size( int size )
    {
        recalc();
        vertex_size = stride / GLSL_FLOAT_SIZE;
        last_vbo = id;
        glBindBuffer( GL_ARRAY_BUFFER, id );
        glBufferData( GL_ARRAY_BUFFER, stride*size, draw_mode);
        this->size = size;
        CHECK_ERROR(VertexBuffer::set_size);
    }

    //! Set the data to an array of floating point numbers. The
    //! attributes are always ordered in the array according to the
    //! order the various add_* functions were called. Note that
    //! add_alias does not add a new attribute, only an alias for an
    //! existing one.
    void set_data( array(float) data )
    {
        recalc();
        vertex_size = stride / GLSL_FLOAT_SIZE;
        if( sizeof(data) % vertex_size )
            error("The size of the data array is not an even multiple of the total\n"
                  "size of the vertex attributes\n");

        this->size = sizeof(data)/vertex_size;
        last_vbo = id;
        glBindBuffer( GL_ARRAY_BUFFER, id );
        glBufferData( GL_ARRAY_BUFFER, data, draw_mode);
        CHECK_ERROR(VertexBuffer::set_data);
    }

    void recalc()
    //! Recalculate the offsets for all attributes. Normally called
    //! automatically from set_data and set_size.
    {
        stride = 0;
        foreach( attribute_list, Attribute attr )
        {
            if( attr->linked_to )
                attr->offset = attr->linked_to->offset;
            else
            {
                attr->offset = stride;
                stride += attr->size * GLSL_FLOAT_SIZE;
            }
        }
    }


    array(array(string|int|bool)) attributes()
    //! Method used by @[GLSLProgram] to get a list of the attributes.
    {
        array res = ({});
        foreach( attribute_list, Attribute attr )
            res += ({ attr->arr() });
        return res;
    }


    protected string _sprintf( int flag )
    {
        switch( flag )
        {
            case 'O':
                if( !stride )
                    recalc();

                array(string) vals = ({});
                foreach( attribute_list, Attribute attr )
                {
                    if( attr->linked_to )
                        vals += ({"alias "+attr->name+"="+attr->linked_to->name});
                    else if( attr->size == 1 )
                        vals += ({"float "+attr->name});
                    else
                        vals += ({"vec"+attr->size+" "+attr->name});
                }
                return sprintf( "%t(%s [stride %d, %d vertices])", this, vals*", ", stride, size);
            case 't':
                return "VertexBuffer";
        }
    }

    //! Create a new vertex buffer, draw_mode is a usage hint.
    //!
    //! GL_STREAM_DRAW: Draw multiple times, updating the data ocassionally
    //! GL_STATIC_DRAW: Set the data once, then draw multiple times
    //! GL_DYNAMIC_DRAW: Draw and update all the time
    //!
    //! The mode is only a hint, it does not really affect the
    //! functionality of the buffer.
    protected void create(int draw_mode)
    {
        this->draw_mode = draw_mode;
        [id] = glGenBuffers( 1 );
    }
}

bool nvidia = true;
int last_vbo;

class GLSLProgram
//! This class keeps track of all state related to a GLSL program,
//! such as the shaders (vertex and pixel), the uniform variables and
//! any vertex buffer objects or textures.
{

    int enabled;
    int prog;
    multiset vertex_disable = (<>);
    mapping(string:int) vars = ([]);
    mapping(string:mixed) var_values = ([]);
    array(array) textures = ({});
    mapping(string:array(int)) vertex_enable = ([]);

    private string shader(string code, string path, string defines, string type)
    {
        // 1: Split code. This part is not really needed on nvidia cards.
        // Still, it won't hurt things.
        string tmp;

        array tokens =
            Parser.C.group(Parser.C.hide_whitespaces(Parser.C.tokenize(Parser.C.split(code),path)));

        foreach( tokens; int i; array|Parser.C.Token token )
        {
            if( !objectp(token) ) continue;

            void remove_current_function()
            {
                int start = i;
                int end = i;
                while( start > 0 && tokens[start]->text != "void" )
                    start--;
                while( end<sizeof(tokens) &&
                       (!arrayp(tokens[end]) ||
                        (tokens[end][0]->text != "{" )))
                    end++;
                if( end == sizeof(tokens))
                    error("End of file while searching for *_main codeblock.\n");
                for( int j = start; j<=end; j++ )
                    tokens[j] = 0;
            };
            void remove_current_variable()
            {
                int start = i;
                int end = i;
                while( end<sizeof(tokens) && tokens[end]->text != ";" )
                    end++;
                if( end == sizeof(tokens) )
                    error("End fo file while searching for end of attribute variable");
                for( int j=start; j<=end; j++ )
                    tokens[j] = 0;
            };
            if( has_prefix( (tmp=token->text), "#" ) )
            {
                if( sscanf( tmp, "#%*sinclude \"%s\"", tmp ) == 2 )
                {
                    tmp = combine_path(path,"..",tmp);

                    foreach( shader_path, string f)
                    {
                        if( file_stat( f=combine_path(f,tmp)))
                            tokens[i] = Parser.C.Token(shader(Stdio.read_file(f), tmp, defines, type));
                    }
                }
            }

            switch( ([object(Parser.C.Token)]token)->text )
            {
                case "attribute":
                    if( type == "fragment" )
                    {
                        remove_current_variable();
                    }
                    break;
                case "pixel_main":
                case "fragment_main":
                    if( type == "fragment" )
                        tokens[i] = Parser.C.Token( "main");
                    else
                        remove_current_function();
                    break;

                case "vertex_main":
                    if( type == "vertex" )
                        tokens[i] = Parser.C.Token("main");
                    else
                        remove_current_function();
                    break;
            }
        }
        code = Parser.C.reconstitute_with_line_numbers( tokens - ({0}) );
        if( !nvidia)
            code = replace(code, sprintf(" %O\n",path), "\n");
        string res =
            (shader_defines||"") + "\n" +
            (defines||"") + "\n" + code;
        array a = res / "\n";
        string found_version;
        a = filter(a, lambda(string s) { if (has_prefix(s, "#version")) { found_version = s; return 0; } return !has_prefix(s, "#line"); });
        if (found_version) a = ({ found_version }) + a;
        return a * "\n";
    }

    //! Compile the given string as a pixel/vertex shader. The string
    //! is compiled twice, once with VERTEX_SHADER defined, the other
    //! time with FRAGMENT_SHADER defined.
    //!
    //! The functions vertex_main and fragment_main are special, in
    //! that when the vertex shader is compiled vertex_main is renamed
    //! to main and fragment_main is totally removed from the source.
    //! When the fragment shader is compiled it's the other way
    //! around.
    //!
    //! This is done to make it easier to develop shaders, it's
    //! generally speaking more convenient to have them in one file
    //! that it is to have them in two.
    int compile_string( string code, string path )
    {
//         FIXME: Add #include/import/whatever support
        int res = glCreateProgram();
        int shdr = glCreateShader( GL_VERTEX_SHADER );
        glShaderSource( shdr, shader(code,path||"-","#define VERTEX_SHADER\n","vertex") );
        glCompileShader( shdr );
        glAttachShader( res, shdr );

        shdr = glCreateShader( GL_FRAGMENT_SHADER );
        glShaderSource( shdr, shader(code, path||"-","#define FRAGMENT_SHADER\n","fragment") );
        glCompileShader( shdr );
        glAttachShader( res, shdr );
        glLinkProgram(res);
        prog=res;
        return res;
    }

    Stdio.Stat current_stat;
    string source_path;

    void check_file(string filename)
    {
        if( file_stat( source_path )->mtime != current_stat->mtime )
        {
            werror(source_path+" updated on disk, recompiling\n");
            compile_file( filename );
        }
        else
            call_out( check_file, 1.0, filename );
    }

    //! Compile the shader source found in @[file_name]. If filename is
    //! relative, the paths added by @[add_shader_path] will be
    //! searched. If -1 is returned, no file was compiled.
    //!
    //! This function is usually called from the @[create] method, but
    //! if no filename is passed there you can call this function (or
    //! the @[compile_string] function) to specify the source.
    //!
    //! The file is compiled twice, once with VERTEX_SHADER defined,
    //! the other time with FRAGMENT_SHADER defined.
    //!
    //! The functions vertex_main and fragment_main are special, in
    //! that when the vertex shader is compiled vertex_main is renamed
    //! to main and fragment_main is totally removed from the source.
    //! When the fragment shader is compiled it's the other way
    //! around.
    int compile_file( string file_name )
    {
        call_out( check_file, 1.0, file_name );
        if( strlen( shader_ext ) )
            file_name += "."+shader_ext;
        foreach( shader_path, string f )
        {
            if( current_stat = file_stat( combine_path(f,file_name ) ) )
            {
                source_path = combine_path(f,file_name );
                return compile_string( Stdio.read_file( combine_path(f,file_name ) ),file_name );
            }
        }
        error("Could not find %O in %{%O %}\n", file_name, shader_path );
        return -1;
    }

    //! Create a new GLSL shader. If @[name] is specified, it
    //! indicates a filename passed to @[compile_file].
    protected void create(string|void name)
    {
        if( name )
        {
            if( mixed error = catch(compile_file(name)) )
                werror("While compiling %O:\n%s", name, describe_error(error));
        }
    }

    //! Adds a texture to the list of textures used by the shader.
    //! If this function is used the allocation of texture units is
    //! done automatically by this class.
    //!
    //! There are really two variants of this function: If
    //! @[type_or_texture] is an integer, the @[id] indicates the
    //! texture object ID (as given by @[glGenTextures]), and
    //! @[type_or_texture] indicates the texture type (GL_TEXTURE_2D
    //! etc).
    //!
    //! If @[type_or_texture] is an object, it's assumed that there is
    //! a texture_type member variable (indicating the texture type,
    //! such as GL_TEXTURE_2D) and a use method that will bind the
    //! texture to the currently active texture unit.
    //!
    //! The GLUE.Texture class meets these requirements. The
    //! @[RenderBuffer] objects does not, however.
    //!
    //! A RenderBuffer can be added as an texture by calling
    //! @[add_texture]( name, buffer->texture_type, buffer->texture )
    //!
    //! There are currently no checks done to ensure that you don't
    //! use more textures than there are texture units on your
    //! graphics card.
    void add_texture(string name, int|object type_or_texture, int|void id)
    {
        if( (vars[name]=glGetUniformLocation(prog,name)) != -1 )
            textures += ({({name,type_or_texture,id})});
    }

    //! Call the function @[x](@@args) with this program activated.
    //!
    //! This will bind all texture units to their correct textures,
    //! set up any vertex pointers that have been defined, and set
    //! uniforms to their value.
    //!
    //! Once the function has been called, all texture units except
    //! the default one will be disabled, and the vertex array
    //! pointers will be reset.
    void draw(function x, mixed ... args)
    {
        use();
        x(@args);
        disable();
    }

    //! Add all vertex attributes defined in the VertexBuffer vbo.
    //! This is equivalent to calling @[vertex_pointer] once for each
    //! attribute (with the difference that stride, size and offset
    //! are calculated automatically for you)
    void add_vertex_buffer( VertexBuffer vbo )
    {
        int ptr = vbo->id;
        int stride = vbo->stride;

        foreach( vbo->attributes(), [string name, int size, int type, bool normalize, int offset] )
            vertex_pointer( name, ptr, size, type, normalize, stride, offset );
    }

    //! Add a single vertex attribute pointer. This is usually used in
    //! combination with glDrawArrays to quickly draw a lot of
    //! primitives.
    //!
    //! @[name] is the name of the vertex type variable in the program.
    //! @[ptr] is the vertex buffer ID.
    //! @[size] is the size of the attribute (a vec3 has size 3, as an example)
    //! @[type] is the type, usually GL_FLOAT (actually, anything else
    //! requires extensions not currently supported by this class)
    //! If @[normalize] is true, the value will be clamped between 0.0 and 1.0.
    //! @[stride] is the distance between two attributes of this type
    //! in the buffer, and @[offset] is the offset from the start of
    //! the buffer to the first attribute.
    void vertex_pointer(string name, int ptr, int size, int type,
                        bool normalize, int stride, int offset )
    {
        if(zero_type(vars[name]))
            vars[name]=glGetAttribLocation(prog,name);
        if( vars[name] == -1 ) return;
        if( enabled )
        {
	    if( last_vbo != ptr )
	    {
		last_vbo = ptr;
		glBindBuffer( GL_ARRAY_BUFFER, ptr );
	    }
            glVertexAttribPointer( vars[name], size, type, normalize, stride, offset );
            glEnableVertexAttribArray( vars[name] );
            vertex_disable[vars[name]]=1;
        }
        else
        {
            vertex_enable[name] = ({ptr, size, type, normalize, stride, offset});
        }
    }

    //! Set the vertex attribute @[name] to @[to]. @[to] is one or
    //! more floats or integers.
    //!
    //! The attibute will have this value for all new glVertex calls
    //! until this function is called again.
    void vertex(string name, mixed ... to )
    {
        if(!vars[name])
            vars[name]=glGetAttribLocation(prog,name);
        glVertexAttrib(vars[name],@to);
    }


    //! Set the uniform variable @[name] to the value @[to] (one or
    //! more floats or integers). The value will remain in effect
    //! until the next time this function is called (even if another
    //! program is used in between)
    void set(string name, mixed ... to )
    {
        if(zero_type(vars[name]))
            vars[name]=glGetUniformLocation(prog,name);
        if(vars[name] != -1 )
        {
            if(!enabled)
                var_values[name]=to;
            else
                glUniform(vars[name],@to);
        }
    }

    //! Disable this program. This will also disable all extra texture
    //! units that were needed to render with it, if any, and unbind
    //! the vertex pointer attributes, if any.
    void disable()
    {
        enabled=0;
        glUseProgram(0);
        foreach( vertex_disable; int v; )
            glDisableVertexAttribArray( v );
        foreach(textures;int n; array texture)
        {
            glActiveTexture(GL_TEXTURE0+n);
            glDisable( objectp(texture[1])?texture[1]->texture_type:texture[1] );
        }
        glActiveTexture(GL_TEXTURE0);
    }

    //! Enable the program, setting all uniform variables to their
    //! values as set by @[set], enabling the required number of
    //! texture units and binding the correct textures, as given by
    //! @[add_texture] and finally initializing any vertex pointers
    //! set using @[vertex_pointer] or @[add_vertex_buffer].
    void use()
    {
	last_vbo = -1;
        enabled=1;
        glUseProgram(prog);
        vertex_disable = (<>);
        foreach(textures;int n; array texture)
        {
            glActiveTexture(GL_TEXTURE0+n);
            if(objectp(texture[1]))
            {
                glEnable(texture[1]->texture_type);
                texture[1]->use();
            }
            else
            {
                glEnable( texture[1] );
                glBindTexture(texture[1],texture[2]);
            }
            glUniform(vars[texture[0]],n);
        }
        foreach(var_values;string name;mixed val)
            glUniform(vars[name],@val);
        foreach( vertex_enable; string v;array data )
            vertex_pointer( v,@data);
    }
}


#if constant(GL.GLSL.glFramebufferTexture2DEXT)

class RenderBuffer
//! A rendering buffer. This is usually used to do
//! offscreeen-rendering with higher precision, or full-screen special
//! effects such as blur etc. This class is not present if there is no
//! FramebufferTexture2DEXT extention available. To my knowledge all
//! cards with GLSL support also support this feature.
{
    //! The object ID of the FBO object
    int fbo = -1;

    //! The object ID of the texture object.
    int texture = -1;

    //! The type of the texture, GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE_ARB typically.
    int texture_type;

    //! The format of the texture object
    int internal_format, format;

    //! The object ID of the depth buffer object, if any. Otherwise -1
    int db = -1;

    //! Does the RenderBuffer has mipmaps and alpha channel?
    bool mipmaps, alpha;

    //! Width and height, in pixels, of the buffer
    int width, height;

#define SCALE(X) (texture_type==GL_TEXTURE_2D?1.0:X)

    //! Width and height, suitable for texture coordinates, of the
    //! buffer. For GL_TEXTURE_2D this is always 1.0 x 1.0. For
    //! GL_TEXTURE_RECTANGLE_ARB it's the pixel sizes (as floats)
    //!
    //! This function knows about GL_TEXTURE_2D and
    //! GL_TEXTURE_RECTANGLE_ARB texture coordinates, but not any
    //! other ones.
    array(float) size()
    {
        return ({SCALE((float)width),SCALE((float)height)});
    }

    //! Draw the buffer using a GL_QUADS at x,y sized w,h. This
    //! function knows about GL_TEXTURE_2D and GL_TEXTURE_RECTANGLE_ARB
    //! texture coordinates, but not any other ones.
    void box(float x, float y, float w, float h)
    {
        glEnable(texture_type);
        glBindTexture( texture_type, texture );
        glColor(1.0,1.0,1.0,1.0);

        glBegin(GL_QUADS);
        glTexCoord(0.0,0.0);                  glVertex(x,y,0.0);
        glTexCoord(SCALE((float)width),0.0);  glVertex(w+x,y,0.0);
        glTexCoord(SCALE((float)width),SCALE((float)height));  glVertex(w+x,y+h,0.0);
        glTexCoord(0.0,SCALE((float)height)); glVertex(x,y+h,0.0);
        glEnd();

        glDisable(texture_type);
    }

    //! Bind the buffer to the currently active texture unit, then
    //! call the specified function.
    void draw_texture(function f )
    {
        glEnable(texture_type);
        glBindTexture( texture_type, texture );
        f();
        glDisable(texture_type);
    }

    //! Convenience function that binds the buffer as the currently
    //! active destination buffer, then calls @[f] and finally binds
    //! the default framebuffer (generally speaking the screen) as the
    //! active buffer again.
    //!
    //! This is equivalent to @[use] followed by @[f](@[args])
    //! followed by @[disable]
    //!
    //! This function is usually what is used to draw into a
    //! RenderBuffer.
    void draw(function f,mixed... args)
    {
        use();
        while(glGetError());
        f(@args);
        int err;
        if( (err = glGetError()) != 0 )
            werror("ERROR: %x\n%s", err, describe_backtrace(backtrace()));
        disable();
    }

    //! Restore the viewport and bind the screen as the active rendering buffer.
    void disable()
    {
        glPopAttrib();
        CHECK_ERROR(PopAttrib);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
    }

    //! Set the viewport to the size of the texture, and set this buffer
    //! as the currently active destination framebuffer.
    //!
    //! disable() must be called (exactly once) once you are done
    //! drawing into this buffer, or OpenGL will run out of attribute
    //! stack space eventually since the current viewport is pused to
    //! it.
    //!
    //! @[draw] will do the use() / disable() handling for you.
    void use()
    {
        CHECK_ERROR(PrePush);
        glPushAttrib(GL_VIEWPORT_BIT);
        CHECK_ERROR(PushAttrib);
        glViewport(0,0,width,height);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);
    }

    //! Create a new render buffer, with the size given by @[w] and
    //! @[h], the texture target will be @[type] (generally speaking
    //! GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE_ARB, the latter is
    //! needed if the size is not a power of two. This is not checked
    //! by the create function)
    //!
    //! @[depth] @[mipmap] and @[alpha] specifies if the corresponding
    //! extra buffers should be created.
    //! @[mipmap] is not supported for GL_TEXTURE_RECTANGLE_ARB
    //!
    //! If @[w] or @[h] is 0, the actual texture creation is postponed
    //! until @[resize] is called. The buffer will not be valid before
    //! it has a size.
    //!
    //! If @[internal_format] and @[format] are specified they are used to
    //! override the defaults (GL_RGB[A]16F_ARB and GL_RGB[A]
    //! respectively)
    //!
    //! Setting these also makes the buffer ignore the @[alpha] paramenter.
    //!
    //! The @[mipmap] parameter depends on the existance of the
    //! glGenerateMipmapEXT extension.
    protected void create(int w, int h, int type, bool depth,
			  bool mipmap, bool alpha,
			  int|void internal_format, int|void format)
    {
        if( mipmap && type == GL_TEXTURE_RECTANGLE_ARB )
            mipmap = false;
	this->alpha = alpha;
        this->mipmaps = mipmap;
        this->texture_type = type;
        this->internal_format = internal_format;
        this->format = format;
        [fbo]=glGenFramebuffersEXT(1);
        [texture]=glGenTextures(1);
        if( depth )
            [db]=glGenRenderbuffersEXT(1);
        CHECK_ERROR(RenderBuffer::create);
	if( w && h )
            resize(w,h);
    }


    //! Resize the buffer to a new size. Returns true if a new texture
    //! was created.
    bool resize(int w, int h )
    {
        if( width == w &&  height == h )
            return false;

        width = w;
        height = h;

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);
        glBindTexture(texture_type,texture);
	int ww = w;
	int hh = h;
        int mode, sm;
        if( internal_format )
        {
            mode = internal_format;
            sm = format;
        }
        else
        {
            mode = 0x8c3a;
            sm = GL_RGB;
            if( alpha )
            {
                mode = GL_RGBA16F_ARB;
                sm = GL_RGBA;
            }
        }
	glTexImage2DNoImage( texture_type, 0, mode, w, h, 0, sm, GL_FLOAT );
        glTexParameter(texture_type, GL_TEXTURE_MIN_FILTER, mipmaps?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);
        glTexParameter(texture_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameter(texture_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameter(texture_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameter(texture_type, GL_TEXTURE_BORDER_COLOR, ({0.0, 0.0, 0.0, 0.0}) );
        glFramebufferTexture2DEXT(GL_COLOR_ATTACHMENT0_EXT,texture_type,texture,0);
        if( db != -1  )
        {
            glBindRenderbufferEXT(db);
            glRenderbufferStorageEXT(GL_DEPTH_COMPONENT24,w,h);
            glFramebufferRenderbufferEXT(GL_DEPTH_ATTACHMENT_EXT, db);
        }
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
        CHECK_ERROR(RenderBuffer::resize);
        return true;
    }
}
#endif





class vec
//! A very basic vector class. Use @[vec2] / @[vec3] or @[vec4]
{

    Math.Matrix v;
    this_program dup() { return this_program(v); }
    mixed ARG(mixed x )
    {
	if( objectp(x) )
	{
	    if( x->v ) return x->v;
	    return x;
	}
	return x;
    }
    protected this_program `*( this_program x ) {
	return this_program(v*ARG(x));
    }
    protected this_program `+( this_program x ) {
	return this_program(v+ARG(x));
    }
    protected this_program `-( this_program x ) {
	return this_program(v-ARG(x));
    }
    protected this_program `/( this_program x ) {
	return this_program(v/ARG(x));
    }

    float dot( vec x )
    {
	return v->dot( x->v );
    }

    this_program cross( vec x )
    {
	return this_program(v->cross( x->v ));
    }

    protected this_program ``*( object x )
    {
	if( x->v ) x = x->v;
	return this_program( Math.Matrix(@column( (array)(x*v), 0 )) );
    }

    float length()
    {
	return v->norm();
    }

    this_program norm()
    {
	return this_program( v->normv() );
    }

    protected void create( Math.Matrix _v )
    {
	v = _v;
    }

    protected mixed `->( string x )
    {
	if( this[x] ) return this[x];
	switch( x )
	{
	    case "x": return ((array)v)[0][0];
	    case "y": return ((array)v)[0][1];
	    case "z": return ((array)v)[0][2];
	    case "q": return ((array)v)[0][3];
	    case "xy": return ((array)v)[0][..1];
	    case "xyz": return ((array)v)[0][..2];
	    case "xyzq": return ((array)v)[0][..3];
	}
    }
}

class vec3
//! A vector class somewhat similar to a GLSL vec3.
{

    inherit vec;

    protected void create( Math.Matrix|float x, float y, float z )
    {
	if( objectp( x ) )
	    ::create( x );
	else
	    v = Math.Matrix( ({ x, y, z }) );
    }
}

class vec4
//! A vector class somewhat similar to a GLSL vec4.
{

    inherit vec;

    protected void create( Math.Matrix|float x, float y, float z, float w )
    {
	if( objectp( x ) )
	    ::create( x );
	else
	    v = Math.Matrix( ({ x, y, z, w }) );
    }
}

class vec2
//! A vector class somewhat similar to a GLSL vec2.
{

    inherit vec;

    protected void create( Math.Matrix|float x, float y )
    {
	if( objectp( x ) )
	    ::create( x );
	else
	    v = Math.Matrix( ({ x, y }) );
    }
}
