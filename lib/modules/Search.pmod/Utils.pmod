// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Utils.pmod,v 1.43 2004/08/25 12:05:28 js Exp $

#if !constant(report_error)
#define report_error werror
#define report_debug werror
#define report_warning werror
#endif

#ifdef SEARCH_DEBUG
# define WERR(X) report_debug("search: "+(X)+"\n");
#else
# define WERR(X)
#endif

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

//! A result entry from the @[ProfileCache].
class ProfileEntry {

  private int last_stat;

  private int database_profile_id;
  private int query_profile_id;
  private ProfileCache my_cache;

  private mapping(string:mixed) database_values;
  private mapping(string:mixed) query_values;

  private Search.Database.MySQL db;
  private Search.RankingProfile ranking;
  private array(string) stop_words;

  //! @decl void create(int database_profile_id, int query_profile_id,@
  //!                   ProfileCache cache)
  //! @param cache
  //!  The parent cache object.
  void create(int _database_profile_id,
	      int _query_profile_id,
	      ProfileCache _my_cache) {
    database_profile_id = _database_profile_id;
    query_profile_id = _query_profile_id;
    my_cache = _my_cache;
    int last_stat = time();

    // Prefetch..
    get_ranking();
  }

  //! Checks if it is time to check if the profile values are
  //! to old.
  int(0..1) check_timeout() {
    if(time()-last_stat < 5*60) return 0;
    last_stat = time();
    return 1;
  }

  //! Returns the database profile value @[index].
  mixed get_database_value(string index) {
    if(!database_values)
      database_values = my_cache->get_value_mapping(database_profile_id);
    return database_values[index];
  }

  //! Returns the query profile value @[index].
  mixed get_query_value(string index) {
    if(!query_values)
      query_values = my_cache->get_value_mapping(query_profile_id);
    return query_values[index];
  }

  //! Returns a cached search database for the current database profile.
  Search.Database.MySQL get_database() {
    if(!db) {
#if constant(DBManager)
      db = Search.Database.MySQL( DBManager.db_url( get_database_value("db_name"), 1) );
#endif
      if(!db)
	THROW("Could not aquire the database URL to database " +
	      get_database_value("db_name") + ".\n");
    }
    return db;
  }

  //! Returns a cached ranking profile for the current database and
  //! query profile.
  Search.RankingProfile get_ranking() {
    if(!ranking)
      ranking = Search.RankingProfile(get_query_value("fi_cut"),
				      get_query_value("px_rank"),
				      get_database(),
				      get_query_value("fi_rank"));
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

  //! Returns a cached array of stop words for the current query profile.
  array(string) get_stop_words() {
    if(!stop_words) {
      ADTSet words = ADTSet();
      foreach(get_query_value("sw_lists"), string fn) {
	string file = Stdio.read_file(fn);
	if(!fn)
	  report_error("Could not load %O.\n", fn);
	else
	  words + (Array.flatten(map(file/"\n",
				     lambda(string in) {
				       return in/" ";
				     }))-({""}));
      }
      words + (Array.flatten(map(get_query_value("sw_words")/"\n",
				 lambda(string in) {
				   return in/" ";
				 }))-({""}));
      stop_words = (array)words;
    }
    return stop_words;
  }
}

//!
class ProfileCache (string db_name) {

  private mapping(string:ProfileEntry) entry_cache = ([]);
  private mapping(int:mapping(string:mixed)) value_cache = ([]);
  private mapping(string:int) db_profile_names = ([]);
  private mapping(string:int) query_profile_names = ([]);
  private mapping(int:int) profile_stat = ([]);

  private Sql.Sql get_db() {
    Sql.Sql db;
#if constant(DBManager)
    db = DBManager.cached_get(db_name);
#endif
    if(!db) THROW("Could not connect to database " + db_name + ".\n");
    return db;
  }

