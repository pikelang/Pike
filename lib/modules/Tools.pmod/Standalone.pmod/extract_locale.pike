#!/usr/local/bin/pike
// By Martin Nilsson and Andreas Lange
//
// $Id: extract_locale.pike,v 1.1 2005/03/22 17:45:30 grubba Exp $
//


// The arguments given to the program
mapping args = ([]);
// All the files to gather strings from
array(string) files = ({});
// All ids used, id:mapping(info)
mapping ids = ([]);
// Reversed id mapping, text:id
mapping(string:string|int) r_ids = ([]);
// Order of the ids in the xml outdata file
array(string|int) id_xml_order = ({});
// Code to add to xml outfile, id:code
mapping(string:string) add = ([]);
// List of ids already in the xml outfile
multiset(string) added = (<>);
// The highest int with all lower ids set; see make_id()
int high_int_id = 0;


int make_id() {
  // Returns the next unused unique id
  //  while ( has_value(id_xml_order, ++high_int_id) );
  if(high_int_id)
    return ++high_int_id;
  high_int_id = max( @map(id_xml_order,
			  lambda(string|int in) {
			    return intp(in)?in:0;
			  }) ) + 1;
  return high_int_id;
}


string get_first_string(string in) {
  // Merges parts, compiles and returns the first string in a line from cpp
  // ie '"a\\n"  "b: " "%s!", string' --> "a\nb: %s!"
  string ret = "";
  int instr = 0;
  for(int i = 0; i<sizeof(in); i++) {
    if(in[i]=='\"')
      if(!(i>0 && in[i-1]=='\\')) {
	instr= instr? 0 : 1;
	if(instr) i++;
      }
    if(instr) ret+=in[i..i];
    else
      if(in[i]==',') break;
  }
  return compile_string("constant q=#\""+ret+"\";")->q;
}


string quotemeta(string in) {
  // Takes a string from cpp and quotes it so it will be
  // regexp-safe and match the string in the source-file
  string ret = "";
  int instr = 0;
  for(int i = 0; i<sizeof(in); i++) {
    switch (in[i])
      {
      case '\"':
	if(!(i>0 && in[i-1]=='\\')) {
	  instr = instr? 0 : 1;
	  if(instr && i>0)
	    ret += ".*";
	}
	ret += "\"";
	break;

      case '\\':
	if((i+1)<sizeof(in) && in[i+1]=='n') {
	  if(instr) {
	    ret += "[\n|\\\\]n*"; // Must handle both "\\n" and '\n'
	    i++;
	  }
	  break;
	}

      case '.': case '+': case '*':
      case '^': case '(': case ')':
      case '$': case '[': case ']':
      case '|':
	if(instr) ret += "\\";

      default:
	if(instr) ret += in[i..i];	
      }
  }
  return ret;
}


function get_encoder(string encoding) {
  // If needed, returns a function which encodes a string
  if(!encoding || encoding=="")
    return 0;
  switch( lower_case(encoding) )
    {
    case "utf-8": case "utf8":
      return lambda(string s) {
	       return string_to_utf8(s);
	     };

    case "utf-16": case "utf16":
    case "unicode":
      return lambda(string s) {
	       return string_to_unicode(s);
	     };

    default:
      object enc;
      if(catch( enc = Locale.Charset.encoder( encoding ) )) {
	werror("\n* Error: Unknown encoding %O!\n", encoding);
	exit(1);
      }
      return lambda(string s) {
	       return enc->clear()->feed(s)->drain();
	     };
    }
}


function get_decoder(string encoding) {
  // If needed, returns a function which decodes a string
  if(!encoding || encoding=="")
    return 0;
  switch( lower_case(encoding) )
    {
    case "iso-8859-1":
      // The normal, no decode needed
      return 0;

    case "utf-8": case "utf8":
      return lambda(string s) {
	       return utf8_to_string(s);
	     };

    case "utf-16": case "utf16":
    case "unicode":
      return lambda(string s) {
	       return unicode_to_string(s);
	     };

    default:
      object dec;
      if(catch( dec = Locale.Charset.decoder( encoding ) )) {
	werror("\n* Error: Unknown encoding %O!\n", encoding);
	exit(1);
      }
      return lambda(string s) {
	       return dec->clear()->feed(s)->drain();
	     };
    }
}


