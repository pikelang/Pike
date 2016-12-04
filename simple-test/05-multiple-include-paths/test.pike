inherit "../test.pike";

constant BASE = __DIR__;

int run()
{
  werror("Run in test: %s\n", basename(BASE));

  compiler->set_include_paths(({
    combine_path(BASE, "i1"),
    combine_path(BASE, "i2")
  }));

  mixed err = catch {
    compiler->compile_file(combine_path(BASE, "input.scss"),
                           combine_path(BASE, "output.css"));
  };

  handle_err(err);
}
