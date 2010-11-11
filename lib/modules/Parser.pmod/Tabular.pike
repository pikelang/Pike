//! This is a parser for line and block oriented data.
//! It provides a flexible yet concise record-description language to parse
//! character/column/delimiter-organised records.
//!
//! @seealso
//!  @[Parser.LR]

#pike __REAL_VERSION__

private Stdio.FILE in;
private int prefetch=1024;	 // TODO: Document and make this available
				 // through compile().
private String.Buffer alread=String.Buffer(prefetch);
private mapping|array fms;
private int eol;
private Regexp simple=Regexp("^[^[\\](){}<>^$|+*?\\\\]+$");
private Regexp emptyline=Regexp("^[ \t\v\r\x1a]*$");
private mixed severity=1;
private int verb=0;
private int recordcount=1;

//! This function initialises the parser.
//!
//! @param input
//! The input stream or string.
//!
//! @param format
//! The format to be used (either precompiled or not).
//! The format description language is documented under @[compile()].
//!
//! @param verbose
//!    If @expr{>1@}, it specifies the number of characters to display
//!    of the beginning of each record as a progress indicator.  Special
//!    values are:
//! @int
//!	 @value -4
//!	    Turns on format debugging with visible mismatches.
//!	 @value -3
//!	    Turns on format debugging with named field contents.
//!	 @value -2
//!	    Turns on format debugging with field contents.
//!	 @value -1
//!	    Turns on basic format debugging.
//!	 @value 0
//!	    Turns off verbosity. Default.
//!	 @value 1
//!	    Is the same as setting it to @expr{70@}.
//! @endint
//!
//! @seealso
//!  @[compile()], @[setformat()], @[fetch()]
void
 create(void|string|Stdio.File|Stdio.FILE input,
  void|array|mapping|string|Stdio.File|Stdio.FILE format,
  void|int verbose)
{ if(zero_type(verbose)&&intp(format))
    verbose=format;
  else
    fms=stringp(format)||objectp(format)?compile(format):format;
  verb=verbose==1?70:verbose;
  if(!input)
    input=" ";
  if(stringp(input))
    input=Stdio.FakeFile(input);
  if(!input->unread)
    (in=Stdio.FILE())->assign(input);
  else
    in=input;
}

#if 0			   // Currently unused function
private int getchar()
{ int c=in->getchar();
  if(c<0)
    throw(severity);
  alread->putchar(c);
  return c;
}
#endif

private string read(int n)
{ string s;
  s=in->read(n);
  alread->add(s);
  if(sizeof(s)!=n)
    throw(severity);
  return s;
}

private string gets(int n)
{ string s;
  if(n)
  { s=read(n);
    if(has_value(s,"\n")||has_value(s,"\r"))
      throw(severity);
  }
  else
  { s=in->gets();
    if(!s)
       throw(severity);
    if(has_value(s,"\r"))	// Retrofix \r-only line endings
    { array t;
      t=s/"\r";
      s=t[0];in->unread(t[1..]*"\n");
    }
    alread->add(s);alread->putchar('\n');
    if(has_suffix(s,"\r"))
      s=s[..<1];
    eol=1;
  }
  return s;
}

private class checkpoint
{ private string oldalread;

  void create()
  { oldalread=alread->get();
  }

  final void release()
  { string s=alread->get();
    alread->add(oldalread);
    alread->add(s);
    oldalread=0;
  }

  protected void destroy()
  { if(oldalread)
    { string back=alread->get();
      if(sizeof(back))
      { in->unread(back);
        if(verb<0)
        { back-="\n";
          if(sizeof(back))
            werror("Backtracking %O\n",back);
        }
      }
      alread->add(oldalread);
    }
  }
}

#define FETCHAR(c,buf,i)	(catch((c)=(buf)[(i)++])?((c)=-1):(c))

