#!/usr/local/bin/pike
// Copyright © 2000, Roxen IS.
// By Martin Nilsson and Andreas Lange
//
// $Id: extract.pike,v 1.2 2000/07/10 17:47:08 lange Exp $
//


// The arguments given to the program
mapping args=([]);
// All the files to gather strings from
array(string) files=({});
// All ids used, id:text
mapping(string:string) ids=([]);
// Reversed id mapping, text:id
mapping(string:string) r_ids=([]);
// Keep track of every id's origin, id:array(filenames)
// (id_origin[id]==0 => from _eng.xml)
mapping(string:array) id_origin = ([]);
// Order of the ids in the _eng.xml file
array(string) id_xml_order=({});
// Code to add to _eng.xml, id:code  
mapping(string:string) add=([]);
// List of ids already in the _eng.xml
multiset(string) added=(<>);
// The highest int with all lower ids set; see make_id_string()
int high_int_id=0;


constant id_characters = "abcdefghijkmnopqrstuvwxyzABCDEFGHIJKLMNPQRSTUVWXYZ0123456789";
string make_id_string(int int_id) {
  // Make a string (as short as possible) based on id_characters and int_id
  string ret="";
  int rest = int_id - 1;
  int val;
  for(int pos=1+(int)floor(log((float)int_id)/log(1.0+sizeof(id_characters))); 
      pos; pos--) {
    if (pos < 2) 
      val = rest;
    else {
      int div = (int)pow(sizeof(id_characters)+1,(pos-1)) - 1;
      val = rest / div;
      rest -= val * div;
      val--;
    }
    val %= sizeof(id_characters);    
    ret += id_characters[val..val];
  }
  return ret;
}


string make_id() {
  // Returns the next unused unique id
  string ret;
  do {
    ret = make_id_string(++high_int_id);
  } while (has_value(id_xml_order,ret));
  return ret;
}


string get_first_string(string in) {
  // Merges parts, compiles and returns the first string in a line from cpp
  // ie '"a\\n"  "b: " "%s!", string' --> "a\nb: %s!"
  string ret="";
  int instr=0;
  for(int i=0; i<sizeof(in); i++) {
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
  string ret="";
  int instr=0;
  for(int i=0; i<sizeof(in); i++) {
    switch (in[i]) 
      {
      case '\"':
	if(!(i>0 && in[i-1]=='\\')) {
	  instr = instr? 0 : 1;
	  if(instr && i>0)
	    ret+=".*";
	}
	ret+="\"";
	break;

      case '\\':
	if((i+1)<sizeof(in) && in[i+1]=='n') {
	  if(instr) { 
	    ret+="[\n|\\\\]n*"; // Must handle both "\\n" and '\n'
	    i++;
	  }
	  break;
	}

      case '.':  case '+':  case '*':
      case '^':  case '(':  case ')':
      case '$':  case '[':  case ']':
      case '|':
	if(instr) ret+="\\";

      default:
	if(instr) ret+=in[i..i];	
      }
  }
  return ret;
}


function get_decoder(string encoding) {
  // If needed, returns a function which decodes a string
  if(!encoding || encoding=="")
    return 0;
  switch(lower_case(encoding)) 
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
      if(catch(dec = Locale.Charset.decoder( encoding ))) {
	werror("\n* Unknown encoding %O!\n", encoding);
	exit(1);
      }
      return lambda(string s) { 
	       return dec->clear()->feed(s)->drain();
	     };
    }
}


