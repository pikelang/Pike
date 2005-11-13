#pike __REAL_VERSION__

inherit .Simple;

static private mapping(string:array(function)) __element_content = ([]);
static private mapping(string:mapping(string:array)) __element_attrs = ([]);
mapping(string:string) __entity_sysid = ([]);
mapping(string:string) __entity_pubid = ([]);
mapping(string:string) __entity_ndata = ([]);
mapping(string:string) __notation_sysid = ([]);
mapping(string:string) __notation_pubid = ([]);
static private multiset(string) __ids_used = (<>);
static private multiset(string) __idrefs_used = (<>);
static private multiset(string) __notations_used = (<>);

static int isname(string s)
{
  return sizeof(s) && .isfirstnamechar(s[0]) &&
    sizeof(filter(s[1..], .isnamechar)) == sizeof(s)-1;
}

static int isnmtoken(string s)
{
  return sizeof(s) &&
    sizeof(filter(s, .isnamechar)) == sizeof(s);
}

static int isnames(string s)
{
  array(string) names = replace(s, ({"\t", "\r", "\n"}),
				({" ", " ", " "}))/" ";
  return sizeof(names) && names[0] != "" && names[-1] != "" &&
    search(map(names-({""}), isname), 0) < 0;
}

static int isnmtokens(string s)
{
  array(string) nmtokens = replace(s, ({"\t", "\r", "\n"}),
				   ({" ", " ", " "}))/" ";
  return sizeof(nmtokens) && nmtokens[0] != "" && nmtokens[-1] != "" &&
    search(map(nmtokens-({""}), isnmtoken), 0) < 0;
}

static private int islegalattribute(string val, array spec)
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

static private class Element {

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
    if(sizeof(step)) {
      content_matcher = step;
      return 1;
    } else
      return 0;
  }

  string check_attributes(mapping(string:string) c_attrs)
  {
    foreach(indices(c_attrs), string name) {
      array spec = attributes[name];
      if(spec) {
	if(!islegalattribute(c_attrs[name], spec))
	  return name;
	if(spec[1][0]=="#FIXED" && c_attrs[name]!=spec[1][1])
	  return name;
	switch(spec[0][0]) {
	 case "ID":
	   if(__ids_used[c_attrs[name]])
	     return name;
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
	   if(!__entity_ndata[c_attrs[name]])
	     return name;
	   break;
	 case "ENTITIES":
	   if(search(rows(__entity_ndata, replace(c_attrs[name],
						  ({"\t", "\r", "\n"}),
						  ({" ", " ", " "}) )/" "-
			  ({""})), 0)>=0)
	     return name;
	   break;
	 case "NOTATION":
	   __notations_used[c_attrs[name]] = 1;
	   break;
	}
      } else
	if(name[..3]!="xml:")
	  return name;
    }
    array(string) missing_req = filter(indices(attributes)-indices(c_attrs),
				       lambda(string a) {
					 return attributes[a][1][0] ==
					   "#REQUIRED";
				       });
    if(sizeof(missing_req))
      return missing_req[0];
  }

  void create(string _name)
  {
    content_matcher = __element_content[name = _name];
    attributes = __element_attrs[name] || ([]);
  }
}

static private array(object) __element_stack = ({});
static private string __root_element_name;

string get_external_entity(string sysid, string|void pubid, int|void unparsed,
			   mixed ... extra)
{
  // Override this function
  return 0;
}

static private array(function) accept_terminate(string x)
{
  return !x && ({ accept_terminate });
}

static private array(function) accept_any(string x)
{
  return ({ accept_any });
}

