
void main(int num, array(string) args) {

  Stdio.File out = Stdio.File(args[2],"cwt");
  foreach(Stdio.read_file(args[1])/"\n";; string row) {
    if(sizeof(row) && row[0]!='#' && row[0]!='@') {
      sscanf(row, "%s#", row);
      row = String.trim_all_whites(row);
    }
    out->write(row+"\n");
  }

}
