//  $Id: Session.pike,v 1.5 1999/07/04 08:18:39 hubbe Exp $
//! module Protocols
//! submodule LysKOM
//! class Session

import ".";

Connection con;

//! variable user
//!	This variable contains the 
//!	<link to=Protocols.LysKOM.Session.Person>person</link>
//!	that are logged in.

object user; // logged in as this Person 

string server;

int protlevel; // level <10 protocol

mapping(int:object) _text=([]);
mapping(int:object) _person=([]);
mapping(int:object) _conference=([]);

//! method void create(string server)
//! method void create(string server,mapping options)
//!	Initializes the session object, and opens
//!	a connection to that server.
//!	
//!	options is a mapping of options,
//!	<data_description type=mapping>
//!	<elem name=login type="int|string">login as this person number<br>(get number from name)</elem>
//!	<elem name=password type=string>send this login password</elem>
//!	<elem name=invisible type="int(0..1)">if set, login invisible</elem>
//!	<elem>advanced</elem>
//!	<elem name=port type=int(0..65535)>server port (default is 4894)</elem>
//!	<elem name=whoami type=string>present as this user<br>(default is from uid/getpwent and hostname)</elem>
//!	</data_description>
//!
//! see also: Connection

void create(object|string _server,void|mapping options)
{
   if (objectp(_server)) // clone
   {
      con        = _server->con;
      server     = _server->server;
      protlevel  = _server->protlevel;
      user       = _server->user;
      _person    = _server->_person;
      _conference= _server->_conference;
      _text      = _server->_text;
      return;
   }
   server=_server;
   con=Connection(_server,options);
   user=(options && options->login)?person(options->login):0;
   protlevel=con->protocol_level;
   werror("protocol_level is %d\n",protlevel);
}


class MiscInfo
{
   class Recpt
   {
      object conf;
      int local_no;
      object received_at;
      object sent_by;
      object sent_at;
   }
   array(Recpt) recpt=({});
   array(Recpt) ccrecpt=({});
   array(Recpt) bccrecpt=({});

   array(object) comm_to=({});
   array(object) comm_in=({});
   array(object) foot_to=({});
   array(object) foot_in=({});

   void create(void|mapping|array(int) a)
   {
      int i;
      object r;
      if (mappingp(a))
      {
	 if (!a->recpt) a->recpt=a->recipient;
	 foreach (arrayp(a->recpt)?a->recpt:
		  a->recpt?({a->recpt}):({}),object o)
	    r=Recpt(),r->conf=o,recpt+=({r});
	 foreach (arrayp(a->cc)?a->cc:
		  a->cc?({a->cc}):({}),object o)
	    r=Recpt(),r->conf=o,ccrecpt+=({r});
	 foreach (arrayp(a->bcc)?a->bcc:
		  a->bcc?({a->bcc}):({}),object o)
	    r=Recpt(),r->conf=o,bccrecpt+=({r});

	 if (a->comm_to)
	    comm_to=(arrayp(a->comm_to)?a->comm_to:({a->comm_to}));
	 if (a->foot_to)
	    foot_to=(arrayp(a->foot_to)?a->foot_to:({a->foot_to}));
	 /*
	 if (a->comm_in)
	    comm_in=(arrayp(a->comm_in)?a->comm_in:({a->comm_in}));
	 if (a->foot_in)
	    foot_in=(arrayp(a->foot_in)?a->foot_in:({a->foot_in}));
	 */
      }
      else if (arrayp(a)) for (i=0; i<sizeof(a);)
      {
	 switch (a[i++])
	 {
	    case  0: recpt+=({r=Recpt()}); r->conf=conference(a[i++]); break;
	    case  1: ccrecpt+=({r=Recpt()}); 
	             r->conf=conference(a[i++]); break;
	    case 15: bccrecpt+=({r=Recpt()}); 
	             r->conf=conference(a[i++]); break;

	    case  2: comm_to+=({text(a[i++])}); break;
	    case  3: comm_in+=({text(a[i++])}); break;
	    case  4: foot_to+=({text(a[i++])}); break;
	    case  5: foot_in+=({text(a[i++])}); break;
	       
	    case  6: r->local_no=a[i++]; break;
	    case  7: r->received_at=ProtocolTypes.LysKOMTime(@a[i..i+8]); 
	             i+=9; break;
	    case  8: r->sent_by=person(a[i++]); break;
	    case  9: r->sent_at=ProtocolTypes.LysKOMTime(@a[i..i+8]); 
	             i+=9; break;
	    default:
	       error("unexpected selection in misc_info: %d\n",a[i-1]);
	 }
      }
   }

