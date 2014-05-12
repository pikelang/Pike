/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Preprocessor template.
 * Based on cpp.c 1.45
 *
 * 1999-02-27 Henrik Grubbström
 */


static ptrdiff_t low_cpp(struct cpp *this,
                           PCHARP data,
			   ptrdiff_t len,
			   int flags,
			   int auto_convert,
			   struct pike_string *charset)
{
  ptrdiff_t pos, tmp, e;
  int include_mode;
  INT_TYPE first_line = this->current_line;
  /* FIXME: What about this->current_file? */

  for(pos=0; pos<len;)
  {
    ptrdiff_t old_pos = pos;
    int c;
/*    fprintf(stderr,"%c",DATA(pos));
      fflush(stderr); */

    switch(c = DATA(pos++))
    {
      case '\n':
        if(flags & CPP_END_AT_NEWLINE) return pos-1;

/*      fprintf(stderr,"CURRENT LINE: %d\n",this->current_line); */
        this->current_line++;
        PUTNL();
        goto do_skipwhite;

      case 0x1b: case 0x9b:	/* ESC or CSI */
        /* Assume ANSI/DEC escape sequence.
         * Format supported:
         * 	<ESC>[\040-\077]+[\100-\177]
         * or
         * 	<CSI>[\040-\077]*[\100-\177]
         */
        /* FIXME: This place is far from enough to make these things
         * behave as whitespace. /mast */
        while ((tmp = DATA(pos)) && (tmp == ((tmp & 0x1f)|0x20))) {
          pos++;
        }
        if (tmp == ((tmp & 0x3f)|0x40)) {
          pos++;
        } else {
          /* FIXME: Warning here? */
        }

        PUTC(' ');
        break;

      case '\t':
      case ' ':
      case '\r':
        PUTC(' ');

      do_skipwhite:
        while( wide_isspace(c=DATA(pos)) && c != '\n' && c != '\r' )
          pos++;
        break;

        /* Minor optimization */
      case '<': case '=': case '>':
	if(DATA(pos)==c  &&
	   DATA(pos+1)==c  &&
	   DATA(pos+2)==c  &&
	   DATA(pos+3)==c  &&
	   DATA(pos+4)==c  &&
	   DATA(pos+5)==c) {
	  cpp_error(this, "Merge conflict detected.");
	  PUTC(c);
	  do {
	    PUTC(c);
	  } while (DATA(++pos) == c);
	  continue;
	}
	/* FALL_THROUGH */

      case '!': case '@': case '$': case '%': case '^': case '&':
      case '*': case '(': case ')': case '-': case '+':
      case '{': case '}': case ':': case '?': case '`': case ';':
      case ',': case '.': case '~': case '[': case ']': case '|':
	PUTC(DATA(pos-1));
        break;

      case '\\':
        if(DATA(pos)=='\n') {
          pos++;
          this->current_line++;
          PUTNL();
          goto do_skipwhite;
        }
        else if ((DATA(pos) == '\r') && (DATA(pos+1) == '\n')) {
          pos += 2;
          this->current_line++;
          PUTNL();
          goto do_skipwhite;
        }
        /* Fall through - identifiers might begin with \uxxxx. */

      default:
        if(OUTP())
        {
          struct pike_string *s;
          struct define *d=0;

          pos--;
          s = GOBBLE_IDENTIFIER();
          if (!s) {
            PUTC (DATA(pos++));
            break;
          }

          if(flags & CPP_DO_IF && s == defined_str)
          {
            /* NOTE: defined() must be handled here, since its argument
             *       must not be macro expanded.
             */
            d = defined_macro;
          }else{
            d=FIND_DEFINE(s);
          }

          if(d && !(d->inside & 1))
          {
            int arg=0;
            INT_TYPE start_line = this->current_line;
            struct string_builder tmp;
            struct define_argument arguments[ MAX_ARGS];
            short inside = d->inside;

            if (d == defined_macro) {
              free_string (s);
              s = NULL;
            }

            if(d->args>=0)
            {
              SKIPWHITE();

              if(!GOBBLE('('))
              {
                if (s) {
                  string_builder_shared_strcat(&this->buf,s);
                  free_string(s);
                }
                /* Restore the post-whitespace. */
                string_builder_putchar(&this->buf, ' ');
                break;
              }

              for(arg=0;arg<d->args;arg++)
              {
                if(arg && DATA(pos)==',')
                {
                  pos++;
                  SKIPWHITE();
                }else{
                  SKIPWHITE();
                  if(DATA(pos)==')')
                  {
                    if((d->varargs && arg + 1 == d->args) ||
                       (!arg && (d->args == 1))) {
                      /* Allow varargs to be left out.
                       *
                       * Allow a single argument to be left out.
                       */
                      arguments[arg].arg = ADD_PCHARP(data,pos);
                      arguments[arg].len=0;
                      continue;
                    }else{
                      cpp_error_sprintf(this,
                                        "Too few arguments to macro %S, expected %d.",
                                        d->link.s, d->args);
                      break;
                    }
                  }
                }
                arguments[arg].arg = ADD_PCHARP(data,pos);

                while(1)
                {
                  if(pos+1>len)
                  {
                    INT_TYPE save_line = this->current_line;
                    this->current_line = start_line;
                    cpp_error(this, "End of file in macro call.");
                    this->current_line = save_line;
                    break;
                  }

                  switch(DATA(pos++))
                  {
                    case '\n':
                      this->current_line++;
                      PUTNL();
                    default: continue;

                    case '"':
                      /* Note: Strings may contain \-escaped newlines.
                       *       They must be removed on insertion to
                       *       avoid being counted twice.
                       */
                      if(DATA(pos-2)!='#') {
                        FIND_END_OF_STRING();
                      }else{
                        FIND_END_OF_STRING2();  /* Newlines allowed */
                      }
                      continue;

                    case '\'':
                      FIND_END_OF_CHAR();
                      continue;

                    case '/':
                      if (DATA(pos) == '*') {
                        pos++;
                        if (this->keep_comments) {
                          SKIPCOMMENT_INC_LINES();
                          goto ADD_TO_BUFFER;
                        }
                        SKIPCOMMENT();
                      } else if (DATA(pos) == '/') {
                        if (this->keep_comments) {
                          FIND_EOL_PRETEND();
                          goto ADD_TO_BUFFER;
                        }
                        FIND_EOL();
                      }
                      continue;

                    case '(':
                      pos=find_end_parenthesis(this, data, len, pos);
                      continue;

                    case '{':
                      pos=find_end_brace(this, data, len, pos);
                      continue;

                    case ',':
                      if(d->varargs && arg+1 == d->args) continue;
                      /* FALL_THROUGH */

                    case ')':
                      pos--;
                      break;
                  }
                  break;
                }
                arguments[arg].len = SUBTRACT_PCHARP(ADD_PCHARP(data,pos),arguments[arg].arg);
              }
              SKIPWHITE();
              if(!GOBBLE(')')) {
                this->current_line = start_line;
                cpp_error_sprintf(this, "Missing ) in the macro %S.",
                                  d->link.s);
              }
            }

            if(d->args >= 0 && arg != d->args)
              cpp_error(this, "Wrong number of arguments to macro.");

            init_string_builder(&tmp, 0);
            if(d->magic)
            {
              d->magic(this, d, arguments, &tmp);
            }else{
              string_builder_shared_strcat(&tmp, d->first);
              for(e=0;e<d->num_parts;e++)
              {
                PCHARP a;
                ptrdiff_t l;

                if((d->parts[e].argument & DEF_ARG_MASK) >= arg)
                {
                  cpp_error(this, "Macro not expanded correctly.");
                  continue;
                }

                a = (arguments[d->parts[e].argument&DEF_ARG_MASK].arg);
                l = arguments[d->parts[e].argument&DEF_ARG_MASK].len;

                if (d->parts[e].argument&DEF_ARG_NEED_COMMA
                    && !(d->varargs && d->args-1
                         == (d->parts[e].argument&DEF_ARG_MASK) && l == 0)) {
		  string_builder_putchar(&tmp, ',');
		  string_builder_putchar(&tmp, ' ');
                }

                if(!(d->parts[e].argument & DEF_ARG_NOPRESPACE))
                  string_builder_putchar(&tmp, ' ');

                if(d->parts[e].argument & DEF_ARG_STRINGIFY)
                {
                  /* NOTE: At entry a[0] is non white-space. */
                  int e = 0;
                  string_builder_putchar(&tmp, '"');
                  for(e=0; e<l;) {
                    if (WC_ISSPACE(INDEX_PCHARP(a,e)) || 
                        INDEX_PCHARP(a,e)=='"' || INDEX_PCHARP(a,e)=='\\') {
                      if (e) {
                        string_builder_append(&tmp, a, e);
                      }
                      if (INDEX_PCHARP(a,e) == '"' || INDEX_PCHARP(a,e)=='\\') {
                        /* String or quote. */
                        string_builder_putchar(&tmp, '\\');
                        string_builder_putchar(&tmp, INDEX_PCHARP(a,e));
                        if (INDEX_PCHARP(a,e) == '"') {
                          for (e++; (e < l) && (INDEX_PCHARP(a,e) != '"'); e++) {
                            string_builder_putchar(&tmp, INDEX_PCHARP(a,e));
                            if (INDEX_PCHARP(a,e) == '\\') {
                              string_builder_putchar(&tmp, '\\');
                              string_builder_putchar(&tmp, INDEX_PCHARP(a,++e));
                              if (INDEX_PCHARP(a,e) == '\\') {
                                string_builder_putchar(&tmp, '\\');
                              }
                            }
                          }
                          string_builder_putchar(&tmp, '\\');
                          string_builder_putchar(&tmp, '"');
                          e++;
                        }
                      } else {
                        /* White space. */
                        while ((e < l) && WC_ISSPACE(INDEX_PCHARP(a,e))) {
                          e++;
                        }
                        if (e != l) {
                          string_builder_putchar(&tmp, ' ');
                        }
                      }
                      INC_PCHARP(a,e);
                      l -= e;
                      e = 0;
                    } else {
                      e++;
                    }
                  }
                  if (l) {
                    string_builder_append(&tmp,a,l);
                  }
                  string_builder_putchar(&tmp, '"');
                }else{
                  if(DEF_ARG_NOPRESPACE)
                    while(l && WC_ISSPACE(EXTRACT_PCHARP(a)))
                      INC_PCHARP(a,1),l--;

                  if(DEF_ARG_NOPOSTSPACE)
                    while(l && WC_ISSPACE(INDEX_PCHARP(a,l-1)))
                      l--;

                  if(d->parts[e].argument & (DEF_ARG_NOPRESPACE | DEF_ARG_NOPOSTSPACE))
                  {
                    string_builder_append( &tmp, a, l );
                  }else{
                    struct string_builder save;
                    INT_TYPE line = this->current_line;
                    /* FIXME: Shouldn't we save current_file too? */
                    save=this->buf;
                    this->buf=tmp;
                    d->inside = 2;
                    low_cpp(this, a, l,
			    flags & ~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE),
			    auto_convert, charset);
                    d->inside = inside;
                    tmp=this->buf;
                    this->buf=save;
                    this->current_line=line;
                  }
                }

                if(!(d->parts[e].argument & DEF_ARG_NOPOSTSPACE))
                  string_builder_putchar(&tmp, ' ');

                string_builder_shared_strcat(&tmp, d->parts[e].postfix);
              }
            }

            /* Remove any newlines from the completed expression. */
            if (!(d->magic == insert_callback_define ||
                  d->magic == insert_callback_define_no_args ||
                  d->magic == insert_pragma))

              for(e=0; e< (ptrdiff_t)tmp.s->len; e++)
                if(index_shared_string(tmp.s,e)=='\n')
                  SET_INDEX_CHARP(tmp.s->str,e,tmp.s->size_shift,' ');

            if(s) d->inside=1;

            string_builder_putchar(&tmp, 0);
            tmp.s->len--;
            low_cpp(this, MKPCHARP_STR(tmp.s),tmp.s->len,
                    flags & ~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE),
                    auto_convert, charset);

            if(s)
            {
              if((d=FIND_DEFINE(s)))
                d->inside = inside;

              free_string(s);
            }

            free_string_builder(&tmp);
          }else{
            if (OUTP())
              string_builder_shared_strcat (&this->buf, s);
            free_string (s);
          }
        }
        break;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        PUTC(DATA(pos-1));
        while(DATA(pos)>='0' && DATA(pos)<='9') PUTC(DATA(pos++));
        break;

