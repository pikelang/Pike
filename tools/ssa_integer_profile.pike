#!/usr/bin/env pike
#charset utf-8
#pike 8.0
/* -*- mode: Pike; c-basic-offset: 2; -*- */

// Summarize stderr from runs with PIKE_SSA_INTEGER_PROFILE=1.
// Counts describe region executions, not all executed Pike instructions.

int main(int argc, array(string) argv)
{
  int json, top = 20;
  array(string) paths = ({});
  for (int i = 1; i < argc; i++) {
    if (argv[i] == "--json") json = 1;
    else if (argv[i] == "--top" && i + 1 < argc) top = (int)argv[++i];
    else if (has_prefix(argv[i], "--")) {
      werror("Unknown option: %s\n", argv[i]);
      return 1;
    } else paths += ({ argv[i] });
  }
  if (!sizeof(paths) || top < 1) {
    werror("Usage: %s [--json] [--top N] log...\n"
           "Use - to read standard input.\n", argv[0]);
    return 1;
  }

  mapping(string:mapping) sources = ([]);
  mapping(string:int) totals = ([ "entries":0, "guards":0, "overflows":0,
    "fast":0, "fast_ops":0, "process_reports":0, "compiled_records":0 ]);
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
        int pid, compiled, entries, guards, overflows, fast, fast_ops;
        if (sscanf(line, "SSA integer runtime: pid=%d compiled=%d "
                   "entries=%d guards=%d overflows=%d fast=%d fast_ops=%d",
                   pid, compiled, entries, guards, overflows, fast,
                   fast_ops) != 7) {
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
          "overflows":overflows, "fast":fast, "fast_ops":fast_ops ]),
          "seen":([ "entries":0, "guards":0, "overflows":0, "fast":0,
                     "fast_ops":0 ]) ]);
        totals->process_reports++;
        totals->compiled_records += compiled;
      }
      if (!has_prefix(line, "SSA integer region\t")) continue;
      array(string) f = line / "\t";
      if (sizeof(f) != 15) {
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
          entries != guards + overflows + fast) {
        werror("%s:%d: Region entry/exit counts disagree.\n", path, line_no);
        return 1;
      }
      // Merge recompilations and separate processes at the same source site.
      string key = f[4..10] * "\t";
      mapping r = sources[key];
      if (!r) sources[key] = r = ([ "file":f[4], "line":(int)f[5],
        "last_line":(int)f[6], "planner":f[7], "ops":(int)f[8],
        "locals":(int)f[9], "next_opcode":f[10], "entries":0,
        "guards":0, "overflows":0, "fast":0, "fast_ops":0 ]);
      mapping counts = ([ "entries":entries, "guards":guards,
        "overflows":overflows, "fast":fast, "fast_ops":fast * r->ops ]);
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
  if (json) {
    write("%s\n", Standards.JSON.encode(([ "totals":totals, "sites":rows ])));
    return 0;
  }
  write("Entries: %d; fast: %d; guard failures: %d; overflows: %d\n"
        "Operations in successful regions: %d\n",
        totals->entries, totals->fast, totals->guards,
        totals->overflows, totals->fast_ops);
  write("%12s %8s %8s %8s %-18s %s\n",
        "Entries", "Fast %", "Guard %", "Ovf %", "Next opcode", "Source");
  foreach (rows[..min(top, sizeof(rows)) - 1], mapping r)
    write("%12d %7.2f%% %7.2f%% %7.2f%% %-18s %s:%d..%d (%s, %d ops)\n",
          r->entries, 100.0 * r->fast / r->entries,
          100.0 * r->guards / r->entries,
          100.0 * r->overflows / r->entries, r->next_opcode,
          r->file, r->line, r->last_line, r->planner, r->ops);
  return 0;
}
