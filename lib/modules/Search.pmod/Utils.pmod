// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Utils.pmod,v 1.15 2001/07/20 22:30:22 nilsson Exp $

public array(string) tokenize_and_normalize( string what )
//! This can be optimized quite significantly when compared to
//! tokenize( normalize( x ) ) in the future, currently it's not all
//! that much faster, but still faster.
{
  return Unicode.split_words_and_normalize( lower_case(what) );
}

public array(string) tokenize(string in)
//! Tokenize the input string (Note: You should first call normalize
//! on it)
{
  return Unicode.split_words( in );
}


public string normalize(string in)
//! Normalize the input string. Performs unicode NFKD normalization
//! and then lowercases the whole string
{
  return Unicode.normalize( lower_case(in), "KD" );
}


#define THROW(X) throw( ({ (X), backtrace() }) )

class ProfileEntry {

  private int last_stat;

  private int database_profile_id;
  private int search_profile_id;
  private ProfileCache my_cache;

  private mapping(string:mixed) database_values;
  private mapping(string:mixed) search_values;

  private Search.Database.MySQL db;
  private Search.RankingProfile ranking;
  private array(string) stop_words;

  void create(int _database_profile_id,
	      int _search_profile_id,
	      ProfileCache _my_cache) {
    database_profile_id = _database_profile_id;
    search_profile_id = _search_profile_id;
    my_cache = _my_cache;
    int last_stat = time(1);
  }

  void refresh() {
    if(time(1)-last_stat < 5*60) return;
    int check = my_cache->up_to_datep(database_profile_id);
    if(check == -1) destruct();
    if(!check) {
      database_values = 0;
      ranking = 0;
      db = 0;
    }
    check = my_cache->up_to_datep(search_profile_id);
    if(check == -1) destruct();
    if(!check) {
      search_values = 0;
      ranking = 0;
      stop_words = 0;
    }
    last_stat = time(1);
  }

  mixed get_database_value(string index) {
    if(!database_values)
      database_values = my_cache->get_value_mapping(database_profile_id);
    return database_values[index];
  }

  mixed get_search_value(string index) {
    if(!search_values)
      search_values = my_cache->get_value_mapping(search_profile_id);
    return search_values[index];
  }

  Search.Database.MySQL get_database() {
    if(!db) {
      db = Search.Database.MySQL( DBManager.db_url( get_database_value("db_name"), 1) );
      if(!db)
	THROW("Could not aquire the database URL to database " +
	      get_database_value("db_name") + ".\n");
    }
    return db;
  }

  Search.RankingProfile get_ranking() {
    if(!ranking)
      ranking = Search.RankingProfile(8,
				      get_search_value("px_rank"),
				      get_database(),
				      get_search_value("fi_rank"));
    return ranking;
  }

  class ADTSet {
    private mapping vals = ([]);

    ADTSet add (string|int|float in) {
      vals[in] = 1;
      return this_object();
    }

    ADTSet sub (string|int|float out) {
      m_delete(vals, out);
      return this_object();
    }

    ADTSet `+(mixed in) {
      if(stringp(in)||intp(in)||floatp(in))
	add(in);
      else
	map((array)in, add);
      return this_object();
    }

    ADTSet `-(mixed out) {
      if(stringp(out)||intp(out)||floatp(out))
	sub(out);
      else
	map((array)out, sub);
      return this_object();
    }

    mixed cast(string to) {
      switch(to) {
      case "object": return this_object();
      case "array": return indices(vals);
      case "multiset": return (multiset)indices(vals);
      default:
	THROW("Can not cast ADTSet to "+to+".\n");
      }
    }
  }

  array(string) get_stop_words() {
    if(!stop_words) {
      ADTSet words = ADTSet();
      foreach(get_search_value("sw_lists"), string fn) {
	string file = Stdio.read_file(fn);
	if(!fn)
	  report_error("Could not load %O.\n", fn);
	else
	  words + (Array.flatten(map(file/"\n",
				     lambda(string in) {
				       return in/" ";
				     }))-({""}));
      }
      words + (Array.flatten(map(get_search_value("sw_words")/"\n",
				 lambda(string in) {
				   return in/" ";
				 }))-({""}));
      stop_words = (array)words;
    }
    return stop_words;
  }
}

