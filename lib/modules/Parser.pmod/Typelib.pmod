#pike __REAL_VERSION__

//!
//! Parser for the typelib binary format.
//!
//! Most of the symbols here map straight to the corresponding
//! symbols in the gobject-introspection documentation.
//!
//! @note
//!   The binary format uses the native byte-order, so only files
//!   with the same byte-order as the pike binary are supprted.
//!
//! @note
//!   Work in progress. The API may change at any time.
//!
//! @seealso
//!   @[GI],
//!   @url{https://gnome.pages.gitlab.gnome.org/gobject-introspection/girepository/gi-GITypelib-Internals.html@}
//!

// This file contains experimental code...
#pragma no_experimental_warnings

// #define PARSER_TYPELIB_DEBUG

//! Magic cookie value for the @[Header()->magic] field.
constant G_IR_MAGIC = "GOBJ\nMETADATA\r\n\032";

//! Blob types.
//!
//! @seealso
//!   @[Blob]
enum GTypelibBlobType {
  BLOB_TYPE_INVALID = 0,
  BLOB_TYPE_FUNCTION,
  BLOB_TYPE_CALLBACK,
  BLOB_TYPE_STRUCT,
  BLOB_TYPE_BOXED,
  BLOB_TYPE_ENUM,
  BLOB_TYPE_FLAGS,
  BLOB_TYPE_OBJECT,
  BLOB_TYPE_INTERFACE,
  BLOB_TYPE_CONSTANT,
  __deprecated__ BLOB_TYPE_ERROR_DOMAIN,
  BLOB_TYPE_UNION,
}

// Typelib files are in native byte-order.
//! @decl private constant program(ADT.Struct.Item) guint8 = Byte
//!   8-bit unsigned integer.

//! @decl private constant program(ADT.Struct.Item) guint16
//!   16-bit unsigned integer in native byte-order.

//! @decl private constant program(ADT.Struct.Item) guint32
//!   32-bit unsigned integer in native byte-order.

#define guint8 Byte
#if __PIKE_BYTEORDER__ == 1234
#define guint16 Drow
#define guint32 Gnol
#else
#define guint16 Word
#define guint32 Long
#endif

//! @[ADT.Struct] with some extensions, and encoding disabled.
class Struct
{
  inherit ADT.Struct;

