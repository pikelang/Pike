#pike __REAL_VERSION__

//  $Id$
//!	This module contains nice abstraction for calls into the
//!	server. They are named "@tt{@i{call@}@}",
//!	"@tt{async_@i{call@}@}" or 
//!	"@tt{async_cb_@i{call@}@}", depending on 
//!	how you want the call to be done. 

import .Helper;
import .ProtocolTypes;

int _no=1;

//!	This is the main request class. All lyskom request
//!	classes inherit this class.
//!
class _Request
{
   private Stdio.File raw;
   private int ref;

   private mixed res;

#if constant(thread_create) && !LYSKOM_UNTHREADED
   private object wait_lock;
   private object wait_mutex;
#endif

   function callback;

   //! @decl void async(mixed ...args)
   //! @decl mixed sync(mixed ...args)
   //!	Initialise an asynchronous or a synchronous call,
   //!	the latter is also evaluating the result. This calls
   //!	@tt{indata()@} in itself, to get the correct arguments to 
   //!	the lyskom protocol call.

   //! @decl void _async(int call, mixed_data)
   //! @decl mixed _sync(int call, mixed_data)
   //!	Initialise an asynchronous or a synchronous call,
   //!	the latter is also evaluating the result. These
   //!	are called by async and sync respectively.

   void _async(int call,mixed ... data)
   {
      ref=raw->send(encode(call,@data),_reply);
   }
   
   mixed _sync(int call,mixed ... data)
   {
#if constant(thread_create) && !LYSKOM_UNTHREADED
      _async(call,@data);
      return `()();
#else
      mixed res=raw->send_sync(encode(call,@data),_reply);
      return _reply(res);
#endif
   }

   //! @decl mixed _reply(object|array what)
   //! @decl mixed reply(object|array what)
   //!	@[_reply()] is called as callback to evaluate the result, 
   //!	and calls @[reply()] in itself to do the real work.

   mixed _reply(object|array what)
   {
      if (objectp(what))
      {
	 error=what;
	 failure(what);
      }
      else
      {
	 ok=1;
	 res=reply(what);
      }
#if constant(thread_create) && !LYSKOM_UNTHREADED
      if (wait_lock)
	 destruct(wait_lock);
#endif
      if (callback) callback(error||res);

      return res;
   }

   mixed reply(array what); // virtual
   void failure(object err); // virtual
   array indata(mixed ... args); // virtual

   //! @decl mixed `()()
   //!	Wait for the call to finish.

#if constant(thread_create) && !LYSKOM_UNTHREADED
   mixed `()() // wait
   {
      if (ok || error) return res;
      wait_mutex=Thread.Mutex();
      wait_lock=wait_mutex->lock(); // lock it
      wait_lock=wait_mutex->lock(1); // lock it again, ie wait
      return res;
   }
#else
   mixed `()() // wait
   {
      if (ok || error) return res;
      mixed tmp=_reply(raw->sync_do(ref));
      return tmp;
   }
#endif

   // public

   //!	Tells if the call has executed ok
   int(0..1) ok;

   //!	How the call failed.
   //!	The call has completed if @expr{(ok||error)@}.
   //!
   object error;

   void create(Stdio.File _raw)
   {
      raw=_raw;
   }

   void async(mixed ...args)
   {
      _async(@indata(@args));
   }

   mixed sync(mixed ...args)
   {
      return _sync(@indata(@args));
   }
}

//
// automatically generated requests
//

/* 0 */
class Login_old
{
   inherit _Request;

