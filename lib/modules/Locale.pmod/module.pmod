// Copyright © 2000, Roxen IS.
// By Martin Nilsson

#pike __REAL_VERSION__

#define CLEAN_CYCLE 60*60
//#define LOCALE_DEBUG
//#define LOCALE_DEBUG_ALL

// project_name:project_path
private mapping(string:string) projects;
// language:(project_name:project)
private mapping(string:mapping(string:object)) locales;

void create() {
  projects=([]);
  locales=([]);
  call_out(clean_cache, CLEAN_CYCLE);
}

void register_project(string name, string path, void|string path_base)
  //! Make a connection between a project name and where its
  //! localization files can be found. The mapping from project
  //! name to locale file is stored in projects.
{
  if(path_base) {
    array tmp=path_base/"/";
    path_base=tmp[..sizeof(tmp)-2]*"/"+"/";
    path=combine_path(path_base, path);
  }
  if(projects[name] && projects[name]!=path) {
    // Same name, but new path. Remove all depreciated objects
    foreach(indices(locales), string lang)
      m_delete(locales[lang], name);
#ifdef LOCALE_DEBUG
    werror("\nChanging project %s from %s to %s\n",
	   name, projects[name], path);
  } else {
    werror("\nRegistering project %O (%s)\n",name,path);
#endif
  }
  projects[name]=path;
}

static class LanguageListObject {

  array(string) languages;  
  int timestamp;

  void create(array _languages) {
    languages = _languages;    
    timestamp = time(1);
  }

}

array(string) list_languages(string project) {
  if(!projects[project]) return ({});

  if(!locales[0])
    // language==0 not allowed, so this is good for internal data
    locales[0] = ([]);
  else if(locales[0][project] &&
	  // Only cache list for three minutes
	  ((3*60 + locales[0][project]->timestamp) > time(1) )) {
    return locales[0][project]->languages;
  }
  
  string pattern = replace(projects[project], "%%", "%");
  string dirbase = (pattern/"%L")[0];
  if(dirbase[-1]!='/') {
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
    if(!file_stat(replace(pattern, "%L", lang))) continue;
    list += ({ lang });
  }
  locales[0][project] = LanguageListObject( list );  

#ifdef LOCALE_DEBUG
  werror("Languages for project %O are%{ %O%}\n", project, list);
#endif
  return list;
}

class LocaleObject {

  // key:string
  private mapping(string|int:string) bindings;
  // key:function
  mapping(string:function) functions;
  int timestamp;

  void create(mapping(string|int:string) _bindings,
	      void|mapping(string:function) _functions) {
    bindings = _bindings;
    if(_functions)
      functions = _functions;
    else
      functions = ([]);
    timestamp = time(1);
  }

  string translate(string|int key) {
#ifdef LOCALE_DEBUG_ALL
    werror("L: %O -> %O\n",key,bindings[key]);
#endif    
    return bindings[key];
  }

  function is_function(string f) {
    return functionp(functions[f]) ? functions[f] : 0;
  }

