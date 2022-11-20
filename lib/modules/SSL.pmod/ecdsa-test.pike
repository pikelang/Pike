
#define ASSERT(X) if(!(X)) error("Assertion failed.\n")
#define log_msg	werror
#define log_status	werror

import SSL.Constants;

class TestContext {
  inherit SSL.Context;

  int(0..1) expect_fail;
  SSL.Alert alert_factory(SSL.Connection con,
			  int level, int description,
			  SSL.Constants.ProtocolVersion version,
			  string|void message)
  {
    if (message && !expect_fail &&
	(description != SSL.Constants.ALERT_close_notify)) {
      log_msg("ALERT [%s: %d: %s]: %s",
	      SSL.Constants.fmt_version(version), level,
	      SSL.Constants.fmt_constant(description, "ALERT"), message);
    }
    return ::alert_factory(con, level, description, version, message);
  }
};

SSL.Context server_ctx = TestContext();
string client_msg = random_string(655/*19*/);

void log_ssl_failure(int cmin, int cmax, int smin, int smax,
		string failure, array(int) suites)
{
  log_msg("%s..%s client with %s..%s server failed.\n",
	  SSL.Constants.fmt_version(cmin),
	  SSL.Constants.fmt_version(cmax),
	  SSL.Constants.fmt_version(smin),
	  SSL.Constants.fmt_version(smax));
  log_msg("Cipher_suite: %s",
	  SSL.Constants.fmt_cipher_suites(suites));
  log_msg("%s", failure);
}

string low_test_ssl_connection(SSL.Context server_ctx, SSL.Context client_ctx,
			       string exp_data, int exp_suite, int exp_version,
			       SSL.Session|void session)
{
  Stdio.File client_con = Stdio.File();
  Stdio.File server_con =
    client_con->pipe(Stdio.PROP_NONBLOCK | Stdio.PROP_BIDIRECTIONAL);

  SSL.File server = SSL.File(server_con, server_ctx);

  SSL.File client = SSL.File(client_con, client_ctx);

  function fun = client->connect;

  if (!fun(UNDEFINED, session)) return 0;
  if (!server->accept()) return 0;

  int state;

  int trigged = 10;
  int remaining = sizeof(client_msg);

  string fail;

  string server_buf = "";
  void server_send_data() {
    trigged = 2;
    int bytes = server->write(server_buf);
    server_buf = server_buf[bytes..];
    remaining -= bytes;
    if (!sizeof(server_buf)) {
      server->set_write_callback(UNDEFINED);
      if (!remaining) {
	server->close();
	state = 2;
      }
    }
  };
  void server_got_data(mixed ignored, string data) {
    trigged = 2;
    if (!sizeof(server_buf)) server->set_write_callback(server_send_data);
    server_buf += data;
  };
  void server_got_close() {
    trigged = 2;
  };
  void server_alert() {
    trigged = 0;
    // server->close();
  };

  void server_check_suite() {
    if ((server->query_version() != exp_version) ||
	(server->query_suite() != exp_suite)) {
      fail =
	sprintf("Unexpected cipher suite (server):\n"
		"Got: %s [%s]\n"
		"Exp: %s [%s]\n",
		fmt_cipher_suite(server->query_suite()),
		fmt_version(server->query_version()),
		fmt_cipher_suite(exp_suite),
		fmt_version(exp_version));
      return;
    }
    server->set_write_callback(sizeof(server_buf) && server_send_data);
  };

  string client_recv_buf = "";
  string client_send_buf = client_msg;
  void client_got_data(mixed ignored, string data) {
    trigged = 2;
    client_recv_buf += data;
  };
  void client_send_data() {
    trigged = 2;
    int bytes = client->write(client_send_buf[..4095]);
    client_send_buf = client_send_buf[bytes..];
    if (!sizeof(client_send_buf)) {
      client->set_write_callback(UNDEFINED);
      state = 1;
    }
  };
  void client_got_close() {
    trigged = 2;
    if (state == 2) state = 3;
    client->close();
  };
  void client_alert() {
    trigged = 0;
    // client->close();
  };

  void client_check_suite() {
    if ((client->query_version() != exp_version) ||
	(client->query_suite() != exp_suite)) {
      fail =
	sprintf("Unexpected cipher suite (client):\n"
		"Got: %s [%s]\n"
		"Exp: %s [%s]\n",
		fmt_cipher_suite(client->query_suite()),
		fmt_version(client->query_version()),
		fmt_cipher_suite(exp_suite),
		fmt_version(exp_version));
      return;
    }
    client->set_write_callback(client_send_data);
    client_send_data();
  };

  server->set_nonblocking(server_got_data, server_check_suite,
			  server_got_close);
  server->set_alert_callback(server_alert);
  client->set_nonblocking(client_got_data, client_check_suite,
			  client_got_close);
  client->set_alert_callback(client_alert);

  // We loop the backend while something happens...
  while (!fail && trigged--) {
    mixed err = catch {
        Pike.DefaultBackend(0.005);
      };
    if (err) {
      state = -1;
      master()->handle_error(err);
      break;
    }
  }

  if (fail) {
  } else if (!exp_data ||
	     ((state == 3) && (client_recv_buf == exp_data))) {
    // if (exp_data) log_status("OK: %s\n", fmt_cipher_suite(exp_suite));
  } else {
    if (state != 3) {
      fail = sprintf("Unexpected exit state: %d.\n", state);
    } else {
      fail =
	sprintf("Unexpected result:\n"
		"Got: %O\n"
		"Exp: %O\n",
		client_recv_buf[..32], exp_data[..32]);
    }
  }
  client->close();
  server->close();
  destruct(client);
  destruct(server);
  return fail;
}

