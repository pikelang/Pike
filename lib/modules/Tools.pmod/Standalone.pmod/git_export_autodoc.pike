
#pike __REAL_VERSION__

//!
//! Tool for converting the Pike git repository to
//! a corresponding git repository containing the
//! extracted autodoc.xml and documentation.
//!
//! It supports incremental operation, so that it
//! can be used to keep track with the source.
//!
//! Typical use:
//! @tt{pike -x git_export_autodoc -v --git-dir=Autodoc.git@}
//!

/*
 * TODO:
 *
 *   [X] Extract autodoc.xml from the Pike repository.
 *       [/] uLPC
 *       [X] Pike 0.5
 *       [X] Pike 0.6
 *       [X] Pike 7.0
 *       [X] Pike 7.2
 *       [/] Pike 7.4
 *       [ ] Pike 7.6
 *       [ ] Pike 7.8
 *       [ ] Pike 7.9
 *
 *   [X] Export directly exportable refs before starting on
 *       each new branch.
 *
 *   [/] Export refs when the corresponding commit is exported.
 *
 *   [X] Convert the autodoc.xml to assembled xml.
 *       [X] onepage.xml
 *       [X] traditional.xml
 *       [X] modref.xml
 *
 *   [X] Convert the assembled xml to html.
 *       [X] onepage.xml
 *       [X] traditional.xml
 *       [X] modref.xml
 *
 *   [ ] Verify that the generated html has proper links
 *       to other pages as well as to images.
 *       [ ] onepage.xml
 *       [ ] traditional.xml
 *       [X] modref.xml
 *
 *   [X] Export the generated images.
 *
 *   [/] Remove/update obsoleted images.
 *       [X] Export only generated images that are referred
 *           to from autodoc.xml.
 *       [/] Images should be regenerated when the lena() image
 *           changes.
 *
 *   [ ] The BMML export is semi-broken for:
 *       [ ] Constants (PI).
 *       [ ] Operators (&&, etc).
 *       [ ] Preprocessor directives. ==> cpp::
 *       [ ] Example code (eg SET_ONERROR() in Pike 0.4pl6).
 *       [ ] C-level documentation (SET_ONERROR()). ==> c::
 *
 *   [ ] The autodoc resolver is broken for old-style module references.
 *       eg file->open ==> /precompiled/file->open.
 *
 *   [X] Fix output when compiling code in MirarDoc compat mode.
 *
 *   [X] Generated xml file from MirarDoc isn't stable.
 *
 *   [X] Generated html output isn't stable.
 *
 *   [X] Reduce output for stage:
 *       [X] Extract wrapper
 *       [X] Extract Autodoc
 *       [X] Extract MirarDoc
 *       [X] Autodoc Joiner
 *       [X] Autodoc Assembler
 *       [X] Autodoc HTML-renderer
 *       [X] Autodoc HTML-splitter
 *
 *   [/] Improve recovery and keep going after errors.
 *       [ ] Extract wrapper
 *       [ ] Extract Autodoc
 *       [ ] Extract MirarDoc
 *       [ ] Autodoc Joiner
 *       [ ] Autodoc Assembler
 *       [X] Autodoc HTML-renderer
 *       [ ] Autodoc HTML-splitter
 *
 *   [X] Support extraction of BMML?
 *
 *   [X] Speed up the joiner by having the extractor generate
 *       separate stamp files and xml-files.
 *
 *   [ ] Improve error diagnostics (eg file and line for parser errors).
 *
 *   [/] Clean the <source-position> tags from autodoc.xml.
 *       [-] BMML
 *       [X] MirarDoc
 *       [/] AutoDoc
 *
 *   [/] Reduce the number of changed files by removing the
 *       Pike version and timestamp from assembled xml.
 *       [X] Use the version and timestamp from the source
 *           Pike and the commit timestamp.
 *       [X] Separate them into files included via
 *           <script type="text/javascript" src="..." />.
 *       [ ] Same for navigation?
 *
 *   [ ] Convert commits in topologic timestamp order
 *       (ie parallel-depth-first order), and update
 *       other branches as appropriate.
 *
 *   [X] Skip extraneous merges (cf 1997-11-06 - 1997-11-09).
 *
 *   [ ] Add a suitable /404.html file.
 */


constant description =
  "Exports a stream of autodoc.xml suitable for git-fast-import.";

constant version_string = "1.0";

int verbose;

string git_binary = "git";
string git_dir;
string work_dir = "doc-work-dir";
string work_git = ".git";

string git(string git_cmd, string ... args)
{
  return Git.git(work_git, git_cmd, @args);
}

void warn(sprintf_format fmt, sprintf_args ... args)
{
  werror("Warning: " + fmt, @args);
}

int prev_progress;
void progress(string message)
{
  if (prev_progress) {
    // Clear the previous message
    werror("\b \b"*prev_progress);
  }
  werror(message);
  prev_progress = sizeof(message);
}

int last_mark;
string new_mark()
{
  return ":" + (++last_mark);
}

//! Mapping from source commit sha1 to doc commit sha1 or export reference.
mapping(string:string) src_to_doc = ([]);

