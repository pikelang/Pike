#! /usr/bin/env pike
#pike __REAL_VERSION__

// SSL Client example

import SSL.Constants;

SSL.sslfile sslfile;

void write_callback(mixed id)
{
  sslfile->set_write_callback(0);
  //  sslfile->write("GET / HTTP/1.0\r\n\r\n");
}

void read_callback(mixed id, string s)
{
  write(s);
}

int main(int argc, array(string) argv)
{
  SSL.context context = SSL.context();

  // Allow only strong crypto
  context->preferred_suites = ({
    SSL_rsa_with_idea_cbc_sha,
    SSL_rsa_with_rc4_128_sha,
    SSL_rsa_with_rc4_128_md5,
    SSL_rsa_with_3des_ede_cbc_sha,
  });

  context->random = Crypto.Random.random_string;
  Stdio.File socket = Stdio.File();
  if(!socket->connect("pike.ida.liu.se", 443))
  {
    werror("couldn't connect\n");
    exit(-1);
  }
  socket->set_nonblocking();
  sslfile = SSL.sslfile(socket, context, 1,0);

  sslfile->set_nonblocking(read_callback, write_callback, exit);

  return -17;
}