  //! Checks if the profile @[profile_id] has been changed, and clears
  //! related caches if so.
  //! @returns
  //!  @int
  //!   @value -1
  //!    The profile is deleted.
  //!   @value 0
  //!    The profile is not up to date.
  //!   @value 1
  //!    The profile is up to date.
  //!  @endint
  int(-1..1) up_to_datep(int profile_id) {
    array(mapping(string:string)) res;
    res = get_db()->query("SELECT altered,type FROM profile WHERE id=%d", profile_id);

    // The profile is deleted. In such a rare event we take the
    // trouble to review all our cached values.
    if(!sizeof(res)) {
      array(int) existing = (array(int))get_db()->query("SELECT id FROM profile")->id;

      foreach(indices(value_cache), int id)
	if(!has_value(existing, id))
	  m_delete(value_cache, id);

      foreach(indices(entry_cache), string id) {
	int dbp, qp;
	sscanf(id, "%d:%d", dbp, qp);
	if(!has_value(existing, dbp))
	  m_delete(entry_cache, id);
	if(!has_value(existing, qp))
	  m_delete(entry_cache, id);
      }

      foreach(indices(db_profile_names), string name)
	if(!has_value(existing, db_profile_names[name]))
	  m_delete(db_profile_names, name);

      foreach(indices(query_profile_names), string name)
	if(!has_value(existing, query_profile_names[name]))
	  m_delete(query_profile_names, name);

      return -1;
    }

    // Not altered
    if((int)res[0]->altered == profile_stat[profile_id]) return 1;
    profile_stat[profile_id] = (int)res[0]->altered;

    // Query profile
    if((int)res[0]->type == 2) 
    {
      m_delete(value_cache, profile_id);
      foreach(indices(entry_cache), string id)
	if(array_sscanf(id, "%d:%d")[1]==profile_id)
	  m_delete(entry_cache, id);
      return 0;
    }

    m_delete(value_cache, profile_id);
    foreach(indices(entry_cache), string id)
      if(array_sscanf(id, "%d:%d")[0]==profile_id)
	m_delete(entry_cache, id);
    return 0;
  }

  //! Returns the profile number for the given database profile.
  int get_db_profile_number(string name) {
    int db_profile;
    if(db_profile=db_profile_names[name])
      return db_profile;

    array res = get_db()->
      query("SELECT id FROM profile WHERE name=%s AND type=2", name);
    if(!sizeof(res))
      THROW("No database profile " + name + " found.\n");

    return db_profile_names[name] = (int)res[0]->id;
  }

  //! Returns the profile number for the given query profile.
  int get_query_profile_number(string name)
  {
    int query_profile;
    if( query_profile=query_profile_names[name] )
      return query_profile;

    array res = get_db()->
      query("SELECT id FROM profile WHERE name=%s AND type=1", name);
    if(!sizeof(res))
      THROW("No query profile " + name + " found.\n");

    return query_profile_names[name] = (int)res[0]->id;
  }

  private int last_db_prof_stat = 0;  // 1970

  //! Returns a list of available database profiles.
  array(string) list_db_profiles() {
    /*
      if (time() - last_db_prof_stat < 5*60)
      return indices(db_profile_names);*/
    array res = get_db()->query("SELECT name, id FROM profile WHERE type=2");
    db_profile_names = mkmapping(
      res->name,
      map(res->id, lambda(string s) { return (int) s; } ));
    if(sizeof(res))
      last_db_prof_stat = time();
    return res->name;
  }

  private int last_query_prof_stat = 0;  // 1970

