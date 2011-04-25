#pike __REAL_VERSION__

//  $Id$

import ".";

Connection con;

//! This variable contains the
//! @[Protocols.LysKOM.Session.Person]
//! that is logged in.
object user; // logged in as this Person

string server;

int protlevel; // protocol level

mapping(int:object) _text=([]);
mapping(int:object) _person=([]);
mapping(int:object) _conference=([]);

//! @decl void create(string server)
//! @decl void create(string server, mapping options)
//!	Initializes the session object, and opens
//!	a connection to that server.
//!
//!	@[options] is a mapping of options:
//!	@mapping
//!	  @member int|string "login"
//!	    login as this person number (get number from name).
//!	  @member string "create"
//!	    create a new person and login with it.
//!	  @member string "password"
//!	    send this login password.
//!	  @member int(0..1) "invisible"
//!	    if set, login invisible.
//!	  @member int(0..65535) "port"
//!	    server port (default is 4894).
//!	  @member string "whoami"
//!	    present as this user (default is from uid/getpwent and hostname).
//!	@endmapping
//!
//! @seealso
//!   @[Connection]
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

   /* setup async stuff */
   con->con->add_async_callback("async-new-name",async_new_name);
   con->con->add_async_callback("async-leave-conf",async_leave_conf);

   if (protlevel>=10)
   {
      con->con->add_async_callback("async-deleted-text",async_deleted_text);
      con->con->add_async_callback("async-new-text",async_new_text);
      con->con->add_async_callback("async-new-recipient",async_new_recipient);
      con->con->add_async_callback("async-sub-recipient",async_sub_recipient);
      con->con->add_async_callback("async-new-membership",
				   async_new_membership);
   }
   else
   {
      con->con->add_async_callback("async-new-text-old",async_new_text_old);
   }

   con->accept_async(con->con->active_asyncs());

   /* create person if wanted */

   if (options->create)
      create_person(options->create,options->password);
}

/** async callbacks from raw session **/

void async_new_name()
{
}

void async_leave_conf()
{
}

void async_deleted_text()
{
}

void async_new_text()
{
}

void async_new_recipient(int textno,int confno,array misc)
{
   if (_text[textno]) _text[textno]->update_misc(MiscInfo(misc));
   // if (_conference[confno]) _conference[confno]->new_texts();
}

void async_sub_recipient(int textno,int confno,array misc)
{
   if (_text[textno]) _text[textno]->update_misc(MiscInfo(misc));
   // if (_conference[confno]) _conference[confno]->sub_text(textno);
}

void async_new_membership()
{
}

void async_new_text_old()
{
}

/** classes *********************/

class MiscInfo
{
  string _sprintf(int t)
  {
    if(t!='O') return 0;
    array(string) to = ({});
    if(sizeof(recpt))
      to += ({ "To: " + String.implode_nicely(recpt->conf->no) });
    if(sizeof(ccrecpt))
      to += ({ "Cc: " + String.implode_nicely(ccrecpt->conf->no) });
    if(sizeof(bccrecpt))
      to += ({ "Bcc: " + String.implode_nicely(bccrecpt->conf->no) });

    return sprintf("%O(%s)", this_program,
		   sizeof(to) ? to * "; " : "No recipients");
  }

