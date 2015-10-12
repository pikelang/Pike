#pike __REAL_VERSION__

//! Various authentication modules and classes.

// Checks if A is an instance of B (either directly or by inheritance)
#define INSTANCE_OF(A,B) (object_program((A)) == object_program((B)) || \
                          Program.inherits(object_program((A)),         \
                          object_program(B)))

//! Parameter collection class
//!
//! @seealso
//!  @[Param]
class Params
{
  //! The parameters.
  protected array(Param) params;

  //! Creates a new instance of @[Params]
  //!
  //! @param args
  protected void create(Param ... args)
  {
    params = args||({});
  }

  //! Sign the parameters
  //!
  //! @param secret
  //!  The API secret
  string sign(string secret)
  {
    return md5(sort(params)->name_value()*"" + secret);
  }

  //! Parameter keys
  protected array(string) _indices()
  {
    return params->get_name();
  }

  //! Parameter values
  protected array(string) _values()
  {
    return params->get_value();
  }

  //! Returns the array of @[Param]eters
  array(Param) get_params()
  {
    return params;
  }

  string to_unencoded_query()
  {
    return params->name_value()*"&";
  }

  //! Turns the parameters into a query string
  string to_query()
  {
    array o = ({});
    foreach (params, Param p)
      o += ({ urlencode(p->get_name()) + "=" + urlencode(p->get_value()) });

    return o*"&";
  }

  //! Turns the parameters into a mapping
  mapping(string:mixed) to_mapping()
  {
    return mkmapping(params->get_name(), params->get_value());
  }

  //! Add a mapping of key/value pairs to the current instance
  //!
  //! @param value
  //!
  //! @returns
  //!  The object being called
  this_program add_mapping(mapping value)
  {
    foreach (value; string k; mixed v)
      params += ({ Param(k, (string)v) });

    return this;
  }

  //! Add @[p] to the array of @[Param]eters
  //!
  //! @param p
  //!
  //! @returns
  //!  A new @[Params] object
  protected this_program `+(Param|this_program p)
  {
    if (mappingp(p)) {
      foreach (p; string k; string v) {
        params += ({ Param(k, v) });
      }

      return this;
    }

    if (INSTANCE_OF(p, this))
      params += p->get_params();
    else
      params += ({ p });

    return this;
  }

  //! Remove @[p] from the @[Param]eters array of the current object.
  //!
  //! @param p
  protected this_program `-(Param|this_program p)
  {
    if (!p) return this;

    array(Param) the_params;
    if (INSTANCE_OF(p, this))
      the_params = p->get_params();
    else
      the_params = ({ p });

    return object_program(this)(@(params-the_params));
  }

  //! Index lookup
  //!
  //! @param key
  //! The name of a @[Param]erter to find.
  protected Param `[](string key)
  {
    foreach (params, Param p)
      if (p->get_name() == key)
        return p;

    return 0;
  }

  //! Clone the current instance
  this_program clone()
  {
    return object_program(this)(@params);
  }

  //! String format method
  //!
  //! @param t
  protected string _sprintf(int t)
  {
    return t == 'O' && sprintf("%O(%O)", object_program(this), params);
  }

  //! Casting method
  //!
  //! @param how
  protected mixed cast(string how)
  {
    switch (how) {
      case "mapping": return to_mapping();
      case "string": return to_query();
    }
    error("Cant cast %O() to %O\n", this_program, how);
  }
}

//! Representation of a parameter.
//!
//! Many Social web services use a RESTful communication and have similiar
//! API's. This class is suitable for many RESTful web services and if this
//! class doesn't suite a particular service, just inherit this class and
//! rewrite the behaviour where needed.
//!
//! @seealso
//!  @[Params]
class Param
{
  //! The name of the parameter
  protected string name;

  //! The value of the parameter
  protected string value;

  //! Creates a new instance of @[Param]
  //!
  //! @param _name
  //! @param _value
  protected void create(string _name, mixed _value)
  {
    name = _name;
    low_set_value((string)_value);
  }

  //! Getter for the parameter name
  string get_name()
  {
    return name;
  }

  //! Setter for the parameter name
  //!
  //! @param _name
  void set_name(string _name)
  {
    name = _name;
  }

  //! Getter for the parameter value
  string get_value()
  {
    return value;
  }

  //! Setter for the parameter value
  //!
  //! @param _value
  void set_value(mixed _value)
  {
    low_set_value((string)_value);
  }

  //! Returns the name and value as querystring key/value pair
  string name_value()
  {
    return name + "=" + value;
  }

  //! Same as @[name_value()] except this URL encodes the value.
  string name_value_encoded()
  {
    return urlencode(name) + "=" + urlencode(value);
  }

  //! Comparer method. Checks if @[other] equals this object
  //!
  //! @param other
  protected int(0..1) `==(mixed other)
  {
    if (!INSTANCE_OF(this, other)) return 0;
    if (name == other->get_name())
      return value == other->get_value();

    return 0;
  }