      case '"':
        FIXSTRING(this->buf,OUTP());
        break;

      case '\'':
        tmp=pos-1;
        FIND_END_OF_CHAR();
        if(OUTP())
          string_builder_append( &this->buf, ADD_PCHARP(data, tmp), pos - tmp);
        else
          for (; tmp < pos; tmp++)
            if (DATA(tmp) == '\n')
              string_builder_putchar (&this->buf, '\n');
        break;

      case '/':
        if(DATA(pos)=='/')
        {
          if (this->keep_comments) {
            FIND_EOL_PRETEND();
            goto ADD_TO_BUFFER;
          }

          FIND_EOL();
          break;
        }

        if(DATA(pos)=='*')
        {
          if (this->keep_comments) {
            SKIPCOMMENT_INC_LINES();
            goto ADD_TO_BUFFER;
          } else {
            PUTC(' ');
            SKIPCOMMENT();
          }
          break;
        }

        PUTC(DATA(pos-1));
        break;

      case '#':
        if(GOBBLE('!'))
        {
          FIND_EOL();
          break;
        }
        SKIPSPACE();

        if (!CHECK_WORD(string_recur_, NELEM(string_recur_))
            && !CHECK_WORD(include_recur_, NELEM(include_recur_)))
        {
          if (this->prefix)
          {
            if( !begins_with( this->prefix->str, ADD_PCHARP(data,pos), this->prefix->len, len-pos, 0 ) ||
                DATA(pos+this->prefix->len) != '_')
            {
              FIND_EOS();
              goto ADD_TO_BUFFER;
            }
            pos += this->prefix->len + 1;
          } else {
            int i;

            for (i = pos; i < len; i++) {
              if (DATA(i) == '_') {
                FIND_EOS();
                goto ADD_TO_BUFFER;
              } else if (!WC_ISIDCHAR(DATA(i)))
                break;
            }
          }
        }

