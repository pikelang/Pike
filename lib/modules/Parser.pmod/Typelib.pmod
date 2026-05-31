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
  //! @note
  //!   The default implementation just renders an XML comment
  //!   with a note that @tt{render_xml()@} is not implemented.
  void render_xml(String.Buffer buf, int|void indent)
  {
    buf->sprintf("%*s<!-- %O->render_xml() not implemented! -->\n",
                 indent, "", object_program(this));
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
    ::low_decode(file, @state);
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
    value->decode(file, attribute_lookup, @state);
  }

  void render_xml(String.Buffer buf, int|void indent)
  {
    string|int val = value->get();
    if (stringp(val)) {
      buf->sprintf("%*s<attribute name='%s' value='%s'/>\n",
                   indent, "", name->get(), val);
    } else {
      buf->sprintf("%*s<attribute name='%s' value='%d'/>\n",
                   indent, "", name->get(), val);
    }
  }
}

//!
__experimental__ class Header {
  inherit Struct;

  Item magic = Chars(16);
  Item major_version = Byte();
  Item minor_version = Byte();
  Item reserved = guint16();
  Item n_entries = guint16();
  array(object) entries = ({});
  Item n_local_entries = guint16();
  array(object) local_entries = ({});
  Item directory = guint32();
  Item n_attributes = guint32();
  Item attributes = guint32();

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

    mapping(int:array(AttributeBlob)) attribute_lookup = ([]);
    file->seek(attributes->get());
    for (int i = 0; i < n_attributes->get(); i++) {
      AttributeBlob attr = AttributeBlob(file, attribute_lookup, @state);
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

  void render_xml(String.Buffer buf, int|void indent)
  {
    buf->sprintf("%*s<namespace name='%s'"
                 " version='%s' c:prefix='%s'>\n",
                 indent, "",
                 namespace->get(), nsversion->get(), c_prefix->get());
    foreach(dependencies->get()/"|", string dep) {
      array(string) a = dep/"-";
      buf->sprintf("%*s<include name='%s' version='%s'/>\n",
                   indent + 2, "",
                   a[0], (sizeof(a) > 1)?a[1]:"");
    }
    foreach(entries, object entry) {
      entry->render_xml(buf, indent + 2);
    }
    buf->sprintf("%*s</namespace>\n", indent, "");
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
  Item offset = guint32();

  string|array(Item) blob = ({});

  void low_decode(Stdio.InputBlockFile file, mixed... state)
  {
    ::low_decode(file, @state);

    int pos = file->tell();

    if (offset->get()) {
      file->seek(offset->get());
      if (local_reserved->get() & 0x0001) {
        blob = ({ blob_factory(file, blob_type->get(), @state) });
      } else {
        Item var_string = Varchars();
        var_string->decode(file);
        blob = var_string->get();
      }
    }

    file->seek(pos);
  }

  protected string _sprintf(int c)
  {
    return sprintf("%s: name: %O, blob: %O\n",
                   ::_sprintf(c),
                   name->get(), blob);
  }

  void render_xml(String.Buffer buf, int|void indent)
  {
    if (arrayp(blob) && sizeof(blob)) {
      blob[0]->render_xml(buf, indent);
    } else {
      buf->sprintf("%*s<record name='%s'>\n", indent, "", name->get());
      buf->sprintf("%*s</record>\n", indent, "");
    }
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

class SimpleTypeBlobFlags {
  inherit Struct;

  Item reserved_pointer_tag = guint32();
}

class SimpleTypeBlob {
  inherit Blob;

  SimpleTypeBlobFlags flags = SimpleTypeBlobFlags();
  Item offset = guint32();
}

class ArgBlob {
  inherit Blob;

  Item name = gstring();
  Item flags = guint32();
  Item closure_arg = Byte();
  Item destroy_arg = Byte();

  Item padding = guint16();

  SimpleTypeBlob arg_type = SimpleTypeBlob();
}

class SignatureBlob {
  inherit Blob;

  SimpleTypeBlob return_type = SimpleTypeBlob();

  Item flags = guint16();

  Item n_arguments = guint16();
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
  Item signature = guint32();

  Item more_flags = guint16();

  Item finish = guint16();
}

class CallbackBlob {
  inherit Blob;

  Item blob_type = guint16();

  Item flags = guint16();
  Item name = gstring();
  Item signature = guint32();
}

class InterfaceTypeBlob {
  inherit Blob;

  Item pointer_and_tag = Byte();
  Item reserved2 = Byte();
  Item interface = guint16();
}

class ArrayTypeDimension {
  inherit Struct;

  Item length = guint16();
  Item size = guint16();
}

class ArrayTypeBlob {
  inherit Blob;

  Item pointer_and_tag = guint16();

  ArrayTypeDimension dimensions = ArrayTypeDimension();

  SimpleTypeBlob type = SimpleTypeBlob();
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

  void render_xml(String.Buffer buf, int|void indent)
  {
    int val = value->get();
    if (!(flags->get() & 0x00000002)) {
      // Signed value.
      if (val & 0x80000000) {
        val -= 0x100000000;
      }
    }
    // FIXME: How to mark deprecated values?
    buf->sprintf("%*s<member name='%s' value='%d'>\n",
                 indent, "", name->get(), val);
    // FIXME: <attribute name="c:identifier" value="XXXX_XXXX"/>
    attributes->render_xml(buf, indent + 2);
    buf->sprintf("%*s</member>\n", indent, "");
  }
}

class FieldBlob {
  inherit Blob;

  Item flags = Byte();
  Item bits = Byte();

  Item struct_offset = guint16();

  Item reserved2 = guint32();

  SimpleTypeBlob type = SimpleTypeBlob();
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
  Item n_methods = guint16();

  Item copy_func = gstring();
  Item free_func = gstring();

  void render_xml(String.Buffer buf, int|void indent)
  {
    if (sizeof(gtype_name->get()) || sizeof(gtype_init->get())) {
      buf->sprintf("%*s<glib:boxed glib:name='%s'"
                   " glib:type-name='%s' glib:get-type='%s'/>\n",
                   indent, "", name->get(),
                   gtype_name->get(), gtype_init->get());
    } else {
      buf->sprintf("%*s<record name='%s'/>\n", indent, "", name->get());
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

  void render_xml(String.Buffer buf, int|void indent)
  {
    buf->sprintf("%*s<enumeration name='%s'>\n", indent, "", name->get());
    foreach(values, Struct value) {
      value->render_xml(buf, indent + 2);
    }
    foreach(methods, Struct method) {
      method->render_xml(buf, indent + 2);
    }
    buf->sprintf("%*s</enumeration>\n", indent, "");
  }
}

class PropertyBlob {
  inherit Blob;

  Item name = gstring();

  Item flags = guint32();

  Item reserved2 = guint32();

  SimpleTypeBlob discriminator_type = SimpleTypeBlob();
}

class SignalBlob {
  inherit Blob;

  Item flags = guint16();

  Item class_closure = guint16();

  Item name = gstring();

  Item reserved2 = guint32();

  Item signature = guint32();
}

class VFuncBlob {
  inherit Blob;

  Item name = gstring();

  Item flags = guint16();
  Item signal = guint16();

  Item struct_offset = guint16();
  Item invoker = guint16();

  Item finish = guint16();
  Item reserved2 = guint16();
  Item reserved3 = guint16();

  Item signature = guint32();
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
  Item n_fields = guint16();
  Item n_properties = guint16();
  Item n_methods = guint16();
  Item n_signals = guint16();
  Item n_vfuncs = guint16();
  Item n_constants = guint16();
  Item n_field_callbacks = guint16();

  Item ref_func = gstring();
  Item unref_func = gstring();
  Item set_value_func = gstring();
  Item get_value_func = gstring();

  Item reserved3 = guint32();
  Item reserved4 = guint32();

  void render_xml(String.Buffer buf, int|void indent)
  {
    buf->sprintf("%*s<class name='%s' parent='%d' "
                 "glib:type-name='%s' glib:get-type='%s'>\n",
                 indent, "",
                 name->get(), parent->get(),
                 gtype_name->get(), gtype_init->get());
    buf->sprintf("%*s</class>\n", indent, "");
  }
}

class InterfaceBlob {
  inherit Blob;

  Item blub_type = guint16();
  Item flags = guint16();
  Item name = gstring();

  Item gtype_name = gstring();
  Item gtype_init = gstring();
  Item gtype_struct = gstring();

  Item n_prerequisites = guint16();
  Item n_properties = guint16();
  Item n_methods = guint16();
  Item n_signals = guint16();
  Item n_vfuncs = guint16();
  Item n_constants = guint16();

  Item padding = guint16();

  Item reserved2 = guint32();
  Item reserved3 = guint32();
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
}

enum GITypelibError {
  G_TYPELIB_ERROR_INVALID = 0,
  G_TYPELIB_ERROR_INVALID_HEADER,
  G_TYPELIB_ERROR_INVALID_DIRECTORY,
  G_TYPELIB_ERROR_INVALID_ENTRY,
  G_TYPELIB_ERROR_INVALID_BLOB,
}
