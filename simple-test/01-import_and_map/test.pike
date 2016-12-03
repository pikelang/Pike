inherit "../test.pike";

constant BASE = __DIR__;

int run()
{
  werror("Run in test: %s\n", basename(BASE));

  compiler->import_http = Tools.Sass.HTTP_IMPORT_GREEDY;

  compiler->set_options(([
    "source_map_file" : combine_path(BASE, "output.source.map"),
    "omit_source_map" : false,
    "include_path"    : combine_path(BASE, "inc")
  ]));

  mixed err = catch {
    compiler->compile_file(combine_path(BASE, "input.scss"),
                           combine_path(BASE, "output.css"));
  };

  handle_err(err);
}
