#! /usr/bin/env pike
// $Id: RCS.pike,v 1.8 2002/02/28 04:32:18 jhs Exp $

//! A RCS file parser that eats a RCS *,v file and presents nice pike
//! data structures of its contents.

#ifdef PARSER_RCS_DEBUG
#define DEBUG(X, Y ...) werror("Parser.RCS: " + X, Y)
#else
#define DEBUG(X, Y ...)
#endif

// regexps based on rcsfile(5) after a really careful read of its poor grammar
// (i e {idchar|num} and {idchar|digit} being unnecessarily bothersome synonyms
//  to {idchar}, given consideration to notions on where whitespace is illegal)
#define OPT(X)	"(" X "|)"
#define ANY(X)	"(" X ")*"
#define REST	"(.*)"
#define WS	"[ \b-\r]*"
#define SWS	"%*[ \b-\r]"
#define STRING	WS "@(([^@]|@@)*)@" WS
#define NUM	"([0-9.][0-9.]*)"
#define DIGIT	"[0-9]"
// Actually, $,. should be in idchar too, but cvs uses at least "." in IDs:
#define IDCHAR	"[^:;@ \b-\r]"
#define ID	OPT(NUM) IDCHAR IDCHAR "*"
#define SYM	ANY(DIGIT) IDCHAR IDCHAR "*"
#define ADMIN	"^" WS "head" WS OPT(NUM) ";" WS \
		OPT("branch" WS OPT(NUM) ";" WS) \
		"access" WS "((" ID WS ")*)" ";" WS \
		"symbols" WS "(" ANY(SYM WS ":" WS NUM WS) ");" WS \
		"locks" WS "(" ANY(ID WS ":" WS NUM WS) ");" \
			OPT(WS "strict" WS ";") WS \
		OPT("comment" WS OPT(STRING) ";" WS) \
		OPT("expand" WS OPT(STRING) ";" WS)
#define DELTA	"^" NUM WS \
		"date" WS NUM ";" WS \
		"author" WS "(" ID ")" WS ";" WS \
		"state"	WS OPT(ID) ";" WS \
		"branches" WS "((" NUM WS ")*)" WS ";" WS \
		"next" WS OPT(NUM) ";" WS

//!
string head;		// num
string branch;		// num
array(string) access;	// ids
int strict_locks;
string comment;
string expand;
string description;

//! Maps from username to revision for users that have aquired locks
//! on this file.
mapping(string:string) locks;	 // id:num

//! Maps tag names (indices) to tagged revision numbers (values).
mapping(string:string) tags;

//! Maps branch numbers (indices) to branch names (values).
mapping(string:string) branches;