  class Recpt
  {
    object conf;
    int local_no;
    object received_at;
    object sent_by;
    object sent_at;
    string _sprintf(int t)
    {
      return t=='O' && sprintf("%O(conf %d: text %d)", this_program,
			       conf && conf->no, local_no);
    }
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
   TYPE VAR;						                \
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
   }                                                                    \
									\
   inline void waitfor_##WHAT()                                         \
   {                                                                    \
      mixed res;                                                        \
      if (VAR) return;                                                  \
      res=con->##CALL(ARGS);                                            \
      if(objectp(res) && res->iserror) err=res; else VAR=CONV;          \
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
      return this_object();                                             \
   }									\
									\
   inline void need_##WHAT()						\
   {									\
      if (VAR1 || VAR2) return;						\
      if (!fetch_##WHAT) prefetch_##WHAT();				\
      if (fetch_##WHAT) fetch_##WHAT();					\
      if (fetch_##WHAT) fetch_##WHAT();					\
   }                                                                    \
									\
   inline void waitfor_##WHAT()                                         \
   {                                                                    \
     mixed res;                                                         \
     if(VAR1||VAR2) return;                                             \
     if(protlevel<10)                                                   \
     {                                                                  \
       res=con->##CALL2(ARGS);                                          \
       if(objectp(res) && res->iserror) err=res; else VAR2=res;         \
     }                                                                  \
     else                                                               \
     {                                                                  \
       res=con->##CALL1(ARGS);                                          \
       if(objectp(res) && res->iserror) err=res; else VAR1=res;         \
     }                                                                  \
   }

#define FETCHERC2b(WHAT,TYPE,VAR,CALL,ARGS,CONV)		        \
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
      if (objectp(res) && res->iserror) err=res;                        \
   	else VAR=CONV;		                                        \
      array m=fetch_##WHAT##_callbacks;					\
      fetch_##WHAT##_callbacks=({});					\
      m(this_object());							\
   }									\
									\
   object prefetch_##WHAT##(void|function callback)			\
   {									\
      if (VAR)                                                          \
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
      return this_object();                                             \
   }									\
									\
   inline void need_##WHAT()						\
   {									\
      if (VAR) return;						        \
      if (!fetch_##WHAT) prefetch_##WHAT();				\
      if (fetch_##WHAT) fetch_##WHAT();					\
      if (fetch_##WHAT) fetch_##WHAT();					\
   }                                                                    \
                                                                        \
   inline void waitfor_##WHAT()                                         \
   {                                                                    \
     mixed res;                                                         \
     if(VAR) return;                                                    \
     if(protlevel<10)                                                   \
       res=con->##CALL_old(ARGS);                                       \
     else                                                               \
       res=con->##CALL(ARGS);                                           \
     if(objectp(res) && res->iserror) err=res; else VAR=CONV;           \
   }

constant itemname_to_tag = ([ "content-type":		1,
			      "fast-reply":		2,
			      "cross-reference":	3,
			      "no-comments":		4,
			      "personal-comment":	5,
			      "request-confirmation":	6,
			      "read-confirm":		7,
			      "redirect":		8,
			      "x-face":			9,
			      "alternate-name":		10,
			      "pgp-signature":		11,
			      "pgp-public-key":		12,
			      "e-mail-address":		13,
			      "faq-text":		14,
			      "creating-software":	15,
			      "mx-author":		16,
			      "mx-from":		17,
			      "mx-reply-to":		18,
			      "mx-to":			19,
			      "mx-cc":			20,
			      "mx-date":		21,
			      "mx-message-id":		22,
			      "mx-in-reply-to":		23,
			      "mx-misc":		24,
			      "mx-allow-filter":	25,
			      "mx-reject-forward":	26,
			      "notify-comments":	27,
			      "faq-for-conf":		28,
			      "recommended-conf":	29,
			      "mx-mime-belongs-to":	10100,
			      "mx-mime-part-in":	10101,
			      "mx-mime-misc":		10102,
			      "mx-envelope-sender":	10103,
			      "mx-mime-file-name":	10104, ]);

//! @fixme
//!   Undocumented
class AuxItemInput
{
  inherit ProtocolTypes.AuxItemInput;

  void create(string|int tag_type, multiset _flags,
	      int _inherit_limit, string _data)
  {
    if(intp(tag_type))
      tag = tag_type;
    else
      tag = itemname_to_tag[replace(tag_type, "_", "-")];
    flags = _flags;
    inherit_limit = _inherit_limit;
    data = _data;
  }

  string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%s)", this_program,
			     search(itemname_to_tag, tag));
  }
}

//! @fixme
//!   Undocumented
class AuxItems
{
  string _sprintf(int t)
  {
    if(t!='O') return 0;
    array desc = ({});
    foreach((array)tag_to_items, [int tag, mixed item])
      desc += ({ search(itemname_to_tag, tag) });
    return sprintf("%O(%s)", this_program,
		   sizeof(desc) ? String.implode_nicely(desc)
		   : "none present");
  }

  mapping(int:array(ProtocolTypes.AuxItem)) tag_to_items = ([]);

  array(ProtocolTypes.AuxItem) aux_items;

  void create(array(ProtocolTypes.AuxItem) _aux_items)
  {
    aux_items=_aux_items;
    foreach(aux_items, ProtocolTypes.AuxItem item)
    {
      if(tag_to_items[item->tag])
	tag_to_items[item->tag] += ({ item });
      else
	tag_to_items[item->tag] = ({ item });
    }
  }

  mixed `[](string what)
  {
    switch (what)
    {
    case "create":
      return create;

    default:
      return tag_to_items[itemname_to_tag[replace(what,"_","-")]] || ({ });
    }
  }

  array(string) _indices()
  {
    return ({ "create" }) + indices(itemname_to_tag);
  }

  mixed `->(string what) { return `[](what); }
}

//! All variables in this class is read only.
//! @fixme
//!   Undocumented
class Text
{

  //! The text number, as spicified to @[create].
  int no;

  MiscInfo _misc;
  AuxItems _aux_items;

  //! Undocumented
  object err;

   object _author;

  //! @decl void create(string textnumber)
  //!	Initializes a Text object.
   void create(int _no)
   {
      no=_no;
   }

   //! @ignore

   FETCHERC(text,array(string),_text,get_text,@({no,0,0x7fffffff}),
	    array_sscanf(res,"%s\n%s"));
   FETCHERC2b(stat,object,_stat,get_text_stat,@({no}),
	      (_misc=MiscInfo(res->misc_info),res));

   //! @endignore

   void update_misc(MiscInfo m)
   {
      _misc=m;
   }

  //! @fixme
  //!   Undocumented.
  void mark_as_read()
  {
    waitfor_stat();
    foreach( ({ @_misc->recpt,
		@_misc->ccrecpt,
		@_misc->bccrecpt }),
	     object rcpt)
      catch(con->async_cb_mark_as_read(0, rcpt->conf->no, ({ rcpt->local_no })));
    /* FIXME: Can't set read text in conferences the user isn't a member of ->
       error */
  }

  string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%d)", this_program, no);
  }

  //! @decl mixed prefetch_text
  //! @decl mixed prefetch_stat
  //! @decl mixed lines
  //! @decl mixed characters
  //! @decl mixed clear_stat
  //! @decl mixed aux_items
  //! @fixme
  //!   Undocumented

  //! @decl string text
  //!   The actual text (or body if you wish).

  //! @decl string subject
  //!   The message subject.

  //! @decl string author
  //!   The author of the text.

  //! @decl mixed misc
  //!   Misc info, including what conferences the message is posted to.
  //! @fixme
  //!   Needs a more complete description.

  //! @decl int marks
  //!   The number of marks on this text.

  //! @decl mixed creation_time
  //!   The time the text was created on the server.

   mixed `[](string what)
   {
      switch (what)
      {
	 case "create":
	    return create;
	 case "prefetch_text":
	   werror("prefetch_text(%d",no);
	    return prefetch_text;
	 case "prefetch_stat":
	    return prefetch_stat;

	 case "no":
	    return no;
	 case "error":
	    return err;

	 case "text":
	    waitfor_text();
	    return _text[1];

	 case "subject":
	    waitfor_text();
	    return _text[0];

	 case "author":
	    waitfor_stat();
	    return _author
	       || (_author=person(_stat->author));

	 case "misc":
	    waitfor_stat();
	    return _misc;

	 case "lines":
	    waitfor_stat();
	    return _stat->no_of_lines;

	 case "characters":
	    waitfor_stat();
	    return _stat->no_of_chars;

         case "marks":
            waitfor_stat();
	    return _stat->no_of_marks;

         case "creation_time":
	   waitfor_stat();
	   return _stat->creation_time;

         case "clear_stat":
           _stat=0;
	   return 0;
         case "aux_items":
	   waitfor_stat();
	   return _aux_items||(_aux_items=AuxItems(_stat->aux_items));

         case "mark_as_read":
            return mark_as_read;
      }
   }

   mixed `->(string what) { return `[](what); }

  array(string) _indices()
  {
    return ({ "create",	"prefetch_text", "misc",
	      "no",	"prefetch_stat", "marks",
	      "error",	"characters",	 "lines",
	      "text",	"clear_stat",	 "subject",
	      "author",	"creation_time", "aux_items",
	      "mark_as_read" });
  }
}

//! Returns the text @[no].
Text text(int no)
{
  if(_text[no])
    return _text[no];
  if(sizeof(_text)>1000)
    _text = ([]);
  
  return (_text[no]=Text(no));
}

//! All variables in this class is read only.
class Membership
{
  int              person;
  int              newtype;

  //!
  object           last_time_read;

  //!
  int(0..255)      priority;

  //!
  int              last_text_read;

  //!
  array(int)       read_texts;

  int(0..65535)    added_by;   // new

  //!
  object           added_at;   // new

  //!
  multiset(string) type;       // new

  //!
  int              position;   // new

  //!
  object conf;

  object err;

  void setup(object mb)
  {
    last_time_read=mb->last_time_read;
    read_texts=mb->read_texts;
    last_text_read=mb->last_text_read;
    priority=mb->priority;
    conf=Conference(mb->conf_no);

    if (mb->type)
    {
      added_at=mb->added_at;
      position=mb->position;
      added_by=mb->added_by;
      type=mb->type;
      newtype=1;
    }
  }

  void create(object mb,int pers)
  {
    person=pers;
    setup(mb);
  }

  //  FETCHER(unread,ProtocolTypes.TextMapping,_unread,local_to_global,@({conf->no,1,255}))

  //!
  int number_unread()
  {
    return (conf->no_of_texts+conf->first_local_no-1)
      -last_text_read -sizeof(read_texts);
  }

  //!
  void query_read_texts()
  {
//     werror("query_read_texts()\n");
//     werror("read_texts: %O\n",read_texts);
    setup(con->query_read_texts(person,conf->no));
//     werror("read_texts: %O\n",read_texts);
  }

  //! @decl array(object) unread_texts()

  array(object) get_unread_texts_blocking()
  {
#ifdef LYSKOM_DEBUG
    werror("get_unread_texts_blocking()\n");
#endif
    int i=last_text_read+1;
    mapping(int:int) local_to_global = ([]);

#ifdef LYSKOM_DEBUG
    werror("i: %d  last_text_read: %d\n",i,last_text_read);
    werror("conf: %d  conf->no_of_texts: %d\n",conf->no, conf->no_of_texts);
#endif
    if(i > con->get_uconf_stat(conf->no)->highest_local_no)
      return ({ }) ;

    /* Get all the global numbers after last-text-read */
    while(1)
    {
      ProtocolTypes.TextMapping textmapping=con->local_to_global(conf->no,i,255);
      ProtocolTypes.LocalToGlobalBlock block=textmapping->block;
      if(block->densep)            /* Use TextList */
      {
	ProtocolTypes.TextList textlist=block->dense;
	int j=textmapping->range_begin;

	foreach(textlist->texts, int global_text)
	  local_to_global[j++]=global_text;
      }
      else                         /* Use array(TextNumberPair) */
      {
	foreach(block->sparse, ProtocolTypes.TextNumberPair pair)
	  local_to_global[pair->local_number]=pair->global_number;
      }

      if(!textmapping->later_texts_exists)
	break;
      i=textmapping->range_end;
    }

    mapping unread_numbers =
      local_to_global -
      mkmapping(read_texts,allocate(sizeof(read_texts)));

    return map( sort(values(unread_numbers)), text );
  }

  mixed `[](string what)
  {
    switch (what)
    {
    case "unread_texts":
      return get_unread_texts_blocking();
    case "last_time_read":
    case "read_texts":
    case "last_text_read":
    case "priority":
    case "conf":
    case "added_at":
    case "position":
    case "type":
    case "number_unread":
    case "query_read_texts":
      return ::`[](what);

    }
  }

  mixed `->(string what) { return `[](what); }