  string `pike_name()
  {
    string name = this->name;

    if (!name || !sizeof(name) || (name[0] > '9')) {
      return name;
    }
    name = this->gtype_name;
    if (name) return name;
    return "_" + this->name;
  }

  string read_cstring_at(Stdio.InputBlockFile file, int offset)
  {
    int pos = file->tell();
    file->seek(offset);

    string data = "";
    do {
      string frag = file->read(16);
      if (!frag || !sizeof(frag)) break;
      array(string) a = frag/"\0";
      data += a[0];
      if (sizeof(a) > 1) break;
    } while (1);

    file->seek(pos);

    return data;
  }

  //! @[Item] that is a file offset to a C-string.
  class gstring {
    inherit guint32;

    protected string str = "";

    void decode(Stdio.InputBlockFile file, mixed... state)
    {
      ::decode(file, @state);

      int offset = value;
      if (offset) {
        str = read_cstring_at(file, offset);
      } else {
        str = "";
      }
    }

    string get()
    {
      return str;
    }

    void set(int|string s)
    {
      if (intp(s)) {
        ::set(s);
      } else {
        str = s;
      }
    }

    protected string _sprintf(int c)
    {
      return sprintf("gstring(%d: %q)", value, str);
    }
  }

  protected string encode(mixed... state)
  {
    error("Encoding not supported.\n");
  }

  //! Render an XML representation of the object to a buffer.
  //!
  //! @param buf
  //!   Buffer to write to.
  //!
  //! @param indent
  //!   Indentation level in characters. This is typically increased
  //!   in increments of @expr{2@}.
  //!
  //! @param header
  //!   Header if known.
  //!
  //! @note
  //!   The default implementation just renders an XML comment
  //!   with a note that @tt{render_xml()@} is not implemented.
  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf("%*n<!-- %O->render_xml() not implemented! -->\n",
                 indent, object_program(this));
  }

  //! Render an Autodoc representation of the object to a buffer.
  //!
  //! @param buf
  //!   Buffer to write to.
  //!
  //! @param indent
  //!   Indentation level in characters. This is typically increased
  //!   in increments of @expr{2@}.
  //!
  //! @param header
  //!   Header if known.
  //!
  //! @note
  //!   The default implementation just renders an XML comment
  //!   with a note that @tt{render_autodoc()@} is not implemented.
  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf("%*n<!-- %O->render_autodoc() not implemented! -->\n",
                 indent, object_program(this));
  }

  //! Render an Autodoc type representation of the object to a buffer.
  //!
  //! @param buf
  //!   Buffer to write to.
  //!
  //! @param header
  //!   Header if known.
  //!
  //! @note
  //!   The default implementation just renders an XML comment
  //!   with a note that @tt{render_autodoc_type()@} is not implemented.
  void render_autodoc_type(String.Buffer buf, Header|void header)
  {
    buf->sprintf("<!-- %O->render_autodoc_type() not implemented! -->",
                 object_program(this));
  }
}

//! @[Struct] that may be annotated with attributes.
class Blob
{
  inherit Struct;

  //! Attributes set on this blob.
  array(this_program) attributes = ({});

  void low_decode(Stdio.InputBlockFile file,
                  mapping(int:array(this_program)) attribute_lookup,
                  mixed... state)
  {
    int file_offset = file->tell();
    if (!(attributes = attribute_lookup[file_offset])) {
      attributes = ({});
    }
    ::low_decode(file, attribute_lookup, @state);
  }
}

class AttributeBlob {
  inherit Blob;

  Item offset = guint32();
  Item name = gstring();
  Item value;

  void low_decode(Stdio.InputBlockFile file,
                  mapping(int:array(this_program)) attribute_lookup,
                  mixed... state)
  {
    ::low_decode(file, attribute_lookup, @state);
    if ((<"c:identifier">)[name->get()]) {
      value = gstring();
    } else {
      value = guint32();
    }
    names["value"] = value;
    value->decode(file, attribute_lookup, @state);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    string|int val = value->get();
    if (stringp(val)) {
      buf->sprintf("%*n<attribute name='%s' value='%s'/>\n",
                   indent, name->get(), val);
    } else {
      buf->sprintf("%*n<attribute name='%s' value='%d'/>\n",
                   indent, name->get(), val);
    }
  }
}

//!
__experimental__ class Header {
  inherit Struct;

  Item magic = Chars(16);
  //!

  Item major_version = Byte();
  Item minor_version = Byte();
  Item reserved = guint16();
  Item n_entries = guint16();
  array(DirEntry) entries = ({});
  Item n_local_entries = guint16();
  Item directory = guint32();
  Item n_attributes = guint32();
  Item attributes = guint32();
  array(AttributeBlob) attribute_array = ({});

  Item dependencies = gstring();

  Item size = guint32();
  Item namespace = gstring();
  Item nsversion = gstring();
  Item shared_library = gstring();
  Item c_prefix = gstring();

  Item entry_blob_size = guint16();
  Item function_blob_size = guint16();
  Item callback_blob_size = guint16();
  Item signal_blob_size = guint16();
  Item vfunc_blob_size = guint16();
  Item arg_blob_size = guint16();
  Item property_blob_size = guint16();
  Item field_blob_size = guint16();
  Item value_blob_size = guint16();
  Item attribute_blob_size = guint16();
  Item constant_blob_size = guint16();
  Item error_domain_blob_size = guint16();

  Item signature_blob_size = guint16();
  Item enum_blob_size = guint16();
  Item struct_blob_size = guint16();
  Item object_blob_size = guint16();
  Item interface_blob_size = guint16();
  Item union_blob_size = guint16();

  Item sections = guint32();
  array(object) section_array = ({});

  Item padding_0 = guint16();
  Item padding_1 = guint16();
  Item padding_2 = guint16();
  Item padding_3 = guint16();
  Item padding_4 = guint16();
  Item padding_5 = guint16();

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    int pos = file->tell();

    attribute_array = ({});
    mapping(int:array(AttributeBlob)) attribute_lookup = ([]);
    file->seek(attributes->get());
    for (int i = 0; i < n_attributes->get(); i++) {
      AttributeBlob attr = AttributeBlob(file, attribute_lookup, @state);
      attribute_array += ({ attr });
      attribute_lookup[attr->offset] += ({ attr });
    }

    entries = ({});
    file->seek(directory->get());
    for (int i = 0; i < n_entries->get(); i++) {
      entries += ({ DirEntry(file, attribute_lookup, @state) });
    }

    file->seek(pos);
  }

  protected string _sprintf(int c)
  {
    return sprintf("%s: entries: %O\n", ::_sprintf(c), entries);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf(#"\
%*n<repository version='1.0'
%*n            xmlns='http://www.gtk.org/introspection/core/1.0'
%*n            xmlns:c='http://www.gtk.org/introspection/c/1.0'
%*n            xmlns:glib='http://www.gtk.org/introspection/glib/1.0'>
", indent, indent, indent, indent);
    foreach(dependencies->get()/"|", string dep) {
      if (sizeof(dep)) {
        array(string) a = dep/"-";
        buf->sprintf("%*n<include name='%s' version='%s'/>\n",
                     indent + 2,
                     a[0], (sizeof(a) > 1)?a[1]:"");
      }
    }
    buf->sprintf("%*n<namespace name='%s'"
                 " version='%s'",
                 indent + 2,
                 namespace->get(), nsversion->get());
    if (sizeof(c_prefix->get())) {
      buf->sprintf(" c:identifier-prefixes='%s'", c_prefix->get());
      buf->sprintf(" c:prefix='%s'", c_prefix->get());
    }
    if (sizeof(shared_library->get())) {
      buf->sprintf(" shared-library='%s'", shared_library->get());
    }
    buf->add(">\n");
    foreach(entries, object entry) {
      entry->render_xml(buf, indent + 4, this);
    }
#ifdef PARSER_TYPELIB_DEBUG
    if (sizeof(attribute_array)) {
      buf->sprintf("%*n<attributes>\n", indent + 4);
      foreach(attribute_array, object attr) {
        attr->render_xml(buf, indent + 6, this);
      }
      buf->sprintf("%*n</attributes>\n", indent + 4);
    }
#endif
    buf->sprintf(#"\
%*n</namespace>
%*n</repository>
", indent + 2, indent);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf(#"\
%*n<autodoc>
%*n  <namespace name='predef'>
%*n    <module name='GI'>
%*n      <module name='repository'>
", indent, indent, indent, indent);
    indent += 8;
    int save_indent = indent;
    foreach(nsversion->get()/"." - ({ "" }), string v) {
      buf->sprintf("%*n<module name='%s'>\n", indent, v);
      indent += 2;
    }
    buf->sprintf("%*n<module name='%s'>\n", indent, namespace->get());
    indent += 2;

    // Add some automatic symbols.
    buf->sprintf(#"\
%*n<docgroup homogen-name='__GI_API_VERSION__' homogen-type='constant'>
%*n  <doc><text><p>This constant contains a string with the version number
%*n      of the <ref>%s</ref> module (in this case <expr>%q</expr>).</p>
%*n    </text>
%*n    <group><seealso/><text><p><ref>__GI_API_VERSION_%s__</ref></p>
%*n  </text></group></doc>
%*n  <constant name='__GI_API_VERSION__'>
%*n    <type><string><min>0</min><max>255</max></string></type>
%*n  </constant>
%*n</docgroup>
",
                 indent, indent, indent, namespace->get(), nsversion->get(),
                 indent, indent, replace(nsversion->get(), ".", "_"),
                 indent, indent, indent, indent, indent);
    buf->sprintf(#"\
%*n<docgroup homogen-name='__GI_API_VERSION_%s__' homogen-type='constant'>
%*n  <doc><text><p>This presence of this constant indicates that this is
%*n      version %s of the <ref>%s</ref> module.</p>
%*n    </text>
%*n    <group><seealso/><text><p><ref>__GI_API_VERSION__</ref></p>
%*n  </text></group></doc>
%*n  <constant name='__GI_API_VERSION_%s__'>
%*n    <type><int><min>1</min><max>1</max></int></type>
%*n  </constant>
%*n</docgroup>
",
                 indent, replace(nsversion->get(), ".", "_"),
                 indent, indent, nsversion->get(), namespace->get(),
                 indent, indent,
                 indent, indent, replace(nsversion->get(), ".", "_"),
                 indent, indent, indent);

    foreach(dependencies->get()/"|", string dep) {
      if (!sizeof(dep)) continue;
      array(string) a = dep/"-";
      string sym = "predef::GI.repository.";
      if (sizeof(a) > 1) {
        sym += a[1..] * "." + ".";
      }
      buf->sprintf(#"\
%*n<docgroup homogen-name='%s' homogen-type='import'>
%*n  <import name='%s' symbol='1' recurse='1'><classname>%s</classname></import>
%*n</docgroup>
",
                   indent, a[0],
                   indent, a[0], sym + a[0],
                   indent);
    }

    entries->render_autodoc(buf, indent, this);
    while (indent > save_indent) {
      indent -= 2;
      buf->sprintf("%*n</module>\n", indent);
    }
    indent -= 8;
    buf->sprintf(#"\
%*n      </module>
%*n    </module>
%*n  </namespace>
%*n</autodoc>
", indent, indent, indent, indent);
  }
}

enum SectionType {
  GI_SECTION_END = 0,
  GI_SECTION_DIRECTORY_INDEX
}

class Section {
  inherit Struct;

  Item section_id = guint32();
  Item offset = guint32();
}

class DirEntry {
  inherit Struct;

  Item blob_type = guint16();

  Item local_reserved = guint16();
  Item name = gstring();

  Item offset;

  Item blob;

  string `pike_name()
  {
    if (blob) return blob->pike_name;
    return ::`pike_name();
  }

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    m_delete(names, "blob");
    if (local_reserved->get() & 0x0001) {
      offset = guint32();
      offset->decode(file);
      names["offset"] = offset;

      if (offset->get()) {
        int pos = file->tell();
        file->seek(offset->get());
        blob = blob_factory(file, blob_type->get(), @state);
        file->seek(pos);
        names["blob"] = blob;
      } else {
        blob = UNDEFINED;
      }
    } else {
      offset = gstring();
      offset->decode(file);
      names["offset"] = offset;
      blob = UNDEFINED;
    }
  }

  protected string _sprintf(int c)
  {
    return sprintf("%s: name: %O, blob: %O\n",
                   ::_sprintf(c),
                   name->get(), blob);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    if (blob) {
      blob->render_xml(buf, indent, header);
    } else {
      string|int off = offset->get();
      if (stringp(off) && sizeof(off)) {
#ifdef PARSER_TYPELIB_DEBUG
        buf->sprintf("%*n<record name='%s' namespace='%s'>\n",
                     indent, name->get(), off);
        buf->sprintf("%*n</record>\n", indent);
#endif
      } else {
        buf->sprintf("%*n<record name='%s'>\n", indent, name->get());
        buf->sprintf("%*n</record>\n", indent);
      }
    }
  }

  string get_gi_name(Header header)
  {
    string res = name->get();
    string ns = offset->get();
    if (stringp(ns)) {
      res = ns + "." + res;
    }
    return res;
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    if (blob) {
      blob->render_autodoc(buf, indent, header);
    }
  }

  string get_pike_name(Header header)
  {
    string res = pike_name;
    string ns = offset->get();
    if (stringp(ns)) {
      res = ns + "." + res;
      foreach(header->dependencies/"|", string dep) {
        if (dep == ns) break;	// Unversioned.
        if (has_prefix(dep, ns + "-")) {
          res = "predef::GI.repository." + (dep/"-")[-1] + "." + res;
          break;
        }
      }
    }
    return res;
  }
}

Blob blob_factory(object file, GTypelibBlobType blob_type, mixed... state)
{
  program(Blob) blob_program =
    ([
      // BLOB_TYPE_INVALID: UNDEFINED,
      BLOB_TYPE_FUNCTION: FunctionBlob,
      BLOB_TYPE_CALLBACK: CallbackBlob,
      BLOB_TYPE_STRUCT: StructBlob,
      // BLOB_TYPE_BOXED: StructOrUnionBlob,
      BLOB_TYPE_ENUM: EnumBlob,
      BLOB_TYPE_FLAGS: EnumBlob,
      BLOB_TYPE_OBJECT: ObjectBlob,
      BLOB_TYPE_INTERFACE: InterfaceBlob,
      BLOB_TYPE_CONSTANT: ConstantBlob,
      // __deprecated__ BLOB_TYPE_ERROR_DOMAIN,
      BLOB_TYPE_UNION: UnionBlob,
    ])[blob_type];
  if (!blob_program) {
    return UNDEFINED;
  }

  return blob_program(file, @state);
}

class SimpleTypeBlob {
  inherit Blob;

  Item flags = guint32();

  Blob blob;

  void low_decode(Stdio.InputBlockFile file,
                  mapping(int:array(this_program)) attribute_lookup,
                  mixed... state)
  {
    ::low_decode(file, attribute_lookup, @state);

    if (flags->get() & 0x00ffffff) {
      // Pointer to type blob.
      int pos = file->tell();
      werror("POS: 0x%04x\n", pos - 4);
      werror("SimpleType: 0x%08x\n", flags->get());
      file->seek(flags->get());
      string c = file->read(1);
      werror("C: %O\n", c);
      file->seek(flags->get());
      int tag = c[0]>>3;
      werror("TAG: %O\n", tag);
      switch(tag) {
        // Non-basic types:
      case GI_TYPE_TAG_ARRAY:
        blob = ArrayTypeBlob();
        break;
      case GI_TYPE_TAG_INTERFACE:
        blob = InterfaceTypeBlob();
        break;
      case GI_TYPE_TAG_GLIST:
      case GI_TYPE_TAG_GSLIST:
      case GI_TYPE_TAG_GHASH:
      case GI_TYPE_TAG_ERROR:
        // FIXME: Not supported yet.
        break;
      }
      if (blob) {
        blob->decode(file, attribute_lookup, @state);

        werror("TypeBlob: %O\n", blob);
      }
      file->seek(pos);
    }
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- SimpleType, flags: 0x%08x -->\n",
                 indent, flags->get());
#endif
    if (blob) {
      blob->render_xml(buf, indent, header);
    } else {
      int tag = flags->get() >> 27;
      buf->sprintf("%*n<type name='%s'/>\n",
                   indent,
                   TypeTagNameLookup[tag] || ("&lt;" + tag + "&gt;"));
    }
  }

  void render_autodoc_type(String.Buffer buf, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("<!-- SimpleType, flags: 0x%08x -->", flags->get());
#endif
    if (blob) {
      blob->render_autodoc_type(buf, header);
    } else {
      int tag = flags->get() >> 27;
      buf->add(TypeTagPikeNameLookup[tag] || ("<!-- " + tag + " -->"));
    }
  }
}

class ArgBlob {
  inherit Blob;

  Item name = gstring();
  Item flags = guint32();
  Item closure_arg = Byte();
  Item destroy_arg = Byte();

  Item padding = guint16();

  SimpleTypeBlob arg_type = SimpleTypeBlob();

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- ArgBlob, flags: 0x%08x -->\n",
                 indent, flags->get());
#endif
    int f = flags->get();

    buf->sprintf("%*n<parameter name='%s' transfer-ownership='%s'",
                 indent,
                 name->get(),
                 (f & 0x20) ? "full" : "none");
    if (f & 0x00000700) {
      int scope = (f & 0x00000700)>>8;
      buf->sprintf(" scope='%s'",
                   ([
                     1: "call",
                     /* 2: "destroy", */ // ???
                     3: "notified",
                   ])[scope] || (string)scope);
    }
    if (f & 0x00000006) {
      if (f & 0x00000001) { // in
        buf->add(" direction='in'");
      }
      if (f & 0x00000002) { // out
        buf->add(" direction='out'");
      }
      buf->sprintf(" caller-allocates='%d'", !!(f & 0x00000004));
    }
    if (flags->get() & 0x00000008) { // nullable
      buf->add(" allow-none='1'");
    }
    // FIXME: nullable, allow-none, introspectable
    if (closure_arg->get() != 0xff) {
      buf->sprintf(" closure='%d'", closure_arg->get());
    }
    if (destroy_arg->get() != 0xff) {
      buf->sprintf(" destroy='%d'", destroy_arg->get());
    }
    // FIXME: scope, direction, caller-allocates, optional, skip
    // FIXME: TransferOwnership
    buf->add(">\n");
    arg_type->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</parameter>\n", indent);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header,
                      int(0..2)|void closure_type)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- ArgBlob, flags: 0x%08x -->\n",
                 indent, flags->get());
#endif
    int f = flags->get();

    buf->sprintf("%*n<argument name='%s'>\n", indent, pike_name);
    buf->sprintf("%*n  <type>", indent);

    if (!closure_type) {
      if (flags->get() & 0x00000008) { // nullable
        buf->add("<or><void/>");
        arg_type->render_autodoc_type(buf, header);
        buf->add("</or>");
      } else {
        arg_type->render_autodoc_type(buf, header);
      }
    } else if (closure_type == 1) {
      buf->add("<array><valuetype><mixed/></valuetype></array>");
    } else { // closure_type == 2.
      buf->add("<varargs><mixed/></varargs>");
    }
    buf->sprintf("</type>\n"
                 "%*n</argument>\n", indent);
  }

  void render_autodoc_type(String.Buffer buf, Header|void header,
                           int(0..2)|void closure_type)
  {
    int f = flags->get();

    buf->add("<argtype>");

    if (!closure_type) {
      if (flags->get() & 0x00000008) { // nullable
        buf->add("<or><void/>");
        arg_type->render_autodoc_type(buf, header);
        buf->add("</or>");
      } else {
        arg_type->render_autodoc_type(buf, header);
      }
    } else if (closure_type == 1) {
      buf->add("<array><valuetype><mixed/></valuetype></array>");
    } else { // closure_type == 2.
      buf->add("<varargs><mixed/></varargs>");
    }

    buf->add("</argtype>");
  }
}

enum GITypeTag {
  GI_TYPE_TAG_VOID =  0,
  GI_TYPE_TAG_BOOLEAN,
  GI_TYPE_TAG_INT8,
  GI_TYPE_TAG_UINT8,
  GI_TYPE_TAG_INT16,
  GI_TYPE_TAG_UINT16,
  GI_TYPE_TAG_INT32,
  GI_TYPE_TAG_UINT32,
  GI_TYPE_TAG_INT64,
  GI_TYPE_TAG_UINT64,
  GI_TYPE_TAG_FLOAT,
  GI_TYPE_TAG_DOUBLE,
  GI_TYPE_TAG_GTYPE,
  GI_TYPE_TAG_UTF8,
  GI_TYPE_TAG_FILENAME,
  GI_TYPE_TAG_ARRAY,
  GI_TYPE_TAG_INTERFACE,
  GI_TYPE_TAG_GLIST,
  GI_TYPE_TAG_GSLIST,
  GI_TYPE_TAG_GHASH,
  GI_TYPE_TAG_ERROR,
  GI_TYPE_TAG_UNICHAR,
}

protected constant TypeTagNameLookup = ([
  GI_TYPE_TAG_VOID: "any",	// Note: "none" for return values.
  GI_TYPE_TAG_BOOLEAN: "gboolean",
  GI_TYPE_TAG_INT8: "gint8",
  GI_TYPE_TAG_UINT8: "guint8",
  GI_TYPE_TAG_INT16: "gint16",
  GI_TYPE_TAG_UINT16: "guint16",
  GI_TYPE_TAG_INT32: "gint32",
  GI_TYPE_TAG_UINT32: "guint32",
  GI_TYPE_TAG_INT64: "gint64",
  GI_TYPE_TAG_UINT64: "guint64",
  GI_TYPE_TAG_FLOAT: "float",
  GI_TYPE_TAG_DOUBLE: "double",
  GI_TYPE_TAG_GTYPE: "gtype",
  GI_TYPE_TAG_UTF8: "utf8",
  GI_TYPE_TAG_FILENAME: "filename",
  GI_TYPE_TAG_ARRAY: "array",
  GI_TYPE_TAG_INTERFACE: "interface",
  GI_TYPE_TAG_GLIST: "glist",
  GI_TYPE_TAG_GSLIST: "gslist",
  GI_TYPE_TAG_GHASH: "ghash",
  GI_TYPE_TAG_ERROR: "error",
  GI_TYPE_TAG_UNICHAR: "unichar",
]);

protected constant TypeTagPikeNameLookup = ([
  GI_TYPE_TAG_VOID: "<mixed/>",	// Note: "void" for return values.
  GI_TYPE_TAG_BOOLEAN: "<int><min>0</min><max>1</max></int>",
  GI_TYPE_TAG_INT8: "<int><min>-128</min><max>127</max></int>",
  GI_TYPE_TAG_UINT8: "<int><min>0</min><max>255</max></int>",
  GI_TYPE_TAG_INT16: "<int><min>-32768</min><max>32767</max></int>",
  GI_TYPE_TAG_UINT16: "<int><min>0</min><max>65535</max></int>",
  GI_TYPE_TAG_INT32: "<int><min/><max/></int>",
  GI_TYPE_TAG_UINT32: "<int><min>0</min><max/></int>",
  GI_TYPE_TAG_INT64: "<int><min/><max/></int>",
  GI_TYPE_TAG_UINT64: "<int><min>0</min><max/></int>",
  GI_TYPE_TAG_FLOAT: "<float/>",
  GI_TYPE_TAG_DOUBLE: "<float/>",
  GI_TYPE_TAG_GTYPE: "<mixed/>",
  GI_TYPE_TAG_UTF8: "<object>utf8_string</object>",
  GI_TYPE_TAG_FILENAME: "<object>utf8_string</object>",
  GI_TYPE_TAG_ARRAY: "<array/>",
  GI_TYPE_TAG_INTERFACE: "<program/>",
  GI_TYPE_TAG_GLIST: "<!-- GList -->",
  GI_TYPE_TAG_GSLIST: "<!-- GSList -->",
  GI_TYPE_TAG_GHASH: "<mapping/>",
  GI_TYPE_TAG_ERROR: "<!-- Error -->",
  GI_TYPE_TAG_UNICHAR: "<int><min>0</min><max/></int>",
]);

class SignatureBlob {
  inherit Blob;

  SimpleTypeBlob return_type = SimpleTypeBlob();

  Item flags = guint16();

  Item n_arguments = guint16();

  array(ArgBlob) arguments = ({});

  void low_decode(Stdio.InputBlockFile file,
                  mapping(int:array(this_program)) attribute_lookup,
                  mixed... state)
  {
    ::low_decode(file, attribute_lookup, @state);

    arguments = ({});
    for (int i = 0; i < n_arguments->get(); i++) {
      arguments += ({ ArgBlob(file, attribute_lookup, @state) });
    }
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- Signature Flags: 0x%04x -->\n",
                 indent, flags->get());
#endif
    buf->sprintf("%*n<return-value transfer-ownership='%s'",
                 indent,
                 (flags->get() & 0x0002)? "full": "none");
    if (flags->get() & 0x01) { // may_return_null
      buf->add(" allow-none='1'");
    }
    // FIXME: introspectable, nullable, closure, destroy, skip, allow-none.
    buf->add(">\n");
    if (!return_type->blob && !(return_type->flags >> 27)) {
      // Special case to avoid type='any'.
      buf->sprintf("%*n<type name='none'/>\n", indent + 2);
    } else {
      return_type->render_xml(buf, indent + 2, header);
    }
    buf->sprintf("%*n</return-value>\n", indent);
    if (sizeof(arguments)) {
      buf->sprintf("%*n<parameters>\n", indent);
      foreach(arguments, ArgBlob arg) {
        arg->render_xml(buf, indent + 2, header);
      }
      buf->sprintf("%*n</parameters>\n", indent);
    }
  }

  array(SimpleTypeBlob|ArgBlob|array(ArgBlob)) get_pike_signature()
  {
    array(SimpleTypeBlob|ArgBlob|array(ArgBlob)|zero) pike_signature =
      ({ UNDEFINED }) + arguments;
    foreach(arguments; int i; ArgBlob arg) {
      if (!pike_signature[i + 1]) continue;
      if (arg->flags & 0x00000002) { // OUT
        if (!pike_signature[0]) {
          // This will be the return value.
          pike_signature[0] = arg->arg_type;
        }
        if (!(arg->flags & 0x00000001)) { // !IN
          // Zap pure out parameters.
          pike_signature[i + 1] = UNDEFINED;
          continue;
        }
      }
      if (arg->flags & 0x00000700) {
        // Scope (ie a function argument).
        if (arg->closure_arg != 0xff) {
          // Mark closure argument as multi.
          pike_signature[arg->closure_arg+1] =
            ({ pike_signature[arg->closure_arg+1] });
        }
        if (arg->destroy_arg != 0xff) {
          // Mark destroy argument as implicit.
          pike_signature[arg->destroy_arg+1] = UNDEFINED;
        }
      } else {
        if (arg->arg_type->blob &&
            (((arg->arg_type->blob->pointer_and_tag >> 3) & 0x1f) ==
             GI_TYPE_TAG_ARRAY)) {
          if ((arg->arg_type->blob->pointer_and_tag & 0x0200) &&
              (arg->arg_type->blob->length != 0xffff)) {
            // Mark array length argument as implicit.
            pike_signature[arg->arg_type->blob->length+1] = UNDEFINED;
          }
        }
      }
    }
    if (!pike_signature[0]) {
      pike_signature[0] = return_type;
    }

    return pike_signature - ({ UNDEFINED });
  }

  // This renders the signature as a method entry.
  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- Signature Flags: 0x%04x -->\n",
                 indent, flags->get());
#endif
    array pike_signature = get_pike_signature();
    if (sizeof(pike_signature) > 1) {
      buf->sprintf("%*n<arguments>\n", indent);
      foreach(pike_signature[1..<1], array(ArgBlob)|ArgBlob arg) {
        if (objectp(arg)) {
          arg->render_autodoc(buf, indent + 2, header);
        } else {
          arg[0]->render_autodoc(buf, indent + 2, header, 1);
        }
      }
      // Special case for many argument.
      if (objectp(pike_signature[-1])) {
        pike_signature[-1]->render_autodoc(buf, indent + 2, header);
      } else {
        pike_signature[-1][0]->render_autodoc(buf, indent + 2, header, 2);
      }
      buf->sprintf("%*n</arguments>\n", indent);
    } else {
      buf->sprintf("%*n<arguments/>\n", indent);
    }

    buf->sprintf("%*n<returntype>", indent);
    SimpleTypeBlob rt = pike_signature[0];
    if (!rt->blob && !(rt->flags >> 27)) {
      // void/any.
      buf->add("<void/>");
    } else if (flags->get() & 0x01) {
      // may_return_null
      buf->add("<or>");
      rt->render_autodoc_type(buf, header);
      buf->add("<zero/></or>");
    } else {
      rt->render_autodoc_type(buf, header);
    }
    buf->add("</returntype>\n");
  }

  // Rendering as a type (ie function(...:...)
  void render_autodoc_type(String.Buffer buf, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- Signature Flags: 0x%04x -->\n",
                 indent, flags->get());
#endif
    buf->add("<function>");

    array pike_signature = get_pike_signature();
    if (sizeof(pike_signature) > 1) {
      foreach(pike_signature[1..<1], array(ArgBlob)|ArgBlob arg) {
        if (objectp(arg)) {
          arg->render_autodoc_type(buf, header);
        } else {
          arg[0]->render_autodoc_type(buf, header, 1);
        }
      }
      // Special case for many argument.
      if (objectp(pike_signature[-1])) {
        pike_signature[-1]->render_autodoc_type(buf, header);
      } else {
        pike_signature[-1][0]->render_autodoc_type(buf, header, 2);
      }
    }

    buf->add("<returntype>");
    SimpleTypeBlob rt = pike_signature[0];
    if (!rt->blob && !(rt->flags >> 27)) {
      // Special case to avoid type='any'.
      buf->add("<void/>");
    } else {
      if (flags->get() & 0x01) { // may_return_null
        buf->add("<or><zero/>");
      }
      // FIXME: introspectable, nullable, closure, destroy, skip, allow-none.
      rt->render_autodoc_type(buf, header);
      if (flags->get() & 0x01) { // may_return_null
        buf->add("</or>");
      }
    }
    buf->add("</returntype></function>");
  }
}

class CommonBlob {
  inherit Blob;

  Item blob_type = guint16();

  Item deprecated_flags = guint16();
  Item name = gstring();
}

class FunctionBlob {
  inherit Blob;

  Item blob_type = guint16();

  Item flags = guint16();

  Item name = gstring();
  Item symbol = gstring();
  Item signature_offset = guint32();

  Item more_flags = guint16();

  Item finish = guint16();

  SignatureBlob signature;

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    int pos = file->tell();
    file->seek(signature_offset->get());
    signature = SignatureBlob(file, @state);
    file->seek(pos);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- Function: flags: 0x%04x, name: %q, sym: %q,\n"
                 "%*n     sign: %d, more: 0x%04x, fin: 0x%04x -->\n",
                 indent, flags->get(), name->get(), symbol->get(),
                 indent, signature_offset->get(), more_flags->get(), finish->get());
#endif
    int fl = flags->get();
    int mfl = more_flags->get();
    string tag = "method";
    if (fl & 0x0008) {
      tag = "constructor";
    } else if (mfl & 0x0001) {
      tag = "function";
    }
    buf->sprintf("%*n<%s name='%s' c:identifier='%s'",
                 indent, tag, name->get(), symbol->get());
    if (fl & 0x0001) {
      buf->add(" deprecated='1'");
    }
    if (fl & 0x0020) {
      buf->add(" throws='1'");
    }
    // FIXME: glib:async-func?
    buf->add(">\n");
    signature->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</%s>\n", indent, tag);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- Function: flags: 0x%04x, name: %q, sym: %q,\n"
                 "%*n     sign: %d, more: 0x%04x, fin: 0x%04x -->\n",
                 indent, flags->get(), pike_name, symbol->get(),
                 indent, signature_offset->get(), more_flags->get(), finish->get());
#endif
    int fl = flags->get();
    int mfl = more_flags->get();
    buf->sprintf(#"\
%*n<docgroup homogen-name='%s' homogen-type='method'>
%*n  <method keep='' name='%s'>
",
                 indent, pike_name,
                 indent, pike_name);
    if (fl & 0x0001) {
      buf->sprintf("%*n<attributes><attribute><prefix/>"
                   "<attribute>\"deprecated\"</attribute>"
                   "</attribute></attributes>\n", indent + 4);
    }
    signature->render_autodoc(buf, indent + 4, header);
    buf->sprintf(#"\
%*n  </method>
%*n</docgroup>
",
                 indent, indent);
  }
}

class CallbackBlob {
  inherit Blob;

  Item blob_type = guint16();

  Item flags = guint16();
  Item name = gstring();
  Item signature_offset = guint32();

  SignatureBlob signature;

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    int pos = file->tell();
    file->seek(signature_offset->get());
    signature = SignatureBlob(file, @state);
    file->seek(pos);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf("%*n<callback name='%s'>\n",
                 indent, name->get());
    signature->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</callback>\n", indent);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf(#"\
%*n<docgroup homogen-name='%s' homogen-type='typedef'>
%*n  <typedef name='%s'><type>",
                 indent, pike_name,
                 indent, pike_name);
    signature->render_autodoc_type(buf, header);
    buf->sprintf(#"</type></typedef>
%*n</docgroup>
", indent);
  }
}

class InterfaceTypeBlob {
  inherit Blob;

  Item pointer_and_tag = Byte();
  Item reserved2 = Byte();
  Item interface = guint16();

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    DirEntry blob = header->entries[interface->get()-1];
    if (stringp(blob->offset)) {
      buf->sprintf("%*n<type name='%s.%s'/>\n",
                   indent, blob->offset, blob->name);
    } else {
      buf->sprintf("%*n<type name='%s'/>\n", indent, blob->name);
    }
  }

  void render_autodoc_type(String.Buffer buf, Header|void header)
  {
    DirEntry entry = header->entries[interface->get()-1];
    string name = entry->pike_name;
    if (stringp(entry->offset)) {
      name = entry->offset + "." + name;
      foreach(header->dependencies/"|", string dep) {
        if (dep == entry->offset) break;	// Unversioned.
        if (has_prefix(dep, entry->offset + "-")) {
          name = "predef::GI.repository." + (dep/"-")[-1] + "." + name;
          break;
        }
      }
    }
    buf->sprintf("<object>%s</object>", name);
  }
}

class ArrayTypeBlob {
  inherit Blob;

  Item pointer_and_tag = guint16();

  Item length = guint16();

  SimpleTypeBlob type = SimpleTypeBlob();

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf("%*n<array", indent);
    if (pointer_and_tag->get() & 0x0100) {
      buf->add(" zero-terminated='1'");
    }
    if (length->get() != 0xffff) {
      if (pointer_and_tag->get() & 0x0200) {
        buf->sprintf(" length='%d'", length->get());
      } else {
        buf->sprintf(" fixed-size='%d'", length->get());
      }
    }
    buf->add(">\n");
    type->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</array>\n", indent);
  }

  void render_autodoc_type(String.Buffer buf, Header|void header)
  {
    buf->add("<array>");
    if ((pointer_and_tag->get() & 0x0400) &&
        (length->get() != 0xffff)) {
      // Array of fixed size.
      buf->sprintf("<length><int><min>%d</min><max>%d</max></int></length>",
                   length->get(), length->get());
    }
    buf->add("<valuetype>");
    type->render_autodoc_type(buf, header);
    buf->add("</valuetype></array>");
  }
}

class ParamTypeBlob {
  inherit Blob;

  Item pointer_and_tag = Byte();

  Item reserved2 = Byte();
  Item n_types = guint16();

  SimpleTypeBlob type = SimpleTypeBlob();
}

class ErrorTypeBlob {
  inherit Blob;

  Item pointer_and_tag = Byte();

  Item reserved2 = Byte();

  Item n_domains = guint16();	// Must be 0!
}

class ValueBlob {
  inherit Blob;

  Item flags = guint32();
  Item name = gstring();
  Item value = guint32();

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    int pos = file->tell();
    file->seek(pos);
    ::low_decode(file, @state);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    int val = value->get();
    if (!(flags->get() & 0x00000002)) {
      // Signed value.
      if (val & 0x80000000) {
        val -= 0x100000000;
      }
    }
    // FIXME: How to mark deprecated values?
    buf->sprintf("%*n<member name='%s' value='%d'>\n",
                 indent, name->get(), val);
    // FIXME: <attribute name="c:identifier" value="XXXX_XXXX"/>
    attributes->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</member>\n", indent);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    int val = value->get();
    if (!(flags->get() & 0x00000002)) {
      // Signed value.
      if (val & 0x80000000) {
        val -= 0x100000000;
      }
    }
    // FIXME: How to mark deprecated values?
    buf->sprintf(#"\
%*n<docgroup homogen-name='%s' homogen-type='constant'>
%*n  <doc><text/></doc>
%*n  <constant name='%s'/>
%*n</docgroup>
",
                 indent, pike_name,
                 indent,
                 indent, pike_name,
                 indent);
  }
}

class FieldBlob {
  inherit Blob;

  Item name = gstring();

  Item flags = Byte();
  Item bits = Byte();

  Item struct_offset = guint16();

  Item reserved2 = guint32();

  SimpleTypeBlob type = SimpleTypeBlob();

  Blob embedded_type;

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    if (flags->get() & 0x04) { // has_embedded_type
      embedded_type = blob_factory(file, type->flags, @state);
    }
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf("%*n<field name='%s'",
                 indent, name->get());
    if (flags->get() & 2) {
      buf->add(" writeable='1'");
    }
    buf->add(">\n");
    (embedded_type || type)->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</field>\n", indent);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf(#"\
%*n<docgroup homogen-name='%s' homogen-type='variable'>
%*n  <doc><text/></doc>
%*n  <variable name='%s'>
%*n    <type>",
                 indent, pike_name,
                 indent,
                 indent, pike_name,
                 indent);
#if 0
    // FIXME: Read-only, etc.
    if (flags->get() & 2) {
      buf->add(" writeable='1'");
    }
#endif
    (embedded_type || type)->render_autodoc_type(buf, header);
    buf->sprintf(#"\
</type>
%*n  </variable>
%*n</docgroup>
",
                 indent, indent);
  }
}

class RegisteredTypeBlob {
  inherit Blob;

  Item blob_type = guint16();
  Item flags = guint16();
  Item name = gstring();

  Item gtype_name = gstring();
  Item gtype_init = gstring();
}

class StructBlob {
  inherit Blob;

  Item blob_type = guint16();
  Item flags = guint16();

  Item name = gstring();

  Item gtype_name = gstring();
  Item gtype_init = gstring();

  Item size = guint32();

  Item n_fields = guint16();
  array(FieldBlob) fields = ({});
  Item n_methods = guint16();
  array(FunctionBlob) methods = ({});

  Item copy_func = gstring();
  Item free_func = gstring();

  void low_decode(Stdio.InputBlockFile file,
                  mapping(int:array(AttributeBlob)) attribute_lookup,
                  mixed... state)
  {
    ::low_decode(file, attribute_lookup, @state);

    fields = ({});
    for (int i = 0; i < n_fields->get(); i++) {
      fields += ({ FieldBlob(file, attribute_lookup, @state) });
    }

    methods = ({});
    for (int i = 0; i < n_methods->get(); i++) {
      methods += ({ FunctionBlob(file, attribute_lookup, @state) });
    }
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- StructBlob, flags: 0x%04x -->\n",
                 indent, flags->get());
#endif
    string tag = "record";
    if (sizeof(gtype_name->get()) || sizeof(gtype_init->get())) {
      buf->sprintf("%*n<glib:boxed glib:name='%s'"
                   " glib:type-name='%s' glib:get-type='%s'",
                   indent, name->get(),
                   gtype_name->get(), gtype_init->get());
      tag = "glib:boxed";
    } else {
      buf->sprintf("%*n<%s name='%s'", indent, tag, name->get());
    }
    if (flags->get() & 0x0004) { // is_gtype_struct
      buf->add(" glib:is-gtype-struct='1'");
    }
    if (flags->get() & 0x0001) { // deprecated
      buf->add(" deprecated='1'");
    }
    if (n_fields->get() || n_methods->get()) {
      buf->add(">\n");
#ifdef PARSER_TYPELIB_DEBUG
      buf->sprintf("%*n<!-- Num fields: %d -->\n",
                   indent + 2, n_fields->get());
#endif
      foreach(fields, FieldBlob field) {
        field->render_xml(buf, indent + 2, header);
      }
#ifdef PARSER_TYPELIB_DEBUG
      buf->sprintf("%*n<!-- Num methods: %d -->\n",
                   indent + 2, n_methods->get());
#endif
      foreach(methods, FunctionBlob method) {
        method->render_xml(buf, indent + 2, header);
      }
      buf->sprintf("%*n</%s>\n", indent, tag);
    } else {
      buf->add("/>\n");
    }
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- StructBlob, flags: 0x%04x -->\n",
                 indent, flags->get());
#endif

    //
    // First: Consolidate any constructors in a Factory module.
    //
    array(FunctionBlob) constructors =
      filter(methods, lambda(FunctionBlob f) {
        if (f->flags & 0x0008) {
          return 1;
        }
        return 0;
      });
    if (sizeof(constructors)) {
      buf->sprintf("%*n<module name='%s_Factory' keep=''>\n",
                   indent, pike_name);
      buf->sprintf(#"\
%*n  <doc><text><p>Constructors for <ref>%s</ref>.</p>
%*n  </text></doc>
",
                   indent, pike_name,
                   indent);
      constructors->render_autodoc(buf, indent + 2, header);
      buf->sprintf("%*n</module>\n", indent);
    }

    //
    // Second: Output the actual class.
    //
    buf->sprintf("%*n<class name='%s' keep=''>\n", indent, pike_name);
    if (flags->get() & 0x0001) { // deprecated
      buf->sprintf(#"\
%*n  <attributes><attribute><prefix/><attribute>\"deprecated\"</attribute>
%*n              </attribute></attributes>
",
                   indent, indent);
    }
    buf->sprintf(#"\
%*n<docgroup homogen-name='GBoxed' homogen-type='inherit'>
%*n  <inherit name='GBoxed'><classname>predef::___GI.GBoxed</classname></inherit>
%*n</docgroup>
",
                   indent + 2,
                   indent + 2,
                 indent + 2);
    foreach(fields, FieldBlob field) {
      field->render_autodoc(buf, indent + 2, header);
    }
    foreach(methods, FunctionBlob method) {
      if (method->flags & 0x0008) continue;
      method->render_autodoc(buf, indent + 2, header);
    }
    buf->sprintf("%*n</class>\n", indent);
  }
}

class UnionBlob {
  inherit Blob;

  Item blob_type = guint16();
  Item flags = guint16();
  Item name = gstring();

  Item gtype_name = gstring();
  Item gtype_init = gstring();

  Item size = guint32();

  Item n_fields = guint16();
  array(FieldBlob) fields;
  Item n_functions = guint16();
  array(FunctionBlob) functions;

  Item copy_func = gstring();
  Item free_func = gstring();

  Item discriminator_offset = guint32();
  SimpleTypeBlob discriminator_type = SimpleTypeBlob();

  void low_decode(Stdio.InputBlockFile file,
                  mapping(int:array(AttributeBlob)) attribute_lookup,
                  mixed... state)
  {
    ::low_decode(file, attribute_lookup, @state);

    fields = ({});
    for (int i = 0; i < n_fields->get(); i++) {
      fields += ({ FieldBlob(file, attribute_lookup, @state) });
    }

    functions = ({});
    for (int i = 0; i < n_functions->get(); i++) {
      functions += ({ FunctionBlob(file, attribute_lookup, @state) });
    }
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    if (sizeof(fields) || sizeof(functions)) {
      buf->sprintf("%*n<union name='%s'>\n", indent, name->get());
      foreach(fields, FieldBlob field) {
        field->render_xml(buf, indent + 2, header);
      }
      foreach(functions, FunctionBlob func) {
        func->render_xml(buf, indent + 2, header);
      }
      buf->sprintf("%*n</union>\n", indent);
    } else {
      buf->sprintf("%*n<union name='%s'/>\n", indent, name->get());
    }
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- UnionBlob, flags: 0x%04x -->\n",
                 indent, flags->get());
#endif

    //
    // First: Consolidate any constructors in a Factory module.
    //
    array(FunctionBlob) constructors =
      filter(functions, lambda(FunctionBlob f) {
        if (f->flags & 0x0008) {
          return 1;
        }
        return 0;
      });
    if (sizeof(constructors)) {
      buf->sprintf("%*n<module name='%s_Factory' keep=''>\n",
                   indent, pike_name);
      buf->sprintf(#"\
%*n  <doc><text><p>Constructors for <ref>%s</ref>.</p>
%*n  </text></doc>
",
                   indent, pike_name,
                   indent);
      constructors->render_autodoc(buf, indent + 2, header);
      buf->sprintf("%*n</module>\n", indent);
    }

    //
    // Second: Output the actual class.
    //
    buf->sprintf("%*n<class name='%s' keep=''>\n", indent, pike_name);
    if (flags->get() & 0x0001) { // deprecated
      buf->sprintf(#"\
%*n  <attributes><attribute><prefix/><attribute>\"deprecated\"</attribute>
%*n              </attribute></attributes>
",
                   indent, indent);
    }
    buf->sprintf(#"\
%*n<docgroup homogen-name='GBoxed' homogen-type='inherit'>
%*n  <inherit name='GBoxed'><classname>predef::___GI.GBoxed</classname></inherit>
%*n</docgroup>
",
                   indent + 2,
                   indent + 2,
                 indent + 2);
    foreach(fields, FieldBlob field) {
      field->render_autodoc(buf, indent + 2, header);
    }
    foreach(functions, FunctionBlob func) {
      if (func->flags & 0x0008) continue;
      func->render_autodoc(buf, indent + 2, header);
    }
    buf->sprintf("%*n</class>\n", indent);
  }
}

class EnumBlob {
  inherit Blob;

  Item blob_type = guint16();

  Item flags = guint16();

  Item name = gstring();

  Item gtype_name = gstring();
  Item gtype_init = gstring();

  Item n_values = guint16();
  Item n_methods = guint16();

  Item error_domain = gstring();

  array(Struct) values = ({});
  array(Struct) methods = ({});

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    values = ({});
    for (int i = 0; i < n_values->get(); i++) {
      values += ({ ValueBlob(file, @state) });
    }
    methods = ({});
    for (int i = 0; i < n_methods->get(); i++) {
      methods += ({ FunctionBlob(file, @state) });
    }
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    string tag = "enumeration";
    if (blob_type->get() == BLOB_TYPE_FLAGS) {
      tag = "bitfield";
    }
    buf->sprintf("%*n<%s name='%s'", indent, tag, name->get());

    if (gtype_name->get() != "") {
      buf->sprintf(" glib:type-name='%s'", gtype_name->get());
    }

    if (gtype_init->get() != "") {
      buf->sprintf(" glib:get-type='%s'", gtype_init->get());
    }
    if (sizeof(error_domain->get())) {
      buf->sprintf(" glib:error-domain='%s'", error_domain->get());
    }
    if (flags->get() & 0x0001) { // deprecated
      buf->add(" deprecated='1'");
    }
    buf->add(">\n");
    foreach(values, Struct value) {
      value->render_xml(buf, indent + 2, header);
    }
    foreach(methods, Struct method) {
      method->render_xml(buf, indent + 2, header);
    }
    buf->sprintf("%*n</%s>\n", indent, tag);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf(#"\
%*n<module name='%s' keep=''>
%*n  <doc><text><p>Module wrapper for the <ref>%s.%s</ref> enum.</p>
%*n    </text>
%*n    <group><seealso/><text><p><ref>%s_Type</ref>, <ref>%s.%s</ref></p>
%*n    </text></group>
%*n  </doc>
%*n  <enum name='%s'>
",
                 indent, pike_name,
                 indent, pike_name, pike_name,
                 indent,
                 indent, pike_name, pike_name, pike_name,
                 indent,
                 indent,
                 indent, pike_name);

    if (flags->get() & 0x0001) { // deprecated
      buf->sprintf(#"\
%*n    <attributes><attribute><prefix/><attribute>\"deprecated\"</attribute>
%*n                </attribute></attributes>
",
                   indent, indent);
    }

    foreach(values, Struct value) {
      value->render_autodoc(buf, indent + 4, header);
    }
#if 0
    foreach(methods, Struct method) {
      method->render_autodoc(buf, indent + 4, header);
    }
#endif
    buf->sprintf(#"\
%*n  </enum>
%*n</module>
", indent, indent);
    buf->sprintf(#"\
%*n<docgroup homogen-name='%s_Type' homogen-type='typedef'>
%*n  <doc><text><p>Type of the <ref>%s.%s</ref> enum.</p></text>
%*n    <group><seealso/><text><p><ref>%s.%s</ref></p>
%*n  </text></group></doc>
%*n  <typedef name='%s_Type'><type><object>%s.%s</object></type></typedef>
%*n</docgroup>
",
                 indent, pike_name,
                 indent, pike_name, pike_name,
                 indent, pike_name, pike_name,
                 indent,
                 indent, pike_name, pike_name, pike_name,
                 indent);
    buf->sprintf(#"\
%*n<docgroup homogen-name='%s' homogen-type='import'>
%*n  <import name='%s'><classname>%s</classname></import>
%*n</docgroup>
",
                 indent, pike_name,
                 indent, pike_name, pike_name,
                 indent);
  }
}

class PropertyBlob {
  inherit Blob;

  Item name = gstring();

  Item flags = guint32();

  Item reserved2 = guint32();

  SimpleTypeBlob prop_type = SimpleTypeBlob();

  void render_xml(String.Buffer buf, int|void indent, Header|void header,
                  array(FunctionBlob)|void methods)
  {
    buf->sprintf("%*n<property name='%s'", indent, name->get());
    if (flags->get() & 0x00000004) {
      buf->add(" writeable='1'");
    }
    if (flags->get() & 0x00000010) {
      buf->add(" construct-only='1'");
    }
    if ((flags->get() & 0x0001ff80) != 0x0001ff80) {
      // Setter.
      int e = (flags->get() & 0x0001ff80) >> 7;
      if (methods && (e < sizeof(methods))) {
        buf->sprintf(" setter='%s'", methods[e]->name);
      } else {
        buf->sprintf(" setter='%d'", e);
      }
    }
    if ((flags->get() & 0x07fe0000) != 0x07fe0000) {
      // Getter.
      int e = (flags->get() & 0x07fe0000) >> 17;
      if (methods && (e < sizeof(methods))) {
        buf->sprintf(" getter='%s'", methods[e]->name);
      } else {
        buf->sprintf(" getter='%d'", e);
      }
    }
    if (flags->get() & 0x00000020) {
      buf->add(" transfer-ownership='full'");
    } else {
      buf->add(" transfer-ownership='none'");
    }
    if (flags->get() & 0x0001) { // deprecated
      buf->add(" deprecated='1'");
    }
    buf->add(">\n");
    prop_type->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</property>\n", indent);
  }
}

class SignalBlob {
  inherit Blob;

  Item flags = guint16();

  Item class_closure = guint16();

  Item name = gstring();

  Item reserved2 = guint32();

  Item signature_offset = guint32();

  SignatureBlob signature;

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    int pos = file->tell();
    file->seek(signature_offset->get());
    signature = SignatureBlob(file, @state);
    file->seek(pos);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- SignalBlob, flags: 0x%04x -->\n",
                 indent, flags->get());
#endif
    int fl = flags->get();

    buf->sprintf("%*n<signal name='%s'", indent, name->get());
    if (fl & 0x0004) {
      buf->add(" when='LAST'");
    }
    if (fl & 0x0040) {
      buf->add(" action='1'");
    }
    buf->add(">\n");
    signature->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</signal>\n", indent);
  }
}

class VFuncBlob {
  inherit Blob;

  Item name = gstring();

  Item flags = guint16();
  Item signal = guint16();

  Item struct_offset = guint16();
  Item invoker = guint16();

  Item finish = guint16();
  Item reserved3 = guint16();

  Item signature_offset = guint32();

  SignatureBlob signature;

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    int pos = file->tell();
    file->seek(signature_offset->get());
    signature = SignatureBlob(file, @state);
    file->seek(pos);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header,
                  array(FunctionBlob)|void methods)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- VFuncBlob, flags: 0x%04x -->\n",
                 indent, flags->get());
#endif
    int fl = flags->get();

    buf->sprintf("%*n<virtual-method name='%s'", indent, name->get());
    if (struct_offset->get() != 0xffff) {
      buf->sprintf(" offset='%d'", struct_offset->get());
    }
    if ((invoker->get() & 0x03ff) != 0x3ff) {
      int e = invoker->get() & 0x03ff;
      if (methods && (e < sizeof(methods))) {
        buf->sprintf(" invoker='%s'", methods[e]->name);
      } else {
        buf->sprintf(" invoker='%d'", e);
      }
    }
    if ((finish->get() & 0x03ff) != 0x3ff) {
      int e = finish->get() & 0x03ff;
      if (e) {
        // NB: Apparently 0 is also used to indicate no finish.
        if (methods && (e < sizeof(methods))) {
          buf->sprintf(" finish='%s'", methods[e]->name);
        } else {
          buf->sprintf(" finish='%d'", finish->get() & 0x3ff);
        }
      }
    }
    buf->add(">\n");
    signature->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</virtual-method>\n", indent);
  }
}

class ObjectBlob {
  inherit Blob;

  Item blob_type = guint16();
  Item flags = guint16();
  Item name = gstring();

  Item gtype_name = gstring();
  Item gtype_init = gstring();

  Item parent = guint16();
  Item gtype_struct = guint16();

  Item n_interfaces = guint16();
  array(guint16) interfaces = ({});
  Item n_fields = guint16();
  array(FieldBlob) fields = ({});
  Item n_properties = guint16();
  array(PropertyBlob) properties = ({});
  Item n_methods = guint16();
  array(FunctionBlob) methods = ({});
  Item n_signals = guint16();
  array(SignalBlob) signals = ({});
  Item n_vfuncs = guint16();
  array(VFuncBlob) vfuncs = ({});
  Item n_constants = guint16();
  array(ConstantBlob) constants = ({});
  Item n_field_callbacks = guint16();
  array(CallbackBlob) field_callbacks = ({});

  Item ref_func = gstring();
  Item unref_func = gstring();
  Item set_value_func = gstring();
  Item get_value_func = gstring();

  Item reserved3 = guint32();
  Item reserved4 = guint32();

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    interfaces = ({});
    for (int i = 0; i < n_interfaces->get(); i++) {
      guint16 entryno = guint16();
      entryno->decode(file, @state);
      interfaces += ({ entryno });
    }
    if (file->tell() & 0x03) file->read(2);	// Pad to 32bit.
    fields = ({});
    for (int i = 0; i < n_fields->get(); i++) {
      fields += ({ FieldBlob(file, @state) });
    }
    properties = ({});
    for (int i = 0; i < n_properties->get(); i++) {
      properties += ({ PropertyBlob(file, @state) });
    }
    methods = ({});
    for (int i = 0; i < n_methods->get(); i++) {
      methods += ({ FunctionBlob(file, @state) });
    }
    signals = ({});
    for (int i = 0; i < n_signals->get(); i++) {
      signals += ({ SignalBlob(file, @state) });
    }
    vfuncs = ({});
    for (int i = 0; i < n_vfuncs->get(); i++) {
      vfuncs += ({ VFuncBlob(file, @state) });
    }
    constants = ({});
    for (int i = 0; i < n_constants->get(); i++) {
      constants += ({ ConstantBlob(file, @state) });
    }
    field_callbacks = ({});
    for (int i = 0; i < n_field_callbacks->get(); i++) {
      field_callbacks += ({ CallbackBlob(file, @state) });
    }
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf("%*n<class name='%s' keep=''", indent, name->get());

    int p = parent->get();
    if (p > 0) {
      if (header && (p <= sizeof(header->entries))) {
        DirEntry entry = header->entries[p-1];
        buf->sprintf(" parent='%s'", entry->get_gi_name());
      } else {
        buf->sprintf(" parent='%d'", p);
      }
    }

    int ts = gtype_struct->get();
    if (ts > 0) {
      if (header && (ts <= sizeof(header->entries))) {
        DirEntry entry = header->entries[ts-1];
        buf->sprintf(" glib:type-struct='%s'", entry->get_gi_name());
      } else {
        buf->sprintf(" glib:type-struct='%d'", ts);
      }
    }

    if (flags->get() & 2) {
      buf->add(" abstract='1'");
    }

    if (gtype_name->get() != "") {
      buf->sprintf(" glib:type-name='%s'", gtype_name->get());
    }

    if (gtype_init->get() != "") {
      buf->sprintf(" glib:get-type='%s'", gtype_init->get());
    }

    buf->add(">\n");
    foreach(interfaces->get(), int entryno) {
      if (entryno <= sizeof(header->entries)) {
        DirEntry entry = header->entries[entryno - 1];
        buf->sprintf("%*n<implements name='%s'/>\n",
                     indent + 2, entry->get_gi_name(header));
      } else {
        buf->sprintf("%*n<implements name='%d'/>\n",
                     indent + 2, entryno - 1);
      }
    }
    // interfaces->render_xml(buf, indent + 2, header);
    fields->render_xml(buf, indent + 2, header);
    methods->render_xml(buf, indent + 2, header);
    signals->render_xml(buf, indent + 2, header);
    properties->render_xml(buf, indent + 2, header, methods);
    vfuncs->render_xml(buf, indent + 2, header, methods);
    constants->render_xml(buf, indent + 2, header);
    field_callbacks->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</class>\n", indent);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    //
    // First: Consolidate any constructors in a Factory module.
    //
    array(FunctionBlob) constructors =
      filter(methods, lambda(FunctionBlob f) {
        if (f->flags & 0x0008) {
          return 1;
        }
        return 0;
      });
    if (sizeof(constructors)) {
      buf->sprintf("%*n<module name='%s_Factory' keep=''>\n",
                   indent, pike_name);
      buf->sprintf(#"\
%*n  <doc><text><p>Constructors for <ref>%s</ref>.</p>
%*n  </text></doc>
",
                   indent, pike_name,
                   indent);
      constructors->render_autodoc(buf, indent + 2, header);
      buf->sprintf("%*n</module>\n", indent);
    }

    //
    // Second: Generate the main class.
    //
    buf->sprintf("%*n<class name='%s' keep=''>\n", indent, pike_name);

    if (sizeof(constructors)) {
      buf->sprintf("%*n  <doc><group><seealso/><text><p>\n", indent);
      foreach(constructors; int i; FunctionBlob f) {
        buf->sprintf("%*n    %s <ref>%s_Factory.%s</ref>\n",
                     indent, i?",":"", pike_name, f->name);
      }
      buf->sprintf("%*n  </p></text></group></doc>\n", indent);
    }

    int p = parent->get();
    if (p > 0) {
      if (header && (p <= sizeof(header->entries))) {
        DirEntry entry = header->entries[p-1];
        string parent = entry->get_pike_name(header);
        buf->sprintf(#"\
%*n<docgroup homogen-name='%s' homogen-type='inherit'>
%*n  <inherit name='%s'><classname>%s</classname></inherit>
%*n</docgroup>
",
                     indent + 2, entry->name,
                     indent + 2, entry->name, parent,
                     indent + 2);
      }
    } else {
      buf->sprintf(#"\
%*n<docgroup homogen-name='GObject' homogen-type='inherit'>
%*n  <inherit name='GObject'><classname>predef::___GI.GObject</classname></inherit>
%*n</docgroup>
",
                   indent + 2,
                   indent + 2,
                   indent + 2);
    }
    if (sizeof(interfaces->get())) {
      foreach(interfaces->get(), int entryno) {
        if (entryno <= sizeof(header->entries)) {
          DirEntry entry = header->entries[entryno - 1];
          string mixin = entry->get_pike_name(header);
          buf->sprintf(#"\
%*n<docgroup homogen-name='%s_Mixin' homogen-type='inherit'>
%*n  <inherit name='%s_Mixin'><classname>%s_Mixin</classname></inherit>
%*n</docgroup>
",
                       indent + 2, entry->name,
                       indent + 2, entry->name, mixin,
                       indent + 2);
        }
      }
      buf->sprintf("%*n<annotations>\n", indent + 2);
      foreach(interfaces->get(), int entryno) {
        if (entryno <= sizeof(header->entries)) {
          DirEntry entry = header->entries[entryno - 1];
          string mixin = entry->get_pike_name(header);
          buf->sprintf("%*n<annotation>"
                       "<ref>predef::Pike.Annotations.Implements</ref>"
                       "(<ref>%s</ref>)</annotation>\n",
                       indent + 4, mixin);
        }
      }
      buf->sprintf("%*n</annotations>\n", indent + 2);
    }

    fields->render_autodoc(buf, indent + 2, header);
    properties->render_autodoc(buf, indent + 2, header, methods);
    foreach(methods, FunctionBlob method) {
      if (method->flags & 0x0008) continue;
      method->render_autodoc(buf, indent + 2, header);
    }
    signals->render_autodoc(buf, indent + 2, header);
    // vfuncs->render_autodoc(buf, indent + 2, header, methods);
    constants->render_autodoc(buf, indent + 2, header);
    // field_callbacks->render_autodoc(buf, indent + 2, header);
    buf->sprintf("%*n</class>\n", indent);

    //
    // Third: Generate the Mixin class.
    //
    buf->sprintf("%*n<class name='%s_Mixin' keep=''>\n", indent, pike_name);

    p = parent->get();
    if (p > 0) {
      if (header && (p <= sizeof(header->entries))) {
        DirEntry entry = header->entries[p-1];
        string parent = entry->get_pike_name(header);
        buf->sprintf(#"\
%*n<docgroup homogen-name='%s_Mixin' homogen-type='inherit'>
%*n  <inherit name='%s_Mixin'><classname>%s_Mixin</classname></inherit>
%*n</docgroup>
",
                     indent + 2, entry->name,
                     indent + 2, entry->name, parent,
                     indent + 2);
      }
    }
    if (sizeof(interfaces->get())) {
      foreach(interfaces->get(), int entryno) {
        if (entryno <= sizeof(header->entries)) {
          DirEntry entry = header->entries[entryno - 1];
          string mixin = entry->get_pike_name(header);
          buf->sprintf(#"\
%*n<docgroup homogen-name='%s_Mixin' homogen-type='inherit'>
%*n  <inherit name='%s_Mixin'><classname>%s_Mixin</classname></inherit>
%*n</docgroup>
",
                       indent + 2, entry->name,
                       indent + 2, entry->name, mixin,
                       indent + 2);
        }
      }
      buf->sprintf("%*n<annotations>\n", indent + 2);
      foreach(interfaces->get(), int entryno) {
        if (entryno <= sizeof(header->entries)) {
          DirEntry entry = header->entries[entryno - 1];
          string mixin = entry->get_pike_name(header);
          buf->sprintf("%*n<annotation>"
                       "<ref>predef::Pike.Annotations.Implements</ref>"
                       "(<ref>%s</ref>)</annotation>\n",
                       indent + 4, mixin);
        }
      }
      buf->sprintf("%*n</annotations>\n", indent + 2);
    }

    fields->render_autodoc(buf, indent + 2, header);
    properties->render_autodoc(buf, indent + 2, header, methods);
    methods->render_autodoc(buf, indent + 2, header);
    signals->render_autodoc(buf, indent + 2, header);
    // vfuncs->render_autodoc(buf, indent + 2, header, methods);
    constants->render_autodoc(buf, indent + 2, header);
    // field_callbacks->render_autodoc(buf, indent + 2, header);
    buf->sprintf("%*n</class>\n", indent);
  }
}

class InterfaceBlob {
  inherit Blob;

  Item blob_type = guint16();
  Item flags = guint16();
  Item name = gstring();

  Item gtype_name = gstring();
  Item gtype_init = gstring();
  Item gtype_struct = guint16();

  Item n_prerequisites = guint16();
  array(guint16) prerequisites = ({});
  Item n_properties = guint16();
  array(PropertyBlob) properties = ({});
  Item n_methods = guint16();
  array(FunctionBlob) methods = ({});
  Item n_signals = guint16();
  array(SignalBlob) signals = ({});
  Item n_vfuncs = guint16();
  array(VFuncBlob) vfuncs = ({});
  Item n_constants = guint16();
  array(ConstantBlob) constants = ({});

  Item padding = guint16();

  Item reserved2 = guint32();
  Item reserved3 = guint32();

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    int off = file->tell();
    string raw = file->read(64);
    file->seek(off);
    ::low_decode(file, @state);

    prerequisites = ({});
    for (int i = 0; i < n_prerequisites->get(); i++) {
      guint16 entryno = guint16();
      entryno->decode(file, @state);
      prerequisites += ({ entryno });
    }
    if (file->tell() & 0x03) file->read(2);	// Pad to 32bit.
    properties = ({});
    for (int i = 0; i < n_properties->get(); i++) {
      properties += ({ PropertyBlob(file, @state) });
    }
    methods = ({});
    for (int i = 0; i < n_methods->get(); i++) {
      methods += ({ FunctionBlob(file, @state) });
    }
    signals = ({});
    for (int i = 0; i < n_signals->get(); i++) {
      signals += ({ SignalBlob(file, @state) });
    }
    vfuncs = ({});
    for (int i = 0; i < n_vfuncs->get(); i++) {
      vfuncs += ({ VFuncBlob(file, @state) });
    }
    constants = ({});
    for (int i = 0; i < n_constants->get(); i++) {
      constants += ({ ConstantBlob(file, @state) });
    }
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- InterfaceBlob, flags: 0x%04x -->\n",
                 indent, flags->get());
#endif
    buf->sprintf("%*n<interface name='%s'", indent, name->get());
    if (sizeof(gtype_name->get()) || sizeof(gtype_init->get())) {
      buf->sprintf(" glib:type-name='%s' glib:get-type='%s'",
                   gtype_name->get(), gtype_init->get());
    }
    int eno = gtype_struct->get();
    if (eno < sizeof(header->entries)) {
      DirEntry e = header->entries[eno];
      buf->sprintf(" glib:gtype-struct='%s'", e->get_gi_name());
    }
    buf->add(">\n");
    foreach(prerequisites->get(), int req) {
      if (req && (req <= sizeof(header->entries))) {
        buf->sprintf("%*n<prerequisite name='%s'/>\n",
                     indent + 2, header->entries[req-1]->get_gi_name());
      } else {
        buf->sprintf("%*n<prerequisite name='%d'/>\n", indent + 2, req-1);
      }
    }
    methods->render_xml(buf, indent + 2, header);
    signals->render_xml(buf, indent + 2, header);
    vfuncs->render_xml(buf, indent + 2, header, methods);
    properties->render_xml(buf, indent + 2, header, methods);
    constants->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</interface>\n", indent);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    //
    // First: Consolidate any constructors in a Factory module.
    //
    array(FunctionBlob) constructors =
      filter(methods, lambda(FunctionBlob f) {
        if (f->flags & 0x0008) {
          return 1;
        }
        return 0;
      });
    if (sizeof(constructors)) {
      buf->sprintf("%*n<module name='%s_Factory' keep=''>\n",
                   indent, pike_name);
      buf->sprintf(#"\
%*n  <doc><text><p>Constructors for %s.</p>
%*n  </text></doc>
",
                   indent, pike_name,
                   indent);
      constructors->render_autodoc(buf, indent + 2, header);
      buf->sprintf("%*n</module>\n", indent);
    }

    //
    // Second: Generate the Mixin class.
    //
    buf->sprintf("%*n<class name='%s_Mixin' keep=''>\n", indent, pike_name);

    if (sizeof(constructors)) {
      buf->sprintf("%*n  <doc><group><seealso/><text><p>\n", indent);
      foreach(constructors; int i; FunctionBlob f) {
        buf->sprintf("%*n    %s <ref>%s_Factory.%s</ref>\n",
                     indent, i?",":"", pike_name, f->name);
      }
      buf->sprintf("%*n  </p></text></group></doc>\n", indent);
    }

    buf->sprintf(#"\
%*n<docgroup homogen-name='GObject_Mixin' homogen-type='inherit'>
%*n  <inherit name='GObject_Mixin'><classname>predef::___GI.GObject_Mixin</classname></inherit>
%*n</docgroup>
",
                 indent + 2,
                 indent + 2,
                 indent + 2);
    if (sizeof(prerequisites)) {
      foreach(prerequisites->get(), int entryno) {
        if (entryno <= sizeof(header->entries)) {
          DirEntry entry = header->entries[entryno - 1];
          string mixin = entry->get_pike_name(header);
          buf->sprintf(#"\
%*n<docgroup homogen-name='%s_Mixin' homogen-type='inherit'>
%*n  <inherit name='%s_Mixin'><classname>%s_Mixin</classname></inherit>
%*n</docgroup>
",
                       indent + 2, entry->name,
                       indent + 2, entry->name, mixin,
                       indent + 2);
        }
      }
      buf->sprintf("%*n<annotations>\n", indent + 2);
      foreach(prerequisites->get(), int entryno) {
        if (entryno <= sizeof(header->entries)) {
          DirEntry entry = header->entries[entryno - 1];
          string mixin = entry->get_pike_name(header);
          buf->sprintf("%*n<annotation>"
                       "<ref>predef::Pike.Annotations.Implements</ref>"
                       "(<ref>%s</ref>)</annotation>\n",
                       indent + 4, mixin);
        }
      }
      buf->sprintf("%*n</annotations>\n", indent + 2);
    }

    properties->render_autodoc(buf, indent + 2, header, methods);
    foreach(methods, FunctionBlob method) {
      if (method->flags & 0x0008) continue;
      method->render_autodoc(buf, indent + 2, header);
    }
    signals->render_autodoc(buf, indent + 2, header);
    // vfuncs->render_autodoc(buf, indent + 2, header, methods);
    constants->render_autodoc(buf, indent + 2, header);
    // field_callbacks->render_autodoc(buf, indent + 2, header);
    buf->sprintf("%*n</class>\n", indent);

    //
    // Third: Generate the instance class.
    //
    buf->sprintf("%*n<class name='%s' keep=''>\n", indent, pike_name);

    buf->sprintf(#"\
%*n<docgroup homogen-name='%s_Mixin' homogen-type='inherit'>
%*n  <inherit name='%s_Mixin'><classname>%s_Mixin</classname></inherit>
%*n</docgroup>
",
                 indent + 2, pike_name,
                 indent + 2, pike_name, pike_name,
                 indent + 2);

    buf->sprintf(#"\
%*n<docgroup homogen-name='GObject' homogen-type='inherit'>
%*n  <inherit name='GObject'><classname>___GI.GObject</classname></inherit>
%*n</docgroup>
",
                 indent + 2,
                 indent + 2,
                 indent + 2);
    buf->sprintf("%*n</class>\n", indent);
  }
}

class ConstantBlob {
  inherit Blob;

  Item blob_type = guint16();
  Item flags = guint16();
  Item name = gstring();

  SimpleTypeBlob type = SimpleTypeBlob();

  Item size = guint32();
  Item offset = guint32();

  Item reserved2 = guint32();

  string(8bit)|int|float value;

  void low_decode(Stdio.InputBlockFile file,
                  mapping(int:array(this_program)) attribute_lookup,
                  mixed... state)
  {
    ::low_decode(file, attribute_lookup, @state);

#if __PIKE_BYTEORDER__ == 1234
    string dir = "-";
#else
    string dir = "";
#endif

    int pos = file->tell();
    file->seek(offset->get());
    value = file->read(size->get());
    if (!type->blob) {
      int tag = type->flags >> 27;
      if ((<
            GI_TYPE_TAG_BOOLEAN,
            GI_TYPE_TAG_INT8,
            GI_TYPE_TAG_UINT8,
            GI_TYPE_TAG_INT16,
            GI_TYPE_TAG_UINT16,
            GI_TYPE_TAG_INT32,
            GI_TYPE_TAG_UINT32,
            GI_TYPE_TAG_INT64,
            GI_TYPE_TAG_UINT64,
            GI_TYPE_TAG_FLOAT,
            GI_TYPE_TAG_DOUBLE,
            GI_TYPE_TAG_UNICHAR,
          >)[tag]) {
        if ((<
              GI_TYPE_TAG_FLOAT,
              GI_TYPE_TAG_DOUBLE,
            >)[tag]) {
          // Float/double.
          sscanf(value, "%" + dir + size->get() + "F", value);
        } else if ((<
                     GI_TYPE_TAG_INT8,
                     GI_TYPE_TAG_INT16,
                     GI_TYPE_TAG_INT32,
                     GI_TYPE_TAG_INT64,
                   >)[tag]){
          // Signed integer.
          sscanf(value, "%+" + dir + size->get() + "c", value);
        } else {
          // Unsigned integer.
          sscanf(value, "%" + dir + size->get() + "c", value);
          if (tag == GI_TYPE_TAG_BOOLEAN) {
            value = !!value;
          }
        }
      }
    }
    file->seek(pos);
  }

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
#ifdef PARSER_TYPELIB_DEBUG
    buf->sprintf("%*n<!-- ConstantBlob, flags: 0x%04x, size: 0x%08x, off: 0x%08x -->\n",
                 indent, flags->get(), size->get(), offset->get());
#endif
    buf->sprintf("%*n<constant name='%s'", indent, name->get());
    if (stringp(value)) {
      if (has_suffix(value, "\0")) {
        buf->sprintf(" value='%s'", value[..<1]);
      } else {
        // FIXME: How should the binary data be encoded?
        buf->sprintf(" value='%s'", String.string2hex(value));
      }
    } else if (intp(value)) {
      buf->sprintf(" value='%d'", value);
    } else if (floatp(value)) {
      buf->sprintf(" value='%5g'", value);
    }
    buf->add(">\n");
    type->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</constant>\n", indent);
  }

  void render_autodoc(String.Buffer buf, int|void indent, Header|void header)
  {
    buf->sprintf(#"\
%*n<docgroup homogen-name='%s' homogen-type='constant'>
%*n  <doc><text/></doc>
%*n  <constant name='%s'>
",
                 indent, pike_name,
                 indent,
                 indent, pike_name);
    type->render_xml(buf, indent + 4, header);
    buf->sprintf(#"\
%*n  </constant>
%*n</docgroup>
",
                 indent, indent);
  }
}

enum GITypelibError {
  G_TYPELIB_ERROR_INVALID = 0,
  G_TYPELIB_ERROR_INVALID_HEADER,
  G_TYPELIB_ERROR_INVALID_DIRECTORY,
  G_TYPELIB_ERROR_INVALID_ENTRY,
  G_TYPELIB_ERROR_INVALID_BLOB,
}