array(mapping) languagefiles(string searchpath, void|string skiplang) {
  // Based on the searchpath, returns list of files - skiplang-file
  string pattern = replace(searchpath, "%%", "%");
  string dirbase = (pattern/"%L")[0];
  if(dirbase=="") {
    dirbase="./";
    pattern = "./" + pattern;
  }
  else if(dirbase[-1]!='/') {
    array split = dirbase/"/";
    dirbase = split[..sizeof(split)-2]*"/"+"/";
  }

  string s_patt;
  if(search(pattern, "/", sizeof(dirbase))==-1)
    s_patt=pattern[sizeof(dirbase)..];
  else
    s_patt=pattern[sizeof(dirbase)..search(pattern, "/", sizeof(dirbase))-1];
  s_patt = replace(s_patt, "%L", "%3s");

  array dirlist = get_dir(dirbase);
  if(!dirlist)
    return ({});
  array list = ({});
  foreach(dirlist, string path) {
    string lang;
    if(!sscanf(path, s_patt, lang)) continue;
    if(lang==skiplang) continue;
    string file = replace(pattern, "%L", lang);
    if(!file_stat(file)) continue;
    list += ({ (["name":file, "lang":lang]) });
  }
  return list;
}


mapping parse_xml_file(string filename, string language) {
  // Reads a language-xml (like project_eng.xml)
  // Marks used ids in ids([]), also adds r_ids([text])
  // Returns mapping,
  //   'encoding' = file encoding,
  //   'data'= file with markers instead of <str>-blocks
  // write_xml_file uses the returned data+id_xml_order to build a new one
  added = (<>);
  id_xml_order = ({});

  if(!filename || filename=="")
    return ([]);
  Stdio.File in=Stdio.FILE();
  if(!in->open(filename, "r"))
    return ([]);
  write("Reading %s%s", 	
	language ? "["+language+"] " : "",
	(filename/"/")[-1]);
  string line = in->gets();
  string indata = in->read();
  in->close();
  if(!indata) {
    write("\n");
    return ([]);
  }

  // Check encoding
  string encoding;
  if(!line)
    line = indata;
  sscanf(line, "%*sencoding=\"%s\"", encoding);
  if(encoding && encoding!="") {
    function decode = get_decoder(encoding);
    if(decode && catch( indata = decode(indata) )) {
      werror("\n* Error: unable to decode from %O in %O\n",
	     encoding, filename);
      exit(1);
    }
  }
  else if(line!=indata)
    indata += line+"\n"+indata;

  write(" - parsing xml...");

  // Parse... First the <str>-parser
  mapping current = ([]);
  Parser.HTML str_parser = Parser.HTML();
  str_parser->case_insensitive_tag(1);

  str_parser->
    add_tag("changed",
	    lambda(object foo, mapping m) {
	      current->changetag = str_parser->current()+"\n";
	      return 0;
	    });

  function t_container =
    lambda(object foo, mapping m, string c) {
      if((int)m->id) m->id = (int)m->id;
      if(!current->id) {
	if(!m->id || m->id=="") {
	  werror("\n* Warning: String %O has no id.", c||current->original);
	  return 0;
	}
	current->id = m->id;
      }
      if(m->id && (m->id != current->id)) {
	werror("\n* Warning: Ignoring string %O. "
	       "Contained in id %O but marked with id %O.",
	       c, current->id, m->id);
	return 0;
      }
      if(has_value(id_xml_order, current->id)) {
	werror("\n* Error: Id %O used more than once.\n", current->id);
	exit(1);
      }
      id_xml_order += ({ current->id });
      c = replace(c, ({"&lt;","&gt;","&amp;"}), ({"<",">","&"}));
      current->text = c;
      current->textargs = m-({"id"});
      return 0;
    };
  str_parser->add_containers( ([ "t"         : t_container,
				 "translate" : t_container ]) );

  function o_container =
    lambda(object foo, mapping m, string c) {
      if(String.trim_whites(c)!="") {
	// Replace encoded entities
	c = replace(c, ({"&lt;","&gt;","&amp;"}), ({"<",">","&"}));
	current->original = c;
	current->originalargs = m-({"id"});
      }
      return 0;
    };
  str_parser->add_containers( ([ "o"        : o_container,
				 "original" : o_container ]) );

  // Main xml file parser
  // "\b" is used as a marker for lines to remove from returned data
  Parser.HTML xml_parser = Parser.HTML();
  xml_parser->case_insensitive_tag(1);
  xml_parser->add_quote_tag("!--", lambda() {return 0;}, "--");
  xml_parser->
    add_container("str",
		  lambda(object foo, mapping m, string c) {
		    current = ([]);  // New <str>, clear slate
		    if(m->id && m->id!="") {
		      if((int)m->id) m->id = (int)m->id;
		      current->id = m->id;
		    }
		    str_parser->feed( c )->finish();
		    if(current->id) {
		      ids[current->id] = current;
		      if(!current->original) current->original = "";
		      if(String.trim_whites(current->original)!="")
			r_ids[current->original] = current->id;
		    }
		    if(has_value(id_xml_order, current->id))
		      // Return marker for write_xml_file()
		      // - where to re-insert <str> again.
		      // This is done to make sure the file
		      // really is updated.
		      return "\7\7\7\7";  // Should be unique enough
		    return "\b";
		  });
  xml_parser->
    add_tag("locale",
	    // Verify the <locale>-xml version
	    lambda(object foo, mapping m) {
	      array n = m->version/".";
	      if(n[0]!="1") {
		werror("\n* Unknown locale version %O!\n", m->version);
		exit(1);
	      }
	      return "\b";
	    });
  xml_parser->
    add_container("project",
		  // Verify that the file is for the this project
		  lambda(object foo, mapping m, string c) {
		    c = String.trim_whites(c);
		    if(args->project && args->project!=c) {
		      werror("\n* xml data is for project %O, not %O!\n",
			     c, args->project);
		      exit(1);
		    } else
		      args->project = c;
		    return "\b";
		  });
  xml_parser->add_tag("added",
		      // Make sure <add>-tags don't get added more than once
		      lambda(object foo, mapping m) {
			m_delete(add, m->id);
			added[m->id] = 1;
			return "\b";
		      });
  // These tags will always be rewritten anyway, so remove them.
  xml_parser->add_quote_tag("?xml", "\b", "?");
  xml_parser->add_containers( (["file"     : "\b",
				"dumped"   : "\b",
				"language" : "\b"]) );
  xml_parser->feed(indata)->finish();

  // Remove markers and lines from removed tags
  string ret = "";
  object RE = Regexp("^[\b \t\n]+$");
  foreach(xml_parser->read()/"\n", string line) {
    if(!RE->match(line))
      ret += line+"\n";
  }
  // Remove silly lines in end of data
  RE = Regexp("^(.*[^\n \t]\n)[ \n\t]*$");
  array hits = RE->split( ret );
  if(hits) ret = hits[0];
  ret = replace(ret, "\n\n\n\n", "\n\n");

  write("\n");
  return ([ "encoding":encoding, "data":ret ]);
}


