/* $Id: streamed_parser.h,v 1.3 1998/03/28 13:49:44 grubba Exp $ */
#ifndef STREAMED_PARSER_H
#define STREAMED_PARSER_H

struct streamed_parser
{
  unsigned char *last_buffer;
  int last_buffer_size;
  struct mapping *start_tags; /* ([ tag : function_ptr ]) */
  struct mapping *content_tags; /* ([ tag : function_ptr ]) */
  struct mapping *end_tags; /* ([ tag : function_ptr ]) */
  struct svalue *digest;
};

void streamed_parser_init(void);
void streamed_parser_destruct(void);
void streamed_parser_set_data( INT32 args );
void streamed_parser_parse( INT32 args );
void streamed_parser_finish( INT32 args );

#endif
