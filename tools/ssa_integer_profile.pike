#!/usr/bin/env pike
#charset utf-8
#pike 8.0
/* -*- mode: Pike; c-basic-offset: 2; -*- */

// Summarize stderr from runs with PIKE_SSA_INTEGER_PROFILE=1.
// Counts describe region executions, not all executed Pike instructions.

int main(int argc, array(string) argv)
{
  int json, boundaries, top = 20;
  array(string) paths = ({});
  for (int i = 1; i < argc; i++) {
    if (argv[i] == "--json") json = 1;
    else if (argv[i] == "--boundaries") boundaries = 1;
    else if (argv[i] == "--top" && i + 1 < argc) top = (int)argv[++i];
    else if (has_prefix(argv[i], "--")) {
      werror("Unknown option: %s\n", argv[i]);
      return 1;
    } else paths += ({ argv[i] });
  }
  if (!sizeof(paths) || top < 1) {
    werror("Usage: %s [--json] [--boundaries] [--top N] log...\n"
           "Use - to read standard input.\n", argv[0]);
    return 1;
  }

  mapping(string:mapping) sources = ([]), rejected_sources = ([]);
  mapping(string:mapping) stops = ([]);
  mapping(string:int) totals = ([ "entries":0, "guards":0, "overflows":0,
    "fast":0, "fast_ops":0, "rejected":0,
    "process_reports":0, "compiled_records":0 ]);
  foreach (paths, string path) {
    string data = path == "-" ? Stdio.stdin->read() : Stdio.read_file(path);
    if (!stringp(data)) {
      werror("Cannot read %s.\n", path);
      return 1;
    }
    int line_no;
    mapping(int:mapping) reports = ([]);
    foreach (data / "\n", string line) {
      line_no++;
      if (has_prefix(line, "SSA integer runtime: ")) {
        int pid, compiled, entries, guards, overflows, fast, fast_ops, rejected;
        int fields = sscanf(line, "SSA integer runtime: pid=%d compiled=%d "
                   "entries=%d guards=%d overflows=%d fast=%d fast_ops=%d "
                   "rejected=%d",
                   pid, compiled, entries, guards, overflows, fast,
                   fast_ops, rejected);
        if (fields < 7 || (search(line, " rejected=") >= 0 && fields != 8) ||
            compiled < 0 || entries < 0 || guards < 0 ||
            overflows < 0 || fast < 0 || fast_ops < 0 || rejected < 0) {
          werror("%s:%d: Invalid runtime profile summary.\n", path, line_no);
          return 1;
        }
        if (reports[pid] &&
            !equal(reports[pid]->expected, reports[pid]->seen)) {
          werror("%s:%d: Previous process report is incomplete.\n",
                 path, line_no);
          return 1;
        }
        reports[pid] = ([ "expected":([ "entries":entries, "guards":guards,
          "overflows":overflows, "fast":fast, "fast_ops":fast_ops,
                       "rejected":rejected ]),
          "seen":([ "entries":0, "guards":0, "overflows":0, "fast":0,
                     "fast_ops":0, "rejected":0 ]) ]);
        totals->process_reports++;
        totals->compiled_records += compiled;
      }
      int rejected = has_prefix(line, "SSA integer rejection\t");
      if (!rejected && !has_prefix(line, "SSA integer region\t")) continue;
      array(string) f = line / "\t";
      if ((sizeof(f) != 15 && sizeof(f) != 21) ||
          (rejected && sizeof(f) != 21)) {
        werror("%s:%d: Incomplete integer profile row.\n", path, line_no);
        return 1;
      }
      mapping report = reports[(int)f[1]];
      if (!report) {
        werror("%s:%d: Region row has no process summary.\n", path, line_no);
        return 1;
      }
      int entries = (int)f[11], guards = (int)f[12];
      int overflows = (int)f[13], fast = (int)f[14];
      if (entries <= 0 || guards < 0 || overflows < 0 || fast < 0 ||
          (rejected ? guards || overflows || fast :
                       entries != guards + overflows + fast)) {
        werror("%s:%d: Region entry/exit counts disagree.\n", path, line_no);
        return 1;
      }
      // Merge recompilations and separate processes at the same source site.
      string reason = sizeof(f) == 21 ? f[15] : "legacy";
      int offset = sizeof(f) == 21 ? (int)f[16] : -1;
      string opcode = sizeof(f) == 21 ? f[17] : f[10];
      int stop_line = sizeof(f) == 21 ? (int)f[18] : 0;
      string rejection = sizeof(f) == 21 ? f[19] : "none";
      int consumed = sizeof(f) == 21 ? (int)f[20] : -1;
      if (sizeof(f) == 21 &&
          (offset < 0 || consumed < 0 || (rejected && consumed) ||
           !sizeof(reason) || !sizeof(rejection))) {
        werror("%s:%d: Invalid planner diagnostic.\n", path, line_no);
        return 1;
      }
      string key = (f[4..10] + f[15..]) * "\t";
      mapping table = rejected ? rejected_sources : sources;
      mapping r = table[key];
      if (!r) table[key] = r = ([ "file":f[4], "line":(int)f[5],
        "last_line":(int)f[6], "planner":f[7], "ops":(int)f[8],
        "locals":(int)f[9], "next_opcode":f[10], "entries":0,
        "guards":0, "overflows":0, "fast":0, "fast_ops":0,
        "rejected":0, "stop_reason":reason, "stop_offset":offset,
        "stop_opcode":opcode, "stop_line":stop_line,
        "rejection_reason":rejection, "consumed":consumed ]);
      string stop_key = reason + "\t" + opcode;
      mapping boundary = stops[stop_key];
      if (!boundary) stops[stop_key] = boundary = ([ "stop_reason":reason,
        "stop_opcode":opcode, "rejection_reasons":([]),
        "opportunities":0, "entries":0, "fast":0, "rejected":0,
        "guards":0, "overflows":0 ]);
      boundary->opportunities += entries;
      boundary->entries += rejected ? 0 : entries;
      boundary->fast += fast;
      boundary->guards += guards;
      boundary->overflows += overflows;
      boundary->rejected += rejected ? entries : 0;
      if (rejected) boundary->rejection_reasons[rejection] += entries;
      mapping counts = ([ "entries":rejected ? 0 : entries, "guards":guards,
        "overflows":overflows, "fast":fast, "fast_ops":fast * r->ops,
        "rejected":rejected ? entries : 0 ]);
      foreach (counts; string name; int value) {
        r[name] += value;
        totals[name] += value;
        report->seen[name] += value;
      }
    }
    foreach (reports; int pid; mapping report)
      if (!equal(report->expected, report->seen)) {
        werror("%s: Process %d report is incomplete or inconsistent.\n",
               path, pid);
        return 1;
      }
  }
  if (!totals->process_reports) {
    werror("No runtime profile reports found. Run with "
           "PIKE_SSA_INTEGER_PROFILE=1 and capture stderr.\n");
    return 1;
  }
  array(mapping) rows = values(sources);
  sort(map(rows, lambda(mapping r) { return -r->entries; }), rows);
  array(mapping) rejections = values(rejected_sources);
  sort(map(rejections, lambda(mapping r) { return -r->rejected; }), rejections);
  array(mapping) boundary_rows = values(stops);
  sort(map(boundary_rows, lambda(mapping r) { return -r->opportunities; }),
       boundary_rows);
  if (json) {
    write("%s\n", Standards.JSON.encode(([ "totals":totals, "sites":rows,
      "rejections":rejections, "boundaries":boundary_rows ])));
    return 0;
  }
  write("Entries: %d; fast: %d; guard failures: %d; overflows: %d\n"
        "Operations in successful regions: %d\n",
        totals->entries, totals->fast, totals->guards,
        totals->overflows, totals->fast_ops);
  write("Rejected candidate entries: %d\n", totals->rejected);
  if (boundaries) {
    write("%12s %12s %12s %-20s %-20s %s\n", "Opportunities",
          "Regions", "Rejected", "Stop reason", "Stop opcode", "Rejections");
    foreach (boundary_rows[..min(top, sizeof(boundary_rows)) - 1], mapping r) {
      array(string) causes = ({});
      foreach (sort(indices(r->rejection_reasons)), string cause)
        causes += ({ sprintf("%s=%d", cause, r->rejection_reasons[cause]) });
      write("%12d %12d %12d %-20s %-20s %s\n", r->opportunities,
            r->entries, r->rejected, r->stop_reason, r->stop_opcode,
            causes * ", ");
    }
    return 0;
  }
  write("%12s %8s %8s %8s %-20s %-18s %s\n", "Entries", "Fast %",
        "Guard %", "Ovf %", "Stop reason", "Stop opcode", "Source");
  foreach (rows[..min(top, sizeof(rows)) - 1], mapping r)
    write("%12d %7.2f%% %7.2f%% %7.2f%% %-20s %-18s "
          "%s:%d..%d (%s, %d ops)\n",
          r->entries, 100.0 * r->fast / r->entries,
          100.0 * r->guards / r->entries,
          100.0 * r->overflows / r->entries, r->stop_reason, r->stop_opcode,
          r->file, r->line, r->last_line, r->planner, r->ops);
  write("\nRejected candidates (overlapping entry counts):\n");
  foreach (rejections[..min(top, sizeof(rejections)) - 1], mapping r)
    write("%12d %-20s %-18s %s:%d (%s; %s)\n", r->rejected,
          r->stop_reason, r->stop_opcode, r->file, r->line,
          r->planner, r->rejection_reason);
  return 0;
}
