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

void streamed_parser_init();
void streamed_parser_destruct();
void streamed_parser_set_data( INT32 args );
void streamed_parser_parse( INT32 args );
void streamed_parser_finish( INT32 args );

#endif
