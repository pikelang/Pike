// This just runs through a bunch of GSSAPI calls and prints out the
// responses. No checking is done whether they are actually correct or
// not. It's difficult to write a proper testsuite since it requires a
// working GSS environment like Kerberos with the proper principals
// etc, and the results also depend a lot on the capabilities of the
// implementation.

constant mech_krb5 = "1.2.840.113554.1.2.2";
constant mech_spnego = "1.3.6.1.5.5.2";

#define LOW_TEST(DESCR, CODE...) do {					\
    if (mixed err = catch {CODE;})					\
      werror ("%s: Error: %s", DESCR, describe_error (err));		\
  } while (0)

#define TEST(EXPR, ARGS...)						\
  LOW_TEST (sprintf (#EXPR, ARGS),					\
	    werror ("%s: %O\n", sprintf (#EXPR, ARGS), (EXPR)))

#define QTEST(EXPR, ARGS...)						\
  LOW_TEST (sprintf (#EXPR, ARGS),					\
	    werror ("%s: %t\n", sprintf (#EXPR, ARGS), (EXPR)))

#define TEST_CODE(CODE...) LOW_TEST ((string) __LINE__, CODE)

int main()
{
  TEST (GSSAPI.indicate_mechs());
  TEST_CODE ({
    foreach (GSSAPI.indicate_mechs(); string mech;)
      TEST (GSSAPI.names_for_mech (mech /*%s*/), mech);
  });

  werror ("\n");

  TEST (GSSAPI.Error (GSSAPI.BAD_MECH));
  TEST (GSSAPI.Error (GSSAPI.BAD_MECH)->message());
  TEST (GSSAPI.Error (GSSAPI.NO_CONTEXT|GSSAPI.CONTINUE_NEEDED,
		      4711, mech_krb5)->message());
  TEST (GSSAPI.Error (GSSAPI.NO_CONTEXT|GSSAPI.CONTINUE_NEEDED|GSSAPI.OLD_TOKEN,
		      4711, mech_krb5)->major_status);
  TEST (GSSAPI.Error (GSSAPI.NO_CONTEXT|GSSAPI.CONTINUE_NEEDED|GSSAPI.OLD_TOKEN,
		      4711, mech_krb5)->major_status_messages());
  TEST (GSSAPI.Error (GSSAPI.NO_CONTEXT|GSSAPI.CONTINUE_NEEDED|GSSAPI.OLD_TOKEN,
		      4711, mech_krb5)->minor_status);
  TEST (GSSAPI.Error (GSSAPI.NO_CONTEXT|GSSAPI.CONTINUE_NEEDED|GSSAPI.OLD_TOKEN,
		      4711, mech_krb5)->minor_status_mech());
  TEST (GSSAPI.Error (GSSAPI.NO_CONTEXT|GSSAPI.CONTINUE_NEEDED|GSSAPI.OLD_TOKEN,
		      4711, mech_krb5)->minor_status_messages());
  TEST (GSSAPI.Error (0, 17, "1.2.3.4.5")->minor_status_messages());
  TEST (GSSAPI.Error (0, 17, mech_krb5)->minor_status_messages());
  TEST (GSSAPI.Error (0, 17)->minor_status_messages());

  werror ("\n");

  TEST (GSSAPI.Name ("person1"));
  TEST (GSSAPI.Name ("person1", "1.4.3.55.43.2"));
  TEST (GSSAPI.Name ("person1", "1.4.3.55.43.2")->canonicalize (mech_krb5));
  TEST (GSSAPI.Name ("person1", GSSAPI.NT_USER_NAME)->canonicalize (mech_krb5));
  TEST (GSSAPI.Name ("http@localhost", GSSAPI.NT_HOSTBASED_SERVICE)->canonicalize (mech_krb5));
  TEST (GSSAPI.Name ("person1")->canonicalize (mech_krb5));
  TEST (GSSAPI.Name ("person1")->canonicalize (mech_spnego));
  TEST (GSSAPI.Name ("person1")->export (mech_krb5));
  TEST (GSSAPI.Name ("person1")->export (mech_spnego));
  TEST (GSSAPI.Name ("person1")->mechs());
  TEST (GSSAPI.Name ("person1")->canonicalize (mech_krb5)->mechs());

  werror ("\n");

  TEST (GSSAPI.Name ("HTTP@lister.roxen.com")->canonicalize (mech_krb5));
  TEST (GSSAPI.Name ("HTTP/lister.roxen.com")->canonicalize (mech_krb5));
  TEST (GSSAPI.Name ("HTTP@lister.roxen.com",
		     GSSAPI.NT_HOSTBASED_SERVICE)->canonicalize (mech_krb5));
  TEST (GSSAPI.Name ("HTTP/lister.roxen.com") ==
	GSSAPI.Name ("HTTP@lister.roxen.com", GSSAPI.NT_HOSTBASED_SERVICE));
  TEST (GSSAPI.Name ("HTTP/lister.roxen.com")->canonicalize (mech_krb5) ==
	GSSAPI.Name ("HTTP@lister.roxen.com",
		     GSSAPI.NT_HOSTBASED_SERVICE)->canonicalize (mech_krb5));

  werror ("\n");

  GSSAPI.Cred c = GSSAPI.Cred();
  TEST (c);
  TEST (c->acquire (GSSAPI.Name ("root"), GSSAPI.INITIATE));
  TEST (c);
  c = GSSAPI.Cred();
  TEST (c->acquire (0, GSSAPI.BOTH, (<mech_spnego>)));
  TEST (c);
  TEST (c->mechs());
  c = GSSAPI.Cred();
  TEST (c->acquire (0, GSSAPI.INITIATE));
  TEST (c);
  TEST (c->mechs());
  TEST (c->release());
  TEST (c->mechs());

  werror ("\n");

  // The SPNEGO mech in krb5 behaves funky..
  TEST (c->acquire ("person1", GSSAPI.INITIATE, (<mech_spnego>)));
  TEST (c);
  TEST (c->name());
  TEST (c->name (mech_spnego));
  TEST (c->cred_usage());
  TEST (c->cred_usage (mech_spnego));
  TEST (c->mechs());

  werror ("\n");

  TEST (c = GSSAPI.Cred());
  TEST (c->mechs());
  TEST (c->add (GSSAPI.Name ("person2"), GSSAPI.ACCEPT, mech_krb5));
  TEST (c);

  werror ("\n");

  TEST (c = GSSAPI.Cred());
  TEST (c->acquire ("person1", GSSAPI.INITIATE));
  TEST (c->name());
  TEST (c->name (mech_krb5));
  TEST (c->cred_usage());
  TEST (c->cred_usage (mech_krb5));
  TEST (c->lifetime());
  TEST (c->init_lifetime (mech_krb5));
  TEST (c->accept_lifetime (mech_krb5));
  TEST (c->release());
  TEST (c->name());
  TEST (c->release());

  werror ("\n");

  TEST (c = GSSAPI.Cred());
  TEST (c->add (0, GSSAPI.ACCEPT, mech_krb5, 10));
  TEST (c->cred_usage (mech_krb5));
  TEST (c->lifetime());
  TEST (c->init_lifetime (mech_krb5));
  TEST (c->accept_lifetime (mech_krb5));

  werror ("\n");

  TEST (c = GSSAPI.Cred());
  TEST (c->add (0, GSSAPI.BOTH, mech_krb5, ({10, -50})));
  TEST (c->add (0, GSSAPI.BOTH, mech_krb5, ({10})));
  TEST (c->add (0, GSSAPI.BOTH, mech_krb5, ({10, 50})));
  TEST (c->cred_usage (mech_krb5));
  TEST (c->lifetime());
  TEST (c->init_lifetime (mech_krb5));
  TEST (c->accept_lifetime (mech_krb5));

  werror ("\n");

  // This doesn't work in pike <= 7.6 since putenv there just updates
  // some silly-dilly mapping in the master that makes no difference
  // whatsoever. You have to set this in the environment running the
  // script.
  putenv ("KRB5_KTNAME", "/home/mast/certs/http-lister.keytab");

  GSSAPI.Name n;
  TEST (n = GSSAPI.Name ("HTTP@lister.roxen.com", GSSAPI.NT_HOSTBASED_SERVICE));
  TEST (c = GSSAPI.Cred());
  TEST (c->acquire (n, GSSAPI.ACCEPT));
  TEST (c);
  TEST (c->mechs());
  TEST (c->cred_usage());
  TEST (c->lifetime());

  werror ("\n");

  GSSAPI.InitContext i;
  string itok;
  TEST (i = GSSAPI.InitContext (c, "HTTP/lister.roxen.com", mech_krb5,
				0xfff, 0xfff, 4711));
  QTEST (itok = i->init());
  TEST (i);

  werror ("\n");

  TEST (i = GSSAPI.InitContext (0, "HTTP/lister.roxen.com", mech_spnego,
				0, 0xfff));
  TEST (i->is_established());
  QTEST (itok = i->init());
  // gss_inquire_context in libgssapi_krb5 dumps core if we use it on
  // a partly established SPNEGO context.
//   TEST (i);
//   TEST (GSSAPI.describe_services (i->required_services()));
//   TEST (i->is_established());
//   TEST (GSSAPI.describe_services (i->services()));
//   TEST (i->locally_initiated());
//   TEST (i->source_name());
//   TEST (i->target_name());
//   TEST (i->lifetime());
//   TEST (i->mech());

  werror ("\n");

  GSSAPI.AcceptContext a;
  string atok;
  TEST (a = GSSAPI.AcceptContext (c));
  TEST (a->is_established());
  QTEST (atok = a->accept (itok));
  TEST (a);
  TEST (GSSAPI.describe_services (a->required_services()));
  TEST (a->is_established());
  TEST (GSSAPI.describe_services (a->services()));
  TEST (a->locally_initiated());
  TEST (a->source_name());
  TEST (a->target_name());
  TEST (a->lifetime());
  TEST (a->mech());
  TEST (a->delegated_cred());

  werror ("\n");

  QTEST (itok = i->init (atok));
  TEST (i);
  TEST (GSSAPI.describe_services (i->required_services()));
  TEST (i->is_established());
  TEST (GSSAPI.describe_services (i->services()));
  TEST (i->locally_initiated());
  TEST (i->source_name());
  TEST (i->target_name());
  TEST (i->lifetime());
  TEST (i->mech());

  werror ("\n");

  QTEST (itok = i->export());
  TEST (i);
  TEST (GSSAPI.describe_services (i->required_services()));
  TEST (i->is_established());
  TEST (GSSAPI.describe_services (i->services()));
  TEST (i->locally_initiated());
  TEST (i->source_name());
  TEST (i->target_name());
  TEST (i->lifetime());
  TEST (i->mech());

  werror ("\n");

  TEST (i = GSSAPI.Context (itok, 0xfff));
  TEST (i = GSSAPI.Context (itok));
  TEST (GSSAPI.describe_services (i->required_services()));
  TEST (i->is_established());
  TEST (GSSAPI.describe_services (i->services()));
  TEST (i->locally_initiated());
  TEST (i->source_name());
  TEST (i->target_name());
  TEST (i->lifetime());
  TEST (i->mech());

  werror ("\n");

  TEST (i->last_major_status());
  TEST (i->last_minor_status());
  TEST (i->last_qop());
  TEST (i->last_confidential());
  TEST (i->process_token ("hej"));
  TEST (GSSAPI.major_status_messages (i->last_major_status()));
  TEST (GSSAPI.minor_status_messages (i->last_minor_status(), i->mech()));

  werror ("\n");

  TEST (itok = i->get_mic ("signed to acceptor", 17));
  TEST (itok = i->get_mic ("signed to acceptor"));
  TEST (a->verify_mic ("signed to acceptor ", itok));
  TEST (GSSAPI.major_status_messages (a->last_major_status()));
  TEST (a->last_minor_status());
  TEST (a->verify_mic ("signed to acceptor", itok));
  TEST (a->last_major_status());
  TEST (a->last_minor_status());
  TEST (a->last_qop());
  TEST (a->verify_mic ("signed to acceptor", itok));
  TEST (a->required_services (GSSAPI.REPLAY_FLAG));
  TEST (a->verify_mic ("signed to acceptor", itok));
  TEST (GSSAPI.major_status_messages (a->last_major_status()));
  TEST (a->last_minor_status());

  werror ("\n");

  TEST (a->wrap_size_limit (1000, 1));
  TEST (a->wrap_size_limit (100, 1));
  TEST (a->wrap_size_limit (50, 1));
  TEST (atok = a->wrap ("encrypted wrapped to initiator 1", 1));
  TEST (atok = a->wrap ("encrypted wrapped to initiator 2", 1));
  TEST (sizeof (atok));
  TEST (i->required_services (GSSAPI.REPLAY_FLAG));
  TEST (i->unwrap (atok));
  TEST (GSSAPI.major_status_messages (i->last_major_status()));
  TEST (i->last_minor_status());
  TEST (i->last_qop());
  TEST (i->last_confidential());

  werror ("\n");

  TEST (atok = a->wrap ("cleartext wrapped to initiator 3"));
  TEST (atok = a->wrap ("cleartext wrapped to initiator 4"));
  TEST (i->required_services (GSSAPI.SEQUENCE_FLAG));
  TEST (i->unwrap (atok));
  TEST (GSSAPI.major_status_messages (i->last_major_status()));
  TEST (i->last_minor_status());
  TEST (i->last_qop());
  TEST (i->last_confidential());

  werror ("\n");

  TEST (a->required_services (GSSAPI.PROT_READY_FLAG));
  TEST (a->required_services (GSSAPI.ANON_FLAG));
  TEST (a);
  TEST (GSSAPI.describe_services (a->required_services()));
  TEST (a->is_established());
  TEST (GSSAPI.describe_services (a->services()));
  TEST (a->locally_initiated());
  TEST (a->source_name());
  TEST (a->target_name());
  TEST (a->lifetime());
  TEST (a->mech());
  TEST (a->delegated_cred());
}