   array(string) encode()
   {
      array res=({});
      foreach (recpt,object r)
	 res+=({"0 "+r->conf->no});
      foreach (ccrecpt,object r)
	 res+=({"1 "+r->conf->no});
      foreach (bccrecpt,object r)
	 res+=({"15 "+r->conf->no});

      foreach (comm_to,object t)
	 res+=({"2 "+t->no});
      foreach (foot_to,object t)
	 res+=({"4 "+t->no});

      /* *** this generates illegal-misc ***
      foreach (comm_in,object t)
	 res+=({"3 "+t->no});
      foreach (foot_in,object t)
	 res+=({"5 "+t->no});
      */
      return res;
   }
}

#define FETCHERC(WHAT,TYPE,VAR,CALL,ARGS,CONV)				\
   TYPE VAR;						\
   private object fetch_##WHAT;						\
   private array(function) fetch_##WHAT##_callbacks=({});		\
									\
   void prefetch_##WHAT##(void|function callback)			\
   {									\
      if (VAR) 								\
      {									\
	 if (callback) callback(this_object());				\
	 return;							\
      }									\
      if (callback) fetch_##WHAT##_callbacks+=({callback});		\
      if (!fetch_##WHAT)						\
	 fetch_##WHAT=							\
	    con->async_cb_##CALL(lambda(mixed res)			\
				 {					\
				    if (objectp(res) && res->iserror)	\
				       err=res;				\
				    else				\
				       VAR=CONV;			\
				    fetch_##WHAT=0;			\
				    array m=fetch_##WHAT##_callbacks;	\
				    fetch_##WHAT##_callbacks=0;		\
				    m(this_object());			\
				 },ARGS);				\
   }									\
									\
   inline void need_##WHAT()						\
   {									\
      if (VAR) return;							\
      if (!fetch_##WHAT) prefetch_##WHAT();				\
      if (fetch_##WHAT) fetch_##WHAT();					\
   }
#define FETCHER(WHAT,TYPE,VAR,CALL,ARGS)				\
	FETCHERC(WHAT,TYPE,VAR,CALL,ARGS,res)

#define FETCHER2(WHAT,TYPE1,VAR1,TYPE2,VAR2,CALL1,CALL2,ARGS)		\
   TYPE1 VAR1;						                \
   TYPE2 VAR2;						                \
   private object fetch_##WHAT;						\
   private array(function) fetch_##WHAT##_callbacks=({});		\
									\
   private void _got_##WHAT##1(mixed res)				\
   {									\
      if (objectp(res) && res->iserror)					\
	 err=res;							\
      else 								\
	 VAR1=res;							\
      array m=fetch_##WHAT##_callbacks;					\
      fetch_##WHAT##_callbacks=({});					\
      m(this_object());							\
   }									\
									\
   private void _got_##WHAT##2(mixed res)				\
   {									\
      if (objectp(res) && res->iserror) err=res; else VAR2=res;		\
      array m=fetch_##WHAT##_callbacks;					\
      fetch_##WHAT##_callbacks=({});					\
      m(this_object());							\
   }									\
									\
   object prefetch_##WHAT##(void|function callback)			\
   {									\
      if (VAR1 || VAR2)                                                 \
      {									\
	 if (callback) callback(this_object());				\
	 return this_object();						\
      }									\
      if (callback) fetch_##WHAT##_callbacks+=({callback});		\
      if (!fetch_##WHAT)						\
	 if (protlevel<10)						\
	    fetch_##WHAT=con->async_cb_##CALL2(_got_##WHAT##2,ARGS);	\
	 else								\
	    fetch_##WHAT=con->async_cb_##CALL1(_got_##WHAT##1,ARGS);	\
      return this_object(); \
   }									\
									\
   inline void need_##WHAT()						\
   {									\
      if (VAR1 || VAR2) return;						\
      if (!fetch_##WHAT) prefetch_##WHAT();				\
      if (fetch_##WHAT) fetch_##WHAT();					\
      if (fetch_##WHAT) fetch_##WHAT();					\
   }

#define FETCHERC2b(WHAT,TYPE,VAR,CALL,ARGS,CONV)		\
   TYPE VAR;						                \
   private object fetch_##WHAT;						\
   private array(function) fetch_##WHAT##_callbacks=({});		\
									\
   private void _got_##WHAT##1(mixed res)				\
   {									\
      if (objectp(res) && res->iserror)					\
      {									\
	 err=res;							\
      }									\
      else 								\
	 VAR=CONV;							\
      array m=fetch_##WHAT##_callbacks;					\
      fetch_##WHAT##_callbacks=({});					\
      m(this_object());							\
   }									\
									\
   private void _got_##WHAT##2(mixed res)				\
   {									\
      if (objectp(res) && res->iserror) err=res; \
   	else VAR=CONV;		\
      array m=fetch_##WHAT##_callbacks;					\
      fetch_##WHAT##_callbacks=({});					\
      m(this_object());							\
   }									\
									\
   object prefetch_##WHAT##(void|function callback)			\
   {									\
      if (VAR)                                                 \
      {									\
	 if (callback) callback(this_object());				\
	 return this_object();						\
      }									\
      if (callback) fetch_##WHAT##_callbacks+=({callback});		\
      if (!fetch_##WHAT)						\
	 if (protlevel<10)						\
	    fetch_##WHAT=con->async_cb_##CALL##_old(_got_##WHAT##2,ARGS); \
	 else								\
	    fetch_##WHAT=con->async_cb_##CALL(_got_##WHAT##1,ARGS);	\
      return this_object(); \
   }									\
									\
   inline void need_##WHAT()						\
   {									\
      if (VAR) return;						\
      if (!fetch_##WHAT) prefetch_##WHAT();				\
      if (fetch_##WHAT) fetch_##WHAT();					\
      if (fetch_##WHAT) fetch_##WHAT();					\
   }

class Text
{
   int no;

   MiscInfo _misc;

   object err;

   object _author;

   void create(int _no)
   {
      no=_no;
   }

   FETCHERC(text,array(string),_text,get_text,@({no,0,0x7fffffff}),
	    array_sscanf(res,"%s\n%s"))
   FETCHERC2b(stat,object,_stat,get_text_stat,@({no}),
	      (_misc=MiscInfo(res->misc_info),res))

   mixed `[](string what)
   {
      switch (what)
      {
	 case "create":
	    return create;
	 case "prefetch_text":
	    return prefetch_text;
	 case "prefetch_stat":
	    return prefetch_stat;

	 case "no":
	    return no;
	 case "error":
	    return err;

	 case "text": 
	    need_text();
	    return _text[1];

	 case "subject": 
	    need_text();
	    return _text[0];

	 case "author":
	    need_stat();
	    return _author 
	       || (_author=person(_stat->author));

	 case "misc":
	    need_stat();
	    return _misc;

	 case "lines":
	    need_text();
	    return String.count(_text[1],"\n")+1;

	 case "characters":
	    need_text();
	    return strlen(_text[1]);
      }
   }

   mixed `->(string what) { return `[](what); }
}


object text(int no)
{
   return _text[no] || (_text[no]=Text(no));
}

class Membership
{
   int              person;
   int 		    newtype;
   object           last_time_read;             
   int(0..255)      priority;                   
   int              last_text_read;             
   array(int)       read_texts;                 
   int(0..65535)    added_by;   // new
   object           added_at;   // new
   multiset(string) type;       // new
   int              position;        // new

   object conf;

   void create(object mb,int pers)
   {
      person=pers;

      last_time_read=mb->last_time_read;
      read_texts=mb->read_texts;
      last_text_read=mb->last_text_read;
      priority=mb->priority;
      conf=conference(mb->conf_no);
	 
      if (mb->type)
      {
	 added_at=mb->added_at;
	 position=mb->position;
	 added_by=mb->added_by;
	 type=mb->type;
	 newtype=1;
      }
   }

   int number_unread()
   {
      return (conf->no_of_texts+conf->first_local_no-1)
	 -last_text_read -sizeof(read_texts);
   }
   
   mixed `[](string what)
   {
      switch (what)
      {
	 case "last_time_read":
	 case "read_texts":
	 case "last_text_read":
	 case "priority":
	 case "conf":
	 case "added_at":
	 case "position":
	 case "type":
	 case "number_unread":
	    return ::`[](what);
      }
   }
}


class Person
{
   int no;
   
   object conf;

   object err;

   FETCHER(stat,ProtocolTypes.Person,_person,get_person_stat,no);
   FETCHERC(unread,array(object),_unread,get_unread_confs,no,
	    Array.map(res,conference));
   FETCHERC2b(membership,array(object),_membership,get_membership,
	      @({no,0,65535,1}),
	      Array.map(res,Membership,no));

   void create(int _no)
   {
      no=_no;
      conf=conference(no);
   }

   mixed `[](string what)
   {
      switch (what)
      {
	 case "create":
	    return create;
	 case "prefetch_stat":
	    return prefetch_stat;
	 case "prefetch_conf":
	    return conf->prefetch_stat;
	 case "prefetch_membership":
	    return prefetch_membership;

	 case "no":
	    return no;
	 case "error":
	    return err;
	 case "user_area":
	    need_stat();
	    return text(_person->user_area);
         case "username":
	 case "privileges":
	 case "flags":
	 case "last_login":
	 case "total_time_present":
	 case "sessions":
	 case "created_lines":
	 case "created_bytes":
	 case "read_texts":
	 case "no_of_text_fetches":
	 case "created_persons":
	 case "created_confs":
	 case "first_created_local_no":
	 case "no_of_created_texts":
	 case "no_of_marks":
	 case "no_of_confs":
	    need_stat();
	    return _person[what];
	 case "unread":
	    need_unread();
	    return _unread;
	 case "membership":
	    need_membership();
	    return _membership;
      }

      return conf[what];
   }

   mixed `->(string what) { return `[](what); }
}

object person(int no)
{
   return _person[no] || (_person[no]=Person(no));
}

class Conference
{
   int no;

   private object err;

   FETCHER2(stat,ProtocolTypes.Conference,_conf,
	    ProtocolTypes.ConferenceOld,_confold,
	    get_conf_stat,get_conf_stat_old,no)

   void create(int _no)
   {
      no=_no;
   }

   mixed `[](string what)
   {
      switch (what)
      {
	 case "create":
	    return create;
	 case "prefetch_stat":
	    return prefetch_stat;

	 case "no":
	    return no;
	 case "error":
	    return err;

	 case "presentation":
	 case "msg_of_day":
	    need_stat();
	    return text( (_conf||_confold)[what] );
	 case "supervisor":
	 case "permitted_submitters":
	 case "super_conf":
	    need_stat();
	    return conference( (_conf||_confold)[what] );
	 case "creator":
	    need_stat();
	    return person( (_conf||_confold)[what] );
	 case "name":
	 case "type":
	 case "creation_time":
	 case "last_written":
	 case "nice":
	 case "no_of_members":
	 case "first_local_no":
	 case "no_of_texts":
	    need_stat();
	    return (_conf||_confold)[what];
      }
   }

   mixed `->(string what) { return `[](what); }
}

object conference(int no)
{
   return _conference[no] || (_conference[no]=Conference(no));
}

//!
//! method array(object) try_complete_person(string orig)
//!	Runs a LysKOM completion on the given string,
//!	returning an array of confzinfos of the match.

array(ProtocolTypes.ConfZInfo) try_complete_person(string orig)
{
   return con->lookup_z_name(orig,1,0);
}

//! method login(int user_no,string password)
//! method login(int user_no,string password,int invisible)
//!	Performs a login. Returns 1 on success or throws a lyskom error.

int(1..1) login(int user_no,string password,
		void|int invisible)
{
   con->login(user_no,password,invisible);
   user=person(user_no);
   return 1;
}

//! method logout()
//!	Logouts from the server.

int(1..1) logout()
{
   if (con) 
      con->logout();
   return 1;
}

//! method object create_text(string subject,string body,mapping options)
//! method object create_text(string subject,string body,mapping options,function callback,mixed ...extra)
//! 	Creates a new text. 
//!
//! 	if "callback" are given, this function will be called when the text 
//! 	is created, with the text as first argument. 
//!	Otherwise, the new text is returned.
//!
//!	options is a mapping that may contain:
//!	<data_description type=mapping>
//!	<elem name=recpt type="Conference|array(Conference)">recipient conferences</elem>
//!	<elem name=cc type="Conference|array(Conference)">cc-recipient conferences</elem>
//!	<elem name=bcc type="Conference|array(Conference)">bcc-recipient conferences *</elem>
//!	<elem></elem>
//!	<elem name=comm_to type="Text|array(Text)">what text(s) is commented</elem>
//!	<elem name=foot_to type="Text|array(Text)">what text(s) is footnoted</elem>
//!	<elem></elem>
//!	<elem name=anonymous type="int(0..1)">send text anonymously</elem>
//!	</data_description>
//!		
//! note:
//!	The above marked with a '*' is only available on a protocol 10
//!	server. A LysKOM error will be thrown if the call fails.
//!
//! see also: Conference.create_text, Text.comment, Text.footnote

object|void create_text(string subject,string body,
			void|mapping options,
			void|function callback,
			void|mixed ...extra)
{
   string text=replace(subject,"\n"," ")+"\n"+body;
   MiscInfo misc=MiscInfo(options);

   if (!options) options=([]);
   
   return _create_text(text,misc,
		       options->aux_info,
		       options->anonymous,
		       callback,@extra);
}

object|void _create_text(string textstring,
			 MiscInfo misc,
			 void|object aux_info,
			 int anonymous,
			 void|function callback,
			 void|mixed ...extra)
{
   int|object res;
   string call;
   if (anonymous) 
      call="create_anonymous_text";
   else 
      call="create_text";

   if (aux_info)
      error("unimplemented\n");

   if (protlevel<10) call+="_old";

   if (callback)
   {
      con["async_cb_"+call]
	 (lambda(int|object res)
	  {
	     if (objectp(res)) return res;
	     callback(text(res),@extra);
	  },textstring,misc->encode());
      return;
   }

   res=con[call](textstring,misc->encode());

   if (objectp(res)) return res;
   return text(res);
}

