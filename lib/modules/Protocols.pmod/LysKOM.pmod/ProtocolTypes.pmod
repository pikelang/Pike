#pike __REAL_VERSION__

import .Helper;


//! Data types as defined by the LysKOM protocol specification.

//
// automatically generated datatypes
//

//!
class LysKOMTime
{
   int              seconds;                       // INT32
   int              minutes;                       // INT32
   int              hours;                         // INT32
   int              day;                           // INT32
   int              month;                         // INT32
   int              year;                          // INT32
   int              day_of_week;                   // INT32
   int              day_of_year;                   // INT32
   int(0..1)        is_dst;                        // BOOL

  string _sprintf(int t){
    return t=='O' && sprintf("%O(%s)", this_program, print(1));
  }

   string print(void|int long)
   {
     string res=sprintf("%d-%02d-%02d %02d:%02d",
			1900+year,month+1,day,hours,minutes);
     if(long)
       res+=sprintf(":%02d",seconds);
     return res;
   }
  
   void create(string|int ... args)
   {
      seconds=(int)args[0];                        // INT32
      minutes=(int)args[1];                        // INT32
      hours=(int)args[2];                          // INT32
      day=(int)args[3];                            // INT32
      month=(int)args[4];                          // INT32
      year=(int)args[5];                           // INT32
      day_of_week=(int)args[6];                    // INT32
      day_of_year=(int)args[7];                    // INT32
      is_dst=(int)args[8];                         // BOOL
   }

   array encode()
   {
      return
         ({
            seconds,                               // INT32
            minutes,                               // INT32
            hours,                                 // INT32
            day,                                   // INT32
            month,                                 // INT32
            year,                                  // INT32
            day_of_week,                           // INT32
            day_of_year,                           // INT32
            is_dst,                                // BOOL
         });
   }
}

//!
class TextStatOld
{
   LysKOMTime       creation_time;                 // Time
   int(0..65535)    author;                        // Pers-No
   int              no_of_lines;                   // INT32
   int              no_of_chars;                   // INT32
   int(0..65535)    no_of_marks;                   // INT16
   array(int)       misc_info;                     // ARRAY Misc-Info

   void create(string|int|array ... args)
   {
      creation_time=LysKOMTime(@args[0..8]);       // Time
      author=(int)args[9];                         // Pers-No
      no_of_lines=(int)args[10];                   // INT32
      no_of_chars=(int)args[11];                   // INT32
      no_of_marks=(int)args[12];                   // INT16
      // --- skip array size: args[13]
      misc_info=(array(int))args[14];              // ARRAY Misc-Info
   }

   array encode()
   {
      return
         ({
            @creation_time->encode(),              // Time
            author,                                // Pers-No
            no_of_lines,                           // INT32
            no_of_chars,                           // INT32
            no_of_marks,                           // INT16
            @A((array(string))misc_info),          // ARRAY Misc-Info
         });
   }
}

//!
class TextNumberPair
{
   int              local_number;                  // Local-Text-No
   int              global_number;                 // Text-No

   void create(string|int ... args)
   {
      local_number=(int)args[0];                   // Local-Text-No
      global_number=(int)args[1];                  // Text-No
   }

   array encode()
   {
      return
         ({
            local_number,                          // Local-Text-No
            global_number,                         // Text-No
         });
   }

  string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%d:%d)", this_program,
			     local_number, global_number);
  }
}

//!
class LocalToGlobalBlock
{
  int densep;
  array(TextNumberPair) sparse;
  object /*TextList*/ dense;

  void create(string|array ...args)
  {
    densep=(int)args[0];
    if(densep)
      dense=TextList(@args[1..]);
    else
      sparse=Array.map(args[2..]/2,               // ARRAY Text-Number-Pair
		       lambda(array z) { return TextNumberPair(@z); });
  }
}

//!
class TextMapping
{
   int              range_begin;                   // Local-Text-No
   int              range_end;                     // Local-Text-No
   int(0..1)        later_texts_exists;            // BOOL
   mixed            block;                         // Local-To-Global-Block

   void create(string|int ... args)
   {
      range_begin=(int)args[0];                    // Local-Text-No
      range_end=(int)args[1];                      // Local-Text-No
      later_texts_exists=(int)args[2];             // BOOL
      block=LocalToGlobalBlock(@args[3..6]);       // Local-To-Global-Block
   }

