// SQL blob based database
// Copyright © 2000,2001 Roxen IS.
//
// $Id: MySQL.pike,v 1.14 2001/05/17 12:49:03 js Exp $

// inherit Search.Database.Base;

// Creates the SQL tables we need.

//#define DEBUG
void create_tables()
{
  catch(db->query("drop table uri"));
  catch(db->query("drop table document"));
  catch(db->query("drop table occurance "));
  catch(db->query("drop table field"));
  catch(db->query("drop table word_hit"));
  
  db->query(
#"create table uri      (id          int unsigned primary key auto_increment not null,
                         uri_first   varchar(235) not null,
                         uri_rest    text not null,
                         uri_md5     char(16) not null,
                         UNIQUE(uri_md5),
                         INDEX index_uri_first (uri_first(235)))"
			 );

  db->query(
#"create table document (id            int unsigned primary key auto_increment not null,
                         uri_id        int unsigned not null,
                         language_code char(3),
                         INDEX index_language_code (language_code),
                         INDEX index_uri_id (uri_id))"
			 );
  
  db->query(
#"create table occurance (word_id      int not null,
                          document_id  int not null,
                          field_id     tinyint not null,
                          pos          smallint not null,
                          link_hash    tinyint not null,
 
                          key(word_id),
                          key(document_id))");

  db->query(
#"create table word_hit (word_id        int not null,
                          first_doc_id   int not null,
            	          hits           mediumblob not null,
                          unique(word_id,first_doc_id))");

  db->query(
#"create table field (id    tinyint unsigned primary key auto_increment not null,
                      name  varchar(127) not null,
                      INDEX index_name (name))");
}

// This is the database object that all queries will be made to.
object db;
int removed;

string host;

void create(string _host)
{
  db=Sql.sql(host=_host);
}

string _sprintf()
{
  return sprintf("Search.Database.MySQL(%O)", host);
}

mapping stats()
{
  mapping tmp=([]);
  tmp->words=(int)db->query("select count(*) as c from occurance")[0]->c;
  tmp->documents=(int)db->query("select count(*) as c from document")[0]->c;
  return tmp;
}

string to_md5(string url)
{
  object md5 = Crypto.md5();
  md5->update(url);
  return md5->digest();
}


// Useful information for crawlers
mapping(string:int) page_stat(string uri)
{
//   array res=db->query("SELECT last_changed, last_indexed, size "
// 		      "FROM document WHERE uri_md5='%s'", to_md5(uri));
//   if(!sizeof(res)) return 0;
//   return (mapping(string:int))res[0];
}


int hash_word(string word)
{
  return hash(word);
  string hashed=Crypto.md5()->update(word[..254]*16)->digest();
  int c;
  sscanf(hashed,"%4c",c);
  return c;
}

array(string) split_uri(string in)
{
  return ({ in[..218], in[219..] });
}


int find_or_create_uri_id(string uri)
{
  string s=sprintf("select id from uri where uri_md5='%s'", db->quote(to_md5(uri)));
  array a=db->query(s);
  if(sizeof(a))
    return (int)a[0]->id;

  s=sprintf("insert into uri (uri_first,uri_rest,uri_md5) "
	    "values ('%s','%s','%s')",
	    @map(split_uri(uri), db->quote), db->quote(to_md5(uri)));
  db->query(s);
  return db->master_sql->insert_id();
}

int find_or_create_document_id(string uri, void|string language_code)
{
  int uri_id=find_or_create_uri_id(uri);
  
  string s=sprintf("select id from document where "
		   "uri_id='%d'", uri_id);
  if(language_code)
    s+=sprintf(" and language_code='%s'",db->quote(language_code));

  array a = db->query(s);
  if(sizeof(a))
    return (int)a[0]->id;

  s=sprintf("insert into document (uri_id, language_code) "
	    "values ('%d',%s)",
	    uri_id,
	    language_code?("'"+language_code+"'"):"NULL");

  db->query(s);
  return db->master_sql->insert_id();
}

mapping field_cache = ([]);
int find_or_create_field_id(string field)
{
  if(field_cache[field])
    return field_cache[field];
  if(field=="body")
    return field_cache[field]=-1;
  string s=sprintf("select id from field where name='%s'",db->quote(field));
  array a=db->query(s);
  if(sizeof(a))
  {
    field_cache[field]=(int)a[0]->id;
    return (int)a[0]->id;
  }

  s=sprintf("insert into field (name) values ('%s')", db->quote(field));
  db->query(s);
  return db->master_sql->insert_id();
}