//! Mapping from doc commit sha1 or export reference
//! to the corresponding list of source commit sha1s.
mapping(string:array(string)) doc_to_src = ([]);

//! Mapping from doc commit sha1 or export reference to
//! sha1 for the autodoc.xml blob.
mapping(string:string) autodoc_hash = ([]);

//! Mapping from source commit to refdoc sha1.
mapping(string:string) refdoc_hash = ([]);

//! Mapping from source reference to source commit sha1.
mapping(string:string) src_refs = ([]);

//! Lookup from source commit sha1 to the corresponding references (if any).
mapping(string:multiset(string)) rev_refs = ([]);

//! Mapping from doc reference to doc commit sha1 or export reference.
mapping(string:string) doc_refs = ([]);

//! Mapping from doc commit sha1 or export reference to
//! array of same for its direct parents.
mapping(string:array(string)) doc_to_parents = ([]);

array(string) git_source_revs_heads = ({});

void get_refs(string git_dir, mapping(string:string) refs, int|void is_src)
{
  foreach(Git.git(git_dir, "show-ref")/"\n", string line) {
    array(string) fields = line/" ";
    if (sizeof(fields) < 2) continue;
    string ref_name = fields[1];
    if (is_src) {
      // Look at the origin heads rather than our own.
      if (!has_prefix(ref_name, "refs/remotes/origin/") &&
	  !has_prefix(ref_name, "refs/tags/")) continue;
      if (has_prefix(ref_name, "refs/remotes/origin/")) {
	// Pretend that we have fast-forwarded all branches.
	ref_name = "refs/heads/" + ref_name[sizeof("refs/remotes/origin/")..];
      }
    } else {
      if (!has_prefix(ref_name, "refs/heads/") &&
	  !has_prefix(ref_name, "refs/tags/") &&
	  !has_prefix(ref_name, "refs/notes/source_revs")) continue;
    }
    if (ref_name == "refs/heads/HEAD") continue;
    refs[ref_name] = fields[0];
  }
}

mapping(string:array(string)) get_commit(string git_dir, string sha1)
{
  mapping(string:array(string)) res = ([]);
  array(string) lines = Git.git(git_dir, "cat-file", "commit", sha1)/"\n";
  foreach(lines; int no; string line) {
    if (line == "") {
      res->message = lines[no+1..];
      break;
    }
    array(string) fields = line/" ";
    res[fields[0]] += ({ fields[1..]*" " });
  }
  return res;
}

string get_sha1_for_path(string git_dir, string tree_sha1, string path)
{
  array(string) fields = Git.git(git_dir, "ls-tree", tree_sha1, path)/" ";
  if (sizeof(fields) > 2) return (fields[2]/"\t")[0];
  return "";
}

array(string) get_doc_parents(string doc_sha1)
{
  array(string) parents = doc_to_parents[doc_sha1];
  if (!zero_type(parents)) return parents;
  mapping(string:array(string)) commit = get_commit(git_dir, doc_sha1);
  if (commit->parent) {
    doc_to_parents[doc_sha1] = commit->parent;
  }
  // Get the sha1 for the autodoc.xml blob while we're at it.
  if (commit->tree) {
    string autodoc_sha1 =
      get_sha1_for_path(git_dir, commit->tree[0], "autodoc.xml");
    autodoc_hash[doc_sha1] = autodoc_sha1;
  }
  return parents;
}

string get_autodoc_hash(string doc_sha1)
{
  string autodoc_sha1 = autodoc_hash[doc_sha1];
  if (!zero_type(autodoc_sha1)) return autodoc_sha1;
  mapping(string:array(string)) commit = get_commit(git_dir, doc_sha1);
  if (commit->tree) {
    autodoc_sha1 =
      get_sha1_for_path(git_dir, commit->tree[0], "autodoc.xml");
    autodoc_hash[doc_sha1] = autodoc_sha1;
  }
  // Set the parents for the commit while we're at it.
  if (commit->parent) {
    doc_to_parents[doc_sha1] = commit->parent;
  }
  return autodoc_sha1;
}

string get_refdoc_sha1(string rev)
{
  string res = refdoc_hash[rev];
  if (res) return res;
  return refdoc_hash[rev] = get_sha1_for_path(work_git, rev, "refdoc");
}

Git.Export exporter;

void add_src_to_doc_map(string src_rev, string doc_rev)
{
  src_to_doc[src_rev] = doc_rev;
  doc_to_src[doc_rev] += ({ src_rev });
}

