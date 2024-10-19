#pike __REAL_VERSION__

// XML-RPC by Fredrik Noring.
// The implementation is based on specifications from http://xml-rpc.org/.

//! This module implements most features of the XML-RPC standard (see
//! @url{http://xml-rpc.org/@}).
//!
//! Translation rules for conversions from Pike datatypes to XML-RPC
//! datatypes:
//!
//! Pike @expr{int@} is translated to XML-RPC @tt{<int>@}.
//! Pike @expr{string@} is translated to XML-RPC @tt{<string>@}.
//! Pike @expr{float@} is translated to XML-RPC @tt{<double>@}.
//! Pike @expr{mapping@} is translated to XML-RPC @tt{<struct>@}.
//! Pike @expr{array@} is translated to XML-RPC @tt{<array>@}.
//! Pike @[Calendar] object is translated to XML-RPC @tt{<dateTime.iso8601@}.
//! Pike @expr{Val.false@} and @expr{Val.true@} is translated to
//!   XML-RPC @tt{<boolean>@}.
//!
//! Translation rules for conversions from XML-RPC datatypes to Pike
//! datatypes:
//!
//! XML-RPC @tt{<i4>@} and @tt{<int>@} are translated to Pike @expr{int@}.
//! XML-RPC @tt{<boolean>@} is translated to Pike @expr{Val.true@} and
//!   @expr{Val.false@}.
//! XML-RPC @tt{<string>@} and @tt{<base64>@} are translated to
//!   Pike @expr{string@}.
//! XML_RPC @tt{<double>@} is translated to Pike @expr{float@}.
//! XML-RPC @tt{<struct>@} is translated to Pike @expr{mapping@}.
//! XML-RPC @tt{<array>@} is translated to Pike @expr{array@}.
//! XML-RPC @tt{<dateTime.iso8601>@} is translated to Pike @[Calendar] object.
//!
//! @note
//! The XML-RPC @tt{<dateTime.iso8601>@} datatype does not assume any
//! time zone, but local time is always used in the conversion to
//! @[Calendar] objects.
//!

//! Represents a function call made to a XML-RPC server.
//!
//! @seealso
//! @[decode_call()]
class Call(string method_name, array params)
{
  //! @decl int method_name
  //! Represents @tt{<methodName>@} in the XML-RPC standard.

  //! @decl array params
  //! Represents @tt{<params>@} in the XML-RPC standard where all
  //! datatypes have been converted to equivalent or similar datatypes
  //! in Pike.

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%O, %d param%s)", this_program,
			     method_name, sizeof(params),
			     sizeof(params) == 1 ? "" : "s");
  }
}

//! Represents a fault response which can be one of the return values
//! from a XML-RPC function call.
//!
//! @seealso
//! @[decode_response()]
class Fault(int fault_code, string fault_string)
{
  //! @decl int fault_code
  //! Represents @tt{faultCode@} in the XML-RPC standard.

  //! @decl int fault_string
  //! Represents @tt{faultString@} in the XML-RPC standard.

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%O, %O)", this_program,
			     fault_code, fault_string);
  }
}

//! Decodes a XML-RPC representation of a function call and returns a
//! @[Call] object.
//!
//! @seealso
//! @[Call]
Call decode_call(string xml_input)
{
  array r = decode(xml_input, call_dtd);
  return Call(r[0], r[1]);
}

//! Decodes a XML-RPC representation of a response and returns an
//! array containing response values, or a @[Fault] object.
//!
//! @seealso
//! @[Fault]
array|Fault decode_response(string xml_input, int|void boolean)
{
  array|mapping r = decode(xml_input, response_dtd, boolean);
  if(arrayp(r))
    return r;
  return Fault(r->faultCode, r->faultString);
}

//! Encodes a function call with the name @[method_name] and the arguments
//! @[params] and returns the XML-RPC string representation.
string encode_call(string method_name, array params)
{
  return
    "<?xml version=\"1.0\"?>\n"
    "<methodCall>\n"
    "<methodName>"+xml_encode_string(method_name)+"</methodName>\n"
    +encode_params(params)+
    "</methodCall>\n";
}

//! Encodes a response containing the multiple values in @[params] and
//! returns the XML-RPC string representation.
string encode_response(array params)
  // Returns the XMLRPC XML representation of a response.
{
  return
    "<?xml version=\"1.0\"?>\n"
    "<methodResponse>\n"+encode_params(params)+"</methodResponse>\n";
}

//! Encodes a response fault containing a @[fault_code] and a
//! @[fault_string] and returns the XML-RPC string representation.
string encode_response_fault(int fault_code, string fault_string)
  // Returns the XMLRPC XML representation of a fault.
{
  return
    "<?xml version=\"1.0\"?>\n"
    "<methodResponse>\n<fault>\n"
    +encode(([ "faultCode":fault_code, "faultString":fault_string ]))+
    "</fault>\n</methodResponse>\n";
}

