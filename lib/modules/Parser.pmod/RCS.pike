#! /usr/bin/env pike
// $Id: RCS.pike,v 1.1 2002/02/23 00:11:51 jhs Exp $

//! A RCS file parser that eats a RCS *,v files and presents nice pike
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

string head;		// num
string branch;		// num
array(string) access;	// ids

//! Maps tag names (indices) to tagged revision numbers (values).
mapping(string:string) tags;

//! Maps branch numbers (indices) to branch names (values).
mapping(string:string) branches;

mapping(string:string) locks;	 // id:num
int strict_locks;
string comment;
string expand;
string description;

string _sprintf(int type)
{
  if(type=='t')
    return "RCS";
  return sprintf("RCS(/* %d revisions */)",
		 revisions && sizeof(revisions));
}

//! Data for all revisions of the file. The indices of the mapping
//! are the revision numbers, whereas the values are mappings of the
//! following format:
//! @mapping
//!   @member string "revision"
//!     the revision number (i e revisions["1.1"]->revision == "1.1")
//!   @member string "branch"
//!     the branch name on which this revision was committed (calculated
//!     according to how cvs manages branches)
//!   @member Calendar.ISO.Second "time"
//!     the date and time when the revision was committed
//!   @member string "author"
//!     the name of the user that committed the revision
//!   @member string "log"
//!     the log message associated with the revision
//!   @member string "state"
//!     the state of the revision - typically "Exp" or "dead"
//!   @member array(string) "branches"
//!     when there are branches from this revision, an array of the revisions
//!     where each branch starts, otherwise 0
//!   @member int "lines"
//!     the number of lines this revision contained, altogether (not of
//!     particular interest for binary files)
//!   @member int "added"
//!     the number of lines that were added from the previous revision to make
//!     this revision (for the initial revision too)
//!   @member int "removed"
//!     the number of lines that were removed from the previous revision to
//!     make this revision
//!   @member string "ancestor"
//!     the revision of the ancestor of this revision or zero, if this was the
//!     initial revision
//!   @member string "next"
//!     the revision that succeeds this revision or 0, if none exists
//!   @member string "rcs_next"
//!     the revision stored next in the rcs file or 0, if none exists
//! @endmapping
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

class Revision
{
  int added, removed, lines, bytes;

  string author;
  string log;
  string branch;

  string revision;
  string ancestor;
  string next;
  string rcs_next;
  string text;
  string rcs_text;

  array(string) branches;

  string state;
  Calendar.ISO.Second time;

  string _sprintf(int|void type)
  {
    if(type == 't')
      return "Revision";
    return sprintf("Revision(/* %s */)", revision||"uninitizlized");
  }

  array read_lines(string data, int offs, int count)
  {
    if(!count)
      return ({ "", offs });
    if(offs >= sizeof(data))
      return ({ "", sizeof(data) });
    int from = offs;
    while(count--)
    {
      offs = search(data, "\n", offs);
      if(offs == -1)
	return ({ data[from..], sizeof(data) });
      offs++;
    }
    return ({ data[from..offs-1], offs });
  }

  string get_contents()
  {
    if(text)
      return text;
    Revision parent = revisions[sizeof(revision/".")==2 ? next : ancestor];
    string old = parent->get_contents(), new = "", diff = rcs_text, op, tmp;
    int scan_from, scan_to, scanned, scan_diff, at_line, from, lines, skip;
    int o;
    //werror("\n\nold:\n%s\ndiff:\n%s\n", old, diff);
    while(sizeof(diff))
    {
      sscanf(diff, "%[ad]%d %d\n%s", op, from, lines, diff);
      //werror("op:%s from:%d lines:%d\n", op, from, lines);
      if(op == "d")
      {
	[tmp, o] = read_lines(old, o, from - at_line - 1);
	new += tmp;
	at_line = from + lines - 1;
	[tmp, o] = read_lines(old, o, lines);
	//werror("before: %O [o:%d, @%d]\n", tmp, o, at_line);
      }
      else
      {
	[tmp, o] = read_lines(old, o, from - at_line);
	new += tmp;
	at_line = from;
	//werror("before: %O [o:%d, @%d]\n", tmp, o, at_line);
	[tmp, scan_diff] = read_lines(diff, 0, lines);
	new += tmp;
	diff = diff[scan_diff..];
	//werror("to add: %O [o:%d, @%d]\n", tmp, o, at_line);
      }
    }
    return text = new += old[o..];
  }
}
