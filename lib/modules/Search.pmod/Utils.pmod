// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Utils.pmod,v 1.12 2001/07/11 20:10:09 js Exp $

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
