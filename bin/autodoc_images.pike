string copy_to = "manual/images";

void parse_directory(string directory) {
  werror("autodoc_images: Entering %s...\n", directory);
  array(string) files = get_dir(directory);
  foreach(files, string file) {
    //    werror("%O\n", directory+file);
    if(file_stat(directory+file)->isdir) {
      parse_directory(directory+file+"/");
      continue;
    }
    if(!has_suffix(file, ".xml"))
      continue;
    if(file=="sub_manual.xml")
      continue;
    string x,y;
    y = Stdio.read_file(directory+file);
#ifdef DEBUG
    array err = catch {
      x = Tools.AutoDoc.ProcessXML.moveImages
	(y, directory, copy_to);
    };
    if(!err) {
      if(x!=y)
	Stdio.write_file(directory+file, x);
    }
    else
      werror(describe_backtrace(err));
#else
    x = Tools.AutoDoc.ProcessXML.moveImages
      (Stdio.read_file(directory+file), directory, copy_to);
    if(x!=y)
      Stdio.write_file(directory+file, x);
#endif
  }
}


int main(int num, array(string) args) {

  if(num<3) throw( "Not enough arguments to autodoc_images.pike\n" );
  copy_to = args[2];
  parse_directory( args[1] );

}