   array encode()
   {
      return
         ({
            range_begin,                           // Local-Text-No
            range_end,                             // Local-Text-No
            later_texts_exists,                    // BOOL
            @error("unimplemented"),               // Local-To-Global-Block
         });
   }
}

constant sessionflagsnames = 
   ({ "invisible", "user_active_used", "user_absent",    
      "reserved3", "reserved4", "reserved5", "reserved6",
      "reserved7"                                         });

multiset(string) SessionFlags(string bits)
{
   multiset(string) res=(<>);
   foreach (Array.transpose( ({sessionflagsnames,values(bits)}) ),
            [string name,int z] )
      res[name]=z-'0';
   return res;
}

//!
class DynamicSessionInfo
{
   int              session;                       // Session-No
   int(0..65535)    person;                        // Pers-No
   int(0..65535)    working_conference;            // Conf-No
   int              idle_time;                     // INT32
   multiset(string) flags;                         // Session-Flags
   string           what_am_i_doing;               // HOLLERITH

   void create(string|int ... args)
   {
      session=(int)args[0];                        // Session-No
      person=(int)args[1];                         // Pers-No
      working_conference=(int)args[2];             // Conf-No
      idle_time=(int)args[3];                      // INT32
      flags=SessionFlags(args[4]);                 // Session-Flags
      what_am_i_doing=args[5];                     // HOLLERITH
   }

   array encode()
   {
      return
         ({
            session,                               // Session-No
            person,                                // Pers-No
            working_conference,                    // Conf-No
            idle_time,                             // INT32
            B(@rows(flags, sessionflagsnames)),    // Session-Flags
            H(what_am_i_doing),                    // HOLLERITH
         });
   }
}

//!
class SessionInfo
{
   int(0..65535)    person;                        // Pers-No
   int(0..65535)    working_conference;            // Conf-No
   int              session;                       // Session-No
   string           what_am_i_doing;               // HOLLERITH
   string           username;                      // HOLLERITH
   int              idle_time;                     // INT32
   LysKOMTime       connection_time;               // Time

   void create(string|int ... args)
   {
      person=(int)args[0];                         // Pers-No
      working_conference=(int)args[1];             // Conf-No
      session=(int)args[2];                        // Session-No
      what_am_i_doing=args[3];                     // HOLLERITH
      username=args[4];                            // HOLLERITH
      idle_time=(int)args[5];                      // INT32
      connection_time=LysKOMTime(@args[6..14]);    // Time
   }

   array encode()
   {
      return
         ({
            person,                                // Pers-No
            working_conference,                    // Conf-No
            session,                               // Session-No
            H(what_am_i_doing),                    // HOLLERITH
            H(username),                           // HOLLERITH
            idle_time,                             // INT32
            @connection_time->encode(),            // Time
         });
   }
}

//!
class WhoInfo
{
   int(0..65535)    person;                        // Pers-No
   int(0..65535)    working_conference;            // Conf-No
   int              session;                       // Session-No
   string           what_am_i_doing;               // HOLLERITH
   string           username;                      // HOLLERITH

   void create(string|int ... args)
   {
      person=(int)args[0];                         // Pers-No
      working_conference=(int)args[1];             // Conf-No
      session=(int)args[2];                        // Session-No
      what_am_i_doing=args[3];                     // HOLLERITH
      username=args[4];                            // HOLLERITH
   }

   array encode()
   {
      return
         ({
            person,                                // Pers-No
            working_conference,                    // Conf-No
            session,                               // Session-No
            H(what_am_i_doing),                    // HOLLERITH
            H(username),                           // HOLLERITH
         });
   }
}

constant personalflagsnames = 
   ({ "unread_is_secret", "flg2", "flg3", "flg4", "flg5",
      "flg6", "flg7", "flg8"                              });

multiset(string) PersonalFlags(string bits)
{
   multiset(string) res=(<>);
   foreach (Array.transpose( ({personalflagsnames,values(bits)}) ),
            [string name,int z] )
      res[name]=z-'0';
   return res;
}

constant auxitemflagsnames = 
   ({ "deleted", "inherit", "secret", "hide_creator",    
      "dont_garb", "reserved2", "reserved3", "reserved4"  });

multiset(string) AuxItemFlags(string bits)
{
   multiset(string) res=(<>);
   foreach (Array.transpose( ({auxitemflagsnames,values(bits)}) ),
            [string name,int z] )
      res[name]=z-'0';
   return res;
}

