
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
    string x = Tools.AutoDoc.ProcessXML.moveImages
      (Stdio.read_file(directory+file), directory, "manual/images");
    Stdio.write_file(directory+file, x);
  }
}


int main(int num, array(string) args) {

  if(num<2) throw( "Not enough arguments to autodoc_images.pike\n" );
  parse_directory( args[1] );

}