class ProfileCache (string db_name) {

  private mapping(int:ProfileEntry) entry_cache = ([]);
  private mapping(int:mapping(string:mixed)) value_cache = ([]);
  private mapping(string:int) db_profile_names = ([]);
  private mapping(string:int) srh_profile_names = ([]);
  private mapping(int:int) profile_stat = ([]);

  private Sql.Sql get_db() {
    Sql.Sql db = DBManager.cached_get(db_name);
    if(!db) THROW("Could not connect to database " + db_name + ".\n");
    return db;
  }

  int(-1..1) up_to_datep(int profile_id) {
    array(mapping(string:string)) res;
    res = get_db()->query("SELECT altered, parent FROM wf_profile WHERE id=%d", profile_id);

    // The profile is deleted. In such a rare event we take the
    // trouble to review all our cached values.
    if(!sizeof(res)) {
      array(int) existing = (array(int))get_db()->query("SELECT id FROM wf_profile")->id;

      foreach(indices(value_cache), int id)
	if(!has_value(existing, id))
	  m_delete(value_cache, id);

      foreach(indices(entry_cache), int id)
	if(!has_value(existing, id))
	  m_delete(entry_cache, id);

      foreach(indices(db_profile_names), string name)
	if(!has_value(existing, db_profile_names[name]))
	  m_delete(db_profile_names, name);

      foreach(indices(srh_profile_names), string name)
	if(!has_value(existing, srh_profile_names[name]))
	  m_delete(srh_profile_names, name);

      return -1;
    }

    // Not altered
    if((int)res[0]->altered == profile_stat[profile_id]) return 1;

    // Search profile
    if((int)res[0]->parent) {
      m_delete(value_cache, profile_id);
      m_delete(entry_cache, profile_id);
      return 0;
    }

    m_delete(value_cache, profile_id);
    foreach((array(int))get_db()->query("SELECT id FROM wf_profile WHERE parent=%d", profile_id)->id,
	    int id)
      m_delete(entry_cache, id);
    return 0;
  }

  //! Return the profile number for the given database profile.
  int get_db_profile_number(string name) {
    int db_profile;
    if(db_profile=db_profile_names[name])
      return db_profile;

    array res = get_db()->
      query("SELECT id FROM wf_profile WHERE name=%s AND parent=0",
	    name);
    if(!sizeof(res))
      THROW("No database profile " + name + " found.\n");

    return db_profile_names[name] = (int)res[0]->id;
  }

  //! Return the profile number for the given search profile.
  int get_srh_profile_number(string name, int db_profile) {
    int srh_profile;
    if(srh_profile=srh_profile_names[(string)db_profile+"\n"+name])
      return srh_profile;

    array res = get_db()->
      query("SELECT id FROM wf_profile WHERE name=%s AND parent=%d",
	    name, db_profile);
    if(!sizeof(res))
      THROW("No search profile " + name + " found.\n");

    return srh_profile_names[(string)db_profile+"\n"+name] = (int)res[0]->id;
  }

  // Used when decoding text encoded pike data types.
  private object compile_handler = class {
      mapping(string:mixed) get_default_module() {
	return ([ "aggregate_mapping":aggregate_mapping,
		  "aggregate_multiset":aggregate_multiset,
		  "aggregate":aggregate,
		  "allocate":allocate,
		  "this_program":0 ]);
      }

      mixed resolv(string id, void|string fn, void|string ch) {
	throw( ({ sprintf("Found symbol %O while trying to decode Roxen Search "
			  "settings. The database is corrupt or has been "
			  "tampered with.\n", id),
		  backtrace() }) );
      }
    }();