void write_xml_file(string filename, string language, string encoding,
		    string outdata, void|mapping old_ids)
  // Updates/creates a language xml-file with id:text-info
  // Reuses a present structure if fead with it in outdata
  // Some headers are always rewritten.
  // The old_ids mapping is supplied when the file is updated in comparison
  // with a base xml file.
{
  if(!sizeof(id_xml_order))
    // No ids changed or read with parse_xml_file()
    return;
  Stdio.File out=Stdio.File();
  if(!out->open(filename, "cw")) {
    werror("* Error: Could not open %s for writing\n", filename);
    exit(1);
  }

  write("Writing %s%s... (%d ids) ",
	language ? "["+language+"] " : "",
	(filename/"/")[-1], sizeof(id_xml_order));

  // Dump some headers
  string newfile = "";
  newfile += "<locale version=\"1.0\"/>\n";
  newfile += "<project>"+args->project+"</project>\n";
  newfile += "<language>" +
#ifdef constant(Standards.ISO639_2)
    Standards.ISO639_2.get_language(language) ||
#endif
    language + "</language>\n";

  if(!args->notime)
    newfile += "<dumped>"+time()+"</dumped>\n";

  // List files included in the project
  foreach(sort(files), string inname)
    newfile += "<file>"+inname+"</file>\n";

  // List blocks added from the config
  foreach(sort(indices(added)+indices(add)), string blockname)
    newfile += "<added id=\""+blockname+"\"/>\n";

  string o_tag = "o";
  string t_tag = "t";
  if(args->verbose) {
    o_tag = "original";
    t_tag = "translate";
  }

  mapping stats = ([]);
  function gen_tag =
    lambda(mixed id) {
      stats->written++;
      string diff = ((old_ids && old_ids[id] && old_ids[id]->changetag) ?
		     old_ids[id]->changetag : "");
      if(old_ids) {
	if(diff!="")
	  stats->changed++;	
	else if(!old_ids[id] || !old_ids[id]->text ||
	   String.trim_whites(old_ids[id]->text)=="" ) {
	  diff = "<new/>\n";
	  stats->new++;
	}
	else if(old_ids[id] && old_ids[id]->original != ids[id]->original) {
	  diff = replace(old_ids[id]->original||"",
			 ({"<",">","&"}), ({"&lt;","&gt;","&amp;"}));
	  diff = "<changed from=\""+ diff +"\"/>\n";
	  stats->changed++;
	}
	else
	  stats->ok++;
      }
      // Make parser-safe
      string original =
	replace(ids[id]->original, ({"<",">","&"}), ({"&lt;","&gt;","&amp;"}));
      string text =
	replace( ( (old_ids && old_ids[id] && old_ids[id]->text) ?
		   old_ids[id]->text : ""),
		({"<",">","&"}),({"&lt;","&gt;","&amp;"}));
      return sprintf("<str id=\"%s\">\n"
		     "%s<%s>%s</%[2]s>\n"
		     "<%s>%s</%[4]s>\n"
		     "</str>",
		     (string)id, diff, o_tag, original, t_tag, text);
    };

  // Reuse structure of old xml
  int i = 0;
  if(outdata) {
    string marker = "\7\7\7\7";    // Marker from parse_xml_file()
    string newstr;
    while( int n=search(outdata, marker) ) {
      if(n<0) break;
      if(i==sizeof(id_xml_order)) {
	// Shrinking file?
	outdata = replace(outdata, marker, "");
	continue;
      }
      if(args->wipe && !ids[id_xml_order[i]]->origin)
	newstr = "";  // Wipe this old string
      else
	newstr = gen_tag(id_xml_order[i]);
      outdata = (outdata[0..n-1] + newstr +
		 outdata[n+sizeof(marker)..sizeof(outdata)-1]);
      i++;
    }
    newfile += outdata;
  }

  // Dump new strings
  for(; i<sizeof(id_xml_order); i++) {
    if(!(args->wipe && !ids[id_xml_order[i]]->origin))
      newfile += "\n" + gen_tag(id_xml_order[i]) + "\n";
  }

  // If any, add missing <add>-blocks from config
  foreach(indices(add), string blockname)
    newfile += "\n"+add[blockname];

  // Determine encoding
  if(!encoding || encoding=="") {
    int width = String.width( newfile );
    if(width==16)
      encoding = "utf-8";
    else if(width==32)
      encoding = "utf-16";
    else
      encoding = "iso-8859-1";
  }
  function encode = get_encoder( encoding );
  if(encode && catch( newfile = encode(newfile) )) {
    werror("\n* Error: unable to encode file %O in %O\n",
	   filename, args->encoding);
    exit(1);
  }
  newfile = "<?xml version=\"1.0\" encoding=\""+ encoding +"\"?>\n"+ newfile;

  out->write( newfile );
  out->truncate( out->tell() );
  out->close();

  // Dump some statistics
  if(args->wipe && stats->written!=sizeof(id_xml_order))
    write("(wiped to %d) ", stats->written);
  if(old_ids) {
    if(stats->written==stats->ok)
      write("all translated");
    else {
      array ret= ({});
      if(stats->ok) ret += ({ sprintf("%d translated", stats->ok) });
      if(stats->new) ret += ({ sprintf("%d new", stats->new) });
      if(stats->changed) ret += ({ sprintf("%d changed", stats->changed) });
      write(String.implode_nicely( ret ));
    }
  }
  write("\n");
}


