#charset utf-8

constant description = "Check HTTP/HTTPS connectivity to a host.";

//
// Return codes are compatible with Nagios 3.
//
// 2015-11-02 Henrik Grubbström
//

constant options = ({
  ({ "help", Getopt.NO_ARG, ({ "-h", "--help" }) }),
  ({ "version", Getopt.NO_ARG, ({ "-V", "--version" }) }),
  ({ "min_ssl", Getopt.HAS_ARG, ({ "-S", "--ssl" }) }),
  ({ "min_ttl", Getopt.HAS_ARG, ({ "-C", "--certificate" }) }),
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

int cert_min_ttl = -1;	// Disabled by default.

void exit(RetCode err, string msg, mixed ... args)
{
  string prefix = ([ RET_WARNING : "WARNING: ",
                     RET_CRITICAL : "CRITICAL: " ])[err] || "";
  msg = prefix+msg;
  Stdio.stdout.write(msg, @args);
  predef::exit(err);
}

string version()
{
  return sprintf("Check HTTP/Pike v%d.%d.%d\n",
                 __REAL_MAJOR__, __REAL_MINOR__, __BUILD__);
}

string usage()
{
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
  doc = version() + "\nUsage:\n" +
    sprintf("%#*s%-=*s\n",
            sizeof(prefix), prefix,
            70 - sizeof(prefix), doc)[1..];
  return replace(doc, "\xa0", " ");
}

string fmt_runtime()
{
  int delta = gethrtime() - start_time;
  return sprintf("runtime=%d.%06d", delta/1000000, delta%1000000);
}

void do_timeout(int timeout)
{
  exit(RET_CRITICAL, "Timeout. | timeout=%d;%s\n", timeout, fmt_runtime());
}

void request_fail(Protocols.HTTP.Query q)
{
  exit(RET_CRITICAL, "Connection failed. | errno=%d;%s\n",
       q->errno, fmt_runtime());
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
		    SSL.Constants.fmt_version(session->version) - " ",
		    SSL.Constants.fmt_cipher_suite(session->cipher_suite));
    array(Standards.X509.TBSCertificate) certs = session->cert_data &&
      session->cert_data->certificates;

    if (sizeof(certs || ({}))) {
      Standards.X509.TBSCertificate cert = certs[-1];
      data += sprintf(";cn=%s;serial=%d;not_before=%d;not_after=%d",
		      cert->subject_str(), cert->serial,
                      cert->not_before, cert->not_after);

      int err = session->cert_data->error_code;
      if( err & Standards.X509.CERT_TOO_NEW )
        exit(RET_CRITICAL, "Certificate not valid yet. | %s\n", data);
      if( err & Standards.X509.CERT_TOO_OLD )
        exit(RET_CRITICAL, "Certificate expired. | %s\n", data);
      if( err & Standards.X509.CERT_ROOT_UNTRUSTED )
        exit(RET_CRITICAL, "Certificate chain root untrusted | %s\n", data);
      if( err & Standards.X509.CERT_BAD_SIGNATURE )
        exit(RET_CRITICAL, "Certificate signature not valid | %s\n", data);
      if( err & Standards.X509.CERT_INVALID )
        exit(RET_CRITICAL, "Certificate structurally wrong | %s\n", data);
      if( err & Standards.X509.CERT_CHAIN_BROKEN )
        exit(RET_CRITICAL, "Certificate chain broken | %s\n", data);
      if( err & Standards.X509.CERT_UNAUTHORIZED_CA )
        exit(RET_CRITICAL, "Invalid signer in certificate chain | %s\n", data);
      if( err & Standards.X509.CERT_EXCEEDED_PATH_LENGTH )
        exit(RET_CRITICAL, "Certificate chain too long | %s\n", data);
      if( err & ~(Standards.X509.CERT_TOO_NEW &
                  Standards.X509.CERT_TOO_OLD &
                  Standards.X509.CERT_ROOT_UNTRUSTED &
                  Standards.X509.CERT_BAD_SIGNATURE &
                  Standards.X509.CERT_INVALID &
                  Standards.X509.CERT_CHAIN_BROKEN &
                  Standards.X509.CERT_UNAUTHORIZED_CA &
                  Standards.X509.CERT_EXCEEDED_PATH_LENGTH) )
        exit(RET_UNKNOWN, "Unknown certificate error | %s\n", data);

      if (cert_min_ttl >= 0) {
        int now = time();
        foreach(certs, cert)
          if (cert->not_after < now + cert_min_ttl)
            exit(RET_WARNING, "Certificate expires soon. | %s\n", data);
      }
    }
  }

  if (q->status > 399)
    exit(RET_CRITICAL, "Bad status code: %s(%d). | %s\n",
         q->status_desc, q->status, data);

  if (q->status > 299)
    exit(RET_WARNING, "Bad status code: %s(%d). | %s\n",
         q->status_desc, q->status, data);

  exit(RET_OK, "OK: Success. | %s\n", data);
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
      exit(RET_OK, usage());
    case "version":
      exit(RET_OK, version());
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
    case "min_ttl":
      // Convert from days to seconds.
      cert_min_ttl = ((int)opt[1]) * 24 * 3600;
      break;
    case "method":
      method = upper_case(opt[1]);
      break;
    case "post_data":
      post_data = opt[1];
      break;
    case "timeout":
      timeout = (int)opt[1];
      if (timeout <= 0)
        exit(RET_UNKNOWN, "Invalid timeout.\n");
      break;
    }
  }
  argv = Getopt.get_args(argv, 1);
  if (sizeof(argv) != 2)
    exit(RET_UNKNOWN, "Invalid arguments.\n\n"+usage());

  url = Standards.URI(argv[1], url);

  if (url->scheme == "https") {
    // Delay loading of the certificates until we
    // know that we will be using ssl/tls.
    q->context->trusted_issuers_cache = Standards.X509.load_authorities();
    q->context->auth_level = SSL.Constants.AUTHLEVEL_require;
  }

  start_time = gethrtime();

  Protocols.HTTP.do_async_method(method, url, UNDEFINED, UNDEFINED,
				 q, post_data);
  call_out(do_timeout, timeout, timeout);
  return -1;
}
