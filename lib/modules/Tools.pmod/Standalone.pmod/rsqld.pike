// $Id$

#pike __REAL_VERSION__

constant description = "Implements an rsql daemon.";

#define RSQL_PORT 3994
#define RSQL_VERSION 1


class Connection
{
  static object clntsock;
  static string got, to_write;
  static function ecallback;
  static array ecbextra;
  static int expected;
  static mapping(int:function) commandset = ([]);
  static object sqlobj;
  static mapping(string:object) queries = ([]);
  static int qbase, qid;
  static private mapping(string:string) users;

  static void timeout()
  {
    destruct(clntsock);
    destruct(this_object());    
  }

  static void expect(int n, function cb, mixed ... extra)
  {
    remove_call_out(timeout);
    call_out(timeout, 3600);
    expected = n;
    ecallback = cb;
    ecbextra = extra;
  }

  static void close_callback()
  {
    remove_call_out(timeout);
    destruct(clntsock);
    destruct(this_object());
  }

  static void read_callback(mixed id, string s)
  {
    if(s) {
      got += s;
      while(sizeof(got)>=expected) {
	string v = got[..expected-1];
	got = got[expected..];
	ecallback(v, @ecbextra);
      }
    }
  }

  static void write_callback()
  {
    if(sizeof(to_write)) {
      int n = clntsock->write(to_write);
      if(n>0)
	to_write = to_write[n..];
    }
  }

  static void write(string s)
  {
    if(s && sizeof(s)) {
      to_write += s;
      write_callback();
    }
  }

  static void reply_cmd(int cmd, int seq, int err, mixed val)
  {
    string v = (val? encode_value(val):"");
    write(sprintf("%c<%c>%4c%4c%s", (err? '!':'.'), cmd, seq, sizeof(v), v));
  }

  static void got_cmd(string a, int cmd, int seq)
  {
    reply_cmd(cmd, seq, 1, catch {
      mixed arg = sizeof(a) && decode_value(a);
      function f = commandset[cmd];
      if(!f)
	throw(({"Unknown/unallowed command invoked\n", backtrace()}));
      reply_cmd(cmd, seq, 0, f(arg));
      expect_command();
      return;
    });
    expect_command();
  }

  static void got_cmdhead(string h)
  {
    if(h[..1]!="?<" || h[3]!='>') {
      werror("SYNC ERROR, disconnecting client\n");
      clntsock->set_blocking();
      clntsock->close();
      destruct(clntsock);
      destruct(this_object());
    }
    int seq, alen;
    sscanf(h[4..], "%4c%4c", seq, alen);
    expect(alen, got_cmd, h[2], seq);
  }

  static void expect_command()
  {
    expect(12, got_cmdhead);
  }

  static int cmd_login(array(string) userpw)
  {
    int authorized = 0;

    if(!users)
      authorized = 1;  // No security enabled
    else if(sizeof(userpw) && userpw[0] && !zero_type(users[userpw[0]])) {
      if(!stringp(users[userpw[0]]))
	authorized = 1;  // Password-less user
      else if(sizeof(userpw)>1 && userpw[1] &&
	      crypt(userpw[1], users[userpw[0]]))
	authorized = 1;  // Correct password for user
    }

    if(authorized)
      commandset_1();
    else
      commandset_0();
    return authorized;
  }

  static void cmd_selectdb(string url)
  {
    sqlobj = 0;
    sqlobj = Sql.sql(url);
  }

  static int|string cmd_error()
  {
    return sqlobj->error();
  }

  static void cmd_create(string db)
  {
    sqlobj->create_db(db);
  }

  static void cmd_drop(string db)
  {
    sqlobj->drop_db(db);
  }

  static string cmd_srvinfo()
  {
    return sqlobj->server_info();
  }

  static string cmd_hostinfo()
  {
    return sqlobj->host_info();
  }

  static void cmd_shutdown()
  {
    sqlobj->shutdown();
  }

  static void cmd_reload()
  {
    sqlobj->reload();
  }

