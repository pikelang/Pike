#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_functions.h"

#include <errno.h>

#define this ((struct parsebuffer *)(fp->current_storage))
#define buff      (this->left_in_buffer->str)
#define buff_pos  (this->bpos)
#define buff_len  (this->left_in_buffer->len)


struct program *feed_program;
struct parsebuffer {
  struct mapping *so_far;
  struct pike_string *left_in_buffer;
  int bpos; /* Temporary */
  int data_left; /* Only used when the state is 'DATA' */
  enum { INITIAL, HEADERS, DATA } state;
};





/* Are we positioned on an end of line character?
 *   If so, skip it and return true.
 */
static int eol()
{
  int n=0;
  if(buff_pos >= buff_len) return 0; 
  if(buff[buff_pos] == '\n')
  {
    n++;
    ++buff_pos;
    if(buff[buff_pos] == '\r')
    {
      n++;
      ++buff_pos;
    }
  } else if(buff[buff_pos] == '\r') {
    n++;
    ++buff_pos;
    if(buff[buff_pos] == '\n')
    {
      n++;
      ++buff_pos;
    }
  }
  if(buff_pos >= buff_len)
    buff_pos = buff_len;
  return n;
}

static int get_word(char c)
{
  int n=0;
  int old_pos;
  if(buff_pos >= buff_len) return 0; 
  old_pos = buff_pos;
  if(c!=' ')
    while(buff[buff_pos] != c)
    {
      n++;
      if(++buff_pos >= buff_len)
      {
	buff_pos = old_pos;
	return 0; /* Not found.. */
      }
    }
  else
    while(!isspace(buff[buff_pos]))
    {
      n++;
      if(++buff_pos >= buff_len)
      {
	buff_pos = old_pos;
	return 0; /* Still more to come.. */
      }
    }
  if(!n) return 0;
  push_string(make_shared_binary_string(buff+old_pos, n));
  if((buff[buff_pos] != '\n') && (buff[buff_pos] != '\r'))
    buff_pos++;
  return 1;
}

static int to_eol()
{
  int n=0;
  int oldest_pos, old_pos;
  oldest_pos = buff_pos;
  if(buff_pos >= buff_len) return 0; 
  while(isspace(buff[buff_pos]))
    if(++buff_pos >= buff_len)
    {
      buff_pos = oldest_pos;
      return 0;
    }
  old_pos = buff_pos;
  while((buff[buff_pos]!='\r') && (buff[buff_pos]!='\n'))
  {
    n++;
    if(++buff_pos >= buff_len)
    {
      buff_pos = oldest_pos;
      return 0; /* No EOL */
    }
  }
  push_string(make_shared_binary_string(buff+old_pos, n));
  eol();
  return 1;
}