//! Attempt to get the version for the Pike source tree.
string get_version()
{
  string data;
  int major, minor, build;
  if (Stdio.exist("src/version.h")) {
    // Pike 0.7.1 or later.
    data = Stdio.read_bytes("src/version.h");
    if (data && sscanf(data,
		       "%*s#define PIKE_MAJOR_VERSION %d\n"
		       "%*s#define PIKE_MINOR_VERSION %d\n"
		       "%*s#define PIKE_BUILD_VERSION %d\n",
		       major, minor, build) >= 4) {
      return sprintf("Pike v%d.%d.%d", major, minor, build);
    }
  }
  string version_string;
  if (Stdio.exist("src/version.c")) {
    // Pike 0.4pl2 or later.
    data = Stdio.read_bytes("src/version.c");
    if (data && sscanf(data,
		       "%*s f_version(INT32 args)\n"
		       "{\n"
		       "  pop_n_elems(args);\n"
		       "  push_text(\"%s\");\n",
		       version_string) >= 2) {
      return version_string;
    }
  }
  if (Stdio.exist("src/main.c")) {
    // A few versions during Pike 0.4pl2.
    data = Stdio.read_bytes("src/main.c");
    if (data && sscanf(data,
		       "%*s#define VERSION%*[ \t\"]%s\"",
		       version_string) >= 3) {
      return version_string;
    }
  }
  if (Stdio.exist("lib/master.pike")) {
    // Pike 0.4pl11 and Pike 0.4pl2.
    data = Stdio.read_bytes("lib/master.pike");
    if (data && sscanf(data,
		       "%*s#define VERSION%*[ \t\"]%s\"",
		       version_string) >= 3) {
      return version_string;
    }
    // Pike 0.1 and later.
    if (data && sscanf(data,
		       "%*sadd_constant(\"version\",lambda() { return \"%s\";",
		       version_string) >= 2) {
      return version_string;
    }
  }
  if (Stdio.exist("lib/simulate.lpc")) {
    // ulpc
    data = Stdio.read_bytes("lib/simulate.lpc");
    if (data && sscanf(data,
		       "%*sadd_efun(\"version\",lambda() { return \"%s\";",
		       version_string) >= 2) {
      return version_string;
    }
  }
  if (Stdio.exist(".cvsignore")) {
    // ulpc.
    data = Stdio.read_bytes(".cvsignore");
    string best;
    build = 0x7fffffff;
    while (data && sscanf(data, "%*suLPC_v%s.tar.gz\n%s",
			  version_string, data) >= 2) {
      int major_, minor_, exp_ = 0;
      if (sscanf(version_string, "%d.%dE-%d", major_, minor_, exp_) >= 2) {
	if ((exp_ < build) ||
	    ((exp_ == build) && ((major_ > major) ||
				 ((major_ == major) && (minor_ > minor))))) {
	  build = exp_;
	  major = major_;
	  minor = minor_;
	  best = version_string;
	}
      }
    }
    if (best) return "uLPC v" + best;
  }
  if (!Stdio.exist("src/svalue.c")) {
    // Not the Pike source proper.
    // Probably a module of some sort.
    if (Stdio.exist("lib/modules/Remote.pmod/Client.pike")) {
      return "pike_modules/Remote";
    }
    if (Stdio.exist("lib/modules/Protocols.pmod/IMAP.pmod/server.pike")) {
      return "pike_modules/Protocols.IMAP";
    }
    if (Stdio.exist("src/post_modules/GL/gen.pike")) {
      return "pike_modules/GL";
    }
    if (Stdio.exist("lib/modules/Search.pmod/types.h") ||
	Stdio.exist("lib/modules/Search.pmod/module.pmod") ||
	Stdio.exist("src/modules/_WhiteFish/whitefish.c")) {
      return "pike_modules/Search";
    }
    if (Stdio.exist("lib/modules/Filesystem.pmod/Monitor.pmod/basic.pike")) {
      return "pike_modules/Filesystem.Monitor";
    }
    if (Stdio.exist("src/modules/Tokenizer/Tokenizer.cmod")) {
      return "pike-modules/Tokenizer";
    }
    if (Stdio.exist("1x1/01-baseline-preview.jpg")) {
      return "extra-tests";
    }
    if (Stdio.exist("tools/sprshd")) {
      return "nt-tools";
    }
    if (Stdio.is_dir("refs")) {
      return "rxnpatch";
    }
  }

  error("Unable to determine version of Pike!\n");
  return UNDEFINED;
}

string prev_img;

