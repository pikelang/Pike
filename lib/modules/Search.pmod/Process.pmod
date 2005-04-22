
static string pike_binary = replace( getenv("PIKE"), "\\", "/");

#if constant(roxen)

class Indexer(Variable wa_var, void|function get_sb_workarea_view_url, Configuration conf)
{
  // Provided API
  public function done_cb;

  static object crawler_remote;

  public mapping params = ([]);
  public string db; // Not URLs, but names of databases in Roxen.
  public string log_db;

  static string userpass;
  
  void setup_conf()
  {
    string cf = (wa_var->query()/"/")[0];
    foreach(roxen->configurations, object _conf)
      if(_conf->name == cf)
	conf = _conf;
  }
  
  
  int ping()
  {
    catch { // throws if crawler_remote is 0 or not really running.
      return crawler_remote->ping();
    };
  }

  static string get_backdoor_userpass()
  {
    setup_conf();
    mapping acvar = roxen->query_var("AC");
    object acmodule = acvar->loaders[conf];
    return acmodule && acmodule->online_db->acdb->open_backdoor(0x7fffffff);
  }

  static void remove_backdoor_userpass(string userpass)
  {
    setup_conf();
    mapping acvar = roxen->query_var("AC");
    acvar->loaders[conf]->online_db->acdb->
      close_backdoor(userpass);
  }
  
  static array prepare_cb(string _url, mapping headers)
  {
    Standards.URI url = Standards.URI(_url);
    Standards.URI sb_url;
    
    int is_internal_url = 0;
    foreach(params->internal_urls, string internal_url)
      if(glob(internal_url, _url)) {
	is_internal_url = 1;
	break;
      }
    
    if(url->scheme=="sitebuilder" || url->scheme=="intrawise")
    {
      if(userpass && conf)
        headers["Authorization"]=sprintf("Basic %s", MIME.encode_base64(userpass));
      headers["Request-Metadata"]="";
      headers["Request-Invisible"]="";
      sb_url = get_sb_workarea_view_url(url);
      headers["host"]=sprintf("%s:%d", sb_url->host, sb_url->port);
      string ip;
      if(!sscanf(sb_url->fragment, "ip=%s", ip))
	error("Error: No ip number found in fragment of url %O\n.", sb_url);
      sb_url->host = ip;
      sb_url->fragment = 0;
      return ({ (string)sb_url, headers });
    }
    else {
      if(is_internal_url) {
	if(userpass && conf)
	  headers["Authorization"]=sprintf("Basic %s", MIME.encode_base64(userpass));
	headers["Request-Metadata"]="";
	headers["Request-Invisible"]="";
      }
      return ({ (string)url, headers });
    }
  }

  // Remote (from the crawler) functions
  class RemoteAPI
  {
    mapping get_params()          { return params; };
    function get_prepare_cb()     { return prepare_cb;  }
    string get_db_url()           { return db;}
    string get_logdb_url()        { return log_db;}
    string get_queue_table()      { return "queue"; }

    void set_crawler( object c )
    {
      crawler_remote = c;
      if( !c && done_cb )
      {
	done_cb();
	done_cb = 0;
      }
    }
  };

  static Remote.Server sv;
  static string remote_id = random(0x7fffffff)->digits( 22 );
  static Process.Process crawler;

  public int(0..1) start_indexer_process( )
  {
    if( crawler && !crawler->status() )
      return 0;

    crawler_remote = 0;

    setup_conf();
    if(!userpass && conf)
      userpass = get_backdoor_userpass();

    mapping env = getenv();
    if( !sv )
    {
      sv = Remote.Server( "127.0.0.1", 0, 3 );
      sv->provide( remote_id, RemoteAPI() );
    }

    env["REMOTE_HOST"] = "127.0.0.1";
    env["REMOTE_PORT"] = (sv->port->query_address()/" ")[-1];
    env["REMOTE_IDENTIFIER"] = remote_id;

    array debug_args = ({});

#ifdef SEARCH_CRAWLER_DEBUG
    debug_args += ({"-DSEARCH_DEBUG"});
#endif

    crawler=Process.create_process(
      ({
	pike_binary,
      }) + debug_args + ({
	"-M",
	"modules/search/pike-modules/",
	"modules/search/programs/multiprocess_crawler.pike",
	"single_process",
      }),
      ([  "env":env,"priority":"low"  ]) );
    return 1;
  }

  public int(0..1) stop_indexer_process( )
  {
    if( !crawler )
      return 0;

    crawler_remote = 0;

#ifdef __NT__
    crawler->kill( 9 );
#else
    crawler->kill( signum( "SIGINT" ) );
#endif

    setup_conf();
    if(userpass && conf)
    {
      remove_backdoor_userpass(userpass);
      userpass = 0;
    }

    return 1;
  }

  string _sprintf() {
    return "Indexer(" + db + ")";
  }
}

class Compactor {
  Process.Process pid;
  function cb;
  void is_perchance_done( )
  {
    if( !pid->kill( 0 ) )
    {
      report_notice("Compactor: Done.\n");
      if( pid->status() )
	report_notice("Compactor: %O\n", pid->status() );
      else
	report_notice("Compactor: All OK\n");
      return 0;
    }
    roxen.background_run( 10, is_perchance_done );
  }

  void create( string db, string log_db, int id, function done )
  {
    cb = done;
    mapping env = getenv();
    env["S_LOGURL"] = log_db;
    env["S_DB"]     = db;
    env["S_PROFILE"]= id;

    pid = Process.create_process(
      ({
	pike_binary,
	"-M",
	"modules/search/pike-modules/",
	"modules/search/programs/compact.pike",
      }),
      ([  "env":env,"priority":"low"  ]) );
    is_perchance_done();
  }
}
#endif