        switch(DATA(pos))
        {
          case 'l':
            {
              if(GOBBLE_WORD(line_))
              {
                /* FIXME: Why not use SKIPSPACE()? */
                /* Because SKIPSPACE skips newlines? - Hubbe */
                /* Actually, no - Per */
                while(DATA(pos)==' ' || DATA(pos)=='\t') pos++;
              }else{
                goto unknown_preprocessor_directive;
              }
              /* Fall through */
            }
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
            {
              INT_TYPE new_lineno;
              PCHARP foo = ADD_PCHARP(data,pos);
              new_lineno=STRTOL_PCHARP(foo, &foo, 10)-1;
              if(OUTP())
              {
                string_builder_binary_strcat(&this->buf, "#line ", 6);
                string_builder_append(&this->buf, ADD_PCHARP(data,pos),
                                      SUBTRACT_PCHARP(foo,ADD_PCHARP(data,pos)));
              }
              pos = SUBTRACT_PCHARP(foo,data);
              SKIPSPACE();

              if(DATA(pos)=='"')
              {
                struct string_builder nf;
                init_string_builder(&nf, 0);

                READSTRING(nf);

                if(OUTP())
                {
                  free_string(this->current_file);
                  this->current_file = finish_string_builder(&nf);

                  string_builder_putchar(&this->buf, ' ');
                  PUSH_STRING_SHIFT (this->current_file->str, this->current_file->len,
                                     this->current_file->size_shift, &this->buf);
                }else{
                  free_string_builder(&nf);
                }
              }

              if (OUTP())
                this->current_line = new_lineno;

              FIND_EOL();
              break;
            }

          case '"':
            {
              struct string_builder nf;
              init_string_builder(&nf, 0);

              READSTRING2(nf);
              if(OUTP())
              {
                PUSH_STRING_SHIFT(nf.s->str, nf.s->len,nf.s->size_shift, &this->buf);
              }
              free_string_builder(&nf);
              break;
            }

          case 's':
            {
              if(GOBBLE_WORD(string_))
              {
                include_mode = 1;
                goto do_include;
              }
              if(GOBBLE_WORD(string_recur_))
              {
                include_mode = 3;
                goto do_include;
              }
            }
            goto unknown_preprocessor_directive;

          case 'i': /* include, if, ifdef */
            {
              int recur = 0;

              if(GOBBLE_WORD(include_) || (recur = GOBBLE_WORD(include_recur_)))
              {
                if (recur) {
                  include_mode = 2;
                } else {
                  include_mode = 0;
                }
              do_include:
                {
                  struct svalue *save_sp=Pike_sp;
                  SKIPSPACE();

                  check_stack(3);

                  switch(DATA(pos++))
                  {
                    case '"':
                      {
                        struct string_builder nf;
                        init_string_builder(&nf, 0);
                        pos--;
                        READSTRING(nf);
                        push_string(finish_string_builder(&nf));
                        if (!TEST_COMPAT(7,6)) {
                          /* In Pike 7.7 and later filenames belonging to Pike
                           * are assumed to be encoded according to UTF-8.
                           */
                          f_string_to_utf8(1);
                        }
                        ref_push_string(this->current_file);
                        push_int(1);
                        break;
                      }

                    case '<':
                      {
                        ptrdiff_t tmp = pos;
                        while(DATA(pos)!='>')
                        {
                          if(DATA(pos)=='\n')
                          {
                            cpp_error(this, "Expecting '>' in include.");
                            break;
                          }
                          pos++;
                        }
                        push_string(make_shared_binary_pcharp(ADD_PCHARP(data,tmp), pos-tmp));
                        if (!TEST_COMPAT(7,6)) {
                          /* In Pike 7.7 and later filenames belonging to Pike
                           * are assumed to be encoded according to UTF-8.
                           */
                          f_string_to_utf8(1);
                        }
                        ref_push_string(this->current_file);
                        pos++;
                        push_int(0);
                        break;
                      }

                    default:
                      if (include_mode & 2) {
                        /* Macro expanding didn't help... */
                        cpp_error(this, "Expected file to include.");
                        break;
                      } else {
                        /* Try macro expanding (Bug 2440). */
                        struct string_builder save = this->buf, tmp;
                        INT_TYPE save_line = this->current_line;
                        init_string_builder(&this->buf, 0);

                        /* Prefix the buffer with the corresponding *_recur
                         * directive, to avoid infinite loops.
                         */
                        if (include_mode & 1) {
                          string_builder_strcat(&this->buf, "#string_recur ");
                        } else {
                          string_builder_strcat(&this->buf, "#include_recur ");
                        }

                        pos--;
                        pos += low_cpp(this, ADD_PCHARP(data,pos), len - pos,
                                       CPP_END_AT_NEWLINE,
                                       auto_convert, charset);

                        string_builder_putchar(&this->buf, '\n');

                        tmp = this->buf;
                        this->buf = save;

                        /* We now have a #include-recur or #string-recur directive
                         * in tmp. Preprocess it.
                         */

                        /* We're processing the line twice. */
                        this->current_line = save_line;
                        low_cpp(this, MKPCHARP_STR(tmp.s), tmp.s->len,
                                flags, auto_convert, charset);
                        free_string_builder(&tmp);

                        this->current_line = save_line;
                        string_builder_sprintf(&this->buf, "\n#line %ld ",
                                               (long)save_line);
                        PUSH_STRING_SHIFT(this->current_file->str,
                                          this->current_file->len,
                                          this->current_file->size_shift,
                                          &this->buf);
                        string_builder_putchar(&this->buf, '\n');
                      }
                      break;
                  }

                  if(Pike_sp==save_sp) {
                    break;
                  }

                  if(OUTP())
                  {
                    struct pike_string *new_file;

                    if (!safe_apply_handler ("handle_include",
                                             this->handler, this->compat_handler,
                                             3, BIT_STRING) ||
                        !(new_file=Pike_sp[-1].u.string) ) {
                      cpp_handle_exception (this, "Couldn't find include file.");
                      pop_n_elems(Pike_sp-save_sp);
                      break;
                    }

                    ref_push_string(new_file);

                    if (!safe_apply_handler ("read_include",
                                             this->handler, this->compat_handler,
                                             1, BIT_STRING|BIT_INT)) {
                      cpp_handle_exception (this, "Couldn't read include file.");
                      pop_n_elems(Pike_sp-save_sp);
                      break;
                    } else if (TYPEOF(Pike_sp[-1]) == PIKE_T_INT) {
                      cpp_error_sprintf(this, "Couldn't read include file \"%S\".",
                                        new_file);
                      pop_n_elems(Pike_sp-save_sp);
                      break;
                    }

                    {
                      char buffer[47];
                      struct pike_string *save_current_file;
                      INT_TYPE save_current_line;

                      save_current_file=this->current_file;
                      save_current_line=this->current_line;
                      copy_shared_string(this->current_file,new_file);
                      this->current_line=1;

                      string_builder_binary_strcat(&this->buf, "#line 1 ", 8);
                      PUSH_STRING_SHIFT(new_file->str, new_file->len,
                                        new_file->size_shift, &this->buf);
                      string_builder_putchar(&this->buf, '\n');
                      if(include_mode & 1)
                      {
                        /* #string */
                        struct pike_string *str = Pike_sp[-1].u.string;
                        PUSH_STRING_SHIFT(str->str, str->len, str->size_shift,
                                          &this->buf);
                      }else{
                        /* #include */
                        if (auto_convert) {
                          struct pike_string *new_str =
                            recode_string(this, Pike_sp[-1].u.string);
                          free_string(Pike_sp[-1].u.string);
                          Pike_sp[-1].u.string = new_str;
                        } else if (charset) {
                          ref_push_string(charset);
                          if (!safe_apply_handler ("decode_charset",
                                                   this->handler, this->compat_handler,
                                                   2, BIT_STRING)) {
                            cpp_handle_exception (this,
                                                  "Charset decoding failed for included file.");
                            pop_n_elems(Pike_sp - save_sp);
                            break;
                          }
                        }
                        if (Pike_sp[-1].u.string->size_shift) {
                          /* Get rid of any byte order marks (0xfeff) */
                          struct pike_string *new_str = filter_bom(Pike_sp[-1].u.string);
                          free_string(Pike_sp[-1].u.string);
                          Pike_sp[-1].u.string = new_str;
                        }
                        low_cpp(this,
                                MKPCHARP_STR(Pike_sp[-1].u.string),
                                Pike_sp[-1].u.string->len,
                                flags&~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE),
                                auto_convert, charset);
                      }

                      free_string(this->current_file);
                      this->current_file=save_current_file;
                      this->current_line=save_current_line;

                      sprintf(buffer,"\n#line %ld ", (long)this->current_line);
                      string_builder_binary_strcat(&this->buf, buffer, strlen(buffer));
                      PUSH_STRING_SHIFT(this->current_file->str,
                                        this->current_file->len,
                                        this->current_file->size_shift,
                                        &this->buf);
                      string_builder_putchar(&this->buf, '\n');
                      if ((include_mode & 2) && (pos < len)) {
                        /* NOTE: The rest of the current buffer has already been
                         *       expanded once.
                         */
                        string_builder_append(&this->buf, ADD_PCHARP(data, pos), len - pos);
                        pos = len;
                      }
                    }
                  }

                  pop_n_elems(Pike_sp-save_sp);
                  break;
                }
              }

              if(GOBBLE_WORD(if_))
              {
                struct string_builder save, tmp;
                INT32 nflags = 0;
#ifdef PIKE_DEBUG
                ptrdiff_t skip;
#endif /* PIKE_DEBUG */

                if(!OUTP())
                  nflags = CPP_REALLY_NO_OUTPUT;

                save=this->buf;
                init_string_builder(&this->buf, 0);
                pos += low_cpp(this, ADD_PCHARP(data,pos), len-pos,
                               nflags | CPP_END_AT_NEWLINE | CPP_DO_IF,
                               auto_convert, charset);
                tmp=this->buf;
                this->buf=save;

                string_builder_putchar(&tmp, 0);
                tmp.s->len--;

                if (!nflags) {
                  calc(this,MKPCHARP_STR(tmp.s),tmp.s->len,0,0);
                  if(SAFE_IS_ZERO(Pike_sp-1)) nflags|=CPP_NO_OUTPUT;
                  pop_stack();
                }
                free_string_builder(&tmp);
                pos += low_cpp(this, ADD_PCHARP(data,pos), len-pos,
                               nflags | CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF,
                               auto_convert, charset);
                break;
              }

              if(GOBBLE_WORD(ifdef_))
              {
                INT32 nflags;
                struct pike_string *s;
                SKIPSPACE();

                s = GOBBLE_IDENTIFIER();
                if(!s)
                  cpp_error(this, "#ifdef what?");

                nflags = CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF | CPP_NO_OUTPUT;
                if(!OUTP())
                  nflags|=CPP_REALLY_NO_OUTPUT;
                if (s) {
                  if(FIND_DEFINE(s))
                    nflags&=~CPP_NO_OUTPUT;
                  free_string (s);
                }

                pos += low_cpp(this, ADD_PCHARP(data,pos), len-pos, nflags,
                               auto_convert, charset);
                break;
              }

              if(GOBBLE_WORD(ifndef_))
              {
                INT32 nflags;
                struct pike_string *s;
                SKIPSPACE();

                s = GOBBLE_IDENTIFIER();
                if(!s)
                  cpp_error(this, "#ifndef what?");

                nflags=CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF;
                if(!OUTP())
                  nflags|=CPP_REALLY_NO_OUTPUT;
                if (s) {
                  if(FIND_DEFINE(s))
                    nflags|=CPP_NO_OUTPUT;
                  free_string (s);
                }

                pos += low_cpp(this, ADD_PCHARP(data,pos), len-pos, nflags,
                               auto_convert, charset);
                break;
              }

              goto unknown_preprocessor_directive;
            }
          case 'e': /* endif, else, elif, error */
            {
              if(GOBBLE_WORD(endif_))
              {
                if(!(flags & CPP_EXPECT_ENDIF))
                  cpp_error(this, "Unmatched #endif.");

                return pos;
              }

              if(GOBBLE_WORD(else_))
              {
                if(!(flags & CPP_EXPECT_ELSE))
                  cpp_error(this, "Unmatched #else.");

                flags&=~CPP_EXPECT_ELSE;
                flags|=CPP_EXPECT_ENDIF;

                if(flags & CPP_NO_OUTPUT)
                  flags&=~CPP_NO_OUTPUT;
                else
                  flags|=CPP_NO_OUTPUT;

                break;
              }

              if(GOBBLE_WORD(elif_) || GOBBLE_WORD(elseif_))
              {
                if(!(flags & CPP_EXPECT_ELSE))
                  cpp_error(this, "Unmatched #elif.");

                flags|=CPP_EXPECT_ENDIF;

                if((flags & (CPP_NO_OUTPUT | CPP_REALLY_NO_OUTPUT)) == CPP_NO_OUTPUT)
                {
                  struct string_builder save,tmp;
                  save=this->buf;
                  init_string_builder(&this->buf, 0);
                  pos += low_cpp(this, ADD_PCHARP(data,pos), len-pos,
                                 CPP_END_AT_NEWLINE | CPP_DO_IF,
                                 auto_convert, charset);
                  tmp=this->buf;
                  this->buf=save;

                  string_builder_putchar(&tmp, 0);
                  tmp.s->len--;

                  calc(this,MKPCHARP_STR(tmp.s),tmp.s->len,0,0);
                  free_string_builder(&tmp);
                  if(!SAFE_IS_ZERO(Pike_sp-1)) flags&=~CPP_NO_OUTPUT;
                  pop_stack();
                } else {
                  FIND_EOL();
                  flags |= CPP_NO_OUTPUT | CPP_REALLY_NO_OUTPUT;
                }
                break;
              }

              if(GOBBLE_WORD(error_))
              {
                ptrdiff_t foo;

                SKIPSPACE();
                foo=pos;
                FIND_EOL();
                if(OUTP())
                {
                  push_string(make_shared_binary_pcharp(ADD_PCHARP(data,foo), pos-foo));
                  cpp_error_sprintf(this, "%O", Pike_sp-1);
                }
                break;
              }
            }
            goto unknown_preprocessor_directive;