//! Inserts the words of a resource into the database
void insert_words(Standards.URI|string uri, void|string language,
		  string field, array(string) words, void|int link_hash)
{
  if(!sizeof(words))
    return;
  werror("field: %O  words: %s\n",field,words*", ");
  int doc_id=find_or_create_document_id((string)uri, language);
  int field_id;
  if(!field)
    field_id=63;
  else
    field_id=find_or_create_field_id(field);

  mapping word_ids=([]);
  int offset;
  
  string s="insert into occurance (word_id, document_id, pos, "
    "field_id, link_hash) values ";
    
  foreach(words, string word)
    s+=sprintf("(%d,%d,%d,%d,%d),",
	       hash_word(word), doc_id, offset++, field_id, link_hash);
  
  db->query(s[..sizeof(s)-2]);
}

void remove_document(string uri)
{
}

// Blob stuff
// -------------------------------------

string encode_hit(mapping hit)
{
  switch(hit->type)
  {
    case "-1":
      return sprintf("%2c", hit->imp<<14 | (hit->pos & 0x1fff));
      
    case "63":
      return sprintf("%2c", 3<<14 | 63<<8 | (hit->pos&0xff)<<4);

    default:
      return sprintf("%2c", 3<<14 | hit->type<<8 | min(hit->pos,0xff));
  }
}

mapping decode_hit(string hit)
{
  int imp,type,pos,hash,c;

  sscanf(hit,"%2c",c);
  imp=c>>14;

  if(imp==3)
  {
    pos = c & 0xff;
    type = (c >> 8) & 0x3f;
    if(type==63)
    {
      hash=pos>>4;
      pos=pos & 0xf;
    }
  }
  else
  {
    type=-1;
    pos=c & 0x1fff;
  }
  return (["type": type,
	   "imp": imp,
	   "pos": pos,
	   "hash": hash]);
}


string encode_hits_header(int doc_id, int num_hits)
{
  return sprintf("%4c", doc_id<<5 | (num_hits&0x1f));
}

array decode_hits_header(string header)
{
  int c;
  sscanf(header,"%4c",c);
  return ({ c >> 5, c & 0x1f });
}

string encode_packet(int|array doc_id, void|array(mapping) hits)
{
  // Ugly trick to splice the argument when it's an array
  if(arrayp(doc_id))
  {
    hits=doc_id[1];
    doc_id=doc_id[0];
  }
  return
    encode_hits_header(doc_id,sizeof(hits))+
    map(hits, encode_hit)*"";
}

array decode_packet(string packet)
{
  int doc_id, num_hits;
  array hits;

  [doc_id, num_hits]=decode_hits_header(packet);

  packet=packet[4..];
  hits=allocate(num_hits);

  for(int i=0; i<num_hits; i++)
  {
    hits[i]=decode_hit(packet[..1]);
    packet=packet[2..];
  }
  return ({ doc_id, num_hits, hits });
}

int decode_docid(string packet)
{
  int i;
  sscanf(packet,"%4c%*s",i);
  return i >> 5;
}

class StringBlob(string hits)
{
  int p=0;
  int p_delta;
  string peek_next_packet()
  {
    if(p>sizeof(hits)-2)
      return 0;
    
    int nhits=hits[p+3] & 0x1f;
    p_delta=4+nhits*2;
    return hits[p..p_delta-1];      
  }

  void increment()
  {
    p+=p_delta;
  }

  string get_rest()
  {
    return hits[p..];
  }
}

class ArrayBlob(array hits)
{
  int p=0;

  string peek_next_packet()
  {
    if(p>sizeof(hits)-1)
      return 0;
    return encode_packet(hits[p][0], hits[p][1]);
  }

  void increment()
  {
    p++;
  }

  string get_rest()
  {
    return map(hits[p..], encode_packet)*"";
  }
}

string preview_packet(string packet)
{
  array p=decode_packet(packet);
  string res="";
  res+=sprintf("  doc_id: %d, num_hits: %d\n",p[0],p[1]);
  res+=sprintf("    | imp | type | hash |   pos |\n");
  foreach(p[2], mapping hit)
    res+=sprintf("    | %3d | %4d | %4d | %5d |\n",hit->imp,hit->type,hit->hash,hit->pos);
  return res;
}


string merge_packet(string a, string b)
{
  // FIXME
  return b;
}