  mixed `() (string f, mixed ... args) {
    if(functionp(functions[f]))
      return functions[f](@args);
    else
      return functions[f];
  }

}

object get_object(string project, string lang) {

  // Is there such a project?
  if(!projects[project])
    return 0;

  // Any language?
  if(!lang)
    return 0;

  // Is there already a locale object?
  LocaleObject locale_object;
  if(!locales[lang]) {
    locales[lang] = ([]);
  }
  else if(locale_object=locales[lang][project]) {
    locale_object->timestamp = time(1);
    return locale_object;
  }

  mapping(string|int:string) bindings = ([]);
  mapping(string:function) functions = ([]);
#ifdef LOCALE_DEBUG
  float sec = gauge{  
#endif
  string filename=replace(projects[project],
			  ({ "%L", "%%" }),
			  ({ lang, "%" }) );
  Stdio.File file=Stdio.FILE();
  if(!(file->open(filename, "r")))
    return 0;
  string line = file->gets();  // First line should be <?xml ?>
  string data = file->read();
  file->close();
  if(!line || !data)
    return 0;

  // Check encoding
  sscanf(line, "%*sencoding=\"%s\"", string encoding);
  if(encoding && encoding!="") {
    function(string:string) decode = 0;
    switch(lower_case(encoding)) 
      {
      case "iso-8859-1":
	// No decode needed
	break;

      case "utf-8": case "utf8":
	decode = 
	  lambda(string s) {
	    return utf8_to_string(s);
	  };
	break;
	
      case "utf-16": case "utf16":
      case "unicode":
	decode = 
	  lambda(string s) {
	    return unicode_to_string(s);
	  };
	break;
	
      default:
	object dec;
	// FIXME: Is this the best way of using Locale.Charset.decoder ?
	if(catch(dec = _Charset.decoder(encoding))) {
	  werror("\n* Warning: unknown encoding %O in %O\n",
		 encoding, filename);
	  break;
	}
	decode =
	  lambda(string s) {
	    return dec->clear()->feed(s)->drain();
	  };
      }
    if(decode && catch( data = decode(data) )) {
      werror("\n* Warning: unable to decode from %O in %O\n",
	     encoding, filename);
      return 0;
    }
  }
  else
    data = line+data;

  string|int id;
  function str_tag = lambda(string t, mapping m, string c) {
		       id = 0;
		       if(m->id && m->id!="" && c!="") {
			 if((int)m->id) m->id = (int)m->id;
			 id = m->id;
			 return c;
		       }
		       return 0;
		     };
  function t_tag = lambda(string t, mapping m, string c) {
		     if(!id) {
		       if(!m->id)
			 return 0;
		       else {
			 if((int)m->id) 
			   id = (int)m->id;
			 else
			   id = m->id;
		       }
		     }
		     if(String.trim_whites(c)=="")
		       return 0;
		     // Replace encoded entities
		     c = replace(c, ({"&lt;","&gt;","&amp;"}),
				 ({ "<",   ">",    "&"  }));
		     bindings[id]=c;
		     return 0;
		   };
  function pike_tag = lambda(string t, mapping m, string c) {
			// Replace encoded entities
			c = replace(c, ({"&lt;","&gt;","&amp;"}),
				       ({ "<",   ">",    "&"  }));
			object gazonk;
			if(catch( gazonk=compile_string("class gazonk {"+
							c+"}")->gazonk() )) {
			  werror("\n* Warning: could not compile code in "
				 "<pike> in %O\n", filename);
			  return 0;
			}
			foreach(indices(gazonk), string name)
			  functions[name]=gazonk[name];
			return 0;
		      };

  Parser.HTML xml_parser = Parser.HTML();
  xml_parser->case_insensitive_tag(1);
  xml_parser->
    add_containers( ([ "str"       : str_tag,
		       "t"         : t_tag,
		       "translate" : t_tag,
		       "pike"      : pike_tag, ]) );
  xml_parser->feed(data)->finish();

#ifdef LOCALE_DEBUG
  };   
  werror("\nLocale: Read %O in %O (bindings: %d, functions: %d) in %.3fs\n", 
	 project, lang, sizeof(bindings), sizeof(functions), sec);
#endif
  locale_object = LocaleObject(bindings, functions);
  locales[lang][project] = locale_object;
  return locale_object;
}

mapping(string:object) get_objects(string lang) {
  //! Reads in and returns a mapping with all the registred projects'  
  //! LocaleObjects in the language 'lang'
  if(!lang)
    return 0;
  foreach(indices(projects), string project)
    get_object(project, lang);
  return locales[lang];
}

string translate(string project, string lang, string|int id, string fallback)
  //! Returns a translation for the given id, or the fallback string
{
  LocaleObject locale_object = get_object(project, lang);
  if(locale_object) {
    string t_str = locale_object->translate(id);
#ifdef LOCALE_DEBUG
    if(t_str) t_str+="("+id+")";
#endif
    if(t_str) return t_str;
  }
#ifdef LOCALE_DEBUG
  else
    werror("\nLocale.translate(): no object for id %O in %s/%s",
	   id, project||"(no project)", lang||"(no language)");
  fallback += "("+id+")";
#endif
  return fallback;
}

function call(string project, string lang, string name, 
	   void|function|string fb)
  //! Returns a localized function
  //! If function not found, tries fallback function fb,
  //! or fallback language fb instead
{
  LocaleObject locale_object = get_object(project, lang);
  function f;
  if(!locale_object || !(f=locale_object->is_function(name))) 
    if(stringp(fb)) {
      locale_object = get_object(project, fb);
      if(!locale_object || !(f=locale_object->is_function(name)))
	return 0;
    }
  return f || [function]fb;
}

static void clean_cache() {
  remove_call_out(clean_cache);
  int t = time(1)-CLEAN_CYCLE;
  foreach(indices(locales), string lang) {
    foreach(indices(locales[lang]), string proj) {
      if(objectp(locales[lang][proj]) &&
	 locales[lang][proj]->timestamp < t) {
#ifdef LOCALE_DEBUG	
	werror("\nLocale.clean_cache: Removing project %O in %O\n",proj,lang);
#endif
	m_delete(locales[lang], proj);
      }
    }
  }
  call_out(clean_cache, CLEAN_CYCLE);
}

class DeferredLocale
{
  static string project;
  static string|int key;
  static string fallback;
  function(void:string) get_lang;
  void create(string project_, function(void:string) get_lang_,
	      string|int key_, string fallback_)
  {
    project = project_;
    get_lang = get_lang_;
    key = key_;
    fallback = fallback_;
  }
  static inline string lookup()
  {
    return translate(project, get_lang(), key, fallback);
  }
  string _sprintf(int c)
  {
    switch(c) {
    case 's':
      return lookup();
    case 'O':
      return
	sprintf("%O", lookup());
    default:
      error(sprintf("Illegal formatting char '%c'\n", c));
    }
  }
  string `+(mixed ... args)
  {
    return predef::`+(lookup(), @args);
  }
  string ``+(mixed ... args)
  {
    return predef::`+(@args, lookup());
  }
  int _sizeof()
  {
    return sizeof(lookup());
  }
  int|string `[](int a,int|void b)
  {
    if (query_num_arg() < 2) {
      return lookup()[a];
    }
    return lookup()[a..b];
  }
  array(string) `/(string s)
  {
    return lookup()/s;
  }
  array(int) _indices()
  {
    return indices(lookup());
  }
  array(int) _values()
  {
    return values(lookup());
  }
  mixed cast(string to)
  {
    if(to=="string") return lookup();
    throw( ({ "Cannot cast DeferredLocale to "+to+".\n", backtrace() }) );
  }
  int _is_type(string type) {
    return type=="string";
  }
};