void extract_autodoc(mapping(string:array(string)) src_commit)
{
  if (verbose) {
    progress("Extracting... ");
  }
#if 0
  rm("build/autodoc.xml.stamp");
  rm("build/autodoc.xml");
#endif
  string img;
  string imgsrc;
  foreach(({"refdoc/src_images", "src/modules/Image/doc", "tutorial" }),
	  imgsrc) {
    if (Stdio.exist(img = imgsrc + "/image_ill.pnm") ||
	Stdio.exist(img = imgsrc + "/lena.ppm") ||
	Stdio.exist(img = imgsrc + "/lena.gif") ||
	Stdio.exist(img = imgsrc + "/lenna.rs")) break;
  }

  if (img != prev_img) {
    // We have a new lena() image.
    //
    // We need to invalidate the cached image documentation
    // from the previous pass, so that the images can be
    // regenerated with the new lena() image.
    foreach(({ "src/modules/image",
	       "src/modules/Image",
	       "src/modules/Image/encodings",
	    }),
	    string module_dir) {
      module_dir = "build/doc/" + module_dir;
      if (Stdio.is_dir(module_dir)) {
	foreach(get_dir(module_dir) || ({}), string fn) {
	  if (has_suffix(fn, ".stamp")) rm(module_dir + "/" + fn);
	}
      }
    }
  }
  prev_img = img;

  // .autodoc files for the compat version directories were
  // missing in early versions of Pike 7.2. Ensure that the
  // documentation is extracted to the correct namespace.
  foreach(({ "", "0.6", "7.0", "7.2" }), string compat_version) {
    string module_dir = "lib/" + compat_version + "/modules";
    if (sizeof(compat_version)) compat_version += "::";
    if (Stdio.is_dir(module_dir) && !Stdio.exist(module_dir + "/.autodoc")) {
      Stdio.write_file(module_dir + "/.autodoc", compat_version + "\n");
#if constant(System.utime)
      // Ensure that we don't invalidate the already extracted
      // files due to recreating this file for every revision.
      System.utime(module_dir + "/.autodoc", 0, 0);
#endif
    }
  }

  string timestamp = (src_commit->author[0]/" ")[-2];
  int num_changes;

  if (verbose) {
    progress("Extracting from src... ");
  }
  num_changes +=
    Tools.Standalone.extract_autodoc()->
    main(12, ({ "extract_autodoc", "-q", "--compat", "--count",
		"--keep-going", "--no-dynamic",
		"--srcdir=src", "--source-timestamp", timestamp,
		"--imgsrc=" + imgsrc,
		"--builddir=build/doc/src",
		"--imgdir=build/doc/images" }));
  if (verbose) {
    progress("Extracting from lib... ");
  }
  num_changes +=
    Tools.Standalone.extract_autodoc()->
    main(12, ({ "extract_autodoc", "-q", "--compat", "--count",
		"--keep-going", "--no-dynamic",
		"--srcdir=lib", "--source-timestamp", timestamp,
		"--imgsrc=" + imgsrc,
		"--builddir=build/doc/lib",
		"--imgdir=build/doc/images" }));
  if (verbose) {
    progress("Extracting from doc... ");
  }
  num_changes +=
    Tools.Standalone.extract_autodoc()->
    main(12, ({ "extract_autodoc", "-q", "--compat", "--count",
		"--keep-going", "--no-dynamic",
		"--srcdir=doc", "--source-timestamp", timestamp,
		"--imgsrc=" + imgsrc,
		"--builddir=build/doc/doc",
		"--imgdir=build/doc/images" }));
  if (num_changes) {
    if (verbose) {
      progress("Joining... ");
    }
    Tools.Standalone.join_autodoc()->
      main(5, ({ "join_autodoc", "--quiet", "--post-process",
		 "build/autodoc.xml", "build/doc" }));
  } else if (verbose) {
    progress("No changes to join. ");
  }
  if (!Stdio.exist("build/autodoc.xml")) {
    // No autodoc.xml created.
    if (verbose) {
      progress("Joining failure. ");
    }
    throw(UNDEFINED);
  }
}

void export_images(Parser.XML.Tree.SimpleNode node,
		   string img_src_dir, string img_dest_dir)
{
  if (!(< Parser.XML.Tree.XML_ROOT,
	  Parser.XML.Tree.XML_ELEMENT >)[node->get_node_type()]) return;
  if (node->get_tag_name() == "image") {
    string fname = node->get_attributes()->file;
    if (fname && Stdio.exist(img_src_dir + "/" + fname)) {
      exporter->export(img_src_dir + "/" + fname, img_dest_dir + "/" + fname);
    }
  } else {
    foreach(node->get_elements(), node) {
      export_images(node, img_src_dir, img_dest_dir);
    }
  }
}

void assemble_autodoc(mapping(string:array(string)) src_commit)
{
  string refdocdir = "build/refdoc";
  if (Stdio.exist("refdoc/structure/modref.xml")) {
    // After modref.xml was added, we can use the refdoc files
    // from the repository for assembly.
    refdocdir = "refdoc";
  }

  exporter->export(combine_path(refdocdir, "src_images"), "images");
  exporter->export(combine_path(refdocdir, "structure/modref.css"),
		   "modref/style.css");
  if (verbose) {
    progress("Assembling... ");
  }
  rm("build/onepage.xml");
  rm("build/traditional.xml");
  rm("build/modref.xml");
  string pike_version = get_version();
  string timestamp = (src_commit->author[0]/" ")[-2];
  string ctimestamp = (src_commit->committer[0]/" ")[-2];

  if ((((int)timestamp) + 86400) < ((int)ctimestamp)) {
    // More than 24 hours between author time and commit time.
    // Use commit time instead. This is to avoid huge backdates
    // (several years in some cases) in the generated manual.
    timestamp = ctimestamp;
  }

  Parser.XML.Tree.SimpleRootNode autodoc =
    Parser.XML.Tree.simple_parse_file("build/autodoc.xml");
  export_images(autodoc, "build/doc/images", "images");

  if (Stdio.exist("refdoc/structure/onepage.xml")) {
    // The single page layout is only interesting
    // after the chapters have been written...
    Tools.Standalone.assemble_autodoc()->
      main(12, ({ "assemble_autodoc", "-o", "build/onepage.xml", "--compat",
		  "--keep-going", "-q", "--pike-version", pike_version,
		  "--timestamp", timestamp,
		  combine_path(refdocdir, "structure/onepage.xml"),
		  "build/autodoc.xml" }));
  }
  Tools.Standalone.assemble_autodoc()->
    main(12, ({ "assemble_autodoc", "-o", "build/traditional.xml", "--compat",
		"--keep-going", "-q", "--pike-version", pike_version,
		"--timestamp", timestamp,
		combine_path(refdocdir, "structure/traditional.xml"),
		"build/autodoc.xml" }));
  Tools.Standalone.assemble_autodoc()->
    main(11, ({ "assemble_autodoc", "-o", "build/modref.xml",
		"--keep-going", "-q", "--pike-version", pike_version,
		"--timestamp", timestamp,
		combine_path(refdocdir, "structure/modref.xml"),
		"build/autodoc.xml" }));
}