  string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%d)", this_program, conf->no);
  }

  array(string) _indices()
  {
    return ({ "unread_texts", "last_time_read",
	      "read_texts",   "last_text_read",
	      "priority",     "conf",
	      "added_at",     "position",
	      "type",         "number_unread",
	      "query_read_texts" });
  }
}


//!
class Person
{
  //!
  int no;

   object conf;

   object err;

   //! @ignore

   FETCHER(stat,ProtocolTypes.Person,_person,get_person_stat,no);
   FETCHERC(unread,array(object),_unread,get_unread_confs,no,
	    Array.map(res,conference));
   FETCHERC2b(membership,array(object),_membership,get_membership,
	      @({no,0,65535,1}),
	      Array.map(res,Membership,no));

   //! @endignore

  //! @decl void create(int no)
   void create(int _no)
   {
      no=_no;
      conf=conference(no);
   }

  //! @decl mixed prefetch_stat
  //! @decl mixed prefetch_conf
  //! @decl mixed prefetch_membership
  //! @fixme
  //!   Undocumented

  //! @decl object error
  //! @decl Text user_area
  //! @decl mixed username
  //! @decl mixed privileges
  //! @decl mixed flags
  //! @decl mixed last_login
  //! @decl mixed total_time_present
  //! @decl mixed sessions
  //! @decl mixed created_lines
  //! @decl mixed created_bytes
  //! @decl mixed read_texts
  //! @decl mixed no_of_text_fetches
  //! @decl mixed created_persons
  //! @decl mixed created_confs
  //! @decl mixed first_created_local_no
  //! @decl mixed no_of_created_texts
  //! @decl mixed no_of_marks
  //! @decl mixed no_of_confs
  //! @decl mixed unread
  //! @decl int(0..0) clear_membership
  //! @decl mixed membership
  //! @fixme
  //!   Undocumented

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
	    waitfor_stat();
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
	    waitfor_stat();
	    return _person[what];
	 case "unread":
	    waitfor_unread();
	    return _unread;
	 case "clear_membership":
            _membership=0;
	    return 0;
	 case "membership":
	    waitfor_membership();
	    return _membership;
      }

      return conf[what];
   }

   mixed `->(string what) { return `[](what); }

  string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%d)", this_program, no);
  }

  array(string) _indices()
  {
    return ({ "prefetch_stat",		"create",
	      "prefetch_conf",		"no",
	      "prefetch_membership",	"error",
	      "user_area",		"privileges",
	      "username",		"flags",
	      "last_login",		"total_time_present",
	      "created_lines",		"sessions",
	      "created_bytes",		"read_texts",
	      "created_persons",	"no_of_text_fetches",
	      "created_confs",		"first_created_local_no",
	      "no_of_created_texts",	"unread",
	      "no_of_marks",		"membership"
	      "no_of_confs",		"clear_membership", })
      | indices(conf);
  }
}

