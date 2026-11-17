#!/usr/bin/env pike
#charset utf-8
#pike 8.0
/* -*- mode: Pike; c-basic-offset: 2; -*- */

// Run with: pike tools/ssa_integer_profile_test.pike [pike [Pike options...]]
// The runtime checks require an SSA-enabled AMD64 or SPARC build; parser
// fixtures also run with an installed Pike 8.0 and older profile logs.

array(string) command;
string tool;
mapping environment;
int checks;

mapping parse(string data, int valid)
{
  mapping result = Process.run(command + ({ tool, "--json", "-" }),
                              ([ "stdin":data, "env":environment ]));
  if (!!result->exitcode == !!valid)
    error("Unexpected parser status for %O:\n%s\n", data, result->stderr);
  checks++;
  return valid ? Standards.JSON.decode(result->stdout) : ([]);
}

string summary(int pid, int entries, int rejected, int guards,
               int overflows, int fast, int fast_ops)
{
  return sprintf("SSA integer runtime: pid=%d compiled=2 entries=%d "
                 "guards=%d overflows=%d fast=%d fast_ops=%d rejected=%d\n",
                 pid, entries, guards, overflows, fast, fast_ops, rejected);
}

string row(int pid, int rejected, int n)
{
  return sprintf("SSA integer %s\t%d\t1\t42\tfixture.pike\t10\t11\tstack\t"
                 "2\t1\treturn\t%d\t0\t0\t%d\tunsupported-opcode\t"
                 "4\t*\t12\t%s\t%d\n", rejected ? "rejection" : "region",
                 pid, n, rejected ? 0 : n, rejected ? "too-short" : "none",
                 rejected ? 0 : 3);
}

int main(int argc, array(string) argv)
{
  command = argc > 1 ? argv[1..] : ({ "pike" });
  tool = combine_path(dirname(__FILE__), "ssa_integer_profile.pike");
  environment = getenv();
  foreach (indices(environment), string key)
    if (has_prefix(key, "PIKE_SSA_")) m_delete(environment, key);

  string legacy = "SSA integer runtime: pid=1 compiled=1 entries=6 "
    "guards=1 overflows=4 fast=1 fast_ops=2\n"
    "SSA integer region\t1\t1\t42\told.pike\t1\t2\tstack\t2\t1\t"
    "return\t6\t1\t4\t1\n";
  mapping p = parse(legacy, 1);
  if (p->totals->entries != 6 || p->totals->rejected ||
      p->sites[0]->stop_reason != "legacy") error("Legacy profile changed.\n");

  string modern = summary(2, 7, 5, 0, 0, 7, 14) + row(2, 0, 7) + row(2, 1, 5);
  p = parse(modern, 1);
  if (p->totals->rejected != 5 || p->sites[0]->consumed != 3 ||
      p->sites[0]->stop_offset != 4 || p->rejections[0]->rejected != 5 ||
      sizeof(p->boundaries) != 1 || p->boundaries[0]->opportunities != 12 ||
      p->boundaries[0]->rejection_reasons["too-short"] != 5)
    error("Lost planner diagnostic.\n");
  p = parse(modern + replace(replace(modern, "pid=2", "pid=3"),
                             "\t2\t1\t42", "\t3\t1\t42"), 1);
  if (p->totals->entries != 14 || p->totals->rejected != 10 ||
      sizeof(p->sites) != 1 || sizeof(p->rejections) != 1)
    error("Process/site aggregation changed.\n");
  int large = (1 << 32) + 17;
  p = parse(summary(4, large, large, 0, 0, large, 2 * large) +
            row(4, 0, large) + row(4, 1, large), 1);
  if (p->totals->entries != large || p->totals->rejected != large)
    error("Counter width lost.\n");
  parse(summary(2, 0, 0, 0, 0, 0, 0), 1);
  parse("", 0);
  parse(row(2, 1, 5), 0);
  parse(summary(2, 7, 5, 0, 0, 7, 14) + row(2, 0, 7), 0);
  parse(summary(2, 0, 5, 0, 0, 0, 0), 0);
  parse(replace(modern, "fast=7", "fast=6"), 0);
  parse(replace(modern, "rejected=5", "rejected=-5"), 0);
  parse(replace(modern, "rejected=5", "rejected=bad"), 0);
  parse(replace(modern, "\ttoo-short\t0\n", "\ttoo-short\t1\n"), 0);
  parse(replace(modern, "\t4\t*\t12", "\t-1\t*\t12"), 0);
  parse(replace(modern, "\ttoo-short\t0\n", "\ttoo-short\n"), 0);
  parse(replace(modern, "\t5\t0\t0\t0", "\t5\t0\t0\t1"), 0);
  parse(summary(2, 0, 5, 0, 0, 0, 0) + modern, 0);

  string source = "int multiply(int a,int b) { return a*b; }\n"
    "int region(int a,int b) { a++; a++; return a+b; }\n";
  string code = sprintf("object o=compile_string(%O, %O)(); "
    "for(int i=0;i<3;i++) if(o->multiply(3,4)!=12 || o->region(3,4)!=9) "
    "error(\"arithmetic changed\\n\"); write(\"ok\\n\");",
    source, "ssa-profile-test.pike");
  mapping plain = Process.run(command + ({ "-e", code }),
                             ([ "env":environment ]));
  if (plain->exitcode || plain->stdout != "ok\n" ||
      search(plain->stderr, "SSA integer runtime:") >= 0)
    error("Unprofiled control failed: %O\n", plain);
  mapping runtime = Process.run(command + ({ "-e", code }),
    ([ "env":environment + ([ "PIKE_SSA_INTEGER_PROFILE":"1" ]) ]));
  if (runtime->exitcode || runtime->stdout != plain->stdout)
    error("Profiled arithmetic changed: %O\n", runtime);
  if (search(runtime->stderr, "SSA integer runtime:") < 0)
    write("SKIP runtime counters: this interpreter has no "
          "integer profiling.\n");
  else {
    p = parse(runtime->stderr, 1);
    int entries, fast, rejected;
    foreach (p->sites, mapping site)
      if (site->file == "ssa-profile-test.pike") {
        entries += site->entries;
        fast += site->fast;
      }
    foreach (p->rejections, mapping site)
      if (site->file == "ssa-profile-test.pike" && site->stop_opcode == "*")
        rejected += site->rejected;
    if (entries != 3 || fast != 3 || rejected != 3)
      error("Unexpected runtime boundary counts: %d/%d/%d\n",
            entries, fast, rejected);
    checks++;
  }
  write("PASS %d profile checks, including legacy logs and malformed input.\n",
        checks);
  return 0;
}