  //! Returns a list of available query profiles.
  array(string) list_query_profiles()
  {
    /*
      if (time() - last_query_prof_stat < 5*60)
      return indices(query_profile_names);*/    
    array res = get_db()->query("SELECT name, id FROM profile WHERE type=1");
    query_profile_names = mkmapping( res->name, (array(int)) res->id );
    if(sizeof(query_profile_names))
      last_query_prof_stat = time();
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

  //! Returns the value mapping for the given profile.
  mapping get_value_mapping(int profile) {
    mapping val;
    if(val=copy_value(value_cache[profile]))
      return val;

    array res = get_db()->
      query("SELECT name,value FROM value WHERE pid=%d", profile);

    val = mkmapping( res->name, map(res->value, reacodec_decode) );
    value_cache[profile] = copy_value(val);
    return val;
  }

  //! Returns a @[ProfileEntry] object with the states needed for
  //! commiting searches in the database profile @[db_name] with
  //! the rules from query profile @[query_name].
  ProfileEntry get_profile_entry(string db_name, void|string query_name) {
    //    werror("Entry: %O\n", indices(entry_cache));
    //    werror("Value: %O\n", indices(value_cache));
    //    werror("Stat : %O\n", profile_stat);

    int db = get_db_profile_number(db_name);
    int query = get_query_profile_number(query_name);

    ProfileEntry entry;
    if(entry=entry_cache[query +":"+ db]) {
      if(!entry->check_timeout()) return entry;
      if(up_to_datep(db) &&
	 up_to_datep(query)) return entry;
    }

    entry = ProfileEntry( db, query, this_object() );
    return entry_cache[query +":"+ db] = entry;
  }

  //! Flushes profile entry @[p] from the profile cache.
  void flush_profile(int p) {
    m_delete(value_cache, p);
    foreach(indices(db_profile_names), string name)
      if(db_profile_names[name]==p)
	m_delete(db_profile_names, name);
    m_delete(query_profile_names, p);
    foreach(indices(entry_cache), string id) {
      array ids = array_sscanf(id, "%d:%d");
      if(ids[0]==p || ids[1]==p)
	m_delete(entry_cache, id);
    }
  }

  //! Empty the whole cache.
  void flush_cache() {
    value_cache = ([]);
    db_profile_names = ([]);
    query_profile_names = ([]);
    last_db_prof_stat = 0;
    last_query_prof_stat = 0;
  }
}

private mapping(string:ProfileCache) profile_cache_cache = ([]);

//! Returns a @[ProfileCache] for the profiles stored
//! in the database @[db_name].
ProfileCache get_profile_cache(string db_name) {
  if(profile_cache_cache[db_name])
    return profile_cache_cache[db_name];
  return profile_cache_cache[db_name] = ProfileCache(db_name);
}

//! Flushes the profile @[p] from all @[ProfileCache] objects
//! obtained with @[get_profile_cache].
void flush_profile(int p) {
  values(profile_cache_cache)->flush_profile(p);
}

private mapping(string:mapping) profile_storages = ([]);

//! Returns a profile storage mapping, which will be shared
//! between all callers with the same @[db_name] given.
mapping get_profile_storage(string db_name) {
  if(profile_storages[db_name])
    return profile_storages[db_name];
  return profile_storages[db_name] = ([]);
}

private mapping(string:Scheduler) scheduler_storage = ([]);

//! Returns a scheduler for the given profile database.
Scheduler get_scheduler(string db_name) {
  mapping dbp = get_profile_storage(db_name);
  if(scheduler_storage[db_name])
    return scheduler_storage[db_name];
  scheduler_storage[db_name] = Scheduler(dbp);
  return scheduler_storage[db_name] = Scheduler(dbp);
}

class Scheduler {

  private int next_run;
  private mapping(int:int) entry_queue = ([]);
  private mapping(int:int) crawl_queue = ([]);
  private mapping(int:int) compact_queue = ([]);
  private array(int) priority_queue = ({});
  private mapping db_profiles;
  private object schedule_process;

  void create(mapping _db_profiles) {
    db_profiles = _db_profiles;
    schedule();
  }

  void check_priority_queue(int profile)
  {
    if (!has_value(priority_queue, profile))
      priority_queue += ({ profile });
  }

  //! Call this method to indicate that a new entry has been added
  //! to the queue. The scheduler will delay indexing with at most
  //! @[latency] minutes.
  void new_entry(int latency, array(int) profiles) {
    int would_be_indexed = time() + latency*60;
    foreach(profiles, int profile)
    {
      entry_queue[profile] = 0;
      check_priority_queue(profile);
    }
    WERR("New entry.  time: "+(would_be_indexed-time())+" profiles: "+
	 (array(string))profiles*",");
    if(next_run && next_run<would_be_indexed && next_run>=time())
      return;
    next_run = would_be_indexed;
    reschedule();
  }

