#pike __REAL_VERSION__
inherit Parser._RCS;

// $Id: RCS.pike,v 1.34 2004/11/08 15:12:21 per Exp $

//! A RCS file parser that eats a RCS *,v file and presents nice pike
//! data structures of its contents.

#ifdef PARSER_RCS_DEBUG
#define DEBUG(X, Y ...) werror("Parser.RCS: " + X, Y)
#else
#define DEBUG(X, Y ...)
#endif

//! Version number of the head version of the file
string head;

//! The default branch (or revision), if present, @expr{0@} otherwise
string|int(0..0) branch;

//! The usernames listed in the ACCESS section of the RCS file
array(string) access;

//! The RCS file comment if present, @expr{0@} otherwise
string|int(0..0) comment;

//! The keyword expansion options (as named by RCS) if present,
//! @expr{0@} otherwise
string expand;

//! The RCS file description
string description;

//! Maps from username to revision for users that have acquired locks
//! on this file
mapping(string:string) locks;	 // id:num

//! 1 if strict locking is set, 0 otherwise
int(0..1) strict_locks;

//! Maps tag names (indices) to tagged revision numbers (values)
mapping(string:string) tags;

//! Maps branch numbers (indices) to branch names (values)
mapping(string:string) branches;

string _sprintf(int type)
{
  return type=='O' && sprintf("%O(/* %d revisions */)", this_program,
			      revisions && sizeof(revisions));
}

//! Data for all revisions of the file. The indices of the mapping are
//! the revision numbers, whereas the values are the data from the
//! corresponding revision.
mapping(string:Revision) revisions;

//! Data for all revisions on the trunk, sorted in the same order as the
//! RCS file stored them - ie descending, most recent first, I'd assume
//! (rcsfile(5), of course, fails to state such irrelevant information).
array(mapping) trunk = ({});

static mapping parse_mapping(array data)
{
     return (mapping)(data/2);
}

static string parse_string(string data, string|void leader)
{
    return replace( data, "@@", "@" );
}

function symbol_is_branch = Regexp("\\.0\\.[0-9]*[02468]$")->match;
function symbol_is_revision = Regexp("^[0-9.]*$")->match;

string rcs_file_name;

//! Initializes the RCS object.
//! @param file_name
//!   The path to the raw RCS file (includes trailing ",v"). Used
//!   mainly for error reporting (truncated RCS file).
//! @param file_contents
//!   If a string is provided, that string will be parsed to
//!   initialize the RCS object. If a zero (@expr{0@}) is sent, no
//!   initialization will be performed at all. If no value is given at
//!   all, but @[file_name] was provided, that file will be loaded and
//!   parsed for object initialization.
void create(string|void file_name, string|int(0..0)|void file_contents)
{
  if(!file_name)
  {
    if(!file_contents)
      return;
  }
  else
  {
    rcs_file_name = file_name;
    if(!zero_type(file_contents) && !file_contents)
      return;
    if(!file_contents)
      file_contents = Stdio.read_file(file_name);
    if(!file_contents)
      error("Couldn't read %s\n", file_name);
  }
  parse(tokenize(file_contents));
}

//! Lower-level API function for parsing only the admin section (the
//! initial chunk of an RCS file, see manpage rcsfile(5)) of an RCS
//! file. After running @[parse_admin_section], the RCS object will be
//! initialized with the values for @[head], @[branch], @[access],
//! @[branches], @[tokenize], @[tags], @[locks], @[strict_locks],
//! @[comment] and @[expand].
//! @param raw
//!   The tokenized RCS file, or the raw RCS-file data.
//! @returns
//!   The rest of the RCS file, admin section removed.
//! @seealso
//!   @[parse_delta_sections], @[parse_deltatext_sections], @[parse], @[create]
//! @fixme
//!   Does not handle rcsfile(5) newphrase skipping.
array parse_admin_section(string|array raw)
{
    if( stringp( raw ) )
	raw = tokenize(raw);
  int i;
  branches = ([]);
loop:
  for( i=0; i<sizeof(raw); i++ )
  {
      switch( raw[i][0] )
      {
	  case "head":
	      head = raw[i][1..1]*"";
	      break;
	  case "access":
	      access = raw[i][1..];
	      break;
	  case "branch":
	      branch = raw[i][1..1]*"";
	      break;
	  case "symbols":
	      tags = parse_mapping( raw[i][1..] );
	      foreach(tags; string name; string revision ) 
	      {
		  if(revision == "1.1.1")
		      branches[revision] = name; // the vendor branch
		  else if(symbol_is_branch(revision))
		  {
		      array(string) nums = revision / "."; // "a.b.c.d.0.e"
		      revision = nums[..sizeof(nums)-3] * "." + "." + nums[-1]; // "a.b.c.d.e"
		      branches[revision] = name;
		  }
		  else
		      tags[name] = revision;
	      }
	      break;
	  case "locks":
	      locks = parse_mapping( raw[i][1..] );
	      break;
	  case "strict":
	      strict_locks=1;
	      break;
	  case "expand":
	      expand = parse_string( raw[i][1] );
	      break;
	  case "comment": // EOL.
	      comment = parse_string( raw[i][1] );
	      break;
	  default:
	      if( symbol_is_revision( raw[i][0] ) )
		  break loop;
	      break;
      }
  }
  return raw[i..];
}

