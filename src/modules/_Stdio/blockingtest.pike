/* Regression tests for emulated blocking socket operations. */

void require(int condition, string message)
{
  if (!condition) error("%s\n", message);
}

int number(Stdio.Port port)
{
  return (int)(port->query_address() / " ")[-1];
}

void check_blocking(Stdio.File file, Stdio.File peer)
{
  require(!(file->mode() & Stdio.PROP_IS_NONBLOCKING), "nonblocking mode");
  Thread.Thread writer = Thread.Thread(lambda() {
    sleep(0.05);
    require(peer->write("x") == 1, "peer write failed");
  });
  require(file->read(1, 1) == "x", "blocking read did not wait for data");
  writer->wait();
}

void run_tests()
{
  Stdio.Port port = Stdio.Port();
  require(port->bind(0, 0, "127.0.0.1"), "bind failed");
  array(Thread.Thread) waiters = ({});
  for (int i = 0; i < 2; i++)
    waiters += ({Thread.Thread(lambda() { return port->accept(); })});
  sleep(0.05);
  port->set_accept_callback(lambda(mixed id) {});
  require(!port->accept(), "nonblocking accept unexpectedly succeeded");
  port->set_accept_callback(0);
  array(Stdio.File) clients = ({});
  for (int i = 0; i < 2; i++) {
    Stdio.File client = Stdio.File();
    require(client->connect("127.0.0.1", number(port)), "connect failed");
    clients += ({client});
    // Let one accept finish while the other remains blocked.
    sleep(0.05);
  }
  foreach (waiters, Thread.Thread waiter) {
    Stdio.File accepted = waiter->wait();
    require(!!accepted, "concurrent accept returned no connection");
    Stdio.File client = clients[0];
    if (client->query_address(1) != accepted->query_address())
      client = clients[1];
    check_blocking(accepted, client);
    check_blocking(client, accepted);
    accepted->close();
    client->close();
  }
  write("Concurrent accepts, callback mode changes and blocking I/O passed.\n");

  Thread.Thread closed = Thread.Thread(lambda() {
    object result = port->accept();
    return ({!!result, port->errno()});
  });
  sleep(0.05);
  port->close();
  // Rebinding can reuse the descriptor; the old wait must still abort.
  require(port->bind(0, 0, "127.0.0.1"), "rebind failed");
  array result = closed->wait();
  require(!result[0] && result[1] == System.EBADF,
          sprintf("accept survived close/rebind: %O", result));
  write("Close and rebind during accept passed.\n");

  Thread.Thread interrupted = Thread.Thread(lambda() {
    return !!catch { port->accept(); };
  });
  sleep(0.05);
  interrupted->interrupt();
  require(interrupted->wait(), "accept interruption was ignored");
  port->set_accept_callback(lambda(mixed id) {});
  require(!port->accept(), "nonblocking accept after interruption failed");
  port->set_accept_callback(0);
  Thread.Thread accepted = Thread.Thread(lambda() { return port->accept(); });
  Stdio.File client = Stdio.File();
  require(client->connect("127.0.0.1", number(port)), "final connect failed");
  Stdio.File peer = accepted->wait();
  require(!!peer, "accept after interruption failed");
  check_blocking(peer, client);
  peer->close();
  client->close();
  int closed_number = number(port);
  port->close();
  require(!client->connect("127.0.0.1", closed_number) &&
          client->errno() == System.ECONNREFUSED,
          "blocking connect did not report ECONNREFUSED");
  write("Interrupted accept cleanup and refused blocking connect passed.\n");
}

int main()
{
  mixed err = catch { run_tests(); };
  if (err) werror("%s", describe_backtrace(err));
  Tools.Testsuite.report_result(!err, !!err);
  return !!err;
}
