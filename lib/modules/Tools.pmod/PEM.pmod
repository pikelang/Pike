#pike __REAL_VERSION__
#pragma strict_types

//! Support for parsing PEM-style messages.


// _PEM

// Regexp used to decide if an encapsulated message includes headers
// (conforming to rfc 934).
static Regexp.SimpleRegexp rfc822_start_re = Regexp(
  "^([-a-zA-Z][a-zA-Z0-9]*[ \t]*:|[ \t]*\n\n)");


// Regexp used to extract the interesting part of an encapsulation
// boundary. Also strips spaces, and requires that the string in the
// middle between ---foo  --- is at least two characters long. Also
// allow a trailing \r or other white space characters.

static Regexp.SimpleRegexp rfc934_eb_re = Regexp(
  "^-*[ \r\t]*([^- \r\t]"	// First non dash-or-space character
  ".*[^- \r\t])"		// Last non dash-or-space character
  "[ \r\t]*-*[ \r\t]*$");	// Trailing space, dashes and space


// Start and end markers for PEM

// A string of at least two charecters, possibly surrounded by whitespace
#define RE "[ \t]*([^ \t].*[^ \t])[ \t]*"

static Regexp.SimpleRegexp begin_pem_re = Regexp("^BEGIN" RE "$");
static Regexp.SimpleRegexp end_pem_re = Regexp("^END" RE "$");

// Strip dashes
static string extract_boundary(string s)
{
  array(string) a = rfc934_eb_re->split(s);
  return a && a[0];
}

// ------------


class EncapsulatedMsg {

  string boundary;
  string body;
  mapping(string:string) headers;

  void create(string eb, string contents)
  {
    boundary = eb;

    if (rfc822_start_re->match(contents))
      {
	array a = MIME.parse_headers(contents);
	headers = [mapping(string:string)]a[0];
	body = [string]a[1];
      } else {
	headers = 0;
	body = contents;
      }
  }
  
  string decoded_body()
  {
    return MIME.decode_base64(body);
  }

  string get_boundary()
  {
    return extract_boundary(boundary);
  }

  string canonical_body()
  {
    // Replace singular LF with CRLF
    array(string) lines = body / "\n";

    // Make all lines terminated with \r (but the last, which is
    // either empty or a "line" that was not terminated).
    for(int i=0; i < sizeof(lines)-1; i++)
      if (!sizeof(lines[i]) || (lines[i][-1] != '\r'))
	lines[i] += "\r";
    return lines * "\n";
  }

  string to_string()
  {
    string s = (headers
		? Array.map(indices(headers),
			    lambda(string hname, mapping(string:string) h)
			    {
			      return hname+": "+h[hname];
			    }, headers) * "\n"
			    : "");
    return s + "\n\n" + body;
  }
}

class RFC934 {

  string initial_text;
  string final_text;
  string final_boundary;
  array(EncapsulatedMsg) encapsulated;
  
  static array(string) dash_split(string data)
  {
    // Find suspected encapsulation boundaries
    array(string) parts = data / "\n-";
    
    // Put the newlines back
    for (int i; i < sizeof(parts) - 1; i++)
      parts[i]+= "\n";
    return parts;
  }

  static string dash_stuff(string msg)
  {
    array(string) parts = dash_split(msg);
    
    if (sizeof(parts[0]) && (parts[0][0] == '-'))
      parts[0] = "- " + parts[0];
    return parts * "- -";
  }

  void create(string data)
  {
    array(string) parts = dash_split(data);

    int i = 0;
    string current = "";
    string boundary = 0;
    
    encapsulated = ({ });
    
    if (sizeof(parts[0]) && (parts[0][0] == '-'))
      parts[0] = parts[0][1..];
    else {
      current += parts[0];
      i++;
    }
    
    // Now each element if parts[i..] is a possible encapsulation
    // boundary, with the initial "-" removed.

    for(; i < sizeof(parts); i++)
      {
#ifdef PEM_DEBUG
	werror("parts[%d] = '%s'\n", i, parts[i]);
#endif
	if (sizeof(parts[i]) && (parts[i][0] == ' '))
	  {
	    /* Escape, just remove the "- " prefix */
	    current += parts[i][1..];
	    continue;
	  }

	// Found an encapsulating boundary. First push the text
	// preceding it.
	if (!initial_text)
	  initial_text = current;
	else
	{
#ifdef PEM_DEBUG
	  werror("boundary='%s'\ncurrent='%s'\n", boundary, current);
#endif
	  encapsulated
	    += ({ EncapsulatedMsg(boundary, current) });
	}
	
	current = "";

	int end = search(parts[i], "\n");
	if (end >= 0)
	{
	  boundary = "-" + parts[i][..end-1];
	  current = parts[i][end..];
	} else {
	  // This is a special case that happens if the input data had
	  // no terminating newline after the final boundary.
#ifdef PEM_DEBUG
	  werror("Final boundary, with no terminating newline.\n"
		 "  boundary='%s'\n", boundary);
#endif

	  boundary = "-" + parts[i];
	  break;
	}
      }
    final_text = current;
    final_boundary = boundary;
  }

  string get_final_boundary()
  {
    return extract_boundary(final_boundary);
  }

  string to_string()
  {
    string s = dash_stuff(initial_text);
    if (sizeof(encapsulated))
      {
	foreach(encapsulated, EncapsulatedMsg m)
	  s += m->boundary + dash_stuff(m->to_string());
	s += final_boundary + dash_stuff(final_text);
      }
    return s;
  }
}

// Disassembles PGP and PEM style messages with parts
// separated by "-----BEGIN FOO-----" and "-----END FOO-----".

class Msg
{
  string initial_text;
  string final_text;
  mapping(string:EncapsulatedMsg) parts;

  void create(string s)
   {
#ifdef PEM_DEBUG
      werror("Msg->create(%O)\n", s);
#endif
      RFC934 msg = RFC934(s);
      parts = ([ ]);

      initial_text = msg->initial_text;

      for(int i = 0; i<sizeof(msg->encapsulated); i += 2 )
      {
	array(string)
	  res = begin_pem_re->split(msg->encapsulated[i]->get_boundary());
	if (!res)
	  // Bad syntax. Return the parts decoded so far
	  break;
    
#ifdef PEM_DEBUG
	werror("Matched start of '%s'\n", res[0]);
#endif
	string name = res[0];

	// Check end delimiter
	if ( (i+1) < sizeof(msg->encapsulated))
	{
	  // Next section is END * followed by daa that is ignored
	  res = end_pem_re
	    ->split(msg->encapsulated[i+1]->get_boundary());
	}
	else
	{
	  // This was the last section. Use the final_boundary.
	  res = end_pem_re->split(msg->get_final_boundary());
	  final_text = msg->final_text;
	}

	if (!res || (res[0] != name))
	  // Bad syntax. Return the parts decoded so far
	  break;
	
	parts[name] = msg->encapsulated[i];
      }
   }
}

// Doesn't use general rfc934 headers and boundaries
string simple_build_pem(string tag, string data)
{
  return sprintf("-----BEGIN %s-----\n"
		 "%s\n"
		 "-----END %s-----\n",
		 tag, MIME.encode_base64(data), tag);
}


// Compat

class pem_msg {
  inherit Msg;
  void create() { }
  this_program init(string s) { ::create(s); return this; }
}

class rfc934 {
  inherit RFC934;
  void create() { }
  this_program init(string s) { ::create(s); return this; }
}

class encapsulated_message {
  inherit EncapsulatedMsg;
  void create() { }
  this_program init(string s, string t) { ::create(s,t); return this; }
}