//! Lower-level API function for parsing only the delta sections (the
//! second chunk of an RCS file, see manpage rcsfile(5)) of an RCS
//! file. After running @[parse_delta_sections], the RCS object will
//! be initialized with the value of @[description] and populated
//! @[revisions] mapping and @[trunk] array. Their @[Revision] members
//! are however only populated with the members @[Revision->revision],
//! @[Revision->branch], @[Revision->time], @[Revision->author],
//! @[Revision->state], @[Revision->branches], @[Revision->rcs_next],
//! @[Revision->ancestor] and @[Revision->next].
//! @param raw
//!   The tokenized RCS file, with admin section removed. (See
//!   @[parse_admin_section].)
//! @returns
//!   The rest of the RCS file, delta sections removed.
//! @seealso
//!   @[parse_admin_section], @[tokenize], @[parse_deltatext_sections],
//!   @[parse], @[create]
//! @fixme
//!   Does not handle rcsfile(5) newphrase skipping.
array parse_delta_sections(array raw)
{
  array got;
  string revision, ptr;
  revisions = ([]);
  
  int i;
  Revision R;
loop:
  for( i = 0; i<sizeof(raw); i++ )
  {
      switch( raw[i][0] )
      {
	  case "state":
	      if( sizeof( raw[i] ) > 1 )
		  R->state = raw[i][1];
	      break;
	  case "author":
	      if( sizeof( raw[i] ) > 1 )
		  R->author = raw[i][1];
	      break;
	  case "branches":
	      R->branches = raw[i][1..];
	      break;
	  case "next":
	      if( sizeof( raw[i] ) > 1 )
		  R->rcs_next = raw[i][1];
	      break;
	  case "desc":
	      // finito
	      break loop;

	  default:
	      if( sizeof(raw[i])>2 && raw[i][1] == "date" )
	      {
		  R = Revision();
		  R->revision = revision = raw[i][0];
		  if( String.count( revision, "." ) == 1)
		      trunk += ({ R });
		  else
		  {
		      sscanf(reverse(revision), "%*d.%s", string branch);
		      R->branch = branches[reverse(branch)];
		  }
		  string date = raw[i][2];
		  if(!(int)date) // RCS dates are "YY.*" for 1900<=year<2000 - compensate for
		      date = "1900" + date[search(date, ".")..]; // Calendar.parse(%y, 0)==2000
		  R->time = Calendar.ISO.parse("%y.%M.%D.%h.%m.%s %z", date + " UTC");
		  revisions[R->revision] = R;
	      }
	      break;
      }
  }

  // finally, set all next/ancestor pointers:
  foreach(values(revisions), Revision R)
  {
    if(ptr = R->rcs_next)
    {
      if(String.count(R->revision, ".") > 1)
      {
	R->next = ptr; // on a branch, the next pointer means the successor
	revisions[ptr]->ancestor = R->revision;
      }
      else // current revision is on the trunk:
      {
	R->ancestor = ptr; // on the trunk, the next pointer is the ancestor
	revisions[ptr]->next = R->revision;
      }
    }
    foreach(R->branches, string branch_point)
      revisions[branch_point]->ancestor = R->revision;
  }
  return raw[i][2..];
}

//! @decl array(array(string)) tokenize( string data )
//! Tokenize an RCS file into tokens suitable as argument to the various
//! parse functions
//! @param data
//!    The RCS file data
//! @returns
//!    An array with arrays of tokens


