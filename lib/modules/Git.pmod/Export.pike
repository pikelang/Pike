#pike __REAL_VERSION__

//! Framework for creating a command-stream suitable for git-fast-import.

protected Stdio.File export_fd;

protected Process.Process fast_importer;

protected int verbosity;

protected mapping(string:string|int) requested_features = ([]);

//! @decl void create()
//! @decl void create(Stdio.File fd)
//! @decl void create(string git_dir)
//!
//!   Create a new fast-import command-stream.
//!
//! @param fd
//!   File to write the command-stream to.
//!
//! @param git_dir
//!   Git directory to modify. If the directory doesn't exist,
//!   it will be created empty. A git-fast-import session will
//!   be started for the directory to receive the command-stream.
//!
//!   If neither @[fd] nor @[git_dir] has been specified, the
//!   command stream will be output to @[Stdio.stdout].
//!
//! @param verbosity
//!   The amount of verbosity on @[Stdio.stderr] for various commands.
protected void create(Stdio.File|string|void fd_or_gitdir,
		      int|void verbosity)
{
  global::verbosity = verbosity;
  if (!stringp(fd_or_gitdir)) {
    export_fd = fd_or_gitdir || Stdio.stdout;
  } else {
    if (!Stdio.is_dir(fd_or_gitdir)) {
      if (verbosity) werror("Creating %s\n", fd_or_gitdir);
      Git.git(fd_or_gitdir, "init", "--bare");
    }
    if (verbosity)
      werror("Starting a fast importer for git-dir %O...\n", fd_or_gitdir);
    export_fd = Stdio.File();
    fast_importer = Git.low_git(([ "stdin":export_fd->pipe() ]),
				fd_or_gitdir, "fast-import");
  }
}

//! Send a raw command.
void command(sprintf_format cmd, sprintf_args ... args)
{
  export_fd->write(cmd, @args);
}

//! End the command-stream and wait for completion.
int done()
{
  if (requested_features["done"]) {
    command("done\n");
  }
  if (export_fd) {
    export_fd->close();
    export_fd = UNDEFINED;
  }
  int ret;
  if (fast_importer) {
    ret = fast_importer->wait();
    fast_importer = UNDEFINED;
  }
  return ret;
}

//! Move a reference.
//!
//! @param ref
//!   Reference to move.
//!
//! @param committish
//!   Commit to reference.
//!
//! This command can also be used to make light-weight tags.
//!
//! @seealso
//!   @[tag]
void reset(string ref, string|void committish)
{
  command("reset %s\n", ref);
  if (committish) {
    command("from %s\n", committish);
  }
}

void mark(string marker)
{
  command("mark %s\n", marker);
}

void data(string data)
{
  command("data %d\n"
	  "%s\n", sizeof(data), data);
}

//! Upload data.
//!
//! @param blob
//!   Data to upload.
//!
//! @param marker
//!   Optional export marker for referring to the data.
void blob(string blob, string|void marker)
{
  command("blob\n");
  if (marker) mark(marker);
  data(blob);
}

//! Flush state to disk.
void checkpoint()
{
  command("checkpoint\n");
}

//! Output a progress message.
//!
//! @param message
//!   Message to output.
//!
//! @note
//!   Note that each line of the message will be prefixed
//!   with @expr{"progress "@}.
void progress(string message)
{
  foreach(message/"\n", string line) {
    command("progress %s\n", line);
  }
}

//! Require that the backend for the stream supports a certian feature.
//!
//! @param feature
//!   Feature to require support for. Typically one of:
//!   @string
//!     @value "date-format"
//!     @value "export-marks"
//!     @value "relative-marks"
//!     @value "no-relative-marks"
//!     @value "force"
//!     @value "import-marks"
//!     @value "import-marks-if-exists"
//!       Same as the corresponding command-line option.
//!     @value "cat-blob"
//!     @value "ls"
//!       Require the @[cat_blob] and @[ls] commands to be supported.
//!     @value "notes"
//!       Require that the backend supports the @[notemodify] subcommand.
//!     @value "done"
//!       Require the stream to terminate with a @[done] command.
//!   @endstring
void feature(string feature, string|void arg)
{
  if (arg) {
    command("feature %s=%s\n", feature, arg);
  } else {
    command("feature %s\n", feature);
  }
  requested_features[feature] = arg || 1;
}

//! Set backend options.
void option(string option)
{
  command("option %s\n", option);
}

//! Create an annotated tag referring to a specific commit.
//!
//! @param name
//!   Tag name. Note that it is automatically
//!   prefixed with @expr{"refs/tags/"@}.
//!
//! @param committish
//!   Commit to tag.
//!
//! @param tagger_info
//!   Name, email and timestamp for the tagger.
//!   See @[format_author()] for details.
//!
//! @param message
//!   Message for the tag.
//!
//! @seealso
//!   @[reset]
void tag(string name, string committish, string tagger_info, string message)
{
  command("tag %s\n"
	  "from %s\n"
	  "tagger %s\n",
	  name, committish, tagger_info);
  data(message);
}

