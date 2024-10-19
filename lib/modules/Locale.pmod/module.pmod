// By Martin Nilsson

#pike __REAL_VERSION__

//! The functions and classes in the top level of the Locale
//! module implements a dynamic localization system, suitable
//! for programs that need to change locale often. It is
//! even possible for different threads to use different locales
//! at the same time.
//!
//! The highest level of the locale system is called projects.
//! Usually one program consists of only one project, but for
//! bigger applications, or highly modular applications it is
//! possible for several projects to coexist.
//!
//! Every project in turn contains several unique tokens, each
//! one representing an entity that is different depending on
//! the currently selected locale.
//!
//! @example
//! @code
//!  // The following line tells the locale extractor what to
//!  // look for.
//!  // <locale-token project="my_project">LOCALE</locale-token>
//!
//!  // The localization macro.
//!  #define LOCALE(X,Y) Locale.translate("my_project", \
//!    get_lang(), X, Y)
//!
//!  string get_lang() { return random(2)?"eng":"swe"; }
//!
//!  int(0..0) main() {
//!    write(LOCALE(0, "This is a line.")+"\n");
//!    write(LOCALE(0, "This is another one.\n");
//!    return 0;
//!  }
//! @endcode
//!
//! @note
//!   In order to update your code to actually use the locale strings
//!   you need to run the locale extractor.
//!
//!   This is available as pike -x extract_locale
//!
//! @code
//!   Syntax: pike -x extract_locale [arguments] infile(s)
//!    Arguments: --project=name  default: first found in infile
//!               --config=file   default: [project].xml
//!               --out=file      default: [project]_eng.xml
//!               --nocopy        update infile instead of infile.new
//!               --notime        don't include dump time in xml files
//!               --wipe          remove unused ids from xml
//!               --sync          synchronize all locale projects
//!               --encoding=enc  default: ISO-8859-1
//!               --verbose       more informative text in xml
//! @endcode

#define CLEAN_CYCLE 60*60
//#define LOCALE_DEBUG
//#define LOCALE_DEBUG_ALL

// project_name:project_path
protected mapping(string:string) projects = ([]);
// language:(project_name:project)
protected mapping(string:mapping(string:object)) locales = ([]);
protected string default_project;

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
    foreach(locales;; mapping(string:object) lang)
      m_delete(lang, name);
#ifdef LOCALE_DEBUG
    werror("\nChanging project %s from %s to %s\n",
	   name, projects[name], path);
  } else {
    werror("\nRegistering project %O (%s)\n",name,path);
#endif
  }
  projects[name]=path;
}

void set_default_project_path(string path)
  //! In the event that a translation is requested in an
  //! unregistered project, this path will be used as the
  //! project path. %P will be replaced with the requested
  //! projects name.
{
  default_project = path;
}

protected class LanguageListObject( array(string) languages )
{
  int timestamp  = time(1);

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(timestamp: %d, %O)", this_program,
			     timestamp, languages);
  }
}

array(string) list_languages(string project)
//! Returns a list of all registered languages for a specific project.
{
  if(!projects[project])
    return ({});

  if(!locales[0]) {
    if(!sizeof(locales))
      call_out(clean_cache, CLEAN_CYCLE);
    // language==0 not allowed, so this is good for internal data
    locales[0] = ([]);
  }
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
  foreach(dirlist, string path)
  {
    string lang;
    if(!sscanf(path, s_patt, lang))
      continue;
    if(!file_stat(replace(pattern, "%L", lang)))
      continue;
    list += ({ lang });
  }
  locales[0][project] = LanguageListObject( list );

#ifdef LOCALE_DEBUG
  werror("Languages for project %O are%{ %O%}\n", project, list);
#endif
  return list;
}

class LocaleObject
{
  // key:string
  protected mapping(string|int:string) bindings;
  // key:function
  public mapping(string:function) functions;
  int timestamp = time(1);
  constant is_locale=1;

