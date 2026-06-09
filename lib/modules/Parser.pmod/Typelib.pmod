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
    buf->sprintf(#"\
%*n</namespace>
%*n</repository>
", indent + 2, indent);
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
    if (blob) {
      blob->render_xml(buf, indent, header);
    } else {
      int tag = flags->get() >> 27;
      buf->sprintf("%*n<type name='%s'/>\n",
                   indent,
                   TypeTagNameLookup[tag] || ("&lt;" + tag + "&gt;"));
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
    int f = flags->get();

    buf->sprintf("%*n<parameter name='%s' transfer-ownership='%s'",
                 indent,
                 name->get(),
                 (f & 0x20) ? "full" : "none");
    if (f & 0x00000700) {
      int scope = (f & 0x00000700)>>8;
      buf->sprintf(" scope='%s'",
                   ([
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
  GI_TYPE_TAG_GTYPE: "",
  GI_TYPE_TAG_UTF8: "utf8",
  GI_TYPE_TAG_FILENAME: "",
  GI_TYPE_TAG_ARRAY: "",
  GI_TYPE_TAG_INTERFACE: "",
  GI_TYPE_TAG_GLIST: "",
  GI_TYPE_TAG_GSLIST: "",
  GI_TYPE_TAG_GHASH: "",
  GI_TYPE_TAG_ERROR: "",
  GI_TYPE_TAG_UNICHAR: "",
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
      buf->sprintf(" fixed-size='%d'", length->get());
    }
    buf->add(">\n");
    type->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</array>\n", indent);
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
      foreach(fields, FieldBlob field) {
        field->render_xml(buf, indent + 2, header);
      }
      foreach(methods, FunctionBlob method) {
        method->render_xml(buf, indent + 2, header);
      }
      buf->sprintf("%*n</%s>\n", indent, tag);
    } else {
      buf->add("/>\n");
    }
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
  Item n_functions = guint16();

  Item copy_func = gstring();
  Item free_func = gstring();

  Item discriminator_offset = guint32();
  SimpleTypeBlob discriminator_type = SimpleTypeBlob();

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    if (sizeof(gtype_name->get()) || sizeof(gtype_init->get())) {
      ::render_xml(buf, indent, header);
    } else {
      buf->sprintf("%*n<union name='%s'/>\n", indent, name->get());
    }
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
    buf->sprintf("%*n<enumeration name='%s'", indent, name->get());
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
    buf->sprintf("%*n</enumeration>\n", indent);
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
      int e = (flags->get() & 0x07fe0000) >> 7;
      if (methods && (e <= sizeof(methods))) {
        buf->sprintf(" setter='%s'", methods[e]->name);
      } else {
        buf->sprintf(" setter='%d'", e);
      }
    }
    if ((flags->get() & 0x07fe0000) != 0x07fe0000) {
      // Getter.
      int e = (flags->get() & 0x07fe0000) >> 17;
      if (methods && (e <= sizeof(methods))) {
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
    buf->sprintf("%*n<!-- StructBlob, flags: 0x%04x -->\n",
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

  void render_xml(String.Buffer buf, int|void indent, Header|void header)
  {
    int fl = flags->get();

    buf->sprintf("%*n<virtual-method name='%s'", indent, name->get());
    if (struct_offset->get() != 0xffff) {
      buf->sprintf(" offset='%d'", struct_offset->get());
    }
    if ((invoker->get() & 0x03ff) != 0x3ff) {
      buf->sprintf("invoker='%d'", invoker->get() & 0x3ff);
    }
    if ((finish->get() & 0x03ff) != 0x3ff) {
      buf->sprintf("finish='%d'", finish->get() & 0x3ff);
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
    buf->sprintf("%*n<class name='%s'", indent, name->get());

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
      DirEntry entry = header->entries[entryno - 1];
      buf->sprintf("%*n<implements name='%s'/>\n",
                   indent + 2, entry->get_gi_name(header));
    }
    fields->render_xml(buf, indent + 2, header);
    properties->render_xml(buf, indent + 2, header);
    methods->render_xml(buf, indent + 2, header);
    signals->render_xml(buf, indent + 2, header);
    vfuncs->render_xml(buf, indent + 2, header);
    constants->render_xml(buf, indent + 2, header);
    field_callbacks->render_xml(buf, indent + 2, header);
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
  Item gtype_struct = gstring();

  Item n_prerequisites = guint16();
  array(InterfaceBlob) prerequisites = ({});
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
    buf->sprintf("%*n<implements name='%s'", indent, name->get());
    buf->add(">\n");
    properties->render_xml(buf, indent + 2, header, methods);
    methods->render_xml(buf, indent + 2, header);
    signals->render_xml(buf, indent + 2, header);
    vfuncs->render_xml(buf, indent + 2, header);
    constants->render_xml(buf, indent + 2, header);
    buf->sprintf("%*n</implements>\n", indent);
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
}

enum GITypelibError {
  G_TYPELIB_ERROR_INVALID = 0,
  G_TYPELIB_ERROR_INVALID_HEADER,
  G_TYPELIB_ERROR_INVALID_DIRECTORY,
  G_TYPELIB_ERROR_INVALID_ENTRY,
  G_TYPELIB_ERROR_INVALID_BLOB,
}