//! Lower-level API function for parsing only the deltatext sections
//! (the final and typically largest chunk of an RCS file, see manpage
//! rcsfile(5)) of an RCS file. After a @[parse_deltatext_sections]
//! run, the RCS object will be fully populated.
//! @param raw
//!   The tokenized RCS file, with admin and delta sections removed.
//!   (See @[parse_admin_section], @[tokenize] and @[parse_delta_sections].)
//! @param progress_callback
//!   This optional callback is invoked with the revision of the
//!   deltatext about to be parsed (useful for progress indicators).
//! @param args
//!   Optional extra trailing arguments to be sent to @[progress_callback]
//! @seealso
//!   @[parse_admin_section], @[parse_delta_sections], @[parse], @[create]
//! @fixme
//!   Does not handle rcsfile(5) newphrase skipping.
void parse_deltatext_sections(array raw,
			      void|function(string:void) progress_callback,
			      array|void callback_args)
{
  DeltatextIterator iterate = DeltatextIterator(raw, progress_callback,
						callback_args);
  while(iterate->next())
    ;
}

//! Iterator for the deltatext sections of the RCS file. Typical usage:
//! @example
//!   string raw = Stdio.read_file(my_rcs_filename);
//!   Parser.RCS rcs = Parser.RCS(my_rcs_filename, 0);
//!   raw = rcs->parse_delta_sections(rcs->parse_admin_section(raw));
//!   foreach(rcs->DeltatextIterator(raw); int n; Parser.RCS.Revision rev)
//!     do_something(rev);
class DeltatextIterator
{
  static int finished, this_no;
  static array raw;
  static string this_rev;

  static function(string, mixed ...:void) callback;
  static array callback_args;

  //! @param deltatext_section
  //! the deltatext section of the RCS file in its entirety
  //! @param progress_callback
  //!   This optional callback is invoked with the revision of the
  //!   deltatext about to be parsed (useful for progress indicators).
  //! @param progress_callback_args
  //!   Optional extra trailing arguments to be sent to @[progress_callback]
  //! @seealso
  //! the @tt{rcsfile(5)@} manpage outlines the sections of an RCS file
  static void create(array deltatext_section,
		     void|function(string, mixed ...:void) progress_callback,
		     void|array(mixed) progress_callback_args)
  {
    raw = deltatext_section;
    callback = progress_callback;
    callback_args = progress_callback_args;
  }

  string _sprintf(int|void type, mapping|void options)
  {
    string name = "DeltatextIterator";
    if(type == 't')
      return name;
    return sprintf("%s(/* processed %d/%d revisions%s */)", name, this_no,
		   revisions && sizeof(revisions), (this_no ? ", now at "+this_rev : ""));
  }

  //  @note
  //  this method requires that @[raw] starts with a valid deltatext entry
  //! Drops the leading whitespace before next revision's deltatext
  //! entry and sets this_rev to the revision number we're about to read.
  static int(0..1) read_next()
  {
    raw = parse_deltatext_section(raw);
    return finished = !!raw;
  }

  //! @returns
  //! the number of deltatext entries processed so far (0..N-1, N
  //! being the total number of revisions in the rcs file)
  int index()
  {
    return this_no;
  }

  //! @returns
  //! the @[Revision] at whose deltatext data we are, updated with its info
  Revision value()
  {
    return revisions[this_rev];
  }

  //! @returns
  //! 1 if the iterator has processed all deltatext entries, 0 otherwise.
  int(0..1) `!()
  {
    return finished;
  }

  //! Advance @[nsteps] sections.
  //!
  //! @returns
  //!   Returns the iterator object.
  this_program `+=(int nsteps)
  {
    while(nsteps--)
      next();
    return this;
  }

  //! like @expr{@[`+=](1)@}, but returns 0 if the iterator is finished
  int(0..1) next()
  {
    return read_next() && (++this_no, 1);
  }

  //! Restart not implemented; always returns 0 (==failed)
  int(0..1) first()
  {
    return 0;
  }

