#pike __REAL_VERSION__

inherit "symlinks.pike";

void data_changed(string path) { werror("data_changed(%O)\r\n", path); }
void attr_changed(string path, Stdio.Stat st) {
  werror("attr_changed(%O, %O)\r\n", path, st);
}
void file_exists(string path, Stdio.Stat st) {
  werror("file_exits(%O, %O)\r\n", path, st);
}
void file_created(string path, Stdio.Stat st) {
  werror("file_created(%O, %O)\r\n", path, st);
}
void file_deleted(string path) { werror("file_deleted(%O)\r\n", path); }
void stable_data_change(string path, Stdio.Stat st) {
  werror("stable_data_change(%O, %O)\r\n", path, st);
}


int check(int|void max_wait, int|void max_cnt,
	  mapping(string:int)|void ret_stats)
{
  int ret;
  werror("check(%O, %O, %O) ==> %O\r\n",
	 max_wait, max_cnt, ret_stats,
	 ret = ::check(max_wait, max_cnt, ret_stats));
  return ret;
}