//!
class AuxItem
{
   int              aux_no;                        // Aux-No
   int              tag;                           // INT32
   int(0..65535)    creator;                       // Pers-No
   LysKOMTime       created_at;                    // Time
   multiset(string) flags;                         // Aux-Item-Flags
   int              inherit_limit;                 // INT32
   string           data;                          // HOLLERITH

   void create(string|int ... args)
   {
      aux_no=(int)args[0];                         // Aux-No
      tag=(int)args[1];                            // INT32
      creator=(int)args[2];                        // Pers-No
      created_at=LysKOMTime(@args[3..11]);         // Time
      flags=AuxItemFlags(args[12]);                // Aux-Item-Flags
      inherit_limit=(int)args[13];                 // INT32
      data=args[14];                               // HOLLERITH
   }

   array encode()
   {
      return
         ({
            aux_no,                                // Aux-No
            tag,                                   // INT32
            creator,                               // Pers-No
            @created_at->encode(),                 // Time
            B(@rows(flags, auxitemflagsnames)),    // Aux-Item-Flags
            inherit_limit,                         // INT32
            H(data),                               // HOLLERITH
         });
   }

  string _sprintf(int t){
    return t=='O' && sprintf("%O(%d)", this_program, tag);
  }
}

//!
class TextStat
{
   LysKOMTime       creation_time;                 // Time
   int(0..65535)    author;                        // Pers-No
   int              no_of_lines;                   // INT32
   int              no_of_chars;                   // INT32
   int(0..65535)    no_of_marks;                   // INT16
   array(int)       misc_info;                     // ARRAY Misc-Info
   array(AuxItem)   aux_items;                     // ARRAY Aux-Item

   void create(string|int|array ... args)
   {
      creation_time=LysKOMTime(@args[0..8]);       // Time
      author=(int)args[9];                         // Pers-No
      no_of_lines=(int)args[10];                   // INT32
      no_of_chars=(int)args[11];                   // INT32
      no_of_marks=(int)args[12];                   // INT16
      // --- skip array size: args[13]
      misc_info=(array(int))args[14];              // ARRAY Misc-Info
      // --- skip array size: args[15]
      aux_items=                                   // ARRAY Aux-Item
         Array.map(args[16]/15,
                   lambda(array z) { return AuxItem(@z); });
   }

