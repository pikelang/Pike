#pike __REAL_VERSION__

// This module implements a generic web crawler.
// Features:
// o Fully asynchronous operation (Several hundred simultaneous requests)
// o Supports the /robots.txt exclusion standard
// o Extensible
//   - URI Queues
//   - Allow/Deny rules
// o Configurable
//   - Number of concurrent fetchers
//   - Bits per second (bandwidth throttling)
//   - Number of concurrent fetchers per host
//   - Delay between fetches from the same host
// o Supports HTTP and HTTPS

// Author:  Johan Schön.
// Copyright (c) Roxen Internet Software 2001
// $Id: Crawler.pmod,v 1.5 2001/05/27 17:49:25 js Exp $

#define CRAWLER_DEBUG
#ifdef CRAWLER_DEBUG
# define CRAWLER_MSG(X) werror("Web.Crawler: "+X+"\n")
# define CRAWLER_MSGS(X, Y) werror("Web.Crawler: "+X+"\n", Y)
#else
# define CRAWLER_MSG(X)
# define CRAWLER_MSGS(X, Y)
#endif

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

class Policy
{
  int max_concurrent_fetchers = 100;
  int max_bits_per_second_total = 0;
  int max_bits_per_second_per_host = 0;
  int bandwidth_throttling_floating_window_width = 30;
  int max_concurrent_fetchers_per_host = 1;
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

  void done(Standards.URI uri);

  int check_link(Standards.URI link, RuleSet allow, RuleSet deny)
  {
    if(link->fragment)
      link->fragment=0;
//        werror("allow: %d  deny: %d (%s)\n",
//  	     allow->check(link), deny->check(link),
//  	     (string)link);
    return !deny->check(link) || allow->check(link);
  }
}

class Rule
{
  int check(Standards.URI uri);
}

class GlobRule(string pattern)
{
  inherit Rule;

  int check(Standards.URI uri)
  {
    return glob(pattern, (string)uri);
  }
}

class RegexpRule
{
  inherit Rule;

  static private Regexp regexp;
  
  void create(string re)
  {
    regexp=Regexp(re);
  }

  int check(Standards.URI uri)
  {
    return regexp->match((string)uri);
  }
}

class RuleSet
{
  multiset rules = (<>);
  
  void add_rule(Rule rule)
  {
    rules[rule]=1;
  }
  
  void remove_rule(Rule rule)
  {
    rules[rule]=0;
  }

  int check(Standards.URI uri)
  {
    foreach(indices(rules), Rule rule)
      if(rule->check(uri))
	return 1;
    return 0;	 
  }
}

class MySQLQueue
{
  Stats stats;
  Policy policy;
  RuleSet allow,deny;

  string host;
  string table;
  
  Sql.Sql db;
  
