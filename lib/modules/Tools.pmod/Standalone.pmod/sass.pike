#! /usr/bin/env pike
// -*- pike -*-
#pike __REAL_VERSION__
#require constant(Web.Sass)

inherit Tools.Standalone.process_files;
string version =
 sprintf("%d.%d.%d",(int)__REAL_VERSION__,__REAL_MINOR__,__REAL_BUILD__);
string description = "Compile SASS SCSS code.";
string usage = #"[options] [<infile.scss>] ...

sass (\"compile SASS SCSS file\") Compiles a SASS SCSS code down to plain CSS.
Available options:
" + default_flag_docs;

int want_args = 0;

string process(string input, string ... args) {
  Web.Sass.Api sc = Web.Sass.Api();
  sc->output_style = Web.Sass.STYLE_COMPRESSED;
  sc->sass_syntax = false;
  sc->source_map_embed = true;
  mapping output = sc->compile_string(input);
  if (!output) {
    werror("Problem converting\n");
    return 0;
  }
  return output->css;
}

Regexp getextension = Regexp("[^/]\\.([^.]+)$");

string getbase(string path) {
  array(string) ext = getextension->split(path);
  return ext ? path[.. < sizeof(ext[0]) + 1] : path;
}

int(0..1) process_file(string path, string ... args) {
  string base = getbase(path) + ".css";
  if (base == path) {
    werror("Input must differ from output, aborted...\n");
    return 1;
  }
  Web.Sass.Compiler sc = Web.Sass.Compiler();
  sc->set_options(
   ([
      "output_style":Web.Sass.STYLE_COMPRESSED,
      "source_map_file":basename(base) + ".map",
   ])
  );
  mapping output = sc->compile_file(path);
  if (!output) {
    werror("Problem converting %O\n", output);
    return 1;
  }
  Stdio.File(base, "wc")->write(output->css);
  Stdio.File(base + ".map", "wc")->write(output->map);
  return 0;
}
