#require constant(GI.repository.Gtk.Application) && constant(Cairo.Context)

import GI.repository;

inherit Gtk.Application;

protected void create()
{
  ::create((["application_id":"apps.pike.pv",
             "flags":Gio.ApplicationFlags.HANDLES_OPEN]));
  ::connect("open", lambda(Gtk.Application app, array(Gio.File) files,
                           int nfiles, string hint) {
    foreach(files;; Gio.File file)
      ::add_window(Tools.PV(file->is_native()? file->get_path() :
                            Standards.URI(file->get_uri())));
  });
  ::connect("activate", lambda(Gtk.Application app) {
    if (sizeof(::get_windows()))
      return;
    write("Nothing to view\n");
    ::quit();
  });
}