string parse_xml_file(string filename, void|mixed wipe_pass) {
  // Reads a language-xml (like project_eng.xml)
  // Marks used ids in ids([]), also adds r_ids([text]) from id-comment
  // Returns file, with markers instead of <--! [id] ""-->\n<t></t>
  // write_xml_file uses the returned data and id_xml_order to build a new one
  // Set parameter wipe_pass=1 to remove ids not in ids[] from file
  if(!filename || filename=="")
    return "";
  Stdio.File in=Stdio.File();
  if(!in->open(filename, "r"))
    return "";
  write("Opening %s", filename);
  string indata = in->read();
  in->close();
  if(!sizeof(indata)) {
    write("\n");
    return "";
  }
  if(wipe_pass)
    write(" - doing wipe pass...");
  else
    write(" - parsing xml...");

  // Comment id mapping - text from <!-- [id] "text" -->, id:text
  // text inserted into ids[id] in the t_tag function
  mapping c_ids=([]); 

  Parser.HTML xml_parser = Parser.HTML();
  function decode=0;
  function t_tag =  
    lambda(object foo, mapping m, string c) {
      if(!m->id||m->id=="") {
	werror("\n* Warning: String %O has no id.",c);
	return 0;
      }
      if(wipe_pass) {
	// This pass is done to remove id's not used anymore
	if(!ids[m->id]) {
	  id_xml_order -= ({ m->id });
	  return "\b";
	}
      } else {
	// Normal pass, update all structures
	if(has_value(id_xml_order, m->id)) {
	  werror("\n* Error: Id %O used more than once.\n",m->id);
	  exit(1);
	}
	id_xml_order += ({m->id});
	c = c_ids[m->id];
	if(!args->wipe)   // Check if there will be a wipe pass later
	  ids[m->id]=c;
	if(c != "")
	  r_ids[c] = m->id;
      }
      // Return marker for write_xml_file() - where to insert id-string again
      // This is done to make sure the file really is updated.
      return "\7\7\7\7";  // Marker unique enough?
    };  

  // "\b" is used as a marker for lines to remove from returned data
  xml_parser->case_insensitive_tag(1);  
  xml_parser->add_containers( ([ "t"         : t_tag,
				 "translate" : t_tag]) );
  xml_parser->
    add_container("locale", 
		    // Verify the <locale>-xml version
		  lambda(object foo, mapping m, string c) {
		    array n = m->version/".";
		    if(n[0]!="1") {
		      werror("\n* Unknown locale version %O!\n",m->version);
		      exit(1);
		    }
		    return "\b"+c;
		  });
  xml_parser->
    add_container("project", 
		  // Verify that the file is for the this project
		  lambda(object foo, mapping m, string c) {
		    c = String.trim_whites(c);
		    if(args->project && args->project!=c) {
		      werror("\n* xml data is for project %O, not %O!\n",
			     c,args->project);
		      exit(1);
		    } else
		      args->project=c;
		    return "\b";
		  });
  xml_parser->
    add_quote_tag("?xml",
		  // Check encoding
		  lambda(object foo, string c) {
		    sscanf(c,"%*sencoding=\"%s\"", string encoding);
		    if(encoding && encoding!="") {
		      if(!args->encoding)
			// Keep encoding if not overrideed
			args->encoding = encoding;
		      decode = get_decoder(encoding);
		    }
		    return "\b";
		  }, "?");
  xml_parser->add_tag("added",
		      // Make sure <add>-tags don't get added more than once
		      lambda(object foo, mapping m) {
			m_delete(add,m->id);
			added[m->id]=1;
			return "\b";
		      });
  xml_parser->
    add_quote_tag("!--",
		  // Might be a normal comment or a <!-- [id] "text" -->
		  lambda(object foo, string c) {
		    string id;
		    sscanf(c," [%s]%s",id,c);		    
		    if(id == 0) {
		      return 0;  // Normal comment tag
		    }
		    // Really make sure quotings are right
		    object RE = Regexp("^[^\"]*\"(.*)\"[^\"]*$");
		    array hits = RE->split(c);
		    if(hits)
		      c = get_first_string(sprintf("%O",hits[0]));
		    // Replace encoded entities
		    c = replace(c,({"&lt;","&gt;","&amp;"}),({"<",">","&"}));
		    if(decode)
		      if(catch( c = decode(c) )) {
			werror("\n* Warning: Decoding from %s failed for "+
			       "comment with id %s\n", args->encoding,id);
			return "\b";
		      }
		    if(id!="" && c!="")
		      // Save text for use in the t_tag function
		      c_ids[id]=c;
		    return "\b";
		  }, "--");
  // These tags will always be rewritten anyway, so remove them.
  xml_parser->add_containers( (["file"     : "\b",
				"dumped"   : "\b",
				"language" : "\b"]) );
  xml_parser->feed(indata)->finish();
  
  // Remove markers and lines from removed tags
  string ret="";
  object RE = Regexp("^[\b \t\n]+$");
  foreach(xml_parser->read()/"\n", string line) {
    if(!RE->match(line))
      ret += line+"\n";
  }
  // Remove silly lines in end of data
  RE = Regexp("^(.*[^\n \t]\n)[ \n\t]*$");
  array hits = RE->split(ret);
  if(hits) ret = hits[0]; 
  
  write("\n");
  return ret;
}


