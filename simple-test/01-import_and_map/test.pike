inherit "../test.pike";

constant BASE = __DIR__;

int run()
{
  werror("Run in test: %s\n", basename(BASE));

  compiler->set_options(([
    "source_map_file" : combine_path(BASE, "output.source.map"),
    "omit_source_map" : false
  ]));

  werror("Include path: %O\n", compiler->get_include_path());

  mixed err = catch {
    compiler->compile_file(combine_path(BASE, "input.scss"),
                           combine_path(BASE, "output.css"));
  };
}