  // Decodes a "readable" pike data type to its actual pike value.
  private mixed reacodec_decode(string str) {
    return compile_string("mixed foo=" + str + ";", 0, compile_handler)()->foo;
  }

  //! Return the value mapping for the given profile.
  mapping get_value_mapping(int profile) {
    mapping val;
    if(val=copy_value(value_cache[profile]))
      return val;

    array res = get_db()->
      query("SELECT name,value FROM wf_value WHERE pid=%d", profile);

    val = mkmapping( res->name, map(res->value, reacodec_decode) );
    value_cache[profile] = copy_value(val);
    return val;
  }

  ProfileEntry get_profile_entry(string db_name, void|string srh_name) {
    int db = get_db_profile_number(db_name);
    int srh = get_srh_profile_number(srh_name||"Default", db);

    ProfileEntry entry;
    if(entry=entry_cache[srh])
      return entry;

    entry = ProfileEntry( db, srh, this_object() );
    return entry_cache[srh] = entry;
  }

  //! Flushes an entry from the profile cache.
  void flush_profile(int p) {
    m_delete(value_cache, p);
    foreach(indices(db_profile_names), string name)
      if(db_profile_names[name]==p)
	m_delete(db_profile_names, name);
    m_delete(srh_profile_names, p);
  }

  //! Empty the whole cache.
  void flush_cache() {
    value_cache = ([]);
    db_profile_names = ([]);
    srh_profile_names = ([]);
  }
}

class Logger {

  private string|Sql.Sql logdb;
  private int profile;

  private Sql.Sql get_db() {
    Sql.Sql db;
#if constant(DBManager)
    if(stringp(logdb))
      db = DBManager.get(logdb);
    else
#endif
      db = logdb;

    //    if(!logdb || !logdb->query)
    //      throw( ({ "Couldn't find any database object.\n", backtrace() }) );

    return db;
  }

  void create(string|Sql.Sql _logdb, int _profile) {
    logdb = _logdb;
    profile = _profile;

    // create table eventlog (event int unsigned auto_increment primary key,
    // at timestamp(14) not null, code int unsigned not null, extra varchar(255))

    Sql.Sql db = get_db();

    if(catch(db->query("SELECT code FROM eventlog WHERE event=0")))
      db->query("CREATE TABLE eventlog ("
		"event int unsigned auto_increment primary key,"
		"at timestamp(14) not null,"
		"profile int unsigned not null,"
		"code int unsigned not null,"
		"type enum('error','warning','notice') not null,"
		"extra varchar(255))");
  }

  void log_event( int code, string type, void|string extra, void|int log_profile ) {
    Sql.Sql db = get_db();
    if(!db) return;

    if(zero_type(log_profile))
      log_profile = profile;

    if(extra)
      db->query("INSERT INTO eventlog (profile,code,type,extra) VALUES (%d,%d,%s,%s)",
		log_profile, code, type, extra);
    else
      db->query("INSERT INTO eventlog (profile, code,type) VALUES (%d,%d,%s)",
		log_profile, code, type);
  }

  void log_error( int code, void|string extra, void|int log_profile ) {
    log_event( code, "error", extra );
  }

  void log_warning( int code, void|string extra, void|int log_profile ) {
    log_event( code, "warning", extra, log_profile );
  }

  void log_notice( int code, void|string extra, void|int log_profile ) {
    log_event( code, "notice", extra, log_profile );
  }

  int add_program_name(int code, string name) {
    int add = search( ({ "multiprocess_crawler", "buffer_c2f", "filter",
			 "buffer_f2i", "indexer" }), name );
    if(add==-1)
      throw( ({ "Unknown program name \""+name+"\".\n", backtrace() }) );

    return code + add;
  }

