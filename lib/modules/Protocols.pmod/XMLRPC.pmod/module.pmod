// XML-RPC by Fredrik Noring. Copyright © 2001 Roxen Internet Software AB.
// The implementation is based on specifications from http://xml-rpc.org/.

//! This module implements most features of the XML-RPC standard (see
//! @url{http://xml-rpc.org/@}).
//!
//! Translation rules for conversions from Pike datatypes to XML-RPC
//! datatypes:
//!
//! Pike @code{int@} is translated to XML-RPC @tt{<int>@}.
//! Pike @code{string@} is translated to XML-RPC @tt{<string>@}.
//! Pike @code{float@} is translated to XML-RPC @tt{<double>@}.
//! Pike @code{mapping@} is translated to XML-RPC @tt{<struct>@}.
//! Pike @code{array@} is translated to XML-RPC @tt{<array>@}.
//!
//! Translation rules for conversions from XML-RPC datatypes to Pike
//! datatypes:
//!
//! XML-RPC @tt{<i4>@}, @tt{<int>@} and @tt{<boolean>@} are
//!   translated to Pike @code{int@}.
//! XML-RPC @tt{<string>@} and @tt{<base64>@} are translated to
//!   Pike @code{string@}.
//! XML_RPC @tt{<double>@} is translated to Pike @code{float@}.
//! XML-RPC @tt{<struct>@} is translated to Pike @code{mapping@}.
//! XML-RPC @tt{<array>@} is translated to Pike @code{array@}.
//! XML-RPC @tt{<dateTime.iso8601>@} is translated to Pike Calendar object.
//!
//! @note
//! The XML-RPC @tt{<dateTime.iso8601>@} datatype is currently only
//! partially implemented. It is decoded but cannot be encoded. Also,
//! the code here does not assume any particular timezone (which is
//! correct according to the specification). The Calendar module,
//! however, seems to assume localtime.
//!

//! Represents a function call made to a XML-RPC server.
//!
//! @seealso
//! @[XMLRPC.decode_call()]
class Call(string method_name, array params)
{
  //! @decl int method_name
  //! Represents @tt{<methodName>@} in the XML-RPC standard.

  //! @decl array params
  //! Represents @tt{<params>@} in the XML-RPC standard where all
  //! datatypes have been converted to equivalent or similar datatypes
  //! in Pike.

  string _sprintf()
  {
    return sprintf("Protocols.XMLRPC.Call(%O, %d param%s)",
		   method_name, sizeof(params), sizeof(params) == 1 ? "" : "s");
  }
}

//! Represents a fault response which can be one of the return values
//! from a XML-RPC function call.
//!
//! @seealso
//! @[XMLRPC.decode_response()]
class Fault(int fault_code, string fault_string)
{
  //! @decl int fault_code
  //! Represents @tt{faultCode@} in the XML-RPC standard.

  //! @decl int fault_string
  //! Represents @tt{faultString@} in the XML-RPC standard.

  string _sprintf()
  {
    return sprintf("Protocols.XMLRPC.Fault(%O, %O)", fault_code, fault_string);
  }
}

//! Decodes a XML-RPC representation of a function call and returns a
//! @[XMLRPC.Call] object.
//!
//! @seealso
//! @[XMLRPC.Call]
Call decode_call(string xml_input)
{
  array r = decode(xml_input, call_dtd);
  return Call(r[0], r[1]);
}

//! Decodes a XML-RPC representation of a response and returns an
//! array containing response values, or a @[Fault] object.
//!
//! @seealso
//! @[XMLRPC.Fault]
array|Fault decode_response(string xml_input)
{
  array|mapping r = decode(xml_input, response_dtd);
  if(arrayp(r))
    return r;
  return Fault(r->faultCode, r->faultString);
}

//! Encodes a function call with the name @[name] and the arguments
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

static constant common_dtd_fragment = #"
  <!ELEMENT params  (param*)>
  <!ELEMENT param   (value)>

  <!ELEMENT value   (boolean|i4|int|double|string|base64|
                     array|struct|dateTime.iso8601)>

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

static constant call_dtd = #"
<!DOCTYPE methodCall [
  <!ELEMENT methodCall (methodName, params)>
  <!ELEMENT methodName (#PCDATA)>
"+common_dtd_fragment+#"]>
";

static constant response_dtd = #"
<!DOCTYPE methodResponse [
  <!ELEMENT methodResponse (params|fault)>
  <!ELEMENT fault          (value)>
"+common_dtd_fragment+#"]>
";

static mixed decode(string xml_input, string dtd_input)
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
			 case "![CDATA[":
			   return data;
			 case "<":
			   return 0;
			 case "<>":
			 case ">":
			   switch(name)
			   {
			     case "methodResponse":
			     case "param":
			     case "value":
			     case "array":
			     case "fault":
			       return data[0];
			     case "i4":
			     case "int":
			     case "boolean":
			       return (int)(data*"") || magic_zero;
			     case "double":
			       return (float)(data*"");
			     case "string":
			     case "name":
			     case "methodName":
			       return data*"";
			     case "base64":
			       return MIME.decode_base64(data*"");
			     case "methodCall":
			     case "params":
			     case "member":
			     case "data":
			       return data;
			     case "struct":
			       return mkmapping(@Array.transpose(data));
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
  return tree[0];
}

static string xml_encode_string(string s)
{
  return replace(s,
		 ({ "&", "<", ">", "\"", "\'", "\000" }),
		 ({ "&amp;", "&lt;", "&gt;", "&#34;", "&#39;", "&#0;" }));
}

static string encode(int|float|string|mapping|array value)
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
  else
    error("Cannot encode %O.\n", value);
  return r+"</value>\n";
}

static string encode_params(array params)
{
  string r = "<params>\n";
  foreach(params, mixed param)
    r += "<param>\n"+encode(param)+"</param>\n";
  return r+"</params>\n";
}

class Client(string|Standards.URI url)
{
  //! This class implements an XML-RPC client that uses HTTP transport.
  //!
  //! @example
  //!   > Protocols.XMLRPC.Client client = Protocls.XMLRPC.Client("http://www.oreillynet.com/meerkat/xml-rpc/server.php");
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
  
  mixed `[](string call)
  {
    return lambda(mixed ... args)
	   {
	     object c=Protocols.HTTP.do_method("POST",
					       url,
					       0,
					       ([ "Content-Type":"text/xml"]),
					       0,
					       encode_call( call, args ));
	     return Protocols.XMLRPC.decode_response(c->data());
	   };
  }

  string _sprintf()
  {
    return sprintf("Protocols.XMLRPC.Client(%O)",
		   url);
  }

  
}