   array indata(int(0..65535) person,
                string passwd)
   {
      return ({0,
               person,
               H(passwd)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 1 */
class Logout
{
   inherit _Request;

   array indata()
   {
      return ({1});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 2 */
class Change_conference
{
   inherit _Request;

   array indata(int(0..65535) conference)
   {
      return ({2,
               conference});
   }

   int(0..65535) reply(array what)
   {
      return what[0]; /* INT16 */
   }

   void failure(object error)
   {
   }
}

/* 3 */
class Change_name
{
   inherit _Request;

   array indata(int(0..65535) conference,
                string new_name)
   {
      return ({3,
               conference,
               H(new_name)});
   }

   string reply(array what)
   {
      return what[0]; /* HOLLERITH */
   }

   void failure(object error)
   {
   }
}

/* 4 */
class Change_what_i_am_doing
{
   inherit _Request;

   array indata(string what_am_i_doing)
   {
      return ({4,
               H(what_am_i_doing)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 5 */
class Create_person_old
{
   inherit _Request;

   array indata(string name,
                string passwd)
   {
      return ({5,
               H(name),
               H(passwd)});
   }

   int reply(array what)
   {
      return (int)what[0]; /* Pers-no */
   }

   void failure(object error)
   {
   }
}

/* 6 */
class Get_person_stat_old
{
   inherit _Request;

   array indata(int(0..65535) person,
                int mask)
   {
      return ({6,
               person,
               mask});
   }


   Person person;

   Person reply(array what)
   {
      /* ( Person ) */
      return Person(@what);
   }

   void failure(object error)
   {
   }
}

/* 7 */
class Set_priv_bits
{
   inherit _Request;

   array indata(int(0..65535) person,
                multiset(string) privileges)
   {
      return ({7,
               person,
               @B(@rows(privileges,privbitsnames))});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 8 */
class Set_passwd
{
   inherit _Request;

   array indata(int(0..65535) person,
                string old_pwd,
                string new_pwd)
   {
      return ({8,
               person,
               H(old_pwd),
               H(new_pwd)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 9 */
class Query_read_texts_old
{
   inherit _Request;

   array indata(int(0..65535) person,
                int(0..65535) conference)
   {
      return ({9,
               person,
               conference});
   }


   MembershipOld membershipold;

   MembershipOld reply(array what)
   {
      /* ( Membership-Old ) */
      return MembershipOld(@what);
   }

   void failure(object error)
   {
   }
}

/* 10 */
class Create_conf_old
{
   inherit _Request;

   array indata(string name,
                multiset(string) type)
   {
      return ({10,
               H(name),
               @B(@rows(type,extendedconftypenames))});
   }

   int(0..65535) reply(array what)
   {
      return what[0]; /* Conf-No */
   }

   void failure(object error)
   {
   }
}

/* 11 */
class Delete_conf
{
   inherit _Request;

   array indata(int(0..65535) conf)
   {
      return ({11,
               conf});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 12 */
class Lookup_name
{
   inherit _Request;

   array indata(string name)
   {
      return ({12,
               H(name)});
   }


   ConfListArchaic conflistarchaic;

   ConfListArchaic reply(array what)
   {
      /* ( Conf-List-Archaic ) */
      return ConfListArchaic(@what);
   }

   void failure(object error)
   {
   }
}

/* 13 */
class Get_conf_stat_older
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int mask)
   {
      return ({13,
               conf_no,
               mask});
   }


   ConferenceOld conferenceold;

   ConferenceOld reply(array what)
   {
      /* ( Conference-Old ) */
      return ConferenceOld(@what);
   }

   void failure(object error)
   {
   }
}

/* 14 */
class Add_member_old
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int(0..65535) pers_no,
                int(0..255) priority,
                int(0..65535) where)
   {
      return ({14,
               conf_no,
               pers_no,
               priority,
               where});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 15 */
class Sub_member
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int(0..65535) pers_no)
   {
      return ({15,
               conf_no,
               pers_no});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 16 */
class Set_presentation
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int text_no)
   {
      return ({16,
               conf_no,
               text_no});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 17 */
class Set_etc_motd
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int text_no)
   {
      return ({17,
               conf_no,
               text_no});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 18 */
class Set_supervisor
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int(0..65535) admin)
   {
      return ({18,
               conf_no,
               admin});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 19 */
class Set_permitted_submitters
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int(0..65535) perm_sub)
   {
      return ({19,
               conf_no,
               perm_sub});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 20 */
class Set_super_conf
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int(0..65535) super_conf)
   {
      return ({20,
               conf_no,
               super_conf});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 21 */
class Set_conf_type
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                multiset(string) type)
   {
      return ({21,
               conf_no,
               @B(@rows(type,extendedconftypenames))});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 22 */
class Set_garb_nice
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int nice)
   {
      return ({22,
               conf_no,
               nice});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 23 */
class Get_marks
{
   inherit _Request;

   array indata()
   {
      return ({23});
   }


   array(Mark) marks;

   array(Mark) reply(array what)
   {
      /* ( ARRAY Mark ) */
      return marks=Array.map(what[1]/2,
                                lambda(array z) { return Mark(@z); });
   }

   void failure(object error)
   {
   }
}

/* 24 */
class Mark_text_old
{
   inherit _Request;

   array indata(int text,
                int(0..255) mark_type)
   {
      return ({24,
               text,
               mark_type});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 25 */
class Get_text
{
   inherit _Request;

   array indata(int text,
                int start_char,
                int end_char)
   {
      return ({25,
               text,
               start_char,
               end_char});
   }

   string reply(array what)
   {
      return what[0]; /* HOLLERITH */
   }

   void failure(object error)
   {
   }
}

/* 26 */
class Get_text_stat_old
{
   inherit _Request;

   array indata(int text_no)
   {
      return ({26,
               text_no});
   }


   TextStatOld textstatold;

   TextStatOld reply(array what)
   {
      /* ( Text-Stat-Old ) */
      return TextStatOld(@what);
   }

   void failure(object error)
   {
   }
}

/* 27 */
class Mark_as_read
{
   inherit _Request;

   array indata(int(0..65535) conference,
                array(int) text)
   {
      return ({27,
               conference,
               @A((array(string))text)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 28 */
class Create_text_old
{
   inherit _Request;

   array indata(string text,
                array(string) misc_info)
   {
      return ({28,
               H(text),
               @A(misc_info)});
   }

   int reply(array what)
   {
      return (int)what[0]; /* Text-No */
   }

   void failure(object error)
   {
   }
}

/* 29 */
class Delete_text
{
   inherit _Request;

   array indata(int text)
   {
      return ({29,
               text});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 30 */
class Add_recipient
{
   inherit _Request;

   array indata(int text_no,
                int(0..65535) conf_no,
                int recpt_type)
   {
      return ({30,
               text_no,
               conf_no,
               recpt_type});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 31 */
class Sub_recipient
{
   inherit _Request;

   array indata(int text_no,
                int(0..65535) conf_no)
   {
      return ({31,
               text_no,
               conf_no});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 32 */
class Add_comment
{
   inherit _Request;

   array indata(int text_no,
                int comment_to)
   {
      return ({32,
               text_no,
               comment_to});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 33 */
class Sub_comment
{
   inherit _Request;

   array indata(int text_no,
                int comment_to)
   {
      return ({33,
               text_no,
               comment_to});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 34 */
class Get_map
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int first_local_no,
                int no_of_texts)
   {
      return ({34,
               conf_no,
               first_local_no,
               no_of_texts});
   }


   TextList textlist;

   TextList reply(array what)
   {
      /* ( Text-List ) */
      return TextList(@what);
   }

   void failure(object error)
   {
   }
}

/* 35 */
class Get_time
{
   inherit _Request;

   array indata()
   {
      return ({35});
   }


   LysKOMTime lyskomtime;

   LysKOMTime reply(array what)
   {
      /* ( Time ) */
      return LysKOMTime(@what);
   }

   void failure(object error)
   {
   }
}

/* 36 */
class Get_info_old
{
   inherit _Request;

   array indata()
   {
      return ({36});
   }


   InfoOld infoold;

   InfoOld reply(array what)
   {
      /* ( Info-Old ) */
      return InfoOld(@what);
   }

   void failure(object error)
   {
   }
}

/* 37 */
class Add_footnote
{
   inherit _Request;

   array indata(int text_no)
   {
      return ({37,
               text_no});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 38 */
class Sub_footnote
{
   inherit _Request;

   array indata(int text_no,
                int footnote_to)
   {
      return ({38,
               text_no,
               footnote_to});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 39 */
class Who_is_on_old
{
   inherit _Request;

   array indata()
   {
      return ({39});
   }


   array(WhoInfoOld) whoinfoolds;

   array(WhoInfoOld) reply(array what)
   {
      /* ( ARRAY Who-Info-Old ) */
      return whoinfoolds=Array.map(what[1]/3,
                                lambda(array z) { return WhoInfoOld(@z); });
   }

   void failure(object error)
   {
   }
}

/* 40 */
class Set_unread
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int no_of_unread)
   {
      return ({40,
               conf_no,
               no_of_unread});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 41 */
class Set_motd_of_lyskom
{
   inherit _Request;

   array indata(int text_no)
   {
      return ({41,
               text_no});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 42 */
class Enable
{
   inherit _Request;

   array indata(int(0..255) level)
   {
      return ({42,
               level});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 43 */
class Sync_kom
{
   inherit _Request;

   array indata()
   {
      return ({43});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 44 */
class Shutdown_kom
{
   inherit _Request;

   array indata(int(0..255) exit_val)
   {
      return ({44,
               exit_val});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 45 */
class Broadcast
{
   inherit _Request;

   array indata(string message)
   {
      return ({45,
               H(message)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 46 */
class Get_membership_old
{
   inherit _Request;

   array indata(int(0..65535) person,
                int(0..65535) first,
                int(0..65535) no_of_confs,
                int(0..1) want_read_texts)
   {
      return ({46,
               person,
               first,
               no_of_confs,
               B(want_read_texts)});
   }


   array(MembershipOld) membershipolds;

   array(MembershipOld) reply(array what)
   {
      /* ( ARRAY Membership-Old ) */
      return membershipolds=Array.map(what[1]/14,
                                lambda(array z) { return MembershipOld(@z); });
   }

   void failure(object error)
   {
   }
}

/* 47 */
class Get_created_texts
{
   inherit _Request;

   array indata(int(0..65535) person,
                int first,
                int no_of_texts)
   {
      return ({47,
               person,
               first,
               no_of_texts});
   }


   TextList textlist;

   TextList reply(array what)
   {
      /* ( Text-List ) */
      return TextList(@what);
   }

   void failure(object error)
   {
   }
}

/* 48 */
class Get_members_old
{
   inherit _Request;

   array indata(int(0..65535) conf,
                int(0..65535) first,
                int(0..65535) no_of_members)
   {
      return ({48,
               conf,
               first,
               no_of_members});
   }

   array(int(0..65535)) persnos;

   array(int(0..65535)) reply(array what)
   {
      /* ( ARRAY Pers-No ) */
      return (array(int(0..65535)))what[1];
   }

   void failure(object error)
   {
   }
}

/* 49 */
class Get_person_stat
{
   inherit _Request;

   array indata(int(0..65535) pers_no)
   {
      return ({49,
               pers_no});
   }


   Person person;

   Person reply(array what)
   {
      /* ( Person ) */
      return Person(@what);
   }

   void failure(object error)
   {
   }
}

/* 50 */
class Get_conf_stat_old
{
   inherit _Request;

   array indata(int(0..65535) conf_no)
   {
      return ({50,
               conf_no});
   }


   ConferenceOld conferenceold;

   ConferenceOld reply(array what)
   {
      /* ( Conference-Old ) */
      return ConferenceOld(@what);
   }

   void failure(object error)
   {
   }
}

/* 51 */
class Who_is_on
{
   inherit _Request;

   array indata()
   {
      return ({51});
   }


   array(WhoInfo) whoinfos;

   array(WhoInfo) reply(array what)
   {
      /* ( ARRAY Who-Info ) */
      return whoinfos=Array.map(what[1]/5,
                                lambda(array z) { return WhoInfo(@z); });
   }

   void failure(object error)
   {
   }
}

/* 52 */
class Get_unread_confs
{
   inherit _Request;

   array indata(int(0..65535) pers_no)
   {
      return ({52,
               pers_no});
   }

   array(int(0..65535)) confnos;

   array(int(0..65535)) reply(array what)
   {
      /* ( ARRAY Conf-No ) */
      return (array(int(0..65535)))what[1];
   }

   void failure(object error)
   {
   }
}

/* 53 */
class Send_message
{
   inherit _Request;

   array indata(int(0..65535) recipient,
                string message)
   {
      return ({53,
               recipient,
               H(message)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 58 */
class Get_last_text
{
   inherit _Request;

   array indata(LysKOMTime before)
   {
      return ({58,
               @before->encode()});
   }

   int reply(array what)
   {
      return what[0]; /* Text-No */
   }

   void failure(object error)
   {
   }
}

/* 59 */
class Create_anonymous_text_old
{
   inherit _Request;

   array indata(string text,
                array(int) misc_info)
   {
      return ({59,
               H(text),
               @A((array(string))misc_info)});
   }

   int reply(array what)
   {
      return (int)what[0]; /* Text-No */
   }

   void failure(object error)
   {
   }
}

/* 60 */
class Find_next_text_no
{
   inherit _Request;

   array indata(int start)
   {
      return ({60,
               start});
   }

   int reply(array what)
   {
      return what[0]; /* Text-No */
   }

   void failure(object error)
   {
   }
}

/* 61 */
class Find_previous_text_no
{
   inherit _Request;

   array indata(int start)
   {
      return ({61,
               start});
   }

   int reply(array what)
   {
      return what[0]; /* Text-No */
   }

   void failure(object error)
   {
   }
}

/* 62 */
class Login
{
   inherit _Request;

   array indata(int(0..65535) person,
                string passwd,
                int(0..1) is_invisible)
   {
      return ({62,
               person,
               H(passwd),
               B(is_invisible)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 63 */
class Who_is_on_ident
{
   inherit _Request;

   array indata()
   {
      return ({63});
   }


   array(WhoInfoIdent) whoinfoidents;

   array(WhoInfoIdent) reply(array what)
   {
      /* ( ARRAY Who-Info-Ident ) */
      return whoinfoidents=Array.map(what[1]/7,
                                lambda(array z) { return WhoInfoIdent(@z); });
   }

   void failure(object error)
   {
   }
}

/* 64 */
class Get_session_info_ident
{
   inherit _Request;

   array indata(int session_no)
   {
      return ({64,
               session_no});
   }


   SessionInfoIdent sessioninfoident;

   SessionInfoIdent reply(array what)
   {
      /* ( Session-Info-Ident ) */
      return SessionInfoIdent(@what);
   }

   void failure(object error)
   {
   }
}

/* 65 */
class Re_lookup_person
{
   inherit _Request;

   array indata(string regexp)
   {
      return ({65,
               H(regexp)});
   }

   array(int(0..65535)) persnos;

   array(int(0..65535)) reply(array what)
   {
      /* ( ARRAY Pers-No ) */
      return (array(int(0..65535)))what[1];
   }

   void failure(object error)
   {
   }
}

/* 66 */
class Re_lookup_conf
{
   inherit _Request;

   array indata(string regexp)
   {
      return ({66,
               H(regexp)});
   }

   array(int(0..65535)) confnos;

   array(int(0..65535)) reply(array what)
   {
      /* ( ARRAY Conf-No ) */
      return (array(int(0..65535)))what[1];
   }

   void failure(object error)
   {
   }
}

/* 67 */
class Lookup_person
{
   inherit _Request;

   array indata(string name)
   {
      return ({67,
               H(name)});
   }

   array(int(0..65535)) persnos;

   array(int(0..65535)) reply(array what)
   {
      /* ( ARRAY Pers-No ) */
      return (array(int(0..65535)))what[1];
   }

   void failure(object error)
   {
   }
}

/* 68 */
class Lookup_conf
{
   inherit _Request;

   array indata(string name)
   {
      return ({68,
               H(name)});
   }

   array(int(0..65535)) confnos;

   array(int(0..65535)) reply(array what)
   {
      /* ( ARRAY Conf-No ) */
      return (array(int(0..65535)))what[1];
   }

   void failure(object error)
   {
   }
}

/* 69 */
class Set_client_version
{
   inherit _Request;

   array indata(string client_name,
                string client_version)
   {
      return ({69,
               H(client_name),
               H(client_version)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 70 */
class Get_client_name
{
   inherit _Request;

   array indata(int session)
   {
      return ({70,
               session});
   }

   string reply(array what)
   {
      return what[0]; /* HOLLERITH */
   }

   void failure(object error)
   {
   }
}

/* 71 */
class Get_client_version
{
   inherit _Request;

   array indata(int session)
   {
      return ({71,
               session});
   }

   string reply(array what)
   {
      return what[0]; /* HOLLERITH */
   }

   void failure(object error)
   {
   }
}

/* 72 */
class Mark_text
{
   inherit _Request;

   array indata(int text,
                int(0..255) mark_type)
   {
      return ({72,
               text,
               mark_type});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 73 */
class Unmark_text
{
   inherit _Request;

   array indata(int text_no)
   {
      return ({73,
               text_no});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 74 */
class Re_z_lookup
{
   inherit _Request;

   array indata(string regexp,
                int(0..1) want_persons,
                int(0..1) want_confs)
   {
      return ({74,
               H(regexp),
               want_persons,
               want_confs});
   }


   array(ConfZInfo) confzinfos;

   array(ConfZInfo) reply(array what)
   {
      /* ( ARRAY Conf-Z-Info ) */
      return confzinfos=Array.map(what[1]/3,
                                lambda(array z) { return ConfZInfo(@z); });
   }

   void failure(object error)
   {
   }
}

/* 75 */
class Get_version_info
{
   inherit _Request;

   array indata()
   {
      return ({75});
   }


   VersionInfo versioninfo;

   VersionInfo reply(array what)
   {
      /* ( Version-Info ) */
      return VersionInfo(@what);
   }

   void failure(object error)
   {
   }
}

/* 76 */
class Lookup_z_name
{
   inherit _Request;

   array indata(string name,
                int(0..1) want_pers,
                int(0..1) want_confs)
   {
      return ({76,
               H(name),
               want_pers,
               want_confs});
   }


   array(ConfZInfo) confzinfos;

   array(ConfZInfo) reply(array what)
   {
      /* ( ARRAY Conf-Z-Info ) */
      return confzinfos=Array.map(what[1]/3,
                                lambda(array z) { return ConfZInfo(@z); });
   }

   void failure(object error)
   {
   }
}

/* 77 */
class Set_last_read
{
   inherit _Request;

   array indata(int(0..65535) conference,
                int last_read)
   {
      return ({77,
               conference,
               last_read});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 78 */
class Get_uconf_stat
{
   inherit _Request;

   array indata(int(0..65535) conference)
   {
      return ({78,
               conference});
   }


   UConference uconference;

   UConference reply(array what)
   {
      /* ( UConference ) */
      return UConference(@what);
   }

   void failure(object error)
   {
   }
}

/* 79 */
class Set_info
{
   inherit _Request;

   array indata(InfoOld info)
   {
      return ({79,
               @info->encode()});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 80 */
class Accept_async
{
   inherit _Request;

   array indata(array(int) request_list)
   {
      return ({80,
               @A((array(string))request_list)});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 81 */
class Query_async
{
   inherit _Request;

   array indata()
   {
      return ({81});
   }

   array(int) reply(array what)
   {
      return what[0]; /* ARRAY INT32 */
   }

   void failure(object error)
   {
   }
}

/* 82 */
class User_active
{
   inherit _Request;

   array indata()
   {
      return ({82});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 83 */
class Who_is_on_dynamic
{
   inherit _Request;

   array indata(int(0..1) want_visible,
                int(0..1) want_invisible,
                int active_last)
   {
      return ({83,
               want_visible,
               want_invisible,
               active_last});
   }


   array(DynamicSessionInfo) dynamicsessioninfos;

   array(DynamicSessionInfo) reply(array what)
   {
      /* ( ARRAY Dynamic-Session-Info ) */
      return dynamicsessioninfos=Array.map(what[1]/6,
                                lambda(array z) { return DynamicSessionInfo(@z); });
   }

   void failure(object error)
   {
   }
}

/* 84 */
class Get_static_session_info
{
   inherit _Request;

   array indata(int session_no)
   {
      return ({84,
               session_no});
   }


   StaticSessionInfo staticsessioninfo;

   StaticSessionInfo reply(array what)
   {
      /* ( Static-Session-Info ) */
      return StaticSessionInfo(@what);
   }

   void failure(object error)
   {
   }
}

/* 85 */
class Get_collate_table
{
   inherit _Request;

   array indata()
   {
      return ({85});
   }

   string reply(array what)
   {
      return what[0]; /* HOLLERITH */
   }

   void failure(object error)
   {
   }
}

/* 86 */
class Create_text
{
   inherit _Request;

   array indata(string text,
                array(int) misc_info,
                array(AuxItemInput) aux_items)
   {
      return ({86,
               H(text),
               @A((array(string))misc_info),
               @A(aux_items->encode())});
   }

   int reply(array what)
   {
      return (int)what[0]; /* Text-No */
   }

   void failure(object error)
   {
   }
}

/* 87 */
class Create_anonymous_text
{
   inherit _Request;

   array indata(string text,
                array(int) misc_info,
                array(AuxItemInput) aux_items)
   {
      return ({87,
               H(text),
               @A((array(string))misc_info),
               @A(aux_items->encode())});
   }

   int reply(array what)
   {
      return (int)what[0]; /* Text-No */
   }

   void failure(object error)
   {
   }
}

/* 88 */
class Create_conf
{
   inherit _Request;

   array indata(string name,
                multiset(string) type,
                array(AuxItemInput) aux_items)
   {
      return ({88,
               H(name),
               @B(@rows(type,extendedconftypenames)),
               @A(aux_items->encode())});
   }

   int(0..65535) reply(array what)
   {
      return what[0]; /* Conf-No */
   }

   void failure(object error)
   {
   }
}

/* 89 */
class Create_person
{
   inherit _Request;

   array indata(string name,
                string passwd,
                array(AuxItemInput) aux_items)
   {
      return ({89,
               H(name),
               H(passwd),
               @A(aux_items->encode())});
   }

   int(0..65535) reply(array what)
   {
      return what[0]; /* Pers-No */
   }

   void failure(object error)
   {
   }
}

/* 90 */
class Get_text_stat
{
   inherit _Request;

   array indata(int text_no)
   {
      return ({90,
               text_no});
   }


   TextStat textstat;

   TextStat reply(array what)
   {
      /* ( Text-Stat ) */
      return TextStat(@what);
   }

   void failure(object error)
   {
   }
}

/* 91 */
class Get_conf_stat
{
   inherit _Request;

   array indata(int(0..65535) conf_no)
   {
      return ({91,
               conf_no});
   }


   Conference conference;

   Conference reply(array what)
   {
      /* ( Conference ) */
      return Conference(@what);
   }

   void failure(object error)
   {
   }
}

/* 92 */
class Modify_text_info
{
   inherit _Request;

   array indata(int text,
                array(int) delete,
                array(AuxItemInput) add)
   {
      return ({92,
               text,
               @A((array(string))delete),
               @A(add->encode())});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 93 */
class Modify_conf_info
{
   inherit _Request;

   array indata(int(0..65535) conf,
                array(int) delete,
                array(AuxItemInput) add)
   {
      return ({93,
               conf,
               @A((array(string))delete),
               @A(add->encode())});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 94 */
class Get_info
{
   inherit _Request;

   array indata()
   {
      return ({94});
   }


   Info info;

   Info reply(array what)
   {
      /* ( Info ) */
      return Info(@what);
   }

   void failure(object error)
   {
   }
}

/* 95 */
class Modify_system_info
{
   inherit _Request;

   array indata(array(int) items_to_delete,
                array(AuxItemInput) items_to_add)
   {
      return ({95,
               @A((array(string))items_to_delete),
               @A(items_to_add->encode())});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 96 */
class Query_predefined_aux_items
{
   inherit _Request;

   array indata()
   {
      return ({96});
   }

   array(int) reply(array what)
   {
      return what[0]; /* ARRAY INT32 */
   }

   void failure(object error)
   {
   }
}

/* 97 */
class Set_expire
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int expire)
   {
      return ({97,
               conf_no,
               expire});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 98 */
class Query_read_texts
{
   inherit _Request;

   array indata(int(0..65535) person,
                int(0..65535) conference)
   {
      return ({98,
               person,
               conference});
   }


   Membership membership;

   Membership reply(array what)
   {
      /* ( Membership ) */
      return Membership(@what);
   }

   void failure(object error)
   {
   }
}

/* 99 */
class Get_membership
{
   inherit _Request;

   array indata(int(0..65535) person,
                int(0..65535) first,
                int(0..65535) no_of_confs,
                int(0..1) want_read_texts)
   {
      return ({99,
               person,
               first,
               no_of_confs,
               B(want_read_texts)});
   }


   array(Membership) memberships;

   array(Membership) reply(array what)
   {
      /* ( ARRAY Membership ) */
      return memberships=Array.map(what[1]/26,
                                lambda(array z) { return Membership(@z); });
   }

   void failure(object error)
   {
   }
}

/* 100 */
class Add_member
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int(0..65535) pers_no,
                int(0..255) priority,
                int(0..65535) where,
                multiset(string) type)
   {
      return ({100,
               conf_no,
               pers_no,
               priority,
               where,
               @B(@rows(type,membershiptypenames))});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 101 */
class Get_members
{
   inherit _Request;

   array indata(int(0..65535) conf,
                int(0..65535) first,
                int(0..65535) no_of_members)
   {
      return ({101,
               conf,
               first,
               no_of_members});
   }


   array(Member) members;

   array(Member) reply(array what)
   {
      /* ( ARRAY Member ) */
      return members=Array.map(what[1]/12,
                                lambda(array z) { return Member(@z); });
   }

   void failure(object error)
   {
   }
}

/* 102 */
class Set_membership_type
{
   inherit _Request;

   array indata(int(0..65535) pers,
                int(0..65535) conf,
                multiset(string) type)
   {
      return ({102,
               pers,
               conf,
               @B(@rows(type,membershiptypenames))});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

/* 103 */
class Local_to_global
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int first_local_no,
                int no_of_existing_texts)
   {
      return ({103,
               conf_no,
               first_local_no,
               no_of_existing_texts});
   }


   TextMapping textmapping;

   TextMapping reply(array what)
   {
      /* ( Text-Mapping ) */
      return TextMapping(@what);
   }

   void failure(object error)
   {
   }
}

/* 104 */
class Map_created_texts
{
   inherit _Request;

   array indata(int(0..65535) author,
                int first_local_no,
                int no_of_existing_texts)
   {
      return ({104,
               author,
               first_local_no,
               no_of_existing_texts});
   }


   TextMapping textmapping;

   TextMapping reply(array what)
   {
      /* ( Text-Mapping ) */
      return TextMapping(@what);
   }

   void failure(object error)
   {
   }
}

/* 105 */
class Set_keep_commented
{
   inherit _Request;

   array indata(int(0..65535) conf_no,
                int keep_commented)
   {
      return ({105,
               conf_no,
               keep_commented});
   }

   void reply(array what)
   {
   }

   void failure(object error)
   {
   }
}