//! Create a new commit on a branch.
//!
//! @param ref
//!   Reference to add the commit to.
//!   Typically @expr{"refs/heads/"@} followed by a branchname,
//!   or @expr{"refs/notes/commits"@}.
//!
//! @param commit_marker
//!   Optional export marker for referring to the new commit.
//!
//! @param author_info
//!   Optional author information. Defaults to @[committer_info].
//!
//! @param committer_info
//!   Name, email and timestamp for the committer.
//!   See @[format_author()] for details.
//!
//! @param message
//!   Commit message.
//!
//! @param parents
//!   The ordered set of parents for the commit.
//!   Defaults to the current HEAD for @[ref], if it exists,
//!   and to the empty set otherwise.
//!
//! The set of files for the commit defaults to the set
//! for the first of the @[parents], and can be modified
//! with @[filemodify], @[filedelete], @[filecopy], @[filerename],
//! @[filedeleteall] and @[notemodify].
void commit(string ref, string|void commit_marker,
	    string|void author_info, string committer_info,
	    string message, string|void ... parents)
{
  command("commit %s\n", ref);
  if (commit_marker) {
    mark(commit_marker);
  }
  if (author_info) {
    command("author %s\n", author_info);
  }
  command("committer %s\n", committer_info);
  data(message);
  if (sizeof(parents)) {
    command("from %s\n"
	    "%{merge %s\n%}",
	    parents[0], parents[1..]);
  }
}

//! Delete all files.
//!
//! Used to start a commit from a clean slate.
void filedeleteall()
{
  command("deleteall\n");
}

//! Create or modify a file.
//!
//! @param mode
//!   Mode for the file. See the @tt{MODE_*@} constants.
//!
//! @param path
//!   Path to the file relative to the repository root.
//!
//! @param dataref
//!   Reference to the data for the file. One of:
//!   @mixed
//!     @type string
//!       A mark reference set by a prior @[blob] command or
//!       a full 40-byte SHA-1 of an existing Git blob.
//!     @type zero
//!       Left out, @[UNDEFINED] or @expr{"inline"@} in which
//!       case the @[filemodify] command must be followed by
//!       a @[data] command.
//!   @endmixed
void filemodify(int mode, string path, string|void dataref)
{
  path = combine_path_unix("/", path)[1..];
  if (path == "") {
    error("Invalid path.\n");
  }
  command("M %06o %s %q\n",
	  mode, dataref || "inline", path);
}

//! Delete a file.
void filedelete(string path)
{
  path = combine_path_unix("/", path)[1..];
  if (path == "") {
    error("Invalid path.\n");
  }
  command("D %q\n", path);
}

//! Copy a file or directory.
void filecopy(string from, string to)
{
  from = combine_path_unix("/", from)[1..];
  to = combine_path_unix("/", to)[1..];
  if ((from == "") || (to == "")) {
    error("Invalid path.\n");
  }
  command("C %q %q\n", from, to);
}

//! Rename a file or directory.
void filerename(string from, string to)
{
  from = combine_path_unix("/", from)[1..];
  to = combine_path_unix("/", to)[1..];
  if ((from == "") || (to == "")) {
    error("Invalid path.\n");
  }
  command("C %q %q\n", from, to);
}

//! Annotate a commit.
//!
//! @param commit
//!   Commit to annotate.
//!
//! @param dataref
//!   Reference to the data for the annotation. One of:
//!   @mixed
//!     @type string
//!       A mark reference set by a prior @[blob] command or
//!       a full 40-byte SHA-1 of an existing Git blob.
//!     @type zero
//!       Left out, @[UNDEFINED] or @expr{"inline"@} in which
//!       case the @[notemodify] command must be followed by
//!       a @[data] command.
//!   @endmixed
//!
//! Note that this command is typically only used when
//! a commit on a ref under @expr{"refs/notes/"@} is active.
void notemodify(string commit, string|void dataref)
{
  if (!requested_features["notes"]) {
    error("The notes feature has not been requested.\n");
  }
  command("N %s %s\n", dataref || "inline", commit);
}

//! Output a blob on the @expr{cat-blob-fd@}.
//!
//! @param dataref
//!   Reference to the blob to output.
void cat_blob(string dataref)
{
  command("cat-blob %s\n", dataref);
}

//! Output a file to the @expr{cat-blob-fd@}.
//!
//! @param path
//!   Path to the file to output.
//!
//! @param dataref
//!   Marker, tag, commit or tree for the root.
//!   Defaults to the commit in progress.
void ls(string path, string|void dataref)
{
  if (dataref) {
    command("ls %s %s\n", dataref, path);
  } else {
    command("ls %q\n", path);
  }
}

//! Convenience funtion for exporting a filesystem file
//! or directory (recursively) to git.
//!
//! @param file_name
//!   Name of the file on disc.
//!
//! @param git_name
//!   Name of the file in git. Defaults to @[file_name].
void export(string file_name, string|void git_name)
{
  Stdio.Stat st = file_stat(file_name);
  if (!st) return;
  int mode = st->mode;
  if (mode & Git.MODE_DIR) {
    mode = Git.MODE_DIR;
  } else if (mode & 0111) {
    mode = Git.MODE_EXE;
  } else if (mode & 0666) {
    mode = Git.MODE_FILE;
  } else {
    error("Unsupported filesystem mode for %O: %03o\n", file_name, mode);
  }
  if (mode == Git.MODE_DIR) {
    foreach(get_dir(file_name), string fn) {
      export(combine_path(file_name, fn),
	     combine_path(git_name || file_name, fn));
    }
  } else {
    filemodify(mode, git_name);
    data(Stdio.read_bytes(file_name));
  }
}
