string copy_to = "manual/images";
#define DEBUG

void parse_file(string file) {

  string dir = getcwd()+"/"+dirname(file)+"/";

  string x,y;
  y = Stdio.read_file(file);
#ifdef DEBUG
  array err = catch {
    x = Tools.AutoDoc.ProcessXML.moveImages
      (y, dir, copy_to);
  };
  if(!err) {
    if(x!=y)
      Stdio.write_file(file, x);
  }
  else {
    if(objectp(err) && _typeof(err) <= Tools.AutoDoc.AutoDocError)
      werror("%O\n", err);
    else if(arrayp(err))
      werror(describe_backtrace(err));
    else
      werror("%O\n", err);
  }
#else
  x = Tools.AutoDoc.ProcessXML.moveImages
    (Stdio.read_file(file), dir, copy_to);
  if(x!=y)
    Stdio.write_file(directory+file, x);
#endif
}


int main(int num, array(string) args) {

  if(num<3) throw( "Not enough arguments to autodoc_images.pike\n" );
  copy_to = args[1];
  foreach(args[2..], string file) {
    if(has_suffix(file,"sub_manual.xml")) continue;
    Stdio.Stat st = file_stat(file);
    if(!st->size) continue;
    parse_file( file );
  }
}