  protected void create(mapping(string|int:string) _bindings,
		     void|mapping(string:function) _functions)
  {
    bindings = _bindings;
    if(_functions)
      functions = _functions;
    else
      functions = ([]);
  }

  array(string|int) list_ids() {
    return indices(bindings);
  }

  string translate(string|int key)
  {
#ifdef LOCALE_DEBUG_ALL
    werror("L: %O -> %O\n",key,bindings[key]);
#endif
    return bindings[key];
  }

  function is_function(string f)
  {
    return functionp(functions[f]) ? functions[f] : 0;
  }

  protected mixed `() (string f, mixed ... args)
  {
    if(functionp(functions[f]))
      return functions[f](@args);
    else
      return functions[f];
  }

  int estimate_size()
  {
    int size=2*64+8; //Two mappings and a timestamp
    foreach(bindings; string|int id; string str) {
      size+=8;
      if(stringp(id)) size+=sizeof(id);
      else size+=4;
      size+=sizeof(str);
    }
    size+=32*sizeof(functions); // The actual functions are not included though...
    size+=sizeof( indices(functions)*"" );
    return size;
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(timestamp: %d, bindings: %d, functions: %d)",
			     this_program, timestamp, sizeof(bindings),
			     sizeof(functions) );
  }
}

object|zero get_object(string project, string lang) {

  // Is there such a project?
  int guess_project;
  if(!projects[project]) {
    if(default_project)
      projects[project] = replace(default_project, "%P", project);
    return 0;
  }

  // Any language?
  if(!lang)
    return 0;

  // Is there already a locale object?
  LocaleObject locale_object;
  if(!locales[lang]) {
    if(!sizeof(locales))
      call_out(clean_cache, CLEAN_CYCLE);
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
  if(!(file->open(filename, "r"))) {
    if(guess_project) m_delete(projects, project);
    return 0;
  }
  string line = file->gets();  // First line should be <?xml ?>
  string data = file->read();
  file->close();
  if(!line || !data)
    return 0;

  // Check encoding
  sscanf(line, "%*sencoding=\"%s\"", string encoding);
  if(encoding && encoding!="") {
    function(string:string)|zero decode = 0;
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
	if(catch(dec = global.Charset.decoder(encoding))) {
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
  string|zero str_tag(Parser.HTML parser, mapping m, string c) {
    id = 0;
    if(m->id && m->id!="" && c!="") {
      if((int)m->id) m->id = (int)m->id;
      id = m->id;
      return c;
    }
    return 0;
  };
  string|zero t_tag(Parser.HTML parser, mapping m, string c) {
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
  string|zero pike_tag(Parser.HTML parser, mapping m, string c) {
    // Replace encoded entities
    c = replace(c, ({"&lt;","&gt;","&amp;"}),
		({ "<",   ">",    "&"  }));
    object wrapper;
    if(catch( wrapper=compile_string("class gazonk {"+
                                     c+"}")->gazonk() )) {
      werror("\n* Warning: could not compile code in "
	     "<pike> in %O\n", filename);
      return 0;
    }
    foreach(indices(wrapper), string name) {
      mixed f = wrapper[name];
      if(functionp(f))
        functions[name]=[function]f;
    }
    return 0;
  };

  object(Parser.HTML)|void xml_parser = Parser.HTML();
  xml_parser->case_insensitive_tag(1);
  xml_parser->
    add_containers( ([ "str"       : str_tag,
		       "t"         : t_tag,
		       "translate" : t_tag,
		       "pike"      : pike_tag, ]) );
  xml_parser->feed(data)->finish();

  xml_parser = 0;		// To avoid trampoline garbage.

#ifdef LOCALE_DEBUG
  };
  werror("\nLocale: Read %O in %O (bindings: %d, functions: %d) in %.3fs\n",
	 project, lang, sizeof(bindings), sizeof(functions), sec);
#endif
  locale_object = LocaleObject(bindings, functions);
  locales[lang][project] = locale_object;
  return locale_object;
}

mapping(string:object)|zero get_objects(string lang)
  //! Reads in and returns a mapping with all the registred projects'
  //! LocaleObjects in the language 'lang'
{
  if(!lang)
    return 0;
  foreach(projects; string project;)
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

function|zero call(string project, string lang, string name,
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

void clean_cache() {
  remove_call_out(clean_cache);
  int t = time(1)-CLEAN_CYCLE;
  foreach(locales; string lang_str; mapping lang) {
    lang_str;	// Fix warning.
    foreach(lang; string proj_str; object proj) {
      proj_str;	// Fix warning.
      if(objectp(proj) && proj->timestamp < t) {
#ifdef LOCALE_DEBUG
	werror("\nLocale.clean_cache: Removing project %O in %O\n",
               proj_str, lang_str);
#endif
	m_delete(lang, proj);
      }
    }
  }
  call_out(clean_cache, CLEAN_CYCLE);
}

void flush_cache() {
#ifdef LOCALE_DEBUG
  werror("Locale.flush_cache()\n");
#endif
  locales=([]);
  // We could throw away the projects too,
  // but then things would probably stop working.
}

mapping(string:int) cache_status() {
  int size=0, lp=0;
  foreach(locales;; mapping l)
    foreach(l;; object o) {
      if(!o->is_locale) continue;
      lp++;
      size+=o->estimate_size();
    }

  return (["bytes":size,
	   "languages":sizeof(locales),
	   "reg_proj":sizeof(projects),
	   "load_proj":lp,
  ]);
}

//! This class simulates a multi-language "string".
//! The actual language to use is determined as late as possible.
class DeferredLocale( protected string project,
		      protected function(:string) get_lang,
		      protected string|int key,
		      protected string fallback )
{
  array get_identifier( )
  //! Return the data nessesary to recreate this "string".
  {
    return ({ project, get_lang, key, fallback });
  }

  protected int `<( mixed what )
  {
    return lookup() < what;
  }

