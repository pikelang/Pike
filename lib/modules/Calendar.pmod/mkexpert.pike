// Update the expert system in TZNames.pmod with the current
// set of zones.
//
// 2019-03-22 Henrik Grubbström

int main(int argc, array(string) argv)
{
  int found;
  foreach(master()->pike_module_path || ({}), string path) {
    if (has_prefix(__FILE__, path)) {
      found = 1;
      break;
    }
  }
  if (!found) {
    werror("MUST be used with the same pike as the module directory.\n");
    exit(1);
  }

  string fname =
    master()->programs_reverse_lookup(object_program(Calendar.TZnames));
  if (!fname) {
    werror("Failed to determine path to TZnames.pmod.\n");
    exit(1);
  }
  string orig_names = Stdio.read_bytes(fname);
  if (!orig_names || !sizeof(orig_names)) {
    werror("Failed to read source file %O.\n", fname);
    exit(1);
  }

  array(string) fragments = orig_names/"timezone_expert_tree =";
  if (sizeof(fragments) == 1) {
    fragments = orig_names/"timezone_expert_tree=";
  }
  if (sizeof(fragments) > 2) {
    fragments = ({
      fragments[0], fragments[1..] * "timezone_expert_tree =",
    });
  }
  fragments[1] = (fragments[1]/"]);")[1..]*"]);";

  mapping timezone_expert_tree = generate_expert_tree(get_all_zones());

  // werror("Timezone_expert_tree: %O\n", timezone_expert_tree);

  String.Buffer code = String.Buffer();
  // werror("%s", code->get());

  code->add(fragments[0]);
  code->add("timezone_expert_tree =");
  render_expert_tree(code, timezone_expert_tree);
  code->add(";");
  code->add(fragments[1]);

  string tznames = code->get();
  if (tznames != orig_names) {
    mv(fname, fname + "~");
    werror("Writing %s (%d bytes)...", fname, sizeof(tznames));
    Stdio.File(fname, "wtc")->write(tznames);
    werror("\n");
  } else {
    werror("No changes.\n");
  }
}

void render_expert_tree(String.Buffer code,
			mapping|array(string)|string tree,
			int|void indent)
{
  if (mappingp(tree)) {
    code->sprintf("\n"
		  "%*s([ %O:%d, // %s\n",
		  indent*3, "", "test", tree->test,
		  Calendar.ISO.Second(tree->test)->
		  set_timezone(Calendar.Timezone.UTC)->format_time());
    foreach(sort(indices(tree) - ({ "test" })), int offset) {
      code->sprintf("%*s   %d:", indent*3, "", offset);
      render_expert_tree(code, tree[offset], indent+2);
      code->sprintf(",\n");
    }
    code->sprintf("%*s])", indent*3, "");
  } else if (arrayp(tree)) {
    code->sprintf("\n"
"%*s({\n", indent*3, "");
    foreach(tree, string dest) {
      code->sprintf("%*s   %O,\n", indent*3, "", dest);
    }
    code->sprintf("%*s})", indent*3, "");
  } else /* stringp(tree) */ {
    code->sprintf("%O", tree);
  }
}

array(object) get_all_zones()
{
  array(object) res = ({});
  foreach(Calendar.TZnames.zones; string region; array(string) places) {
    foreach(places, string place) {
      object zone = Calendar.Timezone[region + "/" + place];
      if (zone->zoneid != region + "/" + place) {
	// Alias.
	continue;
      }
      res += ({ zone });
    }
  }
  return res;
}

mapping|array|string generate_expert_tree(array(object) zones)
{
  if (sizeof(zones) == 1) {
    // A single zone.
    return zones[0]->zoneid;
  }
  array(int) times = Array.uniq(sort(zones->shifts * ({})));
  times = ({ times[0]-1 }) + times;
  int l, h = sizeof(times);
  int start;
  while (l+1 < h) {
    int m = (l + h)/2;
    if (times[m] < 0) l = m+1;
    else h = m;
  }
  start = l;

  // First try splitting at dates post 1970-01-01.
  // This is needed because some implementations of localtime(3C)
  // do not support negative timestamps (eg AIX).
  int best_score = sizeof(zones);
  int best_split = 1;
  int best_t = 0;
  mapping(int:array(object)) best_histogram = ([]);
  for (l = start; l < sizeof(times); l++) {
    mapping(int:array(object)) histogram = mkhistogram(zones, times[l]);
    int score = max(0, @map(values(histogram), sizeof));
    int split = sizeof(histogram);
    if ((split > best_split) ||
	((split == best_split) && (score < best_score))) {
      best_score = score;
      best_split = split;
      best_t = times[l];
      best_histogram = histogram;
    }
  }
  if (best_split == 1) {
    // No suitable date found.
    // Retry with dates pre 1970-01-01.
    // Prefer more recent dates, as the ancient historic dates may
    // have been purged.
    for (l = start; l--;) {
      if ((times[l] < -0x80000000) && (best_split != 1) &&
	  (times[l+1] >= -0x80000000)) {
	// Several implementations of localtime(3C) are 32 bit.
	// We already have a useable date, so stop the search for now.
	break;
      }
      mapping(int:array(object)) histogram = mkhistogram(zones, times[l]);
      int score = max(0, @map(values(histogram), sizeof));
      int split = sizeof(histogram);
      if ((split > best_split) ||
	  ((split == best_split) && (score < best_score))) {
	best_score = score;
	best_split = split;
	best_t = times[l];
	best_histogram = histogram;
      }
    }
    if (best_split == 1) {
      // Still no suitable date found.
      return sort(zones->zoneid);
    }
  }

  mapping ret = ([
    "test":best_t,
  ]);
  foreach(best_histogram; int offset; array(object) subzones) {
    ret[offset] = generate_expert_tree(subzones);
  }
  return ret;
}

mapping(int:array(object)) mkhistogram(array(object) zones, int t)
{
  mapping(int:array(object)) res = ([]);
  foreach(zones, object zone) {
    int offset = Calendar.ISO.Second(t)->set_timezone(zone)->utc_offset();
    res[offset] += ({ zone });
  }
  return res;
}
