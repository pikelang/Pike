#pike __REAL_VERSION__

//! This module implements a generic web crawler.
//!
//! Features:
//! @dl
//!   @item
//!     Fully asynchronous operation (Several hundred simultaneous requests)
//!   @item
//!     Supports the /robots.txt exclusion standard
//!   @item
//!     Extensible
//!     @dl 
//!       @item
//!         URI Queues
//!       @item
//!         Allow/Deny rules
//!     @enddl
//!   @item
//!     Configurable
//!     @dl
//!       @item
//!         Number of concurrent fetchers
//!       @item
//!         Bits per second (bandwidth throttling)
//!       @item
//!         Number of concurrent fetchers per host
//!       @item
//!         Delay between fetches from the same host
//!     @enddl
//!   @item
//!     Supports HTTP and HTTPS
//! @enddl

// Author:  Johan Schön.
// $Id: Crawler.pmod,v 1.21 2004/05/18 10:14:34 nilsson Exp $

#define CRAWLER_DEBUG
#ifdef CRAWLER_DEBUG
# define CRAWLER_MSG(X) werror("Web.Crawler: "+X+"\n")
# define CRAWLER_MSGS(X, Y) werror("Web.Crawler: "+X+"\n", Y)
#else
# define CRAWLER_MSG(X)
# define CRAWLER_MSGS(X, Y)
#endif

//! Statistics.
class Stats(int window_width,
	    int granularity)
{
  
  class FloatingAverage
  {
    // Cache
    static private int amount;
    static private int amount_last_calculated;
    
    static private int num_slots;
    
    static private array(int) amount_floating;

    void create(int _window_width,
		int _granularity)
    {
      window_width=_window_width;
      granularity=_granularity;
      num_slots = window_width/granularity;
      amount_floating =  allocate(num_slots);
    }
    
    void add(int n, int when)
    {
      amount_floating[(when % window_width) / granularity] += n;
    }
    
    int get()
    {
      if(time()==amount_last_calculated)
	return amount;
      
      amount=amount/window_width;
      return amount;
    }
  }


  static private FloatingAverage bps_average = FloatingAverage(10,1);
  static private mapping(string:FloatingAverage) host_bps_average;
  
  int bits_per_second(string|void host)
  {
    if(host)
    {
      if(!host_bps_average[host])
	return 0;

      return host_bps_average[host]->get();
    }
    else
      return bps_average->get();
  }

  static private mapping(string:int) concurrent_fetchers_per_host = ([]);
  static private int concurrent_fetchers_total = 0;

  int concurrent_fetchers(void|string host)
  {
    if(host)
      return concurrent_fetchers_per_host[host];
    else
      return concurrent_fetchers_total;
  }

  int start_fetch(string host)
  {
    concurrent_fetchers_per_host[host]++;
    concurrent_fetchers_total++;
  }
  
  //! This callback is called when data has arrived for a presently crawled
  //! URI, but no more often than once a second.
  void bytes_read_callback(Standards.URI uri, int num_bytes_read)
  {
    
  }

  //! This callback is called whenever the crawling of a URI is finished or
  //! fails.
  void close_callback(Standards.URI uri)
  {
    concurrent_fetchers_per_host[uri->host]--;
    concurrent_fetchers_total--;
  }
  
}

//! The crawler policy object.
class Policy
{
  //! Maximum number of fetchers. Defaults to 100.
  int max_concurrent_fetchers = 100;

  //! Maximum number of bits per second. Defaults to off (0).
  int max_bits_per_second_total = 0;

  //! Maximum number of bits per second, per host. Defaults to off (0).
  int max_bits_per_second_per_host = 0;

  //! Bandwidth throttling floating window width. Defaults to 30.
  int bandwidth_throttling_floating_window_width = 30;

  //! Maximum concurrent fetchers per host. Defaults to 1.
  int max_concurrent_fetchers_per_host = 1;