void write_xml_file(string out_name, string outdata) {
  // Updates/creates the project_eng.xml-file with id:text-info
  // Reuses a present structure if fead with it in outdata
  // Some headers is always rewritten.
  if(!sizeof(id_xml_order))
    // No ids changed or read with parse_xml_file()
    return;
  Stdio.File out=Stdio.File();
  if(!out->open(out_name, "cw")) {
    werror("* Error: Could not open %s for writing\n", out_name);
    exit(1);
  }

  write("Writing %s...",out_name);

  // Default nilencoding
  function encode=0;
  object _enc;
  if(args->encoding) {
    // Set encoder function if encoding known.
    switch(lower_case(args->encoding)) 
      {
      default:
	if(catch(_enc = Locale.Charset.encoder( args->encoding ))) {
	  werror("\n* Unknown encoding %O, using default", args->encoding);
	  args->encoding=0;
	  break;
	}
 	encode = lambda(string s) { 
		   return _enc->clear()->feed(s)->drain();
		 };
	break;

      case "utf-8": case "utf8":
	encode = lambda(string s) { 
		   return string_to_utf8(s);
		 };
	break;

      case "utf-16": case "utf16":
      case "unicode":
	encode = lambda(string s) { 
		   return string_to_unicode(s);
		 };
	break;
      
      case "":
	args->encoding = 0;

      case "iso-8859-1":
	// No encoding needed
      }
  }

  // Dump headers
  out->write("<?xml version=\"1.0\" encoding=\""+
	     (args->encoding||"ISO-8859-1")+"\"?>\n");
  out->write("<locale version=\"1.0\">\n");
  out->write("<project>"+args->project+"</project>\n");
  out->write("<language>English</language>\n");
  out->write("<dumped>"+time()+"</dumped>\n");

  // List files included in the project
  foreach(files, string inname)
    out->write("<file>"+inname+"</file>\n");

  // List blocks added from the config
  foreach(indices(added)+indices(add), string blockname)
    out->write("<added id=\""+blockname+"\"/>\n");

  string tag="t";
  string info="";
  if(args->verbose) {
    tag="translate";
    info="Original: ";
  }
    
  // Reuse structure of old xml
  int i=0;
  if(outdata) {
    string marker = "\7\7\7\7";    // Magic Marker from parse_xml_file()
    while(int n=search(outdata, marker)) {  
      if(n<0) break;
      if(i==sizeof(id_xml_order)) {
	// Shrinking file?
	outdata=replace(outdata,marker,"");
	continue;
      }
      string id=id_xml_order[i];
      string str=ids[id];
      if(encode) str=encode(str);    // Encode and make parser-safe
      str = replace(str, ({"<",">","&"}), ({"&lt;","&gt;","&amp;"}));
      outdata = (outdata[0..n-1] +
		 sprintf("<!-- [%s] %s\"%s\" -->\n<%s id=\"%s\"></%s>",
			 id, info, str, tag, id, tag) +
		 outdata[n+sizeof(marker)..sizeof(outdata)-1]);
      i++;
    }
    out->write(outdata);
  }

  // Dump new strings
  while(i<sizeof(id_xml_order)) {
    string id=id_xml_order[i];
    string str=ids[id];
    if(encode) str=encode(str);    // Encode and make parser-safe
    str = replace(str, ({"<",">","&"}), ({"&lt;","&gt;","&amp;"}));
    out->write("\n<!-- [%s] %s\"%s\" -->\n<%s id=\"%s\"></%s>\n",
	       id, info, str, tag, id, tag);
    i++;
  }
 
  // If any, add missing <add>-blocks from config
  foreach(indices(add), string blockname)
    out->write("\n"+add[blockname]);

  write("\n");
  out->write("\n</locale>\n");
  out->truncate( out->tell() );
  out->close();
}

