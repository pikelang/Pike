// XML-RPC by Fredrik Noring. Copyright © 2001 Roxen Internet Software AB.
// The Implementation is based on specifications from http://xml-rpc.org/.

class Call(string name, array args)
{
  string _sprintf()
  {
    return sprintf("Protocols.XMLRPC.Call(%O, ...)", name);
  }
}

class Fault(int code, string reason)
{
  string _sprintf()
  {
    return sprintf("Protocols.XMLRPC.Fault(%O, %O)", code, reason);
  }
}

Call decode_call(string xml_input)
  // Returns an array where the first element is the function name and
  // the rest of the elements are the function arguments.
{
  array r = decode(xml_input, call_dtd);
  return Call(r[0], r[1..]);
}

array|Fault decode_response(string xml_input)
  // Returns an array with reponse values, or a mapping representing
  // an error message. The mapping contains a "faultCode" integer and
  // a "faultString" message according to the XMLRPC protocol.
{
  array|mapping r = decode(xml_input, response_dtd);
  if(arrayp(r))
    return r;
  return Fault(r->faultCode, r->faultString);
}

string encode_call(string name, array args)
  // Returns the XMLRPC XML representation of a function call.
{
  return
    "<?xml version=\"1.0\"?>\n"
    "<methodCall>\n"
    "<methodName>"+xml_encode_string(name)+"</methodName>\n"
    +encode_params(args)+
    "</methodCall>\n";
}

string encode_response(array args)
  // Returns the XMLRPC XML representation of a response.
{
  return
    "<?xml version=\"1.0\"?>\n"
    "<methodResponse>\n"+encode_params(args)+"</methodResponse>\n";
}

string encode_response_fault(int code, string reason)
  // Returns the XMLRPC XML representation of a fault.
{
  return
    "<?xml version=\"1.0\"?>\n"
    "<methodResponse>\n<fault>\n"
    +encode(([ "faultCode":code, "faultString":reason ]))+
    "</fault>\n</methodResponse>\n";
}

// Internal stuff below.

static constant common_dtd_fragment = #"
  <!ELEMENT params  (param*)>
  <!ELEMENT param   (value)>

  <!ELEMENT value   (boolean|i4|int|double|string|base64|array|struct)>

  <!ELEMENT boolean (#PCDATA)>
  <!ELEMENT i4      (#PCDATA)>
  <!ELEMENT int     (#PCDATA)>
  <!ELEMENT double  (#PCDATA)>
  <!ELEMENT string  (#PCDATA)>
  <!ELEMENT base64  (#PCDATA)>

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
    r += sprintf("<double>%g</double>", value);
  else if(mappingp(value))
  {
    r += "\n<struct>";
    foreach(indices(value), string name)
      r += "<member>"
	   "<name>"+xml_encode_string(name)+"</name>"
	   "<value>"+encode(value[name])+"</value>"
	   "</member>\n";
    r += "<struct>\n";
  }
  else if(arrayp(value))
  {
    r += "\n<array>\n<data>\n";
    foreach(indices(value), string name)
      r += "<value>"+encode(value[name])+"</value>\n";
    r += "</data>\n<array>\n";
  }
  else
    error("Cannot encode %O.\n", value);
  return r+"</value>\n";
}

static string encode_params(array(mixed) params)
{
  string r = "<params>\n";
  foreach(params, mixed param)
    r += "<param>\n"+encode(param)+"</param>\n";
  return r+"</params>\n";
}