// Internal stuff below.

protected constant common_dtd_fragment = #"
  <!ELEMENT params  (param*)>
  <!ELEMENT param   (value)>

  <!ELEMENT value   (#PCDATA|boolean|i4|int|double|string|base64|
                     array|struct|dateTime.iso8601)*>

  <!ELEMENT boolean          (#PCDATA)>
  <!ELEMENT i4               (#PCDATA)>
  <!ELEMENT int              (#PCDATA)>
  <!ELEMENT double           (#PCDATA)>
  <!ELEMENT string           (#PCDATA)>
  <!ELEMENT base64           (#PCDATA)>
  <!ELEMENT dateTime.iso8601 (#PCDATA)>

  <!ELEMENT array   (data)>
  <!ELEMENT data    (value*)>

  <!ELEMENT struct  (member*)>
  <!ELEMENT member  (name,value)>
  <!ELEMENT name    (#PCDATA)>
";

protected constant call_dtd = #"
<!DOCTYPE methodCall [
  <!ELEMENT methodCall (methodName, params)>
  <!ELEMENT methodName (#PCDATA)>
"+common_dtd_fragment+#"]>
";

protected constant response_dtd = #"
<!DOCTYPE methodResponse [
  <!ELEMENT methodResponse (params|fault)>
  <!ELEMENT fault          (value)>
"+common_dtd_fragment+#"]>
";

// One more fix because some people found the specs too easy and
// decided that you can have <value>test</value>
// (that is omitting string inside a value).
protected class StringWrap(string s){};

// Same as magic_zero below
object magic_false = class {}();

// Replace all magic_false with Val.false if necessary
void replace_magic_false(array|mapping a) {
  replace(a,magic_false,Val.false);
  foreach (a;; mixed b) {
    if (mappingp(b) || arrayp(b))
      replace_magic_false(b);
  }
}

protected mixed decode(string xml_input, string dtd_input, int|void boolean)
{
  // We cannot insert 0 integers directly into the parse tree, so
  // we'll use magic_zero as a placeholder and destruct it afterwards.
  object magic_zero = class {}();

  Parser.XML.Validating xml = Parser.XML.Validating();
  array tree = xml->
	       parse(xml_input,
		     lambda(string type,
			    string name,
			    mapping(string:string) attr,
			    mixed data,
			    mixed loc,
			    mixed ... extra)
		     {
		       switch(type)
		       {
			 case "<?xml":
			   xml->parse
			     (dtd_input, lambda(mixed ... args) { return 0; });
			   return 0;
			 case "":
			 case "<![CDATA[":
			   return data;
			 case "<":
			   return 0;
			 case "<>":
			 case ">":
			   switch(name)
			   {
			     case "methodResponse":
			     case "param":
			     case "array":
			     case "fault":
			       return data[0];
			     case "value":
			       foreach(data, mixed value)
			         if(!stringp(value))
				 {
				   if(objectp(value) && value->s)
				     return value->s;
				   return value;
				 }
			       return sizeof(data)?data[0]:"";
			     case "i4":
			     case "int":
			       return (int)(data*"") || magic_zero;
			     case "boolean":
			       if (!boolean)
				 return (int)(data*"") || magic_zero;
			       return (int)(data*"")?Val.true:magic_false;
			     case "double":
			       return (float)(data*"");
			     case "string":
			       return StringWrap(data ? data*"" : "");
			     case "name":
			     case "methodName":
			       return data*"";
			     case "base64":
			       return StringWrap(MIME.decode_base64(data*""));
			     case "methodCall":
			     case "params":
			     case "member":
			     case "data":
			       return data;
			     case "struct":
			       return sizeof (data) ?
				 mkmapping(@Array.transpose(data)) : ([]);
		             case "dateTime.iso8601":
			       // FIXME: Don't assume any particular
			       // timezone here (the Calendar module
			       // seems to assume localtime).
			       return Calendar.parse("%dT%h:%m:%s", data*"") ||
				 magic_zero;
			   }
			   error("Unknown element %O.\n", name);
			 case "error":
			   if(loc && mappingp(loc))
			     error(data+" (location "+loc->location+": "+
				   sprintf("%O", xml_input[loc->location..
							   loc->location+20])+
				   "...)\n");
			   else
			     error(data+"\n");
		       }
		       return 0;
		     });
  destruct(magic_zero);   // Apply Magic! Replace magic_zero with real 0:s.

  if (boolean) {
    // extra magic, change magic_false to Val.false
    replace_magic_false(tree);
  }
  return tree[0];
}

protected string xml_encode_string(string s)
{
  return replace(s,
		 ({ "&", "<", ">", "\"", "\'", "\000" }),
		 ({ "&amp;", "&lt;", "&gt;", "&#34;", "&#39;", "&#0;" }));
}

protected string encode(int|float|string|mapping|array|object value)
{
  string r = "<value>";
  if(intp(value))
    r += sprintf("<int>%d</int>", value);
  else if(stringp(value))
    r += "<string>"+xml_encode_string(value)+"</string>";
  else if(floatp(value))
    r += sprintf("<double>%.8f</double>", value);
  else if(mappingp(value))
  {
    r += "\n<struct>";
    foreach(indices(value), string name)
      r += "<member>"
	   "<name>"+xml_encode_string(name)+"</name>"
	   +encode(value[name])+
	   "</member>\n";
    r += "</struct>\n";
  }
  else if(arrayp(value))
  {
    r += "\n<array>\n<data>\n";
    foreach(indices(value), string name)
      r += encode(value[name]);
    r += "</data>\n</array>\n";
  }
  else if (objectp (value) && value->format_iso_short)
    r += "<dateTime.iso8601>" + value->format_iso_short() +
      "</dateTime.iso8601>";
  else if (objectp(value) && (value->is_val_true || value->is_val_false))
    r += sprintf("<boolean>%d</boolean>",(int)value);
  else
    error("Cannot encode %O.\n", value);
  return r+"</value>\n";
}

protected string encode_params(array params)
{
  string r = "<params>\n";
  foreach(params, mixed param)
    r += "<param>\n"+encode(param)+"</param>\n";
  return r+"</params>\n";
}

//! This class implements an XML-RPC client that uses HTTP transport.
//!
//! There is an optional boolean flag to get the new behavior of
//! booleans being returned as Val instead of ints.
//!
//! @example
//! @pre{
//!   > Protocols.XMLRPC.Client client = Protocols.XMLRPC.Client("http://www.oreillynet.com/meerkat/xml-rpc/server.php");
//!   Result: Protocols.XMLRPC.Client("http://www.oreillynet.com/meerkat/xml-rpc/server.php");
//!   > client["system.listMethods"]();
//!   Result: ({ /* 1 element */
//!  		    ({ /* 9 elements */
//!  			"meerkat.getChannels",
//!  			"meerkat.getCategories",
//!  			"meerkat.getCategoriesBySubstring",
//!  			"meerkat.getChannelsByCategory",
//!  			"meerkat.getChannelsBySubstring",
//!  			"meerkat.getItems",
//!  			"system.listMethods",
//!  			"system.methodHelp",
//!  			"system.methodSignature"
//!  		    })
//!  		})
//! @}
class Client(string|Standards.URI url, int|void boolean)
{

  protected function `[](string call)
  {
    return lambda(mixed ... args)
	   {
	     Protocols.HTTP.Query c=Protocols.
	       HTTP.do_method("POST", url, 0, ([ "Content-Type":"text/xml"]),
			      0, encode_call( call, args ));
	     if(!c) error("Could not connect to %O\n", url);
	     if (c->status != 200)
	       // The xml-rpc spec says "Unless there's a lower-level
	       // error, always return 200 OK."
	       error ("Got invalid return code %d from %O: %O\n%O", c->status,
		      url, c->status_desc, c->data());
	     return decode_response(c->data(),boolean);
	   };
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%O)", this_program, url);
  }
}

//! This class implements an XML-RPC client that uses HTTP transport using
//! non blocking sockets.
//!
//! There is an optional boolean flag to get the new behavior of
//! booleans being returned Val instead of ints.
//!
//! @example
//! @pre{void data_ok(mixed result)
//!{
//!  write("result=%O\n", result);
//!}
//!
//!void fail()
//!{
//!  write("fail\n");
//!}
//!
//!int main(int argc, array argv)
//!{
//!  object async_client = Protocols.XMLRPC.AsyncClient("http://www.oreillynet.com/meerkat/xml-rpc/server.php");
//!  async_client["system.listMethods"](data_ok, fail);
//!  return -1;
//!@}
class AsyncClient
{
  protected Protocols.HTTP.Session http_session;
  protected string _url;
  protected int _boolean;

  protected void create(string|Standards.URI|Protocols.HTTP.Session.SessionURL url, int|void boolean)
  {
    _url = url;
    _boolean = boolean;
     http_session = Protocols.HTTP.Session();
  }

  protected void _data_ok(function user_data_ok, object request)
  {
    mixed result;
    if(request) {
      if (request->status() == 200)
	// The xml-rpc spec says "Unless there's a lower-level error,
	// always return 200 OK."
	result = decode_response(request->data(),_boolean);
    }
    user_data_ok(result);
  }

  protected function `[](string call)
  {
     return lambda(function data_ok, function fail, mixed ...args)
     {
       http_session->async_do_method_url(
           "POST",
           _url,
           0,
           encode_call( call, args ),
           ([ "content-type":"text/xml"]),
           0,
           Function.curry(_data_ok)(data_ok),
           fail,
           ({ }));
     };
  }
}