array(string) get_tokens(string in, mapping args, string filename) {
  // Picks out tokens from <locale-token>-tag in pikesource
  // The order between // blocks and /* */ blocks is not important
  // for our purposes.
  string comments="";
  foreach(in/"//", string line) {
    sscanf(line, "%s\n", line);
    comments+=line+"\n";
  }
  foreach(in/"/\052", string block) {
    string c="";
    sscanf(block, "%s\052/", c);
    comments+=c+"\n";
  }

  array(string) tokens=({});
  Parser.HTML()->      
    add_container("locale-token",
		  lambda(object foo, mapping m, string c) {
		    if(args->project && m->project!=args->project) 
		      return 0;
		    if(has_value(tokens,c))
		      werror("\n* Warning: Token \"%s\" already found\n", c);
		    tokens+=({c});
		    if (m->project)
		      args->project=m->project;
		    else
		      args->project="";
		    return 0;
		  })
    ->feed(comments)->finish();
  if(!sizeof(tokens)) {
    if(args->project)
      werror("\n* Warning: No token for project %O in %s\n",args->project,filename);
    else
      werror("\n* Warning: No token found in file %s\n",filename);
    exit(1);
  }
  return tokens;
}

void update_pike_sourcefiles(array filelist) {
  // Extracts strings from pike sourcefiles in filelist
  // Updates ids, r_ids, id_xml_order with ids and strings
  // If new ids, updates the sourcefile or a copy
  foreach(filelist, string filename) {
    Stdio.File file=Stdio.File();
    if(!file->open(filename, "r")) {
      werror("* Error: Could not open sourcefile %s.\n", filename);
      exit(1);
    }
    write("Reading %s",filename);
    string indata=file->read();
    file->close();

    // Find the tokens
    write(", parsing...");
    array tokens=get_tokens(indata, args, filename);

    // Replace tokens defined in indata with a suitable (unique) pattern
    string presplit  = "\"\">>>";
    string midsplit  = "<\"\"-\"\">";
    string postsplit = "<<<\"\"";
    object(Regexp) RE;
    array hits;
    array id_pike_order=({});
    foreach(tokens, string token) {
      RE = Regexp("^#[ \t]*define[ \t\n]*"+token);
      string newdata = "";
      foreach(indata/"\n", string line) {
	if(RE->match(line))
	  newdata += ("#define " + token + "(X,Y...) "+
		      presplit + "X" + midsplit + "Y" + postsplit + "\n");
	else
	  newdata += line + "\n";
      }

      // Preparse data to mark the strings
      newdata = cpp(newdata);
      // Use this regexp to find the strings
      RE = Regexp(presplit+".*\"(.*)\".*"+midsplit+" (.*) "+postsplit);
      string id, fstr;
      foreach(newdata/"\n", string line) {
	hits = RE->split(line);
	if(hits && sizeof(hits)==2) {
	  // String found: get id and decode cpp'ed string
	  id = hits[0];
	  fstr = get_first_string(hits[1]);
	  if (fstr=="")
	    continue;         // No need to store empty strings
	  if(id == "") {
	    if (r_ids[fstr])
	      id = r_ids[fstr];   // Re-use old id with identical string
	    else
	      id = make_id();     // New string --> Get new id
	    // New id for string --> file needs update, save info.
	    id_pike_order += ({ ({id, token, quotemeta(hits[1])}) });
	  } else {
	    // Verify old id
	    if(!id_origin[id]) {
	      // Remove preread string in r_ids lookup, might be updated
	      m_delete(r_ids, ids[id]);  
	    } else {
	      if(ids[id] && ids[id] != fstr) {
		werror("\n* Error: inconsistant use of id.\n");
		werror("    In file:%{ %s%}\n",id_origin[id]);
		werror("     id %O -> string %O\n",id,ids[id]);
		werror("    In file: %s\n",filename);
		werror("     id %O -> string %O\n",id,fstr);
		exit(1);
	      }
	    }
	    if(r_ids[fstr] && r_ids[fstr]!=id && id_origin[r_ids[fstr]])
	      werror("\n* Warning: %O has id %O in%{ %s%}, id %O in %s",
		     fstr, r_ids[fstr], id_origin[r_ids[fstr]], id, filename);
	  }
	  if(!has_value(id_xml_order,id))
	    // Id not in xml-structure, add to list
	    id_xml_order += ({id});
	  id_origin[id] += ({filename}); // Remember origin
	  ids[id] = fstr;                // Store id:text
	  r_ids[fstr] = id;              // Store text:id
	}
      }
    }

    // Rebuild sourcefile if needed
    if(!sizeof(id_pike_order)) {
      write("\n");  
      continue;
    }
    if(!args->nocopy) 
      filename+=".new"; // Create new file instead of overwriting
    write("\n-> Writing %s with new ids: %d",filename,sizeof(id_pike_order));  
    if(!file->open(filename, "cw")) {
      werror("\n* Error: Could not open %s for writing\n", filename);
      exit(1);
    }

    foreach(id_pike_order, array id) {
      // Insert ids based on tokens and the now regexp-safe string
      // RE = ^(.*TOKEN\( ")(", string \).*)$
      RE = Regexp("^(.*" + id[1] + "\\([ \n\t]*\")" + 
		  "(\"[ ,\n\t]*"+id[2]+"[ \t\n]*\\).*)$");
      hits = RE->split(indata);
      if(hits)
	indata = hits[0] + id[0] + hits[1];
      else
	werror("\n* Failed to set id %O for string %O in %s",
	       id[0], ids[id[0]], filename);      
    }
    write("\n");

    file->write(indata);
    file->truncate( file->tell() );
    file->close();
  } 
}


