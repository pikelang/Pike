//!
//! Validating XML parser.
//!
//! Validates an XML file according to a DTD.
//!
//! cf http://wwww.w3.org/TR/REC-xml/
//!
//! $Id: Validating.pike,v 1.20 2008/12/01 10:03:42 grubba Exp $
//!

#pike __REAL_VERSION__

//! Extends the Simple XML parser.
inherit .Simple;

protected private mapping(string:array(function)) __element_content = ([]);
protected private mapping(string:mapping(string:array)) __element_attrs = ([]);
mapping(string:string) __entity_sysid = ([]);
mapping(string:string) __entity_pubid = ([]);
mapping(string:string) __entity_ndata = ([]);
mapping(string:string) __notation_sysid = ([]);
mapping(string:string) __notation_pubid = ([]);
protected private multiset(string) __ids_used = (<>);
protected private multiset(string) __idrefs_used = (<>);
protected private multiset(string) __notations_used = (<>);

//! Check if @[s] is a valid @tt{Name@}.
int isname(string s)
{
  return sizeof(s) && .isfirstnamechar(s[0]) &&
    sizeof(filter(s[1..], .isnamechar)) == sizeof(s)-1;
}

//! Check if @[s] is a valid @tt{Nmtoken@}.
int isnmtoken(string s)
{
  return sizeof(s) &&
    sizeof(filter(s, .isnamechar)) == sizeof(s);
}

//! Check if @[s] is a valid list of @tt{Name@}s.
int isnames(string s)
{
  array(string) names = replace(s, ({"\t", "\r", "\n"}),
				({" ", " ", " "}))/" ";
  return sizeof(names) && names[0] != "" && names[-1] != "" &&
    search(map(names-({""}), isname), 0) < 0;
}

//! Check if @[s] is a valid list of @tt{Nmtoken@}s.
int isnmtokens(string s)
{
  array(string) nmtokens = replace(s, ({"\t", "\r", "\n"}),
				   ({" ", " ", " "}))/" ";
  return sizeof(nmtokens) && nmtokens[0] != "" && nmtokens[-1] != "" &&
    search(map(nmtokens-({""}), isnmtoken), 0) < 0;
}

protected private int islegalattribute(string val, array spec)
{
  switch(spec[0][0]) {
   case "":
   case "NOTATION":
     return search(spec[0][1..], val)>=0;
   case "ID":
   case "IDREF":
   case "ENTITY":
     return isname(val);
   case "IDREFS":
   case "ENTITIES":
     return isnames(val);
   case "NMTOKEN":
     return isnmtoken(val);
   case "NMTOKENS":
     return isnmtokens(val);
   case "CDATA":
     return 1;
  }
}

//! XML Element node.
protected class Element {

  string name;
  array(function) content_matcher;
  mapping(string:array) attributes;
  
  string _sprintf(int mode, mapping options)
  {
    return mode=='O' && sprintf("%O(%O)", this_program, name);
  }

  int accept_element(string name)
  {
    array(function) step = (content_matcher(name)-({0}))*({});
    if (!sizeof(step)) {
      return 0;
    }
    content_matcher = step;
    return 1;
  }

  void check_attributes(mapping(string:string) c_attrs,
			function(string, string, mixed ...:mixed) xmlerror)
  {
    foreach(indices(c_attrs), string name) {
      array spec = attributes[name];
      if(spec) {
	if(!islegalattribute(c_attrs[name], spec)) {
	  xmlerror(this_program::name,
		   "Invalid value for attribute %s: %O.", name, c_attrs[name]);
	}
	if(spec[1][0]=="#FIXED" && c_attrs[name]!=spec[1][1]) {
	  xmlerror(this_program::name,
		   "Attribute %s must have the value %s.", name, spec[1][1]);
	}
	switch(spec[0][0]) {
	 case "ID":
	   if(__ids_used[c_attrs[name]]) {
	     xmlerror(this_program::name,
		      "Id %O (sttribute %s) has already been used.",
		      c_attrs[name], name);
	   }
	   __ids_used[c_attrs[name]] = 1;
	   break;
	 case "IDREF":
	   __idrefs_used[c_attrs[name]] = 1;
	   break;
	 case "IDREFS":
	   __idrefs_used |=
	     mkmultiset(Array.uniq(replace(c_attrs[name],
					   ({"\t", "\r", "\n"}),
					   ({" ", " ", " "}) )/" "-({""})));
	   break;
	 case "ENTITY":
	   if(!__entity_ndata[c_attrs[name]]) {
	     xmlerror(this_program::name,
		      "Invalid value for attribute %s: %O.",
		      name, c_attrs[name]);
	   }
	   break;
	 case "ENTITIES":
	   if(search(rows(__entity_ndata, replace(c_attrs[name],
						  ({"\t", "\r", "\n"}),
						  ({" ", " ", " "}) )/" "-
			  ({""})), 0)>=0) {
	     xmlerror(this_program::name,
		      "Invalid value for attribute %s: %O.",
		      name, c_attrs[name]);
	   }
	   break;
	 case "NOTATION":
	   __notations_used[c_attrs[name]] = 1;
	   break;
	}
      } else if(name[..3]!="xml:") {
	xmlerror(this_program::name, "Undeclared attribute: %s.", name);
      }
    }
    foreach (filter(indices(attributes)-indices(c_attrs),
		    lambda(string a) {
		      return attributes[a][1][0] ==
			"#REQUIRED";
		    }), string attr) {
      xmlerror(this_program::name,
	       "Node %s is missing required attribute: %s.", name, attr);
    };
  }