   array encode()
   {
      return
         ({
            @creation_time->encode(),              // Time
            author,                                // Pers-No
            no_of_lines,                           // INT32
            no_of_chars,                           // INT32
            no_of_marks,                           // INT16
            @A((array(string))misc_info),          // ARRAY Misc-Info
            A(`+(@Array.map(aux_items,             // ARRAY Aux-Item
                            lambda(AuxItem z) 
                               { return z->encode(); }))),
         });
   }
}

//!
class InfoOld
{
   int              version;                       // INT32
   int(0..65535)    conf_pres_conf;                // Conf-No
   int(0..65535)    pers_pres_conf;                // Conf-No
   int(0..65535)    motd_conf;                     // Conf-No
   int(0..65535)    kom_news_conf;                 // Conf-No
   int              motd_of_lyskom;                // Text-No

   void create(string|int ... args)
   {
      version=(int)args[0];                        // INT32
      conf_pres_conf=(int)args[1];                 // Conf-No
      pers_pres_conf=(int)args[2];                 // Conf-No
      motd_conf=(int)args[3];                      // Conf-No
      kom_news_conf=(int)args[4];                  // Conf-No
      motd_of_lyskom=(int)args[5];                 // Text-No
   }

   array encode()
   {
      return
         ({
            version,                               // INT32
            conf_pres_conf,                        // Conf-No
            pers_pres_conf,                        // Conf-No
            motd_conf,                             // Conf-No
            kom_news_conf,                         // Conf-No
            motd_of_lyskom,                        // Text-No
         });
   }
}

constant membershiptypenames = 
   ({ "invitation", "passive", "secret", "reserved1",    
      "reserved2", "reserved3", "reserved4", "reserved5"  });

multiset(string) MembershipType(string bits)
{
   multiset(string) res=(<>);
   foreach (Array.transpose( ({membershiptypenames,values(bits)}) ),
            [string name,int z] )
      res[name]=z-'0';
   return res;
}

//!
class Membership
{
   int              position;                      // INT32
   LysKOMTime       last_time_read;                // Time
   int(0..65535)    conf_no;                       // Conf-No
   int(0..255)      priority;                      // INT8
   int              last_text_read;                // Local-Text-No
   array(int)       read_texts;                    // ARRAY Local-Text-No
   int(0..65535)    added_by;                      // Pers-No
   LysKOMTime       added_at;                      // Time
   multiset(string) type;                          // Membership-Type

   void create(string|int|array ... args)
   {
      position=(int)args[0];                       // INT32
      last_time_read=LysKOMTime(@args[1..9]);      // Time
      conf_no=(int)args[10];                       // Conf-No
      priority=(int)args[11];                      // INT8
      last_text_read=(int)args[12];                // Local-Text-No
      // --- skip array size: args[13]
      read_texts=(array(int))args[14];             // ARRAY Local-Text-No
      added_by=(int)args[15];                      // Pers-No
      added_at=LysKOMTime(@args[16..24]);          // Time
      type=MembershipType(args[25]);               // Membership-Type
   }

   array encode()
   {
      return
         ({
            position,                              // INT32
            @last_time_read->encode(),             // Time
            conf_no,                               // Conf-No
            priority,                              // INT8
            last_text_read,                        // Local-Text-No
            @A((array(string))read_texts),         // ARRAY Local-Text-No
            added_by,                              // Pers-No
            @added_at->encode(),                   // Time
            B(@rows(type, membershiptypenames)),   // Membership-Type
         });
   }
}

//!
class Member
{
   int(0..65535)    member;                        // Pers-No
   int(0..65535)    added_by;                      // Pers-No
   LysKOMTime       added_at;                      // Time
   multiset(string) type;                          // Membership-Type

   void create(string|int ... args)
   {
      member=(int)args[0];                         // Pers-No
      added_by=(int)args[1];                       // Pers-No
      added_at=LysKOMTime(@args[2..10]);           // Time
      type=MembershipType(args[11]);               // Membership-Type
   }

   array encode()
   {
      return
         ({
            member,                                // Pers-No
            added_by,                              // Pers-No
            @added_at->encode(),                   // Time
            B(@rows(type, membershiptypenames)),   // Membership-Type
         });
   }
}

//!
class MembershipOld
{
   LysKOMTime       last_time_read;                // Time
   int(0..65535)    conf_no;                    // Conf-No
   int(0..255)      priority;                      // INT8
   int              last_text_read;                // Local-Text-No
   array(int)       read_texts;                    // ARRAY Local-Text-No

   void create(string|int|array ... args)
   {
      last_time_read=LysKOMTime(@args[0..8]);      // Time
      conf_no=(int)args[9];                     // Conf-No
      priority=(int)args[10];                      // INT8
      last_text_read=(int)args[11];                // Local-Text-No
      // --- skip array size: args[12]
      read_texts=(array(int))args[13];             // ARRAY Local-Text-No
   }

   array encode()
   {
      return
         ({
            @last_time_read->encode(),             // Time
            conf_no,                            // Conf-No
            priority,                              // INT8
            last_text_read,                        // Local-Text-No
            @A((array(string))read_texts),         // ARRAY Local-Text-No
         });
   }
}

//!
class Info
{
   int              version;                       // INT32
   int(0..65535)    conf_pres_conf;                // Conf-No
   int(0..65535)    pers_pres_conf;                // Conf-No
   int(0..65535)    motd_conf;                     // Conf-No
   int(0..65535)    kom_news_conf;                 // Conf-No
   int              motd_of_lyskom;                // Text-No
   array(AuxItem)   aux_item_list;                 // ARRAY Aux-Item

   void create(string|int|array ... args)
   {
      version=(int)args[0];                        // INT32
      conf_pres_conf=(int)args[1];                 // Conf-No
      pers_pres_conf=(int)args[2];                 // Conf-No
      motd_conf=(int)args[3];                      // Conf-No
      kom_news_conf=(int)args[4];                  // Conf-No
      motd_of_lyskom=(int)args[5];                 // Text-No
      // --- skip array size: args[6]
      aux_item_list=                               // ARRAY Aux-Item
         Array.map(args[7]/15,
                   lambda(array z) { return AuxItem(@z); });
   }

   array encode()
   {
      return
         ({
            version,                               // INT32
            conf_pres_conf,                        // Conf-No
            pers_pres_conf,                        // Conf-No
            motd_conf,                             // Conf-No
            kom_news_conf,                         // Conf-No
            motd_of_lyskom,                        // Text-No
            A(`+(@Array.map(aux_item_list,         // ARRAY Aux-Item
                            lambda(AuxItem z) 
                               { return z->encode(); }))),
         });
   }
}