private mapping getrecord(array fmt,int found)
{ mapping ret=([]),options;
  if(stringp(fmt[0]))
  { options=(["name":fmt[0]]);
    if(fmt[1])
      options+=fmt[1];
    else
      fmt[1]=0;
  }
  else
    options=fmt[0];
  if(found)
  { if(options->single)
      throw(severity);			// early exit, already found one
  }
  else if(options->mandatory)
    severity=2;
  if(verb<0)
    werror("Checking record %d for %O\n",recordcount,options->name);
  eol=0;
  foreach(fmt;int fi;array|mapping m)
  { if(fi<2)
      continue;
    string value;
    if(arrayp(m))
    { array field=m;
      fmt[fi]=m=(["name":field[0]]);
      mixed nm=field[1];
      if(!mappingp(nm))
      { if(arrayp(nm))
          ret+=getrecord(nm,found);
        else
          m+=([(intp(nm)?"width":(stringp(nm)?"match":"delim")):nm]);
        if(sizeof(field)>2)
          m+=field[2];
      }
      fmt[fi]=m;
    }
    if(eol)
      throw(severity);
    if(!zero_type(m->width))
      value=gets(m->width);
    if(m->delim)
    { multiset delim=m->delim;
      int i,pref=m->prefetch || prefetch;
      String.Buffer word=String.Buffer(pref);
      string buf,skipclass;
      skipclass="%[^"+(string)indices(delim)+"\"\r\x1a\n]";
      if(sizeof(delim-(<',',';','\t',' '>)))
delimready:
        for(;;)
        { i=0;
          buf=in->read(pref);
          int c;
          FETCHAR(c,buf,i);
          while(c>=0)
          { if(delim[c])
              break delimready;
            else switch(c)
            { default:
              { string s;
                sscanf(buf[--i..],skipclass,s);
                word->add(s);
                i+=sizeof(s);
                break;
              }
              case '\n':
		FETCHAR(c,buf,i);
		switch(c)
		{ default:i--;
		  case '\r':case '\x1a':;
		}
                eol=1;
                break delimready;
              case '\r':
		FETCHAR(c,buf,i);
		if(c!='\n')
		  i--;
                eol=1;
                break delimready;
	      case '\x1a':;
            }
            FETCHAR(c,buf,i);
          }
          if(!sizeof(buf))
            throw(severity);
          alread->add(buf);
        }
      else
      { int leadspace=1,inquotes=0;
csvready:
        for(;;)
        { i=0;
          buf=in->read(pref);
          int c;
          FETCHAR(c,buf,i);
          while(c>=0)
          { if(delim[c])
            { if(!inquotes)
                break csvready;
              word->putchar(c);
            }
            else switch(c)
            { case '"':leadspace=0;
                if(!inquotes)
                  inquotes=1;
                else if(FETCHAR(c,buf,i)=='"')
                  word->putchar(c);
                else
                { inquotes=0;
                  continue;
                }
                break;
              default:leadspace=0;
              case ' ':case '\t':
                if(!leadspace)
                { string s;
                  sscanf(buf[--i..],skipclass,s);
                  word->add(s);
                  i+=sizeof(s);
                }
                break;
              case '\n':
		FETCHAR(c,buf,i);
		switch(c)
		{ default:i--;
		  case '\r':case '\x1a':;
		}
                if(!inquotes)
                { eol=1;
                  break csvready;
                }
                word->putchar('\n');
		break;
              case '\r':
		FETCHAR(c,buf,i);
		if(c!='\n')
		  i--;
                if(!inquotes)
                { eol=1;
                  break csvready;
                }
                word->putchar('\n');
	      case '\x1a':;
            }
            FETCHAR(c,buf,i);
          }
          if(!sizeof(buf))
            throw(severity);
          alread->add(buf);
        }
      }
      alread->add(buf[..i-1]);
      in->unread(buf[i..]);
      value=word->get();
    }
    if(m->match)
    { Regexp rgx;
      if(stringp(m->match))
      { if(!value && simple->match(m->match))
        { m->width=sizeof(m->match);
          value=gets(m->width);
        }
        m->match=Regexp("^("+m->match+")"+(value?"$":""));
      }
      rgx=m->match;
      if(value)
      { if(!rgx->match(value))
        { if(verb<-3)
            werror(sprintf("Mismatch %O!=%O\n",value,rgx)
             -"Regexp.SimpleRegexp");
          throw(severity);
        }
      }
      else
      { string buf=in->read(m->prefetch || prefetch);
        { array spr;
          if(!buf || !(spr=rgx->split(buf)))
          { alread->add(buf);
            if(verb<-3)
              werror(sprintf("Mismatch %O!=%O\n",buf[..32],rgx)
               -"Regexp.SimpleRegexp");
            throw(severity);
          }
          in->unread(buf[sizeof(value=spr[0])..]);
        }
        alread->add(value);
        value-="\r";
        if(has_suffix(value,"\n"))
          value=value[..<1];
      }
    }
    if(!m->drop)
      ret[m->name]=value;
  }
  if(!eol && gets(0)!="")
    throw(severity);
  severity=1;
  if(verb&&verb!=-1)
  { array s=({options->name,"::"});
    foreach(sort(indices(ret)),string name)
    { string value=ret[name];
      if(sizeof(value))
      { if(verb<-2)
          s+=({name,":"});
        s+=({value,","});
      }
    }
    string out=replace(s[..<1]*"",({"\n","  ","   "}),({""," "," "}));
    out=string_to_utf8(out);	// FIXME Debugging output defaults to UTF-8
    if(verb>0)
      werror("%d %.*s\r",recordcount,verb,out);
    else
      werror("%d %s\n",recordcount,out);
  }
  recordcount++;
  return options->fold?ret:([options->name:ret]);
}

