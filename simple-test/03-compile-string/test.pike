inherit "../test.pike";

constant BASE = __DIR__;

int run()
{
  werror("Run in test: %s\n", basename(BASE));

  compiler->set_include_path (BASE);
  compiler->http_import = Tools.Sass.HTTP_IMPORT_ANY;

  mixed err = catch {
    string filec = Stdio.read_file(combine_path(BASE, "input.scss"));
    string data = compiler->compile_string(filec);
    Stdio.write_file(combine_path(BASE, "output.css"), data);
  };

  handle_err(err);
}