//!
class StaticSessionInfo
{
   string           username;                      // HOLLERITH
   string           hostname;                      // HOLLERITH
   string           ident_user;                    // HOLLERITH
   LysKOMTime       connection_time;               // Time

   void create(string|int ... args)
   {
      username=args[0];                            // HOLLERITH
      hostname=args[1];                            // HOLLERITH
      ident_user=args[2];                          // HOLLERITH
      connection_time=LysKOMTime(@args[3..11]);    // Time
   }

   array encode()
   {
      return
         ({
            H(username),                           // HOLLERITH
            H(hostname),                           // HOLLERITH
            H(ident_user),                         // HOLLERITH
            @connection_time->encode(),            // Time
         });
   }
}

constant extendedconftypenames = 
   ({ "rd_prot", "original", "secret", "letterbox",      
      "allow_anonymous", "forbid_secret", "reserved2",   
      "reserved3"                                         });

multiset(string) ExtendedConfType(string bits)
{
   multiset(string) res=(<>);
   foreach (Array.transpose( ({extendedconftypenames,values(bits)}) ),
            [string name,int z] )
      res[name]=z-'0';
   return res;
}

//!
class TextList
{
   int              first_local_no;                // Local-Text-No
   array(int)       texts;                         // ARRAY Text-No

   void create(string|int|array ... args)
   {
      first_local_no=(int)args[0];                 // Local-Text-No
      // --- skip array size: args[1]
      texts=(array(int))args[2];                   // ARRAY Text-No
   }

   array encode()
   {
      return
         ({
            first_local_no,                        // Local-Text-No
            @A((array(string))texts),              // ARRAY Text-No
         });
   }
}

//!
class AuxItemInput
{
   int              tag;                           // INT32
   multiset(string) flags;                         // Aux-Item-Flags
   int              inherit_limit;                 // INT32
   string           data;                          // HOLLERITH

   void create(string|int ... args)
   {
      tag=(int)args[0];                            // INT32
      flags=AuxItemFlags(args[1]);                 // Aux-Item-Flags
      inherit_limit=(int)args[2];                  // INT32
      data=args[3];                                // HOLLERITH
   }

   array encode()
   {
      return
         ({
            tag,                                   // INT32
            B(@rows(flags, auxitemflagsnames)),    // Aux-Item-Flags
            inherit_limit,                         // INT32
            H(data),                               // HOLLERITH
         });
   }
}

//!
class WhoInfoIdent
{
   int(0..65535)    person;                        // Pers-No
   int(0..65535)    working_conference;            // Conf-No
   int              session;                       // Session-No
   string           what_am_i_doing;               // HOLLERITH
   string           username;                      // HOLLERITH
   string           hostname;                      // HOLLERITH
   string           ident_user;                    // HOLLERITH

   void create(string|int ... args)
   {
      person=(int)args[0];                         // Pers-No
      working_conference=(int)args[1];             // Conf-No
      session=(int)args[2];                        // Session-No
      what_am_i_doing=args[3];                     // HOLLERITH
      username=args[4];                            // HOLLERITH
      hostname=args[5];                            // HOLLERITH
      ident_user=args[6];                          // HOLLERITH
   }

   array encode()
   {
      return
         ({
            person,                                // Pers-No
            working_conference,                    // Conf-No
            session,                               // Session-No
            H(what_am_i_doing),                    // HOLLERITH
            H(username),                           // HOLLERITH
            H(hostname),                           // HOLLERITH
            H(ident_user),                         // HOLLERITH
         });
   }
}

constant conftypenames = 
   ({ "rd_prot", "original", "secret", "letterbox"        });

multiset(string) ConfType(string bits)
{
   multiset(string) res=(<>);
   foreach (Array.transpose( ({conftypenames,values(bits)}) ),
            [string name,int z] )
      res[name]=z-'0';
   return res;
}

//!
class ConfZInfo
{
   string           name;                          // HOLLERITH
   multiset(string) type;                          // Conf-Type
   int(0..65535)    conf_no;                       // Conf-No

   void create(string|int ... args)
   {
      name=args[0];                                // HOLLERITH
      type=ConfType(args[1]);                      // Conf-Type
      conf_no=(int)args[2];                        // Conf-No
   }

