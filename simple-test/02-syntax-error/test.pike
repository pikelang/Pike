inherit "../test.pike";

constant BASE = __DIR__;

int run()
{
  werror("Run in test: %s\n", basename(BASE));

  mixed err = catch {
    compiler->compile_file(combine_path(BASE, "input.scss"),
                           combine_path(BASE, "output.css"));
  };

  if (err) {
    werror(">> Compile file error:\n%s\n", describe_error(err));
  }

  string cont = Stdio.read_file(combine_path(BASE, "input.scss"));

  err = catch {
    compiler->compile_string(cont);
  };

  if (err) {
    werror(">> Compile string error:\n%s\n", describe_error(err));
  }
}