  protected int `> ( mixed what )
  {
    return lookup() > what;
  }

  protected int `==( mixed what )
  {
    return lookup() == what;
  }

  protected inline string lookup()
  {
    return translate(project, get_lang(), key, fallback);
  }

  protected string _sprintf(int c)
  {
    switch(c)
    {
      case 's':
	return lookup();
      case 'O':
	return sprintf("%O(%O)", this_program, lookup());
    }
  }

  protected string `+(mixed arg)
  {
    return predef::`+(lookup(), arg);
  }

  protected string ``+(mixed arg)
  {
    return predef::`+(arg, lookup());
  }

  protected string `-(mixed arg)
  {
    return predef::`-(lookup(), arg);
  }

  protected string ``-(mixed arg)
  {
    return predef::`-(arg, lookup());
  }

  protected string `*(mixed arg)
  {
    return predef::`*(lookup(), arg);
  }

  protected string ``*(mixed arg)
  {
    return predef::`*(arg, lookup());
  }

  protected array(string) `/(mixed arg)
  {
    return predef::`/(lookup(), arg);
  }

  protected int _sizeof()
  {
    return sizeof(lookup());
  }

  protected int|string `[](int a,int|void b)
  {
    if (query_num_arg() < 2) {
      return lookup()[a];
    }
    return lookup()[a..b];
  }

  protected array(int) _indices()
  {
    return indices(lookup());
  }

  protected array(int) _values()
  {
    return values(lookup());
  }

  protected mixed cast(string to)
  {
    if(to=="string") return lookup();
    return UNDEFINED;
  }

  protected int _is_type(string type) {
    return type=="string";
  }
}

//! @module Charset
//!
//!   This is the old location for the @[predef::Charset] module.
//!
//! @deprecated predef::Charset

//! @decl inherit predef::Charset

#pragma no_deprecation_warnings
__deprecated__ object Charset = [__deprecated__(object)]global.Charset;
#pragma deprecation_warnings

//! @endmodule