   array encode()
   {
      return
         ({
            H(name),                               // HOLLERITH
            B(@rows(type, conftypenames)),         // Conf-Type
            conf_no,                               // Conf-No
         });
   }
}

//!
class WhoInfoOld
{
   int(0..65535)    person;                        // Pers-No
   int(0..65535)    working_conference;            // Conf-No
   string           what_am_i_doing;               // HOLLERITH

   void create(string|int ... args)
   {
      person=(int)args[0];                         // Pers-No
      working_conference=(int)args[1];             // Conf-No
      what_am_i_doing=args[2];                     // HOLLERITH
   }

   array encode()
   {
      return
         ({
            person,                                // Pers-No
            working_conference,                    // Conf-No
            H(what_am_i_doing),                    // HOLLERITH
         });
   }
}

//!
class Mark
{
   int              text_no;                       // Text-No
   int(0..255)      type;                          // INT8

   void create(string|int ... args)
   {
      text_no=(int)args[0];                        // Text-No
      type=(int)args[1];                           // INT8
   }

   array encode()
   {
      return
         ({
            text_no,                               // Text-No
            type,                                  // INT8
         });
   }

  string _sprintf(int t){
    return t=='O' && sprintf("%O(%d, text %d)", this_program, text_no, type);
  }
}

//!
class Conference
{
   string           name;                          // HOLLERITH
   multiset(string) type;                          // Extended-Conf-Type
   LysKOMTime       creation_time;                 // Time
   LysKOMTime       last_written;                  // Time
   int(0..65535)    creator;                       // Pers-No
   int              presentation;                  // Text-No
   int(0..65535)    supervisor;                    // Conf-No
   int(0..65535)    permitted_submitters;          // Conf-No
   int(0..65535)    super_conf;                    // Conf-No
   int              msg_of_day;                    // Text-No
   int              nice;                          // Garb-Nice
   int              keep_commented;                // Garb-Nice
   int(0..65535)    no_of_members;                 // INT16
   int              first_local_no;                // Local-Text-No
   int              no_of_texts;                   // INT32
   int              expire;                        // Garb-Nice
   array(AuxItem)   aux_items;                     // ARRAY Aux-Item

   void create(string|int|array ... args)
   {
      name=args[0];                                // HOLLERITH
      type=ExtendedConfType(args[1]);              // Extended-Conf-Type
      creation_time=LysKOMTime(@args[2..10]);      // Time
      last_written=LysKOMTime(@args[11..19]);      // Time
      creator=(int)args[20];                       // Pers-No
      presentation=(int)args[21];                  // Text-No
      supervisor=(int)args[22];                    // Conf-No
      permitted_submitters=(int)args[23];          // Conf-No
      super_conf=(int)args[24];                    // Conf-No
      msg_of_day=(int)args[25];                    // Text-No
      nice=(int)args[26];                          // Garb-Nice
      keep_commented=(int)args[27];                // Garb-Nice
      no_of_members=(int)args[28];                 // INT16
      first_local_no=(int)args[29];                // Local-Text-No
      no_of_texts=(int)args[30];                   // INT32
      expire=(int)args[31];                        // Garb-Nice
      // --- skip array size: args[32]
      aux_items=                                   // ARRAY Aux-Item
         Array.map(args[33]/15,
                   lambda(array z) { return AuxItem(@z); });
   }

