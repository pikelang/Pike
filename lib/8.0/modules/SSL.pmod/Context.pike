#pike 8.1

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

inherit SSL.Context;

//
// --- Compat code below
//

protected Crypto.RSA.State compat_rsa;
protected array(string(8bit)) compat_certificates;

//! The servers default private RSA key.
//! 
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`certificates], @[find_cert()]
Crypto.RSA.State `rsa()
{
  return compat_rsa;
}

//! Set the servers default private RSA key.
//!
//! Compatibility, don't use.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[`certificates=], @[add_cert()]
void `rsa=(Crypto.RSA.State k)
{
  compat_rsa = k;
  if (k && compat_certificates) {
    catch {
      add_cert(k, compat_certificates);
    };
  }
}

//! The server's certificate, or a chain of X509.v3 certificates, with
//! the server's certificate first and root certificate last.
//!
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`rsa], @[find_cert()]
array(string(8bit)) `certificates()
{
  return compat_certificates;
}

//! Set the servers certificate, or a chain of X509.v3 certificates, with
//! the servers certificate first and root certificate last.
//!
//! Compatibility, don't use.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[`rsa=], @[add_cert()]
void `certificates=(array(string(8bit)) certs)
{
  compat_certificates = certs;

  if (compat_rsa && certs) {
    catch {
      add_cert(compat_rsa, certs);
    };
  }
}

//! The clients RSA private key.
//!
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`certificates], @[find_cert()]
Crypto.RSA.State `client_rsa()
{
  return compat_rsa;
}

//! Set the clients default private RSA key.
//!
//! Compatibility, don't use.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[`client_certificates=], @[add_cert()]
void `client_rsa=(Crypto.RSA.State k)
{
  compat_rsa = k;
  if (k && compat_certificates) {
    catch {
      add_cert(k, compat_certificates);
    };
  }
}

//! The client's certificate, or a chain of X509.v3 certificates, with
//! the client's certificate first and root certificate last.
//!
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`rsa], @[find_cert()]
array(array(string(8bit))) `client_certificates()
{
  return compat_certificates && ({ compat_certificates });
}

//! Set the client's certificate, or a chain of X509.v3 certificates, with
//! the client's certificate first and root certificate last.
//!
//! Compatibility, don't use.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[`rsa=], @[add_cert()]
void `client_certificates=(array(array(string(8bit))) certs)
{
  compat_certificates = certs && (sizeof(certs)?certs[0]:({}));

  if (compat_rsa && certs) {
    foreach(certs, array(string(8bit)) chain) {
      catch {
	add_cert(compat_rsa, chain);
      };
    }
  }
}

//! Compatibility.
//! @deprecated find_cert
Crypto.DSA.State `dsa()
{
  return UNDEFINED;
}

//! Compatibility.
//! @deprecated add_cert
void `dsa=(Crypto.DSA.State k)
{
  error("The old DSA API is not supported anymore.\n");
}

//! Set @[preferred_suites] to RSA based methods.
//!
//! @param min_keylength
//!   Minimum acceptable key length in bits.
//!
//! @seealso
//!   @[dhe_dss_mode()], @[filter_weak_suites()]
//!
//! @deprecated get_suites
void rsa_mode(int(0..)|void min_keylength)
{
  SSL3_DEBUG_MSG("SSL.Context: rsa_mode()\n");
  preferred_suites = get_suites(min_keylength, 1);
}

//! Set @[preferred_suites] to DSS based methods.
//!
//! @param min_keylength
//!   Minimum acceptable key length in bits.
//!
//! @seealso
//!   @[rsa_mode()], @[filter_weak_suites()]
//!
//! @deprecated get_suites
void dhe_dss_mode(int(0..)|void min_keylength)
{
  SSL3_DEBUG_MSG("SSL.Context: dhe_dss_mode()\n");
  preferred_suites = get_suites(min_keylength, 1);
}