//! Returns the person @[no].
Person person(int no)
{
   return _person[no] || (_person[no]=Person(no));
}

//!
class Conference
{
  int no;
  AuxItems _aux_items;

  private object err;


  //! @ignore

  FETCHER2(stat,ProtocolTypes.Conference,_conf,
	   ProtocolTypes.ConferenceOld,_confold,
	   get_conf_stat,get_conf_stat_old,no)

  //! @endignore

  //! @decl void create(int no)
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

      //	 case "presentation":
    case "msg_of_day":
      waitfor_stat();
      return text( (_conf||_confold)[what] );
    case "supervisor":
    case "permitted_submitters":
    case "super_conf":
      waitfor_stat();
      return conference( (_conf||_confold)[what] );
    case "creator":
      waitfor_stat();
      return person( (_conf||_confold)[what] );
    case "aux_items":
      waitfor_stat();
      return _aux_items||(_aux_items=AuxItems(_conf->aux_items));
    case "name":
    case "type":
    case "creation_time":
    case "last_written":
    case "nice":
    case "no_of_members":
    case "first_local_no":
    case "no_of_texts":
    case "presentation":
      waitfor_stat();
      return (_conf||_confold)[what];
      }
   }

   mixed `->(string what) { return `[](what); }

  array(string) _indices()
  {
    return ({ "create",	"aux_items",	"prefetch_stat",
	      "no",	"super_conf",	"no_of_members",
	      "error",	"supervisor",	"first_local_no",
	      "name",	"msg_of_day",	//"presentation",
	      "type",	"no_of_texts",
	      "creator","last_written",
	      "nice",	"creation_time",
	      "permitted_submitters", });
  }

  string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%d)", this_program, no);
  }
}