void update_xml_sourcefiles(array filelist) {
  // Extracts strings from html/xml files in filelist
  // Updates ids, r_ids, id_xml_order with ids and strings
  // If new ids, updates the sourcefile or a copy
  foreach(filelist, string filename) {
    Stdio.File file=Stdio.File();
    if(!file->open(filename, "r")) {
      werror("* Error: Could not open sourcefile %s.\n", filename);
      exit(1);
    }
    write("Reading %s",filename);
    string indata=file->read();
    file->close();

    write(", parsing...");
    int new = 0;
    int ignoretag=0;
    function decode=0;
    Parser.HTML xml_parser = Parser.HTML();
    xml_parser->case_insensitive_tag(1);  
    xml_parser->
      add_quote_tag("?xml",
		    // Check for encoding in <?xml?>
		    lambda(object foo, string c) {
		      sscanf(c,"%*sencoding=\"%s\"",string encoding);
		      if(encoding && encoding!="") {
			decode = get_decoder(encoding);
		      }
		      return 0;
		    }, "?");
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
		  ignoretag=1; // Warning, tags might be from another project
		else
		  ignoretag=0;
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
                          // else Correct project, proceed
		      } else
			if(ignoretag) // No proj specified, check ignoretag
			  return 0;
		      string id = m->id||"";
		      string fstr = c;
		      if(decode)
			fstr=decode(fstr);
		      int updated = 0;
		      if (fstr=="")
			return 0;         // No need to store empty strings
		      if(id == "") {
			if (r_ids[fstr])
			  id = r_ids[fstr];   // Re-use old id with same string
			else
			  id = make_id();     // New string --> Get new id
			// Mark that we have a new id here
			updated = ++new;
		      } else {
			// Verify old id
			if(!id_origin[id]) {
			  // Remove preread string in r_ids, might be updated
			  m_delete(r_ids, ids[id]);
			} else {
			  if(ids[id] && ids[id] != fstr) {
			    werror("\n* Error: inconsistant use of id.\n");
			    werror("    In file:%{ %s%}\n",id_origin[id]);
			    werror("     id %O -> string %O\n",id,ids[id]);
			    werror("    In file: %s\n",filename);
			    werror("     id %O -> string %O\n",id,fstr);
			    exit(1);
			  }
			}
			if(r_ids[fstr] && r_ids[fstr]!=id && 
			   id_origin[r_ids[fstr]])
			  werror("\n* Warning: %O has id %O in%{ %s%}, "
				 "id %O in %s", fstr, r_ids[fstr],
				 id_origin[r_ids[fstr]], id, filename);
		      }
		      if(!has_value(id_xml_order,id))
			// Id not in xml-structure, add to list
			id_xml_order += ({id});
		      id_origin[id] += ({filename}); // Remember origin
		      ids[id] = fstr;                // Store id:text
		      r_ids[fstr] = id;              // Store text:id   
		      if(updated) {
			string ret="<translate id=\""+id+"\"";
			foreach(indices(m)-({"id"}), string param)
			  ret+=" "+param+"=\""+m[param]+"\"";
		        return ({ ret+">"+c+"</translate>" });
		      }
		      // Not updated, do not change
		      return 0;
		    });
    xml_parser->feed(indata)->finish();

    // Rebuild sourcefile if needed
    if(!new) {
      write("\n");  
      continue;
    }
    if(!args->nocopy) 
      filename+=".new"; // Create new file instead of overwriting
    write("\n-> Writing %s with new ids: %d", filename, new);  
    if(!file->open(filename, "cw")) {
      werror("\n* Error: Could not open %s for writing\n", filename);
      exit(1);
    }
    write("\n");

    file->write( xml_parser->read() );
    file->truncate( file->tell() );
    file->close();
  } 
}