  //! Minimum delay per host. Defaults to 0.
  int min_delay_per_host = 0;
}


//! A crawler queue.
//! Does not need to be reentrant safe. The Crawler always runs
//! in just one thread.
class Queue
{
  //! Get the next URI to index.
  //! Returns -1 if there are no URIs to index at the time of the function call,
  //! with respect to bandwidth throttling and other limits.
  //! Returns 0 if there are no more URIs to index.
  int|Standards.URI get();

  //! Put one or several URIs in the queue.
  //! Any URIs that were already present in the queue are silently
  //! disregarded.
  void put(string|array(string)|Standards.URI|array(Standards.URI) uri);

  void done(Standards.URI uri, int|void callback_called);

  string normalize_query( string query )
  {
    array q = query / "&";

    if( sizeof( q ) == 1 )
      return query;

    multiset h = (<>);
    string res = "";
    foreach( q, string p )
    {
      string a, b;
      if( sscanf( p, "%s=%s", a, b ) == 2 )
	if( !h[a]++ )
	  res += "&"+a+"="+b;
    }
    if( !sizeof( res ) )
      return 0;
    return res[1..];
  }

  int check_link(Standards.URI link, RuleSet allow, RuleSet deny)
  {
    if(link->fragment)
      link->fragment=0;

    /*  a  d  check_link
        -  -  ----------
        0  0           0
	0  1           0
	1  1           0
	1  0           1
    */

    int a = 1, d = 0;

    if( allow ) a = allow->check(link);
    if( deny )  d = deny->check(link);
    return a && !d;
  }
}

//! Abstract rule class.
class Rule
{
  //!
  int check(string|Standards.URI uri);
}

//! A rule that uses glob expressions
//! @param pattern
//!  a glob pattern that the rule will match against.
//! @example
//! GlobRule("http://pike.ida.liu.se/*.xml");
class GlobRule(string pattern)
{
  inherit Rule;  

  int check(string|Standards.URI uri)
  {
    return glob(pattern, (string)uri);
  }
}

//! A rule that uses @[Regexp] expressions
class RegexpRule
{
  inherit Rule;

  static private Regexp regexp;

//!  
//! @param re
//!   a string describing the @[Regexp] expression
  void create(string re)
  {
    regexp=Regexp(re);
  }

  int check(string|Standards.URI uri)
  {
    return regexp->match((string)uri);
  }
}

//! A set of rules
class RuleSet
{
  multiset rules = (<>);

//!  add a rule to the ruleset
  void add_rule(Rule rule)
  {
    rules[rule]=1;
  }

//! remove a rule from the ruleset  
  void remove_rule(Rule rule)
  {
    rules[rule]=0;
  }

  int check(string|Standards.URI uri)
  {
    foreach(indices(rules), Rule rule)
      if(rule->check(uri))
	return 1;
    return 0;	 
  }
}

//!
class MySQLQueue
{
  Stats stats;
  Policy policy;
  RuleSet allow,deny;

  string host;
  string table;
  
  Sql.Sql db;
  
  inherit Queue;

//!
  void create( Stats _stats, Policy _policy, string _host, string _table,
	       void|RuleSet _allow, void|RuleSet _deny)
  {
    stats = _stats;
    policy = _policy;
    host = _host;
    table = _table;
    allow=_allow;
    deny=_deny;

    db = Sql.Sql( host );

    catch
    {
      db->query("create table "+table+" ("
		"done int(2) unsigned not null, "
		"uri varchar(255) binary not null unique,"
		"KEY uri_index (uri(64)),"
		"INDEX done_i  (done))" );
    };
  }

  static int done_uri( string|Standards.URI uri )
  {
    return sizeof(db->query("select done from "+
			    table+" where done=2 and uri=%s",
			    (string)uri));
  }