string _sprintf(int type)
{
  if(type=='t')
    return "RCS";
  return sprintf("RCS(/* %d revisions */)",
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

Regexp admin = Regexp(ADMIN REST);
Regexp delta = Regexp(DELTA REST);

static array(string) parse_array(string data)
{
  return Array.flatten(array_sscanf(data, "%{%[^ :\b-\r]%*[ :\b-\r]%}"));
}

static mapping parse_mapping(string data)
{
  return (mapping)(parse_array(data) / 2);
}

static array(string) parse_string(string data, string|void leader)
{
  string segment, result, original = data;
  if(leader)
    sscanf(data, SWS + (leader||"") + SWS "%s", data);
  if(2 != sscanf(data, "@%s@%s", result, data))
    return ({ 0, original }); // "no leading @" or "@not terminated"
  while(sscanf(data, "@%s@%s", segment, data))
    result += "@" + segment;
  if(has_prefix(data, "@"))
    return ({ 0, original }); // "@not @@ terminated"
  return ({ result, data });
}

function symbol_is_branch = Regexp("\\.0\\.[0-9]*[02468]$")->match;

string rcs_file_name;

//!
void create(string|void file_name, string|void raw)
{
  if(!file_name)
  {
    if(!raw)
      return;
  }
  else
  {
    rcs_file_name = file_name;
    if(!raw)
      raw = Stdio.read_file(file_name);
    if(!raw)
      error("Couldn't read %s\n", file_name);
  }
  parse(raw);
}

//!
void parse(string raw)
{
  int pos;
  string broken_regexp_kludge = "";
  if(-1 != (pos = search(raw, "\0")))
    broken_regexp_kludge = raw[pos..]; // since (.*) abruptly ends at first \0
  array got = admin->split(raw);
  head = got[0];
  branch = got[3];
  if(sizeof(got[5]))
    access = parse_array(got[5]);
  tags = ([]); branches = ([]);
  foreach((array)parse_mapping(got[9]||""), [string name, string revision])
    if(revision == "1.1.1")
      branches[revision] = name; // the vendor branch
    else if(symbol_is_branch(revision))
    {
      array(string) nums = revision / "."; // "a.b.c.d.0.e"
      revision = nums[..sizeof(nums)-3] * "." + "." + nums[-1]; // "a.b.c.d.e"
      branches[revision] = name;
    } else
      tags[name] = revision;
  locks = parse_mapping(got[13]);   // mapping(id:num)
  strict_locks = has_value(got[18], "strict");
  if(got[21])
    comment = replace(got[21], "@@", "@");
  if(got[25])
    expand = replace(got[25], "@@", "@");
  raw = got[-1];

  // FIXME: If this stops working, introduce newphrase skipping here

  DEBUG("\nhead: %O\nbranch: %O\naccess: %O\n"
	"locks: %O\nstrict: %O\ncomment: %O\n"
	"expand: %O\nrest: %O\n",
	head, branch, access, locks,
	strict_locks, comment, expand, raw[..50]);

  mapping(string:Revision) revs = ([]);
  while(got = delta->split(raw))
  {
    Revision R = Revision();
    R->revision = got[0];
    string date = got[1];
    if(!(int)date)
      date = "1900" + date[search(date, ".")..];
    R->time = Calendar.ISO.parse("%y.%M.%D.%h.%m.%s %z",
				 date + " UTC");
    R->author = got[2];
    R->state = got[5];
    if(got[8])
      R->branches = parse_array(got[8]);
    if(sizeof(got[11]))
      R->rcs_next = got[11];
    revs[R->revision] = R;

    DEBUG("revision: %O\n\n", R);
    raw = got[-1];
  }

  // FIXME: If this stops working, introduce newphrase skipping here

  raw += broken_regexp_kludge; broken_regexp_kludge = 0;
  [description, raw] = parse_string(raw, "desc");

  int is_on_trunk;
  string this_rev, lmsg, diff;

  while(sscanf(raw, SWS "%[0-9.]" SWS "%s", this_rev, raw) &&
	sizeof(this_rev))
  {
    Revision this = revs[this_rev];
    if(is_on_trunk = sizeof(this_rev/".") == 2)
      trunk += ({ this });
    else
    {
      sscanf(reverse(this_rev), "%*d.%s", string branch);
      this->branch = branches[reverse(branch)];
    }

    if(this->rcs_next) // are there more revisions referred?
      if(is_on_trunk)
      {
	this->ancestor = this->rcs_next; // next pointers refer backwards
	revs[this->rcs_next]->next = this_rev;
      }
      else // this revision is a branch
      {
	revs[this->rcs_next]->ancestor = this_rev; // ditto but forwards
	this->next = this->rcs_next;
      }

    foreach(this->branches, string branch_point)
      revs[branch_point]->ancestor = this_rev;

    [this->log, raw] = parse_string(raw, "log");
    [this->rcs_text, raw] = parse_string(raw, "text");
    if(!this->log || !this->rcs_text)
    {
      werror("RCS file %sbroken (truncated?) at rev %s.\n",
	     (rcs_file_name ? rcs_file_name + " " : ""), this_rev);
      break;
    }

    array(string) rows = this->rcs_text / "\n";
    if(this_rev == head)
    {
      this->lines = sizeof(rows);
      this->text = this->rcs_text;
    }
    else
    {
      int added, removed, count;
      string op;
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

      if(is_on_trunk)
      {
	DEBUG("this: %s %+d-%d l%d a%O n%O\n", this_rev, added, removed,
	      this->lines, this->ancestor, this->next);
	Revision next = revs[this->next];
	this->lines = next->lines - removed + added;
	this->added = this->lines; // to make added files show their length
	next->removed = added; // remember, the math is all
	next->added = removed; // backwards on the trunk
      }
      else
      {
	this->lines = revs[this->ancestor]->lines + added - removed;
	this->removed = removed;
	this->added = added;
      }
    }
  }
  revisions = revs;
}

//! All data tied to a particular revision of the file.
class Revision
{
  //! the name of the user that committed the revision
  string author;

  //! when there are branches from this revision, an array of the
  //! revisions where each branch starts, otherwise 0
  array(string) branches;

  //! the state of the revision - typically "Exp" or "dead"
  string state;

  //! the date and time when the revision was committed
  Calendar.ISO.Second time;

  //! the branch name on which this revision was committed (calculated
  //! according to how cvs manages branches)
  string branch;

  //! the revision number (i e revisions["1.1"]->revision == "1.1")
  string revision;

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

  string text; // when parsed once, the text as it was checked in

  string _sprintf(int|void type)
  {
    if(type == 't')
      return "Revision";
    return sprintf("Revision(/* %s */)", revision||"uninitizlized");
  }

  //! Returns the file contents from this revision, without performing
  //! any keyword expansion.
  //! @seealso
  //!   @[expand_keywords]
  string get_contents()
  {
    if(text)
      return text;
    Revision parent = revisions[sizeof(revision/".")==2 ? next : ancestor];
    string old = parent->get_contents(), new = "", diff = rcs_text, op;
    int of, ot, dt, at, cnt, from, lines;
    while(sizeof(diff))
    {
      sscanf(diff, "%[ad]%d %d\n%s", op, from, lines, diff);
      if(op == "d")
      {
	cnt = from - at - 1; // possibly scan forward past a few lines...
	if(cnt && of < sizeof(old))
	{
	  ot = of - 1;
	  while(cnt--)
	  {
	    ot = search(old, "\n", ++ot);
	    if(ot == -1)
	    {
	      ot = sizeof(old);
	      break;
	    }
	  }
	  new += old[of..ot++]; // ...that remained intact since last rev...
	  of = ot;
	}
	at = from + lines - 1; // ...to the [lines] lines from line [at]...
	while(lines--)
	{
	  of = search(old, "\n", of);
	  if(of == -1)
	  {
	    of = sizeof(old);
	    break;
	  }
	  of++;
	} // ...that should simply be deleted (not passed on to [new])
      }
      else // op == "a"
      {
	cnt = from - at; // possibly scan forward past a few lines...
	if(cnt && of < sizeof(old))
	{
	  ot = of - 1;
	  while(cnt--)
	  {
	    ot = search(old, "\n", ++ot);
	    if(ot == -1)
	    {
	      ot = sizeof(old);
	      break;
	    }
	  }
	  new += old[of..ot++]; // ...that remained intact since last rev...
	  of = ot;
	}
	at = from; // ...to the line...
	dt = -1;
	while(lines--)
	{
	  dt = search(diff, "\n", ++dt);
	  if(dt == -1)
	  {
	    dt = sizeof(diff);
	    break;
	  }
	}
	new += diff[..dt++]; // ...where we should add [lines] new rows.
	diff = diff[dt..];
      }
    }
    return text = new += old[of..];
  }

  static string kwchars = Array.uniq(sort("Author" "Date" "Header" "Id" "Name"
					  "Locker" /*"Log"*/ "RCSfile"
					  "Revision" "Source" "State"/1)) * "";

  //! Expand keywords and return the resulting text according to the
  //! expansion rules set for the file.
  //! @param text
  //!   If supplied, substitutes keywords for that text instead, using values
  //!   that would apply for this revision. Otherwise, this revision is used.
  //! @param override_binary
  //!   Perform expansion even if the file was checked in as binary.
  //! @note
  //!   The Log keyword (which lacks sane quoting rules) is not
  //!   expanded. Keyword expansion rules set in CVSROOT/cvswrappers
  //!   are ignored. Only implements the -kkv and -kb expansion modes.
  //! @seealso
  //!   @[get_contents]
  string expand_keywords(string|void text, int|void override_binary)
  {
    if(!text)
      text = get_contents();
    if(expand == "b" && !override_binary)
      return text;
    string before, delimiter, keyword, expansion, rest, result = "";
    string date = replace(time->format_time(), "-", "/"),
	   file = basename(rcs_file_name);
    mapping kws = ([ "Author"	: author,
		     "Date"	: date,
		     "Header"	: ({ rcs_file_name, revision, date,
				     author, state }) * " ",
		     "Id"	: ({ file, revision, date, author, state })*" ",
		     "Name"	: "", // only applies to a checked-out file
		     "Locker"	: search(locks, revision) || "",
		     /*"Log"	: "A horrible mess, at best", */
		     "RCSfile"	: file,
		     "Revision"	: revision,
		     "Source"	: rcs_file_name,
		     "State"	: state ]);
    while(sizeof(text))
    {
      if(sscanf(text, "%s$%["+kwchars+"]%[:$]%s",
		before, keyword, delimiter, rest) < 4)
      {
	result += text;
	break;
      }
      if(expansion = kws[keyword])
      {
	if(has_value(delimiter, "$"))
	  result += sprintf("%s$%s: %s $", before, keyword, expansion);
	else
	{
	  if(sscanf(rest, "%*[^\n]$%s", rest) != 2)
	  {
	    result += text;
	    break;
	  }
	  result += sprintf("%s$%s: %s $", before, keyword, expansion);
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
}