array(string) get_tokens(string in, mapping args, string filename) {
  // Picks out tokens from <locale-token>-tag in pikesource
  // The order between // blocks and /* */ blocks is not important
  // for our purposes.
  string comments = "";
  foreach( (in/"//")[1..], string line) {
    sscanf(line, "%s\n", line);
    comments += line+"\n";
  }

  // This is code is flawed. Breaks in e.g. userfs.pike in Roxen.
  //  foreach(in/"/\052", string block) {
  //    string c = "";
  //    sscanf(block, "%s\052/", c);
  //    comments += c+"\n";
  //  }

  array(string) tokens = ({});
  Parser.HTML()->
    add_container("locale-token",
		  lambda(object foo, mapping m, string c) {
		    if(args->project && m->project!=args->project)
		      return 0;
		    c = String.trim_whites(c);
		    if(has_value(tokens, c)) {
		      werror("\n* Warning: Token %O already found.\n", c);
		    }
		    tokens += ({c});
		    if (m->project)
		      args->project = m->project;
		    else
		      args->project = "";
		    return 0;
		  })
    ->feed( comments )->finish();
  if(!sizeof(tokens)) {
    if(args->project)
      werror("\n* Warning: No token for project %O in %s\n",
	     args->project, filename);
    else
      werror("\n* Warning: No token found in file %s\n", filename);
  }
  return tokens;
}