void export_refdoc(mapping(string:array(string)) src_commit)
{
  string refdocdir = "build/refdoc"; //this_program::refdocdir;
  if (Stdio.exist("refdoc/structure/modref.html")) {
    // After modref.xml was added, we can use the refdoc files
    // from the repository for html.
    refdocdir = "refdoc";
  }

  if (verbose) {
    progress("HTML... ");
  }
  Tools.Standalone.autodoc_to_html converter =
    Tools.Standalone.autodoc_to_html();
  converter->flags = Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_QUIET|
    Tools.AutoDoc.FLAG_NO_DYNAMIC|Tools.AutoDoc.FLAG_COMPAT;
  converter->verbosity = Tools.AutoDoc.FLAG_QUIET;
  converter->image_path = "../images/";
  if (Stdio.exist("build/onepage.xml")) {
    // onepage:
    converter->low_main("Pike Reference Manual", "build/onepage.xml",
			"index.html", exporter);
  }
  // traditional:
  converter->low_main("Pike Reference Manual", "build/traditional.xml",
		      "index.html", exporter);
  if (verbose) {
    progress("Splitted HTML... ");
  }
  // modref:
  Tools.Standalone.autodoc_to_split_html splitter =
    Tools.Standalone.autodoc_to_split_html();
  splitter->flags = Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_QUIET|
    Tools.AutoDoc.FLAG_NO_DYNAMIC|Tools.AutoDoc.FLAG_COMPAT;
  splitter->verbosity = Tools.AutoDoc.FLAG_QUIET;
  splitter->image_path = "../images/";
  splitter->default_namespace = "predef";
  splitter->low_main("build/modref.xml",
		     combine_path(refdocdir, "structure/modref.html"),
		     "modref",
		     exporter);
}

