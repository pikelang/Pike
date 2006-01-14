//!
//!  Retrieves information about a CGI request from the environment
//!  and creates an object with the request information easily
//!  formatted.
//! 

#pike __REAL_VERSION__

static constant http_decode_string = _Roxen.http_decode_string;

//!
mapping(string:array(string)) variables = ([ ]);

//!
mapping(string:int|string|array(string)) misc = ([ ]);

//!
string query, rest_query;

//! 
string data;

//! If used with Roxen or Caudium webservers, this field will be
//! populated with "prestate" information.
multiset(string) prestate;

//! If used with Roxen or Caudium webservers, this field will be
//! populated with "config" information.
multiset(string) config;

//! If used with Roxen or Caudium webservers, this field will be
//! populated with "supports" information.
multiset(string) supports;

//!
string remoteaddr, remotehost;

//!
array(string) client;

//!
array(string) referer;

//!
multiset(string) pragma;

//!
mapping(string:string) cookies;

//!
string prot;

//!
string method;

static void add_variable(string name, string value) {
  if(variables[name])
    variables[name] += ({ value });
  else
    variables[name] = ({ value });
}

static void decode_query() {
  if(!query) return;
  foreach(query / "&", string v) {
    string a, b;
    if(sscanf(v, "%s=%s", a, b) == 2)
    {
      a = http_decode_string(replace(a, "+", " "));
      b = http_decode_string(replace(b, "+", " "));

      add_variable(a, b);
    } else
      if( rest_query )
	rest_query += "&" + http_decode_string( v );
      else
	rest_query = http_decode_string( v );
  }
  rest_query=replace(rest_query, "+", "\0"); /* IDIOTIC STUPID STANDARD */
}

static void decode_cookies(string data)
{
  cookies = ([]);
  foreach(data/";", string c)
  {
    string name, value;
    sscanf(c, "%*[ ]%s", c);
    if(sscanf(c, "%s=%s", name, value) == 2)
    {
      value=http_decode_string(value);
      name=http_decode_string(name);
      cookies[ name ]=value;
      if(name == "RoxenConfig" && sizeof(value))
	config = (multiset)(value/",");
    }
  }
}


static void decode_post()
{
  string a, b;
  if(!data) data="";
  int wanted_data=misc->len;
  int have_data=sizeof(data);
  if(have_data < misc->len) // \r are included. 
  {
    werror("WWW.CGI parse: Short stdin read.\n");
    return;
  }
  data = data[..misc->len];
  switch(lower_case(((misc["content-type"]||"")/";")[0]-" "))
  {
   default: // Normal form data.
    if(misc->len < 200000)
    {
      foreach(replace(data-"\n", "+", " ")/"&", string v)
	if(sscanf(v, "%s=%s", a, b) == 2)
	{
	  a = http_decode_string( a );
	  b = http_decode_string( b );
	  
	  add_variable(a, b);
	}
    }
    break;

   case "multipart/form-data":
    MIME.Message messg = MIME.Message(data, misc);
    foreach(messg->body_parts, object part) {
      if(part->disp_params->filename) {
	variables[part->disp_params->name]=part->getdata();
	variables[part->disp_params->name+".filename"]=
	  part->disp_params->filename;
	if(!misc->files)
	  misc->files = ({ part->disp_params->name });
	else
	  misc->files += ({ part->disp_params->name });
      } else
	add_variable(part->disp_params->name, part->getdata());
    }
    break;
  }
}

//!
//! creates the request object. To use, create a Request object while
//! running in a CGI environment. Environment variables will be parsed
//! and inserted in the appropriate fields of the resulting object.
static void create()
{
  string contents;

  contents = getenv("CONTENT_LENGTH");
  if(contents && sizeof(contents))
    misc->len = (int)(contents - " ");

  contents = getenv("CONTENT_TYPE");
  if(contents && sizeof(contents))
    misc["content-type"] = contents;

  contents = getenv("HTTP_HOST");
  if(contents && sizeof(contents))
    misc->host = contents;

  contents = getenv("HTTP_CONNECTION");
  if(contents && sizeof(contents))
    misc->connection = contents;

  contents = getenv("REMOTE_HOST");
  if(contents && sizeof(contents))
    remotehost = contents;

  remoteaddr = getenv("REMOTE_ADDR") || "unknown";
  referer = (contents = getenv("HTTP_REFERER")) ? contents/" " : ({});
  
  contents = getenv("SUPPORTS");
  if(contents && sizeof(contents))
    supports = mkmultiset(contents / " ");

  contents = getenv("HTTP_USER_AGENT");
  if(contents)
    client = contents / " ";

  contents = getenv("HTTP_COOKIE"); 
  if(contents && sizeof(contents)) {
    misc->cookies = contents;
    decode_cookies(contents);
  }

  contents = getenv("PRESTATES");
  if(contents && sizeof(contents))
    prestate = mkmultiset(map(contents / " ",
			      lambda(string s) {
      return http_decode_string(replace(s, "+", " "));
    }));

  contents = getenv("HTTP_PRAGMA");
  if(contents && sizeof(contents))
    pragma = aggregate_multiset(@replace(contents, " ", "")/ ",");

  query = getenv("QUERY_STRING");
  method = getenv("REQUEST_METHOD") || "GET";
  prot = getenv("SERVER_PROTOCOL");

  foreach( ({"DOCUMENT_ROOT", "HOSTTYPE", "GATEWAY_INTERFACE", "PWD",
	       "SCRIPT_FILENAME", "SCRIPT_NAME", "REQUEST_URI",
	       "SERVER_SOFTWARE", "SERVER_PORT", "SERVER_NAME",
	       "SERVER_ADMIN", "REMOTE_PORT", "PATH_INFO",
	       "PATH_TRANSLATED" }), string s)
    if(contents = getenv(s)) {
      misc[lower_case(s)] = contents;
    }
  
  foreach(({ "HTTP_ACCEPT", "HTTP_ACCEPT_CHARSET", "HTTP_ACCEPT_LANGUAGE",
	       "HTTP_ACCEPT_ENCODING", }), string header)
    if(contents = getenv(header)) {
      header = replace(lower_case(header),
		       ({"http_", "_"}), ({"", "-"}));
      misc[header] = (contents-" ") / ",";
    }
  
  
  if(misc->len)
    data = Stdio.stdin->read(misc->len);
  
  decode_query(); // Decode the query string

  if(misc->len && method == "POST")
    decode_post(); // We have a post.. Decode it.
}