  void schedule(void|int quiet) {

    foreach(indices(db_profiles), int id) {
      object dbp = db_profiles[id];
      if(!dbp) {
	report_warning("Search database profile %d destructed.\n", id);
	m_delete(db_profiles, id);
	continue;
      }
      if(!quiet) WERR("Scheduling for database profile "+dbp->name);
      int next = dbp->next_recrawl();
      if(next != -1) {
	crawl_queue[dbp->id] = next;
	check_priority_queue(id);
	if(!quiet) WERR(" Crawl: "+(next-time()));
      }
      next = dbp->next_compact();
      if(next != -1) {
	compact_queue[dbp->id] = next;
	if(!quiet) WERR(" Compact: "+(next-time()));
      }
      if(!quiet) WERR("");
    }

    if(!sizeof(crawl_queue) && !sizeof(compact_queue) && !sizeof(entry_queue))
      return;
    next_run = max( min( @values(crawl_queue) + values(compact_queue) +
			 values(entry_queue) ),
		    time() + 10 );
    reschedule();
  }

#if constant (roxen)
  private void reschedule() {
    if( schedule_process )
      schedule_process->stop();
    WERR("Scheduler runs next event in "+(next_run-time())+" seconds.");
    // We use BackgroundProcess since there is no support for unscheduling
    // tasks created with background_run.
    schedule_process = 
      roxen.BackgroundProcess(next_run-time(), do_scheduled_stuff);
  }

  void unschedule() {
    if( schedule_process )
      schedule_process->stop();
  }


  private void do_scheduled_stuff() {
    if( schedule_process )
      schedule_process->stop();
    WERR("Running scheduler event.");

    foreach(indices(db_profiles), int id) {
      if (db_profiles[id]->is_running()) {
	WERR("Postponing crawl start, profile "+id+" still running.");
	schedule(1);
	return;
      }
    }

    int t = time();

    WERR(sizeof(crawl_queue)+" profiles in crawl queue.");
    foreach(priority_queue & indices(crawl_queue), int id) {
      if(crawl_queue[id]>t || !db_profiles[id]) continue;
      object dbp = db_profiles[id];
      if(dbp && dbp->ready_to_crawl()) {
	WERR("Scheduler starts crawling "+id);
	dbp->recrawl();
	m_delete(crawl_queue, id);
	m_delete(entry_queue, id);
	priority_queue -= ({ id });
      }
    }

    WERR(sizeof(entry_queue)+" profiles in entry queue.");
    foreach(priority_queue & indices(entry_queue), int id) {
      if(entry_queue[id]>t || !db_profiles[id]) continue;
      object dbp = db_profiles[id];
      if(dbp && dbp->ready_to_crawl()) {
	WERR("Scheduler starts crawling "+id);
	dbp->start_indexer();
	m_delete(entry_queue, id);
	priority_queue -= ({ id });
	break;
      }
    }

    WERR(sizeof(compact_queue)+" profiles in compact queue.");
    foreach(indices(compact_queue), int id) {
      if(compact_queue[id]>t || !db_profiles[id]) continue;
      db_profiles[id]->start_compact();
      m_delete(compact_queue, id);
    }

    schedule();
  }

#else
  private void reschedule() {
    remove_call_out(do_scheduled_stuff);
    WERR("Scheduler runs next event in "+(next_run-time())+" seconds.");
    call_out(do_scheduled_stuff, next_run-time());
  }

  void unschedule() {
    remove_call_out(do_scheduled_stuff);
  }

