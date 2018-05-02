inherit "../test.pike";

constant BASE = __DIR__;

int run()
{
  werror("Run in test: %s\n", basename(BASE));

  compiler->include_path = BASE;
  compiler->http_import = Web.Sass.HTTP_IMPORT_ANY;

  mixed err = catch {
    string filec = Stdio.read_file(combine_path(BASE, "input.scss"));
    mapping data = compiler->compile_string(filec);
    Stdio.write_file(combine_path(BASE, "output.css"), data->css);
  };

  handle_err(err);
}
