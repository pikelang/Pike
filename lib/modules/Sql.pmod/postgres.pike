/*
 * This is part of the Postgres module for Pike.
 * (C) 1997 Francesco Chemolli <kinkie@comedia.it>
 *
 */

#define ERROR(X) throw (({X,backtrace()}))

inherit Postgres.postgres: mo;
private static mixed callout;

void create(void|string host, void|string database, void|string user,
		void|string pass) {
	string real_host=host, real_db=database;
	int port=0;
	if (stringp(host)&&(search(host,":")>=0))
		if (sscanf(host,"%s:%d",real_host,port)!=2)
			ERROR("Error in parsing the hostname!\n");
	
	//postgres docs say that PGHOST, PGPORT, PGDATABASE are used to
	//determine defaults if not otherwise availible. Since on my system
	//this doesn't work, I'll do it myself
	if (!real_host && getenv("PGHOST"))
		real_host=getenv("PGHOST");
	if (!port && getenv("PGPORT"))
		port=(int)getenv("PGPORT");
	if (!real_db && getenv("PGDATABASE"))
		real_db=getenv("PGDATABASE");
	if (!real_host)
		real_host="localhost";
	if (!real_db)
		real_db="template1"; 
	//this allows to connect, since template1 is shipped with Postgres...
	if (port)
		mo::create(real_host,real_db,port);
	else
		mo::create(real_host,real_db); //should use the default port.
}

static void poll (int delay)
{
	callout=call_out(poll,delay,delay);
	big_query("");
}

void set_notify_callback(int|function f, int|float|void poll_delay) {
	if (callout) {
		remove_call_out(callout);
		callout=0;
	}
	if (intp(f)) {
		mo::_set_notify_callback(0);
		return;
	}
	mo::_set_notify_callback(f);
	if(poll_delay>0) 
		poll(poll_delay);
}

void create_db(string db) {
	big_query(sprintf("CREATE DATABASE %s",db));
}

void drop_db(string db) {
	big_query(sprintf("DROP database %s",db));
}

string server_info () {
	return "Postgres/unknown";
}

array(string) list_dbs (void|string wild) {
	array name,ret=({});
	object res=big_query("Select datname from pg_database");
	while (name=res->fetch_row()) {
		if (wild && !glob(wild,name))
			continue;
		ret += ({name[0]});
	}
	return sort(ret);
}

array(string) list_tables (void|string wild) {
	array name,ret=({});
	object res;
	res=big_query(
			"select relname,relkind FROM pg_class WHERE "
			"(relkind = 'r' or relkind = 'i')");
	while (name=res->fetch_row()) {
		if (wild && !glob(wild,name[0])||glob("pg_*",name[0]))
			continue;
		 ret += ({name[0]});
	}
	return sort(ret);
}

int mkbool(string s) {
	if (s=="f")
		return 0;
	return 1;
}

mapping(string:array(mixed)) list_fields (string table) {
	array row;
	mapping ret=([]);
	object res=big_query(
			"SELECT a.attnum, a.attname, t.typname, a.attlen, c.relowner, "
			"c.relexpires, c.relisshared, c.relhasrules, t.typdefault "
			"FROM pg_class c, pg_attribute a, pg_type t "
			"WHERE c.relname = '"+table+"' AND a.attnum > 0 "
			"AND a.attrelid = c.oid AND a.atttypid = t.oid ORDER BY attnum");
	while (row=res->fetch_row()) {
		mapping m;
		string name=row[1];
		row=row[2..];
		row[4]=mkbool(row[4]);
		row[5]=mkbool(row[5]);
		m=mkmapping(({"type","length","owner","expires","is_shared"
				,"has_rules","default"}),row);
		ret[name]=m;
	}
	return ret;
}