  void create(string _name)
  {
    content_matcher = __element_content[name = _name];
    attributes = __element_attrs[name] || ([]);
  }
}

protected private array(object) __element_stack = ({});
protected private string __root_element_name;

//! Get an external entity.
//!
//! Called when a @tt{<!DOCTYPE>@} with a @tt{SYSTEM@} identifier
//! is encountered, or when an entity reference needs expanding.
//!
//! @param sysid
//!   The @tt{SYSTEM@} identifier.
//!
//! @param pubid
//!   The @tt{PUBLIC@} identifier (if any).
//!
//! @param info
//!   The callbackinfo mapping containing the current parser state.
//!
//! @param extra
//!   The @tt{extra@} arguments as passed to @[parse()] or @[parse_dtd()].
//!
//! @returns
//!   Returns a string with a DTD fragment on success.
//!   Returns @expr{0@} (zero) on failure.
//!
//! @note
//!   Returning zero will cause the validator to report an error.
//!
//! @note
//!   In Pike 7.7 and earlier @[info] had the value @expr{0@} (zero).
//!
//! @note
//!   The default implementation always returns @expr{0@} (zero).
//!   Override this function to provide other behaviour.
//!
//! @seealso
//!   @[parse()], @[parse_dtd()]
string get_external_entity(string sysid, string|void pubid,
			   mapping|__deprecated__(int)|void info,
			   mixed ... extra)
{
  // Override this function
  return 0;
}

protected private array(function) accept_terminate(string x)
{
  return !x && ({ accept_terminate });
}

protected private array(function) accept_any(string x)
{
  return ({ accept_any });
}

protected private array(function) compile_language(string|array l,
						array(function) c)
{
  if(stringp(l))
    return ({ lambda(string name) { return name == l && c; } });
  else switch(l[0]) {
   case "|":
     return map(l[1..], compile_language, c)*({});
   case ",":
     foreach(reverse(l[1..]), string|array e)
       c = compile_language(e, c);
     return c;
   case "*":
   case "+":
     array(function) body;
     body = compile_language(l[1], ({lambda(string x) {
				       return (body(x)+c(x)-({0}))*({});
				     }}));
     return (l[0]=="*"? body+c : body);
   case "?":
     return compile_language(l[1], c)+c;
   default:
     error("Internal error\n%O\n", l);
  }
}

protected private string normalize_uri(string uri, mapping info)
{  
  catch {
    // NB: We don't care about this error.
    //     If it's relevant, it will be thrown again below.
    return (string)Standards.URI(uri);
  };
  while (info && !info->context) {
    info = info->previous;
  }
  string base_uri;
  if (info) {
    if (has_prefix(info->context, "%")) {
      base_uri = __entity_sysid[info->context];
    } else {
      base_uri = info->context;
    }
  }
  uri = (string)Standards.URI(uri, base_uri);
  return uri;
}