  static array(string) cmd_listdbs(string wild)
  {
    return sqlobj->list_dbs(wild);
  }

  static array(string) cmd_listtables(string wild)
  {
    return sqlobj->list_tables(wild);
  }

  static array(mapping(string:mixed)) cmd_listflds(array(string) args)
  {
    return sqlobj->list_fields(@args);
  }

  static private string make_id()
  {
    if(!qid) {
      qid=1;
      qbase++;
    }
    return sprintf("%4c%4c", qbase, qid++);
  }

  static string cmd_bigquery(string q)
  {
    object res = sqlobj->big_query(q);
    if(!res)
      return 0;
    string qid = make_id();
    queries[qid] = res;
    return qid;
  }

  static void cmd_zapquery(string qid)
  {
    m_delete(queries, qid);
  }

  static object get_query(string qid)
  {
    return queries[qid] || 
      (throw (({"Query ID has expired.\n", backtrace()})),0);
  }

  static array(mapping(string:mixed)) cmd_query(array args)
  {
    return sqlobj->query(@args);
  }

  static int|array(string|int) cmd_fetchrow(string qid)
  {
    return get_query(qid)->fetch_row();
  }

  static array(mapping(string:mixed)) cmd_fetchfields(string qid)
  {
    return get_query(qid)->fetch_fields();
  }

  static int cmd_numrows(string qid)
  {
    return get_query(qid)->num_rows();
  }

  static int cmd_numfields(string qid)
  {
    return get_query(qid)->num_fields();
  }

  static int cmd_eof(string qid)
  {
    return get_query(qid)->eof();
  }

  static void cmd_seek(array(string|int) args)
  {
    get_query(args[0])->seek(args[1]);
  }

  static string cmd_quote(string s)
  {
    return sqlobj->quote(s);
  }

  static void commandset_0()
  {
    commandset = ([ 'L': cmd_login ]);
  }

  static void commandset_1()
  {
    commandset_0();
    commandset |= ([ 'D': cmd_selectdb, 'E': cmd_error,
		     'C': cmd_create, 'X': cmd_drop, 'I': cmd_srvinfo,
		     'i': cmd_hostinfo, 's': cmd_shutdown, 'r': cmd_reload, 
		     'l': cmd_listdbs, 't': cmd_listtables, 'f': cmd_listflds,
		     'Q': cmd_bigquery, 'Z': cmd_zapquery,
		     'R': cmd_fetchrow, 'F': cmd_fetchfields,
		     'N': cmd_numrows, 'n': cmd_numfields, 'e': cmd_eof,
		     'S': cmd_seek, '@': cmd_query, 'q': cmd_quote]);
  }

  static void client_ident(string s)
  {
    int ver;
    if(s[..3]=="RSQL" && sscanf(s[4..], "%4c", ver) && ver==RSQL_VERSION) {
      write("SQL!");
      commandset_0();
      expect_command();
    } else {
      clntsock->set_blocking();
      clntsock->write("####");
      clntsock->close();
      destruct(clntsock);
      destruct(this_object());
    }
  }

  void create(object s, mapping(string:string) u)
  {
    users = u;
    clntsock = s;
    got = to_write = "";
    qbase = time();
    expect(8, client_ident);
    clntsock->set_nonblocking(read_callback, write_callback, close_callback);
  }
}

class Server
{
  static object servsock;
  static private mapping(string:string) users;

  static void accept_callback()
  {
    object s = servsock->accept();
    if(s)
      Connection(s, users);
  }

  void create(int|void port, string|void ip, mapping(string:string)|void usrs)
  {
    users = usrs;
    if(!port)
      port = RSQL_PORT;
    servsock = Stdio.Port();
    if(!servsock->bind(@({port, accept_callback})+(ip? ({ip}):({}))))
      throw(({"Failed to bind port "+port+".\n", backtrace()}));
  }
}


int main(int argc, array(string) argv)
{
  Server();
  return -17;
}