//! Returns conference number @[no].
Conference conference(int no)
{
   return _conference[no] || (_conference[no]=Conference(no));
}

//! Runs a LysKOM completion on the given string,
//! returning an array of confzinfos of the match.
array(ProtocolTypes.ConfZInfo) try_complete_person(string orig)
{
   return con->lookup_z_name(orig,1,0);
}

//! @decl object login(int user_no,string password)
//! @decl object login(int user_no,string password,int invisible)
//!   Performs a login. Throws a lyskom error if unsuccessful.
//! @returns
//!   The session object logged in.
object login(int user_no,string password,
	     void|int invisible)
{
   con->login(user_no,password,invisible);
   user=person(user_no);
   return this_object();
}

//! Create a person, which will be logged in.
//! returns the new person object
object create_person(string name,string password)
{
   if (!stringp(name)||!stringp(password))
      error("bad types to create_person call\n");
   return user=person(con->create_person_old(name,password));
}

//! Logouts from the server.
//! returns the called object
object logout()
{
   if (con)
      con->logout();
   return this_object();
}

//! @decl object create_text(string subject, string body, mapping options)
//! @decl object create_text(string subject, string body, mapping options, @
//!                          function callback, mixed ...extra)
//! 	Creates a new text.
//!
//! 	if @[callback] is given, the function will be called when the text
//! 	has been created, with the text as first argument.
//!	Otherwise, the new text is returned.
//!
//!	@[options] is a mapping that may contain:
//!	@mapping
//!	  @member Conference|array(Conference) "recpt"
//!	    recipient conferences.
//!	  @member Conference|array(Conference) "cc"
//!	    cc-recipient conferences.
//!	  @member Conference|array(Conference) "bcc"
//!	    bcc-recipient conferences*.
//!	  @member Text|array(Text) "comm_to"
//!	    The text(s) to be commented.
//!	  @member Text|array(Text) "foot_to"
//!	    The text(s) to be footnoted.
//!	  @member int(0..1) "anonymous"
//!	    send text anonymously.
//!	  @member array(AuxItemInput) "aux_items"
//!	    AuxItems you want to set for the text*.
//!	@endmapping
//!
//! @note
//!	The items above marked with '*' are only available on protocol 10
//!	servers. A LysKOM error will be thrown if the call fails.
//!
//! @seealso
//!   @[Conference.create_text()], @[Text.comment()], @[Text.footnote()]
object|void create_text(string subject,string body,
			void|mapping options,
			void|function callback,
			void|mixed ...extra)
{
   string text=replace(subject,"\n"," ")+"\n"+body;
   MiscInfo misc=MiscInfo(options);

   if (!options) options=([]);

   return _create_text(text,misc,
		       options->aux_items,
		       options->anonymous,
		       callback,@extra);
}