int test_ssl_connection(int client_min, int client_max,
			int server_min, int server_max,
			string exp_data, object server_ctx, array(int) suites,
			array(CertificatePair)|void client_certs,
			array(int)|void ffdhe_groups)
{
  server_ctx->min_version = server_min;
  server_ctx->max_version = server_max;

  int exp_version = server_max;
  if (exp_version > client_max) {
    exp_version = client_max;
  }

  if ((exp_version < server_min) ||
      (exp_version < client_min)) {
    exp_data = 0;
  }

  int exp_suite = -1;

  foreach(suites, int suite) {
    if (exp_version < PROTOCOL_TLS_1_2) {
      int ke = CIPHER_SUITES[suite ][0];
      if ((ke == KE_ecdh_rsa) || (ke == KE_dh_rsa)) {
	// We only have self-signed certificates, so all ECDH_RSA and
	// DH_RSA suites will fail prior to TLS 1.2, since they require
	// the certificate to be signed with RSA.
	continue;
      }
      if (sizeof(CIPHER_SUITES[suite ]) == 4) {
	// AEAD ciphers and cipher specific prfs not supported prior
	// to TLS 1.2.
	continue;
      }
      if (client_max < PROTOCOL_TLS_1_0) {
	// SSL 3.0 doesn't support extensions, so it can't support ECC.
	if ((ke == KE_ecdh_ecdsa) || (ke == KE_ecdhe_ecdsa) ||
	    (ke == KE_ecdhe_rsa) || (ke == KE_ecdh_anon)) {
	  continue;
	}
      }
    }
    exp_suite = suite;
    break;
  }

  if (exp_suite == -1) exp_data = 0;

  // A client that supports just a single cipher suite.
  SSL.Context client_ctx = TestContext();
  client_ctx->random = random_string;
  client_ctx->preferred_suites = suites;
  client_ctx->min_version = client_min;
  client_ctx->max_version = client_max;

  if (ffdhe_groups) client_ctx->ffdhe_groups = ffdhe_groups;

  server_ctx->expect_fail = client_ctx->expect_fail = !exp_data;

  if (client_certs) {
    // Add the certificates.
    foreach(client_certs, CertificatePair cp) {
      client_ctx->add_cert(cp);
    }
  }

  SSL.Session session = SSL.Session();

  string fail =
    low_test_ssl_connection(server_ctx, client_ctx,
			    exp_data, exp_suite, exp_version,
			    session);

  if (fail) {
    log_ssl_failure(client_min, client_max, server_min, server_max,
		    fail, suites);

    return 0;
  }

  string sessionid = session->identity;
  if (!sessionid) return 1;

  if (!`==(client_min, client_max, server_min, server_max)) return 1;

  // Try resuming the session.

  fail =
    low_test_ssl_connection(server_ctx, client_ctx,
			    exp_data, suites[0], exp_version,
			    session);

  if (!fail && (sessionid != session->identity)) {
#if __MINOR__
    fail = "New session.";
#endif
  }

  if (fail) {
    log_ssl_failure(client_min, client_max, server_min, server_max,
		    "RESUME: " + fail, suites);

    return 0;
  }
  return 2;
}

array(string) threaded_low_test_ssl_connection(SSL.Context server_ctx,
					       SSL.Context client_ctx,
					       string exp_data,
					       int exp_suite,
					       ProtocolVersion exp_version)
{
#if constant(thread_create)
  Stdio.File client_con = Stdio.File();
  Stdio.File server_con =
    client_con->pipe(Stdio.PROP_NONBLOCK | Stdio.PROP_BIDIRECTIONAL);
  SSL.File server;
  SSL.File client;

  if (!server_con) {
    return ({ sprintf("Failed to create pipe: %s\n",
		      strerror(client_con->errno())) });
  }

  Thread.Thread server_thread =
    Thread.Thread(lambda(int bytes) {
		    server = SSL.File(server_con, server_ctx);
		    server->set_blocking();
		    if (!server->accept()) {
		      return exp_data && "Server accept failed.\n";
		    }

		    if (exp_data &&
			((server->query_version() != exp_version) ||
			 (server->query_suite() != exp_suite))) {
		      return
			sprintf("Unexpected cipher suite (server):\n"
				"Got: %s [%s]\n"
				"Exp: %s [%s]\n",
				fmt_cipher_suite(server->query_suite()),
				fmt_version(server->query_version()),
				fmt_cipher_suite(exp_suite),
				fmt_version(exp_version));
		    }
		    while (bytes) {
		      // NB: Client doesn't close connection before all data has
		      //     been returned, so we can't just use read(1024) here.
		      if (!server->is_open()) {
			return exp_data && "Server connection was closed.\n";
		      }
		      string data = server->read(min(1024, bytes));
		      if (!data) {
			return exp_data && "Server failed to receive all data.\n";
		      }
		      bytes -= sizeof(data);
		      int sent_bytes = server->write(data);
		    }
		    server->close();
		    return 0;
		  }, sizeof(client_msg));

  /* SSL.File */ client = SSL.File(client_con, client_ctx);

  client->set_blocking();
  if (!client->connect()) {
    server_thread->wait();
    return exp_data ? ({ "Client failed to connect.\n" }) : ({});
  }
  if (exp_data &&
      ((client->query_version() != exp_version) ||
       (client->query_suite() != exp_suite))) {
    return ({
      sprintf("Unexpected cipher suite (client):\n"
	      "Got: %s [%s]\n"
	      "Exp: %s [%s]\n",
	      fmt_cipher_suite(client->query_suite()),
	      fmt_version(client->query_version()),
	      fmt_cipher_suite(exp_suite),
	      fmt_version(exp_version)),
    });
  }
  Thread.Thread sender_thread =
    Thread.Thread(lambda() {
		    if (!client->is_open()) {
		      return exp_data && "Client connection was closed.\n";
		    }
		    int bytes = client->write(client_msg);
		    if (bytes == sizeof(client_msg)) return 0;
		    return exp_data && "Client failed to write all data.\n";
		  });

  Thread.Thread reader_thread =
    Thread.Thread(lambda() {
		    if (!client->is_open()) {
		      return exp_data && "Client connection was closed.\n";
		    }
		    string recv = client->read(sizeof(client_msg));
		    client->close();
		    if (recv == client_msg) {
		      if (exp_data) return 0;
		      return "Succeeded where failure expected.\n";
		    }
		    return exp_data && "Failed to read all data.\n";
		  });

  array(string) failures = ({
    sender_thread, server_thread, reader_thread,
  })->wait() - ({ 0 });

#ifdef SSL3_DEBUG
  if (sizeof(failures)) {
    werror("Failures: %O\n", failures);
  }
#endif
  client->close();
  server->close();
  destruct(client);
  destruct(server);
  return failures;
#else /* !constant(thread_create) */
  return ({});
#endif /* constant(thread_create) */
}

int threaded_test_ssl_connection(int client_min, int client_max,
				 int server_min, int server_max,
				 string exp_data, object server_ctx,
				 array(int) suites,
				 array(CertificatePair)|void client_certs,
				 array(int)|void ffdhe_groups)
{
  server_ctx->min_version = server_min;
  server_ctx->max_version = server_max;

  int exp_version = server_max;
  if (exp_version > client_max) {
    exp_version = client_max;
  }

  if ((exp_version < server_min) ||
      (exp_version < client_min)) {
    exp_data = 0;
  }

  // We only have self-signed certificates, so all ECDH_RSA and
  // DH_RSA suites will fail prior to TLS 1.2, since they require
  // the certificate to be signed with RSA.
  if ((exp_version < PROTOCOL_TLS_1_2) && (sizeof(suites) == 1)) {
    int ke = CIPHER_SUITES[suites[0] ][0];
    if ((ke == KE_ecdh_rsa) || (ke == KE_dh_rsa)) {
      exp_data = 0;
    }
    if (sizeof(CIPHER_SUITES[suites[0] ]) == 4) {
      // AEAD ciphers and cipher specific prfs not supported prior
      // to TLS 1.2.
      exp_data = 0;
    }
    if (client_max < PROTOCOL_TLS_1_0) {
      // SSL 3.0 doesn't support extensions, so it can't support ECC.
      if ((ke == KE_ecdh_ecdsa) || (ke == KE_ecdhe_ecdsa) ||
	  (ke == KE_ecdhe_rsa) || (ke == KE_ecdh_anon)) {
	exp_data = 0;
      }
    }
  }

  // A client that supports just a single cipher suite.
  SSL.Context client_ctx = TestContext();
  client_ctx->random = random_string;
  client_ctx->preferred_suites = suites;
  client_ctx->min_version = client_min;
  client_ctx->max_version = client_max;

  if (ffdhe_groups) client_ctx->ffdhe_groups = ffdhe_groups;

  server_ctx->expect_fail = client_ctx->expect_fail = !exp_data;

  if (client_certs) {
    // Add the certificates.
    foreach(client_certs, CertificatePair cp) {
      client_ctx->add_cert(cp);
    }
  }

  array(string) failures =
    threaded_low_test_ssl_connection(server_ctx, client_ctx,
				     exp_data, suites[0], exp_version);

  if (!sizeof(failures)) return 1;

  log_ssl_failure(client_min, client_max, server_min, server_max,
		  failures * "\n", suites);
  return 0;
}

int main(int argc, array(string) argv)
{
  // Make sure that all cipher suites are available server side.
  server_ctx->preferred_suites = server_ctx->get_suites(-1, 2, (<>));

  object ecdsa = Crypto.ECC.SECP_192R1.ECDSA()->
    set_random(random_string)->generate_key();
  mapping attrs = ([
    "organizationName" : "Test",
    "commonName" : "*",
  ]);
  string cert = Standards.X509.make_selfsigned_certificate(ecdsa,
    3600*24, attrs);

  server_ctx->add_cert(ecdsa, ({ cert }));

  cert = Standards.X509.make_selfsigned_certificate(ecdsa,
    3600*24, attrs, UNDEFINED, Crypto.SHA1);

  server_ctx->add_cert(ecdsa, ({ cert }));

  array(int) suites = ({
    SSL.Constants.TLS_ecdhe_ecdsa_with_3des_ede_cbc_sha,
  });

  int client_min = PROTOCOL_SSL_3_0;
  int client_max = PROTOCOL_TLS_1_0;
  int server_min = PROTOCOL_SSL_3_0;
  int server_max = PROTOCOL_SSL_3_0;

  string exp_res = client_msg;

  log_status("Testing %s..%s client with %s..%s server...\n",
	     SSL.Constants.fmt_version(client_min),
	     SSL.Constants.fmt_version(client_max),
	     SSL.Constants.fmt_version(server_min),
	     SSL.Constants.fmt_version(server_max));

  int successes;
  int tests;
  int skip;

  foreach(suites, int suite) {
    int count = test_ssl_connection(client_min, client_max,
				    server_min, server_max, exp_res,
				    server_ctx, ({suite}) );
    successes += count;
    tests += count || 1;
    if (!count) skip += !!exp_res;	// Skipped resume.
  }
  werror("Successes: %d/%d\n", successes, tests);
  if (successes != tests) werror("\n\nBAD\n");
  return successes != tests;
}

