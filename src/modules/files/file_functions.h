  FILE_FUNC("open",file_open,"function(string,string,void|int:int)")
  FILE_FUNC("close",file_close,"function(string|void:int)")
  FILE_FUNC("read",file_read,"function(int|void,int|void:int|string)")
#ifndef __NT__
  FILE_FUNC("peek",file_peek,"function(void:int)")
#endif
  FILE_FUNC("write",file_write,"function(string,void|mixed...:int)")
#ifdef WITH_OOB
  FILE_FUNC("read_oob",file_read_oob,"function(int|void,int|void:int|string)")
  FILE_FUNC("write_oob",file_write_oob,"function(string,void|mixed...:int)")
#endif /* WITH_OOB */

  FILE_FUNC("seek",file_seek,"function(int,int|void,int|void:int)")
  FILE_FUNC("tell",file_tell,"function(:int)")
  FILE_FUNC("stat",file_stat,"function(:int *)")
  FILE_FUNC("errno",file_errno,"function(:int)")

  FILE_FUNC("set_close_on_exec",file_set_close_on_exec,"function(int:void)")
  FILE_FUNC("set_nonblocking",file_set_nonblocking,"function(:void)")

  FILE_FUNC("set_read_callback",file_set_read_callback,"function(mixed:void)")

  FILE_FUNC("set_write_callback",file_set_write_callback,"function(mixed:void)")

#ifdef WITH_OOB
  FILE_FUNC("set_read_oob_callback",file_set_read_oob_callback,"function(mixed:void)")

  FILE_FUNC("set_write_oob_callback",file_set_write_oob_callback,"function(mixed:void)")

#endif /* WITH_OOB */
  FILE_FUNC("set_blocking",file_set_blocking,"function(:void)")

  FILE_FUNC("query_fd",file_query_fd,"function(:int)")

  FILE_FUNC("dup2",file_dup2,"function(object:int)")
  FILE_FUNC("dup",file_dup,"function(void:object)")
  FILE_FUNC("pipe",file_pipe,"function(void|int:object)")

  FILE_FUNC("set_buffer",file_set_buffer,"function(int,string|void:void)")
  FILE_FUNC("open_socket",file_open_socket,"function(int|void,string|void:int)")
  FILE_FUNC("connect",file_connect,"function(string,int:int)")
  FILE_FUNC("query_address",file_query_address,"function(int|void:string)")
  FILE_FUNC("create",file_create,"function(void|string,void|string:void)")
  FILE_FUNC("`<<",file_lsh,"function(mixed:object)")

#ifdef _REENTRANT
  FILE_FUNC("proxy",file_proxy,"function(object:void)")
#endif

#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF) 
  FILE_FUNC("lock",file_lock,"function(void|int:object)")
  FILE_FUNC("trylock",file_trylock,"function(void|int:object)")
#endif

#undef FILE_FUNC