private void add2map(mapping res,string name,mixed entry)
{ mapping|array tm = res[name];
  if(tm)
  { if(arrayp(tm))
      tm+=({entry});
    else
      tm=({tm,entry});
    res[name]=tm;
  }
  else
    res[name]=entry;
}

//! This function can be used to manually skip empty lines in
//! the input.  This is unnecessary if no argument is
//! specified for @[fetch()].
//!
//! @returns
//! It returns true if EOF has been reached.
//!
//! @seealso
//!  @[fetch()]
int skipemptylines()
{ string line; int eof=1;
  while((line=in->gets()) && String.width(line)==8 && emptyline->match(line))
    recordcount++;
  if(line)
    eof=0,in->unread(line+"\n");
  return eof;
}

//! This function consumes as much input as needed to parse
//! the full tabular structures at once.
//!
//! @param format
//! Describes (precompiled only) formats to be parsed.
//! If no format is specified,
//! the format specified on @[create()] is used, and empty lines are
//! automatically skipped.
//!
//! @returns
//! A nested mapping that contains the complete structure as
//! described in the specified format.
//!
//! If nothing matches the specified format, no input is consumed
//! (except empty lines, if the default format is used), and
//! zero is returned.
//!
//! @seealso
//!  @[compile()], @[create()], @[setformat()], @[skipemptylines()]
mapping fetch(void|array|mapping format)
{ mapping ret=([]);
  int skipempty=0;
  if(!format)
  { if(skipemptylines())
      return UNDEFINED;
    skipempty=1;format=fms;
  }
ret:
  { if(arrayp(format))
    { mixed err=catch
      { checkpoint checkp=checkpoint();
        foreach(format;;array|mapping fmt)
          if(arrayp(fmt))
            for(int found=0;;found=1)
            { mixed err=catch
              { checkpoint checkp=checkpoint();
		mapping rec=getrecord(fmt,found);
	        foreach(rec;string name;mixed value)
                  add2map(ret,name,value);
                checkp->release();
                continue;
              };
              severity=1;
              switch(err)
              { case 2:
		  err=1;
		default:
		  throw(err);
		case 1:;
	      }
              break;
            }
          else if(fmt=fetch(fmt))
            ret+=fmt;
        checkp->release();
        break ret;
      };
      switch(err)
      { default:
          throw(err);
        case 1:
          return 0;
      }
      if(skipempty)
        skipemptylines();
    }
    else
    { int found;
      do
      { found=0;
        foreach(format;string name;array|mapping subfmt)
          for(;;)
	  { if(verb<0)
	      werror("Trying format %O\n",name);
	    mapping m;
	    if(m=fetch(subfmt))
	    { found=1;add2map(ret,name,m);
	      continue;
	    }
	    break;
	  }
        if(skipempty && skipemptylines())
          break;
      }
      while(found);
    }
  }
  return sizeof(ret) && ret;
}

//! @param content
//! Is injected into the input stream.
//!
//! @returns
//! This object.
//!
//! @seealso
//!  @[fetch()]
object feed(string content)
{ in->unread(content);
  return this;
}

//! @param format
//! Replaces the default (precompiled only) format.
//!
//! @returns
//! The previous default format.
//!
//! @seealso
//!   @[compile()], @[fetch()]
array|mapping setformat(array|mapping format)
{ array|mapping oldfms=fms;
  fms=format;
  return oldfms;
}

