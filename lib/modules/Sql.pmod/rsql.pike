// Remote SQL server interface

#define RSQL_PORT 3994
#define RSQL_VERSION 1

static object(Stdio.File) sock;
static int seqno = 0;

static mixed do_request(int cmd, mixed|void arg)
{
  if(!sock)
    throw(({"Not connected.\n", backtrace()}));
  arg = (arg? encode_value(arg) : "");
  sock->write(sprintf("?<%c>%4c%4c%s", cmd, ++seqno, sizeof(arg), arg));
  string res;
  int rlen;
  if((res = sock->read(12)) && sizeof(res)==12 &&
     res[1..7]==sprintf("<%c>%4c", cmd, seqno) &&
     sscanf(res[8..11], "%4c", rlen)==1 && (res[0]=='.' || res[0]=='!')) {
    mixed rdat = (rlen? sock->read(rlen) : "");
    if((!rdat) || sizeof(rdat)!=rlen) {
      destruct(sock);
      throw(({"RSQL Phase error, disconnected\n", backtrace()}));
    }
    rdat = (sizeof(rdat)? decode_value(rdat):0);
    switch(res[0]) {
    case '.': return rdat;
    case '!': throw(rdat);
    }
    throw(({"Internal error\n", backtrace()}));    
  } else {
    destruct(sock);
    throw(({"RSQL Phase error, disconnected\n", backtrace()}));
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

void server_info()
{
  do_request('I');
}

object big_query(string q)
{
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

    void create(function(int,mixed:mixed) d_r, mixed i)
    {
      do_request = d_r;
      qid = i;
    }

  }(do_request, qid);
}

static void low_connect(string host, int port, string user, string pw)
{
  object losock = Stdio.File();
  if(!losock->connect(host, port|RSQL_PORT))
    throw(({"Can't connect to "+host+(port? ":"+port:"")+": "+
	    strerror(losock->errno())+"\n", backtrace()}));
  if(8!=losock->write(sprintf("RSQL%4c", RSQL_VERSION)) ||
     losock->read(4) != "SQL!") {
    destruct(losock);
    throw(({"Initial handshake error on "+host+(port? ":"+port:"")+"\n",
	    backtrace()}));    
  }
  sock = losock;
  if(!do_request('L', ({user,pw}))) {
    sock = 0;
    destruct(losock);
    throw(({"Login refused on "+host+(port? ":"+port:"")+"\n",
	    backtrace()}));        
  }
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