string parse_config(string filename) {
  // Read config in xml-format and update args([]) and files({})
  // Commandline arguments have precedence
  // Returns name of outfile (ie project_eng.xml)
  if(!filename || filename=="")
    return "";
  Stdio.File in=Stdio.File();
  if(!in->open(filename, "r"))
    return "";
  string indata = in->read();
  in->close();
  if(!sizeof(indata))
    return "";

  string xml_name="";
  function decode=0;
  Parser.HTML xml_parser = Parser.HTML();
  xml_parser->case_insensitive_tag(1);
  xml_parser->
    add_quote_tag("?xml",
		  // Check for encoding in <?xml?>
		  lambda(object foo, string c) {
		    sscanf(c,"%*sencoding=\"%s\"",string encoding);
		    if(encoding && encoding!="") {
		      decode = get_decoder(encoding);
		    }
		    return 0;
		  }, "?");
  xml_parser->
    add_container("project", 
		  // Only read config for the right project, or the
		  // first found if unspecified
		  lambda(object foo, mapping m, string c) {
		    if(!m->name || m->name=="") {
		      werror("\n* Projectname missing in %s!\n", filename);
		      exit(1);
		    }
		    if( decode && catch(c = decode(c)) ) {
		      werror("\n* Failed to decode %O in config\n",m->name);
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
		    if( decode && catch(c = decode(c)) ) {
		      werror("\n* Failed to decode %O in config\n",m->name);
		      exit(1);
		    }
		    c = String.trim_whites(c);
		    if(c && c!="")
		      xml_name = c;
		    return 0;
		  });
  xml_parser->
    add_container("file", 
		  // Add a file to be parsed
		  lambda(object foo, mapping m, string c) {
		    if( decode && catch(c = decode(c)) ) {
		      werror("\n* Failed to decode %O in config\n",m->name);
		      exit(1);
		    }
		    c = String.trim_whites(c);
		    if(c && c!="")
		      files += ({ c });
		    return 0;
		  });
  xml_parser->
    add_container("encoding", 
		  // Set default encoding
		  lambda(object foo, mapping m, string c) {
		    if( decode && catch(c = decode(c)) ) {
		      werror("\n* Failed to decode %O in config\n",m->name);
		      exit(1);
		    }
		    c = String.trim_whites(c);
		    if(c && c!="" && !args->encoding)
		      args->encoding = c;
		    return 0;
		  });
  xml_parser->
    add_container("includepath", 
		  // Add includepath needed for pikefiles
		  lambda(object foo, mapping m, string c) {
		    if( decode && catch(c = decode(c)) ) {
		      werror("\n* Failed to decode %O in config\n",m->name);
		      exit(1);
		    }
		    c = String.trim_whites(c);
		    if(c && c!="")
		      add_include_path(c);
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
		    if( decode && catch(c = decode(c)) ) {
		      werror("\n* Failed to decode %O in config\n",m->name);
		      exit(1);
		    }
		    add[m->id]=c;
		    return 0;
		  });
  xml_parser->add_tag("nocopy", 
		      // Update the infile instead of creating infile.new
		      lambda(object foo, mapping m) {
			args->nocopy=1;
			return 0;
		      });
  xml_parser->add_tag("verbose", 
		      // More informative text in xml
		      lambda(object foo, mapping m) {
			args->verbose=1;
			return 0;
		      });
  xml_parser->add_tag("wipe", 
		      // Remove all id:strings not used in xml anymore
		      lambda(object foo, mapping m) {
			args->wipe=1;
			return 0;
		      });
  xml_parser->feed(indata)->finish();
  if(xml_name=="" && args->project)
    // Default name of outfile
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
    string key,val="";
    if(sscanf(argv[i], "--%s", key)) {
      sscanf(key, "%s=%s", key, val);
      args[key]=val;
      continue;
    }
    args[argv[i][1..]]=1;
  }

  // Get name of outfile (something like project_eng.xml)
  string xml_name=args->out;

  // Read configfile
  string configname = args->config;
  if(!configname && args->project)
    configname = args->project+".xml";
  string filename = parse_config(configname);
  if(filename!="" && (!xml_name || xml_name==""))
    xml_name = filename;

  if(!sizeof(files) || args->help) {
    sscanf("$Revision: 1.2 $", "$"+"Revision: %s $", string v);
    werror("\n  Locale Extractor Utility "+v+"\n\n");
    werror("  Syntax: extract.pike [arguments] infile(s)\n\n");
    werror("  Arguments: --project=name  default: first found in infile\n");
    werror("             --config=file   default: [project].xml\n");
    werror("             --out=file      default: [project]_eng.xml\n");
    werror("             --nocopy        update infile instead of infile.new\n");
    werror("             --wipe          remove unused ids from xml\n");
    werror("             --encoding=enc  default: ISO-8859-1\n");
    werror("             --verbose       more informative text in xml\n");
    werror("\n");
    return 1;
  }

  // Try to read and parse xml-file
  string xml_data="";
  xml_data = parse_xml_file(xml_name);

  // Read, parse and (if necessary) update the sourcefiles
  object R = Regexp("(\.xml|\.html)$");
  array xmlfiles = Array.filter(files, R->match);
  update_pike_sourcefiles(files-xmlfiles);
  update_xml_sourcefiles(xmlfiles);
  
  // If requested, remove ids not used anymore from the xml
  if(args->wipe)
    xml_data = parse_xml_file(xml_name, "Lets clean this mess up");

  // Save all strings to xml
  if(!xml_name)
    if(args->project && args->project!="")
      xml_name = args->project+"_eng.xml";
    else {
      xml_name = files[0];
      sscanf(xml_name, "%s.pike", xml_name);
      xml_name += "_eng.xml";
    }
  write_xml_file(xml_name, xml_data);

  return 0;
}
