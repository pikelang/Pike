#pike __REAL_VERSION__

/* PEM.pmod
 *
 * Support for parsing PEM-style messages.
 */

class encapsulated_message {

  import ._PEM;

  string boundary;
  string body;
  mapping(string:string) headers;

  object init(string eb, string contents)
  {
    boundary = eb;

    // werror(sprintf("init: contents = '%s\n'", contents));

    if (rfc822_start_re->match(contents))
      {
	array a = MIME.parse_headers(contents);
	headers = a[0];
	body = a[1];
      } else {
	headers = 0;
	body = contents;
      }
    return this_object();
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
    /* Replace singular LF with CRLF */
    array lines = body / "\n";

    /* Make all lines terminated with \r (but the last, which is
     * either empty or a "line" that was not terminated). */
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

class rfc934 {

  import ._PEM;

  string initial_text;
  string final_text;
  string final_boundary;
  array(object) encapsulated;
  
  object init(string data)
  {
    array parts = dash_split(data);
    
//    werror(sprintf("With newlines: %O", parts));
    
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
    
    /* Now each element if parts[i..] is a possible encapsulation
     * boundary, with the initial "-" removed. */

    for(; i < sizeof(parts); i++)
      {
#ifdef PEM_DEBUG
	werror(sprintf("parts[%d] = '%s'\n", i, parts[i]));
#endif
	if (sizeof(parts[i]) && (parts[i][0] == ' '))
	  {
	    /* Escape, just remove the "- " prefix */
	    current += parts[i][1..];
	    continue;
	  }

	/* Found an encapsulating boundary. First push the text
	 * preceding it. */
	if (!initial_text)
	  initial_text = current;
	else
	{
#ifdef PEM_DEBUG
	  werror(sprintf("boundary='%s'\ncurrent='%s'\n",
			 boundary, current));
#endif
	  encapsulated
	    += ({ encapsulated_message()->init(boundary, current) });
	}
	
	current = "";

	int end = search(parts[i], "\n");
	if (end >= 0)
	{
	  boundary = "-" + parts[i][..end-1];
	  current = parts[i][end..];
	} else {
	  /* This is a special case that happens if the input data had
	   * no terminating newline after the final boundary. */
#ifdef PEM_DEBUG
	  werror(sprintf("Final boundary, with no terminating newline.\n"
			 "  boundary='%s'\n", boundary));
#endif

	  boundary = "-" + parts[i];
	  break;
	}
      }
    final_text = current;
    final_boundary = boundary;
    
    return this_object();
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
	foreach(encapsulated, object m)
	  s += m->boundary + dash_stuff(m->to_string());
	s += final_boundary + dash_stuff(final_text);
      }
    return s;
  }
}

/* Disassembles PGP and PEM style messages with parts
 * separated by "-----BEGIN FOO-----" and "-----END FOO-----". */

class pem_msg
{
  import ._PEM;
  
  string initial_text;
  string final_text;
  mapping(string:object) parts;

  object init(string s)
    {
#ifdef PEM_DEBUG
      werror(sprintf("pem_msg->init: '%s'\n", s));
#endif
      object msg = rfc934()->init(s);
      parts = ([ ]);

      initial_text = msg->initial_text;

      for(int i = 0; i<sizeof(msg->encapsulated); i += 2 )
      {
	array(string)
	  res = begin_pem_re->split(msg->encapsulated[i]->get_boundary());
	if (!res)
	  /* Bad syntax. Return the parts decoded so far */
	  break;
    
#ifdef PEM_DEBUG
	werror(sprintf("Matched start of '%s'\n", res[0]));
#endif
	string name = res[0];

	/* Check end delimiter */
    
	if ( (i+1) < sizeof(msg->encapsulated))
	{
	  /* Next section is END * followed by daa that is ignored */
	  res = end_pem_re
	    ->split(msg->encapsulated[i+1]->get_boundary());
	}
	else
	{
	  /* This was the last section. Use the final_boundary. */
	  res = end_pem_re->split(msg->get_final_boundary());
	  final_text = msg->final_text;
	}

	if (!res || (res[0] != name))
	  /* Bad syntax. Return the parts decoded so far */
	  break;
	
	parts[name] = msg->encapsulated[i];
      }
      return this_object();
    }
}

/* Doesn't use general rfc934 headers and boundaries */
string simple_build_pem(string tag, string data)
{
  return sprintf("-----BEGIN %s-----\n"
		 "%s\n"
		 "-----END %s-----\n",
		 tag, MIME.encode_base64(data), tag);
}