private Regexp descrx=Regexp(
 "^([ :]*)([^] \t:;#]*)[ \t]*([0-9]*)[ \t]*(\\[([^]]+)\\]|)"
 "[ \t]*(\"(([^\"]|\\\\\")*)\"|)[ \t]*([a-z][a-z \t]*[a-z]|)[ \t]*([#;].*|)$"
 );
private Regexp tokenise=Regexp("^[ \t]*([^ \t]+)[ \t]*(.*)$");

//! Compiles the format description language into a compiled structure
//! that can be fed to @[setformat], @[fetch], or @[create].
//!
//! @ul
//! @item
//!  The format description is case sensitive.
//! @item
//!  The format description starts with a single line containing:
//! @expr{[Tabular description begin]@}
//! @item
//!  The format description ends with a single line containing:
//! @expr{[Tabular description end]@}
//! @item
//!  Any lines before the startline are skipped.
//! @item
//!  Any lines after the endline are not consumed.
//! @item
//!  Empty lines are skipped.
//! @item
//!  Comments start after a @expr{#@} or @expr{;@}.
//! @item
//!  The depth level of a field is indicated by the number of
//! leading spaces or colons at the beginning of the line.
//! @item
//!  The fieldname must not contain any whitespace.
//! @item
//!  An arbitrary number of single character field delimiters can be
//! specified between brackets, e.g. @expr{[,;]@} or @expr{[,]@} would be
//! for CSV.
//! @item
//!  When field delimiters are being used: in case of CSV type delimiters
//! @expr{[\t,; ]@} the standard CSV quoting rules apply, in case other
//! delimiters
//! are used, no quoting is supported and the last field on a line should
//! not specify a delimiter, but should specify a 0 fieldwidth instead.
//! @item
//!  A fixed field width can be specified by a plain decimal integer,
//! a value of 0 indicates a field with arbitrary length that extends
//! till the end of the line.
//! @item
//!  A matching regular expression can be enclosed in @expr{""@}, it has
//! to match
//! the complete field content and uses @[Regexp.SimpleRegexp] syntax.
//! @item
//!  On records the following options are supported:
//! @dl
//!  @item mandatory
//!   This record is required.
//!  @item fold
//!   Fold this record's contents in the enclosing record.
//!  @item single
//!   This record is present at most once.
//! @enddl
//! @item
//!  On fields the following options are supported:
//! @dl
//!  @item drop
//!   After reading and matching this field, drop the field content
//!   from the resulting mappingstructure.
//! @enddl
//! @endul
//!
//! @seealso
//!   @[setformat()], @[create()], @[fetch()]
//!
//! @example
//! Example of the description language:
//!
//![Tabular description begin]
//!csv
//!:gtz
//!::mybankno           [,]
//!::transferdate       [,]
//!::mutatiesoort       [,]
//!::volgnummer         [,]
//!::bankno             [,]
//!::name               [,]
//!::kostenplaats       [,]                     drop
//!::amount             [,]
//!::afbij              [,]
//!::mutatie            [,]
//!::reference          [,]
//!::valutacode         [,]
//!mt940
//!:messageheader1                     mandatory
//!::exporttime            "0000"               drop
//!::CS1                   " "                  drop
//!::exportday             "01"                 drop
//!::exportaddress      12
//!::exportnumber       5  "[0-9]+"
//!:messageheader3                     mandatory fold single
//!::messagetype           "940"                drop
//!::CS1                   " "                  drop
//!::messagepriority       "00"                 drop
//!:TRN                             fold
//!::tag                   ":20:"               drop
//!::reference             "GTZPB|MPBZ|INGEB"
//!:accountid                     fold
//!::tag                   ":25:"               drop
//!::accountno          10
//!:statementno                     fold
//!::tag                   ":28C:"              drop
//!::settlementno       0                       drop
//!:openingbalance                     mandatory      single
//!::tag                   ":60F:"              drop
//!::creditdebit        1
//!::date               6
//!::currency              "EUR"
//!::amount             0  "[0-9]+,[0-9][0-9]"
//!:statements
//!::statementline                     mandatory fold single
//!:::tag                  ":61:"               drop
//!:::valuedate         6
//!:::creditdebit       1
//!:::amount               "[0-9]+,[0-9][0-9]"
//!:::CS1                  "N"                  drop
//!:::transactiontype   3                          # 3 for Postbank, 4 for ING
//!:::paymentreference  0
//!::informationtoaccountowner                   fold single
//!:::tag                  ":86:"               drop
//!:::accountno            "[0-9]*( |)"
//!:::accountname       0
//!::description         fold
//!:::description       0  "|[^:].*"
//!:closingbalance                     mandatory      single
//!::tag                   ":62[FM]:"           drop
//!::creditdebit        1
//!::date               6
//!::currency              "EUR"
//!::amount             0  "[0-9]+,[0-9][0-9]"
//!:informationtoaccountowner                    fold single
//!::tag                   ":86:"               drop
//!::debit                 "D"                  drop
//!::debitentries       6
//!::credit                "C"                  drop
//!::creditentries      6
//!::debit                 "D"                  drop
//!::debitamount           "[0-9]+,[0-9][0-9]"
//!::credit                "C"                  drop
//!::creditamount          "[0-9]+,[0-9][0-9]"  drop
//!::accountname           "(\n[^-:][^\n]*)*"   drop
//!:messagetrailer                     mandatory      single
//!::start                 "-"
//!::end                   "XXX"
//![Tabular description end]
array|mapping compile(string|Stdio.File|Stdio.FILE input)
{ if(!input)
    input="";
  if(stringp(input))
    input=Stdio.FakeFile(input);
  if(!input->unread) {
    Stdio.FILE tmpf = Stdio.FILE();
    tmpf->assign(input);
    input = tmpf;
  }
  int started=0;
  int lineno=0;
  string beginend="Tabular description ";
  array fields=
   ({"level","name","width",0,"delim",0,"match",0,"options","comment"});
  array strip=({"name","width","delim","match","options","comment"});
  int garbage=0;

  mapping getline()
  { mapping m;
    if(started>=0)
      for(;;)
      { string line=input->gets();
        if(!line)
          error("Missing begin record\n");
        array res=descrx->split(line);
        lineno++;
        if(!res)
          if(!started)
          { if(!garbage)
            { garbage=1;
              werror("Skipping garbage lines... %O\n",line);
            }
            continue;
          }
          else
            error("Line %d parse error: %O\n",lineno,line);
        m=mkmapping(fields,res);
        m_delete(m,0);
        m->level=sizeof(m->level);
        foreach(strip,string s)
          if(m[s]&&!sizeof(m[s])||!m[s]&&intp(m[s]))
             m_delete(m,s);
        if(!started)
        { if(!m->level&&!m->name&&m->delim&&m->delim==beginend+"begin")
            started=1;
          continue;
        }
        if(!m->level&&!m->name)
        { if(m->delim==beginend+"end")
          { started=-1;
            break;
          }
	  if(!m->comment||m->comment&&
	   (has_prefix(m->comment,"#")||has_prefix(m->comment,";")))
            continue;	      // skip comments and empty lines
	}
        if(m->options)
        { mapping options=([]);
          array sp;
          string left=m->options;
          m_delete(m,"options");
          while(sp=tokenise->split(left))
             options[sp[0]]=1, left=sp[1];
           m+=options;
        }
        if(m->match)
           m->match=parsecstring(m->match);
        if(m->delim)
        { multiset delim=(<>);
          foreach(parsecstring(replace(m->delim,"\"","\\\""))/"", string cs)
          delim[cs[0]]=1;
          m->delim=delim;
        }
        if(m->width)
          m->width=(int)m->width;
        m_delete(m,"comment");
        break;
      }
    return m;
  };

  mapping m;

  array|mapping getlevel()
  { array|mapping cur=({});
    cur=({m-(<"level">),0});
    int lastlevel=m->level+1;
    m=0;
    for(;(m || (m=getline())) && lastlevel<=m->level;)
      if(lastlevel==m->level && sizeof(m&(<"delim","match","width">)))
        cur+=({m-(<"level">)}),m=0;
      else
      { array|mapping res=getlevel();
        if(mappingp(res))
        { if(mappingp(cur[sizeof(cur)-1]))
          { cur[sizeof(cur)-1]+=res;
            continue;
          }
          res=({res});
        }
        cur+=res;
      }
    catch
    { if(arrayp(cur) && arrayp(cur[2]))
        return ([cur[0]->name:cur[2..]]);
    };
    return ({cur});
  };

  array|mapping ret;
  m=getline();
  while(started>=0 && m)
  { array|mapping val=getlevel();
    catch
    { ret+=val;
      continue;
    };
    ret=val;
  }
  return ret;
}

private string parsecstring(string s)
{ return compile_string("string s=\""+s+"\";")()->s;
}