          case 'd': /* define */
            {

              if(GOBBLE_WORD(define_))
              {
                struct string_builder str;
                INT32 argno=-1;
                ptrdiff_t tmp3;
                struct pike_string *def_name;
                struct define *def;
                struct svalue *partbase,*argbase=Pike_sp;
                int varargs=0;

                SKIPSPACE();

                def_name = GOBBLE_IDENTIFIER();
                if(!def_name) {
                  cpp_error(this, "Define what?");
                  FIND_EOL();
                  break;
                }

                if(GOBBLE('('))
                {
                  argno=0;
                  SKIPWHITE();

                  while(DATA(pos)!=')')
                  {
                    struct pike_string *arg_name;
                    if(argno)
                    {
                      if(!GOBBLE(','))
                        cpp_error(this,
                                  "Expecting comma in macro definition.");
                      SKIPWHITE();
                    }
                    if(varargs)
                      cpp_error(this,"Expected ) after ...");

                    arg_name = GOBBLE_IDENTIFIER();
                    if(!arg_name)
                    {
                      cpp_error(this, "Expected argument for macro.");
                      break;
                    }

                    check_stack(1);
                    push_string(arg_name);

                    SKIPWHITE();
                    argno++;
                    if(argno>=MAX_ARGS)
                    {
                      cpp_error(this, "Too many arguments in macro definition.");
                      pop_stack();
                      argno--;
                    }

                    if(DATA(pos)=='.' && DATA(pos+1)=='.' && DATA(pos+2)=='.')
                    {
                      varargs=1;
                      pos+=3;
                      SKIPWHITE();
                    }
                  }

                  if(!GOBBLE(')'))
                    cpp_error(this, "Missing ) in macro definition.");
                }

                SKIPSPACE();

                partbase=Pike_sp;
                init_string_builder(&str, 0);

                while(1)
                {
                  INT32 extra=0;
                  ptrdiff_t old_pos = pos;

                  switch(DATA(pos++))
                  {
                    case '/':
                      if(DATA(pos)=='/')
                      {
                        if (this->keep_comments) {
                          FIND_EOL_PRETEND();
                          string_builder_append( &str, ADD_PCHARP(data, old_pos), pos-old_pos );
                          continue;
                        }
                        string_builder_putchar(&str, ' ');
                        FIND_EOL();
                        continue;
                      }

                      if(DATA(pos)=='*')
                      {
                        if (this->keep_comments) {
                          SKIPCOMMENT_INC_LINES();
                          string_builder_append( &str, ADD_PCHARP(data, old_pos), pos-old_pos );
                          continue;
                        }
                        PUTC(' ');
                        SKIPCOMMENT();
                        continue;
                      }

                      string_builder_putchar(&str, '/');
                      continue;

                    case '0': case '1': case '2': case '3':	case '4':
                    case '5': case '6': case '7': case '8':	case '9':
                      string_builder_putchar(&str, DATA(pos-1));
                      while(DATA(pos)>='0' && DATA(pos)<='9')
                        string_builder_putchar(&str, DATA(pos++));
                      continue;

                    case '\\':
                      if(GOBBLE('\n'))
                      {
                        this->current_line++;
                        PUTNL();
                        continue;
                      }
                      if (DATA(pos) == '\r' && DATA(pos+1) == '\n') {
                        pos += 2;
                        this->current_line++;
                        PUTNL();
                        continue;
                      }
                      /* Identifiers might start with \uxxxx, so try to parse
                       * an identifier now. */
                      goto gobble_identifier_in_define;

                    case ',':
                      {
                        int oldpos;

                        oldpos = pos;
                        SKIPSPACE_PRETEND();

                        if (DATA(pos) == '#' && DATA(pos+1) == '#') {
                          extra|= DEF_ARG_NEED_COMMA;
                          pos += 2;
                          goto concat_identifier;
                        } else {
                          pos = oldpos;
                          goto gobble_identifier_in_define;
                        }
                      }
                    case '#':
                      if(GOBBLE('#'))
                      {
                        extra= (extra & DEF_ARG_NEED_COMMA) | DEF_ARG_NOPRESPACE;

                        while(str.s->len && wide_isspace(index_shared_string(str.s,str.s->len-1)))
                          str.s->len--;
                      concat_identifier:
                        if(!str.s->len && Pike_sp-partbase>1)
                        {
#ifdef PIKE_DEBUG
                          if(TYPEOF(Pike_sp[-1]) != PIKE_T_INT)
                            Pike_fatal("Internal error in CPP\n");
#endif
                          Pike_sp[-1].u.integer|=DEF_ARG_NOPOSTSPACE;
                        }
                      }else{
                        extra=DEF_ARG_STRINGIFY;
                      }
                      SKIPSPACE();
                      pos++;
                      /* fall through */

                    gobble_identifier_in_define:
                    default: {
                      struct pike_string *s;
                      pos--;
                      s = GOBBLE_IDENTIFIER();
                      if (s)
                      {
                        tmp3=pos-1;
                        if(argno>0)
                        {
                          if (s->refs > 1)
                          {
                            for(e=0;e<argno;e++)
                            {
                              if(argbase[e].u.string == s)
                              {
                                check_stack(2);
                                push_string(finish_string_builder(&str));
                                init_string_builder(&str, 0);
                                push_int(DO_NOT_WARN(e | extra));
                                extra=0;
                                break;
                              }
                            }
                            if(e!=argno) {
                              free_string (s);
                              continue;
                            }
                          }
                        }
                        string_builder_shared_strcat (&str, s);
                        free_string (s);
                      }else{
                        string_builder_putchar(&str, DATA(pos++));
                      }
                      extra=0;
                      continue;
                    }

                    case '"':
                      FIXSTRING(str, 1);
                      continue;

                    case '\'':
                      tmp3=pos-1;
                      FIND_END_OF_CHAR();
                      string_builder_append(&str, ADD_PCHARP(data, tmp3), pos - tmp3);
                      continue;

                    case '\n':
                      PUTNL();
                      this->current_line++;
                    case 0:
                      break;
                  }
                  push_string(finish_string_builder(&str));
                  break;
                }

                if(OUTP())
                {
                  def = alloc_empty_define(def_name, (Pike_sp-partbase)/2);
                  copy_shared_string(def->first, partbase->u.string);
                  def->args=argno;
                  def->varargs=varargs;

                  for(e=0;e<def->num_parts;e++)
                  {
#ifdef PIKE_DEBUG
                    if(TYPEOF(partbase[e*2+1]) != PIKE_T_INT)
                      Pike_fatal("Cpp internal error, expected integer!\n");

                    if(TYPEOF(partbase[e*2+2]) != PIKE_T_STRING)
                      Pike_fatal("Cpp internal error, expected string!\n");
#endif
                    def->parts[e].argument=partbase[e*2+1].u.integer;
                    copy_shared_string(def->parts[e].postfix,
                                       partbase[e*2+2].u.string);
                  }

#ifdef PIKE_DEBUG
                  if(def->num_parts==1 &&
                     (def->parts[0].argument & DEF_ARG_MASK) > MAX_ARGS)
                    Pike_fatal("Internal error in define\n");
#endif
                  {
                    struct define *d;
                    if ((d = FIND_DEFINE(def->link.s)) && (d->inside)) {
                      cpp_error(this,
                                "Illegal to redefine a macro during its expansion.");
                      free_one_define(&(def->link));
                    } else {
                      this->defines=hash_insert(this->defines, & def->link);
                    }
                  }
                }
                else
                  free_string (def_name);
                pop_n_elems(Pike_sp-argbase);
                break;
              }

              goto unknown_preprocessor_directive;
            }
          case 'u': /* undefine */
            {

              /* NOTE: Reuses undefine_ for undef_ */
              if(GOBBLE_WORD(undefine_) || GOBBLE_WORD(undef_))
              {
                struct pike_string *s;
                SKIPSPACE();
                s = GOBBLE_IDENTIFIER();
                if(!s) {
                  cpp_error(this, "Undefine what?");
                  break;
                }
                if(OUTP())
                {
                  undefine(this,s);
                }
                free_string (s);
                break;
              }

              goto unknown_preprocessor_directive;
            }
          case 'c': /* charset */
            {

              if (GOBBLE_WORD(charset_)) {
                ptrdiff_t p;
                struct pike_string *s;

                if (flags & (CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE)) {
                  /* Only allowed at the top-level */
                  cpp_error(this, "#charset directive inside #if/#endif.");
                  /* Skip to end of line */
                  while (DATA(pos) && DATA(pos) != '\n') {
                    pos++;
                  }
                  break;
                }

                SKIPSPACE();

                p = pos;
                while(DATA(pos) && !wide_isspace(DATA(pos))) {
                  pos++;
                }

                if (pos != p) {
                  /* The rest of the string. */
                  push_string( make_shared_binary_pcharp( ADD_PCHARP(data,pos), len-pos ) );
                  /* The charset name */
                  push_string( make_shared_binary_pcharp( ADD_PCHARP(data,p),pos-p));

                  if (!safe_apply_handler ("decode_charset", this->handler, this->compat_handler,
                                           2, BIT_STRING)) {
                    cpp_handle_exception (this, NULL);
                  } else {
                    low_cpp(this, MKPCHARP_STR(Pike_sp[-1].u.string),Pike_sp[-1].u.string->len,
                            flags,auto_convert, charset);
                    pop_stack();
                  }
                  /* FIXME: Is this the correct thing to return? */
                  return len;
                } else {
                  cpp_error(this, "What charset?");
                }
                break;
              }
              goto unknown_preprocessor_directive;
            }
          case 'p': /* pragma */
            {

              if(GOBBLE_WORD(pragma_))
              {
                if(OUTP())
                  string_builder_strcat(&this->buf, "#pragma");
                else
                  FIND_EOL();
                break;
              }
              if(GOBBLE_WORD(pike_))
              {
                if(OUTP())
                {
                  int major, minor;
                  ptrdiff_t tmp;
                  PCHARP ptr;

                  string_builder_strcat(&this->buf, "#pike");
                  tmp= this->buf.s->len;
                  pos += low_cpp(this, ADD_PCHARP(data,pos), len-pos,
                                 CPP_END_AT_NEWLINE | CPP_DO_IF,
                                 auto_convert, charset);

                  ptr=MKPCHARP_STR(this->buf.s);
                  INC_PCHARP(ptr, tmp);

                  major=STRTOL_PCHARP(ptr, &ptr, 10);
                  if(INDEX_PCHARP(ptr,0) == '.')
                  {
                    INC_PCHARP(ptr, 1);
                    minor=STRTOL_PCHARP(ptr, &ptr, 10);
                    cpp_change_compat(this, major, minor);
                  }else{
                    cpp_error(this, "Missing '.' in #pike.");
                    this->compat_minor=0;
                  }
                }
                else
                  FIND_EOL();
                break;
              }
            }
          case 'r': /* require */
            {
              if(GOBBLE_WORD(require_))
              {
                struct string_builder save, tmp;
                save = this->buf;
                init_string_builder(&this->buf, 0);
                pos += low_cpp(this, ADD_PCHARP(data,pos), len-pos,
                               CPP_END_AT_NEWLINE | CPP_DO_IF,
                               auto_convert, charset);
                tmp = this->buf;
                this->buf = save;
                string_builder_putchar(&tmp, 0);
                tmp.s->len--;

                calc(this,MKPCHARP_STR(tmp.s),tmp.s->len,0,0);
                if(SAFE_IS_ZERO(Pike_sp-1)) this->dependencies_fail=1;
                pop_stack();
                free_string_builder(&tmp);
                if(this->dependencies_fail) return pos;
                break;
              }
              goto unknown_preprocessor_directive;
            }
          case 'w': /* warning */
            {
              if(GOBBLE_WORD(warning_))
              {
                ptrdiff_t foo;

                SKIPSPACE();
                foo=pos;
                FIND_EOL();
                if(OUTP())
                {
                  push_string(make_shared_binary_pcharp(ADD_PCHARP(data,foo), pos-foo));
                  cpp_warning(this, "%O", Pike_sp-1);
                }
                break;
              }
              goto unknown_preprocessor_directive;
            }
          default:
            if(!OUTP() && !this->picky_cpp) break;
          unknown_preprocessor_directive:
            {
              struct pike_string *unknown = GOBBLE_IDENTIFIER();
              if (unknown) {
                cpp_error_sprintf(this, "Unknown preprocessor directive %S.",
                                  unknown);
                free_string(unknown);
              } else {
                cpp_error_sprintf(this, "Invalid preprocessor directive character at %d: '%c'.",
                                  pos, DATA(pos));
              }
            }
        }
    }
    continue;
  ADD_TO_BUFFER:
    // keep line
    string_builder_append(&this->buf, ADD_PCHARP(data,old_pos), pos-old_pos);
  }

  if(flags & CPP_EXPECT_ENDIF) {
    INT_TYPE saved_line = this->current_line;
    this->current_line = first_line;
    cpp_error(this, "End of file while searching for #endif.");
    this->current_line = saved_line;
  }

  return pos;
}
