// Remote SQL server interface

#pike __REAL_VERSION__

#define RSQL_PORT 3994
#define RSQL_VERSION 1

#if constant(thread_create)
#define LOCK object key=mutex->lock()
#define UNLOCK destruct(key)
static private object(Thread.Mutex) mutex = Thread.Mutex();
#else
#define LOCK
#define UNLOCK
#endif

#define ERROR(X ...) predef::error(X)

static object(Stdio.File) sock;
static int seqno = 0;

static private string host, user, pw;
static private int port;

static void low_reconnect()
{
  object losock = Stdio.File();
  if(sock)
    destruct(sock);
  if(!losock->connect(host, port||RSQL_PORT))
    ERROR("Can't connect to "+host+(port? ":"+port:"")+": "+
	  strerror(losock->errno())+"\n");
  if(8!=losock->write("RSQL%4c", RSQL_VERSION) ||
     losock->read(4) != "SQL!") {
    destruct(losock);
    ERROR("Initial handshake error on "+host+(port? ":"+port:"")+"\n");
  }
  sock = losock;
  if(!do_request('L', ({user,pw}), 1)) {
    sock = 0;
    if(losock)
      destruct(losock);
    ERROR("Login refused on "+host+(port? ":"+port:"")+"\n");
  }
}

static void low_connect(string the_host, int the_port, string the_user,
			string the_pw)
{
  host = the_host;
  port = the_port;
  user = the_user;
  pw = the_pw;
  low_reconnect();
}

static mixed do_request(int cmd, mixed|void arg, int|void noreconnect)
{
  LOCK;
  if(!sock)
    if(noreconnect) {
      UNLOCK;
      ERROR("No connection\n");
    } else
      low_reconnect();
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
      else return do_request(cmd, arg, 1);
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
      ERROR("RSQL Phase error, disconnected\n");
    else return do_request(cmd, arg, 1);
  }
}

void select_db(string db)
{
  do_request('D', db);
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
  return do_request('q');
}

int|object big_query(object|string q, mapping(string|int:mixed)|void bindings)
{
  if(bindings)
    q=.sql_util.emulate_bindings(q,bindings,this_object());

  mixed qid = do_request('Q', q);
  return qid && class {

    static function(int,mixed:mixed) do_request;
    static mixed qid;

    void destroy()
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

    void create(function(int,mixed:mixed) d_r, mixed i)
    {
      do_request = d_r;
      qid = i;
    }

  }(do_request, qid);
}

array(mapping(string:mixed)) query(mixed ... args)
{
  return do_request('@', args);
}

void create(string|void host, string|void db, string|void user, string|void pw)
{
  // Reconstruct the original URL (minus rsql://)

  if(!host) {
    destruct(this_object());
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
    destruct(this_object());
    return;
  }

  db = arr[1..]*"/";
  host = arr[0];
  user = pw = 0;

  arr = host/"@";
  if(sizeof(arr)>1) {
    user = arr[..sizeof(arr)-2]*"@";
    host = arr[-1];
    arr = user/":";
    if(sizeof(arr)>1) {
      pw = arr[1..]*":";
      user = arr[0];
    }
  }
  int port = 0;
  sscanf(host, "%s:%d", host, port);
  low_connect(host, port, user, pw);
  select_db(db);
}
