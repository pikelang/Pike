// Remote SQL server interface

#pike __REAL_VERSION__

#define RSQL_PORT 3994
#define RSQL_VERSION 1

#if constant(thread_create)
#define LOCK object key=mutex->lock()
#define UNLOCK destruct(key)
protected private object(Thread.Mutex) mutex = Thread.Mutex();
#else
#define LOCK
#define UNLOCK
#endif

#define ERROR(X ...) predef::error(X)

inherit __builtin.Sql.Connection;

protected object(Stdio.File) sock;
protected int seqno = 0;

protected private string host, user, pw;
protected private int port;
protected private mapping options;

protected private string db;

protected void low_reconnect()
{
  object losock = Stdio.File();
  if(sock)
    destruct(sock);
  if (host == "") {
#ifdef ENABLE_SPAWN_RSQLD
    Process.spawn_pike(({ "-x", "rsqld", "--stdin" }), ([
			 "stdin":losock->pipe(),
		       ]));
#else /* !ENABLE_SPAWN_RSQLD */
    ERROR("Automatic spawning of rsqld not enabled.\n");
#endif /* ENABLE_SPAWN_RSQLD */
  } else {
    if(!losock->connect(host, port||RSQL_PORT))
      ERROR("Can't connect to "+host+(port? ":"+port:"")+": "+
            strerror(losock->errno())+".\n");
  }
  if(8!=losock->write("RSQL%4c", RSQL_VERSION) ||
     losock->read(4) != "SQL!") {
    destruct(losock);
    ERROR("Initial handshake error on "+host+(port? ":"+port:"")+"\n");
  }
  sock = losock;
  // FIXME: Propagate options?
  if(!do_request('L', ({user,pw}), 1)) {
    sock = 0;
    if(losock)
      destruct(losock);
    ERROR("Login refused on "+host+(port? ":"+port:"")+"\n");
  }
}

protected void low_connect(string the_host, int the_port, string the_user,
			   string the_pw, mapping the_options)
{
  host = the_host;
  port = the_port;
  user = the_user;
  pw = the_pw;
  options = the_options;
  low_reconnect();
}

protected mixed do_request(int cmd, mixed|void arg, int|void noreconnect)
{
  LOCK;
  noreconnect = noreconnect || !options || !options->reconnect;
  if(!sock)
    if(noreconnect) {
      UNLOCK;
      ERROR("No connection\n");
    } else {
      UNLOCK;
      low_reconnect();
      select_db(db);
      LOCK;
      noreconnect = 1;
    }
  arg = (arg? encode_value(arg) : "");
  sock->write("?<%c>%4c%4c%s", cmd, ++seqno, sizeof(arg), arg);
  string res;
  int rlen;
  if((res = sock->read(12)) && sizeof(res)==12 &&
     res[1..7]==sprintf("<%c>%4c", cmd, seqno) &&
     sscanf(res[8..11], "%4c", rlen)==1 && (res[0]=='.' || res[0]=='!')) {
    mixed rdat = (rlen? sock->read(rlen) : "");
    if((!rdat) || sizeof(rdat)!=rlen) {
      destruct(sock);
      UNLOCK;
      if(noreconnect)
	ERROR("RSQL Phase error, disconnected\n");
      else return do_request(cmd, arg);
    }
    UNLOCK;
    rdat = (sizeof(rdat)? decode_value(rdat):0);
    switch(res[0]) {
    case '.': return rdat;
    case '!': ERROR(rdat);
    }
    ERROR("Internal error\n");
  } else {
    destruct(sock);
    UNLOCK;
    if(noreconnect)
      ERROR("RSQL connection closed\n");
    else return do_request(cmd, arg);
  }
}

protected mixed do_proxy(string cmd, array(mixed) args)
{
  return do_request('P', ({ cmd, args }));
}

void select_db(string the_db)
{
  do_request('D', the_db);
  db = the_db;
}

int|string error()
{
  return do_request('E');
}

void create_db(string db)
{
  do_request('C', db);
}

void drop_db(string db)
{
  do_request('X', db);
}

string server_info()
{
  return do_request('I');
}

string host_info()
{
  return do_request('i');
}

void shutdown()
{
  do_request('s');
}

int ping()
{
  catch {
    return do_request('p', UNDEFINED, 1);
  };
  catch {
    if (do_request('p') >= 0) return 1;
  };
  return -1;
}

void reload()
{
  do_request('r');
}

array(string) list_dbs(string|void wild)
{
  return do_request('l', wild);
}

array(string) list_tables(string|void wild)
{
  return do_request('t', wild);
}

array(mapping(string:mixed)) list_fields(string ... args)
{
  return do_request('f', args);
}

string quote(string s)
{
  return do_request('q', s);
}

protected class RemoteResult(protected function(int,mixed:mixed) do_request,
			     protected mixed qid)
{
  inherit __builtin.Sql.Result;

  protected void _destruct()
  {
    do_request('Z', qid);
  }

  int|array(string|int) fetch_row()
  {
    return do_request('R', qid);
  }

  array(mapping(string:mixed)) fetch_fields()
  {
    return do_request('F', qid);
  }

  int num_rows()
  {
    return do_request('N', qid);
  }

  int num_fields()
  {
    return do_request('n', qid);
  }

  int eof()
  {
    return do_request('e', qid);
  }

  void seek(int skip)
  {
    do_request('S', ({qid,skip}));
  }
}

variant RemoteResult big_query(object|string q)
{
  mixed qid = do_request('Q', q);
  return qid && RemoteResult(do_request, qid);
}

variant RemoteResult big_typed_query(object|string q)
{
  mixed qid = do_request('Q', ({ "big_typed_query", q }));
  return qid && RemoteResult(do_request, qid);
}

variant RemoteResult streaming_query(object|string q)
{
  mixed qid = do_request('Q', ({ "streaming_query", q }));
  return qid && RemoteResult(do_request, qid);
}

variant RemoteResult streaming_typed_query(object|string q)
{
  mixed qid = do_request('Q', ({ "streaming_typed_query", q }));
  return qid && RemoteResult(do_request, qid);
}

array(mapping(string:mixed)) query(mixed ... args)
{
  return do_request('@', args);
}

int insert_id()
{
  return do_request('#');
}

string get_charset()
{
  return do_request('h');
}

void set_charset(string charset)
{
  do_request('H', charset);
}

protected function|mixed `->(string cmd)
{
  return ::`->(cmd) ||
    lambda(mixed ... args) {
      return do_proxy(cmd, args);
    };
}

protected void create(string|void host, string|void db, string|void user,
		      string|void _pw, mapping|void options)
{
  string|zero pw = _pw;
  _pw = "CENSORED";

  // Reconstruct the original URL (minus rsql://)

  if(!host) {
    destruct(this);
    return;
  }
  if(db)
    host = host+"/"+db;
  if(pw)
    user = (user||"")+":"+pw;
  if(user)
    host = user+"@"+host;

  array(string) arr = host/"/";
  if(sizeof(arr)<2) {
    destruct(this);
    return;
  }

  db = arr[1..]*"/";
  host = arr[0];
  user = pw = 0;

  arr = host/"@";
  if(sizeof(arr)>1) {
    user = arr[..<1]*"@";
    host = arr[-1];
    arr = user/":";
    if(sizeof(arr)>1) {
      pw = arr[1..]*":";
      user = arr[0];
    }
  }
  int port = 0;
  sscanf(host, "%s:%d", host, port);
  low_connect(host, port, user, pw, options);
  select_db(db);
}