  private void do_scheduled_stuff() {
    remove_call_out(do_scheduled_stuff);
    WERR("Running scheduler event.");

    int t = time();

    WERR(sizeof(crawl_queue)+" profiles in crawl queue.");
    foreach(indices(crawl_queue), int id) {
      if(crawl_queue[id]>t || !db_profiles[id]) continue;
      object dbp = db_profiles[id];
      if(dbp && dbp->ready_to_crawl()) {
	WERR("Scheduler starts crawling "+id);
	dbp->recrawl();
	entry_queue = ([]);
      }
    }

    WERR(sizeof(crawl_queue)+" profiles in crawl queue.");
    foreach(indices(crawl_queue), int id) {
      if(crawl_queue[id]>t || !db_profiles[id]) continue;
      object dbp = db_profiles[id];
      if(dbp && dbp->ready_to_crawl()) {
	WERR("Scheduler starts crawling "+id);
	dbp->start_indexer();
      }
    }

    WERR(sizeof(compact_queue)+" profiles in compact queue.");
    foreach(indices(compact_queue), int id) {
      if(compact_queue[id]>t || !db_profiles[id]) continue;
      db_profiles[id]->start_compact();
    }

    schedule();
  }

#endif

  string info() {
    string res = "<table border='1' cellspacing='0' cellpadding='2'>"
      "<tr><th>Profile</th><th>Crawl</th>"
      "<th>Compact</th><th>Next</th></tr>";
    foreach(values(db_profiles), object dbp) {
      if(!dbp) continue;
      res += "</tr><td>" + dbp->name + "</td>";
      int next = dbp->next_crawl();
      if(next == -1)
	res += "<td>Never</td>";
      else
	res +="<td>"+ (next-time()) + "</td>";

      next = dbp->next_compact();
      if(next == -1)
	res += "<td>Never</td>";
      else
	res +="<td>"+ (next-time()) + "</td>";
      res += "</tr>";
    }
    res += "</table>";

    res += "<br />Next run: " + (next_run-time()) + "<br />";

    return res;
  }
}


//!
class Logger {

  private string|Sql.Sql logdb;
  private int profile;
  private int stderr_logging;

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

  //! @decl void create(Sql.Sql db_object, int profile, int stderr_logging)
  //! @decl void create(string db_url, int profile, int stderr_logging)
  void create(string|Sql.Sql _logdb, int _profile, int _stderr_logging) {
    logdb = _logdb;
    profile = _profile;
    stderr_logging = _stderr_logging;

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

  void werror_event( int code, string type, void|string extra, void|int log_profile )
  {
    mapping types = ([ "error" : "Error",
		       "warning" : "Warning",
		       "notice" :  "Notice", ]);

    werror(sprintf("%sSearch: %s: %s\n",
		   "          : ",
		   types[type],
		   extra?sprintf(codes[(int)code], @(extra/"\n")):codes[(int)code]));
  }

    //!
  void log_event( int code, string type, void|string extra, void|int log_profile ) {
    Sql.Sql db = get_db();
    if(!db) return;

    if(zero_type(log_profile))
      log_profile = profile;

    if(stderr_logging)
      werror_event(code, type, extra, log_profile);

    if(extra)
      db->query("INSERT INTO eventlog (profile,code,type,extra) VALUES (%d,%d,%s,%s)",
		log_profile, code, type, extra);
    else
      db->query("INSERT INTO eventlog (profile, code,type) VALUES (%d,%d,%s)",
		log_profile, code, type);
  }


  //!
  void log_error( int code, void|string extra, void|int log_profile ) {
    log_event( code, "error", extra, log_profile );
  }

  //!
  void log_warning( int code, void|string extra, void|int log_profile ) {
    log_event( code, "warning", extra, log_profile );
  }

  //!
  void log_notice( int code, void|string extra, void|int log_profile ) {
    log_event( code, "notice", extra, log_profile );
  }

  //!
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

    60 : "Starting database compactor with %s",
    61 : "Failed to find any data in the database.",
    62 : "Exiting compacter due to signal.",
    63 : "Done with database compacting and maintenance.",

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

    1000: "Disallowed by robots.txt. (%s)",
    1001: "Can't handle scheme. (%s)",
    1002: "No matching filter. (%s)",
    1003: "Too large content file. (%s)",
    1100: "Failed to connect to %s.",
  ]);
    

  //!
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