  //! Checks if this object is greater than @[other]
  //!
  //! @param other
  protected int(0..1) `>(mixed other)
  {
    if (!INSTANCE_OF(this, other)) return 0;
    if (name == other->get_name())
      return value > other->get_value();

    return name > other->get_name();
  }

  //! Checks if this object is less than @[other]
  //!
  //! @param other
  protected int(0..1) `<(mixed other)
  {
    if (!INSTANCE_OF(this, other)) return 0;
    if (name == other->get_name())
      return value < other->get_value();

    return name < other->get_name();
  }

  //! String format method
  //!
  //! @param t
  protected string _sprintf(int t)
  {
    return t == 'O' && sprintf("%O(%O,%O)", object_program(this), name, value);
  }

  protected mixed cast(string how)
  {
    switch (how)
    {
      case "string": return name_value_encoded();
    }

    error("Cant cast %O() to %O\n", this_program, how);
  }

  //! Makes sure @[v] to set as @[value] is in UTF-8 encoding
  //!
  //! @param v
  private void low_set_value(string v)
  {
    value = v;

    if (String.width(value) < 8) {
      if (mixed e = catch(value = string_to_utf8(value))) {
        werror("Warning: Auth.low_set_value(%O): string_to_utf8() failed. "
               "Already encoded?\n%s\n", value, describe_error(e));
      }
    }
  }
}

//! Same as @[Protocols.HTTP.uri_encode()] except this turns spaces into
//! @tt{+@} instead of @tt{%20@}.
//!
//! @param s
string urlencode(string s)
{
#if constant(Protocols.HTTP.uri_encode)
  return Protocols.HTTP.uri_encode(s);
#elif constant(Protocols.HTTP.http_encode_string)
  return Protocols.HTTP.http_encode_string(s);
#endif
}

//! Same as @[Protocols.HTTP.uri_decode()] except this turns spaces into
//! @tt{+@} instead of @tt{%20@}.
//!
//! @param s
string urldecode(string s)
{
#if constant(Protocols.HTTP.uri_decode)
  return Protocols.HTTP.uri_decode(s);
#elif constant(Protocols.HTTP.http_decode_string)
  return Protocols.HTTP.http_decode_string(s);
#else
  return s;
#endif
}

//! Turns a query string into a mapping
//!
//! @param query
mapping query_to_mapping(string query)
{
  mapping m = ([]);
  if (!query || !sizeof(query))
    return m;

  if (query[0] == '?')
    query = query[1..];

  foreach (query/"&", string p) {
    sscanf (p, "%s=%s", string k, string v);
    m[k] = urldecode(v);
  }

  return m;
}

//! MD5 routine
//!
//! @param s
string md5(string s)
{
#if constant(Crypto.MD5)
  return String.string2hex(Crypto.MD5.hash(s));
#else
  return Crypto.string_to_hex(Crypto.md5()->update(s)->digest());
#endif
}