//! The validation callback function.
//!
//! @seealso
//!   @[::parse()]
protected private mixed validate(string kind, string name, mapping attributes,
				 array|string contents,
				 mapping(string:mixed) info,
				 function(string,string,mapping,array|string,
					  mapping(string:mixed),
					  mixed ...:mixed) callback,
				 array(mixed) extra)
{
  // Helper...
  function(string, string, mixed ...:mixed) xmlerror =
    lambda(string tag, string msg, mixed ... args) {
      return callback("error", tag, 0, sprintf(msg, @args), info, @extra);
    };
  switch(kind) {
   case "<!DOCTYPE":
     __root_element_name = name;
     if(attributes->SYSTEM) {
       attributes->SYSTEM = normalize_uri(attributes->SYSTEM, info);
       string dtd=get_external_entity(attributes->SYSTEM, attributes->PUBLIC,
				      info, @extra);
       if(dtd)
	 parse_dtd(dtd, attributes->SYSTEM, callback, @extra);
       else
	 return xmlerror("!DOCTYPE", "External subset of DTD %O not found.",
			 attributes->SYSTEM);
     }
     break;
   case "<!ELEMENT":
     if(__element_content[name])
       return xmlerror("!ELEMENT",
		       "Element type %O declared more than once.", name);
     if(contents == "EMPTY")
       __element_content[name] = ({accept_terminate});
     else if(contents == "ANY")
       __element_content[name] = ({accept_any});
     else if (!contents) {
       __element_content[name] = ({accept_any});
       xmlerror("!ELEMENT", "Invalid element declatation for %O.", name);
     } else if(contents[0] == "#PCDATA") {
       if(sizeof(Array.uniq(contents)) != sizeof(contents))
	 return xmlerror("!ELEMENT",
			 "The same name must not appear more "
			 "than once in a mixed-content declaration (%s).",
			 name);
       __element_content[name] =
	 compile_language(({"*",({"|",""})+contents[1..]}),
			  ({accept_terminate}));
     } else
       __element_content[name] =
	 compile_language(contents, ({accept_terminate}));
     break;
   case "<!NOTATION":
     if(__notation_sysid[name] || __notation_pubid[name])
       return xmlerror("!NOTATION",
		       "More than one notation declaration for name %O.",
		       name);
     if(attributes->SYSTEM)
       __notation_sysid[name] = normalize_uri(attributes->SYSTEM, info);
     if(attributes->PUBLIC)
       __notation_pubid[name] = attributes->PUBLIC;
     break;
   case "<!ENTITY":
     if(attributes) {
       /* External entity,  Take note. */
       if(attributes->NDATA) {
	 __notations_used[attributes->NDATA] = 1;
	 __entity_ndata[name] = attributes->NDATA;
       }
       if(attributes->SYSTEM)
	 __entity_sysid[name] = normalize_uri(attributes->SYSTEM, info);
       if(attributes->PUBLIC)
	 __entity_pubid[name] = attributes->PUBLIC;
     }
     break;
   case "<!ATTLIST":
     if(!__element_attrs[name])
       __element_attrs[name] = ([]);
     foreach(indices(attributes), string attr)
       if(sizeof(attributes[attr])==2 && sizeof(attributes[attr][0])) {
	 if(attributes[attr][0][0] == "ID") {
	   if(search(column(column(values(__element_attrs[name]), 0), 0),
		     "ID")>=0)
	     xmlerror("!ATTLIST",
		      "Element %O has more than one ID attribute.", name);
	   if(attributes[attr][1][0] != "#IMPLIED" &&
	      attributes[attr][1][0] != "#REQUIRED") {
	     xmlerror("!ATTLIST",
		      "ID attribute must be #IMPLIED or #REQUIRED.");
	     attributes[attr][1][0] = "#REQUIRED";
	   }
	 }
	 if(sizeof(attributes[attr][1]) == 2 &&
	    !islegalattribute(attributes[attr][1][1], attributes[attr]))
	   xmlerror("!ATTLIST",
		    "Illegal default attribute value for %s:%s: %O.",
		    name, attr, attributes[attr][1][1]);
	 if(attributes[attr][0][0] == "NOTATION")
	   __notations_used |= mkmultiset(attributes[attr][0][1..]);
	 __element_attrs[name][attr] = attributes[attr];
       }
     break;
   case "<":
   case "<>":
     if(!__element_content[name]) {
       xmlerror(name, "Undeclared element: %O.", name);
       __element_content[name] = ({accept_any});
     }
     if(!sizeof(__element_stack)) {
       if(name != __root_element_name)
	 xmlerror(name, "Root element type mismatch: %s (expected %s).",
		  name, __root_element_name);
     } else {
       if(!__element_stack[-1]->accept_element(name)) {
	 int i;
	 // FIXME: What about the sequence mismatch case?
	 for (i=2; i <= sizeof(__element_stack); i++) {
	   if (__element_stack[-i]->accept_element(name)) break;
	 }
	 if (i <= sizeof(__element_stack)) {
	   int j;
	   for (j = 1; j < i; j++) {
	     xmlerror(name, "Missing end tag: </%s>.",
		      __element_stack[-j]->name);
	     // FIXME: Ought to signal the xml parser to pop contexts here.
	   }
	   __element_stack = __element_stack[..sizeof(__element_stack)-i];
	 } else {
	   xmlerror(name, "Invalid content for element <%s>: <%s>.",
		    __element_stack[-1]->name, name);
	 }
       }
     }

     Element e = Element(name);

     e->check_attributes(attributes, xmlerror);

     if(kind == "<") {
       callback(kind, name, attributes, contents, info, @extra);
       __element_stack += ({ e });
       return 0;
     } else
       if(!e->accept_element(0)) {
	 xmlerror(name, "Element %s may not be empty.", name);
       }
     break;
   case ">":
     if (!sizeof(__element_stack)) {
       return xmlerror(name,
		       "Unmatched end tag at the top level: </%s>.", name);
     } else if (__element_stack[-1]->name != name) {
       return xmlerror(name, "Unmatched end tag: </%s> (expected </%s>).",
		       name, __element_stack[-1]->name);
     }
     if(!__element_stack[-1]->accept_element(0))
       xmlerror(name, "Invalid content for element %s.", name);
     __element_stack = __element_stack[..<1];
     break;
   case "":
   case "<![CDATA[":
     if(!sizeof(__element_stack))
       return xmlerror(0, "All data must be inside tags");
     if(!__element_stack[-1]->accept_element("")) {
       if ((kind != "") || (contents-" "-"\t"-"\r"-"\n" != ""))
	 xmlerror(name, "Invalid content for element %s.", name);
       return 0;
     }
     break;
   case "%":
     name = "%"+name;
   case "&":
     if(!__entity_sysid[name])
       return 0;
     if(__entity_ndata[name]) {
       if (kind == "%") {
	 xmlerror(0, "Reference to unparsed entity: %s;.", name);
       } else {
	 xmlerror(0, "Reference to unparsed entity: %s%s;.", kind, name);
       }
     }
     if(attributes->in_attribute && kind!="%")
       xmlerror(0, "Reference to External entity %s%s; in attribute.",
		kind, name);
     return get_external_entity(__entity_sysid[name], __entity_pubid[name],
				info, @extra) ||
       xmlerror(0, "External entity %s (%O) not found.",
		name, __entity_sysid[name]);
  }
  return callback(kind, name, attributes, contents, info, @extra);
}

