#pike __REAL_VERSION__
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal u. Parser.HTML";
int n=300;

Parser.HTML p=Parser.HTML()->_set_tag_callback("");
// lambda(Parser.HTML p,string s) { return ""; }

string tagremove(string line)
{
   return p->finish(line)->read();
}
