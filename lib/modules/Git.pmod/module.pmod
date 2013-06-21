#pike __REAL_VERSION__

string git_binary = "git";

//! A normal (non-executable) file.
constant MODE_FILE = 0100644;

//! A normal, but executable file.
constant MODE_EXE = 0100755;

//! A symbolic link.
constant MODE_SYMLINK = 0120000;

//! A gitlink (aka submodule reference).
constant MODE_GITLINK = 0160000;

//! A subdirectory.
constant MODE_DIR = 040000;

//! The NULL SHA1.
constant NULL_SHA1 = "0000000000000000000000000000000000000000";

//! Start a git process.
Process.Process low_git(mapping(string:mixed) options,
			string git_dir, string command, string ... args)
{
  return Process.Process(({ git_binary, "--git-dir", git_dir,
			    command, @args }), options);
}

//! Run a git command, and get the output.
string git(string git_dir, string command, string ... args)
{
  array(string) cmd = ({ git_binary, "--git-dir", git_dir, command, @args });
  mapping(string:string|int) res = Process.run(cmd);
  if (res->exitcode) {
    werror("CWD: %O\n", getcwd());
    error("Git command '%s' failed with code %d:\n"
	  "%s", cmd*"' '", res->exitcode, res->stderr||"");
  }
  return res->stdout||"";
}

//! Hash algorithm for blobs that is compatible with git.
string hash_blob(string data)
{
  Crypto.SHA1 sha1 = Crypto.SHA1();
  sha1->update(sprintf("blob %d\0", sizeof(data)));
  sha1->update(data);
  return String.string2hex(sha1->digest());
}