void update_pike_sourcefiles(array filelist) {
  // Extracts strings from pike sourcefiles in filelist
  // Updates ids, r_ids and  id_xml_order with ids and strings
  // If new ids, updates the sourcefile or a copy
  foreach(filelist, string filename) {
    Stdio.File file = Stdio.File();
    if(!file->open(filename, "r")) {
      werror("* Error: Could not open sourcefile %s.\n", filename);
      exit(1);
    }
    write("Reading %s", filename);
    string indata = file->read();
    file->close();

    // Get locale tokens, tokenize pike file
    write(", parsing...");
    array tokens = get_tokens(indata, args, filename);
    if(!sizeof(tokens))
      continue;
    mixed pdata = Parser.Pike.split(indata);
    pdata = Parser.Pike.tokenize(pdata);
    pdata = Parser.Pike.hide_whitespaces(pdata);

    array id_pike_order = ({});
    int no_of_ids = 0;
    string|int id;
    string fstr, token;
    for(int i=0; i<sizeof(pdata); i++) {
      //// Search for tokens
      foreach(tokens, token)
	if(token==pdata[i]) break;  // Loop tokens
      if(token!=pdata[i]) continue; // Verify token
      if(pdata[++i]!="(") continue; // Verify "("

      //// Get id
      id = (string)pdata[++i];
      if(id=="\"\"")
	id = "";
      else if((int)id)
	id = (int)id;
      else
	id = get_first_string(id);

      //// Get string
      string instr = "";
      i++;      // Skip ","
      while( ++i<sizeof(pdata) && pdata[i]!=")" )
	instr += (string)pdata[i];
      if(instr=="\"\"")
	fstr = "";
      else
	fstr = get_first_string(instr);
      if(fstr=="" && id=="")
	continue;  // Neither string nor id, skip!

      //// Check and store id and string
      no_of_ids++;
      if(!id || id=="") {
	if (r_ids[fstr])
	  id = r_ids[fstr];   // Re-use old id with identical string
	else
	  id = make_id();     // New string --> Get new id
	// New id for string --> file needs update, save info.
	id_pike_order += ({ ({id, token, quotemeta(instr)}) });
      } else {
	// Verify old id
	if(!ids[id] || (ids[id] && !ids[id]->origin)) {
	  // Remove preread string in r_ids lookup, might be updated
	  m_delete(r_ids, ids[id]);
	} else {
	  if(ids[id] && ids[id]->original!=fstr) {
	    werror("\n* Error: inconsistant use of id.\n");
	    werror("    In file:%{ %s%}\n", ids[id]->origin);
	    werror("     id %O -> string %O\n", id, ids[id]->original);
	    werror("    In file: %s\n", filename);
	    werror("     id %O -> string %O\n", id, fstr);
	    exit(1);
	  }
	}
	if(r_ids[fstr] && r_ids[fstr]!=id && ids[r_ids[fstr]]->origin)
	  werror("\n* Warning: %O has id %O in%{ %s%}, id %O in %s",
		 fstr, r_ids[fstr], ids[r_ids[fstr]]->origin, id, filename);
      }
      if(!has_value(id_xml_order, id))
	// Id not in xml-structure, add to list
	id_xml_order += ({ id });
      if(!ids[id])
	ids[id] = ([]);
      ids[id]->original = fstr;         // Store id:text
      ids[id]->origin += ({filename});  // Add  origin
      if(String.trim_whites(fstr)!="")
	r_ids[fstr] = id;               // Store text:id
    }

    // Done parsing, rebuild sourcefile if needed
    write(" (%d localization%s)\n", no_of_ids, (no_of_ids==1?"":"s"));
    if(!sizeof(id_pike_order)) {
      continue;
    }
    if(!args->nocopy)
      filename += ".new"; // Create new file instead of overwriting
    write("-> Writing %s (%d new)", filename, sizeof(id_pike_order));
    if(!file->open(filename, "cw")) {
      werror("\n* Error: Could not open %s for writing\n", filename);
      exit(1);
    }

    foreach(id_pike_order, array id) {
      // Insert ids based on tokens and the now regexp-safe string
      object(Regexp) RE;
      // RE = ^(.*TOKEN\( ")(", string \).*)$
      RE = Regexp("^(.*" + id[1] + "\\([ \n\t]*)[\"0]*" +
		  "([ ,\n\t]*"+id[2]+"[ \t\n]*\\).*)$");
      array hits = RE->split(indata);
      if(hits)
	indata = hits[0] + (intp(id[0])?id[0]:"\""+id[0]+"\"") + hits[1];
      else {
	werror("\n* Warning: Failed to set id %O for string %O in %s",
	       id[0], ids[id[0]]->original, filename);
	if(sizeof(ids[id[0]]->origin)<2)
	  id_xml_order -= ({ id[0] });
      }
    }
    write("\n");

    file->write( indata );
    file->truncate( file->tell() );
    file->close();
  }
}


