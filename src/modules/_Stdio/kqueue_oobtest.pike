/* Regression tests for separate kqueue OOB notifications. */

void require(int condition, string message)
{
  if (!condition) error("%s\n", message);
}

void wait_for(Pike.__Backend backend, function(:int) ready, string message)
{
  int deadline = gethrtime() + 2000000;
  while (!ready() && gethrtime() < deadline) backend(0.05);
  require(ready(), message);
}

void check_oob(int ordinary_callback, int forked)
{
  Pike.PollDeviceBackend backend = Pike.PollDeviceBackend();
  Stdio.Port listener = Stdio.Port();
  require(listener->bind(0, 0, "127.0.0.1"), "bind failed");
  int port = (int)(listener->query_address() / " ")[-1];
  Stdio.File sender = Stdio.File();
  require(sender->connect("127.0.0.1", port), "connect failed");
  Stdio.File receiver = listener->accept();
  require(!!receiver, "accept failed");
  listener->close();

  string ordinary = "";
  string urgent = "";
  function read_cb = lambda(mixed id, string data) {
    ordinary += data;
  };
  function oob_cb = lambda(mixed id, string data) {
    urgent += data;
    receiver->set_read_oob_callback(0);
  };
  receiver->set_backend(backend);
  receiver->set_nonblocking(ordinary_callback ? read_cb : 0,
                           0, 0, oob_cb, 0);

  // Register both filters before fork so the child has to restore them.
  mixed child;
#if constant(fork)
  if (forked) child = fork();
#endif
  if (child) {
    int status = child->wait();
    receiver->close();
    sender->close();
    require(!status, "OOB test failed in child");
    return;
  }

  mixed err = catch {
    string before = "ordinary before";
    string after = "ordinary after";
    require(sender->write(before) == sizeof(before), "prefix write failed");
    require(sender->write_oob("!") == 1, "OOB write failed");
    if (ordinary_callback)
      require(sender->write(after) == sizeof(after), "suffix write failed");

    // The ordinary prefix must not prevent an OOB-only callback.
    wait_for(backend, lambda() { return urgent == "!"; },
             "OOB callback missing with ordinary data pending");
    if (ordinary_callback) {
      wait_for(backend, lambda() { return ordinary == before + after; },
               "ordinary callback lost while removing OOB callback");

      require(sender->write("readable") == 8, "ordinary write failed");
      wait_for(backend,
               lambda() { return ordinary == before + after + "readable"; },
               "removing OOB callback disabled ordinary reads");
      require(urgent == "!", "unexpected OOB callback after removal");

      receiver->set_read_oob_callback(oob_cb);
      require(sender->write_oob("?") == 1, "second OOB write failed");
      wait_for(backend, lambda() { return urgent == "!?"; },
               "OOB callback was not re-enabled");
    } else {
      require(receiver->read(sizeof(before), 1) == before,
              "OOB read consumed ordinary data");
    }
  };
  receiver->close();
  sender->close();
  if (forked) {
    if (err) werror("%s", describe_backtrace(err));
    exit(!!err);
  }
  if (err) throw(err);
}

int main(int argc, array(string) argv)
{
  int ordinary = argc > 1 && (int)argv[1];
  int succeeded;
  int failed;
  int skipped;
  // Fork before waiting for children starts Pike's process handling thread.
  foreach (({1, 0}), int forked) {
#if !constant(fork)
    if (forked) {
      skipped++;
      continue;
    }
#endif
    mixed err = catch { check_oob(ordinary, forked); };
    if (err) {
      failed++;
      werror("ordinary=%d forked=%d: %s", ordinary, forked,
             describe_backtrace(err));
    } else {
      succeeded++;
    }
  }
  Tools.Testsuite.report_result(succeeded, failed, skipped);
  return !!failed;
}
