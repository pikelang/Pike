
#pike __REAL_VERSION__

// Cannot dump this since the #if constant(...) check below may depend
// on the presence of system libs at runtime.
constant dont_dump_program = 1;

#if constant(SQLite.SQLite)
inherit SQLite.SQLite;

void create(string a, void|string b, void|mixed c, void|mixed d,
	    void|mapping options) {
  if(b) a += "/"+b;
  ::create(a);
}

array list_fields(string n, string|void wild)
{
  string qry = "";

  qry = "PRAGMA table_info(" + n + ")";

  array r = query(qry);

  // now, we weed out the ones that don't match wild, if provided
  if(wild)
  {
    r = filter(r, lambda(mapping row) 
              { return (search(row->name, wild) !=-1); }
          );
  }

  array fields = ({});

  foreach(r;; mapping f)
  {
    mapping fld = ([]);

    fld->name = f->name;
    fld->table = n;
  
    string t, l;   
 
    if(!sscanf(f->type, "%s(%s)", t, l))
      t = f->type;

    fld->length = (int)l;

    switch(t)
    {
      case "char":
       t = "string";
       break;

      case "int":
       t = "integer";
       break;
    }

    fld->type = t;

    fld->flags = (<>);

    if((int)f->notnull)
      fld->flags->not_null = 1;

    if((int)f->pk)
    {
      // primary key implies not null.
      fld->flags->not_null = 1;
      fld->flags->primary_key = 1;
    }

    fld->default = f->dflt_value;

    fields += ({fld});
  }

  return fields;
}

array list_tables(string|void n)
{
  string qry = "";

  if(n)
    qry = "SELECT name FROM SQLITE_MASTER WHERE name like '" + n + "%' and TYPE='table'";  
  else
    qry = "SELECT name FROM SQLITE_MASTER where TYPE='table'";  

  array r = query(qry);
  array out = ({});

  foreach(r;;mapping row)
  {
    if(row->name)
      out += ({row->name});
    else
      out += ({row["sqlite_master.name"] });
  }
  return out;
}

#else
constant this_program_does_not_exist=1;
#endif

