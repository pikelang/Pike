#pike 8.1

inherit Protocols.HTTP.Server.SSLPort;

private string tmp_key;
private array(string) tmp_cert;

this_program `port()
{
  return this;
}

//! @deprecated add_cert
void set_key(string skey)
{
  tmp_key = skey;
  if( tmp_key && tmp_cert )
    ctx->add_cert( tmp_key, tmp_cert );
}

//! @deprecated add_cert
void set_certificate(string|array(string) certificate)
{
  if(arrayp(certificate))
    tmp_cert = [array(string)]certificate;
  else
    tmp_cert = ({ [string]certificate });
  if( tmp_key && tmp_cert )
    ctx->add_cert( tmp_key, tmp_cert );
}