protected private mixed cleanup_parse(function(string,string,mapping
					    ,array|string,
					    mapping(string:mixed),
					    mixed ...:mixed) callback,
				   array(mixed) extra)
{
  if(sizeof(__idrefs_used - __ids_used)>0)
    return callback("error", 0, 0, "Unmatched IDREF \""+
		    indices(__idrefs_used - __ids_used)[0]+"\".",
		    ([]), @extra);

  if(sizeof(__notations_used -
	    mkmultiset(indices(__notation_sysid|__notation_pubid))))
    return callback("error", 0, 0,
		    "No declaration for notation \""+
		    indices(__notations_used -
			   mkmultiset(indices(__notation_sysid|
					      __notation_pubid)))[0]+
		    "\".", ([]), @extra);
}

//! @fixme
//!   Document this function
array parse(string data,
	    string|function(string, string, mapping, array|string,
			    mapping(string:mixed), mixed ...:mixed) callback,
	    mixed ... extra)
{
  // Note: No need for advanced argument checks here;
  //       ::parse() will check the arguments for us.
  if (stringp(callback) && sizeof(extra)) {
    string context = [string]callback;
    callback = [function]extra[0];
    extra = extra[1..];
    return ::parse(data, context, validate, callback, extra) +
      (({cleanup_parse(callback, extra)}) - ({0}));
  } else {
    return ::parse(data, validate, callback, extra) +
      (({cleanup_parse(callback, extra)}) - ({0}));
  }
}

//! @fixme
//!   Document this function
array parse_dtd(string data,
		string|function(string,string,mapping,array|string,
				mapping(string:mixed),mixed ...:mixed) callback,
		mixed ... extra)
{
  // Note: No need for advanced argument checks here;
  //       ::parse_dtd() will check the arguments for us.
  if (stringp(callback) && sizeof(extra)) {
    string context = [string]callback;
    callback = [function]extra[0];
    extra = extra[1..];
    return ::parse_dtd(data, context, validate, callback, extra);
  } else {
    return ::parse_dtd(data, validate, callback, extra);
  }
}

/* define_entity? */