void update_xml_sourcefiles(array filelist) {
  // Extracts strings from html/xml files in filelist
  // Updates ids, r_ids, id_xml_order with ids and strings
  // If new ids, updates the sourcefile or a copy
  foreach(filelist, string filename) {
    Stdio.File file = Stdio.FILE();
    if(!file->open(filename, "r")) {
      werror("* Error: Could not open sourcefile %s.\n", filename);
      exit(1);
    }
    write("Reading %s", filename);
    string line = file->gets();
    string data = file->read();
    file->close();
    if(!data && !line)
      continue;

    // Check encoding
    if(!line)
      line = data;
    string encoding;
    sscanf(line, "%*sencoding=\"%s\"", encoding);
    if(encoding && encoding!="") {
      function decode = get_decoder(encoding);
      if(decode && catch( data = decode(data) )) {
	werror("\n* Error: unable to decode from %O in %O\n",
	       encoding, filename);
	exit(1);
      }
    }
    else if(line!=data)
      data = line+"\n"+data;

    write(", parsing...");
    int new = 0;
    int ignoretag = 0;
    int no_of_ids = 0;
    Parser.HTML xml_parser = Parser.HTML();
    xml_parser->case_insensitive_tag(1);
    xml_parser->add_quote_tag("!--", lambda() {return 0;}, "--");
    xml_parser->
      add_tag("trans-reg",
	      // Check the registertag for the right project
	      lambda(object foo, mapping m) {
		if(!m->project || m->project=="") {
		  werror("\n * Error: Missing project in %s\n",
			 m->project, filename);
		  exit(1);
		}
		if(args->project && m->project!=args->project)
		  ignoretag = 1; // Tags might be from another project
		else
		  ignoretag = 0;
		if(!args->project)
		  args->project = m->project;
		return 0;
	      });
    xml_parser->		
      add_container("translate",
		    // This is the string container
		    lambda(object foo, mapping m, string c) {
		      if(m->project && m->project!="") {
			if(m->project!=args->project)
			  return 0; // Tag belongs to another project
			// else correct project, proceed
		      } else // No proj specified
			if(ignoretag)
			  return 0; // Check if last proj was another
		      string|int id = m->id;
		      if((int)id) id = (int)id;
		      string fstr = c;
		      int updated = 0;
		      if (String.trim_whites(fstr)=="")
			return 0;         // No need to store empty strings
		      no_of_ids++;
		      if(!id || id=="") {
			if (r_ids[fstr])
			  id = r_ids[fstr];   // Re-use old id with same string
			else
			  id = make_id();     // New string --> Get new id
			// Mark that we have a new id here
			updated = ++new;
		      } else {
			// Verify old id
			if(!ids[id] || (ids[id] && !ids[id]->origin)) {
			  // Remove preread string in r_ids, might be updated
			  m_delete(r_ids, ids[id]);
			} else {
			  if(ids[id] && ids[id]->original!=fstr) {
			    werror("\n* Error: inconsistant use of id.\n");
			    werror("    In file:%{ %s%}\n", ids[id]->origin);
			    werror("     id %O -> string %O\n",
				   id, ids[id]->original);
			    werror("    In file: %s\n", filename);
			    werror("     id %O -> string %O\n", id, fstr);
			    exit(1);
			  }
			}
			if(r_ids[fstr] && r_ids[fstr]!=id &&
			   ids[r_ids[fstr]]->origin)
			  werror("\n* Warning: %O has id %O in%{ %s%}, "
				 "id %O in %s", fstr, r_ids[fstr],
				 ids[r_ids[fstr]]->origin, id, filename);
		      }
		      if(!has_value(id_xml_order, id))
			// Id not in xml-structure, add to list
			id_xml_order += ({ id });
		      if(!ids[id])
			ids[id] = ([]);
		      ids[id]->original = fstr;         // Store id:text
		      ids[id]->origin += ({filename});  // Add  origin
		      if(String.trim_whites(fstr)!="")
			r_ids[fstr] = id;               // Store text:id
		      if(updated) {
			string ret="<translate id=\""+id+"\"";
			foreach(indices(m)-({"id"}), string param)
			  ret+=" "+param+"=\""+m[param]+"\"";
		        return ({ ret+">"+c+"</translate>" });
		      }
		      // Not updated, do not change
		      return 0;
		    });
    xml_parser->feed(data)->finish();

    // Done parsing, rebuild sourcefile if needed
    write(" (%d localization%s)\n", no_of_ids, no_of_ids==1?"":"s");
    if(!new) {
      continue;
    }
    data = xml_parser->read();
    if(encoding && encoding!="") {
      function encode = get_encoder(encoding);
      if(encode && catch( data = encode(data) )) {
	werror("\n* Error: unable to encode data in %O\n", encoding);
	exit(1);
      }
    }

    if(!args->nocopy)
      filename += ".new"; // Create new file instead of overwriting
    write("-> Writing %s (%d new)", filename, new);
    if(!file->open(filename, "cw")) {
      werror("\n* Error: Could not open %s for writing\n", filename);
      exit(1);
    }

    file->write( data );
    file->truncate( file->tell() );
    file->close();
    write("\n");
  }
}