   array encode()
   {
      return
         ({
            H(name),                               // HOLLERITH
            B(@rows(type, extendedconftypenames)), // Extended-Conf-Type
            @creation_time->encode(),              // Time
            @last_written->encode(),               // Time
            creator,                               // Pers-No
            presentation,                          // Text-No
            supervisor,                            // Conf-No
            permitted_submitters,                  // Conf-No
            super_conf,                            // Conf-No
            msg_of_day,                            // Text-No
            nice,                                  // Garb-Nice
            keep_commented,                        // Garb-Nice
            no_of_members,                         // INT16
            first_local_no,                        // Local-Text-No
            no_of_texts,                           // INT32
            expire,                                // Garb-Nice
            A(`+(@Array.map(aux_items,             // ARRAY Aux-Item
                            lambda(AuxItem z) 
                               { return z->encode(); }))),
         });
   }

  string _sprintf(int t){
    return t=='O' && sprintf("%O(%s)", this_program, name);
  }
}

//!
class ConferenceOld
{
   string           name;                          // HOLLERITH
   multiset(string) type;                          // Conf-Type
   LysKOMTime       creation_time;                 // Time
   LysKOMTime       last_written;                  // Time
   int(0..65535)    creator;                       // Pers-No
   int              presentation;                  // Text-No
   int(0..65535)    supervisor;                    // Conf-No
   int(0..65535)    permitted_submitters;          // Conf-No
   int(0..65535)    super_conf;                    // Conf-No
   int              msg_of_day;                    // Text-No
   int              nice;                          // Garb-Nice
   int(0..65535)    no_of_members;                 // INT16
   int              first_local_no;                // Local-Text-No
   int              no_of_texts;                   // INT32

   void create(string|int ... args)
   {
      name=args[0];                                // HOLLERITH
      type=ConfType(args[1]);                      // Conf-Type
      creation_time=LysKOMTime(@args[2..10]);      // Time
      last_written=LysKOMTime(@args[11..19]);      // Time
      creator=(int)args[20];                       // Pers-No
      presentation=(int)args[21];                  // Text-No
      supervisor=(int)args[22];                    // Conf-No
      permitted_submitters=(int)args[23];          // Conf-No
      super_conf=(int)args[24];                    // Conf-No
      msg_of_day=(int)args[25];                    // Text-No
      nice=(int)args[26];                          // Garb-Nice
      no_of_members=(int)args[27];                 // INT16
      first_local_no=(int)args[28];                // Local-Text-No
      no_of_texts=(int)args[29];                   // INT32
   }

   array encode()
   {
      return
         ({
            H(name),                               // HOLLERITH
            B(@rows(type, conftypenames)),         // Conf-Type
            @creation_time->encode(),              // Time
            @last_written->encode(),               // Time
            creator,                               // Pers-No
            presentation,                          // Text-No
            supervisor,                            // Conf-No
            permitted_submitters,                  // Conf-No
            super_conf,                            // Conf-No
            msg_of_day,                            // Text-No
            nice,                                  // Garb-Nice
            no_of_members,                         // INT16
            first_local_no,                        // Local-Text-No
            no_of_texts,                           // INT32
         });
   }

  string _sprintf(int t){
    return t=='O' && sprintf("%O(%s)", this_program, name);
  }
}

//!
class SessionInfoIdent
{
   int(0..65535)    person;                        // Pers-No
   int(0..65535)    working_conference;            // Conf-No
   int              session;                       // Session-No
   string           what_am_i_doing;               // HOLLERITH
   string           username;                      // HOLLERITH
   string           hostname;                      // HOLLERITH
   string           ident_user;                    // HOLLERITH
   int              idle_time;                     // INT32
   LysKOMTime       connection_time;               // Time

   void create(string|int ... args)
   {
      person=(int)args[0];                         // Pers-No
      working_conference=(int)args[1];             // Conf-No
      session=(int)args[2];                        // Session-No
      what_am_i_doing=args[3];                     // HOLLERITH
      username=args[4];                            // HOLLERITH
      hostname=args[5];                            // HOLLERITH
      ident_user=args[6];                          // HOLLERITH
      idle_time=(int)args[7];                      // INT32
      connection_time=LysKOMTime(@args[8..16]);    // Time
   }

   array encode()
   {
      return
         ({
            person,                                // Pers-No
            working_conference,                    // Conf-No
            session,                               // Session-No
            H(what_am_i_doing),                    // HOLLERITH
            H(username),                           // HOLLERITH
            H(hostname),                           // HOLLERITH
            H(ident_user),                         // HOLLERITH
            idle_time,                             // INT32
            @connection_time->encode(),            // Time
         });
   }
}

//!
class VersionInfo
{
   int              protocol_version;              // INT32
   string           server_software;               // HOLLERITH
   string           software_version;              // HOLLERITH

   void create(string|int ... args)
   {
      protocol_version=(int)args[0];               // INT32
      server_software=args[1];                     // HOLLERITH
      software_version=args[2];                    // HOLLERITH
   }

   array encode()
   {
      return
         ({
            protocol_version,                      // INT32
            H(server_software),                    // HOLLERITH
            H(software_version),                   // HOLLERITH
         });
   }


  string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%s version %s; protocol %d)",
			     this_program, server_software,
			     software_version, protocol_version);
  }
}

//!
class ConfListArchaic
{
   array(int(0..65535)) conf_nos;                  // ARRAY Conf-No
   array(multiset(string)) conf_types;             // ARRAY Conf-Type

   void create(string|array ...args)
   {
      // --- skip array size: args[0]
      conf_nos=(array(int))args[1];                // ARRAY Conf-No
      // --- skip array size: args[2]
      conf_types=                                  // ARRAY Conf-Type
         Array.map(args[3],
                   lambda(string z) { return ConfType(z); });
   }

   array encode()
   {
      return
         ({
            @A((array(string))conf_nos),           // ARRAY Conf-No
            A(Array.map(conf_types,                // ARRAY Conf-Type
			lambda(multiset(string) z) 
			{ return B(@rows(z,conftypenames)); })),
         });
   }
}

constant privbitsnames = 
   ({ "wheel", "admin", "statistic", "create_pers",      
      "create_conf", "change_name", "flg7", "flg8",      
      "flg9", "flg10", "flg11", "flg12", "flg13",        
      "flg14", "flg15", "flg16"                           });

multiset(string) PrivBits(string bits)
{
   multiset(string) res=(<>);
   foreach (Array.transpose( ({privbitsnames,values(bits)}) ),
            [string name,int z] )
      res[name]=z-'0';
   return res;
}

//!
class UConference
{
   string           name;                          // HOLLERITH
   multiset(string) type;                          // Extended-Conf-Type
   int              highest_local_no;              // Local-Text-No
   int              nice;                          // Garb-Nice

   void create(string|int ... args)
   {
      name=args[0];                                // HOLLERITH
      type=ExtendedConfType(args[1]);              // Extended-Conf-Type
      highest_local_no=(int)args[2];               // Local-Text-No
      nice=(int)args[3];                           // Garb-Nice
   }

   array encode()
   {
      return
         ({
            H(name),                               // HOLLERITH
            B(@rows(type, extendedconftypenames)), // Extended-Conf-Type
            highest_local_no,                      // Local-Text-No
            nice,                                  // Garb-Nice
         });
   }
}

//!
class Person
{
   string           username;                      // HOLLERITH
   multiset(string) privileges;                    // Priv-Bits
   multiset(string) flags;                         // Personal-Flags
   LysKOMTime       last_login;                    // Time
   int              user_area;                     // Text-No
   int              total_time_present;            // INT32
   int              sessions;                      // INT32
   int              created_lines;                 // INT32
   int              created_bytes;                 // INT32
   int              read_texts;                    // INT32
   int              no_of_text_fetches;            // INT32
   int(0..65535)    created_persons;               // INT16
   int(0..65535)    created_confs;                 // INT16
   int              first_created_local_no;        // INT32
   int              no_of_created_texts;           // INT32
   int(0..65535)    no_of_marks;                   // INT16
   int(0..65535)    no_of_confs;                   // INT16

   void create(string|int ... args)
   {
      username=args[0];                            // HOLLERITH
      privileges=PrivBits(args[1]);                // Priv-Bits
      flags=PersonalFlags(args[2]);                // Personal-Flags
      last_login=LysKOMTime(@args[3..11]);         // Time
      user_area=(int)args[12];                     // Text-No
      total_time_present=(int)args[13];            // INT32
      sessions=(int)args[14];                      // INT32
      created_lines=(int)args[15];                 // INT32
      created_bytes=(int)args[16];                 // INT32
      read_texts=(int)args[17];                    // INT32
      no_of_text_fetches=(int)args[18];            // INT32
      created_persons=(int)args[19];               // INT16
      created_confs=(int)args[20];                 // INT16
      first_created_local_no=(int)args[21];        // INT32
      no_of_created_texts=(int)args[22];           // INT32
      no_of_marks=(int)args[23];                   // INT16
      no_of_confs=(int)args[24];                   // INT16
   }

   array encode()
   {
      return
         ({
            H(username),                           // HOLLERITH
            B(@rows(privileges, privbitsnames)),   // Priv-Bits
            B(@rows(flags, personalflagsnames)),   // Personal-Flags
            @last_login->encode(),                 // Time
            user_area,                             // Text-No
            total_time_present,                    // INT32
            sessions,                              // INT32
            created_lines,                         // INT32
            created_bytes,                         // INT32
            read_texts,                            // INT32
            no_of_text_fetches,                    // INT32
            created_persons,                       // INT16
            created_confs,                         // INT16
            first_created_local_no,                // INT32
            no_of_created_texts,                   // INT32
            no_of_marks,                           // INT16
            no_of_confs,                           // INT16
         });
   }

  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s)", this_program, username);
  }
}