  //! Chops off the first deltatext section from the token array @[raw] and
  //! returns the rest of the string, or the value @expr{0@} (zero) if
  //! we had already visited the final deltatext entry. The deltatext's
  //! data is stored destructively in the appropriate entry of the
  //! @[revisions] array.
  //! @note
  //!   @[raw] must start with a deltatext entry for this method to work
  //! @fixme
  //!   does not handle rcsfile(5) newphrase skipping
  //! @fixme
  //!   if the rcs file is truncated, this method writes a descriptive
  //!   error to stderr and then returns 0 - some nicer error handling
  //!   wouldn't hurt
  static array parse_deltatext_section(array raw)
  {
      if( !sizeof(raw) || !symbol_is_revision( raw[0] ) )
	  return 0;

      this_rev = raw[0];

      if(callback)
	  if(callback_args)
	      callback(this_rev, @callback_args);
	  else
	      callback(this_rev);

      Revision current = revisions[this_rev];

      if( raw[1] != "log" )  return 0;
      if( sizeof(raw)<3 )
      {
        werror("Truncated CVS-file!\n");
        return 0;
      }


      current->log = parse_string(raw[2]);
      if( sizeof(raw) > 4 && raw[3] == "text" )
	current->rcs_text = parse_string(raw[4]);
      else
        current->rcs_text = "";

      if(this_rev == head)
      {
	  current->lines = String.count(current->rcs_text,"\n");
	  current->text = current->rcs_text;
      }
      else
      {
	  int added, removed, count;
	  string op;
	  array(string) rows = current->rcs_text / "\n";
	  for(int row=0; row<sizeof(rows); row++)
	  {
	      DEBUG("%s:%O\n", this_rev, rows[row]);
	      if(sscanf(rows[row], "%[ad]%*d %d", op, count) < 2)
		  break; // no rows at all, for instance
	      if(op == "d")
	      {
		  removed += count;
		  continue;
	      }
	      added += count;
	      row += count;
	  }

	  if(String.count(this_rev, ".") == 1)
	  {
	      DEBUG("current: %s %+d-%d l%d a%O n%O\n", this_rev, added, removed,
		    current->lines, current->ancestor, current->next);
	      Revision next = revisions[current->next];
	      current->lines = next->lines - removed + added;
	      current->added = current->lines; // so added files show their length
	      next->removed = added; // remember, the math is all
	      next->added = removed; // backwards on the trunk
	  }
	  else // current revision was on a branch:
	  {
	      current->lines = revisions[current->ancestor]->lines + added - removed;
	      current->removed = removed;
	      current->added = added;
	  }
      }
      if( sizeof(raw) > 4 && raw[3] == "text" )
	return raw[5..];
      else
	return raw[3..];
  }
}

//! Parse the RCS file @[raw] and initialize all members of this object
//! fully initialized.
//! @param raw
//!   The unprocessed RCS file.
//! @param progress_callback
//!   Passed on to @[parse_deltatext_sections].
//! @returns
//!   The fully initialized object (only returned for API convenience;
//!   the object itself is destructively modified to match the data
//!   extracted from @[raw])
//! @seealso
//!   @[parse_admin_section], @[parse_delta_sections],
//!   @[parse_deltatext_sections], @[create]
this_program parse(array raw, void|function(string:void) progress_callback)
{
    parse_deltatext_sections
	(parse_delta_sections
	 (parse_admin_section( raw,progress_callback ),
	  progress_callback),
	 progress_callback);
    return this;
}


// Methods applying to Parser.RCS()->Revision moved out of that class so there
// will be no cyclic references (to RCS->revisions) that forces the user to do
// manual calls to gc().                                     / jhs, 2004-02-24

//! Returns the file contents from the revision @[rev], without performing
//! any keyword expansion.
//! @seealso
//!   @[expand_keywords_for_revision]
string get_contents_for_revision( string|Revision rev )
{
  if( stringp( rev ) ) rev = revisions[rev];
  if( !rev ) return 0;
  if( rev->text ) return rev->text;
  string revision = rev->revision, next = rev->next, ancestor = rev->ancestor;
  Revision parent = revisions[String.count(revision,".")==1 ? next : ancestor];
  string old = get_contents_for_revision( parent ), diff = rev->rcs_text;
  String.Buffer new = String.Buffer();
  function append = new->add;
  int op, of, ot, dt, at, cnt, from, lines;
  while( sizeof( diff ) )
  {
    sscanf( diff, "%c%d %d\n%s", op, from, lines, diff );
    if( op == 'd' )
    {
      cnt = from - at - 1; // possibly scan forward past a few lines...
      if( cnt && of < sizeof(old) )
      {
	ot = of - 1;
	while( cnt-- )
	{
	  ot = search( old, "\n", ++ot );
	  if( ot == -1 )
	  {
	    ot = sizeof( old );
	    break;
	  }
	}
	append( old[of..ot++] ); // ...who were intact since last rev...
	of = ot;
      }
      at = from + lines - 1; // ...to the [lines] lines from line [at]...
      while( lines-- )
      {
	of = search( old, "\n", of );
	if( of == -1 )
	{
	  of = sizeof( old );
	  break;
	}
	of++;
      } // ...that should simply be deleted (not passed on to [new])
    }
    else // op == 'a'
    {
      cnt = from - at; // possibly scan forward past a few lines...
      if( cnt && of < sizeof(old) )
      {
	ot = of - 1;
	while( cnt-- )
	{
	  ot = search( old, "\n", ++ot );
	  if(ot == -1)
	  {
	    ot = sizeof( old );
	    break;
	  }
	}
	append( old[of..ot++] ); // ...who were intact since last rev...
	of = ot;
      }
      at = from; // ...to the line...
      dt = -1;
      while( lines-- )
      {
	dt = search( diff, "\n", ++dt );
	if(dt == -1)
	{
	  dt = sizeof( diff );
	  break;
	}
      }
      append( diff[..dt++] ); // ...where we should add [lines] new rows.
      diff = diff[dt..];
    }
  }
  append( old[of..] );
  return rev->text = new->get();
}

