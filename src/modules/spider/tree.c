#include "machine.h"
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

/*
 * Root node header
 * Root node data ...
 * { Header for sub node
 *   data for node
 *  { Header for subsubnode
 *    data for subnode
 *     ...
 *  } ...
 * } ...
 *
 */

#ifndef HAVE_STRDUP
char *strdup(const char *w)
{
  char *s;
  s=malloc(strlen(w) + 1);
  strcpy(s,w);
  return s;
}
#endif

#undef TDEBUG

char *parse_tree(char *data)
{
  char *start;
  int nodes=0;
  int flagga=0;

  start = data;

  while(*data && *data !='\n' && *data != '}' && *data!= '{') data++;

  if(!*data) /* End of the tree.. */
  {
    push_string(make_shared_binary_string(start, data-start));
    push_string(make_shared_binary_string(start, 0));
    f_aggregate(0);
    f_aggregate(3);
    return 0;
  }

  push_string(make_shared_binary_string(start, data-start));
  start=data;
  
  while(*data)
  {
    switch(*data)
    {
     case '{':
      if(!flagga)
      {
	/* First, lets push the "data" of this node onto the stack... */
	push_string(make_shared_binary_string(start, data-start));
	flagga=1;
      }
      nodes++;
      data = parse_tree(data+1);
      if(data) continue;

     case '}':
      /* Data for this node is ended here.
       * On the stack, we now have "header" "data" node node ..
       *  We turn this into { "header" "data" { nodes } }.
       */
      if(!flagga) push_string(make_shared_binary_string(start, data-start));
      f_aggregate(nodes); /* Turn the nodes into an array... */
      f_aggregate(3);
      
      if(data && *(data+1))
	return data+1;
      else
	return 0;
    }
    data++;
  }
  if(!flagga)
  {
#ifdef TDEBUG
    puts("No data found in this node.");
#endif
    push_string(make_shared_binary_string(start, data-start-1)); 
    f_aggregate(0);
  } else
    f_aggregate(nodes);
  f_aggregate(3);
#ifdef TDEBUG
  puts("End of node.\n");
#endif
  return 0;
}


void f_parse_tree(INT32 argc)
{
  int nodes;
  char *data;
  if(argc != 1) 
    error("Wrong number of arguments to parse_tree(string data)\n");
  if(sp[-1].type != T_STRING)
    error("Wrong type of argument to parse_tree(string data)\n");
  data=(char *)strdup((char *)sp[-1].u.string->str);
  nodes=1;
  pop_stack(); /* The argument.. */
  parse_tree(data);
  free(data);
}