object|void _create_text(string textstring,
			 MiscInfo misc,
			 void|array(AuxItemInput) aux_items,
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

   array args = ({ textstring, misc->encode() });
   if(protlevel<10)
     call += "_old";
   else
     args += ({ aux_items || ({}) });

   if (callback)
   {
      con["async_cb_"+call]
	 (lambda(int|object res)
	  {
	     if(objectp(res)) return res;
	     callback(text(res),@extra);
	  }, @args);
      return;
   }

   res = con[call](@args);

   if (objectp(res)) return res;
   return text(res);
}

//! 	Sends a message.
//!
//!	@[options] is a mapping that may contain:
//!	@mapping
//!	  @member Conference "recpt"
//!	    recipient conference.
//!	@endmapping
object|void send_message(string textstring, mapping options)
{
  int|object res;
  string call;

  if(!options) options = ([]);

  if(!options->recpt)
    res = con["async_broadcast"](textstring);
  else
    res = con["async_send_message"](options->recpt->no, textstring);

  if (objectp(res)) return res;
  return text(res);
}

//!
void register_async_message_callback(function(int,int,string:void) cb)
{
  con->con->add_async_callback("async-send-message", cb);
}

string _sprintf(int t)
{
  return t=='O' && sprintf("%O(%s)", this_program, server);
}