  inherit Queue;

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
		"uri varchar(255) not null unique primary key)" );
    };
  }

  static int done_uri( string|Standards.URI uri )
  {
    return sizeof(db->query("select done from "+
			    table+" where done=2 and uri=%s",
			    (string)uri));
  }

  static int has_uri( string|Standards.URI uri )
  {
    return sizeof(db->query("select done from "+table+" where uri=%s",
			    (string)uri));
  }

  static void add_uri( Standards.URI uri )
  {
    if(((!allow && !deny) || check_link(uri, allow, deny))
       && !has_uri(uri))
      db->query( "insert into "+table+" (uri) values (%s)", (string)uri );
  }

  static int empty_count;

  int|Standards.URI get()
  {
    if(stats->concurrent_fetchers() > policy->max_concurrent_fetchers)
      return -1;

    db->query( "select GET_LOCK('"+table+"_query',400)");
    array possible =
      db->query( "select uri from "+table+" where done=0 limit 1" );
    if( sizeof( possible ) )
    {
      string u = possible[0]->uri;
      empty_count=0;
      db->query( "UPDATE "+table+" SET done=1 WHERE uri=%s", (string)u );
      db->query( "select RELEASE_LOCK('"+table+"_query')");
      return Standards.URI(u);
    }
    db->query( "select RELEASE_LOCK('"+table+"_query')");

    if(stats->concurrent_fetchers())
      return -1;

    // delay for (quite) a while.
    if( empty_count++ > 100 )
      return 0;
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

class MemoryQueue
{
  Stats stats;
  Policy policy;
    

  void create(Stats _stats, Policy _policy)
  {
    stats=_stats;
    policy=_policy;
    werror("stats: %O\n",indices(stats));
  }
  
  inherit Queue;

  private mapping ready_uris=([]);
  private mapping done_uris=([]);

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

    werror("concurrent_fetchers: %d\n",stats->concurrent_fetchers());
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
    if(!stringp(uri))
      uri=(string)uri;

    if(!ready_uris[uri] && !done_uris[uri])
      ready_uris[uri]=1;
  }

  void done(Standards.URI uri)
  {
    m_delete(ready_uris, (string)uri);
    done_uris[(string)uri]=1;
  }
}


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
      return Crypto.md5()->update(in)->digest();
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
  
  void create(Standards.URI _base_uri,
	      function _done_cb)
  {
    base_uri=_base_uri; done_cb=_done_cb;
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
      async_fetch(request_ok, 1);
    else
    {
      if(httpquery->status!=200)
	reject_globs=({ });
      else
 	reject_globs=parse_robot_txt(httpquery->data(), base_uri);
      got_reply=1;
      
      done_cb(this_object());
    }
  }
  
  void request_fail(object httpquery)
  {
    failed=0;
    done_cb(this_object());
  }
      
  // Given the contents of a /robots.txt file and it's corresponding
  // base URI, return an array of globs. Any URI that matches any of
  // these globs should not be fetched.  See
  // http://info.webcrawler.com/mak/projects/robots/norobots.html
  array(string) parse_robot_txt(string robottxt, Standards.URI uri)
  {
    array(string) collect_rejected=({});
    int rejected=1;
    foreach( robottxt/"\n"-({""}), string line )
    {
      line -= "\r";
      string field, value;
      if(sscanf(line, "%s:%*[ \t]%[^ \t#]", field, value)==3)
      {
	switch(lower_case(field))
	{
          case "user-agent":
	    rejected=glob(value, "PikeCrawler");
	    break;
          case "disallow":
	    if(rejected)
	      collect_rejected+=({ (string)Standards.URI(value, uri) });
	    break;
	}
      }
    }
    return collect_rejected;
  }
}

class Crawler
{
  Queue queue;
  function page_cb, done_cb;

  array(mixed) args;

  mapping _hostname_cache=([]);
  
  class HTTPFetcher
  {
    inherit Protocols.HTTP.Query;
    Standards.URI uri;
    
    void got_data()
    {
      queue->stats->close_callback(uri);
      if(status==200)
	add_links(page_cb(uri, data(), headers, @args));

      if(headers->location)
	add_links(({ Standards.URI(headers->location) }));

      queue->done(uri);
    }
    
    void request_ok(object httpquery)
    {
      async_fetch(got_data);
    }
    
    void request_fail(object httpquery)
    {
      queue->stats->close_callback(uri);
      queue->done(uri);
    }
    
    void create(Standards.URI _uri)
    {
      string get_path_query(  Standards.URI u )
      {
	return u->path + (u->query?"?"+u->query:"");
      };
      uri=_uri;
      hostname_cache=_hostname_cache;
      set_callbacks(request_ok, request_fail);
      async_request(uri->host, uri->port,
		    sprintf("GET %s HTTP/1.0", get_path_query(uri)),
		    (["host": uri->host+":"+uri->port,
		      "user-agent": "Mozilla 4.0 (PikeCrawler)",
		    ]));
    }
  }

  void add_links(array(Standards.URI) links)
  {
    map(links,queue->put);
  }
  
  void get_next_uri()
  {
    //    CRAWLER_MSG("get_next_uri");
    object|int uri=queue->get();
//      CRAWLER_MSGS("uri: %O",uri);

//      queue->debug();

    if(uri==-1)
    {
      call_out(get_next_uri,0.025);
      return;
    }

    if(!uri)
    {
      done_cb();
      return;
    }

    queue->stats->start_fetch(uri->host);
    
    
    if(objectp(uri))
    {
      switch(uri->scheme)
      {
        case "http":
        case "https": HTTPFetcher(uri);
      }
    }
  
    call_out(get_next_uri,0.025);
  }
  
  void create(Queue _queue,
	      function _page_cb, function _done_cb,
	      string|array(string)|Standards.URI|array(Standards.URI) start_uri,
	      mixed ... _args)
  {
    queue=_queue;
    args=_args;
    page_cb=_page_cb;
    done_cb=_done_cb;
    
    queue->put(start_uri);
    call_out(get_next_uri,0);
  }
}
