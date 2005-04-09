#pike __REAL_VERSION__

constant CONNECTION_CLOSED=-100;

string encode(mixed ... z) // encode arguments
{
   return ((array(string))z)*" ";
}

string H(string what) // encode hollerith
{
   return sizeof(what)+"H"+what;
}

string B(int(0..1) ... z) // encode bitfield
{
   string res="";
   res=((array(string))z)*"";
   return res;
}

array(string) A(array z) // encode array 
{
   return ({ ""+sizeof(z), "{ "+encode(@Array.flatten(z))+" }" });
}

class LysKOMError
{
   constant iserror=1;
   constant is_generic_error=1;

   int no;
   string name;
   string desc;
   int status;
   array(mixed) __backtrace;

   void create(int _no,string _name,string _desc,void|int _status)
   {
      no=_no;
      name=_name;
      desc=_desc;
      status=_status;
      __backtrace=backtrace();
      __backtrace=__backtrace[..sizeof(__backtrace)-3];
   }

   LysKOMError clone(int status)
   {
      return LysKOMError(no,name,desc,status);
   }
   
   mixed `[](mixed z)
   {
      switch (z)
      {
	 case 0: return "Protocols.LysKOM: ["+no+"] "+name+"\n";
	 case 1: return __backtrace;
      }
      return ::`[](z);
   }
}

LysKOMError lyskom_error(int no,void|int status)
{
   return LysKOMError(@_lyskomerror[no],status);
}