static void f_feed(INT32 args)
{
  struct svalue *data;
  int p=0, t=0;

  if(args!=1) error("Feed me correctly!\n");
  if(sp[-1].type != T_STRING) error("Wrong type of food.\n");

  if(this->left_in_buffer)
  {
    push_string(this->left_in_buffer);
    sp++;
    sp[-1] = sp[-2];
    sp[-2] = sp[-3];
    sp[-3] = sp[-1];
    sp--;
    f_add(2);
  }
  this->left_in_buffer = sp[-1].u.string;
  this->left_in_buffer->refs++;
  this->bpos = 0;
  pop_stack();
  
  switch(this->state)
  {
   case INITIAL:
    /* Is this a http 0.9 or 1.0 request? */
    push_text("method");
    if(get_word(' ')) /* Method. */
    {
      push_text("file");
      if(get_word(' '))
      {
	push_text("protocol");
	if(get_word(' '))
	{
	  /* Now we _know_ that it is a HTTP/1.0 request. */
	  f_aggregate_mapping(6);
	  this->so_far = sp[-1].u.mapping;
	  this->so_far->refs++;
	  pop_stack();
	  eol(); /* Skip all space to and including end of line */
	} else if(eol()) {
	  /* Now we _know_ that it is a HTTP/0.9 request. Done. */
	  push_text("HTTP/0.9");
	  free_string(this->left_in_buffer);
	  this->left_in_buffer = 0;
	  f_aggregate_mapping(6); /* RETURN VALUE */
	  return;
	} else {
	  /* This is not very likely to happend, so lets just reparse the
	   * data. Not state saving is done here (only one line, you know..)
	   */
	  pop_stack(); /* "method" */
	  pop_stack(); /* method */
	  pop_stack(); /* file */
	  push_int(0); /* RETURN VALUE */
	  return;
	}
      } else {
	 /* This is not very likely to happend, so lets just reparse the
	  * data. Not state saving is done here (only one line, you know..)
	  */
	pop_stack(); /* "method" */
	pop_stack(); /* method */
	pop_stack(); /* file */
	push_int(0); /* RETURN VALUE */
	return;
      }
    } else {
      /* This is not very likely to happend, so lets just reparse the
       * data. Not state saving is done here (only one line, you know..)
       */
      pop_stack(); /* "method" */
      if(eol())
	push_int(-1); /* Error in request. */
      else
	push_int(0); /* RETURN VALUE */
      return;
    }
    this->state = HEADERS;
    
   case HEADERS:
    while(get_word(':')) /* From beginning of line to the ':' character */
    {
      f_lower_case(1);
      if(to_eol()) /* The rest of the line, skipping leading spaces */
      {
	p++;
	if(eol()) /* On another eol. End of heders.. */
	{
	  this->state = DATA;
	  break;
	}
      } else {
	pop_stack();
	break;
      }
    }
    if(p)
    {
      f_aggregate_mapping(p*2);
      push_text("headers");
      if(data=low_mapping_lookup(this->so_far, sp-1))
      {
	pop_stack();
	push_svalue(data);
	f_add(2);
	push_text("headers");
      }
      /* MAPPING "headers" */
      mapping_insert(this->so_far, sp-1, sp-2);
      pop_stack();
      sp--;
    }
    push_string(make_shared_binary_string(this->left_in_buffer->str+this->bpos,
					  this->left_in_buffer->len-this->bpos));
    free_string(this->left_in_buffer);
    this->left_in_buffer = sp[-1].u.string;
    this->left_in_buffer->refs++;
    /* Now all (parsed) headers are gone from the left_in_buffer variable */
    this->bpos = 0;
    pop_stack();

    if(this->state != DATA)
    {
      push_int(0);
      return;
    }
    
    push_text("headers");
    if(data=low_mapping_lookup(this->so_far, sp-1))
    {
      struct mapping *m;
      pop_stack();
      push_text("content-length");
      m = data->u.mapping;
      if(data=low_mapping_lookup(m, sp-1))
      {
	this->data_left = atoi((char *)data->u.string->str);
	fprintf(stderr, "Found len: %d bytes.\n", this->data_left);
      }
    }
    pop_stack();
   case DATA:
    if(this->left_in_buffer->len < this->data_left)
    {
      push_int(0);
      return;
    }
    push_string(make_shared_binary_string(this->left_in_buffer->str,
					  this->data_left));
    push_text("data");
    mapping_insert(this->so_far, sp-1, sp-2);
    pop_n_elems(2);
    push_mapping(this->so_far); /* RETURN VALUE */
    this->so_far = 0;
    free_string(this->left_in_buffer);
    this->left_in_buffer = 0;
    this->state = INITIAL;
    return;
  }
}

static void init_feeder(struct object *o)
{
  struct parsebuffer *p;
  if(!o->prog) error("Destructed object?\n");

  MEMSET(this, 0, sizeof(struct parsebuffer));
#if 0
  this->so_far = 0;
  this->left_in_buffer = 0;
#endif
}

void f_check(INT32 args)
{
  pop_n_elems(args);
  if(this->so_far)
  {
    push_mapping(this->so_far);
    this->so_far->refs++;
  } else
    push_int(0);
}

void init_parse_program()
{
   start_new_program();
   add_storage(sizeof(struct parsebuffer));
   add_function("feed", f_feed, "function(string:int|mapping)", 0);
   add_function("check", f_check, "function(:mapping)", 0);
   set_init_callback(init_feeder);
   feed_program = end_c_program("/precompiled/http_parse");
   feed_program->refs++;
}