void export_autodoc_for_ref(string ref)
{
  string start_doc_rev;
  string start_src_rev;
  array(string) src_revs;
  if ((start_doc_rev = doc_refs[ref]) &&
      (start_src_rev = doc_to_src[start_doc_rev] &&
       doc_to_src[start_doc_rev][-1])) {
    src_revs = git("rev-list", "--topo-order",
		   start_src_rev + ".." + src_refs[ref])/"\n";
    exporter->reset(ref, start_doc_rev);
  } else {
    // This is a not previously exported ref.
    src_revs = git("rev-list", "--topo-order", src_refs[ref])/"\n";
    exporter->reset(ref);
  }
  foreach(reverse(src_revs); int i; string src_rev) {
    if (!sizeof(src_rev)) continue;
    if (!(i & 0x3f)) exporter->checkpoint();
    if (verbose) {
      progress("");
      werror("\rExporting %s (%d/%d)... ", ref, i, sizeof(src_revs));
    }
    string doc_rev;
    if (doc_rev = src_to_doc[src_rev]) {
      exporter->reset(ref, doc_rev);
      doc_refs[ref] = doc_rev;
      continue;
    }
    // Not previously converted.

    // Check out the source.
    if (verbose) {
      progress("Checkout... ");
    }
    git("checkout", "-f", src_rev);
    git("clean", "-f", "-d", "-q", "src", "lib");

    // Create a corresponding commit in the documentation.
    mapping(string:array(string)) src_commit = get_commit(work_git, src_rev);
    array(string) doc_parents = ({});
    string prev_autodoc_sha1;
    string prev_refdoc_sha1 = "";
    if (src_commit->parent) {
      doc_parents = Array.uniq(map(src_commit->parent, src_to_doc));
      prev_refdoc_sha1 = get_refdoc_sha1(src_commit->parent[0]);
    }

    if (sizeof(doc_parents) > 1) {
      // Check if all of the parents are needed.
      // We check the first 1000 unique parents.

      ADT.Queue parent_queue = ADT.Queue(@doc_parents);
      multiset(string) visited = (<>);
      int loop_count;

      while (sizeof(parent_queue) && sizeof(doc_parents) > 1) {
	string p = parent_queue->get();
	if (visited[p]) continue;
	visited[p] = 1;

	array(string) grandparents = get_doc_parents(p);
	if (grandparents) {
	  doc_parents -= grandparents;
	  if (sizeof(doc_parents) <= 1) break;
	  foreach(grandparents, string gp) {
	    if (visited[gp]) continue;
	    parent_queue->put(gp);
	  }
	}
	if (++loop_count >= 1000) break;
      }
    }

    if (sizeof(doc_parents)) {
      prev_autodoc_sha1 = get_autodoc_hash(doc_parents[0]);
    }

    string doc_mark;
    mixed err = catch {
      // Create the autodoc.xml blob.
      extract_autodoc(src_commit);

      if (!Stdio.exist(work_dir + "/build/autodoc.xml")) {
	if (verbose) {
	  progress("Fail!");
	}
	break;
      }

      string autodoc_xml = Stdio.read_bytes(work_dir + "/build/autodoc.xml");
      string new_autodoc_sha1 = Git.hash_blob(autodoc_xml);

      // If it hasn't changed, we don't need to do anything.
      if (new_autodoc_sha1 == prev_autodoc_sha1) {

	// Except if the /refdoc directory has changed...
	string refdoc_sha1 = get_refdoc_sha1(src_rev);
	if (refdoc_sha1 == prev_refdoc_sha1)
	  break;
      }

      if (!sizeof(doc_parents)) {
	// Force the commit to not have any parents.
	exporter->reset(ref);
      }
      doc_mark = new_mark();
      exporter->commit(ref, doc_mark, src_commit->author[0],
		       src_commit->committer[0],
		       src_commit->message*"\n",
		       @doc_parents);

      doc_to_parents[doc_mark] = doc_parents;

      // Start from a clean tree.
      exporter->filedeleteall();

      // Turn off Jekyll processing at GitHub.
      exporter->filemodify(Git.MODE_FILE, ".nojekyll");
      exporter->data("");

      // Export the autodoc.xml blob.
      exporter->filemodify(Git.MODE_FILE, "autodoc.xml");
      exporter->data(autodoc_xml);

      if (Stdio.exist("build/resolution.log")) {
	exporter->export("build/resolution.log", "resolution.log.txt");
      }

      autodoc_hash[doc_mark] = prev_autodoc_sha1 = new_autodoc_sha1;

      // Assemble the autodoc.
      assemble_autodoc(src_commit);

      if (Stdio.exist("build/onepage.xml")) {
	exporter->export("build/onepage.xml", "onepage.xml");
      }
      if (Stdio.exist("build/traditional.xml")) {
	exporter->export("build/traditional.xml", "traditional.xml");
      }
      if (Stdio.exist("build/modref.xml")) {
	exporter->export("build/modref.xml", "modref.xml");
      }

      // Generate and export the html files.
      export_refdoc(src_commit);
    };
    if(!doc_mark) {
      // No change since last commit.
      if (sizeof(doc_parents) != 1) {
	// We need to make a new commit, since we need to merge.
	doc_mark = new_mark();
	exporter->commit(ref, doc_mark, src_commit->author[0],
			 src_commit->committer[0],
			 src_commit->message*"\n",
			 @doc_parents);
	autodoc_hash[doc_mark] = prev_autodoc_sha1;

	doc_to_parents[doc_mark] = doc_parents;
      } else {
	doc_mark = doc_parents[0];
      }
    }
    add_src_to_doc_map(src_rev, doc_mark);
    if (verbose) {
      progress("Done. ");
    }
    exporter->commit("refs/notes/source_revs", UNDEFINED,
		     src_commit->author[0],
		     src_commit->committer[0],
		     src_commit->message*"\n",
		     @git_source_revs_heads);
    if (sizeof(git_source_revs_heads)) {
      // Special case needed to work around initialization bugs
      // for notes in git-fast-import (as of 1.7.8.rc3).
      exporter->filedeleteall();
      foreach(doc_to_src; string mark_or_sha1; array(string) source_sha1s) {
	exporter->notemodify(mark_or_sha1);
	exporter->data(source_sha1s*"\n" + "\n");
      }
      git_source_revs_heads = ({});
    } else {
      exporter->notemodify(doc_mark);
      exporter->data(doc_to_src[doc_mark]*"\n" + "\n");
    }

    // Update any refs to the just committed rev.
    if (rev_refs[src_rev]) {
      int flush;
      foreach(rev_refs[src_rev];string src_ref;) {
	if (src_ref != ref) {
	  exporter->reset(src_ref, doc_mark);
	  m_delete(src_refs, src_ref);
	  if (verbose) {
	    progress(sprintf("Updating %s. ", src_ref));
	  }
	  flush |= has_prefix(src_ref, "refs/heads/");
	}
      }
      if (flush) {
	exporter->checkpoint();
      }
    }

    if (err) {
      string msg = describe_error(err);
      if (!has_prefix(msg, "process_line failed: ") &&
          !has_prefix(msg, "PikeExtractor: undocumented ") &&
          !has_prefix(msg, "Malformed argument list ")) {
	werror("\nmsg: %O\n", msg);
	werror("%s", describe_backtrace(err));
	exit(1);
      }
    }
  }
  if (verbose) {
    werror("\n");
  }
}