string parse_config(string filename) {
  // Read config in xml-format and update args([]) and files({})
  // Commandline arguments have precedence
  // Returns name of outfile (ie project_eng.xml)
  if(!filename || filename=="")
    return "";
  Stdio.File in=Stdio.FILE();
  if(!in->open(filename, "r"))
    return "";
  string line = in->gets();
  string indata = in->read();
  in->close();
  if(!indata)
    return "";

  // Check encoding
  if(!line)
    line = indata;
  sscanf(line, "%*sencoding=\"%s\"", string encoding);
  if(encoding && encoding!="") {
    function decode = get_decoder(encoding);
    if(decode && catch( indata = decode(indata) )) {
      werror("\n* Error: unable to decode from %O in %O\n",
	     encoding, filename);
      exit(1);
    }
  }
  else if(line!=indata)
    indata = line+"\n"+indata;

  string xml_name="";
  Parser.HTML xml_parser = Parser.HTML();
  xml_parser->case_insensitive_tag(1);
  xml_parser->add_quote_tag("!--", lambda() {return 0;}, "--");
  xml_parser->
    add_container("project",
		  // Only read config for the right project, or the
		  // first found if unspecified
		  lambda(object foo, mapping m, string c) {
		    if(!m->name || m->name=="") {
		      werror("\n* Projectname missing in %s!\n", filename);
		      exit(1);
		    }
		    if(args->project && args->project!="" &&
		       args->project!=m->name)
		      return "";  // Skip this project-tag
		    else
		      args->project = m->name;
		    write("Reading config for project %O in %s\n",
			  args->project, filename);
		    return c;
		  });
  xml_parser->
    add_container("out",
		  // Set outname (default: project_eng.xml)
		  lambda(object foo, mapping m, string c) {
		    c = String.trim_whites(c);
		    if(c && c!="")
		      xml_name = c;
		    return 0;
		  });
  xml_parser->
    add_container("file",
		  // Add a file to be parsed
		  lambda(object foo, mapping m, string c) {
		    c = String.trim_whites(c);
		    if(c && c!="")
		      files += ({ c });
		    return 0;
		  });
  xml_parser->
    add_container("encoding",
		  // Set default encoding
		  lambda(object foo, mapping m, string c) {
		    if(args->encoding=="")
		      args->encoding = 0;
		    c = String.trim_whites(c);
		    if(c && c!="" && !args->encoding) {
		      args->encoding = c;
		      get_encoder( c );   // Check if known
		    }
		    return 0;
		  });
  xml_parser->
    add_container("xmlpath",
		  // Project file path
		  lambda(object foo, mapping m, string c) {
		    if(!args->xmlpath) {
		      c = String.trim_whites(c);
		      args->xmlpath = c;
		    }
		    return 0;
		  });
  xml_parser->
    add_container("baselang",
		  // Project file path
		  lambda(object foo, mapping m, string c) {
		    if(!args->baselang) {
		      c = String.trim_whites(c);
		      args->baselang = c;
		    }
		    return 0;
		  });
  xml_parser->
    add_container("add",
		  // Block to add to project-xml-files
		  lambda(object foo, mapping m, string c) {
		    if(!m->id || m->id=="") {
		      werror("\n* Missing id in <add> in %s!\n", filename);
		      exit(1);
		    }
		    add[m->id] = c;
		    return 0;
		  });
  xml_parser->add_tag("nocopy",
		      // Update the infile instead of creating infile.new
		      lambda(object foo, mapping m) {
			args->nocopy = 1;
			return 0;
		      });
  xml_parser->add_tag("verbose",
		      // More informative text in xml
		      lambda(object foo, mapping m) {
			args->verbose = 1;
			return 0;
		      });
  xml_parser->add_tag("wipe",
		      // Remove all id:strings not used in xml anymore
		      lambda(object foo, mapping m) {
			args->wipe = 1;
			return 0;
		      });
  xml_parser->feed(indata)->finish();

  if(xml_name=="")
    // Try to crate name of outfile
    if(args->xmlpath && args->baselang)
      xml_name = replace(args->xmlpath, "%L", args->baselang);
    else if( args->project)
      xml_name = args->project+"_eng.xml";
  return xml_name;
}