static string kwchars = Array.uniq(sort("Author" "Date" "Header" "Id" "Name"
					"Locker" /*"Log"*/ "RCSfile"
					"Revision" "Source" "State"/1)) * "";

//! Expand keywords and return the resulting text according to the
//! expansion rules set for the file.
//! @param rev
//!   The revision to apply the expansion for.
//! @param text
//!   If supplied, substitute keywords for that text instead using values that
//!   would apply for the given revision. Otherwise, revision @[rev] is used.
//! @param override_binary
//!   If 1, perform expansion even if the file was checked in as binary.
//! @note
//!   The Log keyword (which lacks sane quoting rules) is not
//!   expanded. Keyword expansion rules set in CVSROOT/cvswrappers
//!   are ignored. Only implements the -kkv and -kb expansion modes.
//! @seealso
//!   @[get_contents_for_revision]
string expand_keywords_for_revision( string|Revision rev, string|void text,
				     int|void override_binary )
{
  if( stringp( rev ) ) rev = revisions[rev];
  if( !rev ) return 0;
  if( !text ) text = get_contents_for_revision( rev );
  if( rev->expand == "b" && !override_binary )
    return text;
  string before, delimiter, keyword, expansion, rest, result = "";
  string date = replace( rev->time->format_time(), "-", "/" ),
    file = basename( rcs_file_name );
  mapping kws = ([ "Author"	: rev->author,
		   "Date"	: date,
		   "Header"	: ({ rcs_file_name, rev->revision, date,
				     rev->author, rev->state }) * " ",
		   "Id"		: ({ file, rev->revision, date,
				     rev->author, rev->state }) * " ",
		   "Name"	: "", // only applies to a checked-out file
		   "Locker"	: search( locks, rev->revision ) || "",
		   /*"Log"	: "A horrible mess, at best", */
		   "RCSfile"	: file,
		   "Revision"	: rev->revision,
		   "Source"	: rcs_file_name,
		   "State"	: rev->state ]);
  while( sizeof( text ) )
  {
    if( sscanf( text, "%s$%["+kwchars+"]%[:$]%s",
		before, keyword, delimiter, rest ) < 4 )
    {
      result += text;
      break;
    }
    if( expansion = kws[keyword] )
    {
      if( has_value( delimiter, "$" ) )
	result += sprintf( "%s$%s: %s $", before, keyword, expansion );
      else
      {
	if( sscanf( rest, "%*[^\n]$%s", rest ) != 2 )
	{
	  result += text;
	  break;
	}
	result += sprintf( "%s$%s: %s $", before, keyword, expansion );
      }
      text = rest;
    }
    else
    {
      result += before + "$" + keyword;
      text = delimiter + rest; // delimiter could be the start of a keyword
    }
  }
  return result;
}

//! All data tied to a particular revision of the file.
class Revision
{
  //! the revision number (i e @[RCS]->revisions["1.1"]->revision == "1.1")
  string revision;

  //! the name of the user that committed the revision
  string author;

  //! when there are branches from this revision, an array of the
  //! revision numbers where each branch starts, otherwise 0
  array(string) branches;

  //! the state of the revision - typically "Exp" or "dead"
  string state;

  //! the (UTC) date and time when the revision was committed (second
  //! precision)
  Calendar.TimeRange time;

  //! the branch name on which this revision was committed (calculated
  //! according to how cvs manages branches)
  string branch;

  //! the revision stored next in the rcs file, or 0 if none exists
  string rcs_next;


  //! the revision of the ancestor of this revision, or 0 if this was
  //! the initial revision
  string ancestor;

  //! the revision that succeeds this revision, or 0 if none exists
  string next;

  //! the log message associated with the revision
  string log;

  //! the number of lines this revision contained, altogether (not of
  //! particular interest for binary files)
  int lines;

  //! the number of lines that were added from the previous revision
  //! to make this revision (for the initial revision too)
  int added;

  //! the number of lines that were removed from the previous revision
  //! to make this revision
  int removed;

  string rcs_text; // the diff as stored in the rcs file

  string text; // when parsed once, the text, as it was checked in

  string _sprintf(int|void type)
  {
    if(type == 't')
      return "Revision";
    return sprintf("Revision(/* %s */)", revision||"uninitizlized");
  }
}