  mapping hascache = ([]);
  static int has_uri( string|Standards.URI uri )
  {
    uri = (string)uri;
    if( sizeof(hascache) > 100000 )
      hascache = ([]);
    return hascache[uri]||
      (hascache[uri]=sizeof(db->query("select done from "+
				     table+" where uri=%s",uri)));
  }

  static void add_uri( Standards.URI uri )
  {
    if(check_link(uri, allow, deny) && !has_uri(uri))
    {
      db->query( "insert into "+table+" (uri) values (%s)", (string)uri );
    }
  }

  static int empty_count;

  static array possible=({});
  int p_c;
  int|Standards.URI get()
  {
    if(stats->concurrent_fetchers() > policy->max_concurrent_fetchers)
      return -1;

    if( sizeof( possible ) <= p_c )
    {
      p_c = 0;
      possible=db->query("select uri from "+table+" where done=0 limit 20")
	->uri;
    }

    if( sizeof( possible ) > p_c )
    {
      string u = possible[p_c++];
      empty_count=0;
      db->query( "UPDATE "+table+" SET done=1 WHERE uri=%s", (string)u );
      return Standards.URI(u);
    }

    if(stats->concurrent_fetchers())
    {
      return -1;
    }
    // delay for (quite) a while.
    if( empty_count++ > 1000 )
    {
      return 0;
    }
    return -1;
  }

  void put(string|array(string)|Standards.URI|array(Standards.URI) uri)
  {
    if(arrayp(uri))
    {
      foreach(uri, string|object _uri)
        put(_uri);
      return;
    }
    if(!objectp(uri))
      uri=Standards.URI(uri);
    
    add_uri(uri);
  }

  void done(Standards.URI uri)
  {
    db->query( "UPDATE "+table+" SET done=2 WHERE uri=%s", (string)uri );
  }
}

//! Memory queue
class MemoryQueue
{
  Stats stats;
  Policy policy;
  RuleSet allow;
  RuleSet deny;

//!
  void create(Stats _stats, Policy _policy, RuleSet _allow, RuleSet _deny)
  {
    stats=_stats;
    policy=_policy;
    deny=_deny;
    allow=_allow;
  }
  
  inherit Queue;

  private mapping ready_uris=([]);
  private mapping done_uris=([]);

  mapping stage=([]);

  void set_stage(Standards.URI real_uri, int s)
  { 
    ready_uris[(string)real_uri]=s;
  }

  int get_stage(Standards.URI real_uri)
  { 
    if(ready_uris[(string)real_uri])
      return ready_uris[(string)real_uri];
  }

  void debug()
  {
    werror("ready_uris: %O\n",ready_uris);
    werror("done_uris: %O\n",done_uris); 
  }
  
  //! Get the next URI to index.
  //! Returns -1 if there are no URIs to index at the time of the function call,
  //! with respect to bandwidth throttling, outstanding requests and other limits.
  //! Returns 0 if there are no more URIs to index. 
  int|Standards.URI get()
  {
    if(stats->concurrent_fetchers() > policy->max_concurrent_fetchers)
      return -1;
    if(sizeof(ready_uris))
    {
      foreach(indices(ready_uris), string ready_uri)
	if(ready_uris[ready_uri] != 2)
	{
	  ready_uris[ready_uri]=2;
	  return Standards.URI(ready_uri);
	}
    }

    if(stats->concurrent_fetchers())
      return -1;
    return 0;
  }

  //! Put one or several URIs in the queue.
  //! Any URIs that were already present in the queue are silently
  //! disregarded.
  void put(string|array(string)|Standards.URI|array(Standards.URI) uri)
  {
    if(arrayp(uri))
    {
      foreach(uri, string|object _uri)
        put(_uri);
      return;
    }
    
    Standards.URI ouri;

    if(objectp(uri))
      ouri=uri;
    else
      ouri=Standards.URI(uri);

    if(!stringp(uri))
      uri=(string)uri;

    if(!ready_uris[uri] && !done_uris[uri] && check_link(ouri, allow, deny))
      ready_uris[uri]=1;
  }

