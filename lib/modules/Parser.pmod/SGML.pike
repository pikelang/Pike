/* -*- mode: Pike; c-basic-offset: 3; -*- */

#pike __REAL_VERSION__

//!  	This is a handy simple parser of SGML-like
//!	syntax like HTML. It doesn't do anything advanced,
//!	but finding the corresponding end-tags.
//!
//!	It's used like this:
//! @code
//! array res=Parser.SGML()->feed(string)->finish()->result();
//! @endcode
//!
//!	The resulting structure is an array of atoms,
//!	where the atom can be a string or a tag.
//!	A tag contains a similar array, as data.
//!
//! @example
//!	A string
//!     @expr{"<gat>&nbsp;<gurka>&nbsp;</gurka>&nbsp;<banan>&nbsp;<kiwi>&nbsp;</gat>"@}
//!     results in
//!@code
//!({
//!    tag "gat" object with data:
//!    ({
//!        tag "gurka" object with data:
//!        ({
//!            " "
//!        })
//!        tag "banan" object with data:
//!        ({
//!            " "
//!            tag "kiwi" object with data:
//!            ({
//!               " "
//!            })
//!        })
//!    })
//!})
//!@endcode
//!
//!	ie, simple "tags" (not containers) are not detected,
//!	but containers are ended implicitely by a surrounding
//!	container _with_ an end tag.
//!
//! 	The 'tag' is an object with the following variables:
//!	@pre{
//!	 string name;           - name of tag
//!	 mapping args;          - argument to tag
//!	 int line,char,column;  - position of tag
//!	 int eline,echar,ecolumn;  - end position of tag, src[char..echar-1] got the block. add by Xuesong Guo
//!	 string file;           - filename (see <ref>create</ref>)
//!	 array(SGMLatom) data;  - contained data
//!	 int open;		- is not an empty element and has no end tag. add by Xuesong Guo
//!     @}
//!

  //!
   string file;

   //!
   class SGMLatom
   {
      //!
      string name;
      mapping args;
      int line,char,column;
      int eline,echar,ecolumn;
      string file;
      array(SGMLatom) data=({});
      int open;

      protected string _sprintf(int t, mapping m)
      {
	 if (t=='s')
	 {
	    string res=name;
	    if (sizeof(args))
	       foreach (args; string i; string v)
		  res+=sprintf(" %s=%q",i,v);

	    res="<"+res+">";
	    if (m->indent > 50) {
	      res += "...</" + name + ">";
	    }
	    else {
	      string i=" "*(m->indent + 1);
	      if (sizeof(data)) {
		mapping sub_m = (["indent": m->indent + 1]);
		foreach (data,string|SGMLatom a)
		  if (stringp (a))
		    res += sprintf ("\n%s%s", i, a);
		  else
		    res += "\n" + i + a->_sprintf ('s', sub_m);
	      }
	      res += "\n" + (" " * m->indent) + "</" + name + ">";
	    }

	    return res;
	 }

	 else if (t == 'O')
	   return sprintf ("SGMLatom(%s%s%s)",
			   name,
			   (args && sizeof (args) ?
			    map ((array) args,
				 lambda (array arg) {
				   return sprintf (" %s=%q", arg[0], arg[1]);
				 }) * "" : ""),
			   open ? ", open" : "");
      }
   }

   protected array(array(SGMLatom|string))|zero res=({({})});
   protected array(SGMLatom) tagstack=({});
   protected array(object) errors;

   array(SGMLatom|string) data;

   protected private array(string) got_tag(object g,string s)
   {
      string name=name_formater?name_formater(g->tag_name()):g->tag_name();

      if (name!="" && name[0]=='/')
      {
	 int i=search(tagstack->name,name[1..]);
	 if (i!=-1)
	 {
	    tagstack[i]->open=0;
	    i++;
	    while (i--)
	    {
	       SGMLatom t=tagstack[0];
	       t->data=res[0];
	       [t->eline,t->echar,t->ecolumn]=g->at();
	       res=res[1..];
	       tagstack=tagstack[1..];
	    }
	    return ({});
	 }
      }

      SGMLatom t=SGMLatom();
      t->name=name;
      if(argname_formater==0){
	      t->args=g->tag_args();
      }else{
	      t->args=([]);
	      foreach(g->tag_args;string k;mixed d){
		      t->args[argname_formater(k)]=d;
	      }
      }
      [t->line,t->char,t->column]=g->at();
      t->file=file;
      if(!has_suffix(s,"/>"))
	      t->open=1;
      for(int i=0;i<sizeof(res);i++){
	      //werror("%d:%d\n",sizeof(tagstack),sizeof(res));
	      if(i>=sizeof(tagstack)||tagstack[i]->open){
		      res[i]+=({t});
		      break;
	      }
      }
      tagstack=({t})+tagstack;
      res=({({})})+res;

      return ({}); // don't care
   }

   void debug(array|void arr,void|int level)
   {
      level+=2;
      if (!arr) arr=data;
      foreach (arr,string|SGMLatom t)
	 if (stringp(t))
	    write("%*s%-=*s\n",level,"",79-level,sprintf("%O",t));
	 else
	 {
	    write("%*stag %O\n",level,"",t->name,);
	    if (sizeof(t->args))
	       write("%*s%-=*s\n",level+4,"",75-level,sprintf("%O",t->args));
	    debug(t->data,level);
	 }
   }


   private protected object p=.HTML();

   //! @decl void create()
   //! @decl void create(string filename,function|void name_formater,function|void argname_formater)
   //!	This object is created with this filename.
   //!	It's passed to all created tags, for debug and trace purposes.
   //!  All tag name will be replace as name_formater(name)
   //!  All arg_name will be replace as argname_formater(arg_name)
   //! @note
   //! 	No, it doesn't read the file itself. See @[feed()].

   protected int i;

   function name_formater;
   function argname_formater;
   protected void create(void|string _file,
			 function|void _name_formater,
			 function|void _argname_formater)
   {
      file=_file;
      if(_name_formater)
	      name_formater=_name_formater;
      if(_argname_formater)
	      argname_formater=_argname_formater;

      p->_set_tag_callback(got_tag);
      p->_set_data_callback(lambda(object g,string data)
			    {
			    if (data!="") {
				for(int i=0;i<sizeof(res);i++){
				//werror("%d:%d\n",sizeof(tagstack),sizeof(res));
					if(i>=sizeof(tagstack)||tagstack[i]->open){
						res[i]+=({data});
						break;
						}
				}
			    }
			    return ({}); });
   }

   //! @decl object feed(string s)
   //! @decl array(SGMLatom|string) finish()
   //! @decl array(SGMLatom|string) result(string s)
   //!	Feed new data to the object, or finish the stream.
   //!	No result can be used until @[finish()] is called.
   //!
   //! 	Both @[finish()] and @[result()] return the computed data.
   //!
   //!	@[feed()] returns the called object.

   this_program feed(string s)
   {
      p->feed(s);
      return this;
   }

   array(SGMLatom|string) finish()
   {
      p->finish();
      foreach ( tagstack, SGMLatom a )
      {
	 a->data+=res[0];
	 res=res[1..];
      }
      tagstack=({});
      data=res[0];
      res=0;
      return data;
   }

   array(SGMLatom|string) result()
   {
      return data;
   }

// For compatibility with 7.6 and early 7.8.
constant SGML = this_program;
