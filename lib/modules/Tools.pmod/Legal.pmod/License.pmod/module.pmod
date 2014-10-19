#pike __REAL_VERSION__

//! Returns all the licenses as a string, suitable for
//! saving as a file.
string get_text() {
  array licenses = sort(indices(Tools.Legal.License)) -
  ({ "get_text", "module" });
  array list = map(licenses,
		   lambda(string lic) {
		     return lic + " (" + Tools.Legal.License[lic]->get_name() + ")";
		   });
  string ret = sprintf("%-=80s\n",
		       "The Pike source is distributed under " +
		       String.implode_nicely(list) + ". "
		       "These licenses are listed in order below.");
  ret = String.trim_all_whites((ret/"\n")[*])*"\n";
  foreach(licenses, string license)
    ret += "\n\n\f\n" + Tools.Legal.License[license]->get_text();

  return ret;
}
