int main(int argc, array(string) argv)
{
  object sa = COM.CreateObject("Shell.Application");

  object fold = sa->BrowseForFolder(0, "Select folder for Pike shell test", 0);

  if (!fold)
  {
    werror("BrowseForFolder returned null\n");
    return 1;
  }

  werror(sprintf("fold(%%O)                 : %O\n", fold));
  werror(sprintf("fold((string)Title)      : %s\n", (string)fold->Title));
  werror(sprintf("fold(Title)              : %s\n", fold->Title));
  werror(sprintf("fold(title->_value)      : %s\n", fold->title->_value));
  werror(sprintf("fold(title[\"_value\"]())  : %s\n", fold->title["_value"]));
  //  werror(sprintf("fold: %s\n", fold["title"]->value()));
  
  object fitems = fold->Items();

  int count = (int)fitems->Count;
  werror(sprintf("fitems->count: %d\n", count));
  werror(sprintf("fitems->count: %d\n", fitems->Count));
  werror("3*fitems->count: %d\n", 3*fitems->Count);

  for (int x=0; x<count; x++)
  {
    object fitem = fitems->Item(x);
  
    //werror(sprintf("fitem: %s\n", (string)fitem->Path));

    werror(sprintf("%-16s %8s %-24s %17s %4s\n",
                   fold->GetDetailsOf(fitem, 0),
                   fold->GetDetailsOf(fitem, 1),
                   fold->GetDetailsOf(fitem, 2),
                   fold->GetDetailsOf(fitem, 3),
                   fold->GetDetailsOf(fitem, 4),
                   ));
  }
}
