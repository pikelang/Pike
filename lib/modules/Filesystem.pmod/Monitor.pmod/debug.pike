inherit "basic.pike";

void data_changed(string path) { werror("data_changed(%O)\r\n", path); }
void attr_changed(string path, Stdio.Stat st) {
  werror("attr_changed(%O, %O)\r\n", path, st);
}
void file_created(string path, Stdio.Stat st) {
  werror("file_created(%O, %O)\r\n", path, st);
}
void file_deleted(string path) { werror("file_deleted(%O)\r\n", path); }
void stable_data_change(string path) { werror("stable(%O)\r\n", path); }

