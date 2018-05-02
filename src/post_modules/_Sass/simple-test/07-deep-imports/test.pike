inherit "../test.pike";

constant BASE = __DIR__;

typedef string(8bit) s8;

class MyCompiler
{
  inherit Web.Sass.Compiler;
  private s8 root_file;
  private s8 root_dir;

  protected void create(s8 root)
  {
    root_file = root;
    root_dir  = dirname(root_file);
  }

  private s8 prev_dir = "";

  protected array(s8) handle_sass_import(s8 path, s8 abs_path, s8 rel_path)
  {
    if (abs_path == "stdin") {
      abs_path = this::root_file;
    }

    s8 owner = combine_path_unix(root_dir, abs_path);
    s8 fn = combine_path_unix(dirname(owner), path);

    if (Stdio.exist(fn)) {
      return ({ Stdio.read_file(fn), fn });
    }

    s8 cur_dir  = dirname(fn);
    s8 cur_file = basename(fn);

    foreach (({ ".scss", ".sass" }), string ext) {
      fn = combine_path_unix(cur_dir, "_" + cur_file + ext);

      if (Stdio.exist(fn)) {
        return ({ Stdio.read_file(fn), fn });
      }

      fn = combine_path_unix(cur_dir, cur_file + ext);

      if (Stdio.exist(fn)) {
        return ({ Stdio.read_file(fn), fn });
      }
    }
  }
}

int run()
{
  werror("Run in test: %s\n", basename(BASE));

  compiler = MyCompiler(combine_path(__DIR__, "input.scss"));
  compiler->output_style = Web.Sass.STYLE_EXPANDED;
  // compiler->source_map_file = "output.css.map";

  mixed err = catch {
    mapping res =
      compiler->compile_string(Stdio.read_file(combine_path(BASE, "input.scss")));

    Stdio.write_file("output.css", res->css);
  };

  handle_err(err);

  return -1;
}