// ------------------------ The main program --------------------------

int main(int argc, array(string) argv) {

  // Parse arguments
  argv=argv[1..sizeof(argv)-1];
  for(int i=0; i<sizeof(argv); i++) {
    if(argv[i][0]!='-') {
      files += ({argv[i]});
      continue;
    }
    string key, val = "";
    if(sscanf(argv[i], "--%s", key)) {
      sscanf(key, "%s=%s", key, val);
      args[key] = val;
      continue;
    }
    args[argv[i][1..]] = 1;
  }

  // Get name of outfile (something like project_eng.xml)
  string xml_name=args->out;

  // Read configfile
  string configname = args->config;
  if(!configname && args->project)
    configname = args->project+".xml";
  string filename = parse_config(configname);
  if(!xml_name || xml_name=="")
    if(filename!="")
      xml_name = filename;
    else if(args->xmlpath && args->baselang)
      xml_name = replace(args->xmlpath, "%L", args->baselang);

  if( (!(xml_name && args->sync && args->xmlpath && args->baselang)) &&
      (!sizeof(files) || args->help) ) {
    sscanf("$Revision: 1.1 $", "$"+"Revision: %s $", string v);
    werror("\n  Locale Extractor Utility "+v+"\n\n");
    werror("  Syntax: extract.pike [arguments] infile(s)\n\n");
    werror("  Arguments: --project=name  default: first found in infile\n");
    werror("             --config=file   default: [project].xml\n");
    werror("             --out=file      default: [project]_eng.xml\n");
    werror("             --nocopy        update infile instead of infile.new\n");
    werror("             --notime        don't include dump time in xml files\n");
    werror("             --wipe          remove unused ids from xml\n");
    werror("             --sync          synchronize all locale projects\n");
    werror("             --encoding=enc  default: ISO-8859-1\n");
    werror("             --verbose       more informative text in xml\n");
    werror("\n");
    return 1;
  }

  // Try to read and parse xml-file
  mapping xml_data;
  xml_data = parse_xml_file(xml_name, args->baselang);
  write("\n");

  // Read, parse and (if necessary) update the sourcefiles
  object R = Regexp("(\.pike|\.pmod)$");
  foreach(files, string filename)
    if(R->match(filename))
      update_pike_sourcefiles( ({ filename }) );
    else
      update_xml_sourcefiles( ({ filename }) );

  // Save all strings to outfile xml
  if(!xml_name)
    if(args->project && args->project!="")
      xml_name = args->project+"_eng.xml";
    else {
      xml_name = files[0];
      sscanf(xml_name, "%s.pike", xml_name);
      xml_name += "_eng.xml";
    }
  write("\n");
  write_xml_file( xml_name, args->baselang,
		  args->encoding || xml_data->encoding, xml_data->data);

  // Synchronize xmls in other languages
  if (args->sync) {
    write("\n");
    mapping base_ids = ids;
    array base_order = id_xml_order;
    foreach(languagefiles(args->xmlpath, args->baselang), mapping file) {
      ids = ([]);
      string enc = parse_xml_file(file->name, file->lang)->encoding;
      id_xml_order = base_order;
      mapping old_ids = ids;
      ids = base_ids;
      write_xml_file(file->name, file->lang,
		     args->encoding || enc, xml_data->data, old_ids);
    }
  }

  write("\n");
  return 0;
}