static private array(function) compile_language(string|array l,
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

static private mixed validate(string kind, string name, mapping attributes,
			      array|string contents,
			      mapping(string:mixed) info,
			      function(string,string,mapping,array|string,
				       mapping(string:mixed),
				       mixed ...:mixed) callback,
			      array(mixed) extra)
{
  switch(kind) {
   case "<!DOCTYPE":
     __root_element_name = name;
     if(attributes->SYSTEM) {
       string dtd=get_external_entity(attributes->SYSTEM, attributes->PUBLIC,
				      0, @extra);
       if(dtd)
	 parse_dtd(dtd, callback, @extra);
       else
	 return callback("error", 0, 0, "External subset of DTD not found.",
			 info, @extra);
     }
     break;
   case "<!ELEMENT":
     if(__element_content[name])
       return callback("error", 0, 0, "Element type declared more than once.",
		       info, @extra);       
     if(contents == "EMPTY")
       __element_content[name] = ({accept_terminate});
     else if(contents == "ANY")
       __element_content[name] = ({accept_any});
     else if(contents[0] == "#PCDATA") {
       if(sizeof(Array.uniq(contents)) != sizeof(contents))
	 return callback("error", 0, 0, "The same name must not appear more "
			 "than once in a mixed-content declaration.",
			 info, @extra);
       __element_content[name] =
	 compile_language(({"*",({"|",""})+contents[1..]}),
			  ({accept_terminate}));
     } else
       __element_content[name] =
	 compile_language(contents, ({accept_terminate}));
     break;
   case "<!NOTATION":
     if(__notation_sysid[name] || __notation_pubid[name])
       return callback("error", 0, 0,
		       "More than one notation declaration for name.",
		       info, @extra);
     if(attributes->SYSTEM)
       __notation_sysid[name] = attributes->SYSTEM;
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
	 __entity_sysid[name] = attributes->SYSTEM;
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
	     return callback("error", 0, 0,
			     "Element has more than one ID attribute.",
			     info, @extra);
	   if(attributes[attr][1][0] != "#IMPLIED" &&
	      attributes[attr][1][0] != "#REQUIRED")
	     return callback("error", 0, 0,
			     "ID attribute must be #IMPLIED or #REQUIRED.",
			     info, @extra);
	 }
	 if(sizeof(attributes[attr][1]) == 2 &&
	    !islegalattribute(attributes[attr][1][1], attributes[attr]))
	   return callback("error", 0, 0,
			   "Illegal default attribute value.",
			   info, @extra);
	 if(attributes[attr][0][0] == "NOTATION")
	   __notations_used |= mkmultiset(attributes[attr][0][1..]);
	 __element_attrs[name][attr] = attributes[attr];
       }
     break;
   case "<":
   case "<>":
     if(!__element_content[name])
       return callback("error", 0, 0, "Element not declared.",
		       info, @extra);
     if(!sizeof(__element_stack))
       if(name != __root_element_name)
	 return callback("error", 0, 0, "Root element type mismatch.",
			 info, @extra);
       else;
     else
       if(!__element_stack[-1]->accept_element(name))
	 return callback("error", 0, 0, "Invalid content for element.",
			 info, @extra);
     Element e = Element(name);
     if(string aname = e->check_attributes(attributes))
       return callback("error", 0, 0, "Invalid attribute "+aname+
		       " for element.", info, @extra);
     if(kind == "<") {
       callback(kind, name, attributes, contents, info, @extra);
       __element_stack += ({ e });
       return 0;
     } else
       if(!e->accept_element(0))
	 return callback("error", 0, 0, "Invalid content for element.",
			 info, @extra);
     break;
   case ">":
     if(!sizeof(__element_stack) || __element_stack[-1]->name != name)
       return callback("error", 0, 0, "Unmatched end tag.",
		       info, @extra);
     if(!__element_stack[-1]->accept_element(0))
       return callback("error", 0, 0, "Invalid content for element.",
		       info, @extra);
     __element_stack = __element_stack[..sizeof(__element_stack)-2];
     break;
   case "":
   case "<![CDATA[":
     if(!sizeof(__element_stack))
       return callback("error", 0, 0, "All data must be inside tags",
		       info, @extra);
     if(!__element_stack[-1]->accept_element(""))
       if(kind == "" && contents-" "-"\t"-"\r"-"\n" == "")
	 return 0;
       else
	 return callback("error", 0, 0, "Invalid content for element.",
			 info, @extra);
     break;
   case "%":
     name = "%"+name;
   case "&":
     if(!__entity_sysid[name])
       return 0;
     if(__entity_ndata[name])
       return callback("error", 0, 0, "Reference to unparsed entity.",
		       info, @extra);
     if(attributes->in_attribute && kind!="%")
       return callback("error", 0, 0,
		       "Reference to External entity in attribute.",
		       info, @extra);       
     return get_external_entity(__entity_sysid[name], __entity_pubid[name],
				0, @extra) ||
       callback("error", 0, 0, "External entity not found.", info, @extra);
  }
  return callback(kind, name, attributes, contents, info, @extra);
}

static private mixed cleanup_parse(function(string,string,mapping
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
	    function(string,string,mapping,array|string,mapping(string:mixed),
		     mixed ...:mixed) callback, mixed ... extra)
{
  return ::parse(data, validate, callback, extra) +
    (({cleanup_parse(callback, extra)}) - ({0}));
}

//! @fixme
//!   Document this function
array parse_dtd(string data,
		function(string,string,mapping,array|string,
			 mapping(string:mixed),mixed ...:mixed) callback,
		mixed ... extra)
{
  return ::parse_dtd(data, validate, callback, extra);
}

/* define_entity? */