mapping _lyskomerror=
([
   0:({0,"no-error",
       "No error has occurred. This should never "
       "happen, but it might."}),

   2:({2,"not-implemented",
       "The call has not been implemented yet."}),

   3:({3,"obsolete-call",
       "The call is obsolete and no longer "
       "implemented. Status value is "
       "undefined."}),

   4:({4,"invalid-password",
       "Attempt to set a password containing "
       "illegal characters, or to use an "
       "incorrect password."}),

   5:({5,"string-too-long",
       "A string was too long (see descriptions "
       "of each call.) Status value indicates "
       "the maximum string length."}),

       6:({6,"login-first",
	   "Login is required before issuing the "
	   "call. Status value is undefined. "}),

   7:({7,"login-disallowed",
       "The system is in single-user mode. You "
       "need to be privileged to log in despite "
       "this. "}),

   8:({8,"conference-zero",
       "Attempt to use conference number 0. "}),

   9:({9,"undefined-conference",
       "Attempt to access a non-existent or "
       "secret conference. Status value contains "
       "the conference number in question."}),

   10:({10,"undefined-person",
	"Attempt to access a non-existent or"
	"secret person. Status value contains the"
	"person number in question."}),

   11:({11,"access-denied",
	"No read/write access to something. This"
	"might be returned in response to an"
	"attempt to create a text, when the"
	"recipient conference and its super"
	"conferences are read-only, or when"
	"attempting to add a member to a"
	"conference without enough permission to"
	"do so. Status value indicates the object"
	"to which we didn't have enough"
	"permissions to."}),

   12:({12,"permission-denied",
	"Not enough permissions to do something."
	"The exact meaning of this response"
	"depends on the call. Status value"
	"indicated the object for which"
	"permission was lacking, or zero."}),

   13:({13,"not-member",
	"The call requires the caller to be a"
	"member of some conference that the"
	"caller is not a member of. Status value"
	"indicates the conference in question."}),

   14:({14,"no-such-text",
	"Attempt to access a text that either"
	"does not exist or is secret in some way."
	"status value indicates the text number"
	"in question."}),

   15:({15,"text-zero",
	"Attempt to use text number 0. "}),

   16:({16,"no-such-local-text",
	"Attempt to access a text using a local"
	"text number that does not represent an"
	"existing text. Status value indicates"
	"the offending number."}),

   17:({17,"local-text-zero",
	"Attempt to use local text number zero."}),

   18:({18,"bad-name",
	"Attempt to use a name that's too long,"
	"too short or contains invalid"
	"characters. "}),

   19:({19,"index-out-of-range",
	"Attempt to use a number that's out of"
	"range. The range and meaning of the"
	"numbers depends on the call issued."
	"status value is undefined."}),

   20:({20,"conference-exists",
	"Attempt to create a conference or person"
	"with a name that's already occupied. "}),

   21:({21,"person-exists",
	"Attempt to create a person with a name"
	"that's already occupied. This error code"
	"is probably not used, but you never know"
	"for sure."}),

   22:({22,"secret-public",
	"Attempt to give a conference a type with"
	"@code{secret} bit set and the"
	"@code{rd-prot} bit unset. This is an"
	"error since such a conference type is"
	"inconsistent. "}),

   23:({23,"letterbox",
	"Attempt to change the @code{letterbox}"
	"flag of a conference. Status value"
	"indicates the conference number."}),

   24:({24,"ldb-error",
	"Database is corrupted. Status value is"
	"an internal code."}),

   25:({25,"illegal-misc",
	"Attempt to create an illegal misc item."
	"status value contains the index of the"
	"illegal item."}),

   26:({26,"illegal-info-type",
	"Attempt to use a Misc-Info type that the"
	"server knows nothing about. "}),

   27:({27,"already-recipient",
	"Attempt to add a recipient that is"
	"already a recipient of the same type."
	"status value contains the recipient that"
	"already is."}),

   28:({28,"already-comment",
	"Attempt to add a comment to a text twice"
	"over. Status value contains the text"
	"number of the text that already is a"
	"comment."}),

   29:({29,"already-footnote",
	"Attempt to add a footnote to a text"
	"twice over. Status value contains the"
	"text number of the text that already is"
	"a footnote."}),

   30:({30,"not-recipient",
	"Attempt to remove a recipient that isn't"
	"really a recipient. Status value"
	"contains the conference number in"
	"question."}),

   31:({31,"not-comment",
	"Attempt to remove a comment link that"
	"does not exist. Status value contains"
	"the text number that isn't a comment. "}),

   32:({32,"not-footnote",
	"Attempt to remove a footnote link that"
	"does not exist. Status value contains"
	"the text number that isn't a footnote."}),

   33:({33,"recipient-limit",
	"Attempt to add a recipient to a text"
	"that already has the maximum number of"
	"recipients. Status value is the text"
	"that has the maximum number of"
	"recipients."}),

   34:({34,"comment-limit",
	"Attempt to add a comment to a text that"
	"already has the maximum number of"
	"comments. Status value is the text with"
	"the maximum number of comments. "}),

   35:({35,"footnote-limit",
	"Attempt to add a footnote to a text that"
	"already has the maximum number of"
	"footnote. Status value is the text with"
	"the maximum number of footnote. "}),

   36:({36,"mark-limit",
	"Attempt to add a mark to a text that"
	"already has the maximum number of marks."
	"status value is the text with the"
	"maximum number of marks. "}),

   37:({37,"not-author",
	"Attempt to manipulate a text in a way"
	"that required the user to be the author"
	"of the text, when not in fact the"
	"author. Status value contains the text"
	"number in question."}),

   38:({38,"no-connect",
	"Currently unused."}),

   39:({39,"out-of-memory",
	"The server ran out of memory."}),

   40:({40,"server-is-crazy",
	"Currently unused."}),

   41:({41,"client-is-crazy",
	"Currently unused."}),

   42:({42,"undefined-session",
	"Attempt to access a session that does"
	"not exist. Status value contains the"
	"offending session number."}),

   43:({43,"regexp-error",
	"Error using a regexp. The regexp may be"
	"invalid or the server unable to compile"
	"it for other reasons. "}),

   44:({44,"not-marked",
	"Attempt to manipulate a text in a way"
	"that requires the text to be marked,"
	"when in fact it is not marked. Status"
	"value indicates the text in question."}),

   45:({45,"temporary-failure",
	"Temporary failure. Try again later."
	"@code{error-code} is undefined."}),

   46:({46,"long-array",
	"An array sent to the server was too"
	"long. Status value is undefined."}),

   47:({47,"anonymous-rejected",
	"Attempt to send an anonymous text to a"
	"conference that does not accept"
	"anonymous texts. "}),

   48:({48,"illegal-aux-item",
	"Attempt to create an invalid aux-item."
	"Probably the tag or data are invalid."
	"status value contains the index in the"
	"aux-item list where the invalid item"
	"appears."}),

   49:({49,"aux-item-permission",
	"Attempt to manipulate an aux-item"
	"without enough permissions. This"
	"response is sent when attempting to"
	"delete an item set by someone else or an"
	"item that can't be deleted, and when"
	"attempting to create an item without"
	"permissions to do so. Status value"
	"contains the index at which the item"
	"appears in the aux-item list sent to the"
	"server."}),

   50:({50,"unknown-async",
	"Sent in response to a request for an"
	"asynchronous message the server does not"
	"send. The call succeeds, but this is"
	"sent as a warning to the client. Status"
	"value contains the message type the"
	"server did not understand."}),

   51:({51,"internal-error",
	"The server has encountered a possibly"
	"recoverable internal error. "}),

   52:({52,"feature-disabled",
	"Attempt to use a feature that has been"
	"explicitly disabled in the server. "}),

   53:({53,"message-not-sent",
	"Attempt to send an asynchronous message"
	"failed for some reason. Perhaps the"
	"recipient is not accepting messages at"
	"the moment."}),

   54:({54,"invalid-membership-type",
	"A requested membership type was not"
	"compatible with restrictions set on the"
	"server or on a specific conference."
	"status value is undefined unless"
	"specifically mentioned in the"
	"documentation for a specific call."}),

   CONNECTION_CLOSED:({CONNECTION_CLOSED,"connection closed",
		       "LysKOM module: connection has been closed"})
]);
