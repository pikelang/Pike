#charset utf-8

//
// Check http connectivity to a host.
//
// Return codes are compatible with Nagios 3.
//
// 2015-11-02 Henrik Grubbström
//

constant options = ({
  ({ "help", Getopt.NO_ARG, ({ "-h", "--help" }) }),
  ({ "version", Getopt.NO_ARG, ({ "-V", "--version" }) }),
  ({ "min_ssl", Getopt.HAS_ARG, ({ "-S", "--ssl" }) }),
  ({ "method", Getopt.HAS_ARG, ({ "-j", "--method" }) }),
  ({ "post_data", Getopt.HAS_ARG, ({ "-P", "--post" }) }),
  ({ "timeout", Getopt.HAS_ARG, ({ "-t", "--timeout" }) }),
});

enum RetCode {
  RET_OK	= 0,
  RET_WARNING	= 1,
  RET_CRITICAL	= 2,
  RET_UNKNOWN	= 3,
}

int start_time;

void display_version()
{
  Stdio.stdout.write("Check HTTP/Pike v%d.%d.%d\n",
		     __REAL_MAJOR__, __REAL_MINOR__, __BUILD__);
}

void display_usage()
{
  display_version();
  Stdio.stdout.write("\nUsage:\n");
  string prefix = "  pike -x check_http";
  string doc = "";
  foreach(options, array(string|int|array(string)) opt) {
    doc += " [\xa0" + (opt[2]*"\xa0|\xa0");
    if (opt[1] == Getopt.HAS_ARG) {
      doc += " <" + opt[0] + ">";
    }
    doc += "\xa0]";
  }
  doc += " <url>";
  doc = sprintf("%#*s%-=*s\n",
		sizeof(prefix), prefix,
		70 - sizeof(prefix), doc)[1..];
  Stdio.stdout.write(replace(doc, "\xa0", " "));
}

string fmt_runtime()
{
  int delta = gethrtime() - start_time;
  return sprintf("runtime=%d.%06d", delta/1000000, delta%1000000);
}

void do_timeout(int timeout)
{
  Stdio.stdout.write("CRITICAL: Timeout. | timeout=%d;%s\n",
		     timeout, fmt_runtime());
  exit(RET_CRITICAL);
}

void request_fail(Protocols.HTTP.Query q)
{
  Stdio.stdout.write("CRITICAL: Connection failed. | errno=%d;%s\n",
		     q->errno, fmt_runtime());
  exit(RET_CRITICAL);
}

void request_ok(Protocols.HTTP.Query q)
{
  string data = sprintf("code=%d;%s", q->status, fmt_runtime());

  SSL.Session session = q->ssl_session;
  if (session) {
#if 0
    werror("SSL Session: %O\n",
	   mkmapping(indices(session), values(session)));
#endif
    data += sprintf(";ssl=%s;suite=%s",
		    SSL.Constants.fmt_version(session->version),
		    SSL.Constants.fmt_cipher_suite(session->cipher_suite));
  }

  if (q->status > 399) {
    Stdio.stdout.write("FAIL: Bad status code: %s(%d). | %s\n",
		       q->status_desc, q->status, data);
    exit(RET_CRITICAL);
  }
  if (q->status > 299) {
    Stdio.stdout.write("WARNING: Bad status code: %s(%d). | %s\n",
		       q->status_desc, q->status, data);
    exit(RET_WARNING);
  }
  Stdio.stdout.write("OK: Success. | %s\n", data);
  exit(RET_OK);
}

int main(int argc, array(string) argv)
{
  Protocols.HTTP.Query q = Protocols.HTTP.Query();
  q->context = SSL.Context();
  q->set_callbacks(request_ok, request_fail);
  Standards.URI url = Standards.URI("http://localhost/");
  string method = "GET";
  int timeout = 10;
  string post_data = UNDEFINED;

  foreach(Getopt.find_all_options(argv, options, 1), array(string) opt) {
    switch(opt[0]) {
    case "help":
      display_usage();
      exit(0);
    case "version":
      display_version();
      exit(0);
    case "min_ssl":
      url->scheme = "https";
      switch(lower_case(opt[1])) {
      case "ssl": case "3": case "3.0":
	q->context->min_version = SSL.Constants.PROTOCOL_SSL_3_0;
	break;
      default:
	werror("Warning: Unknown version of SSL/TLS: %O\n"
	       "Falling back to TLS 1.0.\n", opt[1]);
	// FALL_THROUGH
      case "tls": case "1": case "1.0": case "3.1":
	q->context->min_version = SSL.Constants.PROTOCOL_TLS_1_0;
	break;
      case "1.1":
	q->context->min_version = SSL.Constants.PROTOCOL_TLS_1_1;
	break;
      case "1.2":
	q->context->min_version = SSL.Constants.PROTOCOL_TLS_1_2;
	break;
      }
      break;
    case "method":
      method = upper_case(opt[1]);
      break;
    case "post_data":
      post_data = opt[1];
      break;
    case "timeout":
      timeout = (int)opt[1];
      if (timeout <= 0) {
	werror("Invalid timeout.\n");
	exit(RET_UNKNOWN);
      }
      break;
    }
  }
  argv = Getopt.get_args(argv, 1);
  if (sizeof(argv) != 2) {
    Stdio.stdout.write("Invalid arguments.\n\n");
    display_usage();
    exit(RET_UNKNOWN);
  }
  url = Standards.URI(argv[1], url);

  if (url->scheme == "https") {
    // Delay loading of the certificates until we
    // know that we will be using ssl/tls.
    q->context->trusted_issuers_cache = Standards.X509.load_authorities();
  }

  start_time = gethrtime();

  Protocols.HTTP.do_async_method(method, url, UNDEFINED, UNDEFINED,
				 q, post_data);
  call_out(do_timeout, timeout, timeout);
  return -1;
}