/*
  array(array) hits
  -----------------

  ({ ({ 4711, ({ ([ "imp": 2, "pos": 5 ]),
                 ([ "imp": 2, "pos": 45 ]) }) }),
     ({ 4713, ({ ([ "imp": 1, "pos": 5 ]),
                 ([ "imp": 3, "pos": 7 ]),
                 ([ "imp": 1, "pos": 9 ]) }) }) })
*/
private void update_blob(Sql.sql db, int word_id, int first,array(array) hits)
{
#ifdef DEBUG  
  werror("%O\n",hits);
  werror("Updating blob %d, first offset %d\n",word_id,first);
  werror("  sizeof(hits): %d\n", sizeof(hits));
#endif
  array arr=db->query("select hits from word_hit where first_doc_id=%d and "
		    "word_id=%d", first, word_id)->hits;
  if(!sizeof(arr))
    arr=({""});
#ifdef DEBUG  
  werror("  Size before: %d bytes\n",sizeof(arr[0]));
#endif
  StringBlob blob_a=StringBlob(arr[0]);
  ArrayBlob blob_b=ArrayBlob(hits);
 
  array(string) parts=({});

  // merge the ordered lists a and b into c
  string a,b;
  String.Buffer c=String.Buffer();
  a=blob_a->peek_next_packet();
  b=blob_b->peek_next_packet();
  while(a && b)
  {
    int docid_a=decode_docid(a), docid_b=decode_docid(b);
    if(docid_a < docid_b)
    {
#ifdef DEBUG  
      werror("- keeping doc_id: %d\n",docid_a);
      werror("  preview(b): %s",preview_packet(b));
#endif
      c->add(a);
      blob_a->increment();
    }
    else if(docid_b < docid_a)
    {
#ifdef DEBUG  
      werror("- adding doc_id: %d\n",docid_a);
      werror("  preview(b): %s",preview_packet(b));
#endif
      c->add(b);
      blob_b->increment();
    }
    else
    {
#ifdef DEBUG  
      werror("- merging doc_id: %d\n",docid_b);
      werror("  preview(b): %s",preview_packet(b));
#endif
      c->add(merge_packet(a,b));
      blob_a->increment();
      blob_b->increment();
    }
    a=blob_a->peek_next_packet();
    b=blob_b->peek_next_packet();

  }

  // a ran out first
  if(a==0)
  {
#ifdef DEBUG  
    werror("- storing rest of b\n");
    while(b=blob_b->peek_next_packet())
    {
      werror("  preview(b): %s",preview_packet(b));
      c->add(b);
      blob_b->increment();
    }
#else
    c->add(blob_b->get_rest());
#endif
   
  }

  // b ran out first
  if(b==0)
  {
#ifdef DEBUG  
    werror("- storing rest of a\n");
    while(a=blob_a->peek_next_packet())
    {
      werror("  preview(a): %s",preview_packet(a));
      c->add(a);
      blob_a->increment();
    }
#else
    c->add(blob_a->get_rest());
#endif
  }
     
  string res=c->get();
#ifdef DEBUG  
  werror("  Size after: %d bytes\n",sizeof(res));
#endif

  db->query("replace into word_hit (word_id,first_doc_id,hits) "
 	    "values(%d,%d,%s)", word_id, first, res);
  
  //   if(sizeof(c) > BLOB_SPLIT_SIZE)
  //     split_blob(db, first, last);
}

void add_hits(Sql.sql db, int word_id, array(mapping(string:string)) hits)
{
  mapping(int:array) res=([]);
  int last_doc_id=-1;

  array a=db->query("select first_doc_id from word_hit where word_id=%d "
		    "order by first_doc_id", word_id)->first_doc_id;
  a=(array(int))a;
  if(!sizeof(a))
    a=({ 0 });
  a+=({ -1 });
  int a_ptr=0;
  
  foreach(hits, mapping hit)  
  {
    hit=(["link_hash":1, "pos":1, "type": 1, "doc_id": 1]) & hit;
    hit=(mapping(string:int))hit;
    if(hit->doc_id==last_doc_id)
      res[a[a_ptr]][-1][-1]+=({ hit });
    else
    {
      while(hit->doc_id>=a[a_ptr+1] && a[a_ptr+1]!=-1)
      {
	a_ptr++;
	res[a[a_ptr]]=({});
      }
      
      res[a[a_ptr]]+=({ ({ hit->doc_id, ({ hit }) }) });
    }
    last_doc_id=hit->doc_id;
  }
  foreach(indices(res), int first_doc_id)
    if(sizeof(res[first_doc_id]))
      update_blob(db, word_id, first_doc_id, res[first_doc_id]);
}

int do_da_blobs()
{
  foreach(db->query("select distinct word_id from occurance")->word_id,
	  string word_id)
  {
    //     if(word_id!=(string)hash("roxen"))
    //       continue;
    add_hits(db,
	     (int)word_id,
	     db->query("select document_id as doc_id,field_id as type,pos,link_hash "
		       "from occurance where word_id=%d "
		       "order by doc_id,field_id,pos,link_hash",(int)word_id));
    db->query("delete from occurance where word_id=%d",
    	      (int)word_id);
  }
}


void zap()
{
  db->query("delete from word_hit");
  db->query("delete from occurance");
  db->query("delete from uri");
  db->query("delete from document");
  db->query("delete from field");
}
// Query functions
// -------------------------------------