  void done(Standards.URI uri)
  {
    m_delete(ready_uris, (string)uri);
    done_uris[(string)uri]=1;
  }
}

//!
class ComplexQueue(Stats stats, Policy policy)
{
  inherit Queue;

  void test()
  {
    werror("Queue: %O\n",stats);
  }
  
  static private ADT.Heap host_heap=ADT.Heap();
  static private mapping(string:URIStack) hosts=([]);

  // One per host
  static private class URIStack
  {
    inherit ADT.Stack;
    int last_mod;
    multiset(string) uris_md5=(<>);
    int num_active;

    string do_md5(string in)
    {
      return Crypto.MD5->hash(in);
    }

    void push(mixed val)
    {
      string md5=do_md5(val);
      if(uris_md5[md5])
	return;
      uris_md5[md5]=1;
      ::push(val);
    }

    mixed pop()
    {
      string uri=::pop();
      if(uri)
	uris_md5[do_md5(uri)]=0;
      return uri;
    }
          
    int `<(mixed other)
    {
      if(policy->max_concurrent_fetchers_per_host &&
         num_active >= policy->max_concurrent_fetchers_per_host)
        return 0;

      // FIXME:
//          if(!size())
//    	return 0;

      return last_mod < other->last_mod;
    }
  }

  void done(Standards.URI uri)
  {
    hosts[uri->host]->num_active--;
  }
  
  int|Standards.URI get()
  {
//      if(policy->max_concurrent_fetchers &&
//         stats->concurrent_fetchers >= policy->max_concurrent_fetchers)
//        return -1;

    if(policy->max_bits_per_second_total &&
       stats->bits_per_second_total >= policy->max_bits_per_second_total)
      return -1;

    //    policy->min_delay_per_host
    URIStack uri_stack=host_heap->top();
    if(!uri_stack->size())
      return 0;
    
    if(policy->max_concurrent_fetchers_per_host &&
       uri_stack->num_active >=
       policy->max_concurrent_fetchers_per_host)
    {
      host_heap->push(uri_stack);
      return -1;
    }

    string|object uri=uri_stack->pop();
    host_heap->push(uri_stack);

    uri=Standards.URI(uri);
    return uri;
  }
  
  void put(string|array(string)|Standards.URI|array(Standards.URI) uri)
  {
    CRAWLER_MSGS("SimpleQueue->put(%O)",uri);
    if(arrayp(uri))
    {
      foreach(uri, string|object _uri)
        put(_uri);
      return;
    }
    if(!objectp(uri))
      uri=Standards.URI(uri);

    if(!hosts[uri->host])
    {
      hosts[uri->host]=URIStack();
      host_heap->push( hosts[uri->host] ); 
    }
    
    URIStack uri_stack = hosts[uri->host];
    uri_stack->push( (string)uri );
    if(uri_stack->size()==1)
      host_heap->adjust(uri_stack);
  }
}


class RobotExcluder
{
  inherit Protocols.HTTP.Query;
  int got_reply;
  int failed;
  array(string) reject_globs=({ });
  string host;
  Standards.URI base_uri;
  function done_cb;
  array(mixed) args;
  string user_agent;

  void create(Standards.URI _base_uri, function _done_cb,
	      void|mixed _user_agent, void|mixed ... _args)
  {
    base_uri=_base_uri; done_cb=_done_cb;
    user_agent = _user_agent || "PikeCrawler";
    args = _args;
    set_callbacks(request_ok, request_fail);
    async_request(base_uri->host, base_uri->port,
		  "GET /robots.txt HTTP/1.0",
		  ([ "Host":base_uri->host+":"+base_uri->port]));
  }
  
  int check(string uri)
  {
    foreach(reject_globs, string gl)
      if(glob(gl, uri))
	return 0;
    return 1;
  }
  
  void request_ok(object httpquery, int gotdata)
  {
    if(!gotdata)
      async_fetch(request_ok, httpquery, 1);
    else
    {
      if(httpquery->status!=200)
	reject_globs=({ });
      else
 	reject_globs=parse_robot_txt(httpquery->data(), base_uri);
      got_reply=1;
      
      done_cb(this, @args);
    }
  }
  
  void request_fail(object httpquery)
  {
    failed=0;
    done_cb(this);
  }
      
  // Given the contents of a /robots.txt file and it's corresponding
  // base URI, return an array of globs. Any URI that matches any of
  // these globs should not be fetched.  See
  // http://info.webcrawler.com/mak/projects/robots/norobots.html
  array(string) parse_robot_txt(string robottxt, Standards.URI uri)
  {
    array(string) collect_rejected=({});
    int rejected=0,
      parsed_disallow=0; 
    foreach( robottxt/"\n"-({""}), string line )
    {
      line -= "\r";
      string field, value;
      if(sscanf(line, "%s:%*[ \t]%[^ \t#]", field, value)==3)
      {
	if(lower_case(field)=="user-agent")
	{
	  if(parsed_disallow)
	  {
	    if(rejected==2)
	      break;
	    parsed_disallow=rejected=0;
	  }
	  if(value=="*")
	  {
	    if(rejected==0)
	      rejected=1;
	    else if(rejected==2)
	      rejected=0;
	  }
	  else if(has_value(lower_case(user_agent), lower_case(value)))
	  {
	    switch(rejected)
	    {
	      case 0: rejected=2; break;
	      case 1: rejected=2; collect_rejected = ({}); break;
	      case 2: if(sizeof(collect_rejected)) rejected=0; break;
	    }
	  }
	}
	else if(lower_case(field)=="disallow")
	{
	  if(rejected)
	  {
	    if (!sizeof(value))
	      collect_rejected = ({});
	    else
	      collect_rejected+=({ (string)Standards.URI(value+"*", uri) });
	  }
	  parsed_disallow=1;
	}
      }
    }
    return collect_rejected;
  }
}

//!
class Crawler
{
  Queue queue;
  function page_cb, done_cb, error_cb;
  function prepare_cb;
  
  array(mixed) args;

  mapping _hostname_cache=([]);
  mapping _robot_excluders=([]);

  class HTTPFetcher
  {
    inherit Protocols.HTTP.Query;
    Standards.URI uri, real_uri;
    
    void got_data()
    {
      queue->stats->close_callback(real_uri);
      if(status>=200 && status<=206)
      {
	add_links(page_cb(real_uri, data(), headers, @args));
      }
      else
      {
	error_cb(real_uri, status, headers, @args);
	if(status>=300 && status <=307)
	  if(headers->location && sizeof(headers->location))
	    add_links(({ Standards.URI(headers->location, real_uri) }));
      }
      if(queue->get_stage(real_uri)<=1)
	queue->set_stage(real_uri, 5);
    }
    
    void request_ok(object httpquery)
    {
      async_fetch(got_data);
    }
    
    void request_fail(object httpquery)
    {
      queue->stats->close_callback(real_uri);
      error_cb(real_uri, 1100, ([]));
      if(queue->get_stage(real_uri)<=1)
	queue->set_stage(real_uri, 6);
    }
    
    void create(Standards.URI _uri, void|Standards.URI _real_uri, mapping extra_headers)
    {
      string pq;
      mapping headers;
      uri=_uri;
      real_uri=_real_uri;
      if(!real_uri)
	real_uri=uri;

      headers = ([
	"host": uri->host+":"+uri->port,
	"user-agent": "Mozilla 4.0 (PikeCrawler)",
      ]);
      if(extra_headers)
	headers |= extra_headers;

      hostname_cache=_hostname_cache;
      set_callbacks(request_ok, request_fail);

      https= (uri->scheme=="https")? 1 : 0;
      async_request(uri->host, uri->port,
		    sprintf("GET %s HTTP/1.0", uri->get_path_query()),
		    headers );
    }
  }

  void add_links( array(Standards.URI) links )
  {
    map(links,queue->put);
  }

  void got_robot_excluder(RobotExcluder excl, Standards.URI _real_uri, mapping _headers)
  {
    get_next_uri(excl->base_uri, _real_uri, _headers);
  }
  
  void get_next_uri(void|Standards.URI _uri, void|Standards.URI _real_uri,
		    void|mapping _headers)
  {
    object|int uri;
    mapping headers;
    Standards.URI real_uri;

    if (_uri) {
      uri = _uri;
      real_uri = _real_uri;
      headers = _headers;
    }
    else
    {
      uri=queue->get();

      if(uri==-1)
      {
	call_out(get_next_uri,0.1);
	return;
      }

      if(!uri)
      {
	done_cb();
	return;
      }

     if(!headers)
       headers=([
	"host": uri->host+":"+uri->port,
	"user-agent": "Mozilla 4.0 (PikeCrawler)" ]);


      queue->stats->start_fetch(uri->host);

      real_uri = uri;
      if( prepare_cb )
	[uri, headers] = prepare_cb( uri );
    }

    if(objectp(uri))
    {
      switch(uri->scheme)
      {
        case "http":
        case "https":
	  string site = uri->host+":"+uri->port;
	  if(!_robot_excluders[site])
	  {
	    _robot_excluders[site] = RobotExcluder(uri, got_robot_excluder,
						   headers["user-agent"],
						   real_uri, headers);
	    return;
	  }

	  if (!_robot_excluders[site]->check((string)uri))
	  {
	    queue->stats->close_callback(real_uri);
	    error_cb(real_uri, 1000, headers, @args); // robots.txt said no!
	    queue->set_stage(real_uri, 6);
	    call_out(get_next_uri,0);
	    return;
	  }

	  HTTPFetcher(uri, real_uri, headers);
	  break;
        default:
	  queue->stats->close_callback(real_uri);
	  error_cb(real_uri, 1001, headers, @args); // Unknown scheme
	  queue->set_stage(real_uri, 6);
      }
    }
    else
    {
      queue->stats->close_callback(real_uri);
//      queue->set_stage(real_uri, 5);
    }
  
    call_out(get_next_uri,0);
  }

//!  
//!  @param _page_cb
//!    function called when a page is retreived. Arguments are: 
//!    Standards.URI uri, mixed data, mapping headers, mixed ... args. 
//!    should return an array containing additional links found within data
//!    that will be analyzed for insertion into the crawler queue (assuming
//!    they are allowed by the allow/deny rulesets.
//!  @param _error_cb
//!    function called when an error is received from a server. Arguments are:
//!    Standards.URI real_uri, int status_code, mapping headers,
//!    mixed ... args. Returns void. 
//!  @param _done_cb
//!    function called when crawl is complete. Accepts mixed ... args and 
//!    returns void.
//!  @param _prepare_cb
//!    argument called before a uri is retrieved. may be used to alter
//!    the request. Argument is Standards.URI uri. Returns array with
//!    element 0 of Standards.URI uri, element 1 is a header mapping for the
//!    outgoing request.
//!  @param start_uri
//!    location to start the crawl from.
//!  @param _args
//!    optional arguments sent as the last argument to the callback 
//!    functions.
  void create(Queue _queue,
	      function _page_cb, function _error_cb,
	      function _done_cb, function _prepare_cb, 
	      string|array(string)|Standards.URI|
	      array(Standards.URI) start_uri,
	      mixed ... _args)
  {
    queue=_queue;
    args=_args;
    page_cb=_page_cb;
    error_cb=_error_cb;
    done_cb=_done_cb;
    prepare_cb=_prepare_cb;
    
    queue->put(start_uri);
    call_out(get_next_uri,0);
  }
}
