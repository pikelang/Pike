#pike 9.0

//! Pike 8.0 serialization interface without object boundary events.

//! @decl inherit Serializer : pre

//! @ignore
inherit Serializer.module : pre;
//! @endignore

protected function field_callback(function callback)
{
  return lambda(mixed value, string name, mixed symbol_type,
                mixed annotations, mixed inherited_annotations) {
    if (name != "\0begin" && name != "\0end")
      callback(value, name, symbol_type);
  };
}

class Serializable
{
  inherit pre::Serializable;

  protected void _serialize(object o, function serializer)
  {
    ::_serialize(o, field_callback(serializer));
  }

  protected void _deserialize(object o, function deserializer)
  {
    ::_deserialize(o, field_callback(deserializer));
  }

  protected void _serialize_variable(function serializer, mixed value,
                                     string name, mixed symbol_type)
  {
    serializer(value, name, symbol_type);
  }

  protected void _deserialize_variable(function deserializer, function setter,
                                       string name, mixed symbol_type)
  {
    ::_deserialize_variable(deserializer, setter, name, symbol_type,
                           UNDEFINED, UNDEFINED);
  }
}

void serialize(object o, function(mixed, string, mixed:void) serializer)
{
  pre::serialize(o, field_callback(serializer));
}

void deserialize(object o,
                 function(function(mixed:void), string, mixed:void) deserializer)
{
  pre::deserialize(o, field_callback(deserializer));
}