  private mapping codes = ([
    10 : "Started crawler with %s.",
    11 : "Started crawler-to-filter buffer with %s.",
    12 : "Started filter with %s.",
    13 : "Started filter-to-indexer buffer with %s.",
    14 : "Started indexer with %s.",

    20 : "Exiting crawler due to signal.",
    21 : "Exiting crawler-to-filter buffer due to signal.",
    22 : "Exiting filter due to signal.",
    23 : "Exiting filter-to-indexer buffer due to signal.",
    24 : "Exiting indexer due to signal.",

    30 : "Crawler failed to set up pipe.",
    31 : "Crawler-to-filter buffer failed to set up pipe.",
    32 : "Filter failed to set up pipe.",
    33 : "Filter-to-indexer buffer failed to set up pipe.",
    34 : "Indexer failed to set up pipe.",

    40 : "Fetched %s.",
    41 : "Unknown language code \"%s\".",
    42 : "Crawler exited normally.",
    43 : "Cleared search database.",
    44 : "Sitebuilder commit triggered indexing of %s.",

    50 : "Crawler did not get any connection from the process.",
    51 : "Crawler-to-filter bufferdid not get any connection from the process.",
    52 : "Filter did not get any connection from the process.",
    53 : "Filter-to-indexer buffer did not get any connection from the process.",
    54 : "Indexer did not get any connection from the process.",

    300: "300 Redirection: Multiple Choices (%s)",
    301: "301 Redirection: Moved Permanently (%s)",
    302: "302 Redirection: Found (%s)",
    303: "303 Redirection: See Other (%s)",
    304: "304 Redirection: Not Modified (%s)",
    305: "305 Redirection: Use Proxy (%s)",
    306: "306 Redirection: (Unused) (%s)",
    307: "307 Redirection: Temporary Redirect (%s)",
    400: "400 Client Error: Bad Request (%s)",
    401: "401 Client Error: Unauthorized (%s)",
    402: "402 Client Error: Payment Required (%s)",
    403: "403 Client Error: Forbidden (%s)",
    404: "404 Client Error: Not Found (%s)",
    405: "405 Client Error: Method Not Allowed (%s)",
    406: "406 Client Error: Not Acceptable (%s)",
    407: "407 Client Error: Proxy Authentication Required (%s)",
    408: "408 Client Error: Request Timeout (%s)",
    409: "409 Client Error: Conflict (%s)",
    410: "410 Client Error: Gone (%s)",
    411: "411 Client Error: Length Required (%s)",
    412: "412 Client Error: Precondition Failed (%s)",
    413: "413 Client Error: Request Entity Too Large (%s)",
    414: "414 Client Error: Request-URI Too Long (%s)",
    415: "415 Client Error: Unsupported Media Type (%s)",
    416: "416 Client Error: Requested Range Not Satisfiable (%s)",
    417: "417 Client Error: Expectation Failed (%s)",
    500: "500 Server Error: Internal Server Error (%s)",
    501: "501 Server Error: Not Implemented (%s)",
    502: "502 Server Error: Bad Gateway (%s)",
    503: "503 Server Error: Service Unavailable (%s)",
    504: "504 Server Error: Gateway Timeout (%s)",
    505: "505 Server Error: HTTP Version Not Supported (%s)",
  ]);
    

  array(array(string|int)) get_log( int profile, array(string) types,
				int from, int to ) {

    string sql = "";
#define SQLADD(X) do{sizeof(sql)?(sql+=" AND "+(X)):(sql=" WHERE "+(X));}while(0)
    if(profile)
      SQLADD("profile=" + profile);
    if(!sizeof(types))
      return ({});
    if(sizeof(types)!=3)
      SQLADD("(type='" + (types*"' OR type='") + "')");
    if(from)
      SQLADD("at>" + from);
    if(to)
      SQLADD("to<" + to);
#undef SQLADD

    Sql.Sql db = get_db();
    if(!db) return ({});

    return map(db->query("SELECT unix_timestamp(at) as at,profile,code,type,extra FROM eventlog" +
			 sql + " ORDER BY event DESC"),
	       lambda(mapping in) {
		 return ({ (int)in->at, (int)in->profile, in->type,
			   in->extra?sprintf(codes[(int)in->code], @(in->extra/"\n")):
			   codes[(int)in->code] });
	       } );
  }
}