int main(int argc, array(string) argv)
{
  string src_git;
  foreach(Getopt.find_all_options(argv, ({
     ({ "version", Getopt.NO_ARG,  "-V,--version"/"," }),
     ({ "verbose", Getopt.NO_ARG,  "-v,--verbose"/"," }),
     ({ "help",    Getopt.NO_ARG,  "-h,--help"/"," }),
     ({ "git-dir", Getopt.HAS_ARG, "--git-dir,--gitdir"/"," }),
     ({ "source",  Getopt.HAS_ARG, "--source-repository"/"," }),
				  })), array(string) opt) {
    switch(opt[0]) {
    case "verbose":
      verbose += 1;
      break;
    case "version":
      if (verbose) write("Git export autodoc version ");
      write("%s\n", version_string);
      return 0;
    case "help":
      write("pike -x git_export_autodoc [--git-dir <doc-repository>]\n"
	    "\t[--source-repository <pike-repository>]\n"
	    "\t[-h|--help] [-v|--verbose] [-V|--version]\n");
      return 0;
    case "git-dir":
      if (git_dir)
	warn("Destination git repository specified multiple times.\n");
      git_dir = opt[1];
      break;
    case "source":
      if (src_git)
	warn("Source git repository specified multiple times.\n");
      src_git = opt[1];
      break;
    }
  }


  exporter = Git.Export(git_dir);

  if (git_dir) {
    // Make absolute path.
    git_dir = combine_path(getcwd(), git_dir);
  }

  if (git_dir && Stdio.exist(git_dir)) {
    string desc = Stdio.read_bytes(git_dir + "/description");
    if (!desc || has_prefix(desc, "Unnamed repository")) {
      // Set a description.
      Stdio.write_file(git_dir + "/description",
		       "Autodoc: Pike documentation.\n");
    }

    if (!Stdio.exist(git_dir + "/git-daemon-export-ok")) {
      // Make the repository available via anonymous git.
      Stdio.write_file(git_dir + "/git-daemon-export-ok", "");
    }

    // Note that the git-show-ref command used by git_refs() below
    // will fail with code 1 for the empty git repository.
    mixed err = catch {
      // Get the initial state of the doc git directory, so
      // that we can run incrementally.
      get_refs(git_dir, doc_refs);

      if (m_delete(doc_refs, "refs/notes/source_revs")) {
	git_source_revs_heads = ({ "refs/notes/source_revs^0" });

	// Initialize the forward and reverse mappings.
	array(string) revs =
	  Git.git(git_dir,
		  "ls-tree", "refs/notes/source_revs",
		  "--name-only", "-r")/"\n";

	int i;
	string doc_sha1;
	array(string) src_sha1s;
	if (verbose) {
	  werror("\rRetrieving existing doc commits (%d/%d)... ",
		 i, sizeof(revs));
	}
	foreach(Git.git(git_dir, "log", "--all", "--format=raw",
			"--notes=refs/notes/source_revs")/"\n",
		string line) {
	  if (!sizeof(line)) continue;
	  if (has_prefix(line, "    ")) {
	    // Log message or source_rev.
	    if (src_sha1s) {
	      src_sha1s += ({ line[4..] });
	      src_to_doc[line[4..]] = doc_sha1;
	    }
	  } else {
	    array(string) fields = line/" ";
	    switch(fields[0]) {
	    case "commit":
	      if (doc_sha1 && src_sha1s && sizeof(src_sha1s)) {
		doc_to_src[doc_sha1] = src_sha1s;
		src_sha1s = UNDEFINED;
		i++;
	      }
	      if (verbose) {
		werror("\rRetrieving existing doc commits (%d/%d)... ",
		       i+1, sizeof(revs));
	      }
	      doc_sha1 = fields[1];
	      break;
	    case "Notes":
	      if (fields[1] == "(source_revs):") {
		src_sha1s = ({});
	      } else if (doc_sha1 && src_sha1s && sizeof(src_sha1s)) {
		doc_to_src[doc_sha1] = src_sha1s;
		src_sha1s = UNDEFINED;
		i++;
	      }
	      break;
	    }
	  }
	}
	if (doc_sha1 && src_sha1s && sizeof(src_sha1s)) {
	  doc_to_src[doc_sha1] = src_sha1s;
	  i++;
	}
	// FIXME: Check that all notes were accounted for?
	if (verbose) werror("\n");
      }
    };
    if (err) {
      werror("%s\n", describe_backtrace(err));
    }
  }

  // Avoid broken commits on failure.
  exporter->feature("done");
  // We use notes to store our state.
  exporter->feature("notes");

  if (!Stdio.exist(work_dir + "/.git")) {
    // Default to the canonical git URL for Pike.
    if (!src_git) {
      src_git = "git://pike-git.lysator.liu.se/pike.git";
      if (verbose) {
	warn("No source repository specified.\n"
	     "Defaulting to %O.\n", src_git);
      }
    }
    if (Stdio.exist(work_dir) && !rm(work_dir)) {
      werror("Move away %O, it is in the way.\n", work_dir);
      exit(1);
    }
    git("clone", "-n", src_git, work_dir);
    cd(work_dir);
  } else {
    cd(work_dir);
    if (src_git) {
      if (catch {
	  string old_src_git =
	    String.trim_all_whites(git("config", "--get",
				       "remotes.origin.url"));
	  if (old_src_git != src_git) {
	    if (verbose) {
	      werror("Updating source repository URL to %O.\n", src_git);
	    }
	    git("config", "--set", "remotes.origin.url", src_git);
	  }
	}) {
	if (verbose) {
	  werror("Adding remote repository URL %O as origin.\n", src_git);
	}
	git("remote", "add", "-f", "--tags", "origin", src_git);
      }
    }
    git("fetch", "--force", "--prune", "--tags", "--update-head-ok", "origin");
    // Due to bugs in --prune in some versions of git (eg 1.7.7),
    // we may have lost the remote heads above, so we refetch them.
    git("fetch", "--force", "--tags", "--update-head-ok", "origin",
	"+refs/heads/*:refs/remotes/origin/*");

    // Clean the work dir.
    if (!has_prefix(git("stash"), "No local changes ")) {
      git("stash", "drop");
    }
  }

  // Make more absolute paths.
  work_git = combine_path(work_dir = getcwd(), ".git");

  // Make sure some directories exist.
  mkdir("build");
  mkdir("build/doc");
  mkdir("build/doc/src");
  mkdir("build/doc/lib");
  mkdir("build/doc/doc");
  mkdir("build/doc/images");

  // Git between 1.7.7 and 1.7.8.1 have an infinite loop in the
  // LF to CRLF filter, which is triggered by some Pike commits.
  // Since we don't care about CRLF (or in fact the affected files
  // at all), we just disable CRLF conversion entirely.
  Stdio.write_file(work_git + "/info/attributes",
		   "# Kludge around infinite loop in git 1.7.7 and 1.7.8.\n"
		   "* eol=lf\n");

  // Set up a refdoc directory with the initial version of modref.html et al.
  if (!Stdio.exist("build/refdoc/structure/modref.html")) {
    werror("Searching for original refdoc version...");
    array(string) revs = git("rev-list", "refs/remotes/origin/7.4",
			     "--", "refdoc/structure/modref.html")/"\n" -
      ({ "" });
    if (!sizeof(revs)) {
      werror("\nFailed to find initial revision for "
	     "refdoc/structure/modref.html.\n");
      exit(1);
    }

    string rev = revs[-1];
    werror(" %s\n", rev);
    git("checkout", "-f", rev);
    if (!Stdio.exist("refdoc/structure/modref.html")) {
      werror("No refdoc/structure/modref.html in revision!\n");
      exit(1);
    }
    if (!Stdio.cp("refdoc", "build/refdoc")) {
      werror("Failed to copy initial refdoc directory.\n");
      exit(1);
    }
  }

  // Get the current references for the source directory.
  get_refs(work_git, src_refs, 1);

  // Find the references which need to be updated.

  // First remove any refs that are obsolete.
  //
  // Note that this can only happen if we have a git_dir.
  foreach(doc_refs - src_refs; string ref; string sha1) {
    if (ref == "HEAD") continue;
    if (verbose) {
      werror("Deleting obsolete %s (obsolete).\n", ref);
    }
    if (has_prefix(ref, "refs/heads/")) {
      Git.git(git_dir, "branch", "-D", ref[sizeof("refs/heads/")..]);
    } else if (has_prefix(ref, "refs/tags/")) {
      Git.git(git_dir, "tag", "-d", ref[sizeof("refs/tags/")..]);
    }
  }

  foreach(src_refs; string ref; string sha1) {
    rev_refs[sha1] += (< ref >);
  }

  // Start with the ref for HEAD.
  string master_ref;
  catch {
    master_ref = String.trim_all_whites(Git.git(work_git, "symbolic-ref",
						"refs/remotes/origin/HEAD"));
    if (has_prefix(master_ref, "refs/remotes/origin/")) {
      master_ref = "refs/heads/" + master_ref[sizeof("refs/remotes/origin/")..];
    }

    Git.git(git_dir, "symbolic-ref", "HEAD", master_ref);
  };

  // Then export the remaining refs.
  while (sizeof(src_refs)) {
    // Check if any of the refs is trivial to export.
    foreach(src_refs; string ref; string src_sha1) {
      string src_sha1 = src_refs[ref];
      string doc_sha1 = doc_refs[ref];
      if (doc_sha1 && (src_to_doc[src_sha1] == doc_sha1)) {
	// Already up-to-date.
	m_delete(src_refs, ref);
	if (ref == master_ref) master_ref = UNDEFINED;
      } else if (doc_sha1 = src_to_doc[src_sha1]) {
	// Already exported.
	exporter->reset(ref, doc_sha1);
	if (has_prefix(ref, "refs/heads/")) {
	  // Flush all heads as soon as possible.
	  exporter->checkpoint();
	}
	m_delete(src_refs, ref);
	if (ref == master_ref) master_ref = UNDEFINED;
      }
    }
    if (!sizeof(src_refs)) break;
    // Update one of the remaining refs at semi-random,
    // The master branch first, then the other branches,
    // and last any remaining refs.
    string ref = master_ref;
    if (!src_refs[ref]) {
      array(string) refs = filter(indices(src_refs), has_prefix, "refs/heads/");
      if (!sizeof(refs)) refs = indices(src_refs);
      ref = random(refs);
    }
    export_autodoc_for_ref(ref);
    m_delete(src_refs, ref);
    exporter->checkpoint();
    master_ref = UNDEFINED;
  }
  exporter->checkpoint();

  Stdio.stdout->close();

  return exporter->done();
}
